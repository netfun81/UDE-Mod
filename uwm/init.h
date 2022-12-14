#ifndef UWM_INIT_H
#define UWM_INIT_H

/* BorderTitleFlags */
#define BT_GROOVE    (1<<0)  /* draw groove into borders? */
#define BT_LINE     (1<<1)/* draw thin black line between windows and borders */
#define BT_INACTIVE_TITLE (1<<2) /* display title of inactive windows? */
#define BT_ACTIVE_TITLE (1<<3)  /* display title of active windows? */
#define BT_DODGY_TITLE (1<<4)  /* remove title as soon as touched by pointer? */
#define BT_CENTER_TITLE (1<<5)                            /* title in center? */
/* icccmFlags */
#define ICF_STAY_ALIVE    (1<<0)  /* don't let yourself replace by another wm */
#define ICF_TRY_HARD      (1<<1)  /* try to replace other icccm wms */
#define ICF_HOSTILE       (1<<2)  /* try to replace other icccm wms violently */
/* BehaviourFlags */
#define BF_IN_WIN_CTRL    (1<<0)  /* process clicks we get from inside a
                                     client window */

typedef struct {
  char StartScript[256];
  char StopScript[256];
  char HexPath[256];
  char MenuFileName[256];
  char RubberMove;
  unsigned long OpaqueMoveSize;
  int MenuBorderWidth,MenuXOffset,MenuYOffset;
  char menuType[3];
  char BorderButtons[3];
  char DragButtons[3];
  char ResizeButtons[3];
  char WMMenuButtons[3];
  char **OtherWms;
  unsigned short OtherWmCount;
  short PlacementStrategy;
  unsigned long PlacementThreshold;
  unsigned char BorderTitleFlags;
  unsigned char icccmFlags;
  unsigned char BehaviourFlags;
  int WarpPointerToNewWinH;
  int WarpPointerToNewWinV;
  short SnapDistance;
} InitStruct;

void InitUWM();
void CreateAppsMenu(char *filename);

#endif
