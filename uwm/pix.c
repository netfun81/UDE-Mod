/***  PIX.C: uwm background picture loader (jpeg/xpm)  ***/

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
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include "uwm.h"

#ifdef HAVE_LIBJPEG
#include <jpeglib.h>
#endif

#include <X11/xpm.h>
#include <X11/Xlib.h>

extern UDEScreen TheScreen;
extern Display *disp;

#ifdef HAVE_LIBJPEG
/* replacement for libjpegs (hardly working and unacceptable) error management */
struct jerrstruct {struct jpeg_error_mgr mgr; jmp_buf jb;} jerr;
void my_jpeg_error_handler(j_common_ptr dobj, int level)
{ /*if(level < 0) fprintf(stderr, "error in jpeg file.\n");*/}
void my_jpeg_error_exit_handler(j_common_ptr dobj)
{ my_jpeg_error_handler(dobj,-1);
  longjmp(((struct jerrstruct *)((struct jpeg_decompress_struct *)dobj)->err)->jb, 1);}

/********************************************************************************
 *
 *  LoadJpeg - load a jpeg file into an X11 pixmap.
 *    + disp: display to create the pixmap on and to get default values from
 *    + draw: drawable used for XCreatePixmap.
 *    + filename: name of the jpeg file.
 *    + xpa: pointer to an XpmAttributes structure in which LoadJpeg returns
 *           the dimensions, depth, allocated colors, visual and colormap of the
 *           returned pixmap.
 *           If the corresponding bits are set in xpa->valuemask LoadJpeg regards
 *           the structure's visual and colormap elements.
 *
 ********************************************************************************/

