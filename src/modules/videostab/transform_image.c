/*
 *  filter_transform.c
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
 * Typical call:
 * transcode -J transform -i inp.mpeg -y xdiv,tcaud inp_stab.avi
 */
//#include "vid.stab/src/transform.h"
#include "vid.stab/src/transformfixedpoint.h"
#include "transform_image.h"
#include <string.h>
#include <stdlib.h>
#include <framework/mlt_log.h>
#include <framework/mlt_types.h>
/**
 * transform_configure:  Configure this instance of the module.  See
 * tcmodule-data.h for function details.
 */
int transform_configure(TransformData *self,int width,int height, mlt_image_format  pixelformat, unsigned char* image ,Transformations* tx,int trans_len,int framesize)
{

    TransformData* td = self;

    /**** Initialise private data structure */
	PixelFormat pix=PF_NONE;
	if (pixelformat==mlt_image_rgb24)
		pix=PF_RGB24;
	if (pixelformat==mlt_image_yuv420p)
		pix=PF_YUV420P;
    VSFrameInfo* fi_src=&(td->fiSrc);
    VSFrameInfo* fi_dest=&(td->fiDest);
	initFrameInfo(fi_src,width,height,pix);
	initFrameInfo(fi_dest,width,height,pix);
	tx->len=trans_len;
		/*if (initTransformData(td,&fi_src,&fi_dest,"videostab2")!=DS_OK){
			mlt_log_debug(NULL,"init of Transform Data failes");
			return 1;
		}*/
			
    if(configureTransformData(td)!= VS_OK){
        mlt_log_debug(NULL, "configuration of Tranform failed");
        return 1;
    }
    
	if (1) {
        mlt_log_debug(NULL, "Image Transformation/Stabilization Settings:\n");
        mlt_log_debug(NULL, "    smoothing = %d\n", td->smoothing);
        mlt_log_debug(NULL, "    maxshift  = %d\n", td->maxShift);
        mlt_log_debug(NULL, "    maxangle  = %f\n", td->maxAngle);
        mlt_log_debug(NULL, "    crop      = %s\n",
                        td->crop ? "Black" : "Keep");
        mlt_log_debug(NULL, "    relative  = %s\n",
                    td->relative ? "True": "False");
        mlt_log_debug(NULL, "    invert    = %s\n",
                    td->invert ? "True" : "False");
        mlt_log_debug(NULL, "    zoom      = %f\n", td->zoom);
        mlt_log_debug(NULL, "    optzoom   = %s\n",
                    td->optZoom ? "On" : "Off");
        mlt_log_debug(NULL, "    interpol  = %s\n",
                    interpolTypes[td->interpolType]);
        mlt_log_debug(NULL, "    sharpen   = %f\n", td->sharpen);
    }
    
    if (preprocessTransforms(td, tx)!= 0 ) {

        mlt_log_debug(NULL, "error while preprocessing transforms!");
        return 1;            
    }  

    // sharpen is still in transcode...
    /* Is this the right point to add the filter? Seems to be the case.*/
    if(td->sharpen>0){
        /* load unsharp filter */
        char unsharp_param[256];
        sprintf(unsharp_param,"luma=%f:%s:chroma=%f:%s", 
                td->sharpen, "luma_matrix=5x5", 
                td->sharpen/2, "chroma_matrix=5x5");
    }
    
    return 0;
}


int transform_filter_video(TransformData *self,Transformations* trans,
		                                          unsigned char *frame,mlt_image_format format,int framesize){
	
	TransformData *td = self;
	VSFrame vsFrame;
	fillFrameFromBuffer(&vsFrame,frame,&td->fiDest);

	transformPrepare(td,&vsFrame,&vsFrame);
	memset(frame,0,10000);
	if (format == mlt_image_rgb24 ) {
		transformRGB(td, getNextTransform(td, trans));
	} else if (format == mlt_image_yuv420p) {
		transformYUV(td, getNextTransform(td, trans));
	} else {
		mlt_log_warning(NULL, "unsupported Codec: %i\n",format );
		return 1;
	}
	transformFinish(td);
	return 0;
}

#if 0 

/**
 * preprocess_transforms: does smoothing, relative to absolute conversion,
 *  and cropping of too large transforms.
 *  This is actually the core algorithm for canceling the jiggle in the
 *  movie. We perform a low-pass filter in terms of transformation size.
 *  This enables still camera movement, but in a smooth fasion.
 *
 * Parameters:
 *            td: tranform private data structure
 * Return value:
 *     1 for success and 0 for failture
 * Preconditions:
 *     None
 * Side effects:
 *     td->trans will be modified
 */
