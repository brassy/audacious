/*
 * main.c
 * Copyright 2007-2013 William Pitcock and John Lindgren
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

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <locale.h>

#ifdef USE_GTK
#include <gtk/gtk.h>
#else
#include <glib.h>
#endif

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#ifdef USE_GTK
#include <libaudgui/libaudgui.h>
#endif
#include <libaudtag/audtag.h>

#ifdef USE_DBUS
#include "aud-dbus.h"
#endif

#include "debug.h"
#include "drct.h"
#include "equalizer.h"
#include "i18n.h"
#include "interface.h"
#include "main.h"
#include "misc.h"
#include "playlist.h"
#include "plugins.h"
#include "scanner.h"
#include "util.h"
#include "api.h"

#define AUTOSAVE_INTERVAL 300 /* seconds */

#ifndef USE_GTK
static GMainLoop * main_loop = NULL;
#endif

#ifdef LIBAUDACIOUS
extern AudAPITable api_table;
#endif

static struct {
    char **filenames;
    int session;
    bool_t play, stop, pause, fwd, rew, play_pause, show_jump_box;
    bool_t enqueue, mainwin, headless;
    bool_t enqueue_to_temp;
    bool_t quit_after_play;
    bool_t version;
    bool_t verbose;
    char *previous_session_id;
} options;

static char * aud_paths[AUD_PATH_COUNT];

static void make_dirs(void)
{
#ifdef S_IRGRP
    const mode_t mode755 = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
#else
    const mode_t mode755 = S_IRWXU;
#endif

    make_directory(aud_paths[AUD_PATH_USER_DIR], mode755);
    make_directory(aud_paths[AUD_PATH_PLAYLISTS_DIR], mode755);
}

static char * relocate_path (const char * path, const char * old, const char * new)
{
    int oldlen = strlen (old);
    int newlen = strlen (new);

    if (oldlen && old[oldlen - 1] == G_DIR_SEPARATOR)
        oldlen --;
    if (newlen && new[newlen - 1] == G_DIR_SEPARATOR)
        newlen --;

#ifdef _WIN32
    if (g_ascii_strncasecmp (path, old, oldlen) || (path[oldlen] && path[oldlen] != G_DIR_SEPARATOR))
#else
    if (strncmp (path, old, oldlen) || (path[oldlen] && path[oldlen] != G_DIR_SEPARATOR))
#endif
    {
        fprintf (stderr, "Failed to relocate a data path.  Falling back to "
         "compile-time path: %s\n", path);
        return str_get (path);
    }

    return str_printf ("%.*s%s", newlen, new, path + oldlen);
}

static void relocate_paths (void)
{
    char bindir[] = HARDCODE_BINDIR;
    char datadir[] = HARDCODE_DATADIR;
    char plugindir[] = HARDCODE_PLUGINDIR;
    char localedir[] = HARDCODE_LOCALEDIR;
    char desktopfile[] = HARDCODE_DESKTOPFILE;
    char iconfile[] = HARDCODE_ICONFILE;

    filename_normalize (bindir);
    filename_normalize (datadir);
    filename_normalize (plugindir);
    filename_normalize (localedir);
    filename_normalize (desktopfile);
    filename_normalize (iconfile);

    /* Compare the compile-time path to the executable and the actual path to
     * see if we have been moved. */
    char * self = get_path_to_self ();
    if (! self)
    {
FALLBACK:
        /* Fall back to compile-time paths. */
        aud_paths[AUD_PATH_BIN_DIR] = str_get (bindir);
        aud_paths[AUD_PATH_DATA_DIR] = str_get (datadir);
        aud_paths[AUD_PATH_PLUGIN_DIR] = str_get (plugindir);
        aud_paths[AUD_PATH_LOCALE_DIR] = str_get (localedir);
        aud_paths[AUD_PATH_DESKTOP_FILE] = str_get (desktopfile);
        aud_paths[AUD_PATH_ICON_FILE] = str_get (iconfile);

        return;
    }

    SCOPY (old, bindir);
    SCOPY (new, self);

    str_unref (self);

    filename_normalize (new);

    /* Strip the name of the executable file, leaving the path. */
    char * base = last_path_element (new);
    if (! base)
        goto FALLBACK;

    cut_path_element (new, base);

    /* Strip innermost folder names from both paths as long as they match.  This
     * leaves a compile-time prefix and a run-time one to replace it with. */
    char * a, * b;
    while ((a = last_path_element (old)) && (b = last_path_element (new)) &&
#ifdef _WIN32
     ! g_ascii_strcasecmp (a, b))
#else
     ! strcmp (a, b))
