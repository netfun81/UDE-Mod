/***  PLACEMENT.C: Contains placement-strategies and snapping routines ***/

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


#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include "uwm.h"
#include "init.h"
#include "windows.h"
#include "nodes.h"
#include "rubber.h"
#include "placement.h"
#include "special.h"

#define FIRSTINC InitS.PlacementStrategy

#ifndef MIN
#define MIN(A,B) ((A)<(B)? (A):(B))
#endif
#ifndef MAX
#define MAX(A,B) ((A)>(B)? (A):(B))
#endif
#ifndef BETWEEN
#define BETWEEN(A,B,C) (((A)<=(B))&&((B)<=(C)))
#endif

#define GRADMAX (1000)  /* Should be sufficient on most machines, if it hasn't
                           terminated by then it usually won't. You may increase
                           this on very huge screens (such as 8000x5000 pixel)*/
extern Display *disp;
extern UDEScreen TheScreen;
extern InitStruct InitS;
extern UltimateContext *ActiveWin;

NodeList* ScanScreen(UltimateContext *win)
{
//  Window dummy,*children;
  Node *winNode;
  NodeList *wins;

  if(!(wins = NodeListCreate())) SeeYa(1, "FATAL: out of mem!");
  winNode = NULL;
  while((winNode = NodeNext(TheScreen.UltimateList, winNode))) {
    UltimateContext *uc = winNode->data;
    if(uc && (uc != win) && WinVisible(uc)) {
      ScanData *data;
      data = MyCalloc(1, sizeof(ScanData));
      data->x1 = uc->Attr.x; data->x2 = uc->Attr.x + uc->Attr.width;
      data->y1 = uc->Attr.y; data->y2 = uc->Attr.y + uc->Attr.height;
      NodeAppend(wins, data);
    }
  }
/*  XQueryTree(disp,TheScreen.root,&dummy,&dummy,&children,&number);

  if(children) {
    for(a=0;a<number;a++) {
      XWindowAttributes xwa;

      XGetWindowAttributes(disp,children[a],&xwa);
      if((children[a] != win) && (xwa.map_state == IsViewable)
         && (xwa.class == InputOutput)) {
        ScanData *data;
        data=MyCalloc(1,sizeof(ScanData));
        data->x1=xwa.x;data->x2=xwa.x+xwa.width;
        data->y1=xwa.y;data->y2=xwa.y+xwa.height;
        NodeAppend(wins,data);
      }
    }
    XFree((char *)children);
  }*/
  return(wins);
}

void FreeScanned(NodeList *wins)
{
  Node *dn;
  dn=NULL;
  while((dn=NodeNext(wins,dn))){
    free(dn->data);
  }
  NodeListDelete(&wins);
}

/** will calculate a position's overlapping value (# of covered pixels) **/
long Overlap(NodeList *wins,int x1,int y1,int x2,int y2)
{
  Node *dn;
  long o;

  o=0;

  dn=NULL;
  while((dn=NodeNext(wins,dn))){
    ScanData *test;
    int dx,dy;
    dx=dy=0;

    test=dn->data;
    if(BETWEEN(x1,test->x1,x2)){
      dx=MIN(x2,test->x2)-test->x1;
    } else if(BETWEEN(test->x1,x1,test->x2)) {
      dx=MIN(x2,test->x2)-x1;
    }
    if(BETWEEN(y1,test->y1,y2)){
      dy=MIN(y2,test->y2)-test->y1;
    } else if(BETWEEN(test->y1,y1,test->y2)) {
      dy=MIN(y2,test->y2)-y1;
    }
    o+=(dx*dy);
  }
  if(!((x1>=0)&&(x2<=TheScreen.width)&&(y1>=0)&&(y2<=TheScreen.height)))
    o=TheScreen.width*TheScreen.height;
  
  return(o);
}

/** will 'move' the window one pixel either in x- or y- direction **/
char lastmove; /* 1:x++;2:x--;3:y++;4:y--;0:none */
Bool GradientMove(NodeList *wins,int *x,int *y,int w,int h)
{
  Node *dn;
  long xo,yo;
  int x1,x2,y1,y2;

  xo=yo=0;
  x2=(x1=((*x)-1))+w+2;
  y2=(y1=((*y)-1))+h+2;

  dn=NULL;
  while((dn=NodeNext(wins,dn))){
    ScanData *test;
    int dx,dy;
    dx=dy=0;

    test=dn->data;
    if(BETWEEN(x1,test->x1,x2)){
      dx=MIN(x2,test->x2)-test->x1;
    } else if(BETWEEN(test->x1,x1,test->x2)) {
      dx=MIN(x2,test->x2)-x1;
    }
    if(BETWEEN(y1,test->y1,y2)){
      dy=MIN(y2,test->y2)-test->y1;
    } else if(BETWEEN(test->y1,y1,test->y2)) {
      dy=MIN(y2,test->y2)-y1;
    }

    if(dy!=h+2){
      if(y1>test->y1) yo+=(dx*dy);
      else yo-=(dx*dy);
    }
    if(dx!=w+2){
      if(x1>test->x1) xo+=(dx*dy);
      else xo-=(dx*dy);
    }
  }
  if(!BETWEEN(1,x1=(*x),TheScreen.width-w-2)) xo=0;
  if(!BETWEEN(1,y1=(*y),TheScreen.height-h-2)) yo=0;

  if(abs(xo)>=abs(yo)) {
    if((xo>0)&&(lastmove!=2)) {(*x)++;lastmove=1;}
    if((xo<0)&&(lastmove!=1)) {(*x)--;lastmove=2;}
  } else {
    if((yo>0)&&(lastmove!=4)) {(*y)++;lastmove=3;}
    if((yo<0)&&(lastmove!=3)) {(*y)--;lastmove=4;}
  }
  if((*x==x1)&&(*y==y1)){
    return(False);
  }
  return(True);
}

