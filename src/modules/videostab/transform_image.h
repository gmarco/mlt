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

#include <math.h>
#include <stdio.h>
#include "tlist.h"
#include <framework/mlt_types.h>
#include "vid.stab/src/transform.h"

/** interpolateBiLinBorder: bi-linear interpolation function that also works at the border.
    This is used by many other interpolation methods at and outsize the border, see interpolate */
int transform_configure(TransformData *self,int width,int height, mlt_image_format  pixelformat, unsigned char* image ,Transformations* tx,int trans_len,int framesize);

int transform_filter_video(TransformData *self, Transformations* trans,
		                                  unsigned char *frame,mlt_image_format pixelformat,int framesize);
