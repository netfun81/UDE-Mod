/*** PROPERTIES.C: Contains routines that update data taken from and written to
                       window properties in the UltimateContexts ***/

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
#include <string.h>
#include <limits.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "uwm.h"
#include "windows.h"
#include "init.h"
#include "properties.h"
#include "wingroups.h"
#include "widgets.h"
#include "special.h"

extern UDEScreen TheScreen;
extern Display *disp;
extern InitStruct InitS;
extern UltimateContext *ActiveWin;
extern Atom WM_STATE_PROPERTY;
extern Atom WM_TAKE_FOCUS;
extern Atom WM_DELETE_WINDOW;
extern Atom WM_PROTOCOLS;
extern Atom MOTIF_WM_HINTS;

void UpdateUWMContext(UltimateContext *uc)
{
  XGetWindowAttributes(disp,uc->win,&(uc->Attributes));
  if(uc->frame) XGetWindowAttributes(disp,uc->frame,&(uc->Attr));
}

/*** Updatera will update the given uc's ra structure, leaves the x,y,r,h-members untouched. ***/

void Updatera(UltimateContext *uc)
{
  XSizeHints sizehints;
  long supplied;

  if(!XGetWMNormalHints(disp,uc->win,&sizehints,&supplied)){
    sizehints.flags = 0;
    uc->flags |= PLACEIT;
  }

  if(sizehints.flags & PWinGravity) uc->ra.gravity = sizehints.win_gravity;
  else uc->ra.gravity = StaticGravity;

  if(sizehints.flags & USPosition) uc->flags &= ~PLACEIT;

  if(sizehints.flags & PResizeInc) {
    uc->ra.wi=sizehints.width_inc;
    uc->ra.hi=sizehints.height_inc;
  } else uc->ra.wi=uc->ra.hi=1;

  if(sizehints.flags & PBaseSize) {
    uc->ra.bw=sizehints.base_width+2*uc->BorderWidth;
    uc->ra.bh=sizehints.base_height+2*uc->BorderWidth+TheScreen.TitleHeight;
  } else if(sizehints.flags & PMinSize) {
    uc->ra.bw=sizehints.min_width+2*uc->BorderWidth;
    uc->ra.bh=sizehints.min_height+2*uc->BorderWidth+TheScreen.TitleHeight;
  } else {
    uc->ra.bw= 1 + 2*uc->BorderWidth;
    uc->ra.bh= 1 + 2*uc->BorderWidth + TheScreen.TitleHeight;
  }
  if(sizehints.flags & PMinSize) {
    uc->ra.minw=sizehints.min_width+2*uc->BorderWidth;
    uc->ra.minh=sizehints.min_height+2*uc->BorderWidth+TheScreen.TitleHeight;
  } else {
    uc->ra.minw=uc->ra.bw;
    uc->ra.minh=uc->ra.bh;
  }
  if(sizehints.flags &PMaxSize) {
    uc->ra.maxw=sizehints.max_width+2*uc->BorderWidth;
    if (uc->ra.maxw<0) /* deal with clients setting max_width to INT_MAX */
      uc->ra.maxw=INT_MAX;
    uc->ra.maxh=sizehints.max_height+2*uc->BorderWidth+TheScreen.TitleHeight;
    if (uc->ra.maxh<0) /* deal with clients setting max_width to INT_MAX */
      uc->ra.maxh=INT_MAX;
  } else {
    uc->ra.maxw=TheScreen.width+1;
    uc->ra.maxh=TheScreen.height+1;
  }

  if(uc->ra.bw>uc->ra.minw) uc->ra.minw=uc->ra.bw;
  if(uc->ra.bh>uc->ra.minh) uc->ra.minh=uc->ra.bh;
/*** Aspect ratios not supported (yet?) ***/
}

void UpdateName(UltimateContext *uc)
{
  XTextProperty prop;
  if(uc->title.name) {
    free(uc->title.name);
  }
  if(!XGetWMName(disp, uc->win, &prop)) {
    uc->title.name = NULL;
  } else {
    char **stringlist;
    int count;
    if((XmbTextPropertyToTextList(disp, &prop, &stringlist, &count) >= 0)
       && (count > 0)) {
      uc->title.name = calloc(strlen(stringlist[0]) + 1, sizeof(char));
      if(uc->title.name) {
        strcpy(uc->title.name, stringlist[0]);
      }
      XFreeStringList(stringlist);
    } else {
      uc->title.name = NULL;
    }
    XFree(prop.value);
  }

  if(uc->title.win != None){
    if(uc->title.name) {
      int x, y, width, height;
      XRectangle r;
      x = uc->title.x;
      y = uc->title.y;
      width = uc->title.width;
      height = uc->title.height;
      XmbTextExtents(TheScreen.TitleFont, uc->title.name,
                     strlen(uc->title.name), NULL, &r);
      XResizeWindow(disp, uc->title.win,
                    uc->title.width = (r.width
                    + (((InitS.BorderTitleFlags & BT_CENTER_TITLE)
                        || (uc->flags & SHAPED)) ? 9 : 6)),
                    uc->title.height = (r.height
                    + ((uc->flags & SHAPED) ? 6 : 3)));
      if((InitS.BorderTitleFlags & BT_CENTER_TITLE) || (uc->flags & SHAPED))
        XMoveWindow(disp, uc->title.win,
                    uc->title.x = ((uc->Attr.width - uc->title.width) / 2),
                    uc->title.y = ((uc->flags & SHAPED) ? 0
                    : (uc->BorderWidth - TheScreen.FrameBevelWidth - 1) / 2
                    + TheScreen.FrameBevelWidth));
      if((!((uc->title.width == width) && (uc->title.height == height)
          && (uc->title.x == x) && (uc->title.y == y))) && (uc->flags & SHAPED))
         ShapeFrame(uc);
      DrawTitle(uc);
    } else XResizeWindow(disp,uc->title.win,0,0);
  }
  DBG(fprintf(TheScreen.errout,"Window Name: %s\n",uc->title.name);)
}

