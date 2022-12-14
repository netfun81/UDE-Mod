/***  SPECIAL.C: Contains miscallaneous routines  ***/

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
  #define CPP_CALL "cpp"
#endif

#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
  #include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef HAVE_TIME_H
  #include <time.h>
#endif


#include "uwm.h"
#include "special.h"
#include "init.h"
#include "uwmmenu.h"
#include "windows.h"
#include "winmenu.h"
#include "applications.h"
#include "move.h"
#include "resize.h"
#include "urm.h"

extern UDEScreen TheScreen;
extern Display *disp;
extern InitStruct InitS;
extern Atom WM_PROTOCOLS;

#define PGRABLEVEL 32    /* usually shouldn't go deeper than 2 or 3. */
unsigned int pgrabmasks[PGRABLEVEL];
Cursor pgrabmice[PGRABLEVEL];
Window pgrabwins[PGRABLEVEL];
int grabstat=0,pgrabstat=-1;

void RaiseWin(UltimateContext *uc)
{
  Node2End(TheScreen.UltimateList,InNodeList(TheScreen.UltimateList,uc));
  if(uc->frame) XRaiseWindow(disp,uc->frame);
  else XRaiseWindow(disp,uc->win);
}

void LowerWin(UltimateContext *uc)
{
  Node2Start(TheScreen.UltimateList,InNodeList(TheScreen.UltimateList,uc));
  if(uc->frame) XLowerWindow(disp,uc->frame);
  else XLowerWindow(disp,uc->win);
}

void SendWMProtocols(UltimateContext *uc, Atom prot)
{
  XEvent event;

  memset(&event,0,sizeof(XEvent));
  event.xclient.type = ClientMessage;
  event.xclient.window = uc->win;
  event.xclient.message_type = WM_PROTOCOLS;
  event.xclient.format = 32;
  event.xclient.data.l[0] = prot;
  event.xclient.data.l[1] = TimeStamp;
  if(!XSendEvent(disp,uc->win,False,0L,&event))
    fprintf(TheScreen.errout,"UWM: Couldn't send message to client.\n");
}


/* ShowMenu, added by Tim Coleman December 12 1998.
 * This is to allow the user to configure which menu is brought up
 * by which button. 
 * This is called from HandleButtonPress()
 */

void ShowMenu(int menuNumber, int x, int y) 
{
  switch (InitS.menuType[menuNumber]) {
    case 'U':
          WMMenu(x,y);
          break;
    case 'D':
          DeiconifyMenu(x,y);
          break;
    case 'A':
          ApplicationMenu(x,y);
          break;
  }
}

void BorderButton(int a,UltimateContext *uc,int x,int y,int x_root,int y_root)
{
  switch (InitS.BorderButtons[a]) {
    case 'M':
             RaiseWin(uc);
             StartWinMenu(uc,x_root,y_root);
          break;
    case 'D':
             StartDragging(uc,x,y);
          break;
    case 'R':
             StartResizing(uc,x_root,y_root);
          break;
  }
}

/*** will send a Configure-event to owner of uc-window ***/
void SendConfigureEvent(UltimateContext *uc)
{
  XEvent ToClient;                  
  ToClient.type=ConfigureNotify;
  ToClient.xconfigure.display=disp;
  ToClient.xconfigure.event=uc->win;
  ToClient.xconfigure.window=uc->win;
  ToClient.xconfigure.x=uc->Attr.x+uc->BorderWidth-uc->Attributes.border_width;
  ToClient.xconfigure.y=uc->Attr.y+uc->BorderWidth+TheScreen.TitleHeight-\
                                              uc->Attributes.border_width;
  ToClient.xconfigure.width=uc->Attributes.width;
  ToClient.xconfigure.height=uc->Attributes.height;
  ToClient.xconfigure.border_width=uc->Attributes.border_width;
  ToClient.xconfigure.above=uc->frame;
  ToClient.xconfigure.override_redirect=False;
  XSendEvent(disp,uc->win,False,StructureNotifyMask,&ToClient);
}

/* needs to be called after each writing access to TheScreen.desktop */
void UpdateDesktop()
{
  XChangeProperty(disp, TheScreen.root, TheScreen.UDE_SETTINGS_PROPERTY,
                  TheScreen.UDE_SETTINGS_PROPERTY, 8, PropModeReplace,
                  (unsigned char *) &TheScreen.desktop, sizeof(UDEDesktop));
  SetResourceDB();
}

