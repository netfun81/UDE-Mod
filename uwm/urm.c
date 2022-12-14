/***  URM.C: (ude resource management) Routines to handle and set
             Xresources.  ***/

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
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/Xmu/SysUtil.h>

#include "uwm.h"
#include "init.h"
#include "special.h"
#include "urm.h"

extern Display *disp;
extern UDEScreen TheScreen;

#define RootOfDisplay(DISP) RootWindow(DISP, DefaultScreen(DISP))

#define CHARBUFSIZE 256
char bufbuf[CHARBUFSIZE], *buffer=NULL;
int bufpos=0;

char *FlushBufBuf()
{
  bufbuf[bufpos] = '\0';
  if(buffer){
    if(!(buffer = realloc(buffer,
                          sizeof(char) * (strlen(buffer) + bufpos + 1))))
      return(NULL);
    memcpy(buffer + sizeof(char) * strlen(buffer), bufbuf,
           sizeof(char) * (bufpos+1));
  } else {
    if(!(buffer = malloc(sizeof(char) * (bufpos+1)))) return(NULL);
    memcpy(buffer, bufbuf, sizeof(char) * (bufpos+1));
  }
  bufpos = 0;
  return(buffer);
}

void Add2CharBuf(char c)
{
  if(bufpos == (CHARBUFSIZE-1)) {
    if(!FlushBufBuf()) SeeYa(-1,"out of mem (urdb initialisation)");
  }
  bufbuf[bufpos] = c;
  bufpos++;
}

void AddStrings2CharBuf(char **strings)
{
  int a;

  for(a=0; strings[a]; a++) {
    int b;
    for(b=0; strings[a][b]!='\0'; b++)
      Add2CharBuf(strings[a][b]);
  }
}

char *GetCharBuf()
{
  char *p, *q, *r, *s;
  if(!(r=FlushBufBuf())) SeeYa(-1,"out of mem (urdb initialisation)");
  buffer=NULL;

  p=r;
  while(isspace(*p)&&(*p!='\0')) p++; /* remove leading space */
  q = p + sizeof(char) * strlen(p);
  while(isspace(*q)) q--;            /* remove trailing space */
  *q='\0';
  s = malloc(sizeof(char) * (strlen(p) + 1));
  strcpy(s,p);
  free(r);
  return(s);
}

void AddSimpleDef(char *name)
{
  char *strings[3];
  strings[0] = " -D";
  strings[1] = name;
  strings[2] = NULL;
  AddStrings2CharBuf(strings);
}
void AddDef(char *name, char *value)
{
  char *strings[5];
  strings[0] = " -D";
  strings[1] = name;
  strings[2] = "=";
  strings[3] = value;
  strings[4] = NULL;
  AddStrings2CharBuf(strings);
}
void AddDefQ(char *name, char *value)
{
  char *strings[7];
  strings[0] = " -D";
  strings[1] = name;
  strings[2] = "=";
  strings[3] = "\"";
  strings[4] = value;
  strings[5] = "\"";
  strings[6] = NULL;
  AddStrings2CharBuf(strings);
}
void AddNum(char *name, int num)
{
  char buffer[256];
  sprintf(buffer,"%d",num);
  AddDef(name,buffer);
}
void AddDefTok(char *name, char *value)
{
  char *strings[5], *p;
  strings[0] = " -D";
  strings[1] = name;
  strings[2] = "_";
  strings[3] = value;
  strings[4] = NULL;
  for(p=strings[3]; *p!='\0'; p++)
    if((!isalpha(*p)) && (!isdigit(*p)) && (*p != '_'))
      *p='_';
  AddStrings2CharBuf(strings);
}


/* parts of the following functions have been taken out of
 * =======================================================
 * xrdb - X resource manager database utility
 *
 * $XConsortium: xrdb.c,v 11.76 95/05/12 18:36:46 mor Exp $
 * $XFree86: xc/programs/xrdb/xrdb.c,v 3.7.2.3 1998/10/22 04:31:12 hohndel Exp $
 *                        COPYRIGHT 1987, 1991
 *                 DIGITAL EQUIPMENT CORPORATION
 *                     MAYNARD, MASSACHUSETTS
 *                 MASSACHUSETTS INSTITUTE OF TECHNOLOGY
 *                     CAMBRIDGE, MASSACHUSETTS
 *                      ALL RIGHTS RESERVED.
 */

