/***  WINDOWS.C: Contains standard Window-Handling UWM-routines  ***/

/* ########################################################################

   uwm - THE ude WINDOW MANAGER

   ########################################################################

   Copyright (c) : Christian Ruppert

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   ######################################################################## */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>

#include <stdlib.h>

#include "uwm.h"
#include "init.h"
#include "properties.h"
#include "nodes.h"
#include "menu.h"
#include "workspaces.h"
#include "widgets.h"
#include "placement.h"
#include "special.h"
#include "i18n.h"
#include "wingroups.h"
#include "windows.h"

extern Display *disp;
extern UDEScreen TheScreen;
extern InitStruct InitS;
extern XContext UWMGroupContext;
extern XContext UWMContext;
extern HandlerTable *Handle;
extern HandlerTable MoveHandle[LASTEvent];
extern HandlerTable ResizeHandle[LASTEvent];
extern Atom WM_TAKE_FOCUS;
extern Atom WM_DELETE_WINDOW;

UltimateContext *ActiveWin=NULL;
UltimateContext *FocusWin=NULL;

char WinVisible(UltimateContext *uc)
{
  if(SeemsNormal(uc)) return(-1);
  else return(0);
}

void DrawWinBorder(UltimateContext *uc)
{
  int active;
  unsigned long winbg;

  if(uc->frame == None) return;
  if((active = (uc == FocusWin))) {
    uc->flags |= ACTIVE_BORDER;
  } else {
    uc->flags &= ~ACTIVE_BORDER;
  }

  winbg = active ? TheScreen.ActiveBorder[TheScreen.desktop.ActiveWorkSpace]
          : TheScreen.InactiveBorder[TheScreen.desktop.ActiveWorkSpace];
  XSetWindowBackground(disp, uc->border, winbg);
  XClearWindow(disp, uc->border);
  if(uc->title.win != None) {
    XSetWindowBackground(disp, uc->title.win, winbg);
    if(uc->title.name && (InitS.BorderTitleFlags
       & (active ? BT_ACTIVE_TITLE : BT_INACTIVE_TITLE))){
      XRaiseWindow(disp,uc->title.win);
      DrawTitle(uc);
    } else {
      XLowerWindow(disp, uc->title.win);
      if(uc->flags & SHAPED) DrawTitle(uc);
    }
  }
  DrawFrameBevel(uc);
}

void ShapeFrame(UltimateContext *uc)
{
  int a, shaped, wbs, hbs;

  XShapeQueryExtents(disp,uc->win,&shaped,&a,&a,&wbs,&hbs,&a,&a,&a,&a,&a);
  if(uc->frame != None) {
    if(shaped){
      uc->flags |= SHAPED;
      XShapeCombineShape(disp, uc->frame, ShapeBounding, uc->BorderWidth,
                         uc->BorderWidth + TheScreen.TitleHeight, uc->win,
                         ShapeBounding, ShapeSet);
      if(uc->title.win != None)
        XShapeCombineShape(disp, uc->frame, ShapeBounding, uc->title.x,
                           uc->title.y , uc->title.win,
                           ShapeBounding, ShapeUnion);
      XUnmapWindow(disp, uc->border);
      UpdateName(uc);
    } else {
      if(uc->flags & SHAPED) {
        XShapeCombineMask(disp, uc->frame, ShapeBounding, 0, 0, None, ShapeSet);
      }
      uc->flags &= ~SHAPED;
      
      XMapWindow(disp, uc->border);
      UpdateName(uc);
    }
  }
}

void ReparentWin(UltimateContext *uc, Window parent, int x, int y)
{
  UpdateUWMContext(uc);
  if(uc->Attributes.map_state != IsUnmapped) uc->expected_unmap_events++;
                                /* prevent unmapping of already mapped window */
  XReparentWindow(disp, uc->win, parent, x, y); /* perform action */
}

/*** width and hight of frame window! ***/

