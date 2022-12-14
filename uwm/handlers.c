/***  HANDLERS.C: Contains the handling routines for standard signals and events  ***/

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

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/extensions/shape.h>

#include "uwm.h"
#include "init.h"
#include "windows.h"
#include "special.h"
#include "properties.h"
#include "selection.h"
#include "nodes.h"
#include "handlers.h"

#include "move.h"
#include "resize.h"
#include "menu.h"
#include "uwmmenu.h"
#include "winmenu.h"
#include "workspaces.h"
#include "widgets.h"
#include "applications.h"

#include "MwmUtil_fixed.h"

extern UDEScreen TheScreen;
extern Display *disp;
extern XContext UWMContext;
extern UltimateContext *ActiveWin;
extern UltimateContext *FocusWin;
extern Atom WM_STATE_PROPERTY;
extern Atom WM_CHANGE_STATE;
extern Atom WM_PROTOCOLS;
extern Atom MOTIF_WM_HINTS;
extern InitStruct InitS;

extern int ignore_mask;

int ShapeEvent;

/***  Contains actual event handler configuration  ***/

HandlerTable *Handle;

/***  possible event handler configurations  ***
 ***  are set up by InitHandlers()           ***
 ***  should remain private                  ***/

HandlerTable DefaultHandle[LASTEvent];
HandlerTable MoveHandle[LASTEvent];
HandlerTable ResizeHandle[LASTEvent];
HandlerTable WinMenuHandle[LASTEvent];
HandlerTable MenuHandle[LASTEvent];

/*** in case we catch a system term signal... ***/
void TermHandler(int dummy)
{
  SeeYa(1,"Term-Signal received");
}

/*** Handler for system Error ***/
void UWMErrorHandler(Display *disp, XErrorEvent *Err)
{
DBG(\
  char et[1024];\
  XGetErrorText(disp,Err->error_code,et,1023);\
  fprintf(TheScreen.errout,"An error #%d occured:\n%s\n",Err->error_code,et);\
  fprintf(TheScreen.errout,"It was caused by command (Major-Minor): %d-%d\n",\
                                           Err->request_code,Err->minor_code);\
  fprintf(TheScreen.errout,"Look up the meanings in X11/Xproto.h\n");\
  fprintf(TheScreen.errout,"The considered resource was: %d\n",\
                                                 Err->resourceid);)
}

/*** Handler invoked in case display connection broke down ***/
void ArmeageddonHandler(void)
{
  SeeYa(1,"Hmm, seems like everything is just about to break down...\nConnect to X-Server lost");
}

/*** in case another WM is running while we start: ***/
void RedirectErrorHandler(void)
{
#ifndef DEBG
  SeeYa(1,"Looks like there is another Window manager running.");
#else
  fprintf (TheScreen.errout,"Looks like there is another Window manager "
           "running.\nBut since we're debuging we'll continue.");
#endif
}


/*** Handlers for X-EVENTS ***/
/* Have to be functions of type: void NAME(XEvent *event) */

/*void HandleCreateNotify(XEvent *event)
{
}*/

void HandleDestroyNotify(XEvent *event)
{
  UltimateContext *uc;
  
  DBG(fprintf(TheScreen.errout,"HandleDestroyNotify\n"));
  
  if(!XFindContext(disp,event->xdestroywindow.window,UWMContext,\
                                                (XPointer *)&uc))
    DeUltimizeWin(uc, False);
}

void HandleEnterNotify(XEvent *event)
{
  UltimateContext *uc;
  
  DBG(fprintf(TheScreen.errout,"HandleEnterNotify\n"));

  StampTime(event->xcrossing.time);

  if(!XFindContext(disp, event->xcrossing.window, UWMContext, (XPointer *)&uc)){
    if(event->xcrossing.window == uc->frame)
      ActivateWin(uc);
    else if((event->xcrossing.window == uc->title.win)
            && (InitS.BorderTitleFlags & BT_DODGY_TITLE))
      XLowerWindow(disp,uc->title.win);
  }
}

