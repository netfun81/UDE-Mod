/***  INIT.C: Contains routines for the initialisation of uwm  ***/

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


#define __USE_GNU

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#warning This will probably not compile with out the config.h
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>
#include <limits.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#error nop! it will not compile.
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/xpm.h>
#include <X11/keysym.h>
#include <X11/extensions/shape.h>

#include "nodes.h"

#include "uwm.h"
#include "special.h"
#include "init.h"
#include "menu.h"
#include "handlers.h"
#include "workspaces.h"
#include "uwmmenu.h"
#include "winmenu.h"
#include "winmenumenu.h"
#include "i18n.h"
#include "urm.h"
#include "pix.h"
#include "MwmUtil_fixed.h"

extern char **environ;

extern UDEScreen TheScreen;
extern Display *disp;
extern XContext UWMContext;
extern XContext UWMGroupContext;
extern Atom WM_STATE_PROPERTY;
extern Atom WM_CHANGE_STATE;
extern Atom WM_TAKE_FOCUS;
extern Atom WM_DELETE_WINDOW;
extern Atom WM_PROTOCOLS;
extern Atom MOTIF_WM_HINTS;

extern InitStruct InitS;

char *RLSpace(char *s)  /*** will remove leading space from string ***/
{
  while(isspace(*s)&&(*s!='\0')) s++;
  return(s);
}

 /*** will dereference all environment-variables found in s. There VEas to be
      enough mem alloc'ed for the dereferenced string to fit into *s.      ***/

void DereferenceENV(char *s)
{
  char *t,*u,*v,*buf;
  int a;

  t=s;
  while((t=strchr(t,'$'))){
    a=0;
    while((u=environ[a]))
      if(!strncmp(u,t+sizeof(char),strlen(u)-strlen(v=strchr(u,'=')))){
        v++;
        buf=MyCalloc(strlen(t),sizeof(char));/* a little more than needed */
        sprintf(buf,"%s",t+(sizeof(char)*(strlen(u)-strlen(v))));
        sprintf(t,"%s%s",v,buf);
        free(buf);
        break;
      } else a++;
    if(!u) t++;
  }
}

void SetupCursors(void)
{
  TheScreen.Mice[C_DEFAULT]=XCreateFontCursor(disp,XC_X_cursor);
  TheScreen.Mice[C_N]=XCreateFontCursor(disp,XC_top_side);
  TheScreen.Mice[C_NO]=XCreateFontCursor(disp,XC_top_right_corner);
  TheScreen.Mice[C_O]=XCreateFontCursor(disp,XC_right_side);
  TheScreen.Mice[C_SO]=XCreateFontCursor(disp,XC_bottom_right_corner);
  TheScreen.Mice[C_S]=XCreateFontCursor(disp,XC_bottom_side);
  TheScreen.Mice[C_SW]=XCreateFontCursor(disp,XC_bottom_left_corner);
  TheScreen.Mice[C_W]=XCreateFontCursor(disp,XC_left_side);
  TheScreen.Mice[C_NW]=XCreateFontCursor(disp,XC_top_left_corner);
  TheScreen.Mice[C_DRAG]=XCreateFontCursor(disp,XC_fleur);
  TheScreen.Mice[C_BORDER]=XCreateFontCursor(disp,XC_center_ptr);
  TheScreen.Mice[C_WINDOW]=XCreateFontCursor(disp,XC_top_left_arrow);
}

char *iconfiles[7]={
  "iconify",
  "close",
  "autorise",
  "back",
  "kill",
  "menu",
  "really"
};

void PrepareIcons()
{
  char filename[512], dirname[300];
  XSetWindowAttributes xswa;
  int a, b;
  int minx, miny;
  XGCValues xgcv;

  a=0;
  if(InitS.HexPath[0]=='\0') sprintf(dirname, "%sgfx/", TheScreen.udedir);
  else sprintf(dirname, "%s/", InitS.HexPath);

  TheScreen.HexMenu.x = INT_MAX;
  TheScreen.HexMenu.y = INT_MAX;
  TheScreen.HexMenu.width = INT_MIN;
  TheScreen.HexMenu.height = INT_MIN;

  minx = miny = 0;
  for(b = 0; b < 7; b++) {
    XpmAttributes xa;
    xa.valuemask = XpmReturnExtensions;
    sprintf(filename, "%s%s.xpm", dirname, iconfiles[b]);
    a |= XpmReadFileToPixmap(disp, TheScreen.root, filename,
                             &TheScreen.HexMenu.icons[b].IconPix,
                             &TheScreen.HexMenu.icons[b].IconShape,
                             &xa);
    TheScreen.HexMenu.icons[b].width = xa.width;
    TheScreen.HexMenu.icons[b].height = xa.height;
    if(xa.valuemask & XpmExtensions) {
      int z;
      for(z = 0; z < xa.nextensions; z++) {
        if((xa.extensions[z].nlines == 0)
           && (2 == sscanf(xa.extensions[z].name, "ude_hex_coords %i %i",
                           &TheScreen.HexMenu.icons[b].x,
                           &TheScreen.HexMenu.icons[b].y))) {
          if(TheScreen.HexMenu.icons[b].x < TheScreen.HexMenu.x) {
            TheScreen.HexMenu.x = TheScreen.HexMenu.icons[b].x;
          }
          if(TheScreen.HexMenu.icons[b].y < TheScreen.HexMenu.y) {
            TheScreen.HexMenu.y = TheScreen.HexMenu.icons[b].y;
          }
          break;
        }
      }
    }
    xa.valuemask = XpmReturnExtensions;
    sprintf(filename, "%s%ss.xpm", dirname, iconfiles[b]);
    a |= XpmReadFileToPixmap(disp, TheScreen.root, filename,
                             &TheScreen.HexMenu.icons[b].IconSelectPix,
                             &TheScreen.HexMenu.icons[b].IconSelectShape,
                             &xa);
    if(TheScreen.HexMenu.icons[b].width < xa.width) {
      TheScreen.HexMenu.icons[b].width = xa.width;
    }
    if(TheScreen.HexMenu.icons[b].height < xa.height) {
      TheScreen.HexMenu.icons[b].height = xa.height;
    }
    if(xa.valuemask & XpmExtensions) {
      int z;
      for(z = 0; z < xa.nextensions; z++) {
        if((xa.extensions[z].nlines == 0)
           && (2 == sscanf(xa.extensions[z].name, "ude_hex_coords %i %i",
                           &TheScreen.HexMenu.icons[b].SelectX,
                           &TheScreen.HexMenu.icons[b].SelectY))) {
          if(TheScreen.HexMenu.icons[b].SelectX < TheScreen.HexMenu.x) {
            TheScreen.HexMenu.x = TheScreen.HexMenu.icons[b].SelectX;
          }
          if(TheScreen.HexMenu.icons[b].SelectY < TheScreen.HexMenu.y) {
            TheScreen.HexMenu.y = TheScreen.HexMenu.icons[b].SelectY;
          }
          break;
        }
      }
    }
    if(TheScreen.HexMenu.width < (TheScreen.HexMenu.icons[b].width
                                  + TheScreen.HexMenu.icons[b].SelectX)) {
      TheScreen.HexMenu.width = TheScreen.HexMenu.icons[b].width
                                + TheScreen.HexMenu.icons[b].SelectX;
    }
    if(TheScreen.HexMenu.width < (TheScreen.HexMenu.icons[b].width
                                  + TheScreen.HexMenu.icons[b].x)) {
      TheScreen.HexMenu.width = TheScreen.HexMenu.icons[b].width
                                + TheScreen.HexMenu.icons[b].x;
    }
    if(TheScreen.HexMenu.height < (TheScreen.HexMenu.icons[b].height
                                  + TheScreen.HexMenu.icons[b].SelectY)) {
      TheScreen.HexMenu.height = TheScreen.HexMenu.icons[b].height
                                + TheScreen.HexMenu.icons[b].SelectY;
    }
    if(TheScreen.HexMenu.height < (TheScreen.HexMenu.icons[b].height
                                  + TheScreen.HexMenu.icons[b].y)) {
      TheScreen.HexMenu.height = TheScreen.HexMenu.icons[b].height
                                + TheScreen.HexMenu.icons[b].y;
    }
  }
  if(a) {
    if(InitS.HexPath[0]=='\0') SeeYa(1, "Icon pixmaps could not be loaded");
    else {
      InitS.HexPath[0] = '\0';
      fprintf(TheScreen.errout,
              "UWM: icons specified in uwmrc not found, trying to load default.\n");
      PrepareIcons();
    }
    return;
  }
  TheScreen.HexMenu.x = -TheScreen.HexMenu.x;
  TheScreen.HexMenu.y = -TheScreen.HexMenu.y;
  TheScreen.HexMenu.width += TheScreen.HexMenu.x;
  TheScreen.HexMenu.height += TheScreen.HexMenu.y;

  xswa.override_redirect = True;
  xswa.save_under = True;
  TheScreen.HexMenu.IconParent
      = XCreateWindow(disp, TheScreen.root, 0, 0,
                      TheScreen.HexMenu.width, TheScreen.HexMenu.height, 0,
                      CopyFromParent, InputOutput, CopyFromParent,
                      (TheScreen.DoesSaveUnders ? CWSaveUnder : 0)
                      | CWOverrideRedirect, &xswa);
  XSelectInput(disp, TheScreen.HexMenu.IconParent, IconParent_EVENT_SELECTION);

  TheScreen.HexMenu.ParentShape
      = XCreatePixmap(disp, TheScreen.HexMenu.IconParent,
                      TheScreen.HexMenu.width, TheScreen.HexMenu.height, 1);

  xgcv.function = GXor;
  TheScreen.HexMenu.ShapeGC = XCreateGC(disp, TheScreen.HexMenu.ParentShape,
                                        GCFunction, &xgcv);

  for(a = 0; a < ICONWINS; a++) {
    TheScreen.HexMenu.icons[a].x += TheScreen.HexMenu.x;
    TheScreen.HexMenu.icons[a].SelectX += TheScreen.HexMenu.x;
    TheScreen.HexMenu.icons[a].y += TheScreen.HexMenu.y;
    TheScreen.HexMenu.icons[a].SelectY += TheScreen.HexMenu.y;
    TheScreen.HexMenu.icons[a].IconWin
        = XCreateWindow(disp, TheScreen.HexMenu.IconParent,
                        TheScreen.HexMenu.icons[a].x,
                        TheScreen.HexMenu.icons[a].y,
                        TheScreen.HexMenu.icons[a].width,
                        TheScreen.HexMenu.icons[a].height,
                        0, CopyFromParent, InputOutput, CopyFromParent,
                        CWOverrideRedirect, &xswa);
    XSelectInput(disp, TheScreen.HexMenu.icons[a].IconWin,
                 IconWin_EVENT_SELECTION);
  }
  XMapSubwindows(disp, TheScreen.HexMenu.IconParent);
}