int preprocess_transforms(TransformData* td)
{
    Transform* ts = td->trans;
    int i;

    if (td->trans_len < 1)
        return 0;
    if (0) {
        mlt_log_debug(NULL,"Preprocess transforms:");
    }
    if (td->smoothing>0) {
        /* smoothing */
        Transform* ts2 = malloc(sizeof(Transform) * td->trans_len);
        memcpy(ts2, ts, sizeof(Transform) * td->trans_len);

        /*  we will do a sliding average with minimal update
         *   \hat x_{n/2} = x_1+x_2 + .. + x_n
         *   \hat x_{n/2+1} = x_2+x_3 + .. + x_{n+1} = x_{n/2} - x_1 + x_{n+1}
         *   avg = \hat x / n
         */
        int s = td->smoothing * 2 + 1;
        Transform null = null_transform();
        /* avg is the average over [-smoothing, smoothing] transforms
           around the current point */
        Transform avg;
        /* avg2 is a sliding average over the filtered signal! (only to past)
         *  with smoothing * 10 horizont to kill offsets */
        Transform avg2 = null_transform();
        double tau = 1.0/(3 * s);
        /* initialise sliding sum with hypothetic sum centered around
         * -1st element. We have two choices:
         * a) assume the camera is not moving at the beginning
         * b) assume that the camera moves and we use the first transforms
         */
        Transform s_sum = null;
        for (i = 0; i < td->smoothing; i++){
            s_sum = add_transforms(&s_sum, i < td->trans_len ? &ts2[i]:&null);
        }
        mult_transform(&s_sum, 2); // choice b (comment out for choice a)

        for (i = 0; i < td->trans_len; i++) {
            Transform* old = ((i - td->smoothing - 1) < 0)
                ? &null : &ts2[(i - td->smoothing - 1)];
            Transform* new = ((i + td->smoothing) >= td->trans_len)
                ? &null : &ts2[(i + td->smoothing)];
            s_sum = sub_transforms(&s_sum, old);
            s_sum = add_transforms(&s_sum, new);

            avg = mult_transform(&s_sum, 1.0/s);

            /* lowpass filter:
             * meaning high frequency must be transformed away
             */
            ts[i] = sub_transforms(&ts2[i], &avg);
            /* kill accumulating offset in the filtered signal*/
            avg2 = add_transforms_(mult_transform(&avg2, 1 - tau),
                                   mult_transform(&ts[i], tau));
            ts[i] = sub_transforms(&ts[i], &avg2);

            if (0 /*verbose*/ ) {
                mlt_log_warning(NULL,"s_sum: %5lf %5lf %5lf, ts: %5lf, %5lf, %5lf\n",
                           s_sum.x, s_sum.y, s_sum.alpha,
                           ts[i].x, ts[i].y, ts[i].alpha);
                mlt_log_warning(NULL,
                           "  avg: %5lf, %5lf, %5lf avg2: %5lf, %5lf, %5lf",
                           avg.x, avg.y, avg.alpha,
                           avg2.x, avg2.y, avg2.alpha);
            }
        }
        free(ts2);
    }


    /*  invert? */
    if (td->invert) {
        for (i = 0; i < td->trans_len; i++) {
            ts[i] = mult_transform(&ts[i], -1);
        }
    }

    /* relative to absolute */
    if (td->relative) {
        Transform t = ts[0];
        for (i = 1; i < td->trans_len; i++) {
            if (0/*verbose*/ ) {
                mlt_log_warning(NULL, "shift: %5lf   %5lf   %lf \n",
                           t.x, t.y, t.alpha *180/M_PI);
            }
            ts[i] = add_transforms(&ts[i], &t);
            t = ts[i];
        }
    }
    /* crop at maximal shift */
    if (td->maxshift != -1)
        for (i = 0; i < td->trans_len; i++) {
            ts[i].x     = TC_CLAMP(ts[i].x, -td->maxshift, td->maxshift);
            ts[i].y     = TC_CLAMP(ts[i].y, -td->maxshift, td->maxshift);
        }
    if (td->maxangle != - 1.0)
        for (i = 0; i < td->trans_len; i++)
            ts[i].alpha = TC_CLAMP(ts[i].alpha, -td->maxangle, td->maxangle);

    /* Calc optimal zoom
     *  cheap algo is to only consider transformations
     *  uses cleaned max and min
     */
    if (td->optzoom != 0 && td->trans_len > 1){
        Transform min_t, max_t;
        cleanmaxmin_xy_transform(ts, td->trans_len, 10, &min_t, &max_t);
        // the zoom value only for x
        double zx = 2*TC_MAX(max_t.x,fabs(min_t.x))/td->width_src;
        // the zoom value only for y
        double zy = 2*TC_MAX(max_t.y,fabs(min_t.y))/td->height_src;
        td->zoom += 100* TC_MAX(zx,zy); // use maximum
        mlt_log_debug(NULL,"Final zoom: %lf\n", td->zoom);
    }

    /* apply global zoom */
    if (td->zoom != 0){
        for (i = 0; i < td->trans_len; i++)
            ts[i].zoom += td->zoom;
    }

    return 1;
}
#endif
/**
 * transform_init:  Initialize this instance of the module.  See
 * tcmodule-data.h for function details.
 */

