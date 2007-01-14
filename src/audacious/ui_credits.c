/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious Team
 *
 *  Based on BMP:
 *  Copyright (C) 2003-2004  BMP development team
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "ui_credits.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "audacious_logo.xpm"


enum {
    COL_LEFT,
    COL_RIGHT,
    N_COLS
};


static const gchar *bmp_brief =
    N_("<big><b>Audacious %s</b></big>\n"
       "The future of UNIX multimedia.\n"
       "\n"
       "Copyright (C) 2005-2007 Audacious Development Team\n");

static const gchar *credit_text[] = {
    N_("Audacious core developers:"),
    "George Averill",
    "Daniel Bradshaw",
    "Adam Cecile",
    "Troels Bang Jensen",
    "Giacomo Lozito",
    "William Pitcock",
    "Derek Pomery",
    "Jonathan Schleifer",
    "Tony Vroon",
    "Yoshiki Yazawa",
    NULL,

    N_("Graphics:"),
    "George Averill",
    "Stephan Sokolow",
    NULL,

    N_("Default skin:"),
    "George Averill",
    "William Pitcock",
    NULL,

    N_("Plugin development:"),
    "Kiyoshi Aman",
    "Luca Barbato",
    "Shay Green",
    "Giacomo Lozito",
    "William Pitcock",
    "Derek Pomery",
    "Jonathan Schleifer",
    "Tony Vroon",
    NULL,

    N_("Patch authors:"),
    "Laszlo Dvornik",
    "Ralf Ertzinger",
    "Mike Frysinger",
    "Chris Kehler",
    "Michael Hanselmann",
    "Henrik Johansson",
    "Rodrigo Martins de Matos Ventura",
    "Diego Pettenò",
    "Kazuki Shimura",
    "Valentine Sinitsyn",
    "Johan Tavelin",
    "Tim Yamin",
    NULL,

    N_("0.1.x developers:"),
    "William Pitcock",
    "Mohammed Sameer",
    "Tony Vroon",
    NULL,

    N_("BMP Developers:"),
    "Artem Baguinski",
    "Edward Brocklesby",
    "Chong Kai Xiong",
    "Milosz Derezynski",
    "David Lau",
    "Ole Andre Vadla Ravnaas",
    "Michiel Sikkes",
    "Andrei Badea",
    "Peter Behroozi",
    "Bernard Blackham",
    "Oliver Blin",
    "Tomas Bzatek",
    "Liviu Danicel",
    "Jon Dowland",
    "Artur Frysiak",
    "Sebastian Kapfer",
    "Lukas Koberstein",
    "Dan Korostelev",
    "Jolan Luff",
    "Michael Marineau",
    "Tim-Philipp Muller",
    "Julien Portalier",
    "Andrew Ruder",
    "Olivier Samyn",
    "Martijn Vernooij",
    NULL,

    NULL
};

static const gchar *translators[] = {
    N_("Brazilian Portuguese:"),
    "Philipi Pinto",
    NULL,
    N_("Breton:"),
    "Thierry Vignaud",
    NULL,
    N_("Bulgarian:"),
    "Andrew Ivanov",
    NULL,
    N_("Czech:"),
    "Jan Narovec",
    NULL,
    N_("Dutch:"),
    "Laurens Buhler",
    "Tony Vroon",
    NULL,
    N_("Finnish:"),
    "Pauli Virtanen",
    NULL,
    N_("French:"),
    "Adam Cecile",
    NULL,
    N_("German:"),
    "Michael Hanselmann",
    "Matthias Debus",
    NULL,
    N_("Georgian:"),
    "George Machitidze",
    NULL,
    N_("Greek:"),
    "Kouzinopoulos Haris",
    "Stavros Giannouris",
    "Stathis Kamperis",
    NULL,
    N_("Hindi:"),
    "Dhananjaya Sharma",
    NULL,
    N_("Hungarian:"),
    "Laszlo Dvornik",
    NULL,
    N_("Italian:"),
    "Alessio D'Ascanio",
    "Diego Pettenò",
    NULL,
    N_("Japanese:"),
    "Dai",
    NULL,
    N_("Korean:"),
    "DongCheon Park",
    NULL,
    N_("Lithuanian:"),
    "Rimas Kudelis",
    NULL,
    N_("Macedonian:"),
    "Arangel Angov",
    NULL,
    N_("Polish:"),
    "Wojciech Myrda",
    NULL,
    N_("Romanian:"),
    "Daniel Patriche",
    NULL,
    N_("Russian:"),
    "Alexandr Orlov",
    NULL,
    N_("Serbian (Latin):"),
    "Strahinja Kustudić",
    NULL,
    N_("Serbian (Cyrillic):"),
    "Strahinja Kustudić",
    NULL,
    N_("Simplified Chinese:"),
    "Yang Zhang",
    NULL,
    N_("Slovak:"),
    "Pavel Kanzelsberger",
    NULL,
    N_("Spanish:"),
    "Gustavo D. Vranjes",
    NULL,
    N_("Swedish:"),
    "Martin Persenius",
    NULL,
    N_("Traditional Chinese:"),
    "Cheng-Wei Chien",
    NULL,
    N_("Ukrainian:"),
    "Mykola Lynnyk",
    NULL,
    N_("Welsh:"),
    "Edward Brocklesby",
    "William Pitcock",
    NULL,

    NULL
};


