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

#ifndef INCLUDED_FIDTRACK_H
#define INCLUDED_FIDTRACK_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "segment.h"
#include "treeidmap.h"
#include "floatpoint.h"

typedef struct FidtrackerX{

    int min_target_root_descendent_count;
    int max_target_root_descendent_count;
    int min_target_root_descendent_range;
    int max_target_root_descendent_range;
    int min_depth, max_depth;

    struct Region root_regions_head;

    char *depth_strings;
    int depth_string_count;
    int depth_string_length;
    int next_depth_string;
    char *temp_coloured_depth_string;

    double black_x_sum, black_y_sum, black_leaf_count;
    double white_x_sum, white_y_sum, white_leaf_count;
	double black_x_sum_warped, black_y_sum_warped, black_leaf_count_warped;
    double white_x_sum_warped, white_y_sum_warped, white_leaf_count_warped;

 //   int min_leaf_width_or_height;
	int total_leaf_count;
    double total_leaf_size;
    double average_leaf_size;

    TreeIdMap *treeidmap;
    ShortPoint *pixelwarp;

} FidtrackerX;

/* pixelwarp is a Width by Height array of pixel coordinates and can be NULL */

void initialize_fidtrackerX( FidtrackerX *ft, TreeIdMap *treeidmap, ShortPoint *pixelwarp );

void terminate_fidtrackerX( FidtrackerX *ft );



#define INVALID_FIDUCIAL_ID  INVALID_TREE_ID
#define FUZZY_NODE_RANGE 2

typedef struct FiducialX{
    int id;                                 /* can be INVALID_FIDUCIAL_ID */
    float x, y;
	//float a, b;
    float angle;
    float leaf_size;
    float root_size;
	int root_colour;
	int node_count;
}FiducialX;

typedef struct RegionX{
    //int type;

    short top,bottom,left,right;
    short width,height;
    short x, y;
	int area;
	struct Span *span;
	int colour;

} RegionX;


/*
    usage:

    #define MAX_FIDUCIAL_COUNT  128
    Fiducial fiducials[ MAX_FIDUCIAL_COUNT ];
    PartialSegmentTopology pst;
    int count;

    count = find_fiducials( segments, &pst, fiducials, MAX_FIDUCIAL_COUNT );
*/

int find_fiducialsX( FiducialX *fiducials, int max_count,
        FidtrackerX *ft, Segmenter *segments, int width, int height);

int find_regionsX( RegionX *regions, int max_count,
        FidtrackerX *ft, Segmenter *segments, int width, int height, int min_size, int max_size);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* INCLUDED_FIDTRACK_H */
