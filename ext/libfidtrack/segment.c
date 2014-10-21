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

#include "segment.h"

#include <stdlib.h>
#include <assert.h>


/* -------------------------------------------------------------------------- */

void initialize_head_region( Region *r )
{
    r->next = r->previous = r;
}


void link_region( Region *head, Region* r )
{
    r->previous = head->previous;
    r->next = head;

    r->previous->next = r;
    r->next->previous = r;
}


void unlink_region( Region* r )
{
    r->previous->next = r->next;
    r->next->previous = r->previous;
}


/* -------------------------------------------------------------------------- */


static RegionReference* new_region( Segmenter *s, int x, int y, int colour )
{
    RegionReference *result;
    Region *r;
//	int i;

    if( s->freed_regions_head ){
        r = s->freed_regions_head;
        s->freed_regions_head = r->next;
    }else{
        r = LOOKUP_SEGMENTER_REGION( s, s->region_count++ );
    }

	assert( colour == 0 || colour == 255 );

    r->colour = colour;

    r->left = r->right = (short)x;
    r->top = r->bottom = (short)y;

	r->flags = NO_REGION_FLAG;
	r->area = 0;

	r->first_span = NULL;
	r->last_span = NULL;

    r->level = UNKNOWN_REGION_LEVEL;
    r->depth = 0;
    r->children_visited_count = 0;
    r->descendent_count = 0x7FFF;

    r->adjacent_region_count = 0;	
/*	i = y*(s->width)+x;
	
	r->first_span = LOOKUP_SEGMENTER_SPAN( s,  i );
	r->first_span->start = i;
	r->first_span->end = i;
	r->last_span = r->first_span;
	r->last_span->next = NULL;
*/
    result = &s->region_refs[ s->region_ref_count++ ];
    result->redirect = result;
    result->region = r;

    return result;
}


#ifndef NDEBUG
static int r1_adjacent_contains_r2( Region* r1, Region* r2 )
{
    int i;

    for( i=0; i < r1->adjacent_region_count; ++i ){
        Region *adjacent = r1->adjacent_regions[i];
        if( adjacent == r2 )
            return 1;
    }
    return 0;
}
#endif


static int is_adjacent( Region* r1, Region* r2 )
{
/*
    int i, adjacent_region_count;
    Region **adjacent_regions, *r;

    if( r1->adjacent_region_count > r2->adjacent_region_count ){
        r = r2;
        adjacent_regions = r1->adjacent_regions;
        adjacent_region_count = r1->adjacent_region_count;
    }else{
        r = r1;
        adjacent_regions = r2->adjacent_regions;
        adjacent_region_count = r2->adjacent_region_count;
    }

    for( i=0; i < adjacent_region_count; ++i )
        if( adjacent_regions[i] == r )
            return 1;
*/

    int i;

    // choose the region with the shorter list for iteration
    if( r1->adjacent_region_count > r2->adjacent_region_count ){
        Region *temp = r1;
        r1 = r2;
        r2 = temp;
    }

    for( i=0; i < r1->adjacent_region_count; ++i )
        if( r1->adjacent_regions[i] == r2 )
            return 1;

    return 0;
}


static void remove_adjacent_from( Region *r1, Region *r2 ) // remove r2 from r1
{
    int i;
#ifndef NDEBUG
    int found = 0;
#endif

    for( i = 0; i < r1->adjacent_region_count; ++i ){
        if( r1->adjacent_regions[i] == r2 ){
            r1->adjacent_regions[i] = r1->adjacent_regions[ --r1->adjacent_region_count ];
#ifndef NDEBUG
            found = 1;
#endif
            break;
        }
    }

    assert( found );
}


static void make_saturated( Region* r1 )
{
    int i;
    int r1_adjacent_region_count = r1->adjacent_region_count; 

    // remove adjacencies from r1 and mark them as fragmented
    for( i = 0; i < r1_adjacent_region_count; ++i ){
        Region *a = r1->adjacent_regions[i];

        remove_adjacent_from( a, r1 );
        a->flags |= FRAGMENTED_REGION_FLAG;
    }
    r1->adjacent_region_count = 0;
    r1->flags |= SATURATED_REGION_FLAG;
}

