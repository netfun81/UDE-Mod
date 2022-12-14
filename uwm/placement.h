#ifndef UWM_PLACEMENT_H
#define UWM_PLACEMENT_H

typedef struct {
  int x1,y1,x2,y2;
} ScanData;

void PlaceWin(UltimateContext *uc);
NodeList *ScanScreen(UltimateContext *win);
void FreeScanned(NodeList *wins);
void SnapWin(NodeList *wins, int *x, int *y, int width, int height);

#endif