void UpdateIconName(UltimateContext *uc)
{
  XTextProperty prop;
  char **stringlist;
  int count;

  if(uc->title.iconname) free(uc->title.iconname);
  if(!XGetTextProperty(disp, uc->win, &prop, XA_WM_ICON_NAME)) {
    uc->title.iconname = NULL;
    return;
  }
  if((XmbTextPropertyToTextList(disp, &prop, &stringlist, &count) >= 0)
     && (count > 0)){
    uc->title.iconname = calloc(strlen(stringlist[0]) + 1, sizeof(char));
    if(uc->title.iconname) strcpy(uc->title.iconname, stringlist[0]);
    XFreeStringList(stringlist);
  } else uc->title.iconname = NULL;
  XFree(prop.value);

  DBG(fprintf(TheScreen.errout,"Window icon Name: %s\n",uc->title.iconname);)
}

void UpdateWMHints(UltimateContext *uc)
{
  if(uc->WMHints) XFree(uc->WMHints);
  uc->WMHints = XGetWMHints(disp,uc->win);
  if((uc == ActiveWin) && ((!uc->WMHints)
     ||(uc->WMHints && (uc->WMHints->flags & InputHint) && uc->WMHints->input)))
  {
    XSetInputFocus(disp, ActiveWin->win, RevertToPointerRoot, TimeStamp);
  }
  UpdateWinGroup(uc);
}

void UpdateMotifHints(UltimateContext *uc)
{
  int format;
  Atom type;
  unsigned long n;
  if(uc->MotifWMHints) XFree(uc->MotifWMHints);
  if(Success!=XGetWindowProperty(disp, uc->win, MOTIF_WM_HINTS, 0,
                                 PROP_MWM_HINTS_ELEMENTS, False, MOTIF_WM_HINTS,
                                 &type, &format, &n, &n,
                                 (unsigned char **)&uc->MotifWMHints))
    uc->MotifWMHints=NULL;

/*** From now on we ignore motif hint changes for enbordered windows.
     they make limited sense anyway and might cause serious trouble with
     some applications
  if(uc->frame) {
    char visible;

    visible = WinVisible(uc);
    DisenborderWin(uc,True);
    EnborderWin(uc);
    if(visible) MapWin(uc, True);
  } */
}

void UpdateTransientForHint(UltimateContext *uc)
{
  if(!XGetTransientForHint(disp, uc->win, &uc->TransientFor))
    uc->TransientFor = None;
}

void UpdateWMProtocols(UltimateContext *uc)
{
  Atom *prots;
  int count,a;

  uc->ProtocolFlags=0;
  if(XGetWMProtocols(disp,uc->win,&prots,&count)) {
    for(a=0;a<count;a++){
      if(prots[a]==WM_TAKE_FOCUS) {
        uc->ProtocolFlags|=TAKE_FOCUS;
        if(uc == ActiveWin) SendWMProtocols(uc, WM_TAKE_FOCUS);
      }
      if(prots[a]==WM_DELETE_WINDOW) uc->ProtocolFlags|=DELETE_WINDOW;
    }
    XFree(prots);
  }
}

void SetIsMapState(UltimateContext *uc, int state)
{
  SetSeemsMapState(uc, uc->uwmstate = state);
}

void SetSeemsMapState(UltimateContext *uc, int state)
{
  unsigned long data[2];
  uc->wmstate = data[0] = (unsigned long) state;
  data[1] = None;
/*  if(uc->WMHints) data[1] = (unsigned long) uc->WMHints->icon_window; */
  switch(state){
    case WithdrawnState: uc->uwmstate = state;
                         UnmapWin(uc);
                         DisenborderWin(uc, True);
                         break;
    default: break;
  }

  XChangeProperty(disp, uc->win, WM_STATE_PROPERTY, WM_STATE_PROPERTY, 32,
                  PropModeReplace, (unsigned char *) data, 2);
}
