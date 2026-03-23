#pragma once

#include <QMainWindow>

namespace mtc {

class SessionManager;
class TDLibAdapter;
class CallController;
class DeviceManager;
class AnalyticsStore;

class MainWindow : public QMainWindow {
public:
    MainWindow(SessionManager &sessionManager,
               TDLibAdapter &tdLibAdapter,
               CallController &callController,
               DeviceManager &deviceManager,
               AnalyticsStore &analyticsStore,
               QWidget *parent = nullptr);

private:
    SessionManager &sessionManager_;
    TDLibAdapter &tdLibAdapter_;
    CallController &callController_;
    DeviceManager &deviceManager_;
    AnalyticsStore &analyticsStore_;
};

}  // namespace mtc