static GtkWidget *
generate_credit_list(const gchar * text[], gboolean sec_space)
{
    GtkWidget *scrollwin;
    GtkWidget *treeview;
    GtkListStore *list_store;
    GtkTreeIter iter;
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    const gchar *const *item;

    list_store = gtk_list_store_new(N_COLS, G_TYPE_STRING, G_TYPE_STRING);

    item = text;

    while (*item) {
        gtk_list_store_append(list_store, &iter);
        gtk_list_store_set(list_store, &iter,
                           COL_LEFT, _(item[0]), COL_RIGHT, _(item[1]), -1);
        item += 2;

        while (*item) {
            gtk_list_store_append(list_store, &iter);
            gtk_list_store_set(list_store, &iter,
                               COL_LEFT, "", COL_RIGHT, _(*item++), -1);
        }

        ++item;

        if (*item && sec_space) {
            gtk_list_store_append(list_store, &iter);
            gtk_list_store_set(list_store, &iter,
                               COL_LEFT, "", COL_RIGHT, "", -1);
        }
    }

    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(treeview), FALSE);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)),
        GTK_SELECTION_NONE);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "xalign", 1.0, NULL);
    column = gtk_tree_view_column_new_with_attributes("Left", renderer,
                                                      "text", COL_LEFT, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "xalign", 0.0, NULL);
    column = gtk_tree_view_column_new_with_attributes("Right", renderer,
                                                      "text", COL_RIGHT,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    scrollwin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
                                   GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollwin), GTK_SHADOW_IN);
    gtk_container_add(GTK_CONTAINER(scrollwin), treeview);
    gtk_container_set_border_width(GTK_CONTAINER(scrollwin), 10);

    gtk_widget_show_all(scrollwin);

    return scrollwin;
}

void
show_credits_window(void)
{
    static GtkWidget *about_window = NULL;

    GdkPixmap *beep_logo_pmap = NULL, *beep_logo_mask = NULL;
    GtkWidget *about_vbox;
    GtkWidget *about_credits_logo_box, *about_credits_logo_frame;
    GtkWidget *about_credits_logo;
    GtkWidget *about_notebook;
    GtkWidget *list;
    GtkWidget *bbox, *close_btn;
    GtkWidget *label;
    gchar *text;

    if (about_window)
        return;

    about_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(about_window),
                             GDK_WINDOW_TYPE_HINT_DIALOG);

    gtk_window_set_default_size(GTK_WINDOW(about_window), -1, 512);
    gtk_window_set_title(GTK_WINDOW(about_window), _("About Audacious"));
    gtk_window_set_position(GTK_WINDOW(about_window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(about_window), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(about_window), 10);

    g_signal_connect(about_window, "destroy",
                     G_CALLBACK(gtk_widget_destroyed), &about_window);

    gtk_widget_realize(about_window);

    about_vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_add(GTK_CONTAINER(about_window), about_vbox);

    if (!beep_logo_pmap)
        beep_logo_pmap =
            gdk_pixmap_create_from_xpm_d(about_window->window,
                                         &beep_logo_mask, NULL, audacious_logo_xpm);

    about_credits_logo_box = gtk_hbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(about_vbox), about_credits_logo_box,
                       FALSE, FALSE, 0);

    about_credits_logo_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(about_credits_logo_frame),
                              GTK_SHADOW_ETCHED_OUT);
    gtk_box_pack_start(GTK_BOX(about_credits_logo_box),
                       about_credits_logo_frame, FALSE, FALSE, 0);

    about_credits_logo = gtk_pixmap_new(beep_logo_pmap, beep_logo_mask);
    gtk_container_add(GTK_CONTAINER(about_credits_logo_frame),
                      about_credits_logo);

    label = gtk_label_new(NULL);
    text = g_strdup_printf(_(bmp_brief), VERSION);
    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    g_free(text);

    gtk_box_pack_start(GTK_BOX(about_vbox), label, FALSE, FALSE, 0);

    about_notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(about_vbox), about_notebook, TRUE, TRUE, 0);

    list = generate_credit_list(credit_text, TRUE);
    gtk_notebook_append_page(GTK_NOTEBOOK(about_notebook), list,
                             gtk_label_new(_("Credits")));

    list = generate_credit_list(translators, FALSE);
    gtk_notebook_append_page(GTK_NOTEBOOK(about_notebook), list,
                             gtk_label_new(_("Translators")));

    bbox = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
    gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
    gtk_box_pack_start(GTK_BOX(about_vbox), bbox, FALSE, FALSE, 0);

    close_btn = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    g_signal_connect_swapped(close_btn, "clicked",
                             G_CALLBACK(gtk_widget_destroy), about_window);
    GTK_WIDGET_SET_FLAGS(close_btn, GTK_CAN_DEFAULT);
    gtk_box_pack_start(GTK_BOX(bbox), close_btn, TRUE, TRUE, 0);
    gtk_widget_grab_default(close_btn);

    gtk_widget_show_all(about_window);
}