void HandleLeaveNotify(XEvent *event)
{ 
  StampTime(event->xcrossing.time);
}

void HandleExpose(XEvent *event)
{
  UltimateContext *uc;

  DBG(fprintf(TheScreen.errout,"HandleExpose\n"));

  if(event->xexpose.count) return;
  if(!XFindContext(disp, event->xexpose.window, UWMContext, (XPointer *)&uc)) {
    if(event->xexpose.window == uc->title.win)
      DrawTitle(uc);
    else if(event->xexpose.window == uc->border) {
      if(!(uc->flags & SHAPED)) DrawFrameBevel(uc);
    }
  }
}

void HandleMapRequest(XEvent *event)
{
  UltimateContext *uc;
  
  DBG(fprintf(TheScreen.errout, "HandleMapRequest\n");)

  if(XFindContext(disp, event->xmaprequest.window, UWMContext, 
                  (XPointer *)&uc)) {
    DBG(fprintf(TheScreen.errout, "HandleMapRequest: ultimizing... ");)
    uc = UltimizeWin(event->xmaprequest.window);
    if(!uc) {
      DBG(fprintf(TheScreen.errout, "fail.\n");)
      XMapWindow(disp, event->xmaprequest.window);
      return;
    }
    DBG(fprintf(TheScreen.errout, "ok.\n");)
  }
  if(event->xmaprequest.window == uc->win) {
    if((uc->WMHints) && (uc->WMHints->flags & StateHint)) {
      DBG(fprintf(TheScreen.errout, "HandleMapRequest: StateHint found\n");)
      switch(uc->WMHints->initial_state) {
        case NormalState:
            DBG(fprintf(TheScreen.errout, "HandleMapRequest: displaying\n");)
            DisplayWin(uc);
            break;
        case IconicState:
            DBG(fprintf(TheScreen.errout, "HandleMapRequest: iconifying\n");)
            IconifyWin(uc);
            break;
      }
    } else {
      DBG(fprintf(TheScreen.errout, "HandleMapRequest: displaying\n");)
      DisplayWin(uc);
    }
  } else {
    XMapWindow(disp, event->xmaprequest.window);
  }
  UpdateUWMContext(uc);
}

void HandleMapNotify(XEvent *event)
{
  UltimateContext *uc;

  DBG(fprintf(TheScreen.errout,"HandleMapNotify\n"));

  if(!XFindContext(disp, event->xmap.window, UWMContext, (XPointer *)&uc)) {
    if(uc->title.win != None) {
      if(InitS.BorderTitleFlags 
         & ((uc == ActiveWin) ? BT_ACTIVE_TITLE : BT_INACTIVE_TITLE)) {
        XRaiseWindow(disp, uc->title.win);
      } else {
        XLowerWindow(disp, uc->title.win);
      }
    }
  }
}

void HandleUnmapNotify(XEvent *event)
{
  UltimateContext *uc;
  XEvent event2;

  DBG(fprintf(TheScreen.errout,"HandleUnmapNotify\n");)

  if(!XFindContext(disp, event->xunmap.window, UWMContext, (XPointer *)&uc)) {
    if(event->xunmap.window == uc->win) {
      if(uc->expected_unmap_events) {
        uc->expected_unmap_events--;
        return;
      }
      if((!XCheckTypedWindowEvent(disp, event->xunmap.event, MapNotify,
                                  &event2))
         && (uc->frame != None)) XUnmapWindow(disp, uc->frame);
/*      if(uc->frame != None) XUnmapWindow(disp, uc->frame); */
      if(uc == ActiveWin) ActivateWin(NULL);
      GrabServer();
      XSync(disp, False);
      if(XCheckTypedWindowEvent(disp, event->xunmap.event,
                                DestroyNotify, &event2)
         && (event2.xdestroywindow.window == uc->win)) {
        DeUltimizeWin(uc, False);
      } else {
        UpdateUWMContext(uc);
        if(uc->own_unmap_events) uc->own_unmap_events--;
        else SetIsMapState(uc, WithdrawnState);
      }
      UngrabServer();
    } else UpdateUWMContext(uc);
  }
}

