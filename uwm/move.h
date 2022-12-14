#ifndef UWM_MOVE_H
#define UWM_MOVE_H

void MoveButtonPress(XEvent *event);
void MoveMotion(XEvent *event);
void MoveButtonRelease(XEvent *event);
void MoveUnmap(XEvent *event);
void StartDragging(UltimateContext *uc,unsigned int x,unsigned int y);

#endif
