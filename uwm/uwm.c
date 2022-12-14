/***  UWM.C:  Main file of UWM  ***/

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

#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xlocale.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#else
#include <string.h>
#endif

#include "i18n.h"
#include "uwm.h"
#include "init.h"
#include "windows.h"
#include "handlers.h"
#include "menu.h"
#include "special.h"
#include "properties.h"

Display *disp=NULL; 	/* 'global' display to Server... */ 
UDEScreen TheScreen;    /* structure that keeps all important global info */
XContext UWMContext;    /* WM-Concidering information connected to a window */
XContext UWMGroupContext;    /* information connected to a window group */
Atom WM_STATE_PROPERTY;
Atom WM_CHANGE_STATE;
Atom WM_TAKE_FOCUS;
Atom WM_DELETE_WINDOW;
Atom WM_PROTOCOLS;
Atom MOTIF_WM_HINTS;
InitStruct InitS;

extern HandlerTable *Handle;
extern int ShapeEvent;

/*** Exit-Procedures ***/

extern pid_t ScreenCommandPID;
void CleanUp(Bool StopScript)
{
  Node *n;
  struct stat stats;
  char rmstr[256],*p;

  if(ScreenCommandPID>0) if(!kill(ScreenCommandPID,SIGTERM)) sleep(1);
                     /* terminate running ScreenCommand and not leave a zombie*/
  if(TheScreen.UltimateList) if((n=TheScreen.UltimateList->first))
    while(n) {
      XMapWindow(disp, ((UltimateContext *)(n->data))->win);
      SetSeemsMapState(n->data, ((UltimateContext *)(n->data))->uwmstate);
      n = PlainDeUltimizeWin(n->data, True);
    }
  if(TheScreen.AppsMenu)     DestroyMenu(TheScreen.AppsMenu);
  if(TheScreen.UWMMenu)      DestroyMenu(TheScreen.UWMMenu);

  if(disp)                   XCloseDisplay(disp);
  signal(SIGCHLD,SIG_DFL);

  if((InitS.StopScript[0]!='\n')&&(InitS.StopScript[0]!='\0')&&StopScript) {
    sprintf(rmstr,"%s/.ude/config/%s",TheScreen.Home,InitS.StopScript);
    if((p=strchr(rmstr,' '))) (*p)='\0';
    if(stat(rmstr,&stats))
      if((errno==ENOENT)||(errno==EACCES)) {
        sprintf(rmstr,"%sconfig/%s",TheScreen.udedir,InitS.StopScript);
        if((p=strchr(rmstr,' '))) (*p)='\0';
        if(stat(rmstr,&stats))
          if((errno==ENOENT)||(errno==EACCES)) {
            sprintf(rmstr,"%s",InitS.StopScript);
            p=NULL;
          }
      }
    if(p) *p=' ';
    system(rmstr);                 /* execute Stop-Script and wait for it */
  }
}

void SeeYa(int ecode, char *vocalz)
{
  CleanUp(True);

  if(vocalz) fprintf(TheScreen.errout,"\nUWM: %s - terminating (sorry)\n\n",vocalz);
  else if(ecode)
    fprintf(TheScreen.errout,"\nUWM: Exiting, I guess there is some reason for it\n\n");
  else fprintf(TheScreen.errout,"\nUWM: Exiting due to user request. (I guess...)\n\n");
  exit(ecode);
}

void CatchWindows()
{
  Window dummy,*children;
  unsigned int number,a,b;
  UltimateContext *uc;
  XWMHints *hints;

  XQueryTree(disp,TheScreen.root,&dummy,&dummy,&children,&number);
  for(a=0;a<number;a++) if(children[a]) {
    if((hints=XGetWMHints(disp,children[a]))) {
      if(hints->flags & IconWindowHint) {
        for(b=0;b<number;b++) 
          if(children[b]==hints->icon_window) {
            children[b]=None;
            break;
          }
      }
      XFree((char *) hints);
    }
  }

  for(a=0;a<number;a++){
    if(children[a]) {
      int format;
      unsigned long number,bytesafter;
      Atom type;
      long *data;
      if(Success!=XGetWindowProperty(disp, children[a], WM_STATE_PROPERTY, 0, 2,
                                     False, WM_STATE_PROPERTY, &type,
				     &format, &number, &bytesafter,
				     (unsigned char **)&data)) data = NULL;
      if((uc = UltimizeWin(children[a]))) {
        if(!uc->Attributes.override_redirect) {
	  CARD32 state = WithdrawnState;
	
          if((uc->WMHints)&&(uc->WMHints->flags & StateHint)&&
             (uc->WMHints->initial_state==IconicState)){
	    state = IconicState;
          } else if((uc->WMHints)&&(uc->WMHints->flags & StateHint)&&
                    (uc->WMHints->initial_state==WithdrawnState)){
	    state = WithdrawnState;
          } else if(uc->Attributes.map_state!=IsUnmapped) {
	    state = NormalState;
          }
          if(data) {
	    if((number == 2) && (type == WM_STATE_PROPERTY)
	       && (format == 32))  state = data[0];
	    XFree(data);
          }
	  switch(state) {
	    case IconicState:
               IconifyWin(uc);
	       break;
	    case WithdrawnState:
	       SetIsMapState(uc, WithdrawnState);
	       break;
	    case NormalState:
               DisplayWin(uc);
	       break;
	  }
        }
      }
    }
  }
  if(number>0) XFree((char *)children);
}

