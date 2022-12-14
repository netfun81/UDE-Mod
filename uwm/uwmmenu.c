/* UWMMENU.C: provides routines for the window-manager menu */

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
#include <string.h>
#ifdef HAVE_UNISTD_H
  #include <unistd.h>
#endif

#include "uwm.h"
#include "init.h"
#include "workspaces.h"
#include "menu.h"
#include "i18n.h"
#include "special.h"

extern UDEScreen TheScreen;
extern InitStruct InitS;

void QuitProc(MenuItem *item)
{
  SeeYa(0,NULL);
}

void RestartUWM(Bool ClUp, Bool StartStopScript)
{
  char name[256];
  char *argv[5];
  int a = 0;
  
  argv[a] = "uwm"; a++;
  if(False == StartStopScript) {
    argv[a] = "--NoStartScript"; a++; }
  if(InitS.StopScript[0] == '\n') {
    argv[a] = "--NoStopScript"; a++; }
  if(InitS.icccmFlags & ICF_STAY_ALIVE) {
    argv[a] = "--StayAlive"; a++; }
  argv[a] = NULL;

  if(True == ClUp) {
    printf("Restarting uwm: * cleaning up\n");
    CleanUp(StartStopScript);
  }

  printf("                * restarting\n");
  sprintf(name,"%suwm",TheScreen.udedir);
  execv(name,argv);
  sprintf(name,"%s/uwm",UDE_BIN_PATH);
  execv(name,argv);
  execvp("uwm",argv);

  fprintf(TheScreen.errout, "Error restarting uwm, terminating\n");
  exit(1);
}

void RestartProc1(MenuItem *item)
{
  RestartUWM(True, True);
}

void RestartProc2(MenuItem *item)
{
  RestartUWM(True, False);
}

void RestartProc3(MenuItem *item)
{
  char name[256];

  printf("closing down uwm: * cleaning up\n");
  strncpy(name, item->name, 255); name[255] = '\0';
  CleanUp(True);
  printf("                  * starting %s\n",name);
  execlp(name,name,NULL);
  fprintf(TheScreen.errout,"UWM: couldn't start %s, restarting uwm.\n",name);
  RestartUWM(False, True);
}

void ReloadMenu(MenuItem* item)
{
  DestroyMenu(TheScreen.AppsMenu);
  FILE *uwmrc;
  if(!(uwmrc=MyOpen("uwmrc",TheScreen.cppincpaths))) {
    fprintf(TheScreen.errout,"UWM: no config file found, using defaults.\n");
    CreateAppsMenu("appmenu");
    return;
  } 
  fclose(uwmrc);
  CreateAppsMenu(InitS.MenuFileName);
}

void ZapWS(XEvent *event,MenuItem *selected)
{
  if(selected) return; /* only react if no item is selected */
  switch(event->xbutton.button){
    case Button1:
         ChangeWS((TheScreen.desktop.ActiveWorkSpace 
                  + TheScreen.desktop.WorkSpaces - 1)
                  % TheScreen.desktop.WorkSpaces);
         break;
    case Button2:
         if(InitS.menuType[0]=='U') { 
           ChangeWS((TheScreen.desktop.ActiveWorkSpace
                    + TheScreen.desktop.WorkSpaces - 1)
                    % TheScreen.desktop.WorkSpaces);
           break;
         }
    case Button3:
         ChangeWS((TheScreen.desktop.ActiveWorkSpace+1)
                  % TheScreen.desktop.WorkSpaces);
         break;
  }
}

void CreateUWMMenu()
{
  Menu *really,*others;
  int j;
  short useTitle = (TheScreen.desktop.flags & UDESubMenuTitles);

  really= MenuCreate (_("Really?!"));
  if(!really)
    SeeYa(1,"FATAL: out of memory!");
  TheScreen.UWMMenu= MenuCreate (_("UWM Menu"));
  if(!TheScreen.UWMMenu)
    SeeYa(1,"FATAL: out of memory!");

  /* quit ude */ 
  AppendMenuItem (really, _("No!"), NULL, I_SELECT);
  AppendMenuItem (really, _("Yes!"), QuitProc, I_SELECT);

  AppendMenuItem (TheScreen.UWMMenu, _("Quit UDE"), really, I_SUBMENU);

  /* restart */
  really= MenuCreate(useTitle ? _("Restart UDE") : NULL);
  if(!really)
    SeeYa(1,"FATAL: out of memory!");
  AppendMenuItem(really, _("Reexecute StartScript"), RestartProc1, I_SELECT);
  AppendMenuItem(really, _("No execute StartScript"),
                   RestartProc2, I_SELECT);

  AppendMenuItem (TheScreen.UWMMenu, _("Restart UDE"), really, I_SUBMENU);

  /* submenu restart "Other WM" */
  if(InitS.OtherWmCount)
    {
      others= MenuCreate(useTitle ? _("Launch another WM") : NULL);
      if(!others)
        SeeYa(1,"FATAL: out of memory!");
      for (j = 0; j < InitS.OtherWmCount; j++)
        {
          AppendMenuItem(others, InitS.OtherWms[j], RestartProc3, I_SELECT);
        }
      AppendMenuItem(really, _("Launch another WM"), others, I_SUBMENU);
    }
    
  AppendMenuItem(TheScreen.UWMMenu, _("Reload application menu"),ReloadMenu,
                 I_SELECT);
}

void WMMenu(int x,int y)
{
  HandlerTable DoIt;
  MenuItem *item;
  item=StartMenu(TheScreen.UWMMenu,x,y,True,ZapWS);
  if(item) {
    if((item->type==I_SELECT)&&(item->data)) {
      DoIt=item->data;
      DoIt(item);
    }
  }
}