XFontSet LoadQueryFontSet(Display *disp, char *font_list, const char *fonttype)
{
  char **missing_charsets;
  int missing_count;
  XFontSet fs;

  fs = XCreateFontSet(disp, font_list, &missing_charsets,
                      &missing_count, NULL);
  if(missing_charsets) {
    if(fonttype) {
      fprintf(TheScreen.errout, "Warning loading %s (%s):\n",
              fonttype, font_list);
      for(missing_count = missing_count; missing_count > 0; missing_count--) {
        fprintf(TheScreen.errout, "  Missing charset for your locale: %s\n",
                missing_charsets[missing_count - 1]);
      }
    }
    XFreeStringList(missing_charsets);
  }
  return fs;
}

char *ReadQuoted(FILE *f) /* allocs mem and writes string to it*/
{
  char c, text[257], *p;
  int i;

  text[0] = '\0';
  do {
    if(EOF==fscanf(f,"%c",&c)) return(NULL);
  } while(c!='"');
  for(i=0;i<256;i++) {
    if(EOF==(c = fgetc(f))) return(NULL); /* unfinished quotation */
    if(c=='\\') {
      if(EOF==(c = fgetc(f))) return(NULL); /* allow "s and \s */
    } else if(c=='"') break;
    text[i] = c;
  }
  text[i] = '\0';
  p=MyCalloc(sizeof(char),strlen(text)+1);
  strcpy(p,text);
  return(p);
}

int GetTriple(char *p,int *a,int *b, int *c)
{
  *a=atoi(p); 
  if(!(p=strchr(p,';'))) return(-1);
  p++;
  *b=atoi(p);
  if(!(p=strchr(p,';'))) return(-1);
  p++;
  *c=atoi(p);
  return(0);
}


#define MENUKEYS 6

const char *menukeys[MENUKEYS] = {
  "SUBMENU","ITEM","LINE;","}","FILE","PIPE"};

enum MenuKeyNames {
  SUBMENU,
  ITEM,
  LINE,
  PAREN_CLOSE,
  FILE_KEY, /* FILE is the stream */
  PIPE
};

NodeList *Stack;
Menu *men;

void
ReadMenuFile (FILE *mf)
{
  AppStruct *app;
  char s[256],*t,*u,v[256],c;
  enum MenuKeyNames a;
  FILE *nmf;
  Node *n;
  short useTitle = (TheScreen.desktop.flags & UDESubMenuTitles);
  
  while (EOF != fscanf (mf, "%s", s)) {
    if((s[0] == '#')||(s[0] == '%')) {
      while(s[0]!='\n') {
        if(EOF==fscanf(mf,"%c",s)) {
          break;
        }
      }
    } else {
      for (a=0; a<MENUKEYS; a++) {
        if(!strncmp (s, menukeys[a], strlen(menukeys[a]))) {
          switch(a) {
            case SUBMENU:
                NodeInsert(Stack,men);
                if(!(t=ReadQuoted(mf))) {
                  fprintf(TheScreen.errout,
                          "UWM: error in Menu-file -- ignored");
                  break;
                }

                n=NULL;
                while((n=NodeNext(men->Items,n))) {
                  if((((MenuItem *)(n->data))->type==I_SUBMENU)
                     && (!strcmp(t,((MenuItem *)(n->data))->name))) {
                    break;
                  }
                }
                if(n) {
                  men=((MenuItem *)(n->data))->data;
                } else {
                  if(!(men=MenuCreate(useTitle ? t : NULL))) {
                    SeeYa(1,"FATAL: out of Memory!(Submenu)");
                  }
                  AppendMenuItem(Stack->first->data,t,men,I_SUBMENU);
                }

                do {
                  if(EOF==fscanf(mf,"%c",&c)) {
                    fprintf(TheScreen.errout,
                            "UWM: error in Menu-file -- ignored");
                    free(t);
                    break;
                  }
                } while(c!='{');
                free(t);
                break;

            case ITEM: 
                if(!(u=ReadQuoted(mf))) {
                  fprintf(TheScreen.errout,
                          "UWM: error in Menu-file -- ignored");
                  break;
                }
                do {
                  if(EOF==fscanf(mf,"%c",&c)) {
                    fprintf(TheScreen.errout,
                            "UWM: error in Menu-file -- ignored");
                    free(u);
                    break;
                  }
                } while(c!=':');
                if(!(t=ReadQuoted(mf))) {
                    fprintf(TheScreen.errout,
                            "UWM: error in Menu-file -- ignored");
                    free(u);
                    break;
                }

                n=NULL;
                while((n=NodeNext(men->Items,n))) {
                  if((((MenuItem *)(n->data))->type==I_SELECT)
                     && (!strcmp(u,((MenuItem *)(n->data))->name))) {
                    break;
                  }
                }
                if(!n) {
                  app=MyCalloc(sizeof(AppStruct),1);
                  app->command=t;
                  AppendMenuItem(men,u,app,I_SELECT);
                } else {
                  free(t);
                }
                free(u);
                break;
                
             case LINE:
                if((n=NodePrev(men->Items,NULL))
                   && (((MenuItem *)(n->data))->type != I_LINE)) {
                  AppendMenuItem(men,NULL,NULL,I_LINE);
                }
                break;  /* only one line at a time */
                
             case PAREN_CLOSE:   /*  end of submenu */
                if(Stack->first) {
                  men=Stack->first->data;
                  NodeDelete(Stack,Stack->first);
                } else {
                  fprintf(TheScreen.errout,
                          "UWM: error in Menu-file -- ignored.\n");
                }
                break;
                
             case FILE_KEY:
             case PIPE:
                if(!(t=ReadQuoted(mf))) {
                  fprintf(TheScreen.errout,
                          "UWM: error in Menu-file -- ignored");
                  break;
                }
                strncpy(v,t,255);v[255]='\0';
                free(t);
                if(a == FILE_KEY) DereferenceENV(v);
                if(a == FILE_KEY) {
                  if(!(nmf=MyOpen(v,TheScreen.cppincpaths)))
                    fprintf(TheScreen.errout,
                            "UWM: file not found: %s.\n",v);
                } else {
#ifdef HAVE_POPEN
                  if(!(nmf=popen(v,"r"))) {
                    fprintf(TheScreen.errout,
                            "UWM: couldn't open pipe process: %s\n",t);
                  }
#else
                  fprintf(TheScreen.errout,
                            "UWM: PIPE not supported on your system\n");
#endif
                  }
                if(nmf) {
                  ReadMenuFile(nmf);
                  if(a==4) {
                    fclose(nmf);
#ifdef HAVE_POPEN
                  } else {
                     pclose(nmf);
#endif
                  }
                }
                break;
          }
        }
      }
    }
  }
}

void CreateAppsMenu(char *filename)
{
  AppStruct *app;
  FILE *mf;

  Stack=NodeListCreate();
  if(!Stack)
    SeeYa(1,"FATAL: out of Memory! (menu stack)");

  men=TheScreen.AppsMenu=MenuCreate(_("Application Menu"));
  if(!men)
    SeeYa(1,"FATAL: out of Memory! (ApplicationMenu)");

  DereferenceENV(filename);

  if(!(mf=MyOpen(filename,TheScreen.cppincpaths))){
    fprintf(TheScreen.errout,"UWM: menu file not found, using default.\n");
    app=MyCalloc(sizeof(AppStruct),1);
    app->command="xterm -bg tan4 -fg wheat1 -fn 7x14";
    AppendMenuItem(TheScreen.AppsMenu,"xterm",app,I_SELECT);
    return;
  }

  ReadMenuFile(mf);
  RemoveMenuBottomLines(TheScreen.AppsMenu);
  
  NodeListDelete(&Stack);
  fclose(mf);
}


