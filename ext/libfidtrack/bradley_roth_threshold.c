/*	Fiducial tracking library.
	Copyright (C) 2004 Ross Bencina <rossb@audiomulch.com>
	Maintainer (C) 2005-2016 Martin Kaltenbrunner <martin@tuio.org>

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

#include "bradley_roth_threshold.h"
#include <stdlib.h>

#define WHITE ((unsigned char)255)
#define BLACK ((unsigned char)0)

/*
    Per-pixel adaptive thresholding using an integral image for O(1) mean.

    Bradley-Roth formula: white if  v > mean * (1 - bias)
    Integer form:  v * count * 1024 > sum * (1024 - bias_int)

    The inner loop is split into border/interior zones to eliminate per-pixel
    clamping branches and hoist constant multipliers out of the hot path,
    enabling auto-vectorisation by the compiler.
*/


void initialize_bradley_roth_thresholder(
        BradleyRothThresholder *thresholder, int width, int padded_height )
{
    int padded = (width + 1) * (padded_height + 1);
    thresholder->integral = (unsigned int*)malloc( padded * sizeof(unsigned int) );
}


void terminate_bradley_roth_thresholder( BradleyRothThresholder *thresholder )
{
    free( thresholder->integral );
}


static void build_integral_image( unsigned int *integral,
        const unsigned char *source,
        int width, int height )
{
    int x, y;
    int w1 = width + 1;

    for( x = 0; x < w1; ++x ) integral[x] = 0;

    for( y = 0; y < height; ++y ){
        unsigned int row_sum = 0;
        const unsigned char *src_row = source + y * width;
        unsigned int *int_row_cur = integral + (y+1) * w1;
        const unsigned int *int_row_prv = integral + y * w1;
        int_row_cur[0] = 0;
        for( x = 0; x < width; ++x ){
            row_sum += src_row[x];
            int_row_cur[x+1] = row_sum + int_row_prv[x+1];
        }
    }
}


void bradley_roth_threshold( BradleyRothThresholder *thresholder,
        unsigned char *dest, const unsigned char *source,
        int width, int padded_height, int strip_height, int src_row_offset,
        int window_size, float bias )
{
    int x, y;
    int w1 = width + 1;

    build_integral_image( thresholder->integral, source, width, padded_height );

    /*
     * Split rows into three zones to eliminate per-pixel clamping:
     *
     *  top halo:  iy in [0,             window_size)              — y0 clamped
     *  interior:  iy in [window_size,   padded_height-window_size) — no clamping
     *  bot halo:  iy in [padded_height-window_size, padded_height) — y1 clamped
     *
     * For fully-interior pixels (interior row AND interior column):
     *   count is constant == (2*window_size+1)^2
     *   lhs_scale = count * 1024 is hoisted outside both loops.
     */
    int bias_int  = (int)(bias * 1024.0f);
    int rhs_scale = 1024 - bias_int;

    int iy_top_end = window_size;
    int iy_bot_beg = padded_height - window_size;

    int x_left_end  = window_size;
    int x_right_beg = width - window_size;

    int full_win   = 2 * window_size + 1;
    int count_full = full_win * full_win;

    /* B: precompute fixed-point reciprocal of count_full (Q20) so the
     * interior hot loop uses one multiply+shift instead of two multiplies.
     * threshold = (sum * rhs_scale) >> 10) * inv_count >> 20
     *           = sum * rhs_scale / (count_full * 1024)               */
    unsigned int inv_count_full = (1u << 20) / (unsigned int)count_full;

    for( y = 0; y < strip_height; ++y ){
        int iy = y + src_row_offset;

        int y0 = iy - window_size; if( y0 < 0 )             y0 = 0;
        int y1 = iy + window_size + 1; if( y1 > padded_height ) y1 = padded_height;
        int count_y = y1 - y0;

        const unsigned int *row_y1 = thresholder->integral + y1 * w1;
        const unsigned int *row_y0 = thresholder->integral + y0 * w1;

        const unsigned char *src_row = source + iy * width;
        unsigned char       *dst_row = dest   + y  * width;

        /* left border columns — count varies, use two-multiply form */
        for( x = 0; x < x_left_end && x < width; ++x ){
            int x0 = x - window_size; if( x0 < 0 ) x0 = 0;
            int x1 = x + window_size + 1; if( x1 > width ) x1 = width;
            unsigned int count = (unsigned int)(x1 - x0) * (unsigned int)count_y;
            unsigned int sum   = row_y1[x1] - row_y1[x0] - row_y0[x1] + row_y0[x0];
            unsigned int v     = src_row[x];
            /* D: branchless — (unsigned)(-(cond)) gives 0xFF..FF or 0 */
            dst_row[x] = (unsigned char)(-(v * count > (sum * (unsigned int)rhs_scale >> 10)));
        }

        if( iy >= iy_top_end && iy < iy_bot_beg ){
            /* interior rows: count constant, use reciprocal multiply (B) */
            for( x = x_left_end; x < x_right_beg; ++x ){
                int x0 = x - window_size;
                int x1 = x + window_size + 1;
                unsigned int sum       = row_y1[x1] - row_y1[x0] - row_y0[x1] + row_y0[x0];
                unsigned int v         = src_row[x];
                unsigned int threshold = ((sum * (unsigned int)rhs_scale) >> 10) * inv_count_full >> 20;
                /* D: branchless */
                dst_row[x] = (unsigned char)(-(v > threshold));
            }
        } else {
            /* top/bottom halo: count_y fixed per row, precompute reciprocal */
            unsigned int inv_count_mid = (x_right_beg > x_left_end)
                ? (1u << 20) / (unsigned int)(full_win * count_y) : 0;
            for( x = x_left_end; x < x_right_beg; ++x ){
                int x0 = x - window_size;
                int x1 = x + window_size + 1;
                unsigned int sum       = row_y1[x1] - row_y1[x0] - row_y0[x1] + row_y0[x0];
                unsigned int v         = src_row[x];
                unsigned int threshold = ((sum * (unsigned int)rhs_scale) >> 10) * inv_count_mid >> 20;
                /* D: branchless */
                dst_row[x] = (unsigned char)(-(v > threshold));
            }
        }

        /* right border columns — count varies, use two-multiply form */
        for( x = x_right_beg; x < width; ++x ){
            int x0 = x - window_size; if( x0 < 0 ) x0 = 0;
            int x1 = x + window_size + 1; if( x1 > width ) x1 = width;
            unsigned int count = (unsigned int)(x1 - x0) * (unsigned int)count_y;
            unsigned int sum   = row_y1[x1] - row_y1[x0] - row_y0[x1] + row_y0[x0];
            unsigned int v     = src_row[x];
            dst_row[x] = (unsigned char)(-(v * count > (sum * (unsigned int)rhs_scale >> 10)));
        }
    }
}
