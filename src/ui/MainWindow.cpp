#include "ui/MainWindow.h"

#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
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

    auto *profileNameInput = new QLineEdit(loginBox);
    profileNameInput->setPlaceholderText("Cuenta principal");
    loginLayout->addRow("Alias", profileNameInput);

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

    auto *saveProfileButton = new QPushButton("Guardar perfil", loginBox);
    loginLayout->addRow(saveProfileButton);

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

    auto *profilesBox = new QGroupBox("Perfiles locales", central);
    auto *profilesLayout = new QVBoxLayout(profilesBox);
    auto *profilesHint = new QLabel(
        "Guarda credenciales base por cuenta para retomar el bootstrap sin depender de variables de entorno.",
        profilesBox);
    profilesHint->setWordWrap(true);
    profilesLayout->addWidget(profilesHint);

    auto *profilesList = new QListWidget(profilesBox);
    profilesLayout->addWidget(profilesList);

    auto *profilesActionsLayout = new QHBoxLayout();
    auto *loadProfileButton = new QPushButton("Cargar perfil", profilesBox);
    auto *removeProfileButton = new QPushButton("Eliminar perfil", profilesBox);
    profilesActionsLayout->addWidget(loadProfileButton);
    profilesActionsLayout->addWidget(removeProfileButton);
    profilesLayout->addLayout(profilesActionsLayout);

    auto *profilesStatusLabel = new QLabel(profilesBox);
    profilesStatusLabel->setWordWrap(true);
    profilesLayout->addWidget(profilesStatusLabel);

    topGrid->addWidget(profilesBox, 0, 1);

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
    topGrid->addWidget(notesBox, 1, 0, 1, 2);

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

    auto refreshProfilesUi =
        [profilesList, profilesStatusLabel, this]() {
            profilesList->clear();

            const QList<SessionProfile> profiles = sessionManager_.profiles();
            if (profiles.isEmpty()) {
                profilesList->addItem("No hay perfiles guardados todavia.");
                profilesList->setEnabled(false);
                profilesStatusLabel->setText("Estado: sin perfiles locales.");
                return;
            }

            profilesList->setEnabled(true);
            for (const SessionProfile &profile : profiles) {
                QString title = profile.displayName.trimmed();
                if (title.isEmpty()) {
                    title = profile.phoneNumber.trimmed();
                }
                if (title.isEmpty()) {
                    title = "Perfil sin nombre";
                }

                const QString detail = profile.phoneNumber.trimmed().isEmpty()
                                           ? "sin telefono"
                                           : profile.phoneNumber.trimmed();
                auto *item = new QListWidgetItem(QString("%1 (%2)").arg(title, detail), profilesList);
                item->setData(Qt::UserRole, profile.id);
            }

            profilesStatusLabel->setText(
                QString::fromStdString(sessionManager_.status()));
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
    refreshProfilesUi();

    connect(submitButton, &QPushButton::clicked, this, [this, apiIdInput, apiHashInput, phoneInput]() {
        tdLibAdapter_.submitBootstrap(apiIdInput->text(),
                                      apiHashInput->text(),
                                      phoneInput->text());
    });

    connect(saveProfileButton,
            &QPushButton::clicked,
            this,
            [this, profileNameInput, apiIdInput, apiHashInput, phoneInput, refreshProfilesUi]() {
                sessionManager_.saveProfile(profileNameInput->text(),
                                            apiIdInput->text(),
                                            apiHashInput->text(),
                                            phoneInput->text());
                refreshProfilesUi();
            });

    connect(submitCodeButton, &QPushButton::clicked, this, [this, codeInput]() {
        tdLibAdapter_.submitAuthenticationCode(codeInput->text());
    });

    connect(loadProfileButton,
            &QPushButton::clicked,
            this,
            [this, profilesList, profileNameInput, apiIdInput, apiHashInput, phoneInput]() {
                QListWidgetItem *item = profilesList->currentItem();
                if (item == nullptr) {
                    return;
                }

                const auto profile =
                    sessionManager_.findProfile(item->data(Qt::UserRole).toString());
                if (!profile.has_value()) {
                    return;
                }

                profileNameInput->setText(profile->displayName);
                apiIdInput->setText(profile->apiId);
                apiHashInput->setText(profile->apiHash);
                phoneInput->setText(profile->phoneNumber);
            });

    connect(removeProfileButton,
            &QPushButton::clicked,
            this,
            [this, profilesList, profileNameInput, apiIdInput, apiHashInput, phoneInput, refreshProfilesUi]() {
                QListWidgetItem *item = profilesList->currentItem();
                if (item == nullptr) {
                    return;
                }

                const QString profileId = item->data(Qt::UserRole).toString();
                if (profileId.isEmpty()) {
                    return;
                }

                sessionManager_.removeProfile(profileId);
                if (profilesList->count() == 1) {
                    profileNameInput->clear();
                    apiIdInput->clear();
                    apiHashInput->clear();
                    phoneInput->clear();
                }
                refreshProfilesUi();
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
