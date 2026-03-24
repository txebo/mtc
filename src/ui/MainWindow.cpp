#include "ui/MainWindow.h"

#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
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

QGroupBox *buildModuleBox(const QString &title, QWidget *parent, QLabel **bodyLabel) {
    auto *box = new QGroupBox(title, parent);
    auto *layout = new QVBoxLayout(box);

    auto *label = new QLabel(box);
    label->setWordWrap(true);
    layout->addWidget(label);
    if (bodyLabel != nullptr) {
        *bodyLabel = label;
    }

    return box;
}

QLabel *buildInfoPanel(const QString &styleSheet, QWidget *parent) {
    auto *label = new QLabel(parent);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    label->setMinimumHeight(64);
    label->setMargin(10);
    label->setStyleSheet(styleSheet);
    return label;
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
    resize(1280, 820);

    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(24, 20, 24, 20);
    rootLayout->setSpacing(16);

    auto *title = new QLabel("MTC - Core Telegram bootstrap", central);
    title->setStyleSheet("font-size: 24px; font-weight: 700;");
    rootLayout->addWidget(title);

    auto *subtitle = new QLabel(
        "Base de autenticacion y estados de autorizacion para la Etapa 1. "
        "Si TDLib no esta instalada, la UI sigue permitiendo validar el flujo.",
        central);
    subtitle->setWordWrap(true);
    rootLayout->addWidget(subtitle);

    auto *workspaceLayout = new QHBoxLayout();
    workspaceLayout->setSpacing(18);
    rootLayout->addLayout(workspaceLayout, 1);

    auto *leftColumn = new QVBoxLayout();
    leftColumn->setSpacing(14);
    workspaceLayout->addLayout(leftColumn, 5);

    auto *rightColumn = new QVBoxLayout();
    rightColumn->setSpacing(14);
    workspaceLayout->addLayout(rightColumn, 4);

    auto *loginBox = new QGroupBox("Bootstrap de TDLib", central);
    auto *loginLayout = new QFormLayout(loginBox);
    loginLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    loginLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    loginLayout->setFormAlignment(Qt::AlignTop);
    loginLayout->setHorizontalSpacing(14);
    loginLayout->setVerticalSpacing(10);

    auto *stepLabel = buildInfoPanel(
        "font-weight: 700; background: #f4f7fb; border: 1px solid #d7e0ea; border-radius: 8px;",
        loginBox);
    loginLayout->addRow("Paso actual", stepLabel);

    auto *stepGuideLabel = buildInfoPanel(
        "color: #4f5d6b; background: #fafbfc; border: 1px solid #dfe5eb; border-radius: 8px;",
        loginBox);
    stepGuideLabel->setMinimumHeight(72);
    stepGuideLabel->setText("1. Credenciales  2. Telefono  3. Codigo  4. Contrasena  5. Listo");
    loginLayout->addRow("Guia", stepGuideLabel);

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

    auto *sessionActionsLayout = new QHBoxLayout();
    sessionActionsLayout->setSpacing(10);
    auto *submitButton = new QPushButton("Iniciar flujo", loginBox);
    auto *saveProfileButton = new QPushButton("Guardar perfil", loginBox);
    submitButton->setMinimumHeight(34);
    saveProfileButton->setMinimumHeight(34);
    sessionActionsLayout->addWidget(submitButton);
    sessionActionsLayout->addWidget(saveProfileButton);
    loginLayout->addRow("Sesion", sessionActionsLayout);

    auto *phoneActionsLayout = new QHBoxLayout();
    phoneActionsLayout->setSpacing(10);
    auto *sendPhoneButton = new QPushButton("Enviar telefono", loginBox);
    auto *submitCodeButton = new QPushButton("Enviar codigo", loginBox);
    sendPhoneButton->setMinimumHeight(34);
    submitCodeButton->setMinimumHeight(34);
    phoneActionsLayout->addWidget(sendPhoneButton);
    phoneActionsLayout->addWidget(submitCodeButton);
    loginLayout->addRow("Validacion", phoneActionsLayout);

    auto *codeInput = new QLineEdit(loginBox);
    codeInput->setPlaceholderText("12345");
    codeInput->setText(QProcessEnvironment::systemEnvironment().value("MTC_TDLIB_CODE"));
    loginLayout->addRow("Codigo", codeInput);

    auto *passwordInput = new QLineEdit(loginBox);
    passwordInput->setPlaceholderText("Contrasena 2FA");
    passwordInput->setEchoMode(QLineEdit::Password);
    loginLayout->addRow("Contrasena", passwordInput);

    auto *securityActionsLayout = new QHBoxLayout();
    securityActionsLayout->setSpacing(10);
    auto *submitPasswordButton = new QPushButton("Enviar contrasena", loginBox);
    auto *logoutButton = new QPushButton("Cerrar sesion", loginBox);
    auto *resetSessionButton = new QPushButton("Reiniciar sesion", loginBox);
    submitPasswordButton->setMinimumHeight(34);
    logoutButton->setMinimumHeight(34);
    resetSessionButton->setMinimumHeight(34);
    securityActionsLayout->addWidget(submitPasswordButton);
    securityActionsLayout->addWidget(logoutButton);
    securityActionsLayout->addWidget(resetSessionButton);
    loginLayout->addRow("Control", securityActionsLayout);

    auto *authStateLabel = buildInfoPanel(
        "font-weight: 600; background: #f6f6f6; border: 1px solid #dddddd; border-radius: 8px;",
        loginBox);
    loginLayout->addRow("Estado", authStateLabel);

    auto *diagnosticLabel = buildInfoPanel(
        "background: #fff8e8; border: 1px solid #ecd9a5; border-radius: 8px;",
        loginBox);
    diagnosticLabel->setMinimumHeight(92);
    loginLayout->addRow("Diagnostico", diagnosticLabel);

    leftColumn->addWidget(loginBox);

    auto *profilesBox = new QGroupBox("Perfiles locales", central);
    auto *profilesLayout = new QVBoxLayout(profilesBox);
    profilesLayout->setContentsMargins(14, 16, 14, 14);
    profilesLayout->setSpacing(10);
    auto *profilesHint = new QLabel(
        "Guarda credenciales base por cuenta para retomar el bootstrap sin depender de variables de entorno.",
        profilesBox);
    profilesHint->setWordWrap(true);
    profilesLayout->addWidget(profilesHint);

    auto *profilesList = new QListWidget(profilesBox);
    profilesList->setMinimumHeight(210);
    profilesLayout->addWidget(profilesList);

    auto *profilesActionsLayout = new QHBoxLayout();
    profilesActionsLayout->setSpacing(10);
    auto *loadProfileButton = new QPushButton("Cargar perfil", profilesBox);
    auto *removeProfileButton = new QPushButton("Eliminar perfil", profilesBox);
    loadProfileButton->setMinimumHeight(34);
    removeProfileButton->setMinimumHeight(34);
    profilesActionsLayout->addWidget(loadProfileButton);
    profilesActionsLayout->addWidget(removeProfileButton);
    profilesLayout->addLayout(profilesActionsLayout);

    auto *profilesStatusLabel = new QLabel(profilesBox);
    profilesStatusLabel->setWordWrap(true);
    profilesLayout->addWidget(profilesStatusLabel);

    rightColumn->addWidget(profilesBox, 1);

    auto *notesBox = new QGroupBox("Siguiente integracion real", central);
    auto *notesLayout = new QVBoxLayout(notesBox);
    notesLayout->setContentsMargins(14, 16, 14, 14);
    auto *notes = new QLabel(
        "1. detectar TDLib por CMake\n"
        "2. enviar setTdlibParameters\n"
        "3. manejar authorizationState* y updates\n"
        "4. leer perfil y chats principales",
        notesBox);
    notes->setWordWrap(true);
    notesLayout->addWidget(notes);
    rightColumn->addWidget(notesBox);

    auto *separator = new QFrame(central);
    separator->setFrameShape(QFrame::HLine);
    rootLayout->addWidget(separator);

    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(18);
    rootLayout->addLayout(bottomLayout, 1);

    auto *telegramDataBox = new QGroupBox("Lectura desde TDLib", central);
    auto *telegramDataLayout = new QVBoxLayout(telegramDataBox);
    telegramDataLayout->setContentsMargins(14, 16, 14, 14);
    telegramDataLayout->setSpacing(10);
    auto *accountLabel = new QLabel("Cuenta: pendiente", telegramDataBox);
    accountLabel->setWordWrap(true);
    telegramDataLayout->addWidget(accountLabel);
    auto *chatList = new QListWidget(telegramDataBox);
    chatList->setMinimumHeight(220);
    telegramDataLayout->addWidget(chatList);
    bottomLayout->addWidget(telegramDataBox, 3);

    auto *modulesBox = new QGroupBox("Estado de modulos", central);
    auto *modulesGrid = new QGridLayout(modulesBox);
    modulesGrid->setContentsMargins(14, 16, 14, 14);
    modulesGrid->setHorizontalSpacing(12);
    modulesGrid->setVerticalSpacing(12);
    bottomLayout->addWidget(modulesBox, 2);

    QLabel *sessionModuleLabel = nullptr;
    QLabel *telegramModuleLabel = nullptr;
    QLabel *callModuleLabel = nullptr;
    QLabel *deviceModuleLabel = nullptr;
    QLabel *analyticsModuleLabel = nullptr;

    modulesGrid->addWidget(buildModuleBox("Sesiones", modulesBox, &sessionModuleLabel),
                           0,
                           0);
    modulesGrid->addWidget(buildModuleBox("Telegram", modulesBox, &telegramModuleLabel),
                           0,
                           1);
    modulesGrid->addWidget(buildModuleBox("Llamadas", modulesBox, &callModuleLabel),
                           1,
                           0);
    modulesGrid->addWidget(buildModuleBox("Dispositivos", modulesBox, &deviceModuleLabel),
                           1,
                           1);
    modulesGrid->addWidget(buildModuleBox("Analitica", modulesBox, &analyticsModuleLabel),
                           2,
                           0,
                           1,
                           2);

    auto updateTelegramUi = [authStateLabel, diagnosticLabel, stepLabel, stepGuideLabel, this]() {
        QString currentStep;
        QString guideText;

        switch (tdLibAdapter_.authorizationState()) {
            case AuthorizationState::NotInitialized:
            case AuthorizationState::WaitingParameters:
                currentStep = "Paso 1 de 5: completar credenciales base";
                guideText = "Ahora: credenciales  |  Siguiente: telefono  |  Luego: codigo y contrasena si aplica";
                break;
            case AuthorizationState::WaitingPhoneNumber:
                currentStep = "Paso 2 de 5: enviar telefono";
                guideText = "Completado: credenciales  |  Ahora: telefono  |  Siguiente: codigo";
                break;
            case AuthorizationState::WaitingCode:
                currentStep = "Paso 3 de 5: validar codigo";
                guideText = "Completado: credenciales y telefono  |  Ahora: codigo  |  Siguiente: acceso o 2FA";
                break;
            case AuthorizationState::WaitingPassword:
                currentStep = "Paso 4 de 5: validar contrasena 2FA";
                guideText = "Completado: credenciales, telefono y codigo  |  Ahora: contrasena 2FA";
                break;
            case AuthorizationState::WaitingOtherDeviceConfirmation:
                currentStep = "Paso 4 de 5: confirmar desde otro dispositivo";
                guideText = "Telegram esta esperando aprobacion desde una sesion ya autenticada.";
                break;
            case AuthorizationState::Ready:
                currentStep = "Paso 5 de 5: sesion lista";
                guideText = "Sesion activa. Ya puedes reutilizar la cuenta, cerrar sesion o reiniciar el flujo.";
                break;
            case AuthorizationState::MissingDependency:
                currentStep = "TDLib no disponible";
                guideText = "La interfaz sigue disponible, pero el backend real de Telegram no esta cargado.";
                break;
            case AuthorizationState::Failed:
                currentStep = "Flujo interrumpido";
                guideText = "Revisa el diagnostico y reinicia el paso correspondiente.";
                break;
        }

        stepLabel->setText(currentStep);
        stepGuideLabel->setText(guideText);
        authStateLabel->setText(tdLibAdapter_.authorizationStateLabel());
        diagnosticLabel->setText(tdLibAdapter_.diagnosticMessage());
    };

    auto updateAuthControls =
        [this,
         apiIdInput,
         apiHashInput,
         phoneInput,
         codeInput,
         passwordInput,
         submitButton,
         saveProfileButton,
         sendPhoneButton,
         submitCodeButton,
         submitPasswordButton,
         logoutButton,
         resetSessionButton]() {
            const AuthorizationState state = tdLibAdapter_.authorizationState();
            const bool hasApiCredentials =
                !apiIdInput->text().trimmed().isEmpty() && !apiHashInput->text().trimmed().isEmpty();
            const bool hasPhoneNumber = !phoneInput->text().trimmed().isEmpty();
            const bool hasCode = !codeInput->text().trimmed().isEmpty();
            const bool hasPassword = !passwordInput->text().trimmed().isEmpty();
            const bool tdLibReadyForActions = tdLibAdapter_.isTdLibAvailable();
            const bool sessionActive = state == AuthorizationState::Ready;
            const bool waitingPhone = state == AuthorizationState::WaitingPhoneNumber;
            const bool waitingCode = state == AuthorizationState::WaitingCode;
            const bool waitingPassword = state == AuthorizationState::WaitingPassword;
            const bool notInitialized = state == AuthorizationState::NotInitialized
                                        || state == AuthorizationState::WaitingParameters
                                        || state == AuthorizationState::Failed;

            apiIdInput->setEnabled(!sessionActive);
            apiHashInput->setEnabled(!sessionActive);
            phoneInput->setEnabled(!sessionActive);
            codeInput->setEnabled(waitingCode);
            passwordInput->setEnabled(waitingPassword);

            submitButton->setEnabled(tdLibReadyForActions && hasApiCredentials && !sessionActive);
            saveProfileButton->setEnabled(hasApiCredentials || hasPhoneNumber);
            sendPhoneButton->setEnabled(tdLibReadyForActions && hasApiCredentials && hasPhoneNumber
                                        && (waitingPhone || notInitialized));
            submitCodeButton->setEnabled(tdLibReadyForActions && waitingCode && hasCode);
            submitPasswordButton->setEnabled(tdLibReadyForActions && waitingPassword && hasPassword);
            logoutButton->setEnabled(tdLibReadyForActions && sessionActive);
            resetSessionButton->setEnabled(tdLibReadyForActions
                                           && state != AuthorizationState::MissingDependency);
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

    auto updateModuleStatus =
        [this, sessionModuleLabel, telegramModuleLabel, callModuleLabel, deviceModuleLabel, analyticsModuleLabel]() {
            sessionModuleLabel->setText(QString::fromStdString(sessionManager_.status()));
            telegramModuleLabel->setText(QString::fromStdString(tdLibAdapter_.status()));
            callModuleLabel->setText(QString::fromStdString(callController_.status()));
            deviceModuleLabel->setText(QString::fromStdString(deviceManager_.status()));
            analyticsModuleLabel->setText(QString::fromStdString(analyticsStore_.status()));
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
    updateAuthControls();
    updateModuleStatus();

    connect(submitButton, &QPushButton::clicked, this, [this, apiIdInput, apiHashInput, phoneInput]() {
        tdLibAdapter_.submitBootstrap(apiIdInput->text(),
                                      apiHashInput->text(),
                                      phoneInput->text());
    });

    connect(saveProfileButton,
            &QPushButton::clicked,
            this,
            [this, profileNameInput, apiIdInput, apiHashInput, phoneInput, refreshProfilesUi, updateModuleStatus]() {
                sessionManager_.saveProfile(profileNameInput->text(),
                                            apiIdInput->text(),
                                            apiHashInput->text(),
                                            phoneInput->text());
                refreshProfilesUi();
                updateModuleStatus();
            });

    connect(sendPhoneButton, &QPushButton::clicked, this, [this, phoneInput]() {
        tdLibAdapter_.submitPhoneNumber(phoneInput->text());
    });

    connect(logoutButton, &QPushButton::clicked, this, [this]() {
        tdLibAdapter_.logout();
    });

    connect(resetSessionButton, &QPushButton::clicked, this, [this]() {
        tdLibAdapter_.resetSession();
    });

    connect(submitCodeButton, &QPushButton::clicked, this, [this, codeInput]() {
        tdLibAdapter_.submitAuthenticationCode(codeInput->text());
    });

    connect(submitPasswordButton, &QPushButton::clicked, this, [this, passwordInput]() {
        tdLibAdapter_.submitAuthenticationPassword(passwordInput->text());
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
            [this, profilesList, profileNameInput, apiIdInput, apiHashInput, phoneInput, refreshProfilesUi, updateModuleStatus]() {
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
                updateModuleStatus();
            });

    connect(&tdLibAdapter_, &TDLibAdapter::stateChanged, this, updateTelegramUi);
    connect(&tdLibAdapter_, &TDLibAdapter::stateChanged, this, updateAuthControls);
    connect(&tdLibAdapter_, &TDLibAdapter::stateChanged, this, updateModuleStatus);
    connect(&tdLibAdapter_, &TDLibAdapter::dataChanged, this, updateTelegramDataUi);
    connect(&tdLibAdapter_, &TDLibAdapter::dataChanged, this, updateModuleStatus);
    connect(apiIdInput, &QLineEdit::textChanged, this, updateAuthControls);
    connect(apiHashInput, &QLineEdit::textChanged, this, updateAuthControls);
    connect(phoneInput, &QLineEdit::textChanged, this, updateAuthControls);
    connect(codeInput, &QLineEdit::textChanged, this, updateAuthControls);
    connect(passwordInput, &QLineEdit::textChanged, this, updateAuthControls);

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