int Resolution(pixels, mm)
    int pixels, mm;
{
    return ((pixels * 100000 / mm) + 50) / 100;
}

char *ClassNames[] = {
    "StaticGray",
    "GrayScale",
    "StaticColor",
    "PseudoColor",
    "TrueColor",
    "DirectColor"
};

char *Initurdbcppopts()
{
#define MAXHOSTNAME 255
  char client[MAXHOSTNAME], server[MAXHOSTNAME], *colon;
  char **extnames, *strings[2];
  int n;
/***/
  Screen *screen;
  Visual *visual;
  XVisualInfo vinfo, *vinfos;
  int nv, i, j;
  char name[50], host[256];
  
  strings[0]=TheScreen.cppincpaths;
  strings[1]=NULL;
  AddStrings2CharBuf(strings);
  
  host[0]='\0';
  XmuGetHostname(client, MAXHOSTNAME);
  strcpy(server, XDisplayName(host));
  colon = strchr(server, ':');
  n = 0;
  if (colon) {
      *colon++ = '\0';
      if (*colon == ':')
          colon++;
      sscanf(colon, "%d", &n);
  }
  if (!*server || !strcmp(server, "unix") || !strcmp(server, "localhost"))
      strcpy(server, client);

  AddDef("HOST", server);
  AddDef("SERVERHOST", server);
  AddDefTok("SRVR", server);
  AddNum("DISPLAY_NUM", n);
  AddDef("CLIENTHOST", client);
  AddDefTok("CLNT", client);
  AddNum("VERSION", ProtocolVersion(disp));
  AddNum("REVISION", ProtocolRevision(disp));
  AddDefQ("VENDOR", ServerVendor(disp));
  AddDefTok("VNDR", ServerVendor(disp));
  AddNum("RELEASE", VendorRelease(disp));
  AddNum("NUM_SCREENS", ScreenCount(disp));
  extnames = XListExtensions(disp, &n);
  while(--n >= 0)
    AddDefTok("EXT", extnames[n]);
  XFree(extnames);
/***/
  screen = ScreenOfDisplay(disp, TheScreen.Screen);
  visual = DefaultVisualOfScreen(screen);
  vinfo.screen = TheScreen.Screen;
  vinfos = XGetVisualInfo(disp, VisualScreenMask, &vinfo, &nv);
  AddNum("SCREEN_NUM", TheScreen.Screen);
  AddNum("WIDTH", screen->width);
  AddNum("HEIGHT", screen->height);
  AddNum("X_RESOLUTION", Resolution(screen->width,screen->mwidth));
  AddNum("Y_RESOLUTION", Resolution(screen->height,screen->mheight));
  AddNum("PLANES", DisplayPlanes(disp, TheScreen.Screen));
  AddNum("BITS_PER_RGB", visual->bits_per_rgb);
  AddDef("CLASS", ClassNames[visual->class]);
  sprintf(name, "CLASS_%s", ClassNames[visual->class]);
  AddNum(name, (int)visual->visualid);
  switch(visual->class) {
    case StaticColor:
    case PseudoColor:
    case TrueColor:
    case DirectColor:
        AddSimpleDef("COLOR");
        break;
  }
  for(i = 0; i < nv; i++) {
    for(j = i; --j >= 0; ) {
      if(vinfos[j].class == vinfos[i].class &&
         vinfos[j].depth == vinfos[i].depth)
        break;
    }
    if(j < 0) {
      sprintf(name, "CLASS_%s_%d",
              ClassNames[vinfos[i].class], vinfos[i].depth);
      AddNum(name, (int)vinfos[i].visualid);
    }
  }
  XFree((char *)vinfos);
  return(GetCharBuf());
}

