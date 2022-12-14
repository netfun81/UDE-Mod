#ifndef UWM_WINGROUPS_H
#define UWM_WINGROUPS_H

#include <X11/Xlib.h>
#include "nodes.h"
#include "uwm.h"

typedef struct _WinGroup {
  Window leader;
  short WorkSpace;
  NodeList *members; /* Data is *UltimateContext */
} WinGroup;

WinGroup *CreateWinGroup(Window leader);
void DeleteWinGroup(WinGroup *group);
void AddWinToGroup(UltimateContext *uc);
int RemoveWinFromGroup(UltimateContext *uc);
void UpdateWinGroup(UltimateContext *uc);

#endif /* UWM_WINGROUPS_H */
