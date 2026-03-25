#pragma once

#include <QObject>
#include <QMap>
#include <QPair>
#include <QString>
#include <QStringList>

#include <memory>
#include <string>

class QTimer;

namespace mtc {

enum class AuthorizationState {
    NotInitialized,
    MissingDependency,
    ClosingSession,
    WaitingParameters,
    WaitingEncryptionKey,
    WaitingPhoneNumber,
    WaitingCode,
    WaitingPassword,
    WaitingOtherDeviceConfirmation,
    Ready,
    Failed,
};

class TDLibAdapter : public QObject {
    Q_OBJECT

public:
    explicit TDLibAdapter(QObject *parent = nullptr);
    ~TDLibAdapter() override;

    void initialize();
    void continueAuthorization(const QString &apiId,
                               const QString &apiHash,
                               const QString &phoneNumber);
    void submitBootstrap(const QString &apiId,
                         const QString &apiHash,
                         const QString &phoneNumber);
    void submitPhoneNumber(const QString &phoneNumber);
    void submitAuthenticationCode(const QString &code);
    void submitAuthenticationPassword(const QString &password);
    void logout();
    void resetSession();

    bool isTdLibAvailable() const;
    AuthorizationState authorizationState() const;
    QString authorizationStateLabel() const;
    QString diagnosticMessage() const;
    QString selfDisplayName() const;
    QStringList chatTitles() const;
    QList<QPair<QString, QString>> chatEntries() const;
    QString selectedChatId() const;
    QString selectedChatTitle() const;
    QStringList selectedChatMessages() const;
    void requestChatHistory(const QString &chatId);
    void sendTextMessage(const QString &chatId, const QString &text);
    std::string status() const;

signals:
    void stateChanged();
    void dataChanged();

private:
    void sendRequest(const std::string &request);
    void sendPhoneNumberRequest();
    QString buildSessionKey(const QString &apiId, const QString &phoneNumber) const;
    void handleCloseSessionTimeout();
    void pollResponses();
    void handleResponse(const char *response);
    void clearSessionData();
    void requestInitialData();
    void submitTdlibParameters();
    QString extractTdLibErrorMessage(const QJsonObject &object) const;
    QString stateKey(AuthorizationState state) const;
    void appendFlowLog(const QString &event) const;
    void setAuthorizationState(AuthorizationState state, const QString &diagnosticMessage);

    bool tdLibAvailable_ = false;
    AuthorizationState authorizationState_ = AuthorizationState::NotInitialized;
    QString diagnosticMessage_;
    QString apiId_;
    QString apiHash_;
    QString phoneNumber_;
    QString selfDisplayName_;
    QString activeSessionKey_;
    QMap<QString, QString> chatTitlesById_;
    QString selectedChatId_;
    QStringList selectedChatMessages_;
    bool tdlibParametersSent_ = false;
    bool initialDataRequested_ = false;
    bool pendingPhoneNumberSubmission_ = false;
    bool phoneNumberRequestInFlight_ = false;
    bool pendingBootstrapSubmission_ = false;
    QString pendingApiId_;
    QString pendingApiHash_;
    QString pendingPhoneNumber_;
    void *tdJsonClient_ = nullptr;
    std::unique_ptr<QTimer> pollTimer_;
    std::unique_ptr<QTimer> closeSessionTimer_;
};

}  // namespace mtc