#endif
    {
        cut_path_element (old, a);
        cut_path_element (new, b);
    }

    /* Do the replacements. */
    aud_paths[AUD_PATH_BIN_DIR] = relocate_path (bindir, old, new);
    aud_paths[AUD_PATH_DATA_DIR] = relocate_path (datadir, old, new);
    aud_paths[AUD_PATH_PLUGIN_DIR] = relocate_path (plugindir, old, new);
    aud_paths[AUD_PATH_LOCALE_DIR] = relocate_path (localedir, old, new);
    aud_paths[AUD_PATH_DESKTOP_FILE] = relocate_path (desktopfile, old, new);
    aud_paths[AUD_PATH_ICON_FILE] = relocate_path (iconfile, old, new);
}

static void init_paths (void)
{
    relocate_paths ();

    const char * xdg_config_home = g_get_user_config_dir ();

    aud_paths[AUD_PATH_USER_DIR] = filename_build (xdg_config_home, "audacious");
    aud_paths[AUD_PATH_PLAYLISTS_DIR] = filename_build (aud_paths[AUD_PATH_USER_DIR], "playlists");

#ifdef _WIN32
    /* Some libraries (libmcs) and plugins (filewriter) use these variables,
     * which are generally not set on Windows. */
    g_setenv ("HOME", g_get_home_dir (), TRUE);
    g_setenv ("XDG_CONFIG_HOME", xdg_config_home, TRUE);
    g_setenv ("XDG_DATA_HOME", g_get_user_data_dir (), TRUE);
    g_setenv ("XDG_CACHE_HOME", g_get_user_cache_dir (), TRUE);
#endif
}

static void free_paths (void)
{
    for (int i = 0; i < AUD_PATH_COUNT; i ++)
        str_unref (aud_paths[i]);
}

const char * get_path (int id)
{
    g_return_val_if_fail (id >= 0 && id < AUD_PATH_COUNT, NULL);
    return aud_paths[id];
}

static GOptionEntry cmd_entries[] = {
    {"rew", 'r', 0, G_OPTION_ARG_NONE, &options.rew, N_("Skip backwards in playlist"), NULL},
    {"play", 'p', 0, G_OPTION_ARG_NONE, &options.play, N_("Start playing current playlist"), NULL},
    {"pause", 'u', 0, G_OPTION_ARG_NONE, &options.pause, N_("Pause current song"), NULL},
    {"stop", 's', 0, G_OPTION_ARG_NONE, &options.stop, N_("Stop current song"), NULL},
    {"play-pause", 't', 0, G_OPTION_ARG_NONE, &options.play_pause, N_("Pause if playing, play otherwise"), NULL},
    {"fwd", 'f', 0, G_OPTION_ARG_NONE, &options.fwd, N_("Skip forward in playlist"), NULL},
    {"show-jump-box", 'j', 0, G_OPTION_ARG_NONE, &options.show_jump_box, N_("Display Jump to File dialog"), NULL},
    {"enqueue", 'e', 0, G_OPTION_ARG_NONE, &options.enqueue, N_("Add files to the playlist"), NULL},
    {"enqueue-to-temp", 'E', 0, G_OPTION_ARG_NONE, &options.enqueue_to_temp, N_("Add new files to a temporary playlist"), NULL},
    {"show-main-window", 'm', 0, G_OPTION_ARG_NONE, &options.mainwin, N_("Display the main window"), NULL},
    {"headless", 'h', 0, G_OPTION_ARG_NONE, &options.headless, N_("Headless mode"), NULL},
    {"quit-after-play", 'q', 0, G_OPTION_ARG_NONE, &options.quit_after_play, N_("Quit on playback stop"), NULL},
    {"version", 'v', 0, G_OPTION_ARG_NONE, &options.version, N_("Show version"), NULL},
    {"verbose", 'V', 0, G_OPTION_ARG_NONE, &options.verbose, N_("Print debugging messages"), NULL},
    {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &options.filenames, N_("FILE..."), NULL},
    {NULL},
};

static void parse_options (int * argc, char *** argv)
{
    GOptionContext *context;
    GError *error = NULL;

    memset (& options, 0, sizeof options);
    options.session = -1;

    context = g_option_context_new(_("- play multimedia files"));
    g_option_context_add_main_entries(context, cmd_entries, PACKAGE);
#ifdef USE_GTK
    g_option_context_add_group(context, gtk_get_option_group(FALSE));
#endif

    if (!g_option_context_parse(context, argc, argv, &error))
    {
        fprintf (stderr,
         _("%s: %s\nTry `%s --help' for more information.\n"), (* argv)[0],
         error->message, (* argv)[0]);
        g_error_free (error);
        exit (EXIT_FAILURE);
    }

    g_option_context_free (context);

    verbose = options.verbose;
}