/** an 'intelligent' placement-algorithm **/
long GradientPlace(NodeList *wins,int w,int h,int *x,int *y)
{
  Node *xn,*yn;
  ScanData *xd,*yd;
  int xa,ya,a,xm,ym;
  long o,o1;

  xa=ya=a=0;
  xm=TheScreen.width-w;
  ym=TheScreen.height-h;
  while((a<GRADMAX)&&GradientMove(wins,&xa,&ya,w,h))a++;
  o=Overlap(wins,xa,ya,xa+w,ya+w);
  *x=xa;
  *y=ya;
  if(!o) return(o);

  xn=yn=NULL;
  while((xn=NodeNext(wins,xn))){
    xd=xn->data;
    while((yn=NodeNext(wins,yn))){
      yd=yn->data;
      if(BETWEEN(xd->y1,yd->y1,xd->y2) && BETWEEN(yd->x1,xd->x1,yd->x2)) {
        xa=MAX(0,xd->x1-w);
        ya=MAX(0,yd->y1-h);
        lastmove=a=0;
        while((a<GRADMAX)&&GradientMove(wins,&xa,&ya,w,h))a++;
        if(o>(o1=Overlap(wins,xa,ya,xa+w,ya+h))){
          o=o1;*x=xa;*y=ya;}
      }
      if(BETWEEN(xd->y1,yd->y2,xd->y2) && BETWEEN(yd->x1,xd->x1,yd->x2)) {
        xa=MAX(0,xd->x1-w);
        ya=MIN(ym,yd->y2);
        lastmove=a=0;
        while((a<GRADMAX)&&GradientMove(wins,&xa,&ya,w,h))a++;
        if(o>(o1=Overlap(wins,xa,ya,xa+w,ya+h))){
          o=o1;*x=xa;*y=ya;}
      }
      if(BETWEEN(xd->y1,yd->y1,xd->y2) && BETWEEN(yd->x1,xd->x2,yd->x2)) {
        xa=MIN(xm,xd->x2);
        ya=MAX(0,yd->y1-h);
        lastmove=a=0;
        while((a<GRADMAX)&&GradientMove(wins,&xa,&ya,w,h))a++;
        if(o>(o1=Overlap(wins,xa,ya,xa+w,ya+h))){
          o=o1;*x=xa;*y=ya;}
      }
      if(BETWEEN(xd->y1,yd->y2,xd->y2) && BETWEEN(yd->x1,xd->x2,yd->x2)) {
        xa=MIN(xm,xd->x2);
        ya=MIN(ym,yd->y2);
        lastmove=a=0;
        while((a<GRADMAX)&&GradientMove(wins,&xa,&ya,w,h))a++;
        if(o>(o1=Overlap(wins,xa,ya,xa+w,ya+h))){
          o=o1;*x=xa;*y=ya;}
      }

      if(!o) return(o);
    }
  }
  return(o);
}