void AllocXColor(unsigned short R, unsigned short G, unsigned short B,\
                                                          XColor *xcol)
{
  xcol->red=((unsigned short)R);
  xcol->green=((unsigned short)G);
  xcol->blue=((unsigned short)B);
  if(XAllocColor(disp,TheScreen.colormap,xcol))
    return;
  else {
    if(((int)R+G+B)<(384<<8)){
      fprintf(TheScreen.errout,\
              "UWM: Cannot alloc color: %d;%d;%d. Using BlackPixel.\n",R,G,B);
      xcol->pixel=BlackPixel(disp,TheScreen.Screen);
    } else {
      fprintf(TheScreen.errout,\
              "UWM: Cannot alloc color: %d;%d;%d. Using WhitePixel.\n",R,G,B);
      xcol->pixel=WhitePixel(disp,TheScreen.Screen);
    }
    XQueryColor(disp,TheScreen.colormap,xcol);
  }
}

unsigned long AllocColor(unsigned short R, unsigned short G, unsigned short B)
{
  XColor xcol;
  xcol.red=((unsigned short)R);
  xcol.green=((unsigned short)G);
  xcol.blue=((unsigned short)B);
  if(XAllocColor(disp,TheScreen.colormap,&xcol))
    return(xcol.pixel);
  else {
    if(((int)R+G+B)<(384<<8)){
      fprintf(TheScreen.errout,\
              "UWM: Cannot alloc color: %d;%d;%d. Using BlackPixel.\n",R,G,B);
      return(BlackPixel(disp,TheScreen.Screen));
    } else {
      fprintf(TheScreen.errout,\
              "UWM: Cannot alloc color: %d;%d;%d. Using WhitePixel.\n",R,G,B);
      return(WhitePixel(disp,TheScreen.Screen));
    }
  }
}

void FreeXPMBackPixmap(int a)
{
  if(TheScreen.BackPixmap[a]!=None){
    XFreePixmap(disp,TheScreen.BackPixmap[a]);
    XFreeColors(disp,TheScreen.BackPixmapAttributes[a].colormap,\
                 TheScreen.BackPixmapAttributes[a].alloc_pixels,\
              TheScreen.BackPixmapAttributes[a].nalloc_pixels,0);
  }
}

void FreeColor(unsigned long color)
{
  if((color!=BlackPixel(disp,TheScreen.Screen))&&\
       (color!=WhitePixel(disp,TheScreen.Screen)))
    XFreeColors(disp,TheScreen.colormap,&color,1,0);
}

void
AllocXColors(R,G,B,c,l,s)
     unsigned short R,G,B;
     XColor *c,*l,*s;
{
  unsigned int r,g,b;
  AllocXColor(R,G,B,c);
  r=R*TheScreen.FrameBevelFactor;r=(r>0xFFFF)?0xFFFF:r;
  g=G*TheScreen.FrameBevelFactor;g=(g>0xFFFF)?0xFFFF:g;
  b=B*TheScreen.FrameBevelFactor;b=(b>0xFFFF)?0xFFFF:b;
  AllocXColor(r,g,b,l);
  r=R/TheScreen.FrameBevelFactor;
  g=G/TheScreen.FrameBevelFactor;
  b=B/TheScreen.FrameBevelFactor;
  AllocXColor(r,g,b,s);
}

void
AllocColors(R,G,B,c,l,s)
     unsigned short R,G,B;
     unsigned long *c,*l,*s;
{
  unsigned int r,g,b;
  *c=AllocColor(R,G,B);
  r=R*TheScreen.FrameBevelFactor;r=(r>0xFFFF)?0xFFFF:r;
  g=G*TheScreen.FrameBevelFactor;g=(g>0xFFFF)?0xFFFF:g;
  b=B*TheScreen.FrameBevelFactor;b=(b>0xFFFF)?0xFFFF:b;
  *l=AllocColor(r,g,b);
  r=R/TheScreen.FrameBevelFactor;
  g=G/TheScreen.FrameBevelFactor;
  b=B/TheScreen.FrameBevelFactor;
  *s=AllocColor(r,g,b);
}

int ParseColor(char *p,int *r,int *g,int *b)
{
  if(GetTriple(p,r,g,b)) {
    XColor xcol;
    p=RLSpace(p);
    if(!XParseColor(disp,TheScreen.colormap,p,&xcol)) return(-1);
    *r=xcol.red;*g=xcol.green;*b=xcol.blue;
  } else {
    *r=*r<<8;*g=*g<<8;*b=*b<<8;
  }
  return(0);
}

void AllocWSS(short wss)
{
  int a;
  unsigned long b,c,d;

  for(a=wss;a<TheScreen.desktop.WorkSpaces;a++){
    FreeColor(TheScreen.Background[a]);
    FreeColor(TheScreen.InactiveBorder[a]);
    FreeColor(TheScreen.ActiveBorder[a]);
    FreeColor(TheScreen.InactiveLight[a]);
    FreeColor(TheScreen.InactiveShadow[a]);
    FreeColor(TheScreen.ActiveLight[a]);
    FreeColor(TheScreen.ActiveShadow[a]);
    FreeColor(TheScreen.ActiveTitleFont[a]);
    FreeColor(TheScreen.InactiveTitleFont[a]);
    for(b=0;b<UDE_MAXCOLORS;b++)
      FreeColor(TheScreen.Colors[a][b].pixel);
    FreeXPMBackPixmap(a);
  }


#define AllocWSSThing(X) if(!(TheScreen.X=realloc(TheScreen.X,wss*sizeof(*TheScreen.X)))) SeeYa(1,"Fatal: out of memory allocating workspace storage.");

  AllocWSSThing(SetBackground);
  AllocWSSThing(Background);
  AllocWSSThing(InactiveBorder);
  AllocWSSThing(ActiveBorder);
  AllocWSSThing(InactiveLight);
  AllocWSSThing(InactiveShadow);
  AllocWSSThing(ActiveLight);
  AllocWSSThing(ActiveShadow);
  AllocWSSThing(ActiveTitleFont);
  AllocWSSThing(InactiveTitleFont);
  AllocWSSThing(BackPixmap);
  AllocWSSThing(BackPixmapAttributes);
  AllocWSSThing(BackCommand);
  AllocWSSThing(Colors);
  AllocWSSThing(WorkSpace);

#undef AllocWSSThing

  for (a= TheScreen.desktop.WorkSpaces; a < wss; a++)
    {
      char ts[32];
      TheScreen.SetBackground[a]= 0;
      b=AllocColor(40<<8,20<<8,20<<8); /* Screen background */
      TheScreen.Background[a]=b;
      AllocColors(139<<8,71<<8,38<<8,&b,&c,&d);/* border of inactive win */
      TheScreen.InactiveBorder[a]=b;
      TheScreen.InactiveLight[a]=c;
      TheScreen.InactiveShadow[a]=d;
      AllocColors(238<<8,180<<8,34<<8,&b,&c,&d); /* border of active win */
      TheScreen.ActiveBorder[a]=b;
      TheScreen.ActiveLight[a]=c;
      TheScreen.ActiveShadow[a]=d;
      AllocXColors(139<<8,101<<8,8<<8,&TheScreen.Colors[a][UDE_Back],
                                      &TheScreen.Colors[a][UDE_Light],
                                      &TheScreen.Colors[a][UDE_Shadow]);
      AllocXColor(255<<8,193<<8,37<<8,&TheScreen.Colors[a][UDE_StandardText]); 
      AllocXColor(210<<8,200<<8,80<<8,&TheScreen.Colors[a][UDE_InactiveText]); 
      AllocXColor(0<<8,0<<8,0<<8,&TheScreen.Colors[a][UDE_HighlightedText]); 
      AllocXColor(226<<8,220<<8,186<<8,&TheScreen.Colors[a][UDE_HighlightedBgr]); 
      AllocXColor(226<<8,220<<8,186<<8,&TheScreen.Colors[a][UDE_TextColor]); 
      AllocXColor(0<<8,0<<8,0<<8,&TheScreen.Colors[a][UDE_TextBgr]); 

      b=AllocColor(70<<8,35<<8,19<<8);  /* ActiveTitleFont */
      TheScreen.ActiveTitleFont[a]=b;
      b=AllocColor(255<<8,193<<8,36<<8); /* InactiveTitleFont */
      TheScreen.InactiveTitleFont[a]=b;
      /* initialize rest to 0 */
      TheScreen.BackCommand[a]=NULL;
      TheScreen.BackPixmap[a]=None;
      sprintf(ts, "%d", a);
      strncpy(TheScreen.WorkSpace[a], ts, 32);
    }
  TheScreen.desktop.WorkSpaces=wss;
}

#define KEYWORDS 55

