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

#include "fidtrackX.h"

#include <math.h>
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

#define NOT_TRAVERSED       UNKNOWN_REGION_LEVEL
#define TRAVERSED           (NOT_TRAVERSED-1)
#define TRAVERSING          (NOT_TRAVERSED-2)


//#define LEAF_GATE_SIZE      0

static void set_depth( Region *r, short depth );


/* -------------------------------------------------------------------------- */


static double calculate_angle( double dx, double dy )
{
    double result;
	
    if( fabs(dx) > 0.001 ) {
		result = atan(dy/dx) - M_PI * .5;
		
		if( dx < 0 )
			result =  result - M_PI;
		
		if( result < 0 )
			result += 2. * M_PI;
	} else {
		if (dy>0) result = 0;
		else result = M_PI;
	}
	
    return result;
}

static void sum_leaf_centers( FidtrackerX *ft, Region *r, int width, int height )
{
    int i;
	float leaf_size;

    double radius = .5 + r->depth;
    double n = radius * radius * M_PI;  // weight according to depth circle area
	
    if( r->adjacent_region_count == 1 ) {
        float x, y;

		leaf_size = ((r->bottom-r->top) + (r->right-r->left)) * .5f;
		//printf("leaf: %f\n",leaf_size);
		ft->total_leaf_size += leaf_size;
		
        x = ((r->left + r->right) * .5f);
        y = ((r->top + r->bottom) * .5f);
		
		if( ft->pixelwarp ){
			int pixel = width*(int)y +(int)x;
			if ((pixel>=0) && (pixel<width*height)) {
				x = ft->pixelwarp[ pixel ].x;
				y = ft->pixelwarp[ pixel ].y;
			}

			if ((x>0) && (y>0)) {
				if( r->colour == 0 ){
					ft->black_x_sum_warped += x * n;
					ft->black_y_sum_warped += y * n;
					ft->black_leaf_count_warped += n;
				}else{
					ft->white_x_sum_warped += x * n;
					ft->white_y_sum_warped += y * n;
					ft->white_leaf_count_warped += n;
				}
			}
		} 
		
		if( r->colour == 0 ){
			ft->black_x_sum += x * n;
			ft->black_y_sum += y * n;
			ft->black_leaf_count += n;
		}else{
			ft->white_x_sum += x * n;
			ft->white_y_sum += y * n;
			ft->white_leaf_count += n;
		}
		
		ft->total_leaf_count +=n;
    }else{
        for( i=0; i < r->adjacent_region_count; ++i ){
            Region *adjacent = r->adjacent_regions[i];
            if( adjacent->level == TRAVERSED
                    && adjacent->descendent_count < r->descendent_count )
                sum_leaf_centers( ft, adjacent, width, height );
        }
    }
}

/*
static int check_leaf_variation( FidtrackerX *ft, Region *r, int width, int height )
{
	int i;
	if (ft->average_leaf_size<=2.0f) return 1;

    if( r->adjacent_region_count == 1 ){

		if (abs(ft->average_leaf_size-((r->bottom-r->top)+(r->right-r->left)/2))>ft->average_leaf_size) return 0;
		
     } else{
        for( i=0; i < r->adjacent_region_count; ++i ){
            Region *adjacent = r->adjacent_regions[i];
            if( adjacent->level == TRAVERSED
                    && adjacent->descendent_count < r->descendent_count )
                return check_leaf_variation( ft, adjacent, width, height );
        }
    }
	
	return 1;
}
*/

// for bcb compatibility
#ifndef _USERENTRY
#define _USERENTRY
#endif

int _USERENTRY depth_string_cmp(const void *a, const void *b)
{
    Region **aa = (Region**)a;
    Region **bb = (Region**)b;

    if( !(*aa)->depth_string ){
        if( !(*bb)->depth_string )
            return 0;
        else
            return 1;
    }else if( !(*bb)->depth_string ){
        return -1;
    }else{
        return -strcmp( (*aa)->depth_string, (*bb)->depth_string ); // left heavy order
    }
}


