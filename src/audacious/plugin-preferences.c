/*
 * plugin-preferences.c
 * Copyright 2012-2013 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include <libaudcore/audstrings.h>
#ifdef USE_GTK
#include <libaudgui/libaudgui-gtk.h>
#endif

#include "i18n.h"
#include "misc.h"
#include "plugin.h"
#include "plugins.h"
#include "preferences.h"
#ifdef USE_GTK
#include "ui_preferences.h"
#endif

typedef struct {
#ifdef USE_GTK
    GtkWidget * about_window;
    GtkWidget * config_window;
#else
    void * about_window;
    void * config_window;
#endif
} PluginMiscData;

void plugin_make_about_window (PluginHandle * plugin)
{
    PluginMiscData * misc = plugin_get_misc_data (plugin, sizeof (PluginMiscData));
    Plugin * header = plugin_get_header (plugin);

    if (misc->about_window)
    {
#ifdef USE_GTK
        gtk_window_present ((GtkWindow *) misc->about_window);
#endif
        return;
    }

    const char * name = header->name;
    const char * text = header->about_text;

    if (PLUGIN_HAS_FUNC (header, domain))
    {
        name = dgettext (header->domain, name);
        text = dgettext (header->domain, text);
    }

#ifdef USE_GTK
    SCONCAT2 (title, _("About "), name);
    audgui_simple_message (& misc->about_window, GTK_MESSAGE_INFO, title, text);
#endif
}

#ifdef USE_GTK
static void response_cb (GtkWidget * window, int response, const PluginPreferences * p)
{
    if (response == GTK_RESPONSE_OK && p->apply)
        p->apply ();

    gtk_widget_destroy (window);
}

static void destroy_cb (GtkWidget * window, const PluginPreferences * p)
{
    if (p->cleanup)
        p->cleanup ();
}
#endif

void plugin_make_config_window (PluginHandle * plugin)
{
#ifdef USE_GTK
    PluginMiscData * misc = plugin_get_misc_data (plugin, sizeof (PluginMiscData));
    Plugin * header = plugin_get_header (plugin);
    const PluginPreferences * p = header->prefs;

    if (misc->config_window)
    {
        gtk_window_present ((GtkWindow *) misc->config_window);
        return;
    }

    if (p->init)
        p->init ();

    const char * name = header->name;
    if (PLUGIN_HAS_FUNC (header, domain))
        name = dgettext (header->domain, header->name);

    GtkWidget * window = gtk_dialog_new ();

    SCONCAT2 (title, name, _(" Settings"));
    gtk_window_set_title ((GtkWindow *) window, title);

    if (p->apply)
    {
        GtkWidget * button1 = audgui_button_new (_("_Set"), "system-run", NULL, NULL);
        GtkWidget * button2 = audgui_button_new (_("_Cancel"), "process-stop", NULL, NULL);
        gtk_dialog_add_action_widget ((GtkDialog *) window, button2, GTK_RESPONSE_CANCEL);
        gtk_dialog_add_action_widget ((GtkDialog *) window, button1, GTK_RESPONSE_OK);
    }
    else
    {
        GtkWidget * button = audgui_button_new (_("_Close"), "window-close", NULL, NULL);
        gtk_dialog_add_action_widget ((GtkDialog *) window, button, GTK_RESPONSE_CLOSE);
    }

    GtkWidget * content = gtk_dialog_get_content_area ((GtkDialog *) window);
    GtkWidget * box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    create_widgets_with_domain (box, p->widgets, p->n_widgets, header->domain);
    gtk_box_pack_start ((GtkBox *) content, box, TRUE, TRUE, 0);

    g_signal_connect (window, "response", (GCallback) response_cb, (void *) p);
    g_signal_connect (window, "destroy", (GCallback) destroy_cb, (void *) p);

    misc->config_window = window;
    g_signal_connect (window, "destroy", (GCallback) gtk_widget_destroyed, & misc->config_window);

    gtk_widget_show_all (window);
#endif
}

void plugin_misc_cleanup (PluginHandle * plugin)
{
#ifdef USE_GTK
    PluginMiscData * misc = plugin_get_misc_data (plugin, sizeof (PluginMiscData));

    if (misc->about_window)
        gtk_widget_destroy (misc->about_window);
    if (misc->config_window)
        gtk_widget_destroy (misc->config_window);
#endif
}
