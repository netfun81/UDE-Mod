/***  WORKSPACES.C: Routines for the workspace-manager  ***/

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


#include <sys/types.h>
#include <signal.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
  #include <unistd.h>
#endif

#include "uwm.h"
#include "windows.h"
#include "wingroups.h"
#include "workspaces.h"
#include "special.h"
#include "nodes.h"
#include "menu.h"
#include "urm.h"

NodeList Workspaces;
NodeList Layers;
NodeList WS_Layer_Defaults;

extern UDEScreen TheScreen;
extern Display *disp;
extern Menu *activemen;
extern HandlerTable *Handle;
extern HandlerTable MoveHandle[LASTEvent];
extern HandlerTable ResizeHandle[LASTEvent];


pid_t ScreenCommandPID=0;

/*  needs to be called after a workspace is added or removed or after
    application colors changed etc.  */
void BroadcastWorkSpacesInfo()
{
  int a,b;
  for(a=0;a<TheScreen.desktop.WorkSpaces;a++) {
    for(b=0;b<UDE_MAXCOLORS;b++) 
      XQueryColors(disp, TheScreen.colormap,
                   TheScreen.Colors[a], UDE_MAXCOLORS);
  }
  SetResourceDB();
}

void SetWSBackground()
{
  unsigned char back_changed= 0;
  
  if(TheScreen.BackPixmap[TheScreen.desktop.ActiveWorkSpace]!=None)
    {
      /* To set the root pixmap and properties pointing to it XGrabServer
	 must be called to make sure that we don't leak the pixmap if
	 somebody else is setting it at the same time. */
      /* use our private wraparound GrabServer instead! */
      GrabServer (disp);
      
      /* ++++++++++++++++  FIXME ++++++++++++++++++++++++++ 
	 Is necessary in this case to kill the old pixmap or will it
	 destroy the pixmaps in the TheScreen.BackPixmap array ???
	 This is how i suppouse the pixmap is removed : */
/* comment by arc: this seems too agressive to me, if the user starts a 
   program setting this property he probably knows what he is doing.
   we definitely shouldn't kill clients without user interaction! */
/*
      XGetWindowProperty (disp, TheScreen.root,
			  XIntermAtom ("ESETROOT_PMAP_ID", FALSE),
			  0L, 1L, False, XA_PIXMAP,
			  &type, &format, &nitems, &bytes_after,
			  &data_esetroot);
      
      if (type == XA_PIXMAP) {
	if (format == 32 && nitems == 4)
	  XKillClient(disp, *((Pixmap*)data_esetroot));
	XFree (data_esetroot);
      }
*/    
	 
      /* Some aplications, like transparent terminals, need this properties
	 to be set to get and update the background pixmap */
      XChangeProperty (disp, TheScreen.root,
		       XInternAtom (disp, "ESETROOT_PMAP_ID", 0), XA_PIXMAP,
		       32, PropModeReplace,
		       (unsigned char *) \
		       &(TheScreen.BackPixmap[TheScreen.desktop.ActiveWorkSpace]),
		       1);
      XChangeProperty (disp, TheScreen.root,
		       XInternAtom (disp, "_XROOTPMAP_ID", 0), XA_PIXMAP,
		       32, PropModeReplace,
		       (unsigned char *) \
		       &(TheScreen.BackPixmap[TheScreen.desktop.ActiveWorkSpace]),
		       1);
      XSetWindowBackgroundPixmap(disp,TheScreen.root,
				 TheScreen.BackPixmap\
				 [TheScreen.desktop.ActiveWorkSpace]);
      
      back_changed= 1;
      UngrabServer (disp);
      XFlush (disp);
    }
  else if (TheScreen.SetBackground [TheScreen.desktop.ActiveWorkSpace])
    {
      XSetWindowBackground(disp,TheScreen.root,
			   TheScreen.Background\
                           [TheScreen.desktop.ActiveWorkSpace]);
      back_changed= 1;
    }
  if (back_changed)
    XClearWindow(disp,TheScreen.root);

  if(ScreenCommandPID>0){              /* terminate previous ScreenCommand */
    kill(ScreenCommandPID,SIGTERM);
    ScreenCommandPID=0;
  }
  if(TheScreen.BackCommand[TheScreen.desktop.ActiveWorkSpace])
    ScreenCommandPID=MySystem(TheScreen.BackCommand
                              [TheScreen.desktop.ActiveWorkSpace]);
  else ScreenCommandPID=0;
}

void ChangeWS(short WS)
{
  Node *n;
  short oldws;

  if(OnActiveWS(WS) || (WS >= TheScreen.desktop.WorkSpaces)
     || (Handle == MoveHandle) || (Handle == ResizeHandle)) return;

  oldws=TheScreen.desktop.ActiveWorkSpace;
  TheScreen.desktop.ActiveWorkSpace=WS;

  UpdateDesktop();
  SetWSBackground();

  n=NULL;
  while((n= NodeNext(TheScreen.UltimateList, n))) {
    UltimateContext *uc;

    uc = n->data;
    if(IsNormal(uc)) {
      if(uc->WorkSpace == oldws) {
	UnmapWin(uc);
      } else if(uc->WorkSpace==WS) {
        MapWin(uc,True);
      } else if(uc->WorkSpace==-1) {
        DrawWinBorder(uc);
      }
    }
  }

  if(activemen) {
    Menu2ws(RootMenu(activemen),WS);
    RedrawMenuTree();
  }
}

void StickyWin(UltimateContext *uc)
{
  if(uc->WorkSpace==-1) Win2WS(uc, TheScreen.desktop.ActiveWorkSpace);
  else Win2WS(uc, -1);
  if(uc->group) {
    Node *n = NULL;
    while((n = NodeNext(uc->group->members, n))) { 
      Win2WS(n->data, uc->WorkSpace);
    }
    uc->group->WorkSpace = uc->WorkSpace;
  }
}

void WithWin2WS(UltimateContext *uc,short ws)
{
  if(uc->group) {
    Node *n = NULL;
    while((n = NodeNext(uc->group->members, n))) { 
      ((UltimateContext *)(n->data))->WorkSpace = ws;
    }
    uc->group->WorkSpace = uc->WorkSpace;
  } else uc->WorkSpace = ws;
  ChangeWS(uc->WorkSpace);
}

void Win2WS(UltimateContext *uc, short ws)
{
  if(uc->WorkSpace == ws) return;
  if(OnActiveWS(uc->WorkSpace) && (!OnActiveWS(ws))) UnmapWin(uc);
  uc->WorkSpace = ws;
  if(IsNormal(uc)) MapWin(uc, True);
}
