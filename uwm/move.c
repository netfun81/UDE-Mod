/***  MOVE.C: Contains the handling routines for signals and events
                                  invoked during a move process  ***/

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

#include "uwm.h"
#include "init.h"
#include "handlers.h"
#include "windows.h"
#include "rubber.h"
#include "special.h"
#include "placement.h"
#include "nodes.h"

extern UDEScreen TheScreen;
extern Display *disp;
extern UltimateContext *ActiveWin;
extern InitStruct InitS;

void HandleUnmapNotify(XEvent *event);

unsigned int xofs,yofs,xstart,ystart;
Bool Risen,Rise,Riseit,RubberMove;
NodeList *movewins;

/* StartDragging installs handlers to be used during the dragging procedure *
 * requires coordinates relative to the frame window                        */

void StartDragging(UltimateContext *uc,unsigned int x,unsigned int y)
{
  xofs=x;
  yofs=y;
  xstart=uc->Attr.x;
  ystart=uc->Attr.y;
  if (uc->frame==None) return;
  GrabPointer(uc->frame, PointerMotionMask | PointerMotionHintMask
              | ButtonPressMask | ButtonReleaseMask, TheScreen.Mice[C_DRAG]);
  InstallMoveHandle();
  if(InitS.SnapDistance) movewins = ScanScreen(uc);
    
  RubberMove=InitS.RubberMove;
  if(((uc->Attr.width*uc->Attr.height)>InitS.OpaqueMoveSize)&&\
                         InitS.OpaqueMoveSize) RubberMove=True;
  Risen=False;
  Rise=False;
  if(RubberMove) {
    StartRubber(uc->Attr.x,uc->Attr.y,uc->Attr.width,uc->Attr.height,\
                                                     uc->BorderWidth);
  }
}

void MoveButtonPress(XEvent *event)
{
  DBG(fprintf(TheScreen.errout,"move button press\n");)
  StampTime(event->xbutton.time);
}

void MoveMotion(XEvent *event)
{
  int x, y, dummy;

  StampTime(event->xmotion.time);
  
  x = event->xmotion.x_root - xofs;
  y = event->xmotion.y_root - yofs;

  if(InitS.SnapDistance)
    SnapWin(movewins, &x, &y, ActiveWin->Attr.width, ActiveWin->Attr.height);
  
  if(RubberMove){
    SqueezeRubber(x, y, 0, 0);
  } else {
    MoveResizeWin(ActiveWin, x, y, 0, 0);
  }
  XGetMotionEvents(disp, TheScreen.root, 2, 1, &dummy);
  /* the start / stop times don't make sense here. this is because we just
     want to trigger the next motion event and these times are an efficient
     way to suppress any results we don't want */
    
}

void MoveUnmap(XEvent *event) /* in case a win is unmapped during MoveProcess */
{
  if(event->xunmap.window==ActiveWin->win) { /* REALLY?! */
    if(RubberMove) {
      int dummy;
      StopRubber(&dummy,&dummy,&dummy,&dummy);
    }
    ReinstallDefaultHandle();
    UngrabPointer();
  }
  HandleUnmapNotify(event);
}

void MoveButtons(int a,XEvent *event)
{
  switch(InitS.DragButtons[a]){
    case 'R':
      if(RubberMove){
        Riseit=Rise=True;
      } else RaiseWin(ActiveWin);
      Risen=True;
      break;
    case 'D':
      if(RubberMove) {
        int dummy,x,y;
        StopRubber(&x,&y,&dummy,&dummy);
        MoveResizeWin(ActiveWin, x, y, 0, 0);
        if(Rise){
          if(Riseit) RaiseWin(ActiveWin);
          else LowerWin(ActiveWin);
        }
      }
      if((abs(ActiveWin->Attr.x-xstart)<3)&&(abs(ActiveWin->Attr.y-ystart)<3)\
                                                                   &&(!Risen))
        LowerWin(ActiveWin);
      if(InitS.SnapDistance) FreeScanned(movewins);
      ReinstallDefaultHandle();
      UngrabPointer();
      break;
    case 'L':
      if(RubberMove){
        Rise=True;Riseit=False;
      } else LowerWin(ActiveWin);break;
  }
}

void MoveButtonRelease(XEvent *event)
{
  DBG(fprintf(TheScreen.errout,"move button release\n");)

  StampTime(event->xbutton.time);

  switch(event->xbutton.button) {
    case Button1: MoveButtons(0,event);break;
    case Button2: MoveButtons(1,event);break;
    case Button3: MoveButtons(2,event);break;
    case Button4:
    case Button5: break;
  }
}