/*
static void make_fragmented( Region* r1 )
{
    int i;
    int r1_adjacent_region_count = r1->adjacent_region_count; 

    // remove adjacencies from r1
    for( i = 0; i < r1_adjacent_region_count; ++i ){
        Region *a = r1->adjacent_regions[i];

        remove_adjacent_from( a, r1 );
    }
    r1->adjacent_region_count = 0;
    r1->flags |= FRAGMENTED_REGION_FLAG;
}
*/

static void make_adjacent( Segmenter *s, Region* r1, Region* r2 )
{
    if( !is_adjacent( r1, r2 ) ){
        if( r1->flags & SATURATED_REGION_FLAG ){
            r2->flags |= FRAGMENTED_REGION_FLAG;

        }else if( r2->flags & SATURATED_REGION_FLAG ){
            r1->flags |= FRAGMENTED_REGION_FLAG;

        }else{
            if( r1->adjacent_region_count == s->max_adjacent_regions ){
                make_saturated(r1);
                r2->flags |= FRAGMENTED_REGION_FLAG;

                if( r2->adjacent_region_count == s->max_adjacent_regions ){
                    make_saturated(r2);
                    r1->flags |= FRAGMENTED_REGION_FLAG;
                }
            }else if( r2->adjacent_region_count == s->max_adjacent_regions ){
                make_saturated(r2);
                r1->flags |= FRAGMENTED_REGION_FLAG;
            }else{
                assert( !(r1->flags & SATURATED_REGION_FLAG) );
                assert( !(r2->flags & SATURATED_REGION_FLAG) );
                assert( r1->adjacent_region_count < s->max_adjacent_regions );
                assert( r2->adjacent_region_count < s->max_adjacent_regions );
                r1->adjacent_regions[ r1->adjacent_region_count++ ] = r2;
                r2->adjacent_regions[ r2->adjacent_region_count++ ] = r1;
            }
        }
    }

    // postcondition, adjacency link is bi-directional or isn't created
    assert(
        (r1_adjacent_contains_r2( r1, r2 ) && r1_adjacent_contains_r2( r2, r1 ))
        ||
        (!r1_adjacent_contains_r2( r1, r2 ) && !r1_adjacent_contains_r2( r2, r1 ))
    );
}


static void replace_adjacent( Region *r1, Region *r2, Region *r3 )  // replace r2 with r3 in the adjacency list of r1
{
    int i;
#ifndef NDEBUG
    int found = 0;
#endif
    for( i = 0; i < r1->adjacent_region_count; ++i ){
        if( r1->adjacent_regions[i] == r2 ){
            r1->adjacent_regions[i] = r3;
#ifndef NDEBUG
            found = 1;
#endif
            break;
        }
    }

    assert( found );    
}