const char *Keywords[KEYWORDS]={
  "BorderWidth",
  "TitleHeight",
  "ScreenColor",
  "ScreenPixmap",
  "InactiveWin",
  "ActiveWin",
  "MenuFont",
  "BackColor",
  "FontColor",
  "MenuFile",
  "StartScript",
  "RubberMove",
  "MenuSize",
  "NarrowBorderWidth",
  "UWMMenuButton",
  "DeiconifyButton",
  "AppMenuButton",
  "SubMenuTitles",
  "TransientMenues",
  "WinMenuButton",
  "DragButtons",
  "ResizeButtons",
  "WorkSpaces",
  "WorkSpaceName",
  "WorkSpaceNr",
  "PlacementStrategy",
  "PlacementThreshold",
  "ScreenCommand",
  "ReadFrom",
  "BevelFactor",
  "FrameBevelWidth",
  "OpaqueMoveSize",
  "TitleFont",
  "ActiveTitle",
  "InactiveTitle",
  "FrameFlags",
  "OtherWMs",
  "MaxWinWidth",
  "MaxWinHeight",
  "StopScript",
  "WarpPointerToNewWinH",
  "WarpPointerToNewWinV",
  "InactiveText",
  "HighlightedText",
  "HighlightedBgr",
  "TextColor",
  "TextBgr",
  "BevelWidth",
  "ResourceFile",
  "SnapDistance",
  "HexPath",
  "TextFont",
  "HighlightFont",
  "InactiveFont",
  "BehaviourFlags"};

enum KeyNames {
  BorderWidth,
  TitleHeight,
  ScreenColor,
  ScreenPixmap,
  InactiveWin,
  ActiveWin,
  MenuFont,
  BackColor,
  FontColor,
  MenuFile,
  StartScript,
  RubberMove,
  MenuSize,
  NarrowBorderWidth,
  UWMMenuButton,
  DeiconifyButton,
  AppMenuButton,
  SubMenuTitles,
  TransientMenues,
  WinMenuButton,
  DragButtons,
  ResizeButtons,
  WorkSpaces,
  WorkSpaceName,
  WorkSpaceNr,
  PlacementStrategy,
  PlacementThreshold,
  ScreenCommand,
  ReadFrom,
  BevelFactor,
  FrameBevelWidth,
  OpaqueMoveSize,
  TitleFont,
  ActiveTitle,
  InactiveTitle,
  FrameFlags,
  OtherWMs,
  MaxWinWidth,
  MaxWinHeight,
  StopScript,
  WarpPointerToNewWinH,
  WarpPointerToNewWinV,
  InactiveText,
  HighlightedText,
  HighlightedBgr,
  TextColor,
  TextBgr,
  BevelWidth,
  ResourceFile,
  SnapDistance,
  HexPath,
  TextFont,
  HighlightFont,
  InactiveFont,
  BehaviourFlags
};

