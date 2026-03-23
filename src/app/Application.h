#pragma once

#include <memory>

namespace mtc {

class SessionManager;
class TDLibAdapter;
class CallController;
class DeviceManager;
class AnalyticsStore;
class MainWindow;

class Application {
public:
    Application();
    ~Application();

    void initialize();
    void show();

private:
    std::unique_ptr<SessionManager> sessionManager_;
    std::unique_ptr<TDLibAdapter> tdLibAdapter_;
    std::unique_ptr<CallController> callController_;
    std::unique_ptr<DeviceManager> deviceManager_;
    std::unique_ptr<AnalyticsStore> analyticsStore_;
    std::unique_ptr<MainWindow> mainWindow_;
};

}  // namespace mtc

