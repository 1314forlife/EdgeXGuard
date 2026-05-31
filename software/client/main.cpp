#include <QApplication>
#include "presentation/ui/main_window/MainWindow.h"
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}