void MoveResizeWin(UltimateContext *uc,int x,int y,int width,int height)
{
  int ow,oh,ox,oy;
  DBG(fprintf(TheScreen.errout, "MoveResizeWin (%d): (%d|%d) - %dx%d\n",
              uc->win, x, y, width, height);)
  ox=uc->Attr.x;
  oy=uc->Attr.y;
  ow=uc->Attr.width;
  oh=uc->Attr.height;
  if(((x == ox) && (y == oy)) && ((width == ow) || (width == 0))
     && ((height == oh) || (height == 0))) return;

  if(!(width||height)){
    if(uc->frame != None) XMoveWindow(disp, uc->frame, x, y);
    else XMoveWindow(disp, uc->win, x, y);
  } else {
    if(uc->frame != None) XMoveResizeWindow(disp,uc->frame,x,y,width,height);
    else XMoveResizeWindow(disp, uc->win, x, y, width, height);
    if(uc->border!= None) XResizeWindow(disp,uc->border,width,height);
    XMoveResizeWindow(disp, uc->win,
		      uc->BorderWidth, uc->BorderWidth + TheScreen.TitleHeight,
		      width - 2 * uc->BorderWidth,
		      height - 2 * uc->BorderWidth - TheScreen.TitleHeight);
    if((uc->title.win != None ) 
       && ((uc->flags & SHAPED)
           || (InitS.BorderTitleFlags & BT_CENTER_TITLE))) {
      XMoveWindow(disp, uc->title.win,
                  uc->title.x = ((width - uc->title.width)/2), uc->title.y);
    }
  }
  UpdateUWMContext(uc);
  if(((x!=ox)||(y!=oy))&&(((width==0)||(width==ow))&&\
                         ((height==0)||(height==oh))))
    SendConfigureEvent(uc);
}

void GravitizeWin(UltimateContext *uc,int *x, int *y, int mode)
{
  switch(uc->ra.gravity){        /*** Y ***/
    case SouthGravity:
    case SouthEastGravity:
    case SouthWestGravity:
      *y -= mode * (2 * (uc->BorderWidth - uc->OldBorderWidth)
                                  + TheScreen.TitleHeight);
      break;
    case CenterGravity:
    case EastGravity:
    case WestGravity:
      *y -= mode * (uc->BorderWidth - uc->OldBorderWidth 
                                  + TheScreen.TitleHeight / 2);
      break;
    case StaticGravity:
      *y -= mode * (uc->BorderWidth - uc->OldBorderWidth
                                  + TheScreen.TitleHeight);
      break;
    default: break;
  }
  switch(uc->ra.gravity){        /*** X ***/
    case EastGravity:
    case SouthEastGravity:
    case NorthEastGravity:
      *x -= mode * (2 * (uc->BorderWidth - uc->OldBorderWidth));
      break;
    case CenterGravity:
    case NorthGravity:
    case StaticGravity:
    case SouthGravity:
      *x -= mode * (uc->BorderWidth - uc->OldBorderWidth);
      break;
    default: break;
  }
}

/***  Will add a Frame-Window behind (and around) the   ***
 ***  real Window. The Frame Window is used as an eazy  ***
 ***  realisation of window borders etc...              ***/

