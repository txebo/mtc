#include "ui/MainWindow.h"

#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "core/analytics/AnalyticsStore.h"
#include "core/calls/CallController.h"
#include "core/devices/DeviceManager.h"
#include "core/session/SessionManager.h"
#include "core/telegram/TDLibAdapter.h"

namespace mtc {

namespace {

QGroupBox *buildModuleBox(const QString &title, const QString &body, QWidget *parent) {
    auto *box = new QGroupBox(title, parent);
    auto *layout = new QVBoxLayout(box);

    auto *label = new QLabel(body, box);
    label->setWordWrap(true);
    layout->addWidget(label);

    return box;
}

}  // namespace

MainWindow::MainWindow(SessionManager &sessionManager,
                       TDLibAdapter &tdLibAdapter,
                       CallController &callController,
                       DeviceManager &deviceManager,
                       AnalyticsStore &analyticsStore,
                       QWidget *parent)
    : QMainWindow(parent),
      sessionManager_(sessionManager),
      tdLibAdapter_(tdLibAdapter),
      callController_(callController),
      deviceManager_(deviceManager),
      analyticsStore_(analyticsStore) {
    setWindowTitle("MTC");
    resize(1220, 760);

    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);

    auto *title = new QLabel("MTC - Core Telegram bootstrap", central);
    title->setStyleSheet("font-size: 24px; font-weight: 700;");
    rootLayout->addWidget(title);

    auto *subtitle = new QLabel(
        "Base de autenticacion y estados de autorizacion para la Etapa 1. "
        "Si TDLib no esta instalada, la UI sigue permitiendo validar el flujo.",
        central);
    subtitle->setWordWrap(true);
    rootLayout->addWidget(subtitle);

    auto *topGrid = new QGridLayout();
    rootLayout->addLayout(topGrid);

    auto *loginBox = new QGroupBox("Bootstrap de TDLib", central);
    auto *loginLayout = new QFormLayout(loginBox);

    auto *apiIdInput = new QLineEdit(loginBox);
    apiIdInput->setPlaceholderText("api_id");
    apiIdInput->setText(QProcessEnvironment::systemEnvironment().value("MTC_TDLIB_API_ID"));
    loginLayout->addRow("API ID", apiIdInput);

    auto *apiHashInput = new QLineEdit(loginBox);
    apiHashInput->setPlaceholderText("api_hash");
    apiHashInput->setText(QProcessEnvironment::systemEnvironment().value("MTC_TDLIB_API_HASH"));
    loginLayout->addRow("API Hash", apiHashInput);
    apiHashInput->setEchoMode(QLineEdit::Password);

    auto *phoneInput = new QLineEdit(loginBox);
    phoneInput->setPlaceholderText("+52...");
    phoneInput->setText(QProcessEnvironment::systemEnvironment().value("MTC_TDLIB_PHONE"));
    loginLayout->addRow("Telefono", phoneInput);

    auto *submitButton = new QPushButton("Iniciar flujo", loginBox);
    loginLayout->addRow(submitButton);

    auto *codeInput = new QLineEdit(loginBox);
    codeInput->setPlaceholderText("12345");
    codeInput->setText(QProcessEnvironment::systemEnvironment().value("MTC_TDLIB_CODE"));
    loginLayout->addRow("Codigo", codeInput);

    auto *submitCodeButton = new QPushButton("Enviar codigo", loginBox);
    loginLayout->addRow(submitCodeButton);

    auto *authStateLabel = new QLabel(loginBox);
    authStateLabel->setWordWrap(true);
    loginLayout->addRow("Estado", authStateLabel);

    auto *diagnosticLabel = new QLabel(loginBox);
    diagnosticLabel->setWordWrap(true);
    loginLayout->addRow("Diagnostico", diagnosticLabel);

    topGrid->addWidget(loginBox, 0, 0);

    auto *notesBox = new QGroupBox("Siguiente integracion real", central);
    auto *notesLayout = new QVBoxLayout(notesBox);
    auto *notes = new QLabel(
        "1. detectar TDLib por CMake\n"
        "2. enviar setTdlibParameters\n"
        "3. manejar authorizationState* y updates\n"
        "4. leer perfil y chats principales",
        notesBox);
    notes->setWordWrap(true);
    notesLayout->addWidget(notes);
    topGrid->addWidget(notesBox, 0, 1);

