#ifndef UI_MAIN_WIDGET_H
#define UI_MAIN_WIDGET_H

#include <QtCore>
#include <QWidget>

extern "C" {
    #include "drct.h"
    #include <libaudcore/core.h>
    #include <libaudcore/index.h>
    #include <libaudcore/hook.h>
    #include <glib.h>
}

class UIMainWidget : public QWidget
{
    Q_OBJECT

    public:
        UIMainWidget();

    public slots:
        void openSlot();
        void playSlot();
        void stopSlot();
        void pauseSlot();
        void prevSlot();
        void nextSlot();

    protected:

        static gboolean open_cb(void *data) {
            drct_pl_open_list((Index *) data);
            return FALSE;
        }

        static gboolean play_cb(void *data) {
            drct_play();
            return FALSE;
        }

        static gboolean stop_cb(void *data) {
            drct_stop();
            return FALSE;
        }

        static gboolean pause_cb(void *data) {
            drct_pause();
            return FALSE;
        }

        static gboolean prev_cb(void *data) {
            drct_pl_prev();
            return FALSE;
        }

        static gboolean next_cb(void *data) {
            drct_pl_next();
            return FALSE;
        }
};

#endif
