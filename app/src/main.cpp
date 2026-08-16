#include <QApplication>

#include "main_window.hpp"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("ProjectApogee");
    QApplication::setApplicationName("ApogeeStudio");

    apogee::app::MainWindow window;
    window.show();

    return QApplication::exec();
}
