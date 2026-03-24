#include "core/telegram/TDLibAdapter.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTimer>

#include <sstream>

#if defined(MTC_HAS_TDLIB)
#include <td/telegram/td_json_client.h>
#endif

namespace mtc {

TDLibAdapter::TDLibAdapter(QObject *parent)
    : QObject(parent) {
    tdLibAvailable_ = false;
    pollTimer_ = std::make_unique<QTimer>(this);
    pollTimer_->setInterval(50);
    connect(pollTimer_.get(), &QTimer::timeout, this, &TDLibAdapter::pollResponses);
}

TDLibAdapter::~TDLibAdapter() {
#if defined(MTC_HAS_TDLIB)
    if (tdJsonClient_ != nullptr) {
        td_json_client_destroy(tdJsonClient_);
        tdJsonClient_ = nullptr;
    }
#endif
}

void TDLibAdapter::initialize() {
#if defined(MTC_HAS_TDLIB)
    if (tdJsonClient_ == nullptr) {
        tdJsonClient_ = td_json_client_create();
    }

    if (tdJsonClient_ != nullptr) {
        tdLibAvailable_ = true;
        pollTimer_->start();
        setAuthorizationState(AuthorizationState::WaitingParameters,
                              "TDLib disponible en runtime. Esperando el estado de autorizacion inicial para continuar.");
        return;
    }
#endif

    tdLibAvailable_ = false;
    if (!tdLibAvailable_) {
        setAuthorizationState(AuthorizationState::MissingDependency,
                              "TDLib no pudo inicializarse en runtime. La UI queda lista para conectarla.");
        return;
    }
}

void TDLibAdapter::submitBootstrap(const QString &apiId,
                                   const QString &apiHash,
                                   const QString &phoneNumber) {
    if (authorizationState_ == AuthorizationState::Ready) {
        requestInitialData();
        setAuthorizationState(AuthorizationState::Ready,
                              "La sesion actual ya estaba autenticada. Se reutilizo la sesion local y se refrescaron los datos.");
        return;
    }

    apiId_ = apiId.trimmed();
    apiHash_ = apiHash.trimmed();
    phoneNumber_ = phoneNumber.trimmed();

    if (!tdLibAvailable_) {
        setAuthorizationState(AuthorizationState::MissingDependency,
                              "No se puede iniciar sesion todavia porque TDLib no esta presente en el sistema.");
        return;
    }

    if (apiId_.isEmpty() || apiHash_.isEmpty()) {
        setAuthorizationState(AuthorizationState::WaitingParameters,
                              "Faltan api_id o api_hash para inicializar TDLib.");
        return;
    }

    if (phoneNumber_.isEmpty()) {
        submitTdlibParameters();
        setAuthorizationState(AuthorizationState::WaitingPhoneNumber,
                              "Falta el numero de telefono para continuar el login.");
        return;
    }

    submitTdlibParameters();
    sendRequest(std::string("{\"@type\":\"setAuthenticationPhoneNumber\",\"phone_number\":\"")
                + phoneNumber_.toStdString()
                + "\",\"settings\":{\"@type\":\"phoneNumberAuthenticationSettings\",\"allow_flash_call\":false,\"allow_missed_call\":false,\"is_current_phone_number\":true,\"allow_sms_retriever_api\":false}}");
    setAuthorizationState(AuthorizationState::WaitingCode,
                          "Solicitud de telefono enviada a TDLib. Esperando el codigo de autenticacion.");
}

void TDLibAdapter::submitAuthenticationCode(const QString &code) {
    const QString trimmedCode = code.trimmed();

    if (authorizationState_ == AuthorizationState::Ready) {
        requestInitialData();
        setAuthorizationState(AuthorizationState::Ready,
                              "La sesion actual ya estaba autenticada. No hace falta reenviar un codigo.");
        return;
    }

    if (!tdLibAvailable_) {
        setAuthorizationState(AuthorizationState::MissingDependency,
                              "No se puede validar el codigo porque TDLib no esta presente en el sistema.");
        return;
    }

    if (trimmedCode.isEmpty()) {
        setAuthorizationState(AuthorizationState::WaitingCode,
                              "Falta capturar el codigo de autenticacion.");
        return;
    }

    sendRequest(std::string("{\"@type\":\"checkAuthenticationCode\",\"code\":\"")
                + trimmedCode.toStdString()
                + "\"}");
    setAuthorizationState(AuthorizationState::WaitingCode,
                          "Codigo enviado a TDLib. Esperando confirmacion de autenticacion.");
}

void TDLibAdapter::submitAuthenticationPassword(const QString &password) {
    const QString trimmedPassword = password.trimmed();

    if (authorizationState_ == AuthorizationState::Ready) {
        requestInitialData();
        setAuthorizationState(AuthorizationState::Ready,
                              "La sesion actual ya estaba autenticada. No hace falta reenviar la contrasena.");
        return;
    }

    if (!tdLibAvailable_) {
        setAuthorizationState(AuthorizationState::MissingDependency,
                              "No se puede validar la contrasena porque TDLib no esta presente en el sistema.");
        return;
    }

    if (trimmedPassword.isEmpty()) {
        setAuthorizationState(AuthorizationState::WaitingPassword,
                              "Falta capturar la contrasena de verificacion en dos pasos.");
        return;
    }

    sendRequest(std::string("{\"@type\":\"checkAuthenticationPassword\",\"password\":\"")
                + trimmedPassword.toStdString()
                + "\"}");
    setAuthorizationState(AuthorizationState::WaitingPassword,
                          "Contrasena enviada a TDLib. Esperando confirmacion de autenticacion.");
}

void TDLibAdapter::sendRequest(const std::string &request) {
#if defined(MTC_HAS_TDLIB)
    if (tdJsonClient_ != nullptr) {
        td_json_client_send(tdJsonClient_, request.c_str());
    }
#else
    static_cast<void>(request);
#endif
}

void TDLibAdapter::pollResponses() {
#if defined(MTC_HAS_TDLIB)
    if (tdJsonClient_ == nullptr) {
        return;
    }

    while (const char *response = td_json_client_receive(tdJsonClient_, 0.0)) {
        handleResponse(response);
    }
#endif
}

void TDLibAdapter::handleResponse(const char *response) {
    const QString payload = QString::fromUtf8(response);

    if (payload.contains("\"authorizationStateWaitTdlibParameters\"")) {
        tdlibParametersSent_ = false;
        setAuthorizationState(AuthorizationState::WaitingParameters,
                              "TDLib espera setTdlibParameters. Completa api_id, api_hash y telefono para seguir.");
        return;
    }

    if (payload.contains("\"authorizationStateWaitPhoneNumber\"")) {
        setAuthorizationState(AuthorizationState::WaitingPhoneNumber,
                              "TDLib acepto los parametros. Falta enviar el numero de telefono.");
        return;
    }

    if (payload.contains("\"authorizationStateWaitCode\"")) {
        setAuthorizationState(AuthorizationState::WaitingCode,
                              "TDLib envio el codigo. El siguiente paso es capturarlo y validarlo en la UI.");
        return;
    }

    if (payload.contains("\"authorizationStateWaitPassword\"")) {
        setAuthorizationState(AuthorizationState::WaitingPassword,
                              "La cuenta requiere verificacion en dos pasos. Falta capturar la contrasena de Telegram.");
        return;
    }

    if (payload.contains("\"authorizationStateWaitOtherDeviceConfirmation\"")) {
        setAuthorizationState(AuthorizationState::WaitingOtherDeviceConfirmation,
                              "Telegram pide confirmar el inicio de sesion desde otro dispositivo ya autenticado.");
        return;
    }

    if (payload.contains("\"authorizationStateReady\"")) {
        requestInitialData();
        setAuthorizationState(AuthorizationState::Ready,
                              "La sesion TDLib quedo autenticada y lista para usar.");
        return;
    }

    if (payload.contains("\"authorizationStateClosed\"")) {
        tdLibAvailable_ = false;
        tdlibParametersSent_ = false;
        setAuthorizationState(AuthorizationState::Failed,
                              "TDLib cerro la sesion. Revisa logs y vuelve a inicializar el cliente.");
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(payload.toUtf8());
    if (!document.isObject()) {
        return;
    }

    const QJsonObject object = document.object();
    const QString type = object.value("@type").toString();

    if (type == "error") {
        setAuthorizationState(AuthorizationState::Failed, extractTdLibErrorMessage(object));
        return;
    }

    if (type == "user") {
        const QString firstName = object.value("first_name").toString().trimmed();
        const QString lastName = object.value("last_name").toString().trimmed();
        const QString username = object.value("username").toString().trimmed();

        QString label = (firstName + " " + lastName).trimmed();
        if (label.isEmpty()) {
            label = username;
        }
        if (label.isEmpty()) {
            label = QStringLiteral("Usuario autenticado");
        }

        selfDisplayName_ = label;
        emit dataChanged();
        return;
    }

    if (type == "chats") {
        const QJsonArray chatIds = object.value("chat_ids").toArray();
        for (const QJsonValue &chatIdValue : chatIds) {
            sendRequest(std::string("{\"@type\":\"getChat\",\"chat_id\":")
                        + std::to_string(chatIdValue.toVariant().toLongLong())
                        + "}");
        }
        return;
    }

    if (type == "chat" || type == "updateNewChat") {
        const QJsonObject chatObject = type == "chat" ? object : object.value("chat").toObject();
        const QString chatId = QString::number(chatObject.value("id").toVariant().toLongLong());
        const QString title = chatObject.value("title").toString().trimmed();
        if (!chatId.isEmpty() && !title.isEmpty()) {
            chatTitlesById_[chatId] = title;
            emit dataChanged();
        }
        return;
    }

    if (type == "updateChatTitle") {
        const QString chatId = QString::number(object.value("chat_id").toVariant().toLongLong());
        const QString title = object.value("title").toString().trimmed();
        if (!chatId.isEmpty() && !title.isEmpty()) {
            chatTitlesById_[chatId] = title;
            emit dataChanged();
        }
    }
}

void TDLibAdapter::requestInitialData() {
    if (initialDataRequested_) {
        return;
    }

    initialDataRequested_ = true;
    sendRequest("{\"@type\":\"getMe\"}");
    sendRequest("{\"@type\":\"loadChats\",\"chat_list\":{\"@type\":\"chatListMain\"},\"limit\":20}");
    sendRequest("{\"@type\":\"getChats\",\"chat_list\":{\"@type\":\"chatListMain\"},\"limit\":20}");
}

void TDLibAdapter::submitTdlibParameters() {
    if (tdlibParametersSent_) {
        return;
    }

    const QString dataRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataRoot + "/tdlib/database");
    QDir().mkpath(dataRoot + "/tdlib/files");

    const std::string parametersRequest =
        std::string("{\"@type\":\"setTdlibParameters\",\"use_test_dc\":false,\"database_directory\":\"")
        + (dataRoot + "/tdlib/database").toStdString()
        + "\",\"files_directory\":\""
        + (dataRoot + "/tdlib/files").toStdString()
        + "\",\"database_encryption_key\":\"\",\"use_file_database\":true,\"use_chat_info_database\":true,\"use_message_database\":true,"
          "\"use_secret_chats\":false,\"api_id\":"
        + apiId_.toStdString()
        + ",\"api_hash\":\""
        + apiHash_.toStdString()
        + "\",\"system_language_code\":\"es\",\"device_model\":\"MTC\",\"system_version\":\"Linux\",\"application_version\":\"0.1.0\","
          "\"enable_storage_optimizer\":true,\"ignore_file_names\":false}";

    sendRequest(parametersRequest);
    tdlibParametersSent_ = true;
}

bool TDLibAdapter::isTdLibAvailable() const {
    return tdLibAvailable_;
}

AuthorizationState TDLibAdapter::authorizationState() const {
    return authorizationState_;
}

QString TDLibAdapter::authorizationStateLabel() const {
    switch (authorizationState_) {
        case AuthorizationState::NotInitialized:
            return "No inicializado";
        case AuthorizationState::MissingDependency:
            return "TDLib ausente";
        case AuthorizationState::WaitingParameters:
            return "Esperando parametros";
        case AuthorizationState::WaitingPhoneNumber:
            return "Esperando telefono";
        case AuthorizationState::WaitingCode:
            return "Esperando codigo";
        case AuthorizationState::WaitingPassword:
            return "Esperando contrasena";
        case AuthorizationState::WaitingOtherDeviceConfirmation:
            return "Esperando confirmacion";
        case AuthorizationState::Ready:
            return "Listo";
        case AuthorizationState::Failed:
            return "Fallido";
    }

    return "Desconocido";
}

QString TDLibAdapter::diagnosticMessage() const {
    return diagnosticMessage_;
}

QString TDLibAdapter::selfDisplayName() const {
    return selfDisplayName_;
}

QStringList TDLibAdapter::chatTitles() const {
    return chatTitlesById_.values();
}

std::string TDLibAdapter::status() const {
    std::ostringstream stream;
    stream << "TDLibAdapter: estado=" << authorizationStateLabel().toStdString()
           << ", tdlib=" << (tdLibAvailable_ ? "disponible" : "ausente");

    if (!diagnosticMessage_.isEmpty()) {
        stream << ". " << diagnosticMessage_.toStdString();
    }

    return stream.str();
}

QString TDLibAdapter::extractTdLibErrorMessage(const QJsonObject &object) const {
    const int code = object.value("code").toInt();
    const QString message = object.value("message").toString().trimmed();

    if (message.isEmpty()) {
        return QString("TDLib devolvio un error sin detalle adicional (codigo %1).").arg(code);
    }

    if (message.contains("PHONE_NUMBER_INVALID", Qt::CaseInsensitive)) {
        return "El numero de telefono no es valido para Telegram. Revisa el formato internacional.";
    }

    if (message.contains("API_ID_INVALID", Qt::CaseInsensitive)
        || message.contains("API_ID_PUBLISHED_FLOOD", Qt::CaseInsensitive)) {
        return "El api_id o api_hash no fue aceptado por Telegram. Revisa las credenciales de TDLib.";
    }

    if (message.contains("PHONE_CODE_INVALID", Qt::CaseInsensitive)) {
        return "El codigo ingresado no es correcto. Captura el codigo mas reciente enviado por Telegram.";
    }

    if (message.contains("PHONE_CODE_EXPIRED", Qt::CaseInsensitive)) {
        return "El codigo ya expiro. Vuelve a solicitar uno nuevo e intentalo otra vez.";
    }

    if (message.contains("PASSWORD_HASH_INVALID", Qt::CaseInsensitive)) {
        return "La verificacion en dos pasos fallo. La contrasena de Telegram no coincide.";
    }

    if (message.contains("AUTH_KEY_UNREGISTERED", Qt::CaseInsensitive)) {
        return "La sesion local ya no es valida. Conviene reiniciar el flujo de autorizacion desde cero.";
    }

    return QString("TDLib error %1: %2").arg(code).arg(message);
}

void TDLibAdapter::setAuthorizationState(AuthorizationState state, const QString &diagnosticMessage) {
    authorizationState_ = state;
    diagnosticMessage_ = diagnosticMessage;
    emit stateChanged();
}

}  // namespace mtc