void EnborderWin(UltimateContext *uc)
{
  XSetWindowAttributes SAttr;
  int a,w,h;
  char HasTitle=-1;

  if(uc->frame) return; /* frame already existing... */

  if(uc->Attributes.override_redirect) return;
  if(uc->TransientFor) {
    uc->BorderWidth=TheScreen.BorderWidth2;
    uc->flags &= ~PLACEIT;
  } else uc->BorderWidth=TheScreen.BorderWidth1;

  if(uc->MotifWMHints){
    if((uc->MotifWMHints->flags & MWM_HINTS_FUNCTIONS)
       && (uc->MotifWMHints->functions==0)) {
      uc->BorderWidth=0;
      HasTitle = 0;
    }
    if(uc->MotifWMHints->flags & MWM_HINTS_DECORATIONS) {
      if(uc->MotifWMHints->decorations==0) uc->BorderWidth=0;
      HasTitle = (uc->MotifWMHints->decorations & MWM_DECOR_TITLE);
    }
  }

  if((InitS.PlacementStrategy & 1) && ((uc->Attributes.x!=0) ||\
                 (uc->Attributes.y!=0))) uc->flags &= ~PLACEIT;

  if(((uc->Attributes.x + uc->Attributes.width) > TheScreen.width)
     || ((uc->Attributes.y + uc->Attributes.height) > TheScreen.height)
     || (uc->Attributes.x < 0) || (uc->Attributes.y < 0)) {
    uc->flags |= PLACEIT;
    if((uc->Attributes.class == InputOutput) && (!IsNormal(uc))) {
      uc->Attributes.x = (TheScreen.width-uc->Attributes.width)/2;
      uc->Attributes.y = (TheScreen.height-uc->Attributes.height)/2;
    }
  }

  if(!(uc->flags & PLACEIT))
    GravitizeWin(uc, &(uc->Attributes.x), &(uc->Attributes.y), UWM_GRAVITIZE);

DBG(fprintf(TheScreen.errout,"reparenting: %d\n",uc->win);)
  GrabServer();

  /*** create a frame to keep the window and its border ***/
  SAttr.override_redirect=True;
  SAttr.cursor=TheScreen.Mice[C_WINDOW];
  uc->frame=XCreateWindow(disp,TheScreen.root,uc->Attributes.x,uc->Attributes.y,
                                        uc->Attributes.width+2*uc->BorderWidth,\
                  uc->Attributes.height+2*uc->BorderWidth+TheScreen.TitleHeight\
                                  ,0,CopyFromParent,InputOutput,CopyFromParent,\
                                            CWCursor|CWOverrideRedirect,&SAttr);
  if(XSaveContext(disp,uc->frame,UWMContext,(XPointer)uc))
    fprintf(TheScreen.errout,"UWM FATAL: Couldn't save Context\n");
  XSelectInput(disp,uc->frame,FRAME_EVENTS);

  /*** create an independent border-window ***/
  SAttr.override_redirect=True;
  SAttr.background_pixel=TheScreen.InactiveBorder
                         [TheScreen.desktop.ActiveWorkSpace];
  SAttr.cursor=TheScreen.Mice[C_BORDER];
  uc->border=XCreateWindow(disp,uc->frame,0,0,uc->Attributes.width+\
                           2*uc->BorderWidth,uc->Attributes.height+\
                           2*uc->BorderWidth+TheScreen.TitleHeight,\
                       0,CopyFromParent,InputOutput,CopyFromParent,\
                    CWCursor|CWOverrideRedirect|CWBackPixel,&SAttr);
  if(XSaveContext(disp,uc->border,UWMContext,(XPointer)uc))
    fprintf(TheScreen.errout,"UWM FATAL: Couldn't save Context\n");
  XSelectInput(disp,uc->border,BORDER_EVENTS);
  XMapWindow(disp,uc->border);

  /*** create title-window if needed ***/
  if((InitS.BorderTitleFlags & (BT_INACTIVE_TITLE|BT_ACTIVE_TITLE)) && HasTitle)
  {
    SAttr.override_redirect=True;
    SAttr.background_pixel=TheScreen.InactiveBorder
                           [TheScreen.desktop.ActiveWorkSpace];
    SAttr.cursor=TheScreen.Mice[C_BORDER];
    a = (uc->BorderWidth-TheScreen.FrameBevelWidth-1) / 2
        + TheScreen.FrameBevelWidth;
    uc->title.y = uc->title.x = a;
    uc->title.win = XCreateWindow(disp, uc->frame, uc->title.x, uc->title.y,
                 1, 1, 0, CopyFromParent, InputOutput, CopyFromParent,
		 CWCursor | CWOverrideRedirect | CWBackPixel, &SAttr);
    if(XSaveContext(disp, uc->title.win, UWMContext, (XPointer)uc))
      fprintf(TheScreen.errout, "UWM FATAL: Couldn't save Context\n");
    XSelectInput(disp, uc->title.win, TITLE_EVENTS);
    XMapRaised(disp, uc->title.win);
  }
  ShapeFrame(uc);                  /* adjust frame to shaped window */

  XAddToSaveSet(disp,uc->win);
  
  ReparentWin(uc, uc->frame, uc->BorderWidth - uc->Attributes.border_width,
              uc->BorderWidth + TheScreen.TitleHeight - uc->Attributes.border_width);

  UpdateUWMContext(uc);
  SendConfigureEvent(uc);

  UngrabServer();

  Updatera(uc);
  UpdateName(uc);

  if(TheScreen.MaxWinWidth||TheScreen.MaxWinHeight){
    w=TheScreen.MaxWinWidth?((TheScreen.MaxWinWidth<uc->Attr.width)?\
                TheScreen.MaxWinWidth:uc->Attr.width):uc->Attr.width;
    w=(w-2*uc->BorderWidth)<uc->ra.minw?uc->ra.minw:w;
    w=((int)((w-uc->ra.bw)/uc->ra.wi))*uc->ra.wi+uc->ra.bw;
    h=TheScreen.MaxWinHeight?((TheScreen.MaxWinHeight<uc->Attr.height)?\
                TheScreen.MaxWinHeight:uc->Attr.height):uc->Attr.height;
    h=(h-2*uc->BorderWidth-uc->title.height)<uc->ra.minh?uc->ra.minh:h;
    h=((int)((h-uc->ra.bh)/uc->ra.hi))*uc->ra.hi+uc->ra.bh;
    if((uc->Attr.width!=w)||(uc->Attr.height!=h))
      MoveResizeWin(uc,uc->Attr.x,uc->Attr.y,w,h);
  }
}

