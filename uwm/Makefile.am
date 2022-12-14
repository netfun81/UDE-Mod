bin_PROGRAMS = uwm
uwm_LDFLAGS  = ${X_LIBS} ${X_EXTRA_LIBS} ${X_PRE_LIBS} ${LIBINTL}
uwm_CPPFLAGS = ${X_CFLAGS} -I../lib -DUWMVERSION=\"@VERSION@\" \
              -DLOCALEDIR=\"${datadir}/locale\" -DUDE_DIR=\"${pkgdatadir}\" \
              -DUDE_BIN_PATH=\"${bindir}\"
uwm_SOURCES = widgets.c \
              special.c\
              rubber.c\
              move.c \
              resize.c \
              windows.c \
              handlers.c \
              properties.c \
              nodes.c \
              winmenu.c \
              winmenumenu.c \
              menu.c \
              applications.c \
              init.c \
              uwmmenu.c \
              placement.c \
              urm.c \
              workspaces.c \
              pix.c \
              selection.c \
              wingroups.c \
              uwm.c

noinst_HEADERS= MwmUtil_fixed.h \
                applications.h \
                handlers.h \
                init.h \
                menu.h \
                move.h\
                nodes.h \
                placement.h \
                properties.h \
                resize.h \
                rubber.h \
                special.h \
                uwm.h \
                uwmmenu.h \
                widgets.h \
                windows.h \
                winmenu.h \
                urm.h \
                winmenumenu.h \
                pix.h \
                selection.h \
                wingroups.h \
                i18n.h \
                gettext.h \
                workspaces.h
