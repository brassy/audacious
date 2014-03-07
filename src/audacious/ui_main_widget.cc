#include <QtGui>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QUrl>

#include "ui_main_widget.h"
#include "ui_main_widget.moc"

UIMainWidget::UIMainWidget() {
    QPushButton *open  = new QPushButton("Open");
    QPushButton *play  = new QPushButton("Play");
    QPushButton *stop  = new QPushButton("Stop");
    QPushButton *pause = new QPushButton("Pause");
    QPushButton *prev  = new QPushButton("Prev");
    QPushButton *next  = new QPushButton("Next");

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(open);
    layout->addWidget(play);
    layout->addWidget(stop);
    layout->addWidget(pause);
    layout->addWidget(prev);
    layout->addWidget(next);
    this->setLayout(layout);

    connect(open, SIGNAL(clicked()), this, SLOT(openSlot()));
    connect(play, SIGNAL(clicked()), this, SLOT(playSlot()));
    connect(stop, SIGNAL(clicked()), this, SLOT(stopSlot()));
    connect(pause, SIGNAL(clicked()), this, SLOT(pauseSlot()));
    connect(prev, SIGNAL(clicked()), this, SLOT(prevSlot()));
    connect(next, SIGNAL(clicked()), this, SLOT(nextSlot()));
}

void UIMainWidget::openSlot() {
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    if (dialog.exec()) {
        Index *files = index_new();
        Q_FOREACH (QUrl url, dialog.selectedUrls()) {
            const char *tmp = url.toEncoded().constData();
            qDebug() << "Opening:" << tmp << endl;
            index_insert(files, -1, str_get(tmp));
            g_idle_add(open_cb, files);
        }
    }
}

void UIMainWidget::playSlot() {
    g_idle_add(play_cb, NULL);
}

void UIMainWidget::stopSlot() {
    g_idle_add(stop_cb, NULL);
}

void UIMainWidget::pauseSlot() {
    g_idle_add(pause_cb, NULL);
}

void UIMainWidget::prevSlot() {
    g_idle_add(prev_cb, NULL);
}

void UIMainWidget::nextSlot() {
    g_idle_add(next_cb, NULL);
}
