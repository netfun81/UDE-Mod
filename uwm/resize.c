/***  RESIZE.C: Contains the handling routines for signals and events
                                invoked during a resize process  ***/

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

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "uwm.h"
#include "handlers.h"
#include "windows.h"
#include "rubber.h"
#include "init.h"
#include "special.h"

extern UDEScreen TheScreen;
extern Display *disp;
extern UltimateContext *ActiveWin;
extern InitStruct InitS;

struct {
  int ox1,ox2,oy1,oy2,x1,x2,y1,y2,minw,maxw,minh,maxh,wi,hi;
  int bw,bh;
} resize_state;

short risen;  /*  0: normal resize; 1: fullscreen; 2:old size */
short risenstart; /* started in risen mode? */

#define LACTIVE (1<<0)
#define RACTIVE (1<<1)
#define TACTIVE (1<<2)
#define BACTIVE (1<<3)
#define XACTIVE (1<<4)
#define YACTIVE (1<<5)
unsigned char borderstat,oldstat;

void SqueezeIt(int mousex,int mousey)
{
  if(!(borderstat & TACTIVE)) 
    if(mousey <= (resize_state.oy1+ActiveWin->BorderWidth+TheScreen.TitleHeight)){
      borderstat&=(~(BACTIVE|YACTIVE));
      borderstat|=TACTIVE;
      resize_state.y2=resize_state.oy2;
    }
  if(!(borderstat & BACTIVE)) 
    if(mousey >= (resize_state.oy2-ActiveWin->BorderWidth)){
      borderstat&=(~(TACTIVE|YACTIVE));
      borderstat|=BACTIVE;
      resize_state.y1=resize_state.oy1;
    }
  if(!(borderstat & LACTIVE)) 
    if(mousex <= (resize_state.ox1+ActiveWin->BorderWidth)){
      borderstat&=(~(RACTIVE|XACTIVE));
      borderstat|=LACTIVE;
      resize_state.x2=resize_state.ox2;
    }
  if(!(borderstat & RACTIVE)) 
    if(mousex >= (resize_state.ox2-ActiveWin->BorderWidth)){
      borderstat&=(~(LACTIVE|XACTIVE));
      borderstat|=RACTIVE;
      resize_state.x1=resize_state.ox1;
    }

  if(borderstat & TACTIVE){
    if((!(borderstat & YACTIVE)) && ((mousey<resize_state.oy1)||\
      (mousey>resize_state.oy1+ActiveWin->BorderWidth+TheScreen.TitleHeight)))
      borderstat|=YACTIVE;
    if(borderstat & YACTIVE){
      resize_state.y1=mousey;
      if((resize_state.y2-resize_state.y1)<resize_state.minh) resize_state.y1=resize_state.y2-resize_state.minh;
      if((resize_state.y2-resize_state.y1)>resize_state.maxh) resize_state.y1=resize_state.y2-resize_state.maxh;
      resize_state.y1=resize_state.y2-((int)((resize_state.y2-resize_state.y1-resize_state.bh)/resize_state.hi))*resize_state.hi-resize_state.bh;
    }
  }
  if(borderstat & BACTIVE){
    if((!(borderstat & YACTIVE)) && ((mousey>resize_state.oy2)||\
      (mousey<resize_state.oy2-ActiveWin->BorderWidth)))
      borderstat|=YACTIVE;
    if(borderstat & YACTIVE){
      resize_state.y2=mousey;
      if((resize_state.y2-resize_state.y1)<resize_state.minh) resize_state.y2=resize_state.y1+resize_state.minh;
      if((resize_state.y2-resize_state.y1)>resize_state.maxh) resize_state.y2=resize_state.y1+resize_state.maxh;
      resize_state.y2=resize_state.y1+((int)((resize_state.y2-resize_state.y1-resize_state.bh)/resize_state.hi))*resize_state.hi+resize_state.bh;
    }
  }
  if(borderstat & LACTIVE){
    if((!(borderstat & XACTIVE)) && ((mousex<resize_state.ox1)||\
      (mousex>resize_state.ox1+ActiveWin->BorderWidth)))
      borderstat|=XACTIVE;
    if(borderstat & XACTIVE){
      resize_state.x1=mousex;
      if((resize_state.x2-resize_state.x1)<resize_state.minw) resize_state.x1=resize_state.x2-resize_state.minw;
      if((resize_state.x2-resize_state.x1)>resize_state.maxw) resize_state.x1=resize_state.x2-resize_state.maxw;
      resize_state.x1=resize_state.x2-((int)((resize_state.x2-resize_state.x1-resize_state.bw)/resize_state.wi))*resize_state.wi-resize_state.bw;
    }
  }
  if(borderstat & RACTIVE){
    if((!(borderstat & XACTIVE)) && ((mousex>resize_state.ox2)||\
      (mousex<resize_state.ox2-ActiveWin->BorderWidth)))
      borderstat|=XACTIVE;
    if(borderstat & XACTIVE){
      resize_state.x2=mousex;
      if((resize_state.x2-resize_state.x1)<resize_state.minw) resize_state.x2=resize_state.x1+resize_state.minw;
      if((resize_state.x2-resize_state.x1)>resize_state.maxw) resize_state.x2=resize_state.x1+resize_state.maxw;
      resize_state.x2=resize_state.x1+((int)((resize_state.x2-resize_state.x1-resize_state.bw)/resize_state.wi))*resize_state.wi+resize_state.bw;
    }
  }
  if(borderstat!=oldstat){
    oldstat = borderstat;
    switch(borderstat&(RACTIVE|LACTIVE|TACTIVE|BACTIVE)){
      case RACTIVE: 
        XChangeActivePointerGrab(disp,PointerMotionMask|ButtonPressMask|\
          ButtonReleaseMask,TheScreen.Mice[C_O],TimeStamp);
        break;
      case RACTIVE|TACTIVE:
        XChangeActivePointerGrab(disp,PointerMotionMask|ButtonPressMask|\
          ButtonReleaseMask,TheScreen.Mice[C_NO],TimeStamp);
        break;
      case TACTIVE:
        XChangeActivePointerGrab(disp,PointerMotionMask|ButtonPressMask|\
          ButtonReleaseMask,TheScreen.Mice[C_N],TimeStamp);
        break;
      case TACTIVE|LACTIVE:
        XChangeActivePointerGrab(disp,PointerMotionMask|ButtonPressMask|\
          ButtonReleaseMask,TheScreen.Mice[C_NW],TimeStamp);
        break;
      case LACTIVE:
        XChangeActivePointerGrab(disp,PointerMotionMask|ButtonPressMask|\
          ButtonReleaseMask,TheScreen.Mice[C_W],TimeStamp);
        break;
      case LACTIVE|BACTIVE:
        XChangeActivePointerGrab(disp,PointerMotionMask|ButtonPressMask|\
          ButtonReleaseMask,TheScreen.Mice[C_SW],TimeStamp);
        break;
      case BACTIVE:
        XChangeActivePointerGrab(disp,PointerMotionMask|ButtonPressMask|\
          ButtonReleaseMask,TheScreen.Mice[C_S],TimeStamp);
        break;
      case BACTIVE|RACTIVE:
        XChangeActivePointerGrab(disp,PointerMotionMask|ButtonPressMask|\
          ButtonReleaseMask,TheScreen.Mice[C_SO],TimeStamp);
        break;
      default:
        XChangeActivePointerGrab(disp,PointerMotionMask|ButtonPressMask|\
          ButtonReleaseMask,TheScreen.Mice[C_DEFAULT],TimeStamp);
        break;
    }
  }
  SqueezeRubber(resize_state.x1,resize_state.y1,resize_state.x2-resize_state.x1,resize_state.y2-resize_state.y1);
}

