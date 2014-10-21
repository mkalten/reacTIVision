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

#ifndef INCLUDED_SEGMENT_H
#define INCLUDED_SEGMENT_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/*
    usage:

    Segmenter s;

    ...
    
    initialize_segmenter( &s, WIDTH, HEIGHT, 8 );

    ...

    step_segmenter( &s, thresholded_image, WIDTH, HEIGHT );

    ...

    terminate_segmenter( &s );
*/

#define NO_REGION_FLAG                  (0)

#define FREE_REGION_FLAG                (1)

#define ADJACENT_TO_ROOT_REGION_FLAG    (2)

/*
    saturated regions are those whose adjacency list became full and other nodes
    couldn't be made adjacent, either during a make_adjacent operation or
    during a merge operation.
*/
#define SATURATED_REGION_FLAG           (4)

/*
    fragmented regions are those which couldn't be made adjacent to another
    region (or had to be detached from another region) because the other region
    was saturated.
*/
#define FRAGMENTED_REGION_FLAG          (8)

#define LOST_SYMBOL_FLAG				(16)

#define VALID_REGION_FLAG				(64)

#define UNKNOWN_REGION_LEVEL            (-1)

#define REGION_GATE_AREA				1

typedef struct Span{
	int start, end;
	struct Span *next;
} Span;

typedef struct Region{
    struct Region *previous, *next;
    unsigned char colour;
    short left, top, right, bottom;
    short center_x, center_y;

	struct Span *first_span;
	struct Span *last_span;
	int area;
	
    int flags;

    short level;                            /* initialized to UNKNOWN_REGION_LEVEL */
    short depth;                            /* initialized to 0 */
    short children_visited_count;           /* initialized to 0 */
    short descendent_count;                 /* initialized to 0x7FFF */
    char *depth_string;                     /* not initialized by segmenter */
    
    short adjacent_region_count;
    struct Region *adjacent_regions[ 1 ];   /* variable length array of length max_adjacent_regions */
} Region;


typedef struct RegionReference{
    Region *region;
    struct RegionReference *redirect;
} RegionReference;



//#define REGIONREF_IS_REDIRECTED(r) ((r)->redirect != (r))


#define RESOLVE_REGIONREF_REDIRECTS( x, r )                                    \
{                                                                              \
    if( r->redirect != r ){                                                    \
        RegionReference *result = r;										   \
		while( result->redirect != result )								       \
            result = result->redirect;                                         \
        r->redirect = result;                                                  \
        x = result;                                                            \
    }else{                                                                     \
        x = r;                                                                 \
    }                                                                          \
}


void initialize_head_region( Region *r );
void link_region( Region *head, Region* r );
void unlink_region( Region* r );


typedef struct Segmenter{
    RegionReference *region_refs;
    int region_ref_count;
    unsigned char *regions;     /* buffer containing raw region ptrs */
    unsigned char *spans;		/* buffer containing raw span ptrs */
    int region_count;
    Region *freed_regions_head;

    int sizeof_region;
    int max_adjacent_regions;
	
	int width, height;

    RegionReference **regions_under_construction;
}Segmenter;

#define LOOKUP_SEGMENTER_REGION( s, index )\
    (Region*)(s->regions + (s->sizeof_region * (index)))

#define LOOKUP_SEGMENTER_SPAN( s, index )\
    (Span*)(s->spans + (sizeof(Span) * (index)))

void initialize_segmenter( Segmenter *segments, int width, int height, int max_adjacent_regions );
void terminate_segmenter( Segmenter *segments );

void step_segmenter( Segmenter *segments, const unsigned char *source );


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* INCLUDED_SEGMENT_H */

