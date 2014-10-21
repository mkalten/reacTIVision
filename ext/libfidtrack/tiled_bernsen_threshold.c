/*
  Fiducial tracking library.
  Copyright (C) 2004 Ross Bencina <rossb@audiomulch.com>
  Maintainer (C) 2005-2014 Martin Kaltenbrunner <martin@tuio.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "tiled_bernsen_threshold.h"

#include <stdlib.h>

#define WHITE ((unsigned char)255)
#define BLACK ((unsigned char)0)


/*
    thresholder works on tile_size tiles of the image. applying one threshold
    to each tile. each tile's threshold is taken of the mean of the min and max
    of a surrounding (2*tile_size) square tile. This is computed by first
    computing min and max for a set of tiles offset by (tile_size/2), these
    tiles are then combined to compute the threshold for each image-aligned
    thresholding tile.

    tiles with a threshold below contrast_threshold are clamped.
*/                    


void initialize_tiled_bernsen_thresholder(
        TiledBernsenThresholder *thresholder, int width, int height, int tile_size )
{
    int first_vector_size = tile_size / 2;
    int full_span_count = (width - first_vector_size) / tile_size;
    int min_max_width = full_span_count + 2;
    int first_block_height = tile_size / 2;
    int full_block_count = (height - first_block_height) / tile_size;
    int min_max_height = full_block_count + 2;
    int threshold_width = (width/tile_size) + 1;
    int threshold_height = (height/tile_size) + 1;

    thresholder->min_max = (unsigned char*)malloc( min_max_width * min_max_height * 2 );
    thresholder->threshold = (unsigned char*)malloc( threshold_width * threshold_height );
}


void terminate_tiled_bernsen_thresholder( TiledBernsenThresholder *thresholder )
{
    free( thresholder->min_max );
    free( thresholder->threshold );
}


static void init_min_max( unsigned char *min_max_dest, int count )
{
    int i;

    for( i = count; i > 0; --i ){
        min_max_dest[0] = 255;
        min_max_dest[1] = 0;
        min_max_dest += 2;
    }
}


static void compute_span_min_max( unsigned char *min_max_dest,
        const unsigned char *source, int source_stride, int count )
{
    int i;
    unsigned char min = min_max_dest[0];
    unsigned char max = min_max_dest[1];


    for( i=count; i > 0; --i ){
		if( *source < min ) min = *source;
		else if( *source > max ) max = *source;
        source += source_stride;
    }
/*
    for( i=count; i > 0; --i ){
        unsigned char x = *source;
        source += source_stride;

        if( x < min )
            min = x;
        else if( x > max )
            max = x;
    }
*/
    min_max_dest[0] = min;
    min_max_dest[1] = max;
}


static void compute_multiple_spans_min_max( unsigned char *min_max_dest,
        const unsigned char *source, int source_stride,
        int vector_size, int span_count )
{
    int i;
	int increment = vector_size * source_stride;
    for( i=span_count; i > 0 ; --i ){
        compute_span_min_max( min_max_dest, source, source_stride, vector_size );
        min_max_dest += 2;
        source += increment;
    }
}


static void compute_line_min_max_spans( unsigned char *min_max_dest,
        const unsigned char *source, int source_stride,
        int first_vector_size, int full_span_count, int last_vector_size,
        int tile_size )
{
    compute_span_min_max( min_max_dest, source, source_stride, first_vector_size );
    source += source_stride * first_vector_size;
    min_max_dest += 2;

    compute_multiple_spans_min_max( min_max_dest, source, source_stride, tile_size, full_span_count );
    source += source_stride * full_span_count * tile_size;
    min_max_dest += 2*full_span_count;

    compute_span_min_max( min_max_dest, source, source_stride, last_vector_size );
}