void RiseIt()
{
  if((!risenstart)&&(risen!=1)){
    resize_state.x1=resize_state.y1=0;
    resize_state.x2=(resize_state.maxw>TheScreen.width)?(resize_state.bw+((int)((TheScreen.width-resize_state.bw-1)/resize_state.wi))*resize_state.wi):resize_state.maxw;
    resize_state.y2=(resize_state.maxh>TheScreen.height)?(resize_state.bh+((int)((TheScreen.height-resize_state.bh-1)/resize_state.hi))*resize_state.hi):resize_state.maxh;
    SqueezeRubber(resize_state.x1,resize_state.y1,resize_state.x2-resize_state.x1,resize_state.y2-resize_state.y1);
    risen=1;
  } else {
    resize_state.x1=ActiveWin->ra.x;resize_state.y1=ActiveWin->ra.y;
    resize_state.x2=resize_state.x1+ActiveWin->ra.w;resize_state.y2=resize_state.y1+ActiveWin->ra.h;
    SqueezeRubber(resize_state.x1,resize_state.y1,resize_state.x2-resize_state.x1,resize_state.y2-resize_state.y1);
    risen=2;
    risenstart=False;
  }
    XChangeActivePointerGrab(disp,PointerMotionMask|ButtonPressMask|\
            ButtonReleaseMask,TheScreen.Mice[C_DEFAULT],TimeStamp);
}