#if 0

static int transform_init(TCModuleInstance *self, uint32_t features)
{

    TransformData* td = NULL;
    TC_MODULE_SELF_CHECK(self, "init");
    TC_MODULE_INIT_CHECK(self, MOD_FEATURES, features);

    td = tc_zalloc(sizeof(TransformData));
    if (td == NULL) {
        tc_log_error(MOD_NAME, "init: out of memory!");
        return TC_ERROR;
    }
    self->userdata = td;
    if (verbose) {
        tc_log_info(MOD_NAME, "%s %s", MOD_VERSION, MOD_CAP);
    }

    return T;
}
#endif

/**
 * transform_configure:  Configure this instance of the module.  See
 * tcmodule-data.h for function details.
 */

#if 0 	
int transform_configure(TransformData *self,int width,int height, mlt_image_format  pixelformat, unsigned char* image ,Transform* tx,int trans_len)
{
    TransformData *td = self;

    /**** Initialise private data structure */

    /* td->framesize = td->vob->im_v_width *
     *  MAX_PLANES * sizeof(char) * 2 * td->vob->im_v_height * 2;
     */
	// rgb24 = w*h*3 , yuv420p = w* h* 3/2
    td->framesize_src = width*height*(pixelformat==mlt_image_rgb24 ? 3 : (3.0/2.0));
    td->src = malloc(td->framesize_src); /* FIXME */
    if (td->src == NULL) {
        mlt_log_error(NULL,"tc_malloc failed\n");
        return -1;
    }

    td->width_src  = width;
    td->height_src = height;

    /* Todo: in case we can scale the images, calc new size later */
    td->width_dest  = width;
    td->height_dest = height;
    td->framesize_dest = td->framesize_src;
    td->dest = 0;

    td->trans = tx;
    td->trans_len = trans_len;
    td->current_trans = 0;
    td->warned_transform_end = 0;

    /* Options */
    // set from  filter td->maxshift = -1;
    // set from  filter td->maxangle = -1;


    // set from  filter td->crop = 0;
    // set from  filter td->relative = 1;
    // set from  filter td->invert = 0;
    // set from  filter td->smoothing = 10;

    td->rotation_threshhold = 0.25/(180/M_PI);

    // set from  filter td->zoom    = 0;
    // set from  filter td->optzoom = 1;
    // set from  filter td->interpoltype = 2; // bi-linear
    // set from  filter td->sharpen = 0.8;

    td->interpoltype = TC_MIN(td->interpoltype,4);
    if (1) {
        mlt_log_debug(NULL, "Image Transformation/Stabilization Settings:\n");
        mlt_log_debug(NULL, "    smoothing = %d\n", td->smoothing);
        mlt_log_debug(NULL, "    maxshift  = %d\n", td->maxshift);
        mlt_log_debug(NULL, "    maxangle  = %f\n", td->maxangle);
        mlt_log_debug(NULL, "    crop      = %s\n",
                        td->crop ? "Black" : "Keep");
        mlt_log_debug(NULL, "    relative  = %s\n",
                    td->relative ? "True": "False");
        mlt_log_debug(NULL, "    invert    = %s\n",
                    td->invert ? "True" : "False");
        mlt_log_debug(NULL, "    zoom      = %f\n", td->zoom);
        mlt_log_debug(NULL, "    optzoom   = %s\n",
                    td->optzoom ? "On" : "Off");
        mlt_log_debug(NULL, "    interpol  = %s\n",
                    interpoltypes[td->interpoltype]);
        mlt_log_debug(NULL, "    sharpen   = %f\n", td->sharpen);
    }

    if (td->maxshift > td->width_dest/2
        ) td->maxshift = td->width_dest/2;
    if (td->maxshift > td->height_dest/2)
        td->maxshift = td->height_dest/2;

    if (!preprocess_transforms(td)) {
        mlt_log_error(NULL,"error while preprocessing transforms!");
        return -1;
    }

    switch(td->interpoltype){
      case 0:  interpolate = &interpolateZero; break;
      case 1:  interpolate = &interpolateLin; break;
      case 2:  interpolate = &interpolateBiLin; break;
      case 3:  interpolate = &interpolateSqr; break;
      case 4:  interpolate = &interpolateBiCub; break;
      default: interpolate = &interpolateBiLin;
    }
    return 0;
}
#endif


