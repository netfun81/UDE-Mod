#ifndef HANDLERS_H
#define HANDLERS_H

#include <X11/Xlib.h>

/*** Prototypes ***/

void UWMErrorHandler(Display *disp, XErrorEvent *Err);
void ArmeageddonHandler(void);
void RedirectErrorHandler(void);
void InitHandlers();
void TermHandler(int dummy);
void ReinstallDefaultHandle();
void HandleShape(XEvent *event);
void InstallMoveHandle();
void InstallResizeHandle();
void InstallWinMenuHandle();
void InstallMenuHandle();
void HandleUnmapNotify(XEvent *event);
void HandleExpose(XEvent *event);

#endif
