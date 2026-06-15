/*	Fiducial tracking library.
	Copyright (C) 2004 Ross Bencina <rossb@audiomulch.com>
	Maintainer (C) 2005-2026 Martin Kaltenbrunner <martin@tuio.org>

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

#ifndef INCLUDED_BRADLEY_ROTH_THRESHOLD_H
#define INCLUDED_BRADLEY_ROTH_THRESHOLD_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef struct BradleyRothThresholder {
    unsigned int *integral;
} BradleyRothThresholder;

void initialize_bradley_roth_thresholder(
        BradleyRothThresholder *thresholder, int width, int padded_height );
void terminate_bradley_roth_thresholder( BradleyRothThresholder *thresholder );

/*
    Per-pixel adaptive thresholding using a sliding rectangular window.
    Uses an integral image for O(1) per-pixel mean computation.

    Bradley-Roth formula: white if  v > mean * (1 - bias)
    Integer form:         v * count * 1024 > sum * (1024 - bias_int)
    where bias_int = (int)(bias * 1024)

    source: monochrome or interleaved (only first byte per pixel examined).
    source_stride: bytes per pixel (1=grey, 3=RGB, 4=RGBA).
    dest: thresholded 8-bit black/white output.
    window_size: half-width of local neighbourhood (full = 2*window_size+1).
    bias: sensitivity, typically 0.05..0.25 (higher = darker threshold).

    padded_height / strip_height / src_row_offset:
        For multi-threaded strip processing with halo rows.
        Pass padded_height==strip_height==height, src_row_offset==0 for
        single-threaded whole-frame use.
*/

void bradley_roth_threshold( BradleyRothThresholder *thresholder,
        unsigned char *dest, const unsigned char *source,
        int width, int padded_height, int strip_height, int src_row_offset,
        int window_size, float bias );

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* INCLUDED_BRADLEY_ROTH_THRESHOLD_H */
