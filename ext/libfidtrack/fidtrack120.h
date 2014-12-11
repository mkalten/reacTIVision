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

#ifndef INCLUDED_FIDTRACK120_H
#define INCLUDED_FIDTRACK120_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "segment.h"
#include "topologysearch.h"


#define MAX_FIDUCIAL120_SYMBOLS     6

#define INVALID_FIDUCIAL120_ID  (-1)

typedef struct Fiducial120{
    int id;                                 /* can be INVALID_FIDUCIAL_ID */
    int symbol_sequence[ MAX_FIDUCIAL120_SYMBOLS ];
    int sequence_length;
    float x, y;
    float angle;
}Fiducial120;

/*
    return a unique id for a permutation of numbers from 0 to length - 1
    
    sequence is an array containing length elements, which are a permutation of
    mutually unique numbers from 0 to length - 1

    length must be 9 or less for this version due to the hardcoded size
    of the lookup tables

    function assumes that sequence begins with an alignment value of 0
*/
int fiducial120_id_from_symbol_sequence( int *sequence, int length );


/*
    usage:

    #define MAX_FIDUCIAL120_COUNT  120
    Fiducial fiducials[ MAX_FIDUCIAL120_COUNT ];
    PartialSegmentTopology pst;
    int count;

    count = find_fiducials( segments, &pst, fiducials, MAX_FIDUCIAL120_COUNT );
*/

int find_fiducials120( Fiducial120 *fiducials, int count,
        PartialSegmentTopology *pst, Segmenter *segments );



#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* INCLUDED_FIDTRACK_H */
