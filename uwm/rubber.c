/***  RUBBER.C: Contains routines for displaying "rubberwindows" that
                               allow size and position previews  ***/

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

#include "uwm.h"
#include "special.h"

extern UDEScreen TheScreen;
extern Display *disp;

unsigned int x,width,y,height,lo,ro;

void StartRubber(lx,ly,lwidth,lheight,BorderWidth)
unsigned int lx,ly,lwidth,lheight,BorderWidth;
{
  lo=BorderWidth/2;
  ro=BorderWidth;
  x=lx;width=lwidth;y=ly;height=lheight;

  GrabServer();

  XSetLineAttributes(disp,TheScreen.rubbercontext,0,\
                        LineSolid,CapButt,JoinMiter);
  XDrawLine(disp,TheScreen.root,TheScreen.rubbercontext,x+width-ro,y+ro,\
                                                       x+ro,y+height-ro);
  XDrawLine(disp,TheScreen.root,TheScreen.rubbercontext,x+ro,y+ro,\
                                           x+width-ro,y+height-ro);
  XSetLineAttributes(disp,TheScreen.rubbercontext,ro,\
                         LineSolid,CapButt,JoinMiter);
  XDrawRectangle(disp,TheScreen.root,TheScreen.rubbercontext,\
                                x+lo,y+lo,width-ro,height-ro);
}

void SqueezeRubber(lx,ly,lwidth,lheight)
unsigned int lx,ly,lwidth,lheight;
{
  if((lx==x)&&(ly==y)&&(lwidth==width)&&(lheight==height)) return;
  XDrawRectangle(disp,TheScreen.root,TheScreen.rubbercontext,\
                                x+lo,y+lo,width-ro,height-ro);
  XSetLineAttributes(disp,TheScreen.rubbercontext,0,\
                        LineSolid,CapButt,JoinMiter);
  XDrawLine(disp,TheScreen.root,TheScreen.rubbercontext,x+width-ro,y+ro,\
                                                       x+ro,y+height-ro);
  XDrawLine(disp,TheScreen.root,TheScreen.rubbercontext,x+ro,y+ro,\
                                           x+width-ro,y+height-ro);
  XFlush(disp);
  x=lx;y=ly;
  if(lheight|lwidth) {
    height=lheight;
    width=lwidth;
  }
  XDrawLine(disp,TheScreen.root,TheScreen.rubbercontext,x+width-ro,y+ro,\
                                                       x+ro,y+height-ro);
  XDrawLine(disp,TheScreen.root,TheScreen.rubbercontext,x+ro,y+ro,\
                                           x+width-ro,y+height-ro);
  XSetLineAttributes(disp,TheScreen.rubbercontext,ro,\
                         LineSolid,CapButt,JoinMiter);
  XDrawRectangle(disp,TheScreen.root,TheScreen.rubbercontext,\
                                x+lo,y+lo,width-ro,height-ro);
}

void StopRubber(lx,ly,lwidth,lheight)
int *lx,*ly,*lwidth,*lheight;
{
  XDrawRectangle(disp,TheScreen.root,TheScreen.rubbercontext,\
                                x+lo,y+lo,width-ro,height-ro);
  XSetLineAttributes(disp,TheScreen.rubbercontext,0,\
                        LineSolid,CapButt,JoinMiter);
  XDrawLine(disp,TheScreen.root,TheScreen.rubbercontext,x+width-ro,y+ro,\
                                                       x+ro,y+height-ro);
  XDrawLine(disp,TheScreen.root,TheScreen.rubbercontext,x+ro,y+ro,\
                                           x+width-ro,y+height-ro);

  UngrabServer();

  *lx=x;*lwidth=width;*ly=y;*lheight=height;
}