void UnriseIt(int x,int y)
{
  resize_state.x1=resize_state.ox1;resize_state.x2=resize_state.ox2;resize_state.y1=resize_state.oy1;resize_state.y2=resize_state.oy2;
  borderstat=0;
  SqueezeIt(x,y);
  risen=0;
}

/* StartResizing installs the handlers used to resize a window
 * requires absolute coordinates (relative to root)           */

void StartResizing(UltimateContext *uc,int x,int y)
{
  risen=0;

  resize_state.minh=uc->ra.minh;    /*** lazy dayze ***/
  resize_state.maxh=uc->ra.maxh;
  resize_state.minw=uc->ra.minw;
  resize_state.maxw=uc->ra.maxw;
  resize_state.wi=uc->ra.wi;
  resize_state.hi=uc->ra.hi;
  resize_state.bw=uc->ra.bw;
  resize_state.bh=uc->ra.bh;

  resize_state.x1=resize_state.ox1=uc->Attr.x;
  resize_state.x2=resize_state.ox2=((uc->Attr.x)+(uc->Attr.width));
  resize_state.y1=resize_state.oy1=uc->Attr.y;
  resize_state.y2=resize_state.oy2=((uc->Attr.y)+(uc->Attr.height));
  if(!(uc->flags & RISEN)){
    ActiveWin->ra.x=resize_state.ox1;
    ActiveWin->ra.y=resize_state.oy1;
    ActiveWin->ra.w=resize_state.ox2-resize_state.ox1;
    ActiveWin->ra.h=resize_state.oy2-resize_state.oy1;
    risenstart=False;
  } else risenstart=True;
  borderstat=oldstat=0;

  GrabPointer(TheScreen.root,PointerMotionMask|ButtonPressMask|\
                                        ButtonReleaseMask,None);
  StartRubber(resize_state.x1,resize_state.y1,resize_state.x2-resize_state.x1,resize_state.y2-resize_state.y1,ActiveWin->BorderWidth);
  GrabServer();
  InstallResizeHandle();
  if(!(uc->flags & RISEN)) SqueezeIt(x,y);

  DBG( fprintf(TheScreen.errout,"min w: %u; h: %u max w: %u; h: %u; wi: %u; hi: %u\n",\
       resize_state.minw,resize_state.minh,resize_state.maxw,resize_state.maxh,resize_state.wi,resize_state.hi);)
}

void ResizeButtons(int a,XEvent *event)
{
  switch(InitS.ResizeButtons[a]){
    case 'U': UnriseIt(event->xbutton.x_root,event->xbutton.y_root);break;
    case 'A': RiseIt();break;
    case 'R': break;
  }
}

void ResizeButtonPress(XEvent *event)
{
  DBG(fprintf(TheScreen.errout,"ResizeButtonPress\n");)
  StampTime(event->xbutton.time);
  switch(event->xbutton.button){
    case Button1: ResizeButtons(0,event);break;
    case Button2: ResizeButtons(1,event);break;
    case Button3: ResizeButtons(2,event);break;
    case Button4:
    case Button5: break;
  }
}

void ResizeMotion(XEvent *event)
{
  StampTime(event->xmotion.time);
  if(!risen) SqueezeIt(event->xmotion.x_root,event->xmotion.y_root);
}

void ResizeButtonRelease(XEvent *event)
{
  int x,y,width,height;
  DBG(fprintf(TheScreen.errout,"ResizeButtonRelease\n");)
  StampTime(event->xbutton.time);
  if(ButtonCount(event->xbutton.state)>1) return;
  if(risen==1) ActiveWin->flags |= RISEN;
  else ActiveWin->flags &=~ RISEN;
  StopRubber(&x,&y,&width,&height);
  MoveResizeWin(ActiveWin,x,y,width,height);
  UngrabPointer();
  ReinstallDefaultHandle();
  UngrabServer();
}