const char *xtranames[MAXEXTRAS] = { "@BACKGROUND@",
                                     "@LIGHTCOLOR@",
                                     "@SHADOWCOLOR@",
                                     "@STANDARDTEXT@",
                                     "@INACTIVETEXT@",
                                     "@HIGHLIGHTEDTEXT@",
                                     "@HIGHLIGHTEDBGR@",
                                     "@TEXTCOLOR@",
                                     "@TEXTBGR@",
                                     "@BEVELWIDTH@",
                                     "@FLAGS@",
                                     "@STANDARDFONT@",
                                     "@INACTIVEFONT@",
                                     "@HIGHLIGHTFONT@",
                                     "@TEXTFONT@",
                                     "@STANDARDFONTSET@",
                                     "@INACTIVEFONTSET@",
                                     "@HIGHLIGHTFONTSET@",
                                     "@TEXTFONTSET@"};
char *uderesources[MAXEXTRAS] = {"ude.background",
                                 "ude.lightcolor",
                                 "ude.shadowcolor",
                                 "ude.standardtext",
                                 "ude.inactivetext",
                                 "ude.highlightedtext",
                                 "ude.highlightedbgr",
                                 "ude.textcolor",
                                 "ude.textbgr",
                                 "ude.bevelwidth",
                                 "ude.flags",
                                 "ude.standardfont",
                                 "ude.inactivefont",
                                 "ude.highlightfont",
                                 "ude.textfont",
                                 "ude.standardfontset",
                                 "ude.inactivefontset",
                                 "ude.highlightfontset",
                                 "ude.textfontset"};

static UDEXrdbEntry *ResourceDB = NULL;

int ReadResourceDBFromFile(char *filename)
{
  FILE *file;
  UDEXrdbEntry *resource = ResourceDB;
  int a,s;
  char *resourcename;

  if(!resource) {
    for(a=0;a<MAXEXTRAS;a++) {
      if(resource){
        resource->next = malloc(sizeof(UDEXrdbEntry));
        resource = resource->next;
      } else {
        ResourceDB = resource = malloc(sizeof(UDEXrdbEntry));
      }
      if(!resource) {
        SeeYa(-1,"FATAL: out of Memory!(reading urdb)");
      }
      resource->name = uderesources[a];
      resource->entry = NULL;
      resource->xtra = a;
      resource->next = NULL;
    }
  }
  
  if(!(file = MyOpen(filename, TheScreen.urdbcppopts))) return(0);

  resourcename =NULL;

  while(EOF!=(s=fgetc(file))) {
    char *data;
    int u;
    switch(s) {
      case '#':  if(resourcename) {
                    Add2CharBuf(s);
                    break;
                 }
      case '%':  /*** remove comments and preprocessor lines ***/
      case '!':  while('\n'!=(u=fgetc(file))); break; /* remove rest of line */
      case '\n': if(!resourcename) break;
                 resource = ResourceDB;
                 while(resource && strcmp(resource->name,resourcename))
                   resource = resource->next;
                 if(resource) {
                   free(resourcename);
                 } else {
                   resource = malloc(sizeof(UDEXrdbEntry));
                   resource->next = ResourceDB;
                   ResourceDB = resource;
                   resource->name = resourcename;
                   resource->entry = NULL;
                 }
                 resource->xtra = -1;
                 if(resource->entry) free(resource->entry);
                 data = GetCharBuf();
                 for(a=0;a<MAXEXTRAS;a++)
                   if(strstr(data,xtranames[a])) resource->xtra = a;
                 if(resource->xtra == -1) resource->entry = data;
                 else {
                   free(data);
                   resource->entry = NULL;
                 }
                 resourcename = NULL;
                 break;
      case ':':  if(!resourcename) resourcename = GetCharBuf(); 
                 else Add2CharBuf(':');
                 break;
      case '\\': switch(u = fgetc(file)){
                   case '\\': Add2CharBuf('\\');
                              break;
                   case '\n': break;
                   default:   Add2CharBuf('\\');
                              Add2CharBuf(u);
                              break;
                 }
                 break;
      default: Add2CharBuf(s);
    }
  }

  fclose(file);

  return(-1);
}

