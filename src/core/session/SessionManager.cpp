#include "core/session/SessionManager.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUuid>

#include <sstream>

namespace mtc {

namespace {

QJsonObject toJson(const SessionProfile &profile) {
    QJsonObject object;
    object["id"] = profile.id;
    object["display_name"] = profile.displayName;
    object["api_id"] = profile.apiId;
    object["api_hash"] = profile.apiHash;
    object["phone_number"] = profile.phoneNumber;
    return object;
}

SessionProfile fromJson(const QJsonObject &object) {
    SessionProfile profile;
    profile.id = object.value("id").toString();
    profile.displayName = object.value("display_name").toString();
    profile.apiId = object.value("api_id").toString();
    profile.apiHash = object.value("api_hash").toString();
    profile.phoneNumber = object.value("phone_number").toString();
    return profile;
}

QString profileTitle(const SessionProfile &profile) {
    if (!profile.displayName.trimmed().isEmpty()) {
        return profile.displayName.trimmed();
    }

    if (!profile.phoneNumber.trimmed().isEmpty()) {
        return profile.phoneNumber.trimmed();
    }

    return QStringLiteral("Perfil sin nombre");
}

}  // namespace

QList<SessionProfile> SessionManager::profiles() const {
    QFile file(storagePath());
    if (!file.exists()) {
        return {};
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isArray()) {
        return {};
    }

    QList<SessionProfile> result;
    const QJsonArray array = document.array();
    for (const QJsonValue &value : array) {
        if (!value.isObject()) {
            continue;
        }

        SessionProfile profile = fromJson(value.toObject());
        if (profile.id.trimmed().isEmpty()) {
            profile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        }
        result.append(profile);
    }

    return result;
}

SessionProfile SessionManager::saveProfile(const QString &displayName,
                                           const QString &apiId,
                                           const QString &apiHash,
                                           const QString &phoneNumber) {
    QList<SessionProfile> currentProfiles = profiles();

    SessionProfile profile;
    profile.displayName = displayName.trimmed();
    profile.apiId = apiId.trimmed();
    profile.apiHash = apiHash.trimmed();
    profile.phoneNumber = phoneNumber.trimmed();

    int existingIndex = -1;
    if (!profile.phoneNumber.isEmpty()) {
        for (int index = 0; index < currentProfiles.size(); ++index) {
            if (currentProfiles[index].phoneNumber.trimmed() == profile.phoneNumber) {
                existingIndex = index;
                break;
            }
        }
    }

    if (existingIndex >= 0) {
        profile.id = currentProfiles[existingIndex].id;
        currentProfiles[existingIndex] = profile;
    } else {
        profile.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        currentProfiles.append(profile);
    }

    persistProfiles(currentProfiles);
    return profile;
}

std::optional<SessionProfile> SessionManager::findProfile(const QString &id) const {
    const QList<SessionProfile> currentProfiles = profiles();
    for (const SessionProfile &profile : currentProfiles) {
        if (profile.id == id) {
            return profile;
        }
    }

    return std::nullopt;
}

bool SessionManager::removeProfile(const QString &id) {
    QList<SessionProfile> currentProfiles = profiles();

    for (int index = 0; index < currentProfiles.size(); ++index) {
        if (currentProfiles[index].id == id) {
            currentProfiles.removeAt(index);
            return persistProfiles(currentProfiles);
        }
    }

    return false;
}

std::string SessionManager::status() const {
    const QList<SessionProfile> currentProfiles = profiles();

    std::ostringstream stream;
    stream << "SessionManager: " << currentProfiles.size() << " perfil(es) local(es) disponibles";
    if (!currentProfiles.isEmpty()) {
        stream << ". Ultimo perfil: " << profileTitle(currentProfiles.back()).toStdString();
    } else {
        stream << ". Aun no hay perfiles guardados.";
    }

    return stream.str();
}

QString SessionManager::storagePath() const {
    const QString dataRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataRoot);
    return dataRoot + "/session_profiles.json";
}

bool SessionManager::persistProfiles(const QList<SessionProfile> &profiles) const {
    const QString path = storagePath();
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QJsonArray array;
    for (const SessionProfile &profile : profiles) {
        array.append(toJson(profile));
    }

    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    return true;
}

}  // namespace mtc
