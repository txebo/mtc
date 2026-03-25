#include "ui/MainWindow.h"

#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QScrollArea>
#include <QString>
#include <QStandardPaths>
#include <QTimer>
#include <QToolBox>
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

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *content = new QWidget(scrollArea);
    auto *rootLayout = new QVBoxLayout(content);
    rootLayout->setContentsMargins(24, 20, 24, 20);
    rootLayout->setSpacing(16);

    auto *title = new QLabel("MTC - Core Telegram bootstrap", content);
    title->setStyleSheet("font-size: 24px; font-weight: 700;");
    rootLayout->addWidget(title);

    auto *subtitle = new QLabel(
        "Base de autenticacion y estados de autorizacion para la Etapa 1. "
        "Si TDLib no esta instalada, la UI sigue permitiendo validar el flujo.",
        content);
    subtitle->setWordWrap(true);
    rootLayout->addWidget(subtitle);

    auto *toolsMenu = menuBar()->addMenu("Herramientas");
    auto *viewFlowLogAction = toolsMenu->addAction("Ver Log de Flujo");
    connect(viewFlowLogAction, &QAction::triggered, this, [this]() {
        auto *dialog = new QDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowTitle("Log de Flujo TDLib");
        dialog->resize(920, 620);

        auto *layout = new QVBoxLayout(dialog);
        layout->setContentsMargins(12, 12, 12, 12);
        layout->setSpacing(10);

        const QString dataRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        const QString logPath = dataRoot + "/tdlib_auth_flow.log";

        auto *pathLabel = new QLabel(QString("Archivo: %1").arg(logPath), dialog);
        pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        pathLabel->setCursor(Qt::IBeamCursor);
        layout->addWidget(pathLabel);

        auto *logView = new QPlainTextEdit(dialog);
        logView->setReadOnly(true);
        logView->setLineWrapMode(QPlainTextEdit::NoWrap);
        layout->addWidget(logView, 1);

        auto reloadLog = [logView, logPath]() {
            QFile file(logPath);
            if (!QFileInfo::exists(logPath)) {
                logView->setPlainText("Aun no existe log de flujo para esta sesion.");
                return;
            }
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                logView->setPlainText("No se pudo abrir el log de flujo.");
                return;
            }
            logView->setPlainText(QString::fromUtf8(file.readAll()));
            logView->moveCursor(QTextCursor::End);
        };
        reloadLog();

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
        auto *reloadButton = buttons->addButton("Recargar", QDialogButtonBox::ActionRole);
        connect(reloadButton, &QPushButton::clicked, dialog, reloadLog);
        connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::close);
        layout->addWidget(buttons);

        dialog->show();
    });

    auto *workspaceLayout = new QHBoxLayout();
    workspaceLayout->setSpacing(18);
    rootLayout->addLayout(workspaceLayout, 1);

    auto *leftColumn = new QVBoxLayout();
    leftColumn->setSpacing(14);
    workspaceLayout->addLayout(leftColumn, 5);

    auto *rightColumn = new QVBoxLayout();
    rightColumn->setSpacing(14);
    workspaceLayout->addLayout(rightColumn, 4);

    auto *loginBox = new QGroupBox("Panel de sesion", content);
    auto *loginLayout = new QVBoxLayout(loginBox);
    loginLayout->setContentsMargins(14, 16, 14, 14);
    loginLayout->setSpacing(12);

    auto *panelIntro = new QLabel(
        "Usa este panel como asistente: prepara credenciales, valida la cuenta y controla la sesion actual desde un solo lugar.",
        loginBox);
    panelIntro->setWordWrap(true);
    loginLayout->addWidget(panelIntro);

    auto *stepLabel = buildInfoPanel(
        "font-weight: 700; background: #f4f7fb; border: 1px solid #d7e0ea; border-radius: 8px;",
        loginBox);
    auto *stepGuideLabel = buildInfoPanel(
        "color: #4f5d6b; background: #fafbfc; border: 1px solid #dfe5eb; border-radius: 8px;",
        loginBox);
    stepGuideLabel->setMinimumHeight(156);
    stepGuideLabel->setText(
        "Fases de validacion:\n"
        "Fase 1. Preparar credenciales y perfiles.\n"
        "Fase 2. Enviar telefono.\n"
        "Fase 3. Validar codigo.\n"
        "Fase 4. Validar contrasena o confirmacion externa.\n"
        "Fase 5. Revisar sesion, chats y controles finales.");

    auto *phaseSelector = new QComboBox(loginBox);
    phaseSelector->setMinimumHeight(34);

    auto *phaseChecklistLabel = buildInfoPanel(
        "background: #f9fafb; border: 1px solid #d9dee5; border-radius: 8px;",
        loginBox);
    phaseChecklistLabel->setMinimumHeight(156);

    loginLayout->addWidget(stepLabel);
    loginLayout->addWidget(stepGuideLabel);
    loginLayout->addWidget(phaseSelector);
    loginLayout->addWidget(phaseChecklistLabel);

    auto *accountBox = new QGroupBox("Preparar cuenta", loginBox);
    auto *accountLayout = new QFormLayout(accountBox);
    accountLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    accountLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    accountLayout->setFormAlignment(Qt::AlignTop);
    accountLayout->setHorizontalSpacing(14);
    accountLayout->setVerticalSpacing(10);

    auto *profileNameInput = new QLineEdit(accountBox);
    profileNameInput->setPlaceholderText("Cuenta principal");
    accountLayout->addRow("Alias", profileNameInput);

    auto *apiIdInput = new QLineEdit(accountBox);
    apiIdInput->setPlaceholderText("api_id");
    apiIdInput->setText(QProcessEnvironment::systemEnvironment().value("MTC_TDLIB_API_ID"));
    accountLayout->addRow("API ID", apiIdInput);

    auto *apiHashInput = new QLineEdit(accountBox);
    apiHashInput->setPlaceholderText("api_hash");
    apiHashInput->setText(QProcessEnvironment::systemEnvironment().value("MTC_TDLIB_API_HASH"));
    accountLayout->addRow("API Hash", apiHashInput);
    apiHashInput->setEchoMode(QLineEdit::Password);

    auto *phoneInput = new QLineEdit(accountBox);
    phoneInput->setPlaceholderText("+52...");
    phoneInput->setText(QProcessEnvironment::systemEnvironment().value("MTC_TDLIB_PHONE"));
    accountLayout->addRow("Telefono", phoneInput);

    auto *sessionActionsLayout = new QHBoxLayout();
    sessionActionsLayout->setSpacing(10);
    auto *submitButton = new QPushButton("Continuar", accountBox);
    auto *saveProfileButton = new QPushButton("Guardar perfil", accountBox);
    submitButton->setMinimumHeight(34);
    saveProfileButton->setMinimumHeight(34);
    sessionActionsLayout->addWidget(submitButton);
    sessionActionsLayout->addWidget(saveProfileButton);
    accountLayout->addRow("Acciones", sessionActionsLayout);

    auto *profilesBox = new QGroupBox("Perfiles locales", loginBox);
    auto *profilesLayout = new QVBoxLayout(profilesBox);
    profilesLayout->setContentsMargins(14, 16, 14, 14);
    profilesLayout->setSpacing(10);
    auto *profilesHint = new QLabel(
        "Selecciona un perfil para preparar automaticamente la sesion y continuar el flujo por estado.",
        profilesBox);
    profilesHint->setWordWrap(true);
    profilesLayout->addWidget(profilesHint);

    auto *profilesList = new QListWidget(profilesBox);
    profilesList->setMinimumHeight(180);
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

    auto *authBox = new QGroupBox("Validar acceso", loginBox);
    auto *authLayout = new QFormLayout(authBox);
    authLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    authLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    authLayout->setFormAlignment(Qt::AlignTop);
    authLayout->setHorizontalSpacing(14);
    authLayout->setVerticalSpacing(10);

    auto *codeInput = new QLineEdit(authBox);
    codeInput->setPlaceholderText("12345");
    codeInput->setText(QProcessEnvironment::systemEnvironment().value("MTC_TDLIB_CODE"));
    authLayout->addRow("Codigo", codeInput);

    auto *passwordInput = new QLineEdit(authBox);
    passwordInput->setPlaceholderText("Contrasena 2FA");
    passwordInput->setEchoMode(QLineEdit::Password);
    authLayout->addRow("Contrasena", passwordInput);

    auto *authActionsLayout = new QHBoxLayout();
    authActionsLayout->setSpacing(10);
    auto *submitCodeButton = new QPushButton("Enviar codigo", authBox);
    auto *submitPasswordButton = new QPushButton("Enviar contrasena", authBox);
    submitCodeButton->setMinimumHeight(34);
    submitPasswordButton->setMinimumHeight(34);
    authActionsLayout->addWidget(submitCodeButton);
    authActionsLayout->addWidget(submitPasswordButton);
    authLayout->addRow("Acciones", authActionsLayout);

    auto *sessionStepsToolbox = new QToolBox(loginBox);
    sessionStepsToolbox->addItem(profilesBox, "1) Perfiles");
    sessionStepsToolbox->addItem(accountBox, "2) Preparar cuenta");
    sessionStepsToolbox->addItem(authBox, "3) Validar acceso");
    sessionStepsToolbox->setCurrentIndex(0);
    loginLayout->addWidget(sessionStepsToolbox);

    auto *sessionBox = new QGroupBox("Estado y control de sesion", loginBox);
    auto *sessionBoxLayout = new QFormLayout(sessionBox);
    sessionBoxLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    sessionBoxLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignTop);
    sessionBoxLayout->setFormAlignment(Qt::AlignTop);
    sessionBoxLayout->setHorizontalSpacing(14);
    sessionBoxLayout->setVerticalSpacing(10);

    auto *sessionControlLayout = new QHBoxLayout();
    sessionControlLayout->setSpacing(10);
    auto *logoutButton = new QPushButton("Cerrar sesion", sessionBox);
    auto *resetSessionButton = new QPushButton("Reiniciar sesion", sessionBox);
    logoutButton->setMinimumHeight(34);
    resetSessionButton->setMinimumHeight(34);
    sessionControlLayout->addWidget(logoutButton);
    sessionControlLayout->addWidget(resetSessionButton);
    sessionBoxLayout->addRow("Control", sessionControlLayout);

    auto *authStateLabel = buildInfoPanel(
        "font-weight: 600; background: #f6f6f6; border: 1px solid #dddddd; border-radius: 8px;",
        sessionBox);
    sessionBoxLayout->addRow("Estado", authStateLabel);

    auto *diagnosticLabel = buildInfoPanel(
        "background: #fff8e8; border: 1px solid #ecd9a5; border-radius: 8px;",
        sessionBox);
    diagnosticLabel->setMinimumHeight(92);
    diagnosticLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    diagnosticLabel->setCursor(Qt::IBeamCursor);
    sessionBoxLayout->addRow("Diagnostico", diagnosticLabel);
    loginLayout->addWidget(sessionBox);

    leftColumn->addWidget(loginBox);

    auto *notesBox = new QGroupBox("Siguiente integracion real", content);
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

    auto *separator = new QFrame(content);
    separator->setFrameShape(QFrame::HLine);
    rootLayout->addWidget(separator);

    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(18);
    rootLayout->addLayout(bottomLayout, 1);

    auto *telegramDataBox = new QGroupBox("Lectura desde TDLib", content);
    auto *telegramDataLayout = new QVBoxLayout(telegramDataBox);
    telegramDataLayout->setContentsMargins(14, 16, 14, 14);
    telegramDataLayout->setSpacing(10);
    auto *accountLabel = new QLabel("Cuenta: pendiente", telegramDataBox);
    accountLabel->setWordWrap(true);
    telegramDataLayout->addWidget(accountLabel);
    auto *chatList = new QListWidget(telegramDataBox);
    chatList->setMinimumHeight(140);
    telegramDataLayout->addWidget(chatList);

    auto *messagesList = new QListWidget(telegramDataBox);
    messagesList->setMinimumHeight(180);
    telegramDataLayout->addWidget(messagesList);

    auto *messageComposerLayout = new QHBoxLayout();
    messageComposerLayout->setSpacing(10);
    auto *messageInput = new QLineEdit(telegramDataBox);
    messageInput->setPlaceholderText("Escribe un mensaje...");
    auto *sendMessageButton = new QPushButton("Responder", telegramDataBox);
    sendMessageButton->setMinimumHeight(34);
    messageComposerLayout->addWidget(messageInput, 1);
    messageComposerLayout->addWidget(sendMessageButton);
    telegramDataLayout->addLayout(messageComposerLayout);

    bottomLayout->addWidget(telegramDataBox, 3);

    auto *modulesBox = new QGroupBox("Estado de modulos", content);
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
                currentStep = "Fase 1 de 5: preparar credenciales y perfiles";
                guideText =
                    "Te toca probar la Fase 1:\n"
                    "1. Llena API ID y API Hash.\n"
                    "2. Verifica que Iniciar flujo se habilite.\n"
                    "3. Guarda un perfil, cargalo y eliminalo.\n"
                    "4. Inicia el flujo y confirma que el siguiente paso sea telefono.";
                break;
            case AuthorizationState::WaitingEncryptionKey:
                currentStep = "Fase 1 de 5: inicializar base local";
                guideText =
                    "TDLib esta abriendo su base local:\n"
                    "1. Espera a que avance automaticamente.\n"
                    "2. Si se detiene, usa Reiniciar sesion.\n"
                    "3. Luego continua con telefono cuando lo solicite.\n"
                    "4. Verifica que el estado cambie a telefono.";
                break;
            case AuthorizationState::WaitingPhoneNumber:
                currentStep = "Fase 2 de 5: enviar telefono";
                guideText =
                    "Te toca probar la Fase 2:\n"
                    "1. Captura o corrige el telefono.\n"
                    "2. Pulsa Continuar.\n"
                    "3. Verifica que el estado cambie a codigo.\n"
                    "4. Confirma que el panel Telegram refleje el nuevo estado.";
                break;
            case AuthorizationState::WaitingCode:
                currentStep = "Fase 3 de 5: validar codigo";
                guideText =
                    "Te toca probar la Fase 3:\n"
                    "1. Introduce el codigo recibido.\n"
                    "2. Pulsa Continuar.\n"
                    "3. Verifica si avanza a listo o a contrasena 2FA.\n"
                    "4. Revisa que el diagnostico explique el resultado.";
                break;
            case AuthorizationState::WaitingPassword:
                currentStep = "Fase 4 de 5: validar contrasena 2FA";
                guideText =
                    "Te toca probar la Fase 4:\n"
                    "1. Escribe la contrasena 2FA.\n"
                    "2. Pulsa Continuar.\n"
                    "3. Verifica que la sesion quede lista.\n"
                    "4. Si falla, confirma que el diagnostico sea claro.";
                break;
            case AuthorizationState::WaitingOtherDeviceConfirmation:
                currentStep = "Fase 4 de 5: confirmar desde otro dispositivo";
                guideText =
                    "Te toca probar la Fase 4:\n"
                    "1. Aprueba el acceso desde otro dispositivo autenticado.\n"
                    "2. Espera el cambio a sesion lista.\n"
                    "3. Confirma que cuenta y chats se carguen.\n"
                    "4. Revisa el panel Telegram y el de modulos.";
                break;
            case AuthorizationState::Ready:
                currentStep = "Fase 5 de 5: validar sesion operativa";
                guideText =
                    "Te toca probar la Fase 5:\n"
                    "1. Confirma que aparezcan cuenta y chats.\n"
                    "2. Verifica que el panel de modulos se actualice.\n"
                    "3. Prueba guardar, cargar y eliminar perfiles.\n"
                    "4. Ejecuta Cerrar sesion y luego Reiniciar sesion.";
                break;
            case AuthorizationState::MissingDependency:
                currentStep = "TDLib no disponible";
                guideText =
                    "Te toca validar la interfaz sin backend:\n"
                    "1. Revisa el layout y la distribucion de controles.\n"
                    "2. Valida guardar, cargar y eliminar perfiles.\n"
                    "3. Confirma que los botones se habiliten o deshabiliten bien.\n"
                    "4. Revisa el panel de modulos y diagnostico.";
                break;
            case AuthorizationState::ClosingSession:
                currentStep = "Cerrando sesion TDLib";
                guideText =
                    "TDLib esta cerrando la sesion anterior:\n"
                    "1. Espera confirmacion de cierre.\n"
                    "2. No inicies flujo todavia.\n"
                    "3. Al quedar no inicializado, vuelve a iniciar.\n"
                    "4. Luego continua con el telefono.";
                break;
            case AuthorizationState::Failed:
                currentStep = "Flujo interrumpido";
                guideText =
                    "Te toca corregir y repetir la fase actual:\n"
                    "1. Lee el diagnostico.\n"
                    "2. Corrige el dato del paso actual.\n"
                    "3. Repite solo la accion indicada.\n"
                    "4. Si persiste, prueba Reiniciar sesion.";
                break;
        }

        stepLabel->setText(currentStep);
        stepGuideLabel->setText(guideText);
        authStateLabel->setText(tdLibAdapter_.authorizationStateLabel());
        diagnosticLabel->setText(tdLibAdapter_.diagnosticMessage());
    };

    auto updatePhaseGuide = [this, phaseSelector, phaseChecklistLabel]() {
        const AuthorizationState state = tdLibAdapter_.authorizationState();

        const auto recommendedPhase = [state]() -> int {
            switch (state) {
                case AuthorizationState::NotInitialized:
                case AuthorizationState::WaitingParameters:
                case AuthorizationState::WaitingEncryptionKey:
                case AuthorizationState::MissingDependency:
                case AuthorizationState::ClosingSession:
                case AuthorizationState::Failed:
                    return 0;
                case AuthorizationState::WaitingPhoneNumber:
                    return 1;
                case AuthorizationState::WaitingCode:
                    return 2;
                case AuthorizationState::WaitingPassword:
                case AuthorizationState::WaitingOtherDeviceConfirmation:
                    return 3;
                case AuthorizationState::Ready:
                    return 4;
            }

            return 0;
        }();

        const auto phaseStatus = [state](int phaseIndex) -> QString {
            const int currentPhase = [&]() -> int {
                switch (state) {
                    case AuthorizationState::NotInitialized:
                    case AuthorizationState::WaitingParameters:
                    case AuthorizationState::WaitingEncryptionKey:
                    case AuthorizationState::MissingDependency:
                    case AuthorizationState::ClosingSession:
                    case AuthorizationState::Failed:
                        return 0;
                    case AuthorizationState::WaitingPhoneNumber:
                        return 1;
                    case AuthorizationState::WaitingCode:
                        return 2;
                    case AuthorizationState::WaitingPassword:
                    case AuthorizationState::WaitingOtherDeviceConfirmation:
                        return 3;
                    case AuthorizationState::Ready:
                        return 4;
                }

                return 0;
            }();

            if (state == AuthorizationState::Failed && phaseIndex == currentPhase) {
                return "error";
            }
            if (phaseIndex < currentPhase) {
                return "realizado";
            }
            if (phaseIndex == currentPhase) {
                return state == AuthorizationState::Ready && phaseIndex == 4 ? "realizado" : "en proceso";
            }
            return "pendiente";
        };

        const QStringList phaseNames = {
            "Fase 1 - Preparar credenciales y perfiles",
            "Fase 2 - Enviar telefono",
            "Fase 3 - Validar codigo",
            "Fase 4 - Validar contrasena o confirmacion",
            "Fase 5 - Revisar sesion operativa",
            "Fase 6 - Multi-cuenta real",
            "Fase 7 - Mensajeria y archivos",
            "Fase 8 - Llamadas y dispositivos",
            "Fase 9 - Analitica y auditoria",
        };

        const int previousSelection = phaseSelector->currentIndex();
        phaseSelector->blockSignals(true);
        phaseSelector->clear();
        for (int index = 0; index < phaseNames.size(); ++index) {
            phaseSelector->addItem(QString("%1 [%2]").arg(phaseNames[index], phaseStatus(index)));
        }
        phaseSelector->setCurrentIndex(previousSelection >= 0 ? previousSelection : recommendedPhase);
        if (phaseSelector->currentIndex() < 0) {
            phaseSelector->setCurrentIndex(recommendedPhase);
        }
        phaseSelector->blockSignals(false);

        const int selectedPhase = phaseSelector->currentIndex() >= 0 ? phaseSelector->currentIndex() : recommendedPhase;
        QString checklist;
        switch (selectedPhase) {
            case 0:
                checklist =
                    "Pruebas de la Fase 1:\n"
                    "1. Llena API ID y API Hash.\n"
                    "2. Verifica que Iniciar flujo se habilite.\n"
                    "3. Guarda un perfil.\n"
                    "4. Carga el perfil guardado.\n"
                    "5. Elimina el perfil y confirma que desaparezca.";
                break;
            case 1:
                checklist =
                    "Pruebas de la Fase 2:\n"
                    "1. Escribe o corrige el telefono.\n"
                    "2. Pulsa Enviar telefono.\n"
                    "3. Verifica que el estado cambie a codigo.\n"
                    "4. Revisa que el panel Telegram refleje el avance.";
                break;
            case 2:
                checklist =
                    "Pruebas de la Fase 3:\n"
                    "1. Introduce el codigo recibido.\n"
                    "2. Pulsa Enviar codigo.\n"
                    "3. Verifica si avanza a listo o a Fase 4.\n"
                    "4. Confirma que el diagnostico explique el resultado.";
                break;
            case 3:
                checklist =
                    "Pruebas de la Fase 4:\n"
                    "1. Si aplica 2FA, escribe la contrasena y enviala.\n"
                    "2. Si aplica confirmacion externa, apruebala desde otro dispositivo.\n"
                    "3. Verifica que el flujo avance a sesion lista.\n"
                    "4. Revisa diagnostico y panel Telegram.";
                break;
            case 4:
                checklist =
                    "Pruebas de la Fase 5:\n"
                    "1. Confirma que aparezcan cuenta y chats.\n"
                    "2. Verifica que el panel de modulos se actualice.\n"
                    "3. Prueba Cerrar sesion.\n"
                    "4. Prueba Reiniciar sesion.\n"
                    "5. Confirma que la UI siga consistente tras esos cambios.";
                break;
            case 5:
                checklist =
                    "Fase futura 6 - Multi-cuenta real:\n"
                    "1. Persistir varias sesiones activas.\n"
                    "2. Cambiar de cuenta sin rehacer login completo.\n"
                    "3. Reflejar sesion activa en UI y paneles.\n"
                    "Estado actual: pendiente.";
                break;
            case 6:
                checklist =
                    "Fase futura 7 - Mensajeria y archivos:\n"
                    "1. Cargar chats principales con mas detalle.\n"
                    "2. Enviar texto.\n"
                    "3. Adjuntar imagen, PDF y video.\n"
                    "Estado actual: pendiente.";
                break;
            case 7:
                checklist =
                    "Fase futura 8 - Llamadas y dispositivos:\n"
                    "1. Integrar llamadas 1 a 1 y group call.\n"
                    "2. Detectar y seleccionar dispositivos.\n"
                    "3. Manejar llamada entrante durante llamada activa.\n"
                    "Estado actual: pendiente.";
                break;
            case 8:
                checklist =
                    "Fase futura 9 - Analitica y auditoria:\n"
                    "1. Persistir eventos en SQLite.\n"
                    "2. Separar base analitica de la operativa.\n"
                    "3. Mostrar auditoria basica.\n"
                    "Estado actual: pendiente.";
                break;
            default:
                break;
        }

        phaseChecklistLabel->setText(checklist);
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
            const bool waitingCode = state == AuthorizationState::WaitingCode;
            const bool waitingPassword = state == AuthorizationState::WaitingPassword;
            bool canContinue = false;
            QString continueLabel = "Continuar";

            switch (state) {
                case AuthorizationState::NotInitialized:
                case AuthorizationState::WaitingParameters:
                case AuthorizationState::Failed:
                    continueLabel = "Iniciar flujo";
                    canContinue = tdLibReadyForActions && hasApiCredentials;
                    break;
                case AuthorizationState::WaitingPhoneNumber:
                    continueLabel = "Enviar telefono";
                    canContinue = tdLibReadyForActions && hasApiCredentials && hasPhoneNumber;
                    break;
                case AuthorizationState::WaitingCode:
                    continueLabel = "Enviar codigo";
                    canContinue = tdLibReadyForActions && hasCode;
                    break;
                case AuthorizationState::WaitingPassword:
                    continueLabel = "Enviar contrasena";
                    canContinue = tdLibReadyForActions && hasPassword;
                    break;
                case AuthorizationState::ClosingSession:
                    continueLabel = "Cerrando sesion...";
                    break;
                case AuthorizationState::WaitingEncryptionKey:
                    continueLabel = "Inicializando...";
                    break;
                case AuthorizationState::WaitingOtherDeviceConfirmation:
                    continueLabel = "Esperando confirmacion...";
                    break;
                case AuthorizationState::Ready:
                    continueLabel = "Sesion activa";
                    break;
                case AuthorizationState::MissingDependency:
                    continueLabel = "TDLib ausente";
                    break;
            }

            apiIdInput->setEnabled(!sessionActive);
            apiHashInput->setEnabled(!sessionActive);
            phoneInput->setEnabled(!sessionActive);
            codeInput->setEnabled(waitingCode);
            passwordInput->setEnabled(waitingPassword);

            submitButton->setText(continueLabel);
            submitButton->setEnabled(canContinue);
            saveProfileButton->setEnabled(hasApiCredentials || hasPhoneNumber);
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

    auto updateTelegramDataUi = [accountLabel, chatList, messagesList, this]() {
        const QString selfDisplayName = tdLibAdapter_.selfDisplayName();
        accountLabel->setText(selfDisplayName.isEmpty()
                                  ? "Cuenta: pendiente"
                                  : QString("Cuenta: %1").arg(selfDisplayName));

        const QString selectedChatId = tdLibAdapter_.selectedChatId();
        const auto chatEntries = tdLibAdapter_.chatEntries();
        chatList->blockSignals(true);
        chatList->clear();
        if (chatEntries.isEmpty()) {
            chatList->addItem("Sin chats cargados todavia.");
            chatList->setEnabled(false);
            chatList->blockSignals(false);
            messagesList->clear();
            messagesList->addItem("Selecciona un chat para leer mensajes.");
            return;
        }

        chatList->setEnabled(true);
        int selectedRow = -1;
        for (int index = 0; index < chatEntries.size(); ++index) {
            const auto &entry = chatEntries[index];
            auto *item = new QListWidgetItem(entry.second, chatList);
            item->setData(Qt::UserRole, entry.first);
            if (!selectedChatId.isEmpty() && entry.first == selectedChatId) {
                selectedRow = index;
            }
        }
        if (selectedRow >= 0) {
            chatList->setCurrentRow(selectedRow);
        }
        chatList->blockSignals(false);

        messagesList->clear();
        const QStringList messages = tdLibAdapter_.selectedChatMessages();
        if (messages.isEmpty()) {
            messagesList->addItem("No hay mensajes cargados para este chat.");
            return;
        }

        for (const QString &line : messages) {
            messagesList->addItem(line);
        }
        messagesList->scrollToBottom();
    };

    auto updateMessagingControls = [this, messageInput, sendMessageButton]() {
        const bool canSend = tdLibAdapter_.authorizationState() == AuthorizationState::Ready
                             && !tdLibAdapter_.selectedChatId().isEmpty()
                             && !messageInput->text().trimmed().isEmpty();
        sendMessageButton->setEnabled(canSend);
    };

    updateTelegramUi();
    updateTelegramDataUi();
    updateMessagingControls();
    refreshProfilesUi();
    updateAuthControls();
    updatePhaseGuide();
    updateModuleStatus();

    connect(submitButton, &QPushButton::clicked, this, [this, apiIdInput, apiHashInput, phoneInput, codeInput, passwordInput]() {
        const AuthorizationState state = tdLibAdapter_.authorizationState();
        if (state == AuthorizationState::WaitingCode) {
            tdLibAdapter_.submitAuthenticationCode(codeInput->text());
            return;
        }

        if (state == AuthorizationState::WaitingPassword) {
            tdLibAdapter_.submitAuthenticationPassword(passwordInput->text());
            return;
        }

        tdLibAdapter_.continueAuthorization(apiIdInput->text(),
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
            [this, profilesList, profileNameInput, apiIdInput, apiHashInput, phoneInput, codeInput, passwordInput]() {
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
                codeInput->clear();
                passwordInput->clear();

                tdLibAdapter_.continueAuthorization(apiIdInput->text(),
                                                    apiHashInput->text(),
                                                    phoneInput->text());
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

    connect(chatList, &QListWidget::itemSelectionChanged, this, [this, chatList, messageInput, updateMessagingControls]() {
        QListWidgetItem *item = chatList->currentItem();
        if (item == nullptr) {
            updateMessagingControls();
            return;
        }

        const QString chatId = item->data(Qt::UserRole).toString();
        if (!chatId.isEmpty()) {
            tdLibAdapter_.requestChatHistory(chatId);
        }
        messageInput->setFocus();
        updateMessagingControls();
    });

    connect(sendMessageButton, &QPushButton::clicked, this, [this, messageInput, updateMessagingControls]() {
        const QString text = messageInput->text().trimmed();
        if (text.isEmpty()) {
            return;
        }
        tdLibAdapter_.sendTextMessage(tdLibAdapter_.selectedChatId(), text);
        messageInput->clear();
        updateMessagingControls();
    });

    connect(&tdLibAdapter_, &TDLibAdapter::stateChanged, this, updateTelegramUi);
    connect(&tdLibAdapter_, &TDLibAdapter::stateChanged, this, updateAuthControls);
    connect(&tdLibAdapter_, &TDLibAdapter::stateChanged, this, updatePhaseGuide);
    connect(&tdLibAdapter_, &TDLibAdapter::stateChanged, this, updateModuleStatus);
    connect(&tdLibAdapter_, &TDLibAdapter::stateChanged, this, updateMessagingControls);
    connect(&tdLibAdapter_, &TDLibAdapter::dataChanged, this, updateTelegramDataUi);
    connect(&tdLibAdapter_, &TDLibAdapter::dataChanged, this, updateModuleStatus);
    connect(&tdLibAdapter_, &TDLibAdapter::dataChanged, this, updateMessagingControls);
    connect(phaseSelector, &QComboBox::currentIndexChanged, this, updatePhaseGuide);
    connect(apiIdInput, &QLineEdit::textChanged, this, updateAuthControls);
    connect(apiHashInput, &QLineEdit::textChanged, this, updateAuthControls);
    connect(phoneInput, &QLineEdit::textChanged, this, updateAuthControls);
    connect(codeInput, &QLineEdit::textChanged, this, updateAuthControls);
    connect(passwordInput, &QLineEdit::textChanged, this, updateAuthControls);
    connect(messageInput, &QLineEdit::textChanged, this, updateMessagingControls);
    scrollArea->setWidget(content);
    setCentralWidget(scrollArea);
}

}  // namespace mtc