static char *build_left_heavy_depth_string( FidtrackerX *ft, Region *r )
{
    int i;
    char *result;
    Region *adjacent;
    char *p, *p2;

//mk    assert( ft->next_depth_string < ft->depth_string_count );
    result = &ft->depth_strings[ ft->depth_string_length * (ft->next_depth_string++) ];

    result[0] = (char)('0' + r->depth);
    result[1] = '\0';
    p = &result[1];

    if( r->adjacent_region_count != 1 ){

        for( i=0; i < r->adjacent_region_count; ++i ){
            adjacent = r->adjacent_regions[i];
            if( adjacent->level == TRAVERSED
                    && adjacent->descendent_count < r->descendent_count ){

                adjacent->depth_string = build_left_heavy_depth_string( ft, adjacent );
            }else{
                adjacent->depth_string = 0;
            }
        }

        qsort( r->adjacent_regions, r->adjacent_region_count, sizeof(Region*), depth_string_cmp ); 

        for( i=0; i < r->adjacent_region_count; ++i ){
            Region *adjacent = r->adjacent_regions[i];
            if( adjacent->depth_string ){
                p2 = adjacent->depth_string;
                while( *p2 )
                    *p++ = *p2++;

                adjacent->depth_string = 0;
            }
        }

        *p = '\0';
    }

    return result;
}

#ifndef NDEBUG
static void print_unordered_depth_string( Region *r )
{
    int i;
    Region *adjacent;

    printf( "(%d", r->depth );
    if( r->adjacent_region_count != 1 ){

        for( i=0; i < r->adjacent_region_count; ++i ){
            adjacent = r->adjacent_regions[i];
            if( adjacent->level == TRAVERSED
                    && adjacent->descendent_count < r->descendent_count ){

                print_unordered_depth_string( adjacent );
            }
        }
    }

    printf( ")" );
}
#endif

