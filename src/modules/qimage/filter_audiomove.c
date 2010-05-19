/*
 * filter_audiomove.c -- convert from one audio format to another
 * Copyright (C) 2010 Ushodaya Enterprises Limited
 * Author: Marco Gittler <g.marco@freenet.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int move_audio( mlt_frame frame, void **audio, mlt_audio_format *format, mlt_audio_format requested_format )
{
	int error = 0;
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
	int channels = mlt_properties_get_int( properties, "audio_channels" );
	int samples = mlt_properties_get_int( properties, "audio_samples" );
	int to_channels=6;
	int size = 0;
	int i=0;
	void*  buffer;

	switch( *format )
	{
		case mlt_image_none:
			size = 0;
			buffer = NULL;
			break;
		case mlt_audio_s16:
			size = samples * to_channels * sizeof( int16_t );
			break;
		case mlt_audio_s32:
			size = samples * to_channels * sizeof( int32_t );
			break;
		case mlt_audio_float:
			size = samples * to_channels * sizeof( float );
			break;
	}
	if ( size )
		buffer = mlt_pool_alloc( size );
	if ( buffer )
		memset( buffer, 0, size );

	 mlt_log_debug( NULL, "[filter audiomove] ch_b=%d ch_a=%d samples_b=%d samples_a=%d",channels,to_channels,samples,size/to_channels);
	 printf("[filter audiomove] ch_b=%d ch_a=%d samples_b=%d samples_a=%d\n",channels,to_channels,samples,size/to_channels);
	for ( i=0 ;i< size ;i++){
		*((uint8_t*)buffer+i)=(i+255)%255;
	}
	//mlt_properties_set_data( properties, "audio", buffer, size, ( mlt_destructor )mlt_pool_release, NULL );

	if ( size )
	{
		*audio=buffer; 
		mlt_properties_set_int( properties, "audio_channels", to_channels );
		mlt_frame_set_audio( frame, *audio, requested_format, size, mlt_pool_release );
		*format = requested_format;
	}
	return error;
}

/** Filter processing.
*/

static mlt_frame filter_process( mlt_filter this, mlt_frame frame )
{
	frame->convert_audio = move_audio;
	return frame;
}

/** Constructor for the filter.
*/

mlt_filter filter_audiomove_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter this = calloc( sizeof( struct mlt_filter_s ), 1 );
	if ( mlt_filter_init( this, this ) == 0 )
		this->process = filter_process;
	return this;
}