/*** read config file ***/
void ReadConfigFile(FILE *uwmrc)
{
  char s[256],*p,*token;
  int b,r,g;
  enum KeyNames a;
  FILE *secuwmrc;

  while(fgets (s, 255, uwmrc))
    {
      if (*s && (s[strlen (s) - 1] == '\n'))
        s[strlen(s)-1]= '\0';
      if ((s[0] != '#')||(s[0] != '%'))
        for (a=0; a < KEYWORDS; a++)
          if(!strncmp(s, Keywords[a], strlen (Keywords[a])))
            {
              p= strchr(s, '=');
              if(p)
                {
                  p++;
                  if(strlen (p))
                    switch(a)
                      {
                      case BorderWidth:
                        TheScreen.BorderWidth1=atoi(p);
                        break;
                      case TitleHeight:
                        TheScreen.TitleHeight=atoi(p);
                        break;
                      case ScreenColor:
                        if(ParseColor (p, &r, &g, &b))
                          {
                            fprintf(TheScreen.errout,
                                    _("UWM: error in config-file. Ignored.\n"));
                            break;
                          }
                        FreeColor(TheScreen.Background
                                  [TheScreen.desktop.ActiveWorkSpace]);
                        TheScreen.SetBackground[TheScreen.desktop.
                                                ActiveWorkSpace] = 1;
                        TheScreen.Background[TheScreen.desktop.ActiveWorkSpace]
                                            = AllocColor(r, g, b);
                        break;
                      case ScreenPixmap:
                        DereferenceENV(p);
                        p=RLSpace(p);
                        FreeXPMBackPixmap(TheScreen.desktop.ActiveWorkSpace);
                        if(!LoadPic(p, &TheScreen.BackPixmap[TheScreen.desktop.\
                                    ActiveWorkSpace], &TheScreen.\
                                    BackPixmapAttributes[TheScreen.desktop.\
                                    ActiveWorkSpace])){
                          fprintf(TheScreen.errout,\
                                 "UWM: Background could not be loaded: %s\n",p);
                          TheScreen.BackPixmap[TheScreen.desktop.\
                                               ActiveWorkSpace]=None;
                        }
                        break;
                      case InactiveWin:
                        if(ParseColor(p,&r,&g,&b)) {
                          fprintf(TheScreen.errout,\
                                  _("UWM: error in config-file. Ignored.\n"));
                          break;
                        }
                        FreeColor(TheScreen.InactiveBorder[TheScreen.desktop.\
                                                          ActiveWorkSpace]);
                        FreeColor(TheScreen.InactiveLight[TheScreen.desktop.\
                                                         ActiveWorkSpace]);
                        FreeColor(TheScreen.InactiveShadow[TheScreen.desktop.\
                                                          ActiveWorkSpace]);
                        AllocColors(r,g,b,
                                    &TheScreen.InactiveBorder[TheScreen.\
                                    desktop.ActiveWorkSpace],\
                                    &TheScreen.InactiveLight[TheScreen.\
                                    desktop.ActiveWorkSpace],\
                                    &TheScreen.InactiveShadow[TheScreen.\
                                    desktop.ActiveWorkSpace]);
                        break;
                      case ActiveWin:
                        if(ParseColor(p,&r,&g,&b)) {
                          fprintf(TheScreen.errout,\
                                  _("UWM: error in config-file. Ignored.\n"));
                          break;
                        }
                        FreeColor(TheScreen.ActiveBorder[TheScreen.desktop.\
                                  ActiveWorkSpace]);
                        FreeColor(TheScreen.ActiveLight[TheScreen.desktop.\
                                  ActiveWorkSpace]);
                        FreeColor(TheScreen.ActiveShadow[TheScreen.desktop.\
                                  ActiveWorkSpace]);
                        AllocColors(r,g,b,
                                    &TheScreen.ActiveBorder[TheScreen.desktop.\
                                    ActiveWorkSpace],\
                                    &TheScreen.ActiveLight[TheScreen.desktop.\
                                    ActiveWorkSpace],\
                                    &TheScreen.ActiveShadow[TheScreen.desktop.\
                                    ActiveWorkSpace]);
                        break;
                      case MenuFont:
                        { XFontSet Font;
                          char *c;
                          p=RLSpace(p);
                          if((Font=LoadQueryFontSet(disp,p,
                                                    Keywords[MenuFont]))){
                            XFreeFontSet(disp,TheScreen.MenuFont);
                            free(TheScreen.desktop.StandardFontSet);
                            TheScreen.desktop.StandardFontSet = MyStrdup(p);
                            if((c = strchr(p, ','))) {
                              *c = '\0';
                              p=RLSpace(p);
                            }
                            free(TheScreen.desktop.StandardFont);
                            TheScreen.desktop.StandardFont = MyStrdup(p);
                            TheScreen.MenuFont=Font;
                          }
                        }
                        break;
                      case BackColor:
                        if(ParseColor(p,&r,&g,&b)) {
                          fprintf(TheScreen.errout,\
                                  _("UWM: error in config-file. Ignored.\n"));
                          break;
                        }
                        FreeColor(TheScreen.Colors[TheScreen.desktop.\
                                  ActiveWorkSpace][UDE_Back].pixel);
                        FreeColor(TheScreen.Colors[TheScreen.desktop.\
                                  ActiveWorkSpace][UDE_Light].pixel);
                        FreeColor(TheScreen.Colors[TheScreen.desktop.\
                                  ActiveWorkSpace][UDE_Shadow].pixel);
                        AllocColors(r,g,b,\
                                    &TheScreen.Colors[TheScreen.desktop.\
                                    ActiveWorkSpace][UDE_Back],\
                                    &TheScreen.Colors[TheScreen.desktop.\
                                    ActiveWorkSpace][UDE_Light],\
                                    &TheScreen.Colors[TheScreen.desktop.\
                                    ActiveWorkSpace][UDE_Shadow]);
                        break;
                      case FontColor:
                        if(ParseColor(p,&r,&g,&b)) {
                          fprintf(TheScreen.errout,\
                                  _("UWM: error in config-file. Ignored.\n"));
                          break;
                        }
                        FreeColor(TheScreen.Colors[TheScreen.desktop.\
                                  ActiveWorkSpace][UDE_StandardText].pixel);
                        AllocXColor(r,g,b,&TheScreen.Colors[TheScreen.desktop.
                                    ActiveWorkSpace][UDE_StandardText]);
                        break;
                      case MenuFile:
                        p=RLSpace(p);
                        strncpy(InitS.MenuFileName,p,255);
                        break;
                      case StartScript:
                        if(InitS.StartScript[0]!='\n'){
                          p=RLSpace(p);
                          strncpy(InitS.StartScript,p,255);
                          DereferenceENV(InitS.StartScript);
                        }
                        break;
                      case RubberMove:
                        InitS.RubberMove=atoi(p);
                        break;
                      case MenuSize:
                        if(GetTriple(p,&InitS.MenuBorderWidth,\
                                     &InitS.MenuXOffset,&InitS.MenuYOffset))
                           fprintf(TheScreen.errout,\
                                   _("UWM: error in config-file. Ignored.\n"));
                        break;
                      case NarrowBorderWidth:
                        TheScreen.BorderWidth2=atoi(p);
                        break;
                      case UWMMenuButton:
                        b=atoi(p);  /* never trust a user to death 
                                       (i.e. segmentation fault) */
                        if((b<4)&&(b>0)) InitS.menuType[b-1] = 'U';
                        break;
                      case DeiconifyButton:
                        b=atoi(p);
                        if((b<4)&&(b>0)) InitS.menuType[b-1] = 'D';
                        break;
                      case AppMenuButton:
                        b=atoi(p);
                        if((b<4)&&(b>0)) InitS.menuType[b-1] = 'A';
                        break;
                     case SubMenuTitles:
                       TheScreen.desktop.flags = (atoi(p)!=0) 
                                   ? TheScreen.desktop.flags|UDESubMenuTitles
                                   : TheScreen.desktop.flags&~UDESubMenuTitles;
                        break;
                      case TransientMenues:
                        TheScreen.desktop.flags = (atoi(p)!=0) 
                                   ? TheScreen.desktop.flags|UDETransientMenus
                                   : TheScreen.desktop.flags&~UDETransientMenus;
                        break;
                      case WinMenuButton:
                        b=atoi(p);
                        if((b<4)&&(b>0))
                          InitS.BorderButtons[b-1]=InitS.WMMenuButtons[b-1]='M';
                        if(!(p=strchr(p,';'))){
                          b=b%3;
                          InitS.WMMenuButtons[b] = 'Z';
                          b=(b+1)%3;
                          InitS.WMMenuButtons[b] = 'X';
                          fprintf(TheScreen.errout,"You're still using old WinMenuButton-format in uwmrc - please update\n");
                          break;
                        }
                        p++;
                        b=atoi(p);
                        if((b<4)&&(b>0)) InitS.WMMenuButtons[b-1] = 'Z';
                        if(!(p=strchr(p,';'))){
                          fprintf(TheScreen.errout,\
                                   _("UWM: error in config-file. Ignored.\n"));
                          break;
                        }
                        p++;
                        b=atoi(p);
                        if((b<4)&&(b>0)) InitS.WMMenuButtons[b-1] = 'X';
                        break;
                        break;
                      case DragButtons:
                        b=atoi(p);
                        if((b<4)&&(b>0))
                          InitS.BorderButtons[b-1]=InitS.DragButtons[b-1]='D';
                        if(!(p=strchr(p,';'))){
                          fprintf(TheScreen.errout,\
                                   _("UWM: error in config-file. Ignored.\n"));
                          break;
                        }
                        p++;
                        b=atoi(p);
                        if((b<4)&&(b>0)) InitS.DragButtons[b-1] = 'R';
                        if(!(p=strchr(p,';'))){
                          fprintf(TheScreen.errout,\
                                   _("UWM: error in config-file. Ignored.\n"));
                          break;
                        }
                        p++;
                        b=atoi(p);
                        if((b<4)&&(b>0)) InitS.DragButtons[b-1] = 'L';
                        break;
                      case ResizeButtons:
                        b=atoi(p);
                        if((b<4)&&(b>0))
                          InitS.BorderButtons[b-1]=InitS.ResizeButtons[b-1]='R';
                        if(!(p=strchr(p,';'))){
                          fprintf(TheScreen.errout,\
                                   _("UWM: error in config-file. Ignored.\n"));
                     break;
                        }
                        p++;
                        b=atoi(p);
                        if((b<4)&&(b>0)) InitS.ResizeButtons[b-1] = 'A';
                        if(!(p=strchr(p,';'))){
                          fprintf(TheScreen.errout,\
                                   _("UWM: error in config-file. Ignored.\n"));
                          break;
                        }
                        p++;
                        b=atoi(p);
                        if((b<4)&&(b>0)) InitS.ResizeButtons[b-1] = 'U';
                        break;
                      case WorkSpaces:
                        if((b=atoi(p))>0)
                          AllocWSS(b);
                        break;
                      case WorkSpaceName:
                        p=RLSpace(p);
                        strncpy(TheScreen.WorkSpace[TheScreen.desktop.\
                                 ActiveWorkSpace],p,31);
                        break;
                      case WorkSpaceNr:
                        b=atoi(p);
                        if(b>=TheScreen.desktop.WorkSpaces)
                           b=TheScreen.desktop.WorkSpaces-1;
                        TheScreen.desktop.ActiveWorkSpace=b;
                        break;
                      case PlacementStrategy:
                        InitS.PlacementStrategy=atoi(p);
                        break;
                      case PlacementThreshold:
                        InitS.PlacementThreshold=atol(p);
                        break;
                      case ScreenCommand:
                        p=RLSpace(p);
                        TheScreen.BackCommand[TheScreen.desktop.ActiveWorkSpace]
                                           = MyCalloc(1,strlen(p)+sizeof(char));
                        strcpy(TheScreen.BackCommand\
                                         [TheScreen.desktop.ActiveWorkSpace],p);
                        fprintf(stderr,"read %d as %s\n",\
                                TheScreen.desktop.ActiveWorkSpace,p);
                        break;
                      case ReadFrom:
                        DereferenceENV(p);
                        p=RLSpace(p);
                        if(!(secuwmrc=MyOpen(p,TheScreen.cppincpaths))){
                          fprintf(TheScreen.errout,\
                                  "UWM: file not found: %s.\n",p);
                          break;
                        }
                        if(secuwmrc){
                          ReadConfigFile(secuwmrc);
                          fclose(secuwmrc);
                        }
                        break;     
                      case BevelFactor:
                        TheScreen.FrameBevelFactor=atof(p);
                        if(TheScreen.FrameBevelFactor==0)
                          TheScreen.FrameBevelFactor=1;   /* avoid 0div. */
                        break;
                      case FrameBevelWidth:
                        TheScreen.FrameBevelWidth=atoi(p);
                        break;
                      case OpaqueMoveSize:
                        InitS.OpaqueMoveSize=atol(p);
                        break;
                      case TitleFont:
                        { XFontSet Font;
                          p=RLSpace(p);
                          if((Font=LoadQueryFontSet(disp,p,
                                                    Keywords[TitleFont]))){
                            XFreeFontSet(disp,TheScreen.TitleFont);
                            TheScreen.TitleFont=Font;
                          }
                        }
                        break;
                      case ActiveTitle:
                        if(ParseColor(p,&r,&g,&b)) {
                          fprintf(TheScreen.errout,\
                                  _("UWM: error in config-file. Ignored.\n"));
                          break;
                        }
                        FreeColor(TheScreen.ActiveTitleFont\
                                  [TheScreen.desktop.ActiveWorkSpace]);
                        TheScreen.ActiveTitleFont[TheScreen.desktop.\
                                  ActiveWorkSpace]=AllocColor(r,g,b);
                        break;
                      case InactiveTitle:
                        if(ParseColor(p,&r,&g,&b)) {
                          fprintf(TheScreen.errout,\
                                  _("UWM: error in config-file. Ignored.\n"));
                          break;
                        }
                        FreeColor(TheScreen.InactiveTitleFont\
                                  [TheScreen.desktop.ActiveWorkSpace]);
                        TheScreen.InactiveTitleFont[TheScreen.desktop.\
                                    ActiveWorkSpace]=AllocColor(r,g,b);
                        break;
                      case FrameFlags:
                        InitS.BorderTitleFlags=(char)atoi(p);
                        break;
                      case OtherWMs:
                        token = strtok(p, ",");
                        while (token) {
                          InitS.OtherWmCount++;
                          if(!(InitS.OtherWms=realloc(InitS.OtherWms,\
                                  InitS.OtherWmCount*sizeof(char *))))
                            SeeYa(1,"FATAL: out of Mem!");
                          
                          token=RLSpace(token);
                          InitS.OtherWms[InitS.OtherWmCount-1] =
                                         MyCalloc(strlen(token)+1,sizeof(char));
                          strcpy(InitS.OtherWms[InitS.OtherWmCount-1], token);
                          token=strtok(NULL, ",");
                        }                                  
                        break;
                      case MaxWinWidth:
                        TheScreen.MaxWinWidth=atoi(p);
                        break;
                      case MaxWinHeight:
                        TheScreen.MaxWinHeight=atoi(p);
                        break;
                      case StopScript:
                        if(InitS.StopScript[0]!='\n'){
                          p=RLSpace(p);
                          strncpy(InitS.StopScript,p,255);
                          DereferenceENV(InitS.StopScript);
                        }
                        break;
                      case WarpPointerToNewWinH:
                        InitS.WarpPointerToNewWinH=atoi(p);
                        if((InitS.WarpPointerToNewWinH < -2) ||
                           (InitS.WarpPointerToNewWinH > 100))
                          InitS.WarpPointerToNewWinH=-1;
                        break;
                      case WarpPointerToNewWinV:
                        InitS.WarpPointerToNewWinV=atoi(p);
                        if((InitS.WarpPointerToNewWinV < -2) ||
                           (InitS.WarpPointerToNewWinV > 100))
                          InitS.WarpPointerToNewWinV=-1;
                        break;
                      case InactiveText:
                        if(ParseColor(p,&r,&g,&b)) {
                          fprintf(TheScreen.errout,\
                                  _("UWM: error in config-file. Ignored.\n"));
                          break;
                        }
                        FreeColor(TheScreen.Colors[TheScreen.desktop.\
                                  ActiveWorkSpace][UDE_InactiveText].pixel);
                        AllocXColor(r,g,b,&TheScreen.Colors[TheScreen.desktop.\
                                    ActiveWorkSpace][UDE_InactiveText]);
                        break;
                      case HighlightedText:
                        if(ParseColor(p,&r,&g,&b)) {
                          fprintf(TheScreen.errout,\
                                  _("UWM: error in config-file. Ignored.\n"));
                          break;
                        }
                        FreeColor(TheScreen.Colors[TheScreen.desktop.\
                                  ActiveWorkSpace][UDE_HighlightedText].pixel);
                        AllocXColor(r,g,b,&TheScreen.Colors[TheScreen.desktop.\
                                    ActiveWorkSpace][UDE_HighlightedText]);
                        break;
                      case HighlightedBgr:
                        if(ParseColor(p,&r,&g,&b)) {
                          fprintf(TheScreen.errout,\
                                  _("UWM: error in config-file. Ignored.\n"));
                          break;
                        }
                        FreeColor(TheScreen.Colors[TheScreen.desktop.\
                                  ActiveWorkSpace][UDE_HighlightedBgr].pixel);
                        AllocXColor(r,g,b,&TheScreen.Colors[TheScreen.desktop.\
                                    ActiveWorkSpace][UDE_HighlightedBgr]);
                        break;
                      case TextColor:
                        if(ParseColor(p,&r,&g,&b)) {
                          fprintf(TheScreen.errout,\
                                  _("UWM: error in config-file. Ignored.\n"));
                          break;
                        }
                        FreeColor(TheScreen.Colors[TheScreen.desktop.\
                                  ActiveWorkSpace][UDE_TextColor].pixel);
                        AllocXColor(r,g,b,&TheScreen.Colors[TheScreen.desktop.\
                                    ActiveWorkSpace][UDE_TextColor]);
                        break;
                      case TextBgr:
                        if(ParseColor(p,&r,&g,&b)) {
                          fprintf(TheScreen.errout,\
                                  _("UWM: error in config-file. Ignored.\n"));
                          break;
                        }
                        FreeColor(TheScreen.Colors[TheScreen.desktop.\
                                  ActiveWorkSpace][UDE_TextBgr].pixel);
                        AllocXColor(r,g,b,&TheScreen.Colors[TheScreen.desktop.\
                                    ActiveWorkSpace][UDE_TextBgr]);
                        break;
                      case BevelWidth:
                        TheScreen.desktop.BevelWidth=atoi(p);
                        break;
                      case ResourceFile:
                        DereferenceENV(p);
                        p=RLSpace(p);
                        ReadResourceDBFromFile(p);
                        break;
                      case SnapDistance:
                        InitS.SnapDistance=atoi(p);
                        break;
                      case HexPath:
                        p=RLSpace(p);
                        strncpy(InitS.HexPath,p,255);
                        DereferenceENV(InitS.HexPath);
                        break;
                      case InactiveFont:
                        { XFontSet Font;
                          char *c;
                          p=RLSpace(p);
                          if((Font=LoadQueryFontSet(disp,p,
                                                    Keywords[InactiveFont]))){
                            XFreeFontSet(disp,Font);
                            free(TheScreen.desktop.InactiveFontSet);
                            TheScreen.desktop.InactiveFontSet = MyStrdup(p);
                            if((c = strchr(p, ','))) {
                              *c = '\0';
                              p=RLSpace(p);
                            }
                            free(TheScreen.desktop.InactiveFont);
                            TheScreen.desktop.InactiveFont = MyStrdup(p);
                          }
                        }
                        break;
                      case HighlightFont:
                        { XFontSet Font;
                          char *c;
                          p=RLSpace(p);
                          if((Font=LoadQueryFontSet(disp,p,
                                                    Keywords[HighlightFont]))){
                            XFreeFontSet(disp,Font);
                            free(TheScreen.desktop.HighlightFontSet);
                            TheScreen.desktop.HighlightFontSet = MyStrdup(p);
                            if((c = strchr(p, ','))) {
                              *c = '\0';
                              p=RLSpace(p);
                            }
                            free(TheScreen.desktop.HighlightFont);
                            TheScreen.desktop.HighlightFont = MyStrdup(p);
                          }
                        }
                        break;
                      case TextFont:
                        { XFontSet Font;
                          char *c;
                          p=RLSpace(p);
                          if((Font=LoadQueryFontSet(disp,p,
                                                    Keywords[TextFont]))){
                            XFreeFontSet(disp,Font);
                            free(TheScreen.desktop.TextFontSet);
                            TheScreen.desktop.TextFontSet = MyStrdup(p);
                            if((c = strchr(p, ','))) {
                              *c = '\0';
                              p=RLSpace(p);
                            }
                            free(TheScreen.desktop.TextFont);
                            TheScreen.desktop.TextFont = MyStrdup(p);
                          }
                        }
                        break;
                      case BehaviourFlags:
                        InitS.BehaviourFlags=atoi(p);
                        break;
                      }
                }
            }
    }
}