void UnmapWin(UltimateContext *uc)
{
  UpdateUWMContext(uc);
  if(uc->Attributes.map_state != IsUnmapped) uc->own_unmap_events++;
  XUnmapWindow(disp,uc->win);
  if(!SeemsWithdrawn(uc)) SetSeemsMapState(uc, IconicState);
}

void MapWin(UltimateContext *uc,Bool NoPlacement){
  if(uc->frame == None) EnborderWin(uc);

  if(OnActiveWS(uc->WorkSpace)) {
    if(!NoPlacement) PlaceWin(uc);
    XMapRaised(disp, uc->win);
    SetSeemsMapState(uc, NormalState);
    if(uc->frame != None){
      XMapSubwindows(disp, uc->frame);
      if(uc->flags & SHAPED) XUnmapWindow(disp, uc->border);
      XMapRaised(disp, uc->frame);
      DrawWinBorder(uc);
    }
    if(!((InitS.WarpPointerToNewWinH == -1) ||
	 (InitS.WarpPointerToNewWinV == -1))
       && (!NoPlacement)) {
      if((InitS.WarpPointerToNewWinH == -2) ||
	 (InitS.WarpPointerToNewWinV == -2))
	XWarpPointer(disp, None, uc->frame, 0, 0, 0, 0, 0, 0);
      else
	XWarpPointer(disp, None, uc->win, 0, 0, 0, 0,
		     uc->Attributes.width*InitS.WarpPointerToNewWinH/100,
		     uc->Attributes.height*InitS.WarpPointerToNewWinV/100);
    }
  } else {
    SetSeemsMapState(uc, IconicState);
  }
  uc->uwmstate = NormalState;
}

void ActivateWin(UltimateContext *uc)
{
  UltimateContext *OldActive;

  if((Handle==MoveHandle)||(Handle==ResizeHandle)) return;
                       /* Don't confuse window moving or resizing routines!!! */
  if(!uc) {
    Window dummywin, win;
    int dummy;
    XQueryPointer(disp, TheScreen.root, &dummywin, &win, &dummy, &dummy,
                  &dummy, &dummy, &dummy);
    if(XFindContext(disp, win, UWMContext, (XPointer *)&uc)) uc = NULL;
  }
  if(uc && (!WinVisible(uc)))
    uc = NULL;

  OldActive = ActiveWin;
  ActiveWin = uc;

  if(uc) {
    int focus_set;

    focus_set = 0;
    UpdateUWMContext(uc);
    if(!((uc->WMHints) && (uc->WMHints->flags & InputHint)
                       && (uc->WMHints->input == False))) {
      XSetInputFocus(disp, ActiveWin->win, RevertToPointerRoot, TimeStamp);
      focus_set = 1;
    }

    if(ActiveWin->ProtocolFlags & TAKE_FOCUS) {
      SendWMProtocols(ActiveWin, WM_TAKE_FOCUS);
      focus_set = 1;
    }

    if((!focus_set) && (ActiveWin->frame != None)) {
      XSetInputFocus(disp, ActiveWin->frame, RevertToPointerRoot, TimeStamp);
    }
  } else if(OldActive) {
    XSetInputFocus(disp, PointerRoot, RevertToPointerRoot, TimeStamp);
  }
}

