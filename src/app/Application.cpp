#include "app/Application.h"

#include "core/analytics/AnalyticsStore.h"
#include "core/calls/CallController.h"
#include "core/devices/DeviceManager.h"
#include "core/session/SessionManager.h"
#include "core/telegram/TDLibAdapter.h"
#include "ui/MainWindow.h"

namespace mtc {

Application::Application() = default;
Application::~Application() = default;

void Application::initialize() {
    sessionManager_ = std::make_unique<SessionManager>();
    tdLibAdapter_ = std::make_unique<TDLibAdapter>();
    callController_ = std::make_unique<CallController>();
    deviceManager_ = std::make_unique<DeviceManager>();
    analyticsStore_ = std::make_unique<AnalyticsStore>();
    tdLibAdapter_->initialize();
    mainWindow_ = std::make_unique<MainWindow>(*sessionManager_,
                                               *tdLibAdapter_,
                                               *callController_,
                                               *deviceManager_,
                                               *analyticsStore_);
}

void Application::show() {
    if (mainWindow_) {
        mainWindow_->show();
    }
}

}  // namespace mtc