//Copied from xbindkeys
int numlock_mask, scrolllock_mask, capslock_mask;
int ignore_mask;

void
get_offending_modifiers (Display * dpy)
{
  int i;
  XModifierKeymap *modmap;
  KeyCode nlock, slock;
  static int mask_table[8] = {
    ShiftMask, LockMask, ControlMask, Mod1Mask,
    Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
  };

  nlock = XKeysymToKeycode (dpy, XK_Num_Lock);
  slock = XKeysymToKeycode (dpy, XK_Scroll_Lock);

  /*
   * Find out the masks for the NumLock and ScrollLock modifiers,
   * so that we can bind the grabs for when they are enabled too.
   */
  modmap = XGetModifierMapping (dpy);

  if (modmap != NULL && modmap->max_keypermod > 0)
    {
      for (i = 0; i < 8 * modmap->max_keypermod; i++)
	{
	  if (modmap->modifiermap[i] == nlock && nlock != 0)
	    numlock_mask = mask_table[i / modmap->max_keypermod];
	  else if (modmap->modifiermap[i] == slock && slock != 0)
	    scrolllock_mask = mask_table[i / modmap->max_keypermod];
	}
    }

  capslock_mask = LockMask;

  ignore_mask=~(capslock_mask|numlock_mask|scrolllock_mask);
  if (modmap)
    XFreeModifiermap (modmap);
}

