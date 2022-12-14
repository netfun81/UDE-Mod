#ifndef UWM_MENU_H
#define UWM_MENU_H

#include <X11/Xlib.h>

#include "nodes.h"

/*** Item Types: ***/
#define I_SELECT 1
#define I_SWITCH_ON 2
#define I_SWITCH_OFF 3
#define I_SUBMENU 4
#define I_LINE 5

#define SWITCHTYPE(A) (((A)>>1)==1)

typedef struct _MenuItem{
  char *name;
  short type;
  void *data;        /* pointer for item-dependent data */
  Window win;
  int y, height;
  XRectangle TextExtents;
  struct _Menu *menu;
} MenuItem;

typedef struct _Menu{
  char *name;
  NodeList *Items;   /* Data of type MenuItem *  */
  XFontSet font;
  Window win;
  int x,y;
  struct _Menu *parent;
  XRectangle TitleExtents;
  int width, height;
} Menu;

Menu *MenuCreate(char *name);
void AppendMenuItem(Menu *menu, char *name,void *data,short type);
void RemoveMenuBottomLines(Menu *men);
MenuItem *StartMenu(Menu *menu,int x,int y,Bool q,void (*prc)(XEvent *event, MenuItem *selected));
void DestroyMenu(Menu *menu);

void MenuEnterNotify(XEvent *event);
void MenuLeaveNotify(XEvent *event);
void MenuVisibility(XEvent *event);
void MenuButtonPress(XEvent *event);
void MenuButtonRelease(XEvent *event);
void RedrawMenuTree();
void Menu2ws(Menu *menu,short ws);
Menu *RootMenu(Menu *men);
void MenuDontKeepItAnymore();
void MenuExpose(XEvent *event);

#endif