/*** system() call that forks but continues with the program instead of waiting
     for the process to terminate (zombie elimination is done by the sigchld-
     routine) */
pid_t MySystem(char *comm)
{
  pid_t r;
  if(!(r=fork())){
    execl("/bin/sh","/bin/sh","-c",comm,NULL);
    fprintf(TheScreen.errout,"UWM: couldn't start application: %s\n",comm);
    exit(0);
  }
  return(r);
}

/* MyCalloc: allocates memory and quits if allocation fails. */
/* ATTENTION: Unlike calloc() MyCalloc() does not clear alloc'ed mem!!! */
void *MyCalloc(size_t n,size_t s)
{
  void *ret;
  if(!(ret=malloc(n*s))) SeeYa(1,"Fatal: couldn't allocate menory");
  return(ret);
}

/* MyStrdup replaces strdup also for those systems where it exists */
/* and quits on error */
char *MyStrdup(char *s)
{
  char *c;
  c = MyCalloc(strlen(s) + 1, sizeof(char));
  strcpy(c, s);
  return(c);
}

/*** MyOpen will open a file like fopen(...,"r") with regard to the ude  ***
 *** configuration search paths. the file will be passed through the c   ***
 *** preprocessor if available.                                          ***
 *** TO CHECK FOR A PREPROCESSOR CheckCPP MUST BE RUN BEFORE THE FIRST   ***
 *** CALL OF MyOpen!!!                                                   ***/
FILE *MyOpen(char *name, char *ppopts)
{
  int files[2];
  pid_t pid;
  FILE *r;
  char *temp = NULL;
  struct stat stats;
#define str3(T, A, B, C) if(T) free(T); T = MyCalloc(1 + strlen(A) + strlen(B) + strlen(C), sizeof(char)); sprintf(T, "%s%s%s", A, B, C)
  str3(temp, TheScreen.Home, "/.ude/config/", name);

  if(stat(temp,&stats)){
    str3(temp, TheScreen.udedir, "config/", name);
    if(stat(temp,&stats)) {
      str3(temp, name, "", "");
      if(stat(temp,&stats)){
        fprintf(TheScreen.errout, "UWM: file not found: %s.\n", name);
        return(NULL);
      }
    }
  }
#undef str3

  if(!TheScreen.cppcall) return(fopen(temp, "r"));

  pipe(files);

  if(!(pid = fork())) {     /* Child Process */
    char *temp2;
    close(files[0]);
    fclose(stdout);
    if(STDOUT_FILENO != fcntl(files[1], F_DUPFD, STDOUT_FILENO)) exit(-1);
    close(files[1]);
    if(-1==fcntl(STDOUT_FILENO,F_SETFD,0)) exit(-1);

    if(!(temp2 = malloc(sizeof(char) * (strlen(TheScreen.cppcall) 
                                        + strlen(ppopts) + strlen(temp) + 10))))
      exit(-1);
    sprintf(temp2,"%s %s - < %s", TheScreen.cppcall, ppopts, temp);

    execl("/bin/sh","/bin/sh","-c",temp2,NULL);
    fprintf(TheScreen.errout,"UWM: couldn't start /bin/sh\n");
    exit(-1);
  }
  free(temp);

  close(files[1]);
  if(!(r=fdopen(files[0], "r"))) {
    close(files[0]);
    return(NULL);
  }

  if(pid < 0) {
    fclose(r);
    return(NULL);
  }
  return(r);
}

/*** CheckCPP will check if a preprocessor is available on the system      ***
 *** this function will initialize MyOpen and must be run before the first ***
 *** call of MyOpen()!!!                                                   ***/