/* windows reparented into other client windows must not be handled by uwm */
void HandleReparentNotify(XEvent *event)
{
  UltimateContext *uc;

  DBG(fprintf(TheScreen.errout,"HandleUnmapNotify\n");)
  
  if(event->xreparent.parent == TheScreen.root) return;
                                        /* ignore reparenting to root */
  if(!XFindContext(disp, event->xreparent.window, UWMContext,
                   (XPointer *)&uc)) {
    if((event->xreparent.window == uc->win)
       && (event->xreparent.parent != uc->frame)) {
      DeUltimizeWin(uc, True);
    }
  }
}

void HandleButtonPress(XEvent *event)
{
  UltimateContext *uc;

  DBG(fprintf(TheScreen.errout,"HandleButtonPress\n");)

  StampTime(event->xbutton.time);

  if((event->xbutton.window == TheScreen.root)
     && (event->xbutton.subwindow == None)){
    switch(event->xbutton.button){
      case Button1: ShowMenu(0,event->xbutton.x,event->xbutton.y);
                    break;
      case Button2: ShowMenu(1,event->xbutton.x,event->xbutton.y); 
                    break;
      case Button3: ShowMenu(2,event->xbutton.x,event->xbutton.y);
                    break;
      case Button4: break;
      case Button5: break;
    }
  } else if((InitS.BehaviourFlags & BF_IN_WIN_CTRL)
            || (event->xbutton.window != TheScreen.root)
            || ( ( event->xbutton.state & ignore_mask ) == UWM_MODIFIERS)) {
    Window win;

    win = (event->xbutton.subwindow == None) ? event->xbutton.window 
          : event->xbutton.subwindow;
 
    if(!XFindContext(disp, win, UWMContext, (XPointer *)&uc)){
      int x,y;

      if(uc->frame != None){
        x = event->xbutton.x_root - uc->Attr.x;
        y = event->xbutton.y_root - uc->Attr.y;
      ActivateWin(uc);
        switch(event->xbutton.button){
          case Button1: BorderButton(0,uc,x,y,event->xbutton.x_root,\
                                              event->xbutton.y_root);
                        break;
          case Button2: BorderButton(1,uc,x,y,event->xbutton.x_root,\
                                              event->xbutton.y_root);
                        break;
          case Button3: BorderButton(2,uc,x,y,event->xbutton.x_root,\
                                              event->xbutton.y_root);
                        break;
          case Button4: DBG(fprintf(TheScreen.errout,"4\n");)break;
          case Button5: DBG(fprintf(TheScreen.errout,"5\n");)break;
        }
      }
    }
  }
}

void HandleButtonRelease(XEvent *event)
{
  StampTime(event->xbutton.time);
}

void HandleMotionNotify(XEvent *event)
{
  StampTime(event->xmotion.time);
}