    auto *separator = new QFrame(central);
    separator->setFrameShape(QFrame::HLine);
    rootLayout->addWidget(separator);

    auto *telegramDataBox = new QGroupBox("Lectura desde TDLib", central);
    auto *telegramDataLayout = new QVBoxLayout(telegramDataBox);
    auto *accountLabel = new QLabel("Cuenta: pendiente", telegramDataBox);
    accountLabel->setWordWrap(true);
    telegramDataLayout->addWidget(accountLabel);
    auto *chatList = new QListWidget(telegramDataBox);
    telegramDataLayout->addWidget(chatList);
    rootLayout->addWidget(telegramDataBox);

    auto *modulesGrid = new QGridLayout();
    rootLayout->addLayout(modulesGrid);

    modulesGrid->addWidget(buildModuleBox("Sesiones",
                                          QString::fromStdString(sessionManager_.status()),
                                          central),
                           0,
                           0);
    modulesGrid->addWidget(buildModuleBox("Telegram",
                                          QString::fromStdString(tdLibAdapter_.status()),
                                          central),
                           0,
                           1);
    modulesGrid->addWidget(buildModuleBox("Llamadas",
                                          QString::fromStdString(callController_.status()),
                                          central),
                           1,
                           0);
    modulesGrid->addWidget(buildModuleBox("Dispositivos",
                                          QString::fromStdString(deviceManager_.status()),
                                          central),
                           1,
                           1);
    modulesGrid->addWidget(buildModuleBox("Analitica",
                                          QString::fromStdString(analyticsStore_.status()),
                                          central),
                           2,
                           0,
                           1,
                           2);

    auto updateTelegramUi = [authStateLabel, diagnosticLabel, this]() {
        authStateLabel->setText(tdLibAdapter_.authorizationStateLabel());
        diagnosticLabel->setText(tdLibAdapter_.diagnosticMessage());
    };

    auto updateTelegramDataUi = [accountLabel, chatList, this]() {
        const QString selfDisplayName = tdLibAdapter_.selfDisplayName();
        accountLabel->setText(selfDisplayName.isEmpty()
                                  ? "Cuenta: pendiente"
                                  : QString("Cuenta: %1").arg(selfDisplayName));

        chatList->clear();
        const QStringList chatTitles = tdLibAdapter_.chatTitles();
        if (chatTitles.isEmpty()) {
            chatList->addItem("Sin chats cargados todavia.");
            return;
        }

        for (const QString &title : chatTitles) {
            chatList->addItem(title);
        }
    };

    updateTelegramUi();
    updateTelegramDataUi();

    connect(submitButton, &QPushButton::clicked, this, [this, apiIdInput, apiHashInput, phoneInput]() {
        tdLibAdapter_.submitBootstrap(apiIdInput->text(),
                                      apiHashInput->text(),
                                      phoneInput->text());
    });

    connect(submitCodeButton, &QPushButton::clicked, this, [this, codeInput]() {
        tdLibAdapter_.submitAuthenticationCode(codeInput->text());
    });

    connect(&tdLibAdapter_, &TDLibAdapter::stateChanged, this, updateTelegramUi);
    connect(&tdLibAdapter_, &TDLibAdapter::dataChanged, this, updateTelegramDataUi);

    if (!apiIdInput->text().isEmpty() && !apiHashInput->text().isEmpty()) {
        QTimer::singleShot(0, this, [this, apiIdInput, apiHashInput, phoneInput]() {
            tdLibAdapter_.submitBootstrap(apiIdInput->text(),
                                          apiHashInput->text(),
                                          phoneInput->text());
        });
    }

    if (!codeInput->text().isEmpty()) {
        QTimer::singleShot(250, this, [this, codeInput]() {
            tdLibAdapter_.submitAuthenticationCode(codeInput->text());
        });
    }

    setCentralWidget(central);
}

}  // namespace mtc