int CheckCPP()
{
  int files[2], ofiles[2], ret, ok;
  pid_t pid;
  FILE *r, *w;
  char *cppenv;
  char buffer[512];

  if((cppenv = getenv("CPP"))) TheScreen.cppcall = cppenv;
  else TheScreen.cppcall = CPP_CALL;

  recheck:  /* ugly but undangerous in this case. */

  if(pipe(files)) return(0);
  if(pipe(ofiles)) return(0);

  if(!(pid=fork())) {     /* Child Process */
    char *temp;

    temp = MyCalloc(strlen(TheScreen.cppcall) + 55, sizeof(char));
    sprintf(temp,
	    "%s -Dtest=toast - || while read t ; do echo ERROR ; done",
	    TheScreen.cppcall);

    close(files[0]);
    close(ofiles[1]);

    fclose(stdout);
    if(STDOUT_FILENO != fcntl(files[1], F_DUPFD, STDOUT_FILENO)) exit(-1);
    close(files[1]);
    if(-1==fcntl(STDOUT_FILENO, F_SETFD, 0)) exit(-1);

    fclose(stdin);
    if(STDIN_FILENO != fcntl(ofiles[0], F_DUPFD, STDIN_FILENO)) exit(-1);
    close(ofiles[0]);
    if(-1 == fcntl(STDIN_FILENO, F_SETFD, 0)) exit(-1);

    execl("/bin/sh", "/bin/sh", "-c", temp, NULL);
    fprintf(TheScreen.errout,"UWM: couldn't start application: /bin/sh\n");
    {
      char buf[16];
      while(read(STDIN_FILENO, buf, 16)) ;
    }
    exit(-1);
  }

  close(files[1]);
  close(ofiles[0]);

  if(pid < 0) {
    close(files[0]);
    close(ofiles[1]);
    return(0);
  }

  if(!(r=fdopen(files[0],"r"))) {
    close(files[0]);
    return(0);
  }
  if(!(w=fdopen(ofiles[1],"w"))) {
    close(ofiles[1]);
    return(0);
  }
  fprintf(w,"preprocessor test\n");
  fclose(w);
  ok=-1;
  while(fgets(buffer, 512, r)) if(strstr(buffer, "preprocessor toast")) ok = 0;
  waitpid(pid, &ret, 0);
  fclose(r);

  if(ret || ok) {
    if(cppenv) {
      cppenv = NULL;
      TheScreen.cppcall = CPP_CALL;
      goto recheck;
    }
    fprintf(TheScreen.errout,
            "UWM: looks like you don't have a working C preprocessor.\n");
    TheScreen.cppcall = NULL;
    return(0);
  }
  return(-1);
}

void GrabPointer(Window win,unsigned int mask,Cursor mouse)
{
  int wt = 0;
  pgrabstat++;
  if(!(pgrabstat<PGRABLEVEL))
    SeeYa(1,"UWM: probably got stuck in some loop (pointer grab)");
  pgrabmasks[pgrabstat]=mask;
  pgrabmice[pgrabstat]=mouse;
  pgrabwins[pgrabstat]=win;
  while(XGrabPointer(disp, pgrabwins[pgrabstat], True, pgrabmasks[pgrabstat],\
                     GrabModeAsync, GrabModeAsync, TheScreen.root,
		     pgrabmice[pgrabstat], CurrentTime) != GrabSuccess) {
#ifdef HAVE_NANOSLEEP
    struct timespec req,dummy;
    req.tv_sec = 0;
    req.tv_nsec = 20000;
    nanosleep(&req,&dummy);
#else
    sleep(1);
#endif
    wt = -1;
  }
  if(wt){
    unsigned int mask;
    int dint;
    Window dwin;
    XEvent discard;
    XQueryPointer(disp, TheScreen.root, &dwin, &dwin, &dint, &dint, &dint,
                  &dint, &mask);
    while(mask & (Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask)){
#ifdef HAVE_NANOSLEEP
      struct timespec req,dummy;
      req.tv_sec = 0;
      req.tv_nsec = 20000;
      nanosleep(&req,&dummy);
#else
      sleep(1);
#endif
      XQueryPointer(disp, TheScreen.root, &dwin, &dwin, &dint, &dint, &dint,
                    &dint, &mask);
    }
    while(True == XCheckMaskEvent(disp, ButtonPressMask | ButtonReleaseMask, 
			          &discard));
  }
}

void UngrabPointer()
{
  pgrabstat--;
  if(pgrabstat<0) XUngrabPointer(disp,CurrentTime);
  else {
    XGrabPointer(disp,pgrabwins[pgrabstat],True,pgrabmasks[pgrabstat],\
                GrabModeAsync,GrabModeAsync,None,pgrabmice[pgrabstat],\
                                                          CurrentTime);
  }
}

void GrabServer()
{
  if(!grabstat) XGrabServer(disp);
  grabstat++;
}

void UngrabServer()
{
  grabstat--;
  if(!grabstat) XUngrabServer(disp);
}