void HandleConfigureRequest(XEvent *event)
{
  UltimateContext *uc;
  XWindowChanges xwc;
  
  DBG(fprintf(TheScreen.errout,"HandleConfigureRequest\n");)
  
  xwc.x = event->xconfigurerequest.x;
  xwc.y = event->xconfigurerequest.y;
  xwc.width = event->xconfigurerequest.width;
  xwc.height = event->xconfigurerequest.height;
  xwc.border_width = 0;
  if(event->xconfigurerequest.value_mask & CWSibling) {
    UltimateContext *sc;
    if((!XFindContext(disp, event->xconfigurerequest.above, UWMContext,
                      (XPointer *) &sc))
       && (sc->frame != None))
      xwc.sibling = sc->frame;
    else xwc.sibling = event->xconfigurerequest.above;
  }
  xwc.stack_mode = event->xconfigurerequest.detail;

  if((!XFindContext(disp, event->xconfigurerequest.window, UWMContext,
                    (XPointer *)&uc))
     && (event->xconfigurerequest.window == uc->win)) {
    if(event->xconfigurerequest.value_mask & (CWX | CWY | CWWidth | CWHeight)) {
      if(uc->frame != None) {
        XConfigureWindow(disp, uc->win, event->xconfigurerequest.value_mask
                         & (CWWidth | CWHeight), &xwc);
        xwc.width += 2 * uc->BorderWidth;
        xwc.height += TheScreen.TitleHeight + 2 * uc->BorderWidth;
        XConfigureWindow(disp, uc->border, event->xconfigurerequest.value_mask
                         & (CWWidth | CWHeight), &xwc);
        if(event->xconfigurerequest.value_mask & (CWX | CWY))
          GravitizeWin(uc, &(xwc.x), &(xwc.y), UWM_GRAVITIZE);
        XConfigureWindow(disp, uc->frame, event->xconfigurerequest.value_mask
                         & (CWSibling | CWStackMode | CWX | CWY
                            | CWWidth | CWHeight), &xwc);
        if((event->xconfigurerequest.value_mask & CWWidth)
           && (uc->title.win != None)
           && ((InitS.BorderTitleFlags & BT_CENTER_TITLE) 
               || (uc->flags & SHAPED))) {
          XMoveWindow(disp, uc->title.win,
                      uc->title.x = (xwc.width - uc->title.width) / 2,
                      uc->title.y);
        }
        if((event->xconfigurerequest.value_mask & CWStackMode)
           || ((event->xconfigurerequest.value_mask & (CWX|CWY))
           && (!(event->xconfigurerequest.value_mask & (CWWidth|CWHeight)))))
          SendConfigureEvent(uc);
      } else {
        XConfigureWindow(disp, uc->win, event->xconfigurerequest.value_mask,
                         &xwc);
      }
    }
    UpdateUWMContext(uc);
  } else XConfigureWindow(disp, event->xconfigurerequest.window,
                          event->xconfigurerequest.value_mask, &xwc);
}

/*void HandleColormapNotify(XEvent *event)
{
}*/

void HandleClientMsg(XEvent *event)
{
  
  DBG(fprintf(TheScreen.errout,"HandleClientMsg\n");)

  if((event->xclient.message_type == WM_CHANGE_STATE)&&\
             (event->xclient.data.l[0] == IconicState)){
    UltimateContext *uc;
    if(!XFindContext(disp,event->xclient.window,UWMContext,(XPointer *)&uc)){
      IconifyWin(uc);
    }
  }
}

void HandleKeyPress(XEvent *event)
{
  StampTime(event->xkey.time);
}

void HandleKeyRelease(XEvent *event)
{
  DBG(fprintf(TheScreen.errout,"HandleKeyRelease\n");)
  
  StampTime(event->xkey.time);

  if(event->xkey.root != TheScreen.root) return;

  if( (event->xkey.state & ignore_mask)==UWM_MODIFIERS){
    Node *n,*n2;
    int dummy;
    switch(*XGetKeyboardMapping(disp,event->xkey.keycode,1,&dummy)){
      case XK_Right: ChangeWS((TheScreen.desktop.ActiveWorkSpace +1)\
                                     % TheScreen.desktop.WorkSpaces);
                     break;
      case XK_Left:  ChangeWS((TheScreen.desktop.ActiveWorkSpace
                              + TheScreen.desktop.WorkSpaces
                              - 1) % TheScreen.desktop.WorkSpaces);
                     break;
      case XK_Up:    n=n2=InNodeList(TheScreen.UltimateList, ActiveWin);{
                       do {
                         if((n2=NodePrev(TheScreen.UltimateList, n2)))
                           if(WinVisible(n2->data)) break;
                       } while(n!=n2);
                       if(n2) ActivateWin(n2->data);
                       else ActivateWin(NULL);
                     }
                     break;
      case XK_Down:  n=n2=InNodeList(TheScreen.UltimateList, ActiveWin);{
                       do {
                         if((n2=NodeNext(TheScreen.UltimateList, n2)))
                           if(WinVisible(n2->data)) break;
                       } while(n!=n2);
                       if(n2) ActivateWin(n2->data);
                       else ActivateWin(NULL);
                     }
                     break;
      case XK_Page_Up: if(ActiveWin) RaiseWin(ActiveWin);
                     break;
      case XK_Page_Down: if(ActiveWin) LowerWin(ActiveWin);
                     break;
      case XK_End:   if(ActiveWin) IconifyWin(ActiveWin);
                     break;
    }
  }
}