Pixmap LoadJpeg(Display *disp, Drawable draw, char *filename, XpmAttributes *xpa)
{
  struct jpeg_decompress_struct dobj;
  struct jerrstruct jerr;
  FILE *jpg_file = NULL;
  Pixmap ret = None;
  XGCValues xgcv;
  GC gc;
  JSAMPROW *rows = NULL;
  XColor col;
  int b;
  unsigned long d;
/* we need these to be static to make sure they won't be affected by longjmp */
  static XImage *image = NULL;
  static JSAMPLE *data = NULL;
  static Pixel *idata = NULL;
  static int  c = 0;

  if(!(jpg_file = fopen(filename,"rb"))) return(None);
  xpa->pixels = NULL;

  if(setjmp(jerr.jb)) {
/*    printf("\bprocessing of jpeg file %s aborted!\n",filename);*/
    jpeg_destroy_decompress(&dobj);
    if(data)           free(data);
    if(jpg_file)       fclose(jpg_file);
    if(xpa->pixels)  { XFreeColors(disp, xpa->colormap, xpa->pixels, c, 0);
                       free(xpa->pixels);}
    if(idata)          free(idata);
    if(image)        { image->data = NULL;
                       XDestroyImage(image);}
    return(None);
  }

  dobj.err = jpeg_std_error(&jerr.mgr);
  jerr.mgr.error_exit = my_jpeg_error_exit_handler;
  jerr.mgr.emit_message = my_jpeg_error_handler;
  jpeg_create_decompress(&dobj);
  jpeg_stdio_src(&dobj, jpg_file);
  jpeg_read_header(&dobj, TRUE);

  dobj.out_color_space = JCS_RGB;
  dobj.quantize_colors = TRUE;

  jpeg_start_decompress(&dobj);

  xpa->width = dobj.output_width;
  xpa->height = dobj.output_height;
  if(!(xpa->valuemask & XpmVisual))
    xpa->visual = DefaultVisual(disp,DefaultScreen(disp));
  xpa->depth = DefaultDepth(disp,DefaultScreen(disp));
  if(!(xpa->valuemask & XpmColormap))
    xpa->colormap = DefaultColormap(disp,DefaultScreen(disp));
  xpa->pixels = NULL;

  if((!(data = calloc(dobj.output_components * dobj.output_width 
               * dobj.output_height, sizeof(JSAMPLE)))) 
     || (!(rows = calloc(dobj.output_height, sizeof(JSAMPROW))))) {
    longjmp(jerr.jb, 1);
  }

  for(b=0;b<dobj.output_height;b++) rows[b] = data + b * dobj.output_components 
                                              * dobj.output_width * sizeof(JSAMPLE);

  while(dobj.output_scanline < dobj.output_height)
    if(!jpeg_read_scanlines(&dobj, &rows[dobj.output_scanline], dobj.output_height)){
      longjmp(jerr.jb, 1);
    }

  free(rows); rows = NULL;

  xpa->npixels = dobj.actual_number_of_colors;

  if((!(xpa->pixels = calloc(dobj.actual_number_of_colors, sizeof(*(xpa->pixels))))) 
     || (!(idata = calloc(dobj.output_components * dobj.output_width 
                   * dobj.output_height, sizeof(Pixel))))
     || (!(image = XCreateImage(disp, xpa->visual, xpa->depth, ZPixmap, 0, (char *)idata,
                   dobj.output_width, dobj.output_height, 32,
                   dobj.output_width * sizeof(Pixel))))){
    longjmp(jerr.jb, 1);
  }

#ifdef WORDS_BIGENDIAN
  image->byte_order = MSBFirst;
#else
  image->byte_order = LSBFirst;
#endif
  image->bits_per_pixel = 8 * sizeof(unsigned long);

/*  _XInitImageFuncPtrs(image);  *** Not quite sure if this is needed, works fine ***/
   /*** without it on my system and the function is to be considered private to Xlib ***/

  for(c=0; c < dobj.actual_number_of_colors; c++) {
    col.red = dobj.colormap[0][c] << (8 * sizeof(col.red) - BITS_IN_JSAMPLE);
    col.green = dobj.colormap[1][c] << (8 * sizeof(col.green) - BITS_IN_JSAMPLE);
    col.blue = dobj.colormap[2][c] << (8 * sizeof(col.blue) - BITS_IN_JSAMPLE);
    col.flags = DoRed | DoGreen | DoBlue;
    if(!XAllocColor(disp, xpa->colormap, &col)) {
      longjmp(jerr.jb, 1);
    }
    xpa->pixels[c] = col.pixel;
  }

  for(d=0; d < (dobj.output_components * dobj.output_width * dobj.output_height); d++)
    idata[d] = xpa->pixels[data[d]];
  
  jpeg_finish_decompress(&dobj);
  fclose(jpg_file); jpg_file = NULL;
  jpeg_destroy_decompress(&dobj);
  free(data);

  if(None == (ret = XCreatePixmap(disp, draw, dobj.output_width,
                                  dobj.output_height, image->depth))){
    free(idata);
    XFreeColors(disp, xpa->colormap, xpa->pixels, xpa->npixels, 0);
    free(xpa->pixels);
    XDestroyImage(image);
    return(None);
  }

  xgcv.function = GXcopy;
  gc = XCreateGC(disp, ret, GCFunction, &xgcv);
  XPutImage(disp, ret, gc, image, 0, 0, 0, 0, image->width, image->height);
  XFreeGC(disp, gc);

  free(image->data);                  /*** don't forget: image->date == idata!!! ***/
  image->data = NULL;
  XDestroyImage(image);

  xpa->alloc_pixels = xpa->pixels;
  xpa->nalloc_pixels = xpa->npixels;
  xpa->valuemask = XpmVisual | XpmColormap | XpmDepth | XpmSize | XpmColorTable 
                   | XpmReturnColorTable | XpmReturnAllocPixels;
  return(ret);
}

#endif   /* HAVE_LIBJPEG */

/******************************************************************************
 *
 *  LoadPic - determines file type of a given file and loads jpeg/xpm image.
 *     returns 0 on error, 1 on success.
 *
 ******************************************************************************/


int LoadPic(char *filename, Pixmap *pm, XpmAttributes *xa)
{
  Pixmap shape;

#ifdef HAVE_LIBJPEG
  xa->valuemask=XpmColormap|XpmReturnAllocPixels;
  xa->colormap=TheScreen.colormap;
  if(None == (*pm = LoadJpeg(disp, TheScreen.root, filename, xa))) {
#endif
    xa->valuemask=XpmColormap|XpmReturnAllocPixels;
    xa->colormap=TheScreen.colormap;
    if(XpmReadFileToPixmap(disp,TheScreen.root, filename, pm, &shape, xa)){
      *pm = None;
      return(0);
    }
    if(shape != None) XFreePixmap(disp,shape);
#ifdef HAVE_LIBJPEG
  }
#endif
  return(1);
}
