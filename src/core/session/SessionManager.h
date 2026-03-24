#pragma once

#include <QList>
#include <QString>

#include <optional>
#include <string>

namespace mtc {

struct SessionProfile {
    QString id;
    QString displayName;
    QString apiId;
    QString apiHash;
    QString phoneNumber;
};

class SessionManager {
public:
    QList<SessionProfile> profiles() const;
    SessionProfile saveProfile(const QString &displayName,
                               const QString &apiId,
                               const QString &apiHash,
                               const QString &phoneNumber);
    std::optional<SessionProfile> findProfile(const QString &id) const;
    bool removeProfile(const QString &id);
    std::string status() const;

private:
    QString storagePath() const;
    bool persistProfiles(const QList<SessionProfile> &profiles) const;
};

}  // namespace mtc