void HandlePropertyNotify(XEvent *event)
{
  UltimateContext *uc;
  
  DBG(fprintf(TheScreen.errout,"HandlePropertyNotify\n");)

  StampTime(event->xproperty.time);

  GrabServer();
  if(!XFindContext(disp,event->xproperty.window,UWMContext,(XPointer *)&uc)){
    if((event->xproperty.window == uc->win)) switch(event->xproperty.atom){
      case XA_WM_NORMAL_HINTS: Updatera(uc); break;
      case XA_WM_NAME: UpdateName(uc); break;
      case XA_WM_ICON_NAME: UpdateIconName(uc); break;
      case XA_WM_HINTS: UpdateWMHints(uc); break;
      default:
        if(event->xproperty.atom==MOTIF_WM_HINTS) UpdateMotifHints(uc);
        if(event->xproperty.atom==WM_PROTOCOLS)
          UpdateWMProtocols(uc);
    }
  }
  UngrabServer();
}  

void HandleShape(XEvent *event)
{
  UltimateContext *uc;
  XShapeEvent *shev;
  
  DBG(fprintf(TheScreen.errout,"HandleShape\n");)

  shev=(XShapeEvent *)event;
  if(!XFindContext(disp,shev->window,UWMContext,(XPointer *)&uc)){
    ShapeFrame(uc);
  }
}  

void HandleSelectionClear(XEvent *event)
{
  DBG(fprintf(TheScreen.errout,"HandleSelectionClear\n");)

  StampTime(event->xselectionclear.time);

  if((event->xselectionclear.selection == TheScreen.WM_Sx)
     && (TheScreen.inputwin != XGetSelectionOwner(disp, TheScreen.WM_Sx))){
    if(!(InitS.icccmFlags & ICF_STAY_ALIVE))
      SeeYa(0,"passing on control to another wm");
    else XSetSelectionOwner(disp, TheScreen.WM_Sx, TheScreen.inputwin,
                            event->xselectionclear.time);
  }
}

void HandleSelectionRequest(XEvent *event)
{
  Atom prop = None;

  DBG(fprintf(TheScreen.errout,"HandleSelectionRequest\n");)

  StampTime(event->xselectionrequest.time);

  if(event->xselectionrequest.selection == TheScreen.WM_Sx) {
    if((event->xselectionrequest.time >= TheScreen.start_tstamp)
       || (event->xselectionrequest.time == CurrentTime)){
      prop = Parse_WM_Sx_Selection(event->xselectionrequest.target, 
                                   event->xselectionrequest.property,
                                   event->xselectionrequest.requestor);
    }
  }
  SendSelectionNotify(event, prop);
}

void HandleFocusIn(XEvent *event)
{
  UltimateContext *uc;

  if(!XFindContext(disp, event->xfocus.window, UWMContext, (XPointer *)&uc)){
    FocusWin = uc;
    if(event->xfocus.detail != NotifyPointer) FocusWin = uc;
    DrawWinBorder(uc);
    /* if(!(uc->flags & ACTIVE_BORDER)) DrawWinBorder(uc); */
    XInstallColormap(disp, uc->Attributes.colormap);
  }
}

void HandleFocusOut(XEvent *event)
{
  UltimateContext *uc;

  FocusWin = NULL;
  if(!XFindContext(disp, event->xfocus.window, UWMContext, (XPointer *)&uc)){
    DrawWinBorder(uc);
    /* if(uc->flags & ACTIVE_BORDER) DrawWinBorder(uc); */
    XInstallColormap(disp, TheScreen.colormap);
  }
}

