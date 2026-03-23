#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>

#include <memory>
#include <string>

class QTimer;

namespace mtc {

enum class AuthorizationState {
    NotInitialized,
    MissingDependency,
    WaitingParameters,
    WaitingPhoneNumber,
    WaitingCode,
    Ready,
    Failed,
};

class TDLibAdapter : public QObject {
    Q_OBJECT

public:
    explicit TDLibAdapter(QObject *parent = nullptr);
    ~TDLibAdapter() override;

    void initialize();
    void submitBootstrap(const QString &apiId,
                         const QString &apiHash,
                         const QString &phoneNumber);
    void submitAuthenticationCode(const QString &code);

    bool isTdLibAvailable() const;
    AuthorizationState authorizationState() const;
    QString authorizationStateLabel() const;
    QString diagnosticMessage() const;
    QString selfDisplayName() const;
    QStringList chatTitles() const;
    std::string status() const;

signals:
    void stateChanged();
    void dataChanged();

private:
    void sendRequest(const std::string &request);
    void pollResponses();
    void handleResponse(const char *response);
    void requestInitialData();
    void submitTdlibParameters();
    void setAuthorizationState(AuthorizationState state, const QString &diagnosticMessage);

    bool tdLibAvailable_ = false;
    AuthorizationState authorizationState_ = AuthorizationState::NotInitialized;
    QString diagnosticMessage_;
    QString apiId_;
    QString apiHash_;
    QString phoneNumber_;
    QString selfDisplayName_;
    QMap<QString, QString> chatTitlesById_;
    bool tdlibParametersSent_ = false;
    bool initialDataRequested_ = false;
    void *tdJsonClient_ = nullptr;
    std::unique_ptr<QTimer> pollTimer_;
};

}  // namespace mtc
