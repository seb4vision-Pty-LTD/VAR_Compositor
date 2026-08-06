#include "VAR_Compositor_Control_Application.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    VAR_Compositor_Control_Application window;
    window.show();
    return app.exec();
}