void DisenborderWin(UltimateContext *uc, Bool alive)
{
  if(uc->frame!=None) {
    if(alive){
      GravitizeWin(uc, &(uc->Attr.x), &(uc->Attr.y), UWM_DEGRAVITIZE);
      ReparentWin(uc, TheScreen.root, uc->Attr.x, uc->Attr.y);
      XRemoveFromSaveSet(disp, uc->win);
    }
    XDeleteContext(disp,uc->border,UWMContext);
    XDestroyWindow(disp,uc->border);
    if(uc->title.win != None) {
      XDeleteContext(disp, uc->title.win, UWMContext);
      XDestroyWindow(disp, uc->title.win);
    }
    XDeleteContext(disp,uc->frame,UWMContext);
    XDestroyWindow(disp,uc->frame);
    uc->frame = uc->border = uc->title.win = None;
  }
}

Node* PlainDeUltimizeWin(UltimateContext *uc,Bool alive)
{
  Node *n;
  WinGroup *group;

  if(!XFindContext(disp, uc->win, UWMGroupContext, (XPointer *)&group))
    DeleteWinGroup(group);

  if(uc->group) RemoveWinFromGroup(uc);

  n = NodeDelete(TheScreen.UltimateList, InNodeList(TheScreen.UltimateList,uc));
  XDeleteContext(disp,uc->win,UWMContext);

  if(alive) XSetWindowBorderWidth(disp,uc->win,uc->OldBorderWidth);
  DisenborderWin(uc,alive);

  if(uc == ActiveWin) ActivateWin(NULL);

  free(uc);

  return(n);
}

Node* DeUltimizeWin(UltimateContext *uc,Bool alive)
{
  if(alive) SetIsMapState(uc, WithdrawnState);
  return(PlainDeUltimizeWin(uc, alive));
}

/***************************************************************************/
UltimateContext *UltimizeWin(Window win)
{
  UltimateContext *uc;
  XWindowAttributes Attr;

  if(TheScreen.root == win) return(NULL); /* never ultimize root window */

  if(!XFindContext(disp, win, UWMContext, (XPointer *)&uc)) return(uc);
  XGetWindowAttributes(disp,win,&Attr);
  if(Attr.override_redirect) return(NULL);

DBG(fprintf(TheScreen.errout,"ULTIMIZING WIN #%d: no override redirect.\n",win);)

  if(!(uc = malloc(sizeof(UltimateContext)))) {
    fprintf(TheScreen.errout,"UWM: Cannot create Window. (Not enough Memory)\n");
    return(NULL);
  }

  uc->win=win;
  uc->frame=None;
  uc->border=None;

  uc->BorderWidth=0;
  uc->OldBorderWidth=Attr.border_width;
  XSetWindowBorderWidth(disp,uc->win,0);

  uc->title.win = None;
  uc->title.name=NULL;
  uc->title.iconname=NULL;

  uc->WorkSpace = TheScreen.desktop.ActiveWorkSpace;

  uc->WMHints=NULL;
  uc->group=NULL;
  uc->MotifWMHints=NULL;

  uc->flags=PLACEIT;

  uc->expected_unmap_events = 0;
  uc->own_unmap_events = 0;
  
/*  SetIsMapState(uc,WithdrawnState); */
  uc->uwmstate = uc->wmstate = WithdrawnState;

  if(!NodeAppend(TheScreen.UltimateList,uc))
    SeeYa(1,"FATAL: out of memory!");

/* we need this already here to tell some of the fuctions below that
   window is already being ultimized */
  if(XSaveContext(disp,uc->win,UWMContext,(XPointer)uc))
    fprintf(TheScreen.errout,"UWM FATAL: Couldn't save Context\n");

  UpdateUWMContext(uc);

  UpdateName(uc);
  UpdateIconName(uc);

  Updatera(uc);

  UpdateWMHints(uc);
  UpdateMotifHints(uc);
  UpdateTransientForHint(uc);
  UpdateWMProtocols(uc);

  XSelectInput(disp, uc->win, WINDOW_EVENTS);
  XShapeSelectInput(disp,uc->win,ShapeNotifyMask);

  return(uc);
}

