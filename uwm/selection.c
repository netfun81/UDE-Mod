/***  SELECTION.C: Contains routines related to managing selections  ***/

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
#else
  #define CPP_CALL cpp
#endif

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>

#include "uwm.h"
#include "init.h"
#include "selection.h"

extern UDEScreen TheScreen;
extern Display *disp;
extern InitStruct InitS;

void SendSelectionNotify(XEvent *request, Atom property)
{
  XEvent response;

  response.type = SelectionNotify;
  response.xselection.requestor = request->xselectionrequest.requestor;
  response.xselection.selection = request->xselectionrequest.selection;
  response.xselection.target = request->xselectionrequest.target;
  response.xselection.property = property;
  response.xselection.time = request->xselectionrequest.time;
  XSendEvent(disp, request->xselectionrequest.requestor, False, 0L, &response);
}

/*** icccm requires us to know if a XChangeProperty succeeded or not.
     ChangeProperty will call XChangeProperty with the appropriate arguments
     returns 0 on success, -1 on error. ***/
int _ChangePropertyReturnValue;
int ChangePropertyErrorHandler(Display *d, XErrorEvent *e)
{
  _ChangePropertyReturnValue = -1;
  return(_ChangePropertyReturnValue);
}
int ChangeProperty(Window win, Atom property, Atom type, int format, int mode,
                   char *data, int nelements)
{
  int (*olderrorhandler)();
  _ChangePropertyReturnValue = 0;

  XSync(disp, False); /*** no X11 protocol routines between HERE ... ***/
  olderrorhandler = XSetErrorHandler(ChangePropertyErrorHandler);
  XChangeProperty(disp, win, property, type, format, mode, data, nelements);
  XSync(disp, False); 
  XSetErrorHandler(olderrorhandler); /*** ...and HERE (except the XChangeP) ***/
  return(_ChangePropertyReturnValue);
}

Atom Parse_WM_Sx_Selection(Atom target, Atom prop, Window requestor)
{
  if(target == TheScreen.MULTIPLE) {
    if(prop != None) {
      unsigned long n, ba, rl, i;
      Atom *list, at;
      int af;
      XGetWindowProperty(disp, requestor, prop, 0, 0, False,
			 TheScreen.ATOM_PAIR, &at, &af, &n, &ba,
			 (unsigned char**)&list);
      if(list) XFree(list);
      rl = ba/4;
      XGetWindowProperty(disp, requestor, prop, 0, rl, False,
			 TheScreen.ATOM_PAIR, &at, &af, &n, &ba,
			 (unsigned char **) &list);
      if((at == TheScreen.ATOM_PAIR) && (af == 32) && list){
        for(i = 0; i < n; i+=2) {
          list[i+1] = Parse_WM_Sx_Selection(list[i], list[i+1], requestor);
        }
	XChangeProperty(disp, requestor, prop, TheScreen.ATOM_PAIR,
			32, PropModeReplace, (unsigned char *)list, n);
      } else prop = None; /* deny unprocessable requests */
      if(list) XFree(list);
    }
  } else {
    if(prop == None) prop = target;
    if(target == TheScreen.VERSION_ATOM) {
      INT32 icccmVersion[2] = {ICCCM_MAJOR, ICCCM_MINOR};
      if(ChangeProperty(requestor, prop, XA_INTEGER, 32, PropModeReplace,
                        (char *)icccmVersion, 2)) return(None);
    } else if(target == TheScreen.TARGETS) {
#define NUMBER_OF_TARGETS 4
      Atom targets[NUMBER_OF_TARGETS] = {TheScreen.VERSION_ATOM,
                                         TheScreen.TARGETS,
                                         TheScreen.MULTIPLE,
                                         TheScreen.TIMESTAMP};
      if(ChangeProperty(requestor, prop, XA_ATOM, 32, PropModeReplace,
                        (char *)targets, NUMBER_OF_TARGETS)) return(None);
#undef NUMBER_OF_TARGETS
    } else if(target == TheScreen.TIMESTAMP) {
      if(ChangeProperty(requestor, prop, XA_INTEGER, 32, PropModeReplace,
                        (char *)&TheScreen.start_tstamp, 1)) return(None);
    }
  }
  return(prop);
}

