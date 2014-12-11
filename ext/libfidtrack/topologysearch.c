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

#define BLACK (0)
#define WHITE (255)

#include "topologysearch.h"

#include <math.h>
#include <assert.h>


static void find_black_leafs( PartialSegmentTopology *pst, Segmenter *s )
{
    int i;

    for( i=0; i < s->region_count; ++i ){
        Region *r = LOOKUP_SEGMENTER_REGION( s, i );

        if( r->colour == BLACK
                && r->adjacent_region_count == 1 ){ /* it's a leaf */

            r->level = FID_LEAF_LEVEL;
            link_region( &pst->black_leaf_regions_head, r );
        }else{
            r->level = FID_UNKNOWN_LEVEL;
        }
    }
}


static int has_only_one_non_leaf_adjacent_region( Region* r )
{
    int i;

    int non_leaf_adjacent_count = 0;
    for( i=0; i < r->adjacent_region_count; ++i ){
        if( r->adjacent_regions[i]->level != FID_LEAF_LEVEL )
            ++non_leaf_adjacent_count;
    }

    if (non_leaf_adjacent_count == 1)
        return 1;
    else
        return 0;
}


static void find_black_leaf_container_regions( PartialSegmentTopology *pst, Segmenter *s, int symbol_count )
{
    Region *next = pst->black_leaf_regions_head.next;
    while( next != &pst->black_leaf_regions_head ){
		Region *potential_container;

        assert( next->level == FID_LEAF_LEVEL );
		assert( next->adjacent_region_count == 1 );
		assert( next->colour == BLACK );

		/* leafs have only one adjacent region, so next->adjacent_regions[0] must be the parent */
        potential_container = next->adjacent_regions[0];
		assert( potential_container->colour == WHITE );

        if( potential_container->level == FID_UNKNOWN_LEVEL ){

            if( has_only_one_non_leaf_adjacent_region( potential_container )
                    && potential_container->adjacent_region_count >= 2
                    && potential_container->adjacent_region_count <= symbol_count + 1 
                 ){

                potential_container->level = FID_LEAF_CONTAINER_LEVEL;
                link_region( &pst->black_leaf_container_regions_head, potential_container );

            }else{
                potential_container->level = FID_DISCARDED_LEVEL;
            }
        }

        next = next->next;
    }
}


static int count_non_leaf_container_adjacent_regions( Region* r )
{
    int result = 0;
    int i;

    for( i=0; i < r->adjacent_region_count; ++i ){
        if( r->adjacent_regions[i]->level != FID_LEAF_CONTAINER_LEVEL )
            ++result;
    }

    return result;
}


static void find_root_regions( PartialSegmentTopology *pst, Segmenter *s, int symbol_count )
{
    int i;

    Region *next = pst->black_leaf_container_regions_head.next;
    while( next != &pst->black_leaf_container_regions_head ){

        for( i=0; i < next->adjacent_region_count; ++i ){

            Region *potential_root = next->adjacent_regions[i];
            int non_leaf_container_adjacent_count;

            non_leaf_container_adjacent_count =
                    count_non_leaf_container_adjacent_regions( potential_root );

            if( potential_root->level == FID_UNKNOWN_LEVEL
                    && ((potential_root->adjacent_region_count == symbol_count
                         && non_leaf_container_adjacent_count == 0 )
                         || (potential_root->adjacent_region_count == symbol_count + 1
                         && non_leaf_container_adjacent_count == 1 ) ) ){

                //resolve_all_adjacent_region_redirects( potential_root );
                potential_root->level = FID_ROOT_LEVEL;
                link_region( &pst->root_regions_head, potential_root );
            }
        }

        next = next->next;
    }
}


static void find_roots_bottom_up( PartialSegmentTopology *pst, Segmenter *s, int symbol_count )
{
    find_black_leafs( pst, s );
    find_black_leaf_container_regions( pst, s, symbol_count );
    find_root_regions( pst, s, symbol_count );
}


void initialize_segment_topology( PartialSegmentTopology *pst,
        Segmenter *segments, int symbol_count )
{
    initialize_head_region( &pst->black_leaf_regions_head );
    initialize_head_region( &pst->black_leaf_container_regions_head );
    initialize_head_region( &pst->root_regions_head );

    find_roots_bottom_up( pst, segments, symbol_count );
}