void SnapWin(NodeList *wins, int *x, int *y, int width, int height)
{
  ScanData *test;
  Node *n=NULL;
  int origx, origy;
  
  origx = *x;
  origy = *y;

  if(BETWEEN(-((int)InitS.SnapDistance), origx,
             InitS.SnapDistance))
    *x = 0;
  if(BETWEEN(-((int)InitS.SnapDistance), ((int)TheScreen.width) - origx - width,
             InitS.SnapDistance))
    *x = TheScreen.width - width;

  if(BETWEEN(-((int)InitS.SnapDistance), origy,
             InitS.SnapDistance))
    *y = 0;
  if(BETWEEN(-((int)InitS.SnapDistance), ((int)TheScreen.height)-origy - height,
             InitS.SnapDistance))
    *y = TheScreen.height - height;

  while((n=NodeNext(wins,n))){
    test = n->data;
    if(((test->y1 - InitS.SnapDistance) < (*y + height)) 
       && ((test->y2 + InitS.SnapDistance) > *y)){
      if(BETWEEN(-((int)InitS.SnapDistance), test->x2 - origx,
                 InitS.SnapDistance)
         && ((abs(test->x2 - origx) <= abs(*x - origx)) || (*x == origx)))
        *x = test->x2;
      if(BETWEEN(-((int)InitS.SnapDistance), test->x1 - origx - width,
                 InitS.SnapDistance)
         && ((abs(test->x1 - width - origx) <= abs(*x - origx))
             || (*x == origx)))
        *x = test->x1 - width;
    }

    if((((test->x1) - InitS.SnapDistance) < (*x + width))
       && ((test->x2 + InitS.SnapDistance) > *x)){
      if(BETWEEN(-((int)InitS.SnapDistance), test->y2 - origy,
                 InitS.SnapDistance)
         && ((abs(test->y2 - origy) <= abs(*y - origy)) || (*y == origy)))
        *y = test->y2;
      if(BETWEEN(-((int)InitS.SnapDistance), test->y1 - origy - height,
                 InitS.SnapDistance)
         && ((abs(test->y1 - height - origy) <= abs(*y - origy))
             || (*y == origy)))
        *y = test->y1 - height;
    }
  }
}

void ManualPlace(NodeList *wins,int w,int h,int *x,int *y)
{
  int dummy,placing;
  Window dummyWin;
  UltimateContext *RealActive;

  MenuDontKeepItAnymore();
  GrabPointer(TheScreen.root,PointerMotionMask|ButtonReleaseMask|\
                            ButtonPressMask,TheScreen.Mice[C_NW]);
  XQueryPointer(disp,TheScreen.root,&dummyWin,&dummyWin,x,y,&dummy,\
                                                     &dummy,&dummy);
  GrabServer();
  RealActive = ActiveWin;
  ActivateWin(NULL);
  XSetInputFocus(disp, TheScreen.inputwin, RevertToPointerRoot,
                 TimeStamp);
  XSync(disp,False);
  StartRubber(*x,*y,w,h,TheScreen.BorderWidth1);
  placing=-1;
  while(placing){
    XEvent event;
    KeySym keysym;

    XMaskEvent(disp,PointerMotionMask|ButtonReleaseMask|ButtonPressMask
                    |KeyPressMask,&event);
    switch(event.type){
      int dummy;
      case MotionNotify:      *x=event.xmotion.x_root;
                              *y=event.xmotion.y_root;
                              if(InitS.SnapDistance) SnapWin(wins,x,y,w,h);
                              SqueezeRubber(*x,*y,w,h);
                              break;
      case KeyPress:          keysym=*XGetKeyboardMapping(disp,
                                                          event.xkey.keycode,
                                                          1, &dummy);
                              if(
#ifdef XK_Return
                                 (keysym == XK_Return) ||
#endif
#ifdef XK_KP_Enter
                                 (keysym == XK_KP_Enter) ||
#endif
#ifdef XK_Linefeed
                                 (keysym == XK_Linefeed) ||
#endif
                                 0) {
                                placing=0;
                                StampTime(event.xkey.time);
                              }
                              break;
      case ButtonRelease:     placing=0;
                              StampTime(event.xbutton.time);
                              break;
      default:                break;
    }
  }
  ActivateWin(RealActive);
  StopRubber(&dummy,&dummy,&dummy,&dummy);
  UngrabServer();
  UngrabPointer();
}



/** a brute-force algorithm that will find out a windows 'best' position **/
/*
long LeastOverlap(NodeList *wins,int w,int h,int *x,int *y)
{
  int xc,yc;
  long o;

  o=TheScreen.width*TheScreen.height;

  for(xc=0;xc<TheScreen.width-w;xc++){
    for(yc=0;yc<TheScreen.height-h;yc++){
      long o1;
      if((o1=Overlap(wins,xc,yc,xc+w,yc+h))<o) {
        o=o1;
        *x=xc;*y=yc;
        if(o==0) return(0);
      }
    }
  }
 
  return(o);
}*/

void PlaceWin(UltimateContext *uc)
{
  NodeList *wins;
  int x,y,width,height;


  if(InitS.PlacementStrategy && (uc->flags & PLACEIT)) {
    if(uc->frame!=None) {
      width=uc->Attr.width;
      height=uc->Attr.height;
    } else {
      width=uc->Attributes.width;
      height=uc->Attributes.height;
    }

    wins=ScanScreen(uc);

    switch(InitS.PlacementStrategy>>1){
      case 2:if(InitS.PlacementThreshold<GradientPlace(wins,width,height,&x,&y))
                ManualPlace(wins,width,height,&x,&y);
              break;
      case 3: ManualPlace(wins,width,height,&x,&y);
              break;
      default: GradientPlace(wins,width,height,&x,&y);
    }

    FreeScanned(wins);

    MoveResizeWin(uc,x,y,0,0);
  }
  uc->flags &= ~PLACEIT;
}
