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

#ifndef INCLUDED_TOPOLOGYSEARCH_H
#define INCLUDED_TOPOLOGYSEARCH_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "segment.h"

/* values for Region::level */

#define FID_LEAF_LEVEL              (0)
#define FID_LEAF_CONTAINER_LEVEL    (1)
#define FID_ROOT_LEVEL              (2)     /* container of leaf containers */
#define FID_UNKNOWN_LEVEL           (-1)
#define FID_DISCARDED_LEVEL         (-2)    /* sentinel level for leafs we want to ignore */


typedef struct PartialSegmentTopology{
    struct Region black_leaf_regions_head;
    struct Region black_leaf_container_regions_head;
    struct Region root_regions_head;
}PartialSegmentTopology;


void initialize_segment_topology( PartialSegmentTopology *pst,
        Segmenter *segments, int symbol_count );


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* INCLUDED_TOPOLOGYSEARCH_H */