static void compute_fiducial_statistics( FidtrackerX *ft, FiducialX *f,
        Region *r, int width, int height )
{
    double all_x = 0.;
	double all_y = 0.;
    double black_x = 0.;
	double black_y = 0.;
	
    double all_x_warped = 0.;
	double all_y_warped = 0.;
    double black_x_warped = 0.;
	double black_y_warped = 0.;
    char *depth_string;

    ft->black_x_sum = 0.;
    ft->black_y_sum = 0.;
    ft->black_leaf_count = 0.;
    ft->white_x_sum = 0.;
    ft->white_y_sum = 0.;
    ft->white_leaf_count = 0.;

	ft->black_x_sum_warped = 0.;
    ft->black_y_sum_warped = 0.;
    ft->black_leaf_count_warped = 0.;
    ft->white_x_sum_warped = 0.;
    ft->white_y_sum_warped = 0.;
    ft->white_leaf_count_warped = 0.;
	
    ft->total_leaf_count = 0;
    ft->total_leaf_size = 0.;	
    ft->average_leaf_size = 0.;

//    ft->min_leaf_width_or_height = 0x7FFFFFFF;

    set_depth( r, 0 );
    sum_leaf_centers( ft, r, width, height );
    ft->average_leaf_size = ft->total_leaf_size / (double)(ft->total_leaf_count);

	all_x = (double)(ft->black_x_sum + ft->white_x_sum) / (double)(ft->black_leaf_count + ft->white_leaf_count);
	all_y = (double)(ft->black_y_sum + ft->white_y_sum) / (double)(ft->black_leaf_count + ft->white_leaf_count);
	
	black_x = (double)ft->black_x_sum / (double)ft->black_leaf_count;
	black_y = (double)ft->black_y_sum / (double)ft->black_leaf_count;
	
	if (ft->pixelwarp) {
		if (ft->total_leaf_count>(ft->black_leaf_count_warped+ft->white_leaf_count_warped)) { 
			int pixel = width*(int)all_y+(int)all_x;

			if ((pixel>=0) && (pixel<width*height)) {
				all_x_warped = ft->pixelwarp[pixel].x;
				all_y_warped = ft->pixelwarp[pixel].y;
				if ((all_x_warped>0) || (all_y_warped>0)) {

					pixel = (int)black_y*width+(int)black_x;
					if ((pixel>=0) && (pixel<width*height)) {
						black_x_warped = ft->pixelwarp[pixel].x;
						black_y_warped = ft->pixelwarp[pixel].y;
						if ((black_x_warped>0) || (black_y_warped>0)) {
							f->angle = calculate_angle( all_x_warped - black_x_warped, all_y_warped - black_y_warped );
						} else f->angle = 0.0f;
					} else f->angle = 0.0f;
						
					f->x = all_x_warped;
					f->y = all_y_warped;	
				} else {

					f->x = 0.0f;
					f->y = 0.0f;
					f->angle = 0.0f;
					r->flags |= LOST_SYMBOL_FLAG;
				}
			} else r->flags |= LOST_SYMBOL_FLAG;
		} else {
			all_x_warped = (double)(ft->black_x_sum_warped + ft->white_x_sum_warped) / (double)(ft->black_leaf_count_warped + ft->white_leaf_count_warped);
			all_y_warped = (double)(ft->black_y_sum_warped + ft->white_y_sum_warped) / (double)(ft->black_leaf_count_warped + ft->white_leaf_count_warped);
			
			black_x_warped = (double)ft->black_x_sum_warped / (double)ft->black_leaf_count_warped;
			black_y_warped = (double)ft->black_y_sum_warped / (double)ft->black_leaf_count_warped;
						
			f->x = all_x_warped;
			f->y = all_y_warped;
			f->angle = calculate_angle( all_x_warped - black_x_warped, all_y_warped - black_y_warped );
		}
		
	} else {
		f->x = all_x;
		f->y = all_y;
		f->angle = calculate_angle( all_x - black_x, all_y - black_y );
	}
	
	//f->a = black_x;
	//f->b = black_y;
   
    f->leaf_size = (float)(ft->average_leaf_size);
	f->root_size = r->right-r->left;
	if ((r->bottom-r->top)>f->root_size) f->root_size = r->bottom-r->top;
	f->root_colour=r->colour;
	f->node_count=r->descendent_count;

/*
    print_unordered_depth_string( r );
    printf( "\n" );
    fflush( stdout );
*/

/*
	// can differ due to fuzzy fiducial tracking
    assert( r->depth == 0 );
    assert( r->descendent_count >= ft->min_target_root_descendent_count );
    assert( r->descendent_count <= ft->max_target_root_descendent_count );
*/

	if (r->flags & LOST_SYMBOL_FLAG) f->id = INVALID_FIDUCIAL_ID;
	else {
        ft->next_depth_string = 0;
        depth_string = build_left_heavy_depth_string( ft, r );
			
        ft->temp_coloured_depth_string[0] = (char)( r->colour ? 'w' : 'b' );
        ft->temp_coloured_depth_string[1] = '\0';
        strcat( ft->temp_coloured_depth_string, depth_string );

		f->id = treestring_to_id( ft->treeidmap, ft->temp_coloured_depth_string );
		/*if (f->id != INVALID_FIDUCIAL_ID) {
			if (!(check_leaf_variation(ft, r, width, height)))  {
				f->id = INVALID_FIDUCIAL_ID;
				printf("filtered %f\n",ft->average_leaf_size);
			}
		}*/
    }

}

/* -------------------------------------------------------------------------- */

#define MAX( a, b ) (((a)>(b))?(a):(b))

