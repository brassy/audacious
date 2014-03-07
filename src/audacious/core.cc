#include <QtCore>
#include <QApplication>

#include "core.h"
#include "glib_thread.h"
#include "ui_main_widget.h"

using namespace std;

int core_init(int argc, char * * argv) {
    QApplication app(argc, argv);

    GlibThread *thread = new GlibThread;
    thread->start();

    UIMainWidget widget;
    widget.show();

    qDebug() << "Qt GUI" << QThread::currentThread();

    return app.exec();
}