bool_t headless_mode (void)
{
    return options.headless;
}

static Index * convert_filenames (void)
{
    if (! options.filenames)
        return NULL;

    Index * filenames = index_new ();
    char * * f = options.filenames;
    char * cur = g_get_current_dir ();

    for (int i = 0; f[i]; i ++)
    {
        char * uri = NULL;

        if (strstr (f[i], "://"))
            uri = str_get (f[i]);
        else if (g_path_is_absolute (f[i]))
            uri = filename_to_uri (f[i]);
        else
        {
            char * tmp = filename_build (cur, f[i]);
            uri = filename_to_uri (tmp);
            str_unref (tmp);
        }

        if (uri)
            index_insert (filenames, -1, uri);
    }

    g_free (cur);
    return filenames;
}

#ifdef USE_DBUS
static void do_remote (void)
{
    GDBusConnection * bus = NULL;
    ObjAudacious * obj = NULL;
    GError * error = NULL;

    if (! (bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, & error)))
        goto ERR;

    if (! (obj = obj_audacious_proxy_new_sync (bus, 0, "org.atheme.audacious",
     "/org/atheme/audacious", NULL, & error)))
        goto ERR;

    /* check whether remote is running */
    char * version = NULL;
    obj_audacious_call_version_sync (obj, & version, NULL, NULL);

    if (! version)
        goto DONE;

    AUDDBG ("Connected to remote version %s.\n", version);

    Index * filenames = convert_filenames ();

    /* if no command line options, then present running instance */
    if (! (filenames || options.play || options.pause || options.play_pause ||
     options.stop || options.rew || options.fwd || options.show_jump_box ||
     options.mainwin))
        options.mainwin = TRUE;

    if (filenames)
    {
        int n_filenames = index_count (filenames);
        const char * * list = g_new (const char *, n_filenames + 1);

        for (int i = 0; i < n_filenames; i ++)
            list[i] = index_get (filenames, i);

        list[n_filenames] = NULL;

        if (options.enqueue_to_temp)
            obj_audacious_call_open_list_to_temp_sync (obj, list, NULL, NULL);
        else if (options.enqueue)
            obj_audacious_call_add_list_sync (obj, list, NULL, NULL);
        else
            obj_audacious_call_open_list_sync (obj, list, NULL, NULL);

        g_free (list);
        index_free_full (filenames, (IndexFreeFunc) str_unref);
    }

    if (options.play)
        obj_audacious_call_play_sync (obj, NULL, NULL);
    if (options.pause)
        obj_audacious_call_pause_sync (obj, NULL, NULL);
    if (options.play_pause)
        obj_audacious_call_play_pause_sync (obj, NULL, NULL);
    if (options.stop)
        obj_audacious_call_stop_sync (obj, NULL, NULL);
    if (options.rew)
        obj_audacious_call_reverse_sync (obj, NULL, NULL);
    if (options.fwd)
        obj_audacious_call_advance_sync (obj, NULL, NULL);
    if (options.show_jump_box)
        obj_audacious_call_show_jtf_box_sync (obj, TRUE, NULL, NULL);
    if (options.mainwin)
        obj_audacious_call_show_main_win_sync (obj, TRUE, NULL, NULL);

    g_free (version);
    g_object_unref (obj);

    exit (EXIT_SUCCESS);

ERR:
    fprintf (stderr, "D-Bus error: %s\n", error->message);
    g_error_free (error);

DONE:
    if (obj)
        g_object_unref (obj);

    return;
}
#endif

static void do_commands (void)
{
    bool_t resume = get_bool (NULL, "resume_playback_on_startup");

    Index * filenames = convert_filenames ();
    if (filenames)
    {
        if (options.enqueue_to_temp)
        {
            drct_pl_open_temp_list (filenames);
            resume = FALSE;
        }
        else if (options.enqueue)
            drct_pl_add_list (filenames, -1);
        else
        {
            drct_pl_open_list (filenames);
            resume = FALSE;
        }
    }

    if (resume)
        playlist_resume ();

    if (options.play || options.play_pause)
    {
        if (! drct_get_playing ())
            drct_play ();
        else if (drct_get_paused ())
            drct_pause ();
    }

#ifdef USE_GTK
    if (options.show_jump_box && ! options.headless)
        audgui_jump_to_track ();
    if (options.mainwin && ! options.headless)
        interface_show (TRUE);
#endif
}