/***********************/
void InitHandlers()
{
  int i;
  
  for(i = 0; i < LASTEvent; i++)
    DefaultHandle[i] = NULL;
  DefaultHandle[Expose] = HandleExpose;
/*  DefaultHandle[CreateNotify] = HandleCreateNotify; */
  DefaultHandle[DestroyNotify] = HandleDestroyNotify;
  DefaultHandle[UnmapNotify] = HandleUnmapNotify;
  DefaultHandle[ClientMessage] = HandleClientMsg;
  DefaultHandle[ConfigureRequest] = HandleConfigureRequest;
/*  DefaultHandle[ColormapNotify] = HandleColormap; */
  DefaultHandle[MapRequest] = HandleMapRequest;
  DefaultHandle[ReparentNotify] = HandleReparentNotify;
  DefaultHandle[MapNotify] = HandleMapNotify;
  DefaultHandle[EnterNotify] = HandleEnterNotify;
  DefaultHandle[LeaveNotify] = HandleLeaveNotify;
  DefaultHandle[ButtonPress] = HandleButtonPress;
  DefaultHandle[MotionNotify] = HandleMotionNotify; 
  DefaultHandle[ButtonRelease] = HandleButtonRelease;
  DefaultHandle[KeyPress] = HandleKeyPress;
  DefaultHandle[KeyRelease] = HandleKeyRelease;
  DefaultHandle[PropertyNotify] = HandlePropertyNotify;
  DefaultHandle[SelectionClear] = HandleSelectionClear;
  DefaultHandle[SelectionRequest] = HandleSelectionRequest;
  DefaultHandle[FocusIn] = HandleFocusIn;
  DefaultHandle[FocusOut] = HandleFocusOut;

  if(!XShapeQueryExtension(disp,&ShapeEvent,&i))
     SeeYa(-1, "Your X-server doesn't support the Shapes extension which is required for uwm");
  ShapeEvent += ShapeNotify;

  Handle = DefaultHandle;
  for(i = 0;i<LASTEvent;i++) MoveHandle[i]=DefaultHandle[i];
  MoveHandle[EnterNotify] = NULL;
  MoveHandle[LeaveNotify] = NULL;
  MoveHandle[ButtonPress] = MoveButtonPress;
  MoveHandle[MotionNotify] = MoveMotion;
  MoveHandle[ButtonRelease] = MoveButtonRelease;
  MoveHandle[UnmapNotify] = MoveUnmap;
  for(i = 0;i<LASTEvent;i++) ResizeHandle[i]=DefaultHandle[i];
  ResizeHandle[EnterNotify] = NULL;
  ResizeHandle[LeaveNotify] = NULL;
  ResizeHandle[ButtonPress] = ResizeButtonPress;
  ResizeHandle[MotionNotify] = ResizeMotion;
  ResizeHandle[ButtonRelease] = ResizeButtonRelease;
  for(i = 0;i<LASTEvent;i++) WinMenuHandle[i]=DefaultHandle[i];
  WinMenuHandle[EnterNotify] = WinMenuEnterNotify;
  WinMenuHandle[LeaveNotify] = WinMenuEnterNotify;
  WinMenuHandle[ButtonPress] = WinMenuButtonPress;
  WinMenuHandle[ButtonRelease] = WinMenuButtonRelease;
  WinMenuHandle[VisibilityNotify] = WinMenuVisibility;
  WinMenuHandle[UnmapNotify] = WinMenuUnmapNotify;
  for(i = 0;i<LASTEvent;i++) MenuHandle[i]=DefaultHandle[i];
  MenuHandle[EnterNotify] = MenuEnterNotify;
  MenuHandle[LeaveNotify] = MenuLeaveNotify;
  MenuHandle[ButtonPress] = MenuButtonPress;
  MenuHandle[ButtonRelease] = MenuButtonRelease;
  MenuHandle[VisibilityNotify] = MenuVisibility;
  MenuHandle[Expose] = MenuExpose;
}

/**********/

void ReinstallDefaultHandle()
{
  Handle = DefaultHandle;
}

void InstallMoveHandle()
{
  Handle = MoveHandle;
}

void InstallResizeHandle()
{
  Handle = ResizeHandle;
}

void InstallWinMenuHandle()
{
  Handle = WinMenuHandle;
}

void InstallMenuHandle()
{
  Handle = MenuHandle;
}
