#include "core/telegram/TDLibAdapter.h"

#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>

#include <sstream>

#if defined(MTC_HAS_TDLIB)
#include <td/telegram/td_json_client.h>
#endif

namespace mtc {

namespace {

QJsonObject formattedText(const QString &text) {
    QJsonObject value;
    value["@type"] = "formattedText";
    value["text"] = text;
    return value;
}

QString captionText(const QJsonObject &content) {
    return content.value("caption").toObject().value("text").toString().trimmed();
}

QString messageSummaryFromContent(const QJsonObject &content) {
    const QString type = content.value("@type").toString();

    if (type == "messageText") {
        const QString text = content.value("text").toObject().value("text").toString().trimmed();
        return text.isEmpty() ? "[Texto vacio]" : text;
    }

    if (type == "messagePhoto") {
        const QString caption = captionText(content);
        return caption.isEmpty() ? "[Foto]" : QString("[Foto] %1").arg(caption);
    }

    if (type == "messageVideo") {
        const QString fileName = content.value("video").toObject().value("file_name").toString().trimmed();
        const QString caption = captionText(content);
        if (!caption.isEmpty()) {
            return QString("[Video] %1").arg(caption);
        }
        return fileName.isEmpty() ? "[Video]" : QString("[Video] %1").arg(fileName);
    }

    if (type == "messageDocument") {
        const QString fileName = content.value("document").toObject().value("file_name").toString().trimmed();
        const QString caption = captionText(content);
        if (!caption.isEmpty()) {
            return QString("[Documento] %1").arg(caption);
        }
        return fileName.isEmpty() ? "[Documento]" : QString("[Documento] %1").arg(fileName);
    }

    if (type == "messageAudio") {
        const QJsonObject audio = content.value("audio").toObject();
        const QString performer = audio.value("performer").toString().trimmed();
        const QString title = audio.value("title").toString().trimmed();
        if (!performer.isEmpty() || !title.isEmpty()) {
            return QString("[Audio] %1 %2").arg(performer, title).trimmed();
        }
        return "[Audio]";
    }

    if (type == "messageVoiceNote") {
        return "[Nota de voz]";
    }

    if (type == "messageSticker") {
        const QString emoji = content.value("sticker").toObject().value("emoji").toString().trimmed();
        return emoji.isEmpty() ? "[Sticker]" : QString("[Sticker] %1").arg(emoji);
    }

    if (type == "messageAnimation") {
        const QString caption = captionText(content);
        return caption.isEmpty() ? "[Animacion/GIF]" : QString("[Animacion/GIF] %1").arg(caption);
    }

    if (type == "messageVideoNote") {
        return "[Video nota]";
    }

    if (type == "messageLocation") {
        const QJsonObject location = content.value("location").toObject();
        const double lat = location.value("latitude").toDouble();
        const double lon = location.value("longitude").toDouble();
        return QString("[Ubicacion] %1, %2").arg(lat, 0, 'f', 5).arg(lon, 0, 'f', 5);
    }

    if (type == "messageContact") {
        const QJsonObject contact = content.value("contact").toObject();
        const QString name = QString("%1 %2")
                                 .arg(contact.value("first_name").toString().trimmed(),
                                      contact.value("last_name").toString().trimmed())
                                 .trimmed();
        return name.isEmpty() ? "[Contacto]" : QString("[Contacto] %1").arg(name);
    }

    if (type == "messagePoll") {
        const QString question = content.value("poll").toObject().value("question").toString().trimmed();
        return question.isEmpty() ? "[Encuesta]" : QString("[Encuesta] %1").arg(question);
    }

    if (type == "messageChatChangeTitle") {
        return QString("[Sistema] Cambio de titulo: %1").arg(content.value("title").toString().trimmed());
    }

    if (type == "messageChatAddMembers") {
        return "[Sistema] Se agregaron miembros al chat";
    }

    if (type == "messageChatDeleteMember") {
        return "[Sistema] Se elimino un miembro del chat";
    }

    if (type == "messageUnsupported") {
        return "[Contenido no soportado]";
    }

    return type.isEmpty() ? "[Mensaje no textual]" : QString("[Tipo %1]").arg(type);
}

QString messageLineFromObject(const QJsonObject &messageObject) {
    const QString direction = messageObject.value("is_outgoing").toBool(false) ? "Yo" : "Chat";
    const QDateTime at = QDateTime::fromSecsSinceEpoch(messageObject.value("date").toInt(), Qt::UTC);
    const QString timeLabel = at.isValid() ? at.toLocalTime().toString("HH:mm") : "--:--";
    const QString text = messageSummaryFromContent(messageObject.value("content").toObject());
    return QString("[%1] %2: %3").arg(timeLabel, direction, text);
}

qint64 jsonToInt64(const QJsonValue &value) {
    return value.toVariant().toLongLong();
}

bool fileIsDownloaded(const QJsonObject &fileObject) {
    const QJsonObject localObject = fileObject.value("local").toObject();
    return localObject.value("is_downloading_completed").toBool(false)
           && !localObject.value("path").toString().trimmed().isEmpty();
}

QString contentTypeKey(const QString &contentType) {
    if (contentType == "messageText") {
        return "text";
    }
    if (contentType == "messagePhoto") {
        return "photo";
    }
    if (contentType == "messageVideo") {
        return "video";
    }
    if (contentType == "messageDocument") {
        return "document";
    }
    if (contentType == "messageAudio") {
        return "audio";
    }
    if (contentType == "messageVoiceNote") {
        return "voice_note";
    }
    if (contentType == "messageSticker") {
        return "sticker";
    }
    if (contentType == "messageAnimation") {
        return "animation";
    }
    if (contentType == "messageVideoNote") {
        return "video_note";
    }
    if (contentType == "messageLocation") {
        return "location";
    }
    if (contentType == "messageContact") {
        return "contact";
    }
    if (contentType == "messagePoll") {
        return "poll";
    }
    return contentType.isEmpty() ? "unknown" : contentType;
}

QJsonObject bestPhotoFileObject(const QJsonObject &photoObject) {
    const QJsonArray sizes = photoObject.value("sizes").toArray();
    QJsonObject bestFile;
    qint64 bestScore = -1;

    for (const QJsonValue &sizeValue : sizes) {
        const QJsonObject sizeObject = sizeValue.toObject();
        const QJsonObject fileObject = sizeObject.value("photo").toObject();
        if (fileObject.isEmpty()) {
            continue;
        }

        const qint64 width = sizeObject.value("width").toInt();
        const qint64 height = sizeObject.value("height").toInt();
        const qint64 score = width * height;
        if (bestFile.isEmpty() || score >= bestScore) {
            bestFile = fileObject;
            bestScore = score;
        }
    }

    return bestFile;
}

QJsonObject bestDownloadedPhotoFileObject(const QJsonObject &photoObject) {
    const QJsonArray sizes = photoObject.value("sizes").toArray();
    QJsonObject bestFile;
    qint64 bestScore = -1;

    for (const QJsonValue &sizeValue : sizes) {
        const QJsonObject sizeObject = sizeValue.toObject();
        const QJsonObject fileObject = sizeObject.value("photo").toObject();
        if (fileObject.isEmpty() || !fileIsDownloaded(fileObject)) {
            continue;
        }

        const qint64 width = sizeObject.value("width").toInt();
        const qint64 height = sizeObject.value("height").toInt();
        const qint64 score = width * height;
        if (bestFile.isEmpty() || score >= bestScore) {
            bestFile = fileObject;
            bestScore = score;
        }
    }

    return bestFile;
}

QJsonObject thumbnailFileFromObject(const QJsonObject &object) {
    return object.value("thumbnail").toObject().value("file").toObject();
}

QJsonObject primaryFileObjectFromContent(const QJsonObject &content) {
    const QString type = content.value("@type").toString();
    if (type == "messagePhoto") {
        return bestPhotoFileObject(content.value("photo").toObject());
    }
    if (type == "messageVideo") {
        return content.value("video").toObject().value("video").toObject();
    }
    if (type == "messageDocument") {
        return content.value("document").toObject().value("document").toObject();
    }
    if (type == "messageAudio") {
        return content.value("audio").toObject().value("audio").toObject();
    }
    if (type == "messageVoiceNote") {
        return content.value("voice_note").toObject().value("voice").toObject();
    }
    if (type == "messageSticker") {
        return content.value("sticker").toObject().value("sticker").toObject();
    }
    if (type == "messageAnimation") {
        return content.value("animation").toObject().value("animation").toObject();
    }
    if (type == "messageVideoNote") {
        return content.value("video_note").toObject().value("video").toObject();
    }
    return QJsonObject();
}

struct FileMetadata {
    qint64 fileId = 0;
    QString localPath;
    bool canDownload = false;
};

FileMetadata fileMetadataFromObject(const QJsonObject &fileObject) {
    FileMetadata metadata;
    if (fileObject.isEmpty()) {
        return metadata;
    }

    metadata.fileId = jsonToInt64(fileObject.value("id"));
    if (metadata.fileId <= 0) {
        return metadata;
    }

    const QJsonObject localObject = fileObject.value("local").toObject();
    const bool isDownloaded = localObject.value("is_downloading_completed").toBool(false);
    metadata.localPath = localObject.value("path").toString().trimmed();
    if (!isDownloaded) {
        metadata.localPath.clear();
    }
    metadata.canDownload = localObject.value("can_be_downloaded").toBool(false);
    if (!metadata.localPath.isEmpty()) {
        metadata.canDownload = false;
    }
    return metadata;
}

QJsonObject previewFileObjectFromContent(const QJsonObject &content) {
    const QString type = content.value("@type").toString();
    if (type == "messagePhoto") {
        const QJsonObject photo = content.value("photo").toObject();
        const QJsonObject downloaded = bestDownloadedPhotoFileObject(photo);
        if (!downloaded.isEmpty()) {
            return downloaded;
        }
        return bestPhotoFileObject(photo);
    }
    if (type == "messageVideo") {
        const QJsonObject videoObject = content.value("video").toObject();
        const QJsonObject thumbnailFile = thumbnailFileFromObject(videoObject);
        if (!thumbnailFile.isEmpty()) {
            return thumbnailFile;
        }
        return videoObject.value("video").toObject();
    }
    if (type == "messageDocument") {
        const QJsonObject documentObject = content.value("document").toObject();
        const QJsonObject thumbnailFile = thumbnailFileFromObject(documentObject);
        if (!thumbnailFile.isEmpty()) {
            return thumbnailFile;
        }
        return documentObject.value("document").toObject();
    }
    if (type == "messageAnimation") {
        const QJsonObject animationObject = content.value("animation").toObject();
        const QJsonObject thumbnailFile = thumbnailFileFromObject(animationObject);
        if (!thumbnailFile.isEmpty()) {
            return thumbnailFile;
        }
        return animationObject.value("animation").toObject();
    }
    if (type == "messageSticker") {
        const QJsonObject stickerObject = content.value("sticker").toObject();
        const QJsonObject thumbnailFile = thumbnailFileFromObject(stickerObject);
        if (!thumbnailFile.isEmpty()) {
            return thumbnailFile;
        }
        return stickerObject.value("sticker").toObject();
    }
    if (type == "messageVideoNote") {
        const QJsonObject videoNoteObject = content.value("video_note").toObject();
        const QJsonObject thumbnailFile = thumbnailFileFromObject(videoNoteObject);
        if (!thumbnailFile.isEmpty()) {
            return thumbnailFile;
        }
        return videoNoteObject.value("video").toObject();
    }

    if (type == "messageAudio") {
        return content.value("audio").toObject().value("album_cover_thumbnail").toObject().value("file").toObject();
    }

    return primaryFileObjectFromContent(content);
}

ChatMessageEntry chatMessageEntryFromObject(const QJsonObject &messageObject) {
    ChatMessageEntry entry;
    entry.messageId = jsonToInt64(messageObject.value("id"));
    entry.line = messageLineFromObject(messageObject);

    const QJsonObject content = messageObject.value("content").toObject();
    entry.contentType = contentTypeKey(content.value("@type").toString());

    const FileMetadata primaryMetadata = fileMetadataFromObject(primaryFileObjectFromContent(content));
    entry.fileId = primaryMetadata.fileId;
    entry.localPath = primaryMetadata.localPath;
    entry.canDownload = primaryMetadata.canDownload;

    const FileMetadata previewMetadata = fileMetadataFromObject(previewFileObjectFromContent(content));
    entry.previewFileId = previewMetadata.fileId;
    entry.previewLocalPath = previewMetadata.localPath;
    entry.previewCanDownload = previewMetadata.canDownload;

    if (entry.previewFileId <= 0) {
        entry.previewFileId = entry.fileId;
        entry.previewLocalPath = entry.localPath;
        entry.previewCanDownload = entry.canDownload;
    }

    return entry;
}

}  // namespace

TDLibAdapter::TDLibAdapter(QObject *parent)
    : QObject(parent) {
    tdLibAvailable_ = false;
    pollTimer_ = std::make_unique<QTimer>(this);
    pollTimer_->setInterval(50);
    connect(pollTimer_.get(), &QTimer::timeout, this, &TDLibAdapter::pollResponses);
    closeSessionTimer_ = std::make_unique<QTimer>(this);
    closeSessionTimer_->setSingleShot(true);
    closeSessionTimer_->setInterval(8000);
    connect(closeSessionTimer_.get(), &QTimer::timeout, this, &TDLibAdapter::handleCloseSessionTimeout);
    appendFlowLog("adapter_created");
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

void TDLibAdapter::continueAuthorization(const QString &apiId,
                                         const QString &apiHash,
                                         const QString &phoneNumber) {
    const QString nextApiId = apiId.trimmed();
    const QString nextApiHash = apiHash.trimmed();
    const QString nextPhoneNumber = phoneNumber.trimmed();
    appendFlowLog(QString("continue_authorization state=%1").arg(stateKey(authorizationState_)));

    if (!tdLibAvailable_) {
        setAuthorizationState(AuthorizationState::MissingDependency,
                              "No se puede iniciar sesion todavia porque TDLib no esta presente en el sistema.");
        return;
    }

    switch (authorizationState_) {
        case AuthorizationState::ClosingSession:
            return;
        case AuthorizationState::WaitingPhoneNumber:
            apiId_ = nextApiId;
            apiHash_ = nextApiHash;
            submitPhoneNumber(nextPhoneNumber);
            return;
        case AuthorizationState::WaitingCode:
        case AuthorizationState::WaitingPassword:
        case AuthorizationState::WaitingOtherDeviceConfirmation:
            return;
        case AuthorizationState::Ready:
        case AuthorizationState::NotInitialized:
        case AuthorizationState::MissingDependency:
        case AuthorizationState::WaitingParameters:
        case AuthorizationState::WaitingEncryptionKey:
        case AuthorizationState::Failed:
            submitBootstrap(nextApiId, nextApiHash, nextPhoneNumber);
            return;
    }
}

void TDLibAdapter::submitBootstrap(const QString &apiId,
                                   const QString &apiHash,
                                   const QString &phoneNumber) {
    const QString nextApiId = apiId.trimmed();
    const QString nextApiHash = apiHash.trimmed();
    const QString nextPhoneNumber = phoneNumber.trimmed();
    const QString nextSessionKey = buildSessionKey(nextApiId, nextPhoneNumber);

    if (authorizationState_ == AuthorizationState::Ready) {
        if (!activeSessionKey_.isEmpty() && nextSessionKey != activeSessionKey_) {
            pendingBootstrapSubmission_ = true;
            pendingApiId_ = nextApiId;
            pendingApiHash_ = nextApiHash;
            pendingPhoneNumber_ = nextPhoneNumber;
            resetSession();
            return;
        }

        apiId_ = nextApiId;
        apiHash_ = nextApiHash;
        phoneNumber_ = nextPhoneNumber;
        requestInitialData();
        setAuthorizationState(AuthorizationState::Ready,
                              "La sesion actual ya estaba autenticada. Se reutilizo la sesion local y se refrescaron los datos.");
        return;
    }

    apiId_ = nextApiId;
    apiHash_ = nextApiHash;
    phoneNumber_ = nextPhoneNumber;

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

    submitTdlibParameters();
    if (phoneNumber_.isEmpty()) {
        setAuthorizationState(AuthorizationState::WaitingPhoneNumber,
                              "Falta el numero de telefono para continuar el login.");
        return;
    }

    submitPhoneNumber(phoneNumber_);
}

void TDLibAdapter::submitPhoneNumber(const QString &phoneNumber) {
    phoneNumber_ = phoneNumber.trimmed();

    if (authorizationState_ == AuthorizationState::Ready) {
        requestInitialData();
        setAuthorizationState(AuthorizationState::Ready,
                              "La sesion actual ya estaba autenticada. No hace falta reenviar el telefono.");
        return;
    }

    if (!tdLibAvailable_) {
        setAuthorizationState(AuthorizationState::MissingDependency,
                              "No se puede enviar el telefono porque TDLib no esta presente en el sistema.");
        return;
    }

    if (apiId_.isEmpty() || apiHash_.isEmpty()) {
        setAuthorizationState(AuthorizationState::WaitingParameters,
                              "Primero completa api_id y api_hash para preparar la sesion TDLib.");
        return;
    }

    if (phoneNumber_.isEmpty()) {
        setAuthorizationState(AuthorizationState::WaitingPhoneNumber,
                              "Falta el numero de telefono para continuar el login.");
        return;
    }

    if (authorizationState_ == AuthorizationState::WaitingCode
        || authorizationState_ == AuthorizationState::WaitingPassword
        || authorizationState_ == AuthorizationState::WaitingOtherDeviceConfirmation) {
        setAuthorizationState(authorizationState_,
                              "El flujo ya avanzo al siguiente paso. Continua con codigo o confirmacion segun corresponda.");
        return;
    }

    submitTdlibParameters();
    if (authorizationState_ == AuthorizationState::WaitingParameters
        || authorizationState_ == AuthorizationState::NotInitialized
        || authorizationState_ == AuthorizationState::Failed) {
        pendingPhoneNumberSubmission_ = true;
        setAuthorizationState(AuthorizationState::WaitingPhoneNumber,
                              "Telefono capturado. TDLib aun prepara parametros; se enviara automaticamente al quedar listo.");
        return;
    }

    if (phoneNumberRequestInFlight_) {
        return;
    }

    sendPhoneNumberRequest();
    setAuthorizationState(AuthorizationState::WaitingPhoneNumber,
                          "Solicitud de telefono enviada a TDLib. Esperando confirmacion para pedir el codigo.");
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

void TDLibAdapter::logout() {
    if (!tdLibAvailable_) {
        setAuthorizationState(AuthorizationState::MissingDependency,
                              "No se puede cerrar sesion porque TDLib no esta presente en el sistema.");
        return;
    }

    sendRequest("{\"@type\":\"logOut\"}");
    initialDataRequested_ = false;
    setAuthorizationState(AuthorizationState::WaitingParameters,
                          "Solicitud de cierre de sesion enviada. Esperando confirmacion de TDLib.");
}

void TDLibAdapter::resetSession() {
    if (!tdLibAvailable_) {
        setAuthorizationState(AuthorizationState::MissingDependency,
                              "No se puede reiniciar la sesion porque TDLib no esta presente en el sistema.");
        return;
    }

    sendRequest("{\"@type\":\"close\"}");
    clearSessionData();
    initialDataRequested_ = false;
    tdlibParametersSent_ = false;
    activeSessionKey_.clear();
    setAuthorizationState(AuthorizationState::ClosingSession,
                          "Solicitud de reinicio enviada. Esperando a que TDLib cierre la sesion actual.");
    closeSessionTimer_->start();
    appendFlowLog("reset_session_requested");
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

void TDLibAdapter::sendPhoneNumberRequest() {
    if (phoneNumberRequestInFlight_) {
        return;
    }

    sendRequest(std::string("{\"@type\":\"setAuthenticationPhoneNumber\",\"phone_number\":\"")
                + phoneNumber_.toStdString()
                + "\",\"settings\":{\"@type\":\"phoneNumberAuthenticationSettings\",\"allow_flash_call\":false,\"allow_missed_call\":false,\"is_current_phone_number\":true,\"allow_sms_retriever_api\":false}}");
    pendingPhoneNumberSubmission_ = false;
    phoneNumberRequestInFlight_ = true;
}

QString TDLibAdapter::buildSessionKey(const QString &apiId, const QString &phoneNumber) const {
    QString rawKey = apiId.trimmed();
    if (!phoneNumber.trimmed().isEmpty()) {
        if (!rawKey.isEmpty()) {
            rawKey += "_";
        }
        rawKey += phoneNumber.trimmed();
    }

    if (rawKey.isEmpty()) {
        rawKey = "default";
    }

    QString sanitized;
    sanitized.reserve(rawKey.size());
    for (const QChar character : rawKey) {
        if (character.isLetterOrNumber() || character == '_' || character == '-' || character == '.') {
            sanitized.append(character);
        } else {
            sanitized.append('_');
        }
    }

    return sanitized;
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

void TDLibAdapter::handleCloseSessionTimeout() {
    if (authorizationState_ != AuthorizationState::ClosingSession) {
        return;
    }
    appendFlowLog("close_session_timeout");

#if defined(MTC_HAS_TDLIB)
    if (tdJsonClient_ != nullptr) {
        td_json_client_destroy(tdJsonClient_);
        tdJsonClient_ = nullptr;
    }
    tdJsonClient_ = td_json_client_create();
    tdLibAvailable_ = tdJsonClient_ != nullptr;
#endif

    tdlibParametersSent_ = false;
    initialDataRequested_ = false;
    phoneNumberRequestInFlight_ = false;
    activeSessionKey_.clear();
    clearSessionData();

    if (!tdLibAvailable_) {
        setAuthorizationState(AuthorizationState::MissingDependency,
                              "No se pudo recuperar TDLib tras esperar el cierre de sesion.");
        appendFlowLog("close_session_timeout_recovery_failed");
        return;
    }

    setAuthorizationState(AuthorizationState::WaitingParameters,
                          "TDLib no confirmo cierre a tiempo. Se reinicio el cliente y se reanuda el flujo.");
    appendFlowLog("close_session_timeout_recovery_ok");

    if (pendingBootstrapSubmission_) {
        const QString pendingApiId = pendingApiId_;
        const QString pendingApiHash = pendingApiHash_;
        const QString pendingPhoneNumber = pendingPhoneNumber_;
        pendingBootstrapSubmission_ = false;
        pendingApiId_.clear();
        pendingApiHash_.clear();
        pendingPhoneNumber_.clear();
        submitBootstrap(pendingApiId, pendingApiHash, pendingPhoneNumber);
    }
}

void TDLibAdapter::handleResponse(const char *response) {
    const QString payload = QString::fromUtf8(response);

    if (payload.contains("\"authorizationStateWaitTdlibParameters\"")) {
        tdlibParametersSent_ = false;
        phoneNumberRequestInFlight_ = false;
        setAuthorizationState(AuthorizationState::WaitingParameters,
                              "TDLib espera setTdlibParameters. Completa api_id, api_hash y telefono para seguir.");
        return;
    }

    if (payload.contains("\"authorizationStateWaitEncryptionKey\"")) {
        sendRequest("{\"@type\":\"checkDatabaseEncryptionKey\",\"encryption_key\":\"\"}");
        setAuthorizationState(AuthorizationState::WaitingEncryptionKey,
                              "TDLib pide validar la clave de base local. Se envio clave vacia por defecto para continuar.");
        return;
    }

    if (payload.contains("\"authorizationStateWaitPhoneNumber\"")) {
        if (pendingPhoneNumberSubmission_ && !phoneNumber_.isEmpty()) {
            sendPhoneNumberRequest();
            setAuthorizationState(AuthorizationState::WaitingPhoneNumber,
                                  "TDLib acepto los parametros y se envio el telefono automaticamente.");
            return;
        }

        setAuthorizationState(AuthorizationState::WaitingPhoneNumber,
                              "TDLib acepto los parametros. Falta enviar el numero de telefono.");
        return;
    }

    if (payload.contains("\"authorizationStateWaitCode\"")) {
        phoneNumberRequestInFlight_ = false;
        setAuthorizationState(AuthorizationState::WaitingCode,
                              "TDLib envio el codigo. El siguiente paso es capturarlo y validarlo en la UI.");
        return;
    }

    if (payload.contains("\"authorizationStateWaitPassword\"")) {
        phoneNumberRequestInFlight_ = false;
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
        phoneNumberRequestInFlight_ = false;
        requestInitialData();
        setAuthorizationState(AuthorizationState::Ready,
                              "La sesion TDLib quedo autenticada y lista para usar.");
        return;
    }

    if (payload.contains("\"authorizationStateClosed\"")) {
        closeSessionTimer_->stop();
        clearSessionData();
        tdlibParametersSent_ = false;
        initialDataRequested_ = false;
        phoneNumberRequestInFlight_ = false;
        activeSessionKey_.clear();
        setAuthorizationState(AuthorizationState::NotInitialized,
                              "TDLib cerro la sesion actual. Puedes iniciar un nuevo flujo de autenticacion.");

        if (pendingBootstrapSubmission_) {
            const QString pendingApiId = pendingApiId_;
            const QString pendingApiHash = pendingApiHash_;
            const QString pendingPhoneNumber = pendingPhoneNumber_;
            pendingBootstrapSubmission_ = false;
            pendingApiId_.clear();
            pendingApiHash_.clear();
            pendingPhoneNumber_.clear();
            submitBootstrap(pendingApiId, pendingApiHash, pendingPhoneNumber);
        }
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(payload.toUtf8());
    if (!document.isObject()) {
        return;
    }

    const QJsonObject object = document.object();
    const QString type = object.value("@type").toString();

    if (type == "error") {
        phoneNumberRequestInFlight_ = false;
        const int code = object.value("code").toInt();
        const QString message = object.value("message").toString().trimmed();
        const bool isRequestAborted = message.contains("REQUEST_ABORTED", Qt::CaseInsensitive)
                                      || message.contains("REQUEST ABORTED", Qt::CaseInsensitive)
                                      || message.contains("REQUEST_ABORT", Qt::CaseInsensitive)
                                      || message.contains("REQUEST ABORT", Qt::CaseInsensitive)
                                      || message.contains("request aborted", Qt::CaseInsensitive);
        if (code == 500 && isRequestAborted) {
            appendFlowLog(QString("request_aborted state=%1").arg(stateKey(authorizationState_)));
            if (authorizationState_ == AuthorizationState::ClosingSession) {
                return;
            }

            if (authorizationState_ == AuthorizationState::WaitingParameters
                || authorizationState_ == AuthorizationState::NotInitialized) {
                pendingPhoneNumberSubmission_ = !phoneNumber_.isEmpty();
                setAuthorizationState(AuthorizationState::WaitingPhoneNumber,
                                      "TDLib devolvio REQUEST_ABORTED mientras preparaba el login. Se reintentara el telefono cuando TDLib quede listo.");
                return;
            }

            pendingPhoneNumberSubmission_ = false;
            pendingBootstrapSubmission_ = true;
            pendingApiId_ = apiId_;
            pendingApiHash_ = apiHash_;
            pendingPhoneNumber_ = phoneNumber_;
            resetSession();
            return;
        }

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

    if (type == "messages") {
        const QJsonArray messages = object.value("messages").toArray();
        QString chatId = selectedChatId_;
        if (!messages.isEmpty()) {
            chatId = QString::number(jsonToInt64(messages.first().toObject().value("chat_id")));
        }

        if (!chatId.isEmpty() && chatId == selectedChatId_) {
            QStringList lines;
            QList<ChatMessageEntry> entries;
            lines.reserve(messages.size());
            entries.reserve(messages.size());
            for (int index = messages.size() - 1; index >= 0; --index) {
                const QJsonObject messageObject = messages[index].toObject();
                const ChatMessageEntry entry = chatMessageEntryFromObject(messageObject);
                lines.append(entry.line);
                entries.append(entry);
            }
            selectedChatMessages_ = lines;
            selectedChatMessageEntries_ = entries;
            emit dataChanged();
        }
        return;
    }

    if (type == "updateNewMessage") {
        const QJsonObject messageObject = object.value("message").toObject();
        const QString chatId = QString::number(jsonToInt64(messageObject.value("chat_id")));
        if (!chatId.isEmpty() && chatId == selectedChatId_) {
            const ChatMessageEntry entry = chatMessageEntryFromObject(messageObject);
            selectedChatMessages_.append(entry.line);
            selectedChatMessageEntries_.append(entry);
            emit dataChanged();
        }
        return;
    }

    if (type == "file" || type == "updateFile") {
        const QJsonObject fileObject = type == "updateFile" ? object.value("file").toObject() : object;
        const qint64 fileId = jsonToInt64(fileObject.value("id"));
        if (fileId <= 0) {
            return;
        }

        const QJsonObject localObject = fileObject.value("local").toObject();
        QString localPath = localObject.value("path").toString().trimmed();
        if (!localObject.value("is_downloading_completed").toBool(false)) {
            localPath.clear();
        }
        bool canDownload = localObject.value("can_be_downloaded").toBool(false);
        if (!localPath.isEmpty()) {
            canDownload = false;
            downloadRequestsInFlight_.remove(fileId);
        } else if (!canDownload) {
            downloadRequestsInFlight_.remove(fileId);
        }

        bool changed = false;
        for (ChatMessageEntry &entry : selectedChatMessageEntries_) {
            if (entry.fileId == fileId) {
                if (entry.localPath != localPath) {
                    entry.localPath = localPath;
                    changed = true;
                }
                if (entry.canDownload != canDownload) {
                    entry.canDownload = canDownload;
                    changed = true;
                }
            }
            if (entry.previewFileId == fileId) {
                if (entry.previewLocalPath != localPath) {
                    entry.previewLocalPath = localPath;
                    changed = true;
                }
                if (entry.previewCanDownload != canDownload) {
                    entry.previewCanDownload = canDownload;
                    changed = true;
                }
            }
        }

        if (changed) {
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

void TDLibAdapter::clearSessionData() {
    selfDisplayName_.clear();
    chatTitlesById_.clear();
    selectedChatId_.clear();
    selectedChatMessages_.clear();
    selectedChatMessageEntries_.clear();
    downloadRequestsInFlight_.clear();
    emit dataChanged();
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
    const QString sessionKey = buildSessionKey(apiId_, phoneNumber_);
    const QString profileRoot = dataRoot + "/tdlib/profiles/" + sessionKey;
    QDir().mkpath(profileRoot + "/database");
    QDir().mkpath(profileRoot + "/files");

    const std::string parametersRequest =
        std::string("{\"@type\":\"setTdlibParameters\",\"use_test_dc\":false,\"database_directory\":\"")
        + (profileRoot + "/database").toStdString()
        + "\",\"files_directory\":\""
        + (profileRoot + "/files").toStdString()
        + "\",\"database_encryption_key\":\"\",\"use_file_database\":true,\"use_chat_info_database\":true,\"use_message_database\":true,"
          "\"use_secret_chats\":false,\"api_id\":"
        + apiId_.toStdString()
        + ",\"api_hash\":\""
        + apiHash_.toStdString()
        + "\",\"system_language_code\":\"es\",\"device_model\":\"MTC\",\"system_version\":\"Linux\",\"application_version\":\"0.1.0\","
          "\"enable_storage_optimizer\":true,\"ignore_file_names\":false}";

    sendRequest(parametersRequest);
    tdlibParametersSent_ = true;
    activeSessionKey_ = sessionKey;
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
        case AuthorizationState::ClosingSession:
            return "Cerrando sesion";
        case AuthorizationState::WaitingParameters:
            return "Esperando parametros";
        case AuthorizationState::WaitingEncryptionKey:
            return "Esperando clave local";
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

QList<QPair<QString, QString>> TDLibAdapter::chatEntries() const {
    QList<QPair<QString, QString>> entries;
    for (auto it = chatTitlesById_.cbegin(); it != chatTitlesById_.cend(); ++it) {
        entries.append(qMakePair(it.key(), it.value()));
    }
    return entries;
}

QString TDLibAdapter::selectedChatId() const {
    return selectedChatId_;
}

QString TDLibAdapter::selectedChatTitle() const {
    return chatTitlesById_.value(selectedChatId_);
}

QStringList TDLibAdapter::selectedChatMessages() const {
    return selectedChatMessages_;
}

QList<ChatMessageEntry> TDLibAdapter::selectedChatMessageEntries() const {
    return selectedChatMessageEntries_;
}

void TDLibAdapter::requestChatHistory(const QString &chatId) {
    const QString trimmedChatId = chatId.trimmed();
    if (trimmedChatId.isEmpty()) {
        return;
    }

    selectedChatId_ = trimmedChatId;
    selectedChatMessages_.clear();
    selectedChatMessageEntries_.clear();
    downloadRequestsInFlight_.clear();
    emit dataChanged();

    sendRequest(std::string("{\"@type\":\"getChatHistory\",\"chat_id\":")
                + trimmedChatId.toStdString()
                + ",\"from_message_id\":0,\"offset\":0,\"limit\":50,\"only_local\":false}");
}

void TDLibAdapter::sendTextMessage(const QString &chatId, const QString &text) {
    const QString trimmedChatId = chatId.trimmed();
    const QString trimmedText = text.trimmed();
    if (trimmedChatId.isEmpty() || trimmedText.isEmpty()) {
        return;
    }

    QJsonObject inputText;
    inputText["@type"] = "formattedText";
    inputText["text"] = trimmedText;

    QJsonObject content;
    content["@type"] = "inputMessageText";
    content["text"] = inputText;
    content["clear_draft"] = false;

    QJsonObject request;
    request["@type"] = "sendMessage";
    request["chat_id"] = trimmedChatId.toLongLong();
    request["input_message_content"] = content;

    sendRequest(QJsonDocument(request).toJson(QJsonDocument::Compact).toStdString());
}

void TDLibAdapter::sendMediaMessage(const QString &chatId, const QString &filePath, const QString &caption) {
    const QString trimmedChatId = chatId.trimmed();
    const QString trimmedPath = filePath.trimmed();
    if (trimmedChatId.isEmpty() || trimmedPath.isEmpty()) {
        return;
    }

    const QString extension = QFileInfo(trimmedPath).suffix().toLower();
    const QStringList imageExtensions = {"jpg", "jpeg", "png", "bmp", "webp", "gif", "heic"};
    const QStringList videoExtensions = {"mp4", "m4v", "mov", "mkv", "webm", "avi"};
    const QStringList audioExtensions = {"mp3", "m4a", "aac", "wav", "ogg", "opus", "flac"};

    QJsonObject inputFile;
    inputFile["@type"] = "inputFileLocal";
    inputFile["path"] = trimmedPath;

    QJsonObject content;
    if (imageExtensions.contains(extension)) {
        content["@type"] = "inputMessagePhoto";
        content["photo"] = inputFile;
        content["added_sticker_file_ids"] = QJsonArray();
        content["width"] = 0;
        content["height"] = 0;
        content["caption"] = formattedText(caption.trimmed());
    } else if (videoExtensions.contains(extension)) {
        content["@type"] = "inputMessageVideo";
        content["video"] = inputFile;
        content["duration"] = 0;
        content["width"] = 0;
        content["height"] = 0;
        content["supports_streaming"] = true;
        content["caption"] = formattedText(caption.trimmed());
        content["added_sticker_file_ids"] = QJsonArray();
    } else if (audioExtensions.contains(extension)) {
        content["@type"] = "inputMessageAudio";
        content["audio"] = inputFile;
        content["duration"] = 0;
        content["title"] = QFileInfo(trimmedPath).completeBaseName();
        content["performer"] = "";
        content["caption"] = formattedText(caption.trimmed());
    } else {
        content["@type"] = "inputMessageDocument";
        content["document"] = inputFile;
        content["disable_content_type_detection"] = false;
        content["caption"] = formattedText(caption.trimmed());
    }

    QJsonObject request;
    request["@type"] = "sendMessage";
    request["chat_id"] = trimmedChatId.toLongLong();
    request["input_message_content"] = content;

    sendRequest(QJsonDocument(request).toJson(QJsonDocument::Compact).toStdString());
}

void TDLibAdapter::requestFileDownload(qint64 fileId) {
    if (fileId <= 0) {
        return;
    }
    if (downloadRequestsInFlight_.contains(fileId)) {
        return;
    }

    QJsonObject request;
    request["@type"] = "downloadFile";
    request["file_id"] = fileId;
    request["priority"] = 16;
    request["offset"] = 0;
    request["limit"] = 0;
    request["synchronous"] = false;
    downloadRequestsInFlight_.insert(fileId);
    sendRequest(QJsonDocument(request).toJson(QJsonDocument::Compact).toStdString());
    appendFlowLog(QString("download_file file_id=%1").arg(fileId));
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

QString TDLibAdapter::stateKey(AuthorizationState state) const {
    switch (state) {
        case AuthorizationState::NotInitialized:
            return "not_initialized";
        case AuthorizationState::MissingDependency:
            return "missing_dependency";
        case AuthorizationState::ClosingSession:
            return "closing_session";
        case AuthorizationState::WaitingParameters:
            return "waiting_parameters";
        case AuthorizationState::WaitingEncryptionKey:
            return "waiting_encryption_key";
        case AuthorizationState::WaitingPhoneNumber:
            return "waiting_phone";
        case AuthorizationState::WaitingCode:
            return "waiting_code";
        case AuthorizationState::WaitingPassword:
            return "waiting_password";
        case AuthorizationState::WaitingOtherDeviceConfirmation:
            return "waiting_other_device_confirmation";
        case AuthorizationState::Ready:
            return "ready";
        case AuthorizationState::Failed:
            return "failed";
    }

    return "unknown";
}

void TDLibAdapter::appendFlowLog(const QString &event) const {
    const QString dataRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataRoot);

    QFile file(dataRoot + "/tdlib_auth_flow.log");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&file);
    stream << QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
           << "Z"
           << " | "
           << event
           << "\n";
}

void TDLibAdapter::setAuthorizationState(AuthorizationState state, const QString &diagnosticMessage) {
    if (authorizationState_ == state && diagnosticMessage_ == diagnosticMessage) {
        return;
    }

    appendFlowLog(QString("state_change %1 -> %2")
                      .arg(stateKey(authorizationState_), stateKey(state)));

    authorizationState_ = state;
    diagnosticMessage_ = diagnosticMessage;
    emit stateChanged();
}

}  // namespace mtc