static void compute_frame_min_max_tiles( unsigned char *min_max_dest,
        const unsigned char *source, int source_stride,
        int width, int height, int tile_size )
{
    int first_vector_size = tile_size / 2;
    int full_span_count = (width - first_vector_size) / tile_size;
    int last_vector_size = width - first_vector_size - (full_span_count * tile_size );
    int first_block_height = tile_size / 2;
    int full_block_count = (height - first_block_height) / tile_size;
    int last_block_height = height - first_block_height - (full_block_count * tile_size );
    int i, j;
	int increment = source_stride * width;
	
    init_min_max( min_max_dest, full_span_count + 2 );
    for( i= first_block_height; i > 0; --i ){
        compute_line_min_max_spans( min_max_dest, source, source_stride,
                first_vector_size, full_span_count, last_vector_size, tile_size );
        source += increment;
    }
    min_max_dest += 2 * (full_span_count + 2);

    for( i=full_block_count; i > 0 ; --i ){
        init_min_max( min_max_dest, full_span_count + 2 );
        for( j= tile_size; j > 0; --j ){
            compute_line_min_max_spans( min_max_dest, source, source_stride,
                    first_vector_size, full_span_count, last_vector_size, tile_size );
            source += increment;
        }
        min_max_dest += 2 * (full_span_count + 2);
    }

    init_min_max( min_max_dest, full_span_count + 2 );
    for( i=last_block_height; i > 0 ; --i ){
        compute_line_min_max_spans( min_max_dest, source, source_stride,
                first_vector_size, full_span_count, last_vector_size, tile_size );
        source += increment;
    }
}


static void compute_row_thresholds( unsigned char *thresholds_dest,
        const unsigned char *min_max_a, const unsigned char *min_max_b,
        int count, int contrast_threshold )
{
    int i;
    unsigned char min_a, max_a, min_b, max_b, tt;

    for( i = count; i > 0 ; --i ){
        min_a = min_max_a[0];
        max_a = min_max_a[1];
        if( min_max_a[2] < min_a )
            min_a = min_max_a[2];
        if( min_max_a[3] > max_a )
            max_a = min_max_a[3];
        min_max_a += 2;

        min_b = min_max_b[0];
        max_b = min_max_b[1];
        if( min_max_b[2] < min_b )
            min_b = min_max_b[2];
        if( min_max_b[3] > max_b )
            max_b = min_max_b[3];
        min_max_b += 2;

        if( min_b < min_a )
            min_a = min_b;
        if( max_b > max_a )
            max_a = max_b;

        tt = (unsigned char)((min_a + max_a) / 2);
        if( (max_a - min_a) < contrast_threshold ) {
            if (tt<85) *thresholds_dest++ = WHITE;
            else *thresholds_dest++ = BLACK;
        } else {
            *thresholds_dest++ = tt;
		}

        /*if( max_a - min_a < contrast_threshold ) {
			*thresholds_dest++ = WHITE;
        } else {
            *thresholds_dest++ = (unsigned char)((min_a + max_a) / 2);
		}*/
    }
}


static void compute_frame_threshold_tiles( unsigned char *thresholds_dest,
        const unsigned char *min_max,
        int width, int height, int tile_size, int contrast_threshold )
{
    int first_vector_size = tile_size / 2;
    int full_span_count = (width - first_vector_size) / tile_size;
    int min_max_width = full_span_count + 2;
    int threshold_width = (width/tile_size) + 1;
    int threshold_height = (height/tile_size) + 1;
    int i;

    for( i = threshold_height; i > 0 ; --i ){
        compute_row_thresholds( thresholds_dest, min_max, min_max + min_max_width * 2,
                threshold_width, contrast_threshold );
        thresholds_dest += threshold_width;
        min_max += min_max_width * 2;
    }
}


static void apply_frame_thresholds( unsigned char *dest,
        const unsigned char *source, int source_stride,
        const unsigned char *threshold,
        int width, int height, int tile_size )
{
    int j, k, m, n;
    int tile_width, tile_height;
    const unsigned char *t;
    int threshold_width = (width/tile_size) + 1;

    for( j = 0; j < height; j += tile_size ){
        tile_height = (j + tile_size > height) ? height - j : tile_size;
        for( k = tile_height; k > 0; --k ){
            t = threshold;
            for( m=0; m < width; m += tile_size ){
                unsigned char tt = *t++;
                tile_width = (m + tile_size > width) ? width - m : tile_size;
                for( n=tile_width; n > 0; --n ){
                    *dest++ = (*source > tt) ? WHITE : BLACK;
                    source += source_stride;
                }
            }
        }
        threshold += threshold_width;
    }
}


void tiled_bernsen_threshold( TiledBernsenThresholder *thresholder,
        unsigned char *dest, const unsigned char *source, int source_stride,
        int width, int height, int tile_size, int contrast_threshold )
{
    compute_frame_min_max_tiles( thresholder->min_max,
            source, source_stride, width, height, tile_size );

    compute_frame_threshold_tiles( thresholder->threshold, thresholder->min_max,
            width, height, tile_size, contrast_threshold );

    apply_frame_thresholds( dest, source, source_stride, thresholder->threshold,
             width, height, tile_size );
}



