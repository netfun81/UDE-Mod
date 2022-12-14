#ifndef UWM_SELECTION_H
#define UWM_SELECTION_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>

void SendSelectionNotify(XEvent *request, Atom property);
int ChangeProperty(Window win, Atom property, Atom type, int format, int mode, 
                   char *data, int nelements);
Atom Parse_WM_Sx_Selection(Atom target, Atom prop, Window requestor);

#endif