// merge r2 into r1 by first removing all common adjacencies from r2
// then transferring the remainder of adjacencies to r1 after discarding
// any excess adjacencies.
static void merge_regions( Segmenter *s, Region* r1, Region* r2 )
{
    int i;
 
    assert( r1 != r2 );
    assert( !r1_adjacent_contains_r2( r1, r2 ) && !r1_adjacent_contains_r2( r2, r1 ) );

    if( r1->flags & SATURATED_REGION_FLAG ){
        make_saturated( r2 );

        if( r2->flags & SATURATED_REGION_FLAG )
            make_saturated( r1 );

    }else if( r2->flags & SATURATED_REGION_FLAG ){
        make_saturated( r1 );

    }else{

        // remove adjacencies of r2 which are already in r1
        for( i = 0; i<r2->adjacent_region_count; ++i ){
            Region *a = r2->adjacent_regions[i];
            if( is_adjacent( a, r1 ) ){
                remove_adjacent_from( a, r2 );
                r2->adjacent_regions[i] = r2->adjacent_regions[ --r2->adjacent_region_count ];
            }
        }

        if( r1->adjacent_region_count + r2->adjacent_region_count > s->max_adjacent_regions ){
            make_saturated( r1 );
            make_saturated( r2 );
        }else{
            // remove adjacencies from r2 and add them to r1
            
            for( i=0; i < r2->adjacent_region_count; ++i ){
                Region *a = r2->adjacent_regions[ i ];

                replace_adjacent( a, r2, r1 ); // replace r2 with r1 in the adjacency list of a
                r1->adjacent_regions[r1->adjacent_region_count++] = a;

                assert( a->adjacent_region_count <= s->max_adjacent_regions );
                assert( r1->adjacent_region_count <= s->max_adjacent_regions );
            }

            r2->adjacent_region_count = 0;
        }
    }

    r1->flags |= r2->flags;

    if( r2->left < r1->left )
        r1->left = r2->left;

    if( r2->top < r1->top )
        r1->top = r2->top;

    if( r2->right > r1->right )
        r1->right = r2->right;

    if( r2->bottom > r1->bottom )
        r1->bottom = r2->bottom;

    // postcondition: all adjacencies to r1 are bidirectional

#ifndef NDEBUG
    for( i = 0; i < r1->adjacent_region_count; ++i ){
        Region *a = r1->adjacent_regions[i];
        assert( r1_adjacent_contains_r2( r1, a ) );
    }
#endif
}