void SetResourceDB()
{
  UDEXrdbEntry *resource = ResourceDB;
  unsigned long size = 1;
  char *string1, *string2;
  char *xtras[MAXEXTRAS];
  char nums[MAXNUMS][16];
  XTextProperty prop;
  int a;

  if(!resource) return;

#define COLOR(I) TheScreen.Colors[TheScreen.desktop.ActiveWorkSpace][I].red,\
                 TheScreen.Colors[TheScreen.desktop.ActiveWorkSpace][I].green,\
                 TheScreen.Colors[TheScreen.desktop.ActiveWorkSpace][I].blue

  sprintf(nums[BACKGROUND],"#%.4X%.4X%.4X",COLOR(UDE_Back));
  sprintf(nums[LIGHTCOLOR],"#%.4X%.4X%.4X",COLOR(UDE_Light));
  sprintf(nums[SHADOWCOLOR],"#%.4X%.4X%.4X",COLOR(UDE_Shadow));
  sprintf(nums[STANDARDTEXT],"#%.4X%.4X%.4X",COLOR(UDE_StandardText));
  sprintf(nums[INACTIVETEXT],"#%.4X%.4X%.4X",COLOR(UDE_InactiveText));
  sprintf(nums[HIGHLIGHTEDTEXT],"#%.4X%.4X%.4X",COLOR(UDE_HighlightedText));
  sprintf(nums[HIGHLIGHTEDBGR],"#%.4X%.4X%.4X",COLOR(UDE_HighlightedBgr));
  sprintf(nums[TEXTCOLOR],"#%.4X%.4X%.4X",COLOR(UDE_TextColor));
  sprintf(nums[TEXTBGR],"#%.4X%.4X%.4X",COLOR(UDE_TextBgr));
  sprintf(nums[BEVELWIDTH],"%d",TheScreen.desktop.BevelWidth);
  sprintf(nums[FLAGS],"%X",TheScreen.desktop.flags);
  for(a = 0; a < MAXNUMS; a++) {
    xtras[a] = nums[a];
  }
  xtras[STANDARDFONT]     = TheScreen.desktop.StandardFont;
  xtras[INACTIVEFONT]     = TheScreen.desktop.InactiveFont;
  xtras[HIGHLIGHTFONT]    = TheScreen.desktop.HighlightFont;
  xtras[TEXTFONT]         = TheScreen.desktop.TextFont;
  xtras[STANDARDFONTSET]  = TheScreen.desktop.StandardFontSet;
  xtras[INACTIVEFONTSET]  = TheScreen.desktop.InactiveFontSet;
  xtras[HIGHLIGHTFONTSET] = TheScreen.desktop.HighlightFontSet;
  xtras[TEXTFONTSET]      = TheScreen.desktop.TextFontSet;

#undef COLOR

  while(resource) {
    size += strlen(resource->name) + 3
            + ((resource->xtra==-1)
              ? (resource->entry ? strlen(resource->entry) : 0)
              : strlen(xtras[resource->xtra]));
    resource = resource->next;
  }

  size *= sizeof(char);
  string1 = malloc(size);
  string2 = malloc(size);
  if((string1 == NULL)||(string2 == NULL)) {
    /*** fatal error ***/
    SeeYa(-1,"FATAL: out of mem(setting resources)\n");
  }
  
  resource = ResourceDB;
  string1[0] = '\0';
  while(resource) {
    memcpy(string2, string1, size);
    if(resource->xtra == -1) {
      if(resource->entry)
        sprintf(string1, "%s%s: %s\n", string2, resource->name,
                resource->entry);
      else
        sprintf(string1, "%s%s: \n", string2, resource->name);
    } else {
      sprintf(string1, "%s%s: %s\n", string2, resource->name,
              xtras[resource->xtra]);
    }
    resource = resource->next;
  }
  free(string2);

  if(!XStringListToTextProperty(&string1, 1, &prop)) {
    SeeYa(-1,"out of mem\n");
  }
  free(string1);
  XSetTextProperty(disp, RootOfDisplay(disp), &prop, XA_RESOURCE_MANAGER);
  XFree(prop.value);
}