void InitDefaults()
{
  FILE *uwmrc;

/*** set values to default before reading config file. ***/

  TheScreen.BorderWidth1=10;
  TheScreen.BorderWidth2=3;
  TheScreen.TitleHeight=0;

  TheScreen.MaxWinWidth=0;
  TheScreen.MaxWinHeight=0;

  TheScreen.desktop.ActiveWorkSpace=0;
  TheScreen.desktop.WorkSpaces=0;
  TheScreen.desktop.flags=UDETransientMenus;
  TheScreen.desktop.BevelWidth=2;
  TheScreen.SetBackground= 0;
  TheScreen.Background=TheScreen.InactiveBorder=TheScreen.ActiveBorder=NULL;
  TheScreen.BackPixmap=NULL;
  TheScreen.BackCommand=NULL;
  TheScreen.WorkSpace=NULL;
  TheScreen.Colors=NULL;

  TheScreen.FrameBevelFactor=1.5;
  TheScreen.FrameBevelWidth=2;

  AllocWSS(1);

  if(!(TheScreen.TitleFont=LoadQueryFontSet(disp,"fixed",NULL))) {
    fprintf(TheScreen.errout,"UWM: Standard Title-font does not exist.\n");
    fprintf(TheScreen.errout," loading fixed.\n");
  }
  TheScreen.desktop.InactiveFont = MyStrdup("fixed");
  if(!(TheScreen.MenuFont=LoadQueryFontSet(disp,
                                           TheScreen.desktop.InactiveFont,
                                           NULL))){
    fprintf(TheScreen.errout,"UWM: Standard Inactive Font does not exist.\n");
  }
  if(TheScreen.MenuFont) XFreeFontSet(disp, TheScreen.MenuFont);
  TheScreen.desktop.HighlightFont = MyStrdup("fixed");
  if(!(TheScreen.MenuFont=LoadQueryFontSet(disp,
                                           TheScreen.desktop.HighlightFont,
                                           NULL)))
  {
    free(TheScreen.desktop.HighlightFont);
    fprintf(TheScreen.errout,"UWM: Standard Highlight Font does not exist.\n");
  }
  if(TheScreen.MenuFont) XFreeFontSet(disp, TheScreen.MenuFont);
  TheScreen.desktop.TextFont = MyStrdup("fixed");
  if(!(TheScreen.MenuFont=LoadQueryFontSet(disp,TheScreen.desktop.TextFont,
                                           NULL))){
    free(TheScreen.desktop.TextFont);
    fprintf(TheScreen.errout,"UWM: Standard Text Font does not exist.\n");
  }
  if(TheScreen.MenuFont) XFreeFontSet(disp, TheScreen.MenuFont);
  TheScreen.desktop.StandardFont = MyStrdup("fixed");
  if(!(TheScreen.MenuFont=LoadQueryFontSet(disp,
                                           TheScreen.desktop.StandardFont,
                                           NULL))){
    free(TheScreen.desktop.StandardFont);
    fprintf(TheScreen.errout,"UWM: Standard Font does not exist.\n");
  }
  TheScreen.desktop.StandardFontSet = MyStrdup(TheScreen.desktop.StandardFont);
  TheScreen.desktop.InactiveFontSet = MyStrdup(TheScreen.desktop.InactiveFont);
  TheScreen.desktop.HighlightFontSet
                                   = MyStrdup(TheScreen.desktop.HighlightFont);
  TheScreen.desktop.TextFontSet = MyStrdup(TheScreen.desktop.TextFont);

  InitS.menuType[0]='U';
  InitS.menuType[1]='D';
  InitS.menuType[2]='A';
  InitS.MenuBorderWidth=2;
  InitS.MenuXOffset=4;
  InitS.MenuYOffset=2;

  InitS.BorderButtons[0]='M';
  InitS.BorderButtons[1]='D';
  InitS.BorderButtons[2]='R';
  InitS.DragButtons[0]='R';
  InitS.DragButtons[1]='D';
  InitS.DragButtons[2]='L';
  InitS.ResizeButtons[0]='U';
  InitS.ResizeButtons[1]='A';
  InitS.ResizeButtons[2]='R';
  InitS.WMMenuButtons[0]='M';
  InitS.WMMenuButtons[1]='Z';
  InitS.WMMenuButtons[2]='X';

  InitS.RubberMove=0;
  InitS.OpaqueMoveSize=0;
  InitS.PlacementStrategy=3;
  InitS.PlacementThreshold=0;
  InitS.OtherWms=NULL;
  InitS.OtherWmCount=0;
  InitS.WarpPointerToNewWinH=-1;
  InitS.WarpPointerToNewWinV=-1;
  InitS.SnapDistance=10;
  InitS.HexPath[0]='\0';
  InitS.BehaviourFlags=0;

  InitS.BorderTitleFlags = BT_GROOVE|BT_LINE|BT_INACTIVE_TITLE|BT_ACTIVE_TITLE|\
                                                                 BT_DODGY_TITLE;

  if(!(uwmrc=MyOpen("uwmrc",TheScreen.cppincpaths))) {
    fprintf(TheScreen.errout,"UWM: no config file found, using defaults.\n");
    CreateAppsMenu("appmenu");
    return;
  } 
  ReadConfigFile(uwmrc);
  fclose(uwmrc);
  CreateAppsMenu(InitS.MenuFileName);
}

