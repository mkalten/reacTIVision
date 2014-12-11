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

#ifndef INCLUDED_THRESHOLD_H
#define INCLUDED_THRESHOLD_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/*
    source can be a monochrome (source_stride==1) or RGB (source_stride==3) or
    RGBA (source_stride==4) etc image. only the first byte is examined and
    assumed to be luma.

    dest is the thresholded data as 8 bit black and white

    example usage:

    simple_threshold( source, 3, dest, WIDTH, HEIGHT, 115 );
    simple_adaptive_threshold( source, 3, dest, WIDTH, HEIGHT, 32 );
    overlapped_adaptive_threshold( source, 3, dest, WIDTH, HEIGHT, 16 );
*/

void simple_threshold( const unsigned char *source, int source_stride,
        unsigned char *dest,
        int width, int height,
        int threshold );

void simple_adaptive_threshold( const unsigned char *source, int source_stride,
        unsigned char *dest,
        int width, int height,
        int tile_size );

void overlapped_adaptive_threshold( const unsigned char *source, int source_stride,
        unsigned char *dest,
        int width, int height,
        int tile_size );

void overlapped_adaptive_threshold2( const unsigned char *source, int source_stride,
        unsigned char *dest,
        int width, int height,
        int tile_size, int gradient_threshold );


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* INCLUDED_THRESHOLD_H */