// traverse downwards from r setting the depth value of each visited node
// depends on all nodes having assigned adjacent->descendent_count values
// to know which nodes to visit
static void set_depth( Region *r, short depth )
{
    int i;
    short child_depth = (short)(depth + 1);

    r->depth = depth;

    if( r->adjacent_region_count != 1 ){  // if not a leaf
        for( i=0; i < r->adjacent_region_count; ++i ){
            Region *adjacent = r->adjacent_regions[i];
            if( adjacent->descendent_count < r->descendent_count )
               set_depth( adjacent, child_depth );
        }
    }
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


// recursively propagate descendent count and max depth upwards
// r->visit_counter is incremented each time we have an opportunity
// to traverse to a parent. we only actually visit it once the visit
// counter reaches adjacent_region_count - 1 i.e. that we have already
// visited all of its other children
// as we travel upwards we collect nodes with the constraints defined by
// min_target_root_descendent_count, max_target_root_descendent_count,
// min_depth and max_depth
// we never traverse above a node which exceeds these constraints. we also
// never traverse nodes which are saturated or fragmented because it is
// ambiguous whether such a node has a parent, or if all it's children are
// attched, and we can't determine this in a single pass (we could save a list
// of these nodes for a later pass but we don't bother.)
// during the calls to this function we store the maximum leaf-to-node depth
// in r->depth, later this field has a different meaning
static void propagate_descendent_count_and_max_depth_upwards(
        Segmenter *s, Region *r, FidtrackerX *ft)
{
    int i;
    Region *parent = 0;

    assert( r->level == NOT_TRAVERSED );
    assert( r->children_visited_count == (r->adjacent_region_count - 1)         // has an untraversed parent 
            || r->children_visited_count == r->adjacent_region_count );   // is adjacent to root region

    r->descendent_count = 0;
    r->depth = 0;
    r->level = TRAVERSING;

    for( i=0; i < r->adjacent_region_count; ++i ){
        Region *adjacent = r->adjacent_regions[i];
        assert( r1_adjacent_contains_r2( adjacent, r ) );

        if( adjacent->level == TRAVERSED ){
            r->descendent_count += (short)(adjacent->descendent_count + 1);
            r->depth = (short)MAX( r->depth, (adjacent->depth + 1) );
        }else{
            assert( parent == 0 );
            parent = adjacent;
        }
    }

    r->level = TRAVERSED;

    if( r->descendent_count == ft->max_target_root_descendent_count
            && r->depth >= ft->min_depth && r->depth <= ft->max_depth ){

        // found fiducial candidate
        link_region( &ft->root_regions_head, r );
    }else{

        if( r->descendent_count >= ft->min_target_root_descendent_count
            && r->descendent_count < ft->max_target_root_descendent_count
            && r->depth >= ft->min_depth && r->depth <= ft->max_depth ) {
				link_region( &ft->root_regions_head, r );
       } else if( r->descendent_count >= ft->min_target_root_descendent_range
            && r->descendent_count < ft->max_target_root_descendent_range
            && r->depth >= ft->min_depth && r->depth <= ft->max_depth ) {
				r->flags |= LOST_SYMBOL_FLAG;
				link_region( &ft->root_regions_head, r );
       }
  

        if( parent
                && !(r->flags & (   SATURATED_REGION_FLAG |
                                    ADJACENT_TO_ROOT_REGION_FLAG |
                                    FREE_REGION_FLAG ) ) ){
        

            ++parent->children_visited_count;

            if( r->descendent_count < ft->max_target_root_descendent_count
                    && r->depth < ft->max_depth ){

                // continue propagating depth and descendent count upwards
                // so long as parent isn't a saturated node in which case it is
                // ambiguous whether parent has a parent or not so we skip it

                if(
                    !(parent->flags & SATURATED_REGION_FLAG)
                    &&
                    ( ( (!(parent->flags & (ADJACENT_TO_ROOT_REGION_FLAG | FRAGMENTED_REGION_FLAG) )
                        && parent->children_visited_count == (parent->adjacent_region_count - 1))
                    ||
                    ((parent->flags & (ADJACENT_TO_ROOT_REGION_FLAG | FRAGMENTED_REGION_FLAG) )
                        && parent->children_visited_count == parent->adjacent_region_count) ) )
                        ){

                    assert( r1_adjacent_contains_r2( r, parent ) );
                    assert( r1_adjacent_contains_r2( parent, r ) );

                    propagate_descendent_count_and_max_depth_upwards( s, parent, ft);
                }
            }
        }
    }
}





#ifndef NDEBUG
void sanity_check_region_initial_values( Segmenter *s )
{
    int i;
    for( i=0; i < s->region_count; ++i ){
        Region *r = LOOKUP_SEGMENTER_REGION( s, i );

        assert( r->level == NOT_TRAVERSED );
        assert( r->children_visited_count == 0 );
        assert( r->descendent_count == 0x7FFF );
    }
}
#endif


static void find_roots( Segmenter *s, FidtrackerX *ft)
{
    int i;

    // we depend on the segmenter initializing certain region fields for us
    // check that here
#ifndef NDEBUG
    sanity_check_region_initial_values( s );
#endif

    // find fiducial roots beginning at leafs

    for( i=0; i < s->region_count; ++i ){
        Region *r = LOOKUP_SEGMENTER_REGION( s, i );

        if( r->adjacent_region_count == 1
                && !(r->flags & (   SATURATED_REGION_FLAG |
                                    FRAGMENTED_REGION_FLAG |
                                    ADJACENT_TO_ROOT_REGION_FLAG |
                                    FREE_REGION_FLAG ) )
                ){

            assert( r->level == NOT_TRAVERSED );
            assert( r->children_visited_count == 0 );
            propagate_descendent_count_and_max_depth_upwards( s, r, ft);
       } 

    }
}

/* -------------------------------------------------------------------------- */


void initialize_fidtrackerX( FidtrackerX *ft, TreeIdMap *treeidmap, ShortPoint *pixelwarp )
{
	
    ft->min_target_root_descendent_count = treeidmap->min_node_count - 1;
    ft->max_target_root_descendent_count = treeidmap->max_node_count - 1;
	ft->min_target_root_descendent_range = ft->min_target_root_descendent_count - FUZZY_NODE_RANGE;
	if (ft->min_target_root_descendent_range<3) ft->min_target_root_descendent_range = 3;
    ft->max_target_root_descendent_range = ft->max_target_root_descendent_count + FUZZY_NODE_RANGE;
    ft->min_depth = treeidmap->min_depth;
    ft->max_depth = treeidmap->max_depth;

    ft->depth_string_count = treeidmap->max_node_count;
    ft->depth_string_length = treeidmap->max_node_count + 1;
    ft->depth_strings = (char*)malloc( ft->depth_string_count * ft->depth_string_length );

    ft->temp_coloured_depth_string = (char*)malloc( ft->depth_string_length + 1 );
    // includes space for colour prefix

    ft->treeidmap = treeidmap;
    ft->pixelwarp = pixelwarp;
}


void terminate_fidtrackerX( FidtrackerX *ft )
{
    free( ft->depth_strings );
    free( ft->temp_coloured_depth_string );
}


int find_fiducialsX( FiducialX *fiducials, int max_count,
        FidtrackerX *ft, Segmenter *segments, int width, int height)
{
    int i = 0;
    Region *next;

    initialize_head_region( &ft->root_regions_head );

    find_roots( segments, ft);

    next = ft->root_regions_head.next;
    while( next != &ft->root_regions_head ){

        compute_fiducial_statistics( ft, &fiducials[i], next, width, height );

        next = next->next;
        ++i;
        if( i >= max_count )
            return i;
    }

    return i;
}

int find_regionsX( RegionX *regions, int max_count,
        FidtrackerX *ft, Segmenter *segments, int width, int height, int min_size, int max_size)
{

	int i,j=0;
	int max_object_size,min_object_size = 0;
	int pixel = 0;
	
	if (max_size>height) max_object_size=height;
	else max_object_size = max_size; 

	if (min_size<2) min_object_size = 2;
	else min_object_size = min_size;

    initialize_head_region( &ft->root_regions_head );	
	
	for( i=0; i < segments->region_count; ++i ) {
		Region *r = LOOKUP_SEGMENTER_REGION( segments, i );

			regions[j].left = r->left;
			regions[j].right = r->right;
			regions[j].top = r->top;
			regions[j].bottom = r->bottom;

			regions[j].width = r->right-r->left+1;
			regions[j].height = r->bottom-r->top+1;
			regions[j].span = r->first_span;
			regions[j].area = r->area;
			regions[j].colour = r->colour;

			if ((regions[j].width>min_object_size) && (regions[j].width<max_object_size) && (regions[j].height>min_object_size) && (regions[j].height<max_object_size)) {
				regions[j].x = regions[j].left+regions[j].width/2;
				regions[j].y = regions[j].top+regions[j].height/2;

				if(ft->pixelwarp) {
					pixel = regions[j].y*width + regions[j].x;
					if ((pixel<0) || (pixel>=width*height)) continue;
					regions[j].x = ft->pixelwarp[ pixel ].x;
					regions[j].y = ft->pixelwarp[ pixel ].y;
					if ((regions[j].x==0) && (regions[j].y==0)) continue;
					
					/*pixel = (int)(width * regions[j].top + regions[j].left);
					if ((pixel<0) || (pixel>=width*height)) continue;
					p = &ft->pixelwarp[ pixel ];
					regions[j].left = p->x;
					regions[j].top = p->y;

					pixel = (int)(width * regions[j].bottom + regions[j].right);
					if ((pixel<0) || (pixel>=width*height)) continue;
					p = &ft->pixelwarp[ pixel ];
					regions[j].right = p->x;
					regions[j].bottom = p->y;*/
				} 

				j++;
				if (j==max_count) return j;
			} 
	}

	return j;
	
}
