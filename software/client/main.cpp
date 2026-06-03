#include <QApplication>
#include <QDebug>
#include "presentation/ui/main_window/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qDebug() << "启动 EdgeXGuard 客户端...";

    MainWindow window;
    window.show();

    return app.exec();
}