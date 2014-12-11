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

#ifndef INCLUDED_TILED_BERNSEN_THRESHOLD_H
#define INCLUDED_TILED_BERNSEN_THRESHOLD_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef struct TiledBernsenThresholder{
    unsigned char *min_max;
    unsigned char *threshold;
} TiledBernsenThresholder;

void initialize_tiled_bernsen_thresholder(
        TiledBernsenThresholder *thresholder, int width, int height, int tile_size );
void terminate_tiled_bernsen_thresholder( TiledBernsenThresholder *thresholder );


/*
    source can be a monochrome (source_stride==1) or RGB (source_stride==3) or
    RGBA (source_stride==4) etc image. only the first byte is examined and
    assumed to be luma.

    dest is the thresholded data as 8 bit black and white

    example usage:

    TiledBernsenThresholder t;
    initialize_tiled_bernsen_thresholder( &t );
    ...
    tiled_bernsen_threshold( &t, dest, source, 3, WIDTH, HEIGHT, 16, 40 );
    ...
    terminate_tiled_bernsen_thresholder( &t );
*/

void tiled_bernsen_threshold( TiledBernsenThresholder *thresholder,
        unsigned char *dest, const unsigned char *source, int source_stride,
        int width, int height, int tile_size, int contrast_threshold );
        

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* INCLUDED_TILED_BERNSEN_THRESHOLD_H */
