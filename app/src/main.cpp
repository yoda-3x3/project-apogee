#include <QApplication>

#include "main_window.hpp"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("ProjectApogee");
    QApplication::setApplicationName("ApogeeStudio");
    // seed.qrc lives in rocket_data, a STATIC library -- without this, its
    // resource-registering initializer never gets linked in and
    // :/seed/*.json silently doesn't exist at runtime (see tests/main.cpp
    // and tools/seed_tool/main.cpp for the same fix, and Phase 2's commit
    // message for how this was originally diagnosed).
    Q_INIT_RESOURCE(seed);

    apogee::app::MainWindow window;
    window.show();

    return QApplication::exec();
}
