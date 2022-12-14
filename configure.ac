AC_INIT([uwm], [0.2.11b], [arc@users.sf.net])
AM_INIT_AUTOMAKE([-Wall])
AC_USE_SYSTEM_EXTENSIONS

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_DEFINE_UNQUOTED(CPP_CALL, "$CPP", [command line for preprocessor])
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_YACC

AM_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.18.2])

AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_PATH_XTRA
AC_CHECK_HEADERS([fcntl.h strings.h unistd.h time.h locale.h])
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset strchr strstr uname popen nanosleep putenv])
AC_TYPE_PID_T

AC_ARG_ENABLE([backingstore],
              AC_HELP_STRING([--disable-backingstore],
                             [Disable the use of your X-servers backing store
                              feature. XFree4's backing store still seems to be
                              buggy and so use this option in case you have
                              refresh problems with menus under XFree4. With
                              this option it won't be perfect but a lot
                              better.]),
              [if test "$enableval" != "yes" ; then
                  AC_DEFINE([DISABLE_BACKING_STORE], 1,
                            [define this if you don't want uwm to use your
                             x-servers BackingStore and SaveUnder features
                             (which are buggy in XF4)])
               fi])
AC_ARG_WITH([libjpeg],
            AC_HELP_STRING([--without-libjpeg],
                           [disable jpeg background images support]),
            [if test "$withval" = "yes" ; then
                AC_CHECK_LIB(jpeg, main)
             fi] ,
            AC_CHECK_LIB(jpeg, main))
AC_CHECK_LIB([ICE], [main],,,$X_CFLAGS $X_PRE_LIBS $X_EXTRA_LIBS $X_LIBS)
AC_CHECK_LIB([SM],  [main],,,$X_CFLAGS $X_PRE_LIBS $X_EXTRA_LIBS $X_LIBS)
AC_CHECK_LIB([X11], [main],, 
             AC_MSG_ERROR([No Xlib found. Please check if your X11 is
                           installed correctly.]),
             $X_CFLAGS $X_PRE_LIBS $X_EXTRA_LIBS $X_LIBS)
AC_CHECK_LIB([Xext], [main],,
             AC_MSG_ERROR([uwm requires X11 shape extension. Please make sure
                           that libXext is installed correctly.]),
             $X_CFLAGS $X_PRE_LIBS $X_EXTRA_LIBS $X_LIBS)
AC_CHECK_LIB([Xpm], [main],, AC_MSG_ERROR([uwm requires libXpm.]), 
             $X_CFLAGS $X_PRE_LIBS $X_EXTRA_LIBS $X_LIBS)
AC_CHECK_LIB([Xmu], [XmuGetHostname],, AC_MSG_ERROR([uwm requires libXmu.]), 
             $X_CFLAGS $X_PRE_LIBS $X_EXTRA_LIBS $X_LIBS)



AC_CONFIG_HEADERS([config.h])
AC_OUTPUT([intl/Makefile uwm/Makefile config/Makefile doc/Makefile extras/Makefile extras/tools/Makefile gfx/Makefile Makefile ude.spec config/StartScript po/Makefile.in])