static void init_one (void)
{
#ifdef HAVE_SIGWAIT
    signals_init_one ();
#endif

    init_paths ();
    make_dirs ();

    setlocale (LC_ALL, "");
    bindtextdomain (PACKAGE, aud_paths[AUD_PATH_LOCALE_DIR]);
    bind_textdomain_codeset (PACKAGE, "UTF-8");
    bindtextdomain (PACKAGE "-plugins", aud_paths[AUD_PATH_LOCALE_DIR]);
    bind_textdomain_codeset (PACKAGE "-plugins", "UTF-8");
    textdomain (PACKAGE);
}

static void init_two (int * p_argc, char * * * p_argv)
{
#ifdef USE_GTK
    if (! options.headless)
        gtk_init (p_argc, p_argv);
#endif

#ifdef HAVE_SIGWAIT
    signals_init_two ();
#endif

    AUDDBG ("Loading configuration.\n");
    config_load ();

    AUDDBG ("Initializing.\n");
    art_init ();
    chardet_init ();
    eq_init ();
    playlist_init ();

    tag_set_verbose (verbose);
    vfs_set_verbose (verbose);

    AUDDBG ("Loading lowlevel plugins.\n");
    start_plugins_one ();

    AUDDBG ("Starting worker threads.\n");
    adder_init ();
    scanner_init ();

    AUDDBG ("Restoring state.\n");
    load_playlists ();

    do_commands ();

    AUDDBG ("Loading highlevel plugins.\n");
    start_plugins_two ();

#ifdef USE_DBUS
    dbus_server_init ();
#endif
}

static void shut_down (void)
{
    AUDDBG ("Saving playlist state.\n");
    save_playlists (TRUE);

    AUDDBG ("Unloading highlevel plugins.\n");
    stop_plugins_two ();

#ifdef USE_DBUS
    dbus_server_cleanup ();
#endif

    AUDDBG ("Stopping playback.\n");
    if (drct_get_playing ())
        drct_stop ();

    AUDDBG ("Stopping worker threads.\n");
    adder_cleanup ();
    scanner_cleanup ();

    AUDDBG ("Unloading lowlevel plugins.\n");
    stop_plugins_one ();

    AUDDBG ("Saving configuration.\n");
    config_save ();
    config_cleanup ();

    AUDDBG ("Cleaning up.\n");
    art_cleanup ();
    chardet_cleanup ();
    eq_cleanup ();
    history_cleanup ();
    playlist_end ();

    free_paths ();
    strpool_shutdown ();
}

bool_t do_autosave (void)
{
    AUDDBG ("Saving configuration.\n");
    hook_call ("config save", NULL);
    save_playlists (FALSE);
    config_save ();
    return TRUE;
}

static bool_t check_should_quit (void)
{
    return options.quit_after_play && ! drct_get_playing () && ! playlist_add_in_progress (-1);
}

static void maybe_quit (void)
{
    if (check_should_quit ())
#ifdef USE_GTK
        gtk_main_quit ();
#else
        g_main_loop_quit (main_loop);
#endif
}

#ifdef LIBAUDACIOUS
EXPORT int audacious_main (int argc, char * * argv)
#else
int main (int argc, char * * argv)
#endif
{
    init_one ();
    parse_options (& argc, & argv);

    if (options.version)
    {
        printf ("%s %s (%s)\n", _("Audacious"), VERSION, BUILDSTAMP);
        return EXIT_SUCCESS;
    }

#if USE_DBUS
    do_remote (); /* may exit */
#endif

    AUDDBG ("No remote session; starting up.\n");
    init_two (& argc, & argv);

    AUDDBG ("Startup complete.\n");
    g_timeout_add_seconds (AUTOSAVE_INTERVAL, (GSourceFunc) do_autosave, NULL);

    if (check_should_quit ())
        goto QUIT;

    hook_associate ("playback stop", (HookFunction) maybe_quit, NULL);
    hook_associate ("playlist add complete", (HookFunction) maybe_quit, NULL);

#ifdef USE_GTK
    gtk_main ();
#else
    main_loop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (main_loop);
    g_main_loop_unref (main_loop);
#endif

    hook_dissociate ("playback stop", (HookFunction) maybe_quit);
    hook_dissociate ("playlist add complete", (HookFunction) maybe_quit);

QUIT:
    shut_down ();
    return EXIT_SUCCESS;
}

#ifdef LIBAUDACIOUS
EXPORT AudAPITable * audacious_init (void) {
    return & api_table;
}
#endif

void drct_quit (void)
{
#ifdef USE_GTK
    gtk_main_quit ();
#else
    g_main_loop_quit (main_loop);
#endif
}
