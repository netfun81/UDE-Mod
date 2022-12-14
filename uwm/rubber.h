#ifndef UWM_RUBBER_H
#define UWM_RUBBER_H

void StartRubber(unsigned int lx,unsigned int ly,unsigned int lwidth,\
                       unsigned int lheight,unsigned int BorderWidth);
void SqueezeRubber(unsigned int lx,unsigned int ly,unsigned int lwidth,\
                                                  unsigned int lheight);
void StopRubber(int *lx,int *ly,int *lwidth, int *lheight);

#endif
