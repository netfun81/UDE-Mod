/***  APPLICATIONS.C: Routines for the application menu  ***/

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


#include "uwm.h"
#include "menu.h"
#include "special.h"

extern UDEScreen TheScreen;

MenuItem *lastsel;

void OtherSelect(XEvent *event,MenuItem *selected)
{
  if((lastsel=selected)) if((selected->type==I_SELECT)&&(selected->data))
    MySystem(((AppStruct *)selected->data)->command);
}


void ApplicationMenu(int x,int y)
{
  MenuItem *item;
  lastsel=NULL;
  item=StartMenu(TheScreen.AppsMenu,x,y,True,OtherSelect);
  if(item && (item!=lastsel)) if((item->type==I_SELECT)&&(item->data))
    MySystem(((AppStruct *)item->data)->command);
}