void InitUWM()
{
  char *env;
  XGCValues xgcv;
  XSetWindowAttributes xswa;
  XEvent event;
  Window otherwmswin;

  int mask,i;

/*** some inits to let SeeYa know what to deallocate etc... ***/

  TheScreen.UltimateList= NULL;   /* in case we have to quit very soon */
  TheScreen.AppsMenu= NULL;
  TheScreen.UWMMenu= NULL;

/*** from now on SeeYa can be called 8-) ***/

/*** setting up general X-stuff and initializing some TheScreen-members ***/

  TheScreen.Home= getenv("HOME");
  if(!TheScreen.Home) TheScreen.Home= ".";

  disp= XOpenDisplay(NULL);
  if(!disp)
    SeeYa(1,"Cannot open display");
  /* That's it already in case we can't get a Display */

  TheScreen.Screen= DefaultScreen (disp);
  TheScreen.width= DisplayWidth (disp,TheScreen.Screen);
  TheScreen.height= DisplayHeight (disp,TheScreen.Screen);
#ifndef DISABLE_BACKING_STORE
  TheScreen.DoesSaveUnders = DoesSaveUnders(ScreenOfDisplay(disp,
                                                            TheScreen.Screen));
  TheScreen.DoesBackingStore = DoesBackingStore(ScreenOfDisplay(disp,
                                                            TheScreen.Screen));
#else
  TheScreen.DoesSaveUnders = False;
  TheScreen.DoesBackingStore = False;
#endif

  /* Mark Display as close-on-exec (important for restarts and starting other wms) */
  if(fcntl (ConnectionNumber(disp), F_SETFD, 1) == -1)
    SeeYa(1,"Looks like something is wrong with the Display I got");

  TheScreen.root= RootWindow (disp, TheScreen.Screen);
  if(TheScreen.root == None)
    SeeYa(1,"No proper root Window available");

  xswa.override_redirect = True;
  TheScreen.inputwin = XCreateWindow(disp, TheScreen.root, -2, -2, 1, 1, 0, 0,
                                     InputOnly, CopyFromParent,
                                     CWOverrideRedirect, &xswa);
  if(TheScreen.inputwin == None) SeeYa(1,"couldn't initialize keyboard input");
  XMapWindow(disp, TheScreen.inputwin);

/*** The following WM_Sx stuff is requested by icccm 2.0 ***/
  XSelectInput(disp, TheScreen.inputwin, PropertyChangeMask);
  XStoreName(disp, TheScreen.inputwin, ""); /* obtain a time stamp */
  XWindowEvent(disp, TheScreen.inputwin, PropertyChangeMask, &event);
  TheScreen.now = TheScreen.start_tstamp = event.xproperty.time;
  XSelectInput(disp, TheScreen.inputwin, INPUTWIN_EVENTS); /* set the real event mask */

  {
    char WM_Sx[64];
    sprintf(WM_Sx,"WM_S%d",TheScreen.Screen);
    TheScreen.WM_Sx = XInternAtom(disp,WM_Sx,False);
  }
  if(None != (otherwmswin = XGetSelectionOwner(disp, TheScreen.WM_Sx))) {
    printf("UWM: another icccm compliant window manager seems to be running\n");
    if(InitS.icccmFlags & (ICF_TRY_HARD | ICF_HOSTILE)) {
      int a,b;

      printf("UWM: --TryHard selected, overtaking control ");fflush(stdout);
      XSetSelectionOwner(disp, TheScreen.WM_Sx, TheScreen.inputwin,
                         TheScreen.start_tstamp);
      XSelectInput(disp, otherwmswin, StructureNotifyMask);
      for(a=0;a<10;a++){
        printf(".");fflush(stdout);
        if((b=XCheckWindowEvent(disp, otherwmswin, StructureNotifyMask, &event))
           && (event.type == DestroyNotify)
           && (event.xdestroywindow.window == otherwmswin)) break;
        if(a < 9) sleep(1);
      }
      printf("\n");
      if(b == False) {
        printf("UWM: Other WM still active,\n");
        if(InitS.icccmFlags & ICF_HOSTILE) {
          printf("     getting hostile.\n");
          XKillClient(disp, otherwmswin);
        } else {
          SeeYa(1,"start uwm with --Hostile option to try harder to replace the wm\n\
    ATTENTION: replacing another running wm might terminate your X-session,\n\
               only use this option if you really know what you are doing!\n\
UWM ");
        }
      }
    } else {
     SeeYa(1,"start uwm with --TryHard option to try to replace the wm\n\
    ATTENTION: replacing another running wm might terminate your X-session,\n\
               only use this option if you really know what you are doing!\n\
UWM ");
    }
  }
  XSetSelectionOwner(disp, TheScreen.WM_Sx, TheScreen.inputwin,
                     TheScreen.start_tstamp);
  if(TheScreen.inputwin != XGetSelectionOwner(disp, TheScreen.WM_Sx))
    SeeYa(1,"Couldn't Acquire WM_Sx selection");
  event.type = ClientMessage;
  event.xclient.format = 32;
  event.xclient.window = TheScreen.root;
  event.xclient.message_type = XA_RESOURCE_MANAGER;
  event.xclient.data.l[0] = TheScreen.start_tstamp;
  event.xclient.data.l[1] = TheScreen.WM_Sx;
  event.xclient.data.l[2] = TheScreen.inputwin;
  event.xclient.data.l[3] = event.xclient.data.l[4] = 0;
  XSendEvent(disp, TheScreen.root, False, StructureNotifyMask, &event);
/* end of icccm requested WM_Sx selection stuff, see also handlers.c */

  XSetErrorHandler((XErrorHandler)RedirectErrorHandler);   
    /* just to find out if everything goes right with the initialisation... */
  XSetIOErrorHandler((XIOErrorHandler)ArmeageddonHandler);   
    /* In case everything is lost... */
  XSelectInput(disp,TheScreen.root,HANDLED_EVENTS);
    /* in case this succeeds, we're in! */
  XSync(disp,False);   /* Never let an error wait... */
    /* in case we have not stopped yet we're accepted as window manager! */

  XSetErrorHandler((XErrorHandler)UWMErrorHandler);
    /* The real UWM-Error-handler */

  SetupCursors();  /* get the cursors we are going to use */
  InitHandlers();  /* set up the handler jump-tables */

  XDefineCursor(disp,TheScreen.root,TheScreen.Mice[C_DEFAULT]);
  TheScreen.colormap = DefaultColormap(disp,DefaultScreen(disp));

  /*** find out where ude is installed. ***/
  env= getenv("UDEdir");
  if(!env) {
    /* UDE_DIR is a macro that is defined in the Makefile (by automake) and
       it will contain the default ude directory which is the same pkgdatadir
       it will usually be /usr/local/share/ude */
    char *e;
    sprintf(TheScreen.udedir, "%s/", UDE_DIR);
    e = MyCalloc(strlen(TheScreen.udedir) + strlen("UDEdir=") +1, sizeof(char));
    sprintf(e,"UDEdir=%s", TheScreen.udedir);
    putenv(e);     /* errors setting UDEdir are not fatal, so we ignore them. */
  } else {
      char *e;
      
      e= RLSpace(env);
      if('/' != (*(strchr (e, '\0') - sizeof (char))))
        sprintf(TheScreen.udedir,"%s/",e);
      else
        sprintf(TheScreen.udedir,"%s",e);
  }

  TheScreen.cppincpaths = malloc(sizeof(char) * (1 + strlen("-I ")
         + strlen(TheScreen.Home) + strlen("/.ude/config/ -I ") 
         + strlen(TheScreen.udedir) + strlen("config/ -I ./")));
  sprintf(TheScreen.cppincpaths, "-I %s/.ude/config/ -I %sconfig/ -I ./",
          TheScreen.Home, TheScreen.udedir);

  TheScreen.urdbcppopts = Initurdbcppopts();

/*** set up some uwm-specific stuff ***/

  UWMContext = XUniqueContext();   /* Create Context to store UWM-data */
  UWMGroupContext = XUniqueContext();   /* Create Context to store group data */
  TheScreen.MenuContext = XUniqueContext();   
  TheScreen.MenuFrameContext = XUniqueContext();   
    /* Create Context to store menu-data */

  /* Atoms for selections */
  TheScreen.VERSION_ATOM = XInternAtom(disp,"VERSION",False);
  TheScreen.ATOM_PAIR = XInternAtom(disp,"ATOM_PAIR",False);
  TheScreen.TARGETS = XInternAtom(disp,"TARGETS",False);
  TheScreen.MULTIPLE = XInternAtom(disp,"MULTIPLE",False);
  TheScreen.TIMESTAMP = XInternAtom(disp,"TIMESTAMP",False);

  /* window management related Atoms */
  WM_STATE_PROPERTY = XInternAtom(disp,"WM_STATE",False);
  WM_CHANGE_STATE = XInternAtom(disp,"WM_CHANGE_STATE",False);
  WM_TAKE_FOCUS = XInternAtom(disp,"WM_TAKE_FOCUS",False);
  WM_DELETE_WINDOW = XInternAtom(disp,"WM_DELETE_WINDOW",False);
  WM_PROTOCOLS = XInternAtom(disp,"WM_PROTOCOLS",False);
  MOTIF_WM_HINTS = XInternAtom(disp,_XA_MOTIF_WM_HINTS,False);

  if(!(TheScreen.UltimateList=NodeListCreate()))
    SeeYa(1,"FATAL: out of memory!");

  /*** read configuration files ***/

  InitDefaults();   
 
  PrepareIcons();

  /*** prepare menues ***/;

  CreateUWMMenu();
  InitWSProcs();

  /*** set up more UWM-specific stuff ***/
  xgcv.function=GXinvert;
  xgcv.line_style=LineSolid;
  xgcv.line_width=TheScreen.BorderWidth1;
  xgcv.cap_style=CapButt;
  xgcv.subwindow_mode=IncludeInferiors;
  TheScreen.rubbercontext=XCreateGC(disp,TheScreen.root,GCFunction|\
          GCCapStyle|GCLineStyle|GCLineWidth|GCSubwindowMode,&xgcv);
  xgcv.function=GXcopy;
  xgcv.line_style=LineSolid;
  xgcv.line_width=0;
  xgcv.cap_style=CapButt;
  xgcv.foreground=BlackPixel(disp,TheScreen.Screen);
  TheScreen.blackcontext=XCreateGC(disp,TheScreen.root,GCFunction|\
            GCCapStyle|GCLineStyle|GCLineWidth|GCForeground,&xgcv);

  xgcv.function=GXcopy;
  xgcv.foreground=TheScreen.Colors[TheScreen.desktop.ActiveWorkSpace]
                                  [UDE_Light].pixel;
  xgcv.line_width=0;
  xgcv.line_style=LineSolid;
  xgcv.cap_style=CapButt;
  TheScreen.MenuLightGC=XCreateGC(disp, TheScreen.root, GCFunction
                                  | GCForeground | GCCapStyle | GCLineWidth
                                  | GCLineStyle, &xgcv);
  xgcv.function=GXcopy;
  xgcv.foreground=TheScreen.Colors[TheScreen.desktop.ActiveWorkSpace]
                                  [UDE_Shadow].pixel;
  xgcv.line_width=0;
  xgcv.line_style=LineSolid;
  xgcv.cap_style=CapButt;
  TheScreen.MenuShadowGC=XCreateGC(disp, TheScreen.root, GCFunction
                                  | GCForeground | GCCapStyle | GCLineWidth
                                  | GCLineStyle, &xgcv);
  xgcv.function=GXcopy;
  xgcv.foreground=TheScreen.Colors[TheScreen.desktop.ActiveWorkSpace]
                                  [UDE_Back].pixel;
  xgcv.line_width=0;
  xgcv.line_style=LineSolid;
  xgcv.cap_style=CapButt;
  TheScreen.MenuBackGC=XCreateGC(disp, TheScreen.root, GCFunction
                                 | GCForeground | GCCapStyle | GCLineWidth
                                 | GCLineStyle, &xgcv);
  xgcv.function=GXcopy;
  xgcv.foreground=TheScreen.Colors[TheScreen.desktop.ActiveWorkSpace]\
                                             [UDE_StandardText].pixel;
  xgcv.fill_style=FillSolid;
  TheScreen.MenuTextGC=XCreateGC(disp,TheScreen.root, GCFunction
                                 | GCForeground | GCFillStyle, &xgcv);

  // set grabs regardless of state of CapsLock, NumLock and ScrollLock
  get_offending_modifiers(disp);

  mask=capslock_mask|numlock_mask|scrolllock_mask;
  for( i = mask ; ; i = ( i - 1 ) & mask )
  {
    XGrabButton(disp, AnyButton, UWM_MODIFIERS | i, TheScreen.root,
                True, ButtonPressMask | ButtonReleaseMask, GrabModeAsync,
                GrabModeAsync, None, None);
    XGrabKey(disp, XKeysymToKeycode(disp,XK_Right), UWM_MODIFIERS|i,
             TheScreen.root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(disp, XKeysymToKeycode(disp,XK_Left), UWM_MODIFIERS|i,
             TheScreen.root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(disp, XKeysymToKeycode(disp,XK_Up), UWM_MODIFIERS|i,
             TheScreen.root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(disp, XKeysymToKeycode(disp,XK_Down), UWM_MODIFIERS|i,
           TheScreen.root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(disp, XKeysymToKeycode(disp,XK_Page_Down), UWM_MODIFIERS|i,
             TheScreen.root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(disp, XKeysymToKeycode(disp,XK_Page_Up), UWM_MODIFIERS|i,
             TheScreen.root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(disp, XKeysymToKeycode(disp,XK_End), UWM_MODIFIERS|i,
             TheScreen.root, True, GrabModeAsync, GrabModeAsync);
    if(i==0) break;
  }

  /* set up ude stuff (broadcast color and desktop information) */
  TheScreen.UDE_WORKSPACES_PROPERTY = XInternAtom(disp,\
                                              "_UDE_WORKSPACES_PROPERTY",False);
  TheScreen.UDE_SETTINGS_PROPERTY = XInternAtom(disp,"_UDE_SETTINGS_PROPERTY",\
                                                False);
  TheScreen.UDE_WINDOW_PROPERTY = XInternAtom(disp,"_UDE_WINDOW_PROPERTY",\
                                                False);

  BroadcastWorkSpacesInfo();
  UpdateDesktop();

  /* Set the background for the current desktop. */
  SetWSBackground();
}
