#ifndef UWM_RESIZE_H
#define UWM_RESIZE_H

void StartResizing(UltimateContext *uc,int x,int y);
void ResizeButtonPress(XEvent *event);
void ResizeMotion(XEvent *event);
void ResizeButtonRelease(XEvent *event);

#endif
