/*
 *  filter_stabilize.c
 *
 *  Copyright (C) Georg Martius - June 2007
 *   georg dot martius at web dot de
 *  mlt adaption by Marco Gittler marco at gitma dot de 2011
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
#include "stabilize.h"
#include "vid.stab/src/localmotion2transform.h"
#include <stdlib.h>
#include <string.h>
#include <framework/mlt_types.h>
#include <framework/mlt_log.h>
#define MAX(a,b)         ((a < b) ?  (b) : (a))
#define MIN(a,b)         ((a < b) ?  (a) : (b))


/*************************************************************************/

/* Module interface routines and data. */

/*************************************************************************/
void addTrans(StabData* sd, Transform sl)
{
	    if (!sd->transs) {
		       sd->transs = tlist_new(0);
		    }
			tlist_append(sd->transs, &sl,sizeof(Transform) );
}
		
/*
 * stabilize_configure:  Configure this instance of the module.  See
 * tcmodule-data.h for function details.
 */
int stabilize_configure(StabData* instance
/*const char *options,
                               int pixelfmt */
                               /*TCModuleExtraData *xdata[]*/)
{
    MotionDetect *sd = &(instance->md);

    sd->hasSeenOneFrame = 0;
    instance->transs = 0;
    sd->allowMax   = 0;
    sd->fieldSize  = MIN(sd->fi.width, sd->fi.height)/12;
    //TODO sd->maxAngleVariation = 1;

    sd->shakiness = MIN(10,MAX(1,sd->shakiness));
    sd->accuracy  = MAX(sd->shakiness,MIN(15,MAX(1,sd->accuracy)));
    if (1) {
        mlt_log_debug (NULL, "Image Stabilization Settings:\n");
        mlt_log_debug (NULL, "     shakiness = %d\n", sd->shakiness);
        mlt_log_debug (NULL, "      accuracy = %d\n", sd->accuracy);
        mlt_log_debug (NULL, "      stepsize = %d\n", sd->stepSize);
        mlt_log_debug (NULL, "          algo = %d\n", sd->algo);
        mlt_log_debug (NULL, "   mincontrast = %f\n", sd->contrastThreshold);
        mlt_log_debug (NULL, "          show = %d\n", sd->show);
    }

#ifndef USE_SSE2
	mlt_log_info(NULL,"No SSE2 support enabled, this will slow down a lot\n");
#endif
    // shift and size: shakiness 1: height/40; 10: height/4
    sd->maxShift    = MIN(sd->fi.width, sd->fi.height)*sd->shakiness/40;
    sd->fieldSize   = MIN(sd->fi.width, sd->fi.height)*sd->shakiness/40;

    mlt_log_debug ( NULL,  "Fieldsize: %i, Maximal translation: %i pixel\n",
                sd->fieldSize, sd->maxShift);
    if (sd->algo==1) {
        // initialize measurement fields. field_num is set here.
        if (!initFields(sd)) {
            return -1;
        }
        sd->maxFields = (sd->accuracy) * sd->fieldNum / 15;
        mlt_log_debug ( NULL, "Number of used measurement fields: %i out of %i\n",
                    sd->maxFields, sd->fieldNum);
    }


    return 0;
}


/**
 * stabilize_filter_video: performs the analysis of subsequent frames
 * See tcmodule-data.h for function details.
 */

int stabilize_filter_video(StabData* instance,
                                  unsigned char *frame,mlt_image_format pixelformat)
{
    MotionDetect *md = &(instance->md);
		Transform t;
		LocalMotions localmotions;
		VSFrame vsFrame;
		fillFrameFromBuffer(&vsFrame,frame,&md->fi);
		md->modName="test";
		if (motionDetection(md,&localmotions,&vsFrame)!=VS_OK){
			mlt_log_info(NULL,"error in MotionDetect");
			return -1;
		}
		TransformData td;
		//td.maxAngle=20;
		//td.maxAngleVariation=.2;
		td.modName="videostab2 stabilize";
		t=simpleMotionsToTransform(&td,&localmotions);
		addTrans(instance,t);
		instance->t++;
    return 0;
}

/**
 * stabilize_stop:  Reset this instance of the module.  See tcmodule-data.h
 * for function details.
 */

int stabilize_stop(StabData* instance)
{
    MotionDetect *md = &(instance->md);
		cleanupMotionDetection(md);
    return 0;
}