static void build_regions( Segmenter *s, const unsigned char *source )
{
    //Span *new_span;
	int x, y, i;
    RegionReference **current_row = &s->regions_under_construction[0];
    RegionReference **previous_row = &s->regions_under_construction[s->width];

    s->region_ref_count = 0;
    s->region_count = 0;
    s->freed_regions_head = 0;

    // top line

    x = 0;
    y = 0;
    current_row[0] = new_region( s, x, y, source[0] );
    current_row[0]->region->flags |= ADJACENT_TO_ROOT_REGION_FLAG;
    for( x=1, y=0, i=1 ; x < s->width; ++x, ++i ){

        if( source[i] == source[i-1] ){
            current_row[x] = current_row[x-1];
        }else{
//			current_row[x-1]->region->last_span->end=i-1;
//			current_row[x-1]->region->area += i-current_row[x-1]->region->last_span->start;
            current_row[x] = new_region( s, x, y, source[i] );
            current_row[x]->region->flags |= ADJACENT_TO_ROOT_REGION_FLAG;
            make_adjacent( s, current_row[x]->region, current_row[x-1]->region );
        }
    }

    // process lines

    for( y=1; y < s->height; ++y ){

        // swap previous and current rows
        RegionReference **temp = previous_row;
        previous_row = current_row;
        current_row = temp;

        i = y * s->width;
        x = 0;

        // left edge

        RESOLVE_REGIONREF_REDIRECTS( previous_row[x], previous_row[x] );
        if( source[i] == previous_row[x]->region->colour ){
            current_row[x] = previous_row[x];
/*
			new_span = LOOKUP_SEGMENTER_SPAN( s,  i );
			new_span->start = i;
			new_span->end = i;
			new_span->next = NULL;
			current_row[x]->region->last_span->next = new_span;
			current_row[x]->region->last_span = new_span;
*/
		}else{ // source[i] != previous_row[x]->colour

            current_row[x] = new_region( s, x, y, source[i] );
            current_row[x]->region->flags |= ADJACENT_TO_ROOT_REGION_FLAG;
            make_adjacent( s, current_row[x]->region, previous_row[x]->region );
        }

        ++i;
        x=1;

        // center span

        for( ; x < s->width; ++x, ++i ){
            //RESOLVE_REGIONREF_REDIRECTS( current_row[x-1], current_row[x-1] );   // this isn't needed because the the west cell's redirect is always up to date
            RESOLVE_REGIONREF_REDIRECTS( previous_row[x], previous_row[x] );

            if( source[i] == source[i-1] ){

                current_row[x] = current_row[x-1];

                if( current_row[x] != previous_row[x]
                        && source[i] == previous_row[x]->region->colour ){

                    // merge the current region into the previous one
                    // this should be more efficient than merging the previous
                    // into the current because it keeps long-lived regions
                    // alive and only frees newer (less connected?) ones
/*
					previous_row[x]->region->last_span->next = current_row[x]->region->first_span;
					previous_row[x]->region->last_span = current_row[x]->region->last_span;
					previous_row[x]->region->area += current_row[x]->region->area;
*/
                    merge_regions( s, previous_row[x]->region, current_row[x]->region );
                    current_row[x]->region->flags = FREE_REGION_FLAG;
                    current_row[x]->region->next = s->freed_regions_head;
                    s->freed_regions_head = current_row[x]->region;
                    current_row[x]->region = 0;
                    current_row[x]->redirect = previous_row[x];
                    current_row[x] = previous_row[x];
                }


            }else{ // source_image_[i] != source_image_[i-1]
/*
				current_row[x-1]->region->last_span->end=i-1; // set the span end, it is more efficient here
				current_row[x-1]->region->area+=i-current_row[x-1]->region->last_span->start;
				
				// mark single pixels fragmented
				if (current_row[x-1]->region->area<=REGION_GATE_AREA) {
					if (((y+1)==s->height) || (source[i-1]!=source[i-1+s->width]))
						make_fragmented(current_row[x-1]->region);
				}
*/
                if( current_row[x-1]->region->right < x - 1 )
                    current_row[x-1]->region->right = (short)( x - 1 );

                if( source[i] == previous_row[x]->region->colour ){
                    current_row[x] = previous_row[x];
                    current_row[x]->region->bottom = (short)y;
/*
					new_span = LOOKUP_SEGMENTER_SPAN( s,  i );
					new_span->start = i;
					new_span->end = i;
					new_span->next = NULL;
					current_row[x]->region->last_span->next = new_span;
					current_row[x]->region->last_span = new_span;
*/
                }else{
                    current_row[x] = new_region( s, x, y, source[i] );
                    make_adjacent( s, current_row[x]->region, previous_row[x]->region );
                    if( current_row[x-1]->region != previous_row[x]->region )
                        make_adjacent( s, current_row[x]->region, current_row[x-1]->region );
                }
            }
        }

        // right edge
        current_row[s->width-1]->region->flags |= ADJACENT_TO_ROOT_REGION_FLAG;
//		current_row[s->width-1]->region->last_span->end=i;
//		current_row[x-1]->region->area+=i-current_row[x-1]->region->last_span->start+2;
    }

    // make regions of bottom row adjacent or merge with root

    for( x = 0; x < s->width; ++x ){
        RESOLVE_REGIONREF_REDIRECTS( current_row[x], current_row[x] );
        current_row[x]->region->flags |= ADJACENT_TO_ROOT_REGION_FLAG;
    }
}


/* -------------------------------------------------------------------------- */


void initialize_segmenter( Segmenter *s, int width, int height, int max_adjacent_regions )
{
    //max_adjacent_regions += 2; //workaround for #44
    s->max_adjacent_regions = max_adjacent_regions;
    s->region_refs = (RegionReference*)malloc( sizeof(RegionReference) * width * height );
    s->region_ref_count = 0;
    s->sizeof_region = sizeof(Region) + sizeof(Region*) * (max_adjacent_regions-1);
    s->regions = (unsigned char*)malloc( s->sizeof_region * width * height );
   // s->spans = (unsigned char*)malloc(  sizeof(Span) * width * height );
    s->region_count = 0;
	
	s->width = width;
	s->height = height;
	
    s->regions_under_construction = (RegionReference**)malloc( sizeof(RegionReference*) * width * 2 );
}

void terminate_segmenter( Segmenter *s )
{
    free( s->region_refs );
    free( s->regions );
	//free( s->spans );
    free( s->regions_under_construction );
}

void step_segmenter( Segmenter *s, const unsigned char *source )
{
    if( s->region_refs && s->regions && s->regions_under_construction /*&& s->spans*/) 
		build_regions( s, source );
}
