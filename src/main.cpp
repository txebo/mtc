#include <QApplication>

#include "app/Application.h"

int main(int argc, char *argv[]) {
    QApplication qtApp(argc, argv);

    mtc::Application application;
    application.initialize();
    application.show();

    return QApplication::exec();
}