void ManagEm()
{
  while(1)
  {
    static XEvent event;

    XNextEvent(disp,&event);
    if(event.type<LASTEvent){  /* Standard X11-events */
      if(Handle[event.type]) (*Handle[event.type])(&event);
    } else {                   /* Extensions */
      if(event.type==ShapeEvent) HandleShape(&event);
    }
  }
}

void ShellQuit(int dummy)
{
  while (waitpid(-1, NULL, WNOHANG) > 0);
  signal(SIGCHLD,ShellQuit);
}

void TermSig(int dummy)
{
  SeeYa(0,"Term-Signal received!");
}

#define HELPARRAYLINES 8
char *HelpArray[HELPARRAYLINES]=
 {"UWM HELP","Options & Switches:",
  "  --NoStartScript     prevents uwm from executing StartScript",
  "  --NoStopScript      prevents uwm from executing StopScript",
  "  --TryHard           try to replace another running icccm compliant wm",
  "  --Hostile           try harder replacing another icccm \'compliant\' wm",
  "  --StayAlive         don't give away wmment control voluntarily",""};

void PromptCommandLine(int argc,char **argv)
{
  int a,b;

  InitS.StartScript[0]='\0';
  InitS.StopScript[0]='\0';
  InitS.icccmFlags = 0;

  printf("\n");

  for(a=1;a<argc;a++){
    if(!strcmp("--help",argv[a])) {
      for(b=0;b<HELPARRAYLINES;b++) printf("%s\n",HelpArray[b]);
      exit(0);
    }
    else if(!strcmp("--NoStartScript",argv[a])) InitS.StartScript[0]='\n';
    else if(!strcmp("--NoStopScript",argv[a])) InitS.StopScript[0]='\n';
    else if(!strcmp("--TryHard",argv[a])) InitS.icccmFlags |= ICF_TRY_HARD;
    else if(!strcmp("--Hostile",argv[a])) InitS.icccmFlags |= ICF_HOSTILE;
    else if(!strcmp("--StayAlive",argv[a])) InitS.icccmFlags |= ICF_STAY_ALIVE;
    else {
      printf("Unknown Option: %s\nType %s --help for more info\n",argv[a],\
                                                                  argv[0]);
      exit(0);
    }
  }
}

/*** MAIN ***/

int main(int argc,char **argv)
{
  struct stat stats;
  char dispstr[128], hdispstr[128], rmstr[256], *p;
  /*** too lazy to alloc Mem... ***/

#ifdef ENABLE_NLS
  /* init i18n */
  /*setlocale (LC_ALL, "");  don't need everything, LC_NUMERIC will confuse
                             initialisation (dec points in BevelFactor)*/
#ifdef LC_COLLATE
  setlocale(LC_COLLATE, "");
#endif
#ifdef LC_CTYPE
  setlocale(LC_CTYPE, "");
#endif
#ifdef LC_MONETARY
  setlocale(LC_MONETARY, "");
#endif
#ifdef LC_TIME
  setlocale(LC_TIME, "");
#endif
#ifdef LC_MESSAGES
  setlocale(LC_MESSAGES,"");
#endif
#ifdef LC_RESPONSES
  setlocale(LC_RESPONSES,"");
#endif
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
#endif /* ENABLE_NLS */

#ifdef DEVEL
  TheScreen.errout=fopen("/dev/tty10","w");
  fprintf(TheScreen.errout,"UWM: Using this display for Error output!\n");
  fflush(TheScreen.errout);
#else
  TheScreen.errout=stderr;
#endif

  printf("\n\n     UDE - the Unix Desktop Environment\n\n");
  printf("Version %s\n",UWMVERSION);

  PromptCommandLine(argc,argv);

  printf("\nAttempting to start UWM - the Ultimate Window Manager...\n");

  CheckCPP();
            /*** from now on MyOpen can be called */

  /* ugly thing, but can't really use sigaction due to differences between *
   * sigaction structure on posix-std.-signal systems and bsd-like ones... */
  signal(SIGCHLD,ShellQuit); /* kick out zombies */

  signal(SIGTERM,TermSig);

  InitUWM();

#ifdef HAVE_PUTENV
  sprintf(dispstr,"DISPLAY=%s",XDisplayString(disp));/* Give 'em some info... */
  putenv(dispstr);
#ifdef HAVE_UNAME
  {
    struct utsname hname;
    uname(&hname);
    sprintf(hdispstr,"HOSTDISPLAY=%s%s",hname.nodename,strchr(dispstr,':'));
    putenv(hdispstr);
  }
#endif /* HAVE_UNAME */
#endif /* HAVE_PUTENV */

  CatchWindows();

  printf("    ...made it!\n");

  if((InitS.StartScript[0]!='\n')&&(InitS.StartScript[0]!='\0')) {
    sprintf(rmstr,"%s/.ude/config/%s",TheScreen.Home,InitS.StartScript);
    if((p=strchr(rmstr,' '))) (*p)='\0';
    if(stat(rmstr,&stats))
      if((errno==ENOENT)||(errno==EACCES)) {
        sprintf(rmstr,"%sconfig/%s",TheScreen.udedir,InitS.StartScript);
        if((p=strchr(rmstr,' '))) (*p)='\0';
        if(stat(rmstr,&stats))
          if((errno==ENOENT)||(errno==EACCES)) {
            sprintf(rmstr,"%s",InitS.StartScript);
            p=NULL;
          }
      }
    if(p) *p=' ';
    MySystem(rmstr);                 /* execute Startup-Script  */
  }

  ManagEm();                     /* what a window manager is supposed to do */
  SeeYa(0,NULL);

  return(0);
}
