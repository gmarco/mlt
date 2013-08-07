/*
 *  filter_stabilize.c
 *
 *  Copyright (C) Georg Martius - June 2007
 *   georg dot martius at web dot de  
 *
 *  This file is part of transcode, a video stream processing tool
 *      
 *  transcode is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  transcode is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

/* Typical call:
 *  transcode -V -J stabilize=shakiness=5:show=1,preview 
 *         -i inp.mpeg -y null,null -o dummy
 *  all parameters are optional
*/

#define MOD_NAME    "filter_stabilize.so"
#define MOD_VERSION "v0.75 (2010-04-07)"
#define MOD_CAP     "extracts relative transformations of \n\
    subsequent frames (used for stabilization together with the\n\
    transform filter in a second pass)"
#define MOD_AUTHOR  "Georg Martius"

/* Ideas:
 - Try OpenCL/Cuda, this should work great
 - use smoothing on the frames and then use gradient decent!
 - stepsize could be adapted (maybe to check only one field with large
   stepsize and use the maximally required for the other fields
*/

#define MOD_FEATURES \
    TC_MODULE_FEATURE_FILTER|TC_MODULE_FEATURE_VIDEO
#define MOD_FLAGS  \
    TC_MODULE_FLAG_RECONFIGURABLE | TC_MODULE_FLAG_DELAY
  

//#include "transform.h"

#include "vid.stab/src/motiondetect.h"
#include <math.h>
#include <libgen.h>
#include <stdio.h>
#include "tlist.h"
#include <framework/mlt_types.h>

// structure that contains the contrast and the index of a field
typedef struct _contrast_idx {
    double contrast;
    int index;
} contrast_idx;

/* private date structure of this filter*/
typedef struct _stab_data {
		MotionDetect md;
    tlist* transs;
		unsigned char* grayimage;
		int t;
		mlt_image_format pixelformat;
} StabData;


void addTrans(StabData* sd, Transform sl);

int stabilize_configure(StabData* instance);
int stabilize_stop(StabData* instance);

int stabilize_filter_video(StabData* instance, unsigned char *frame,mlt_image_format imageformat);


