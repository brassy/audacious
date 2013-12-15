include ../extra.mk

ifeq ($(USE_GTK),yes)
SUBDIRS := audacious libaudcore libaudgui libaudtag
else
SUBDIRS := audacious libaudcore libaudtag
endif

ifeq ($(USE_DBUS),yes)
SUBDIRS := dbus audtool ${SUBDIRS}
endif

include ../buildsys.mk

ifeq ($(USE_GTK),yes)
audacious libaudgui libaudtag: libaudcore
audacious: libaudgui libaudtag
else
audacious libaudtag: libaudcore
audacious: libaudtag
endif

ifeq ($(USE_DBUS),yes)
audacious audtool: dbus
endif