void IconifyWin(UltimateContext *uc)
{
  if(IsIconic(uc)) return;

  UnmapWin(uc);

  SetIsMapState(uc, IconicState);
}

void DisplayWin(UltimateContext *uc)
{
  if(SeemsNormal(uc)) return;

  ChangeWS(uc->WorkSpace);

  MapWin(uc, False);

  SetIsMapState(uc, NormalState);
}

void CloseWin(UltimateContext *uc)
{
  if(uc->ProtocolFlags & DELETE_WINDOW)
    SendWMProtocols(uc, WM_DELETE_WINDOW);
}

void DeiconifyMenu(int x, int y)
{
  int a;
  Node *ucn;
  Menu *men, **wspaces=NULL, *sticky;
  MenuItem *item;
  short useTitle = (TheScreen.desktop.flags & UDESubMenuTitles);

  men=MenuCreate(_("Windows Menu"));
  if(!men) {
    SeeYa(1, "FATAL: out of memory!");
  }

  wspaces=MyCalloc(TheScreen.desktop.WorkSpaces,sizeof(Menu *));
  if(TheScreen.desktop.WorkSpaces>1){
    for(a=0;a<TheScreen.desktop.WorkSpaces;a++){
      wspaces[a]=MenuCreate(useTitle ? TheScreen.WorkSpace[a] : NULL);
      AppendMenuItem(men,TheScreen.WorkSpace[a],wspaces[a],I_SUBMENU);
    }
    AppendMenuItem (men, NULL, NULL, I_LINE);
    sticky = MenuCreate(useTitle ? _("Sticky Windows") : NULL);
    AppendMenuItem (men, _("Sticky Windows"), sticky, I_SUBMENU);
  } else {
    wspaces[0]=men;
    sticky = men;
  }

  GrabServer();  /* Make sure no window gets destroyed meanwhile */

  ucn=NULL;
  while((ucn = NodeNext(TheScreen.UltimateList,ucn))){
    UltimateContext *uc;
    uc=ucn->data;
    if((IsNormal(uc)) || (IsIconic(uc))) {
      AppendMenuItem((uc->WorkSpace == -1) ? sticky : wspaces[uc->WorkSpace],
                     (IsIconic(uc) && uc->title.iconname) ? uc->title.iconname
		     : (uc->title.name ? uc->title.name 
		        : (uc->title.iconname ? uc->title.iconname
                           : _(">nameless window<"))),
		     uc, (IsIconic(uc)) ? I_SWITCH_OFF : I_SWITCH_ON);
    }
  }

  if((item = StartMenu(men, x, y, True, NULL))){
    if(SWITCHTYPE(item->type)){
      UltimateContext *uc;
      uc=item->data;
      if(SeemsIconic(uc)) {
        DisplayWin(uc);
      } else {
/*        ChangeWS(uc->WorkSpace); */
        ActivateWin(uc);
        if(uc->frame != None) {
          XRaiseWindow(disp,uc->frame);
        } else {
          XRaiseWindow(disp,uc->win);
        }
      }
    }
    if(item->type==I_SUBMENU) {
      for(a=0;a<TheScreen.desktop.WorkSpaces;a++)
        if(wspaces[a]==item->data) { ChangeWS(a);break;}
    }
  }

  UngrabServer();

  DestroyMenu(men);
  if(wspaces) free(wspaces);
}
