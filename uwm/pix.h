#ifndef UWM_PIX_H
#define UWM_PIX_H

#include <X11/Xlib.h>
#include <X11/xpm.h>

int LoadPic(char *filename, Pixmap *pm, XpmAttributes *xa);

#endif
