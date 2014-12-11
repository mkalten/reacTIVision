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

#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

#include "fidtrack120.h"

#include <math.h>
#include <assert.h>


typedef struct RegionPoint{
    Region *r;
    float x, y;
}RegionPoint;


static float squared_distance( RegionPoint* a, RegionPoint* b )
{
    float x = a->x - b->x;
    float y = a->y - b->y;
    return x*x + y*y;
}


static double calculate_angle( double dx, double dy )
{
    double result;

    if( fabs(dx) < 0.000001 )
        result = M_PI * .5;
    else
        result = fabs( atan(dy/dx) );

    if( dx < 0 )
        result = M_PI - result;

    if( dy < 0 )
        result = -result;

    if( result < 0 )
        result += 2. * M_PI;

    return result;
}


static int intersects( RegionPoint *A, RegionPoint *B, RegionPoint *C, RegionPoint *D )
{
    float a, b, c;

    float ABx = (B->x - A->x);
    float ABy = (B->y - A->y);

    float CDx = (D->x - C->x);
    float CDy = (D->y - C->y);

    a = CDx * ABy - CDy * ABx;

    if( a == 0. || a == -0. )
        return 0;

    b = (C->y - A->y) * ABx + (-C->x + A->x) * ABy;

    c = b / a;

    if (c >= 0. && c <= 1.)
        return 1;
    else
        return 0;
}

/* given a sequence of points [begin, end), find the three which are
    closest to a straight line.

    the algorithm used is as follows: permute through all combinations of
    3 point sequences. for each 3 point sequence form a triangle. the set
    of points which are considered to be on a straight line are those where
    the ratio of the hypotenuse to the sum of the two other sides is
    smallest. (ie the flattest triangle)
*/
static void find_straightest_line_of_3( RegionPoint *seq_begin, RegionPoint *seq_end,
    RegionPoint **line_start, RegionPoint **line_middle, RegionPoint **line_end )
{
    RegionPoint *i, *j, *k;
    float best_distance = 0x7FFFFFFF;
    float n;

    for( i = seq_begin; i != seq_end; ++i ){

        for( j = seq_begin; j != seq_end; ++j ){

            if( j != i ){
                for( k = seq_begin; k != seq_end; ++k ){

                    if( k != i && k != j ){

                        assert( i != j );
                        assert( j != k );
                        assert( i != k );

                        n = (float)(squared_distance( i, j ) + squared_distance( j, k ))
                                / (float)squared_distance( i, k );

                        if( n < best_distance ){
                            best_distance = n;
                            *line_start = i;
                            *line_middle = j;
                            *line_end = k;
                        }
                    }
                }
            }
        }
    }
}

/*
    length may not exceed 16
*/
static int has_all_symbols_once( int *sequence, int length )
{
    int i;
    int symbol;
    char symbols_present[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    assert( length < 16 );

    for( i=0; i < length; ++i ){
	symbol = sequence[i];
	if( symbol < 0 || symbol >= length )
		return 0; /* symbol value is out of range */
	else if( symbols_present[ symbol ] )
		return 0; /* symbol is duplicate */
	else
		symbols_present[ symbol ] = 1;
    }

    return 1;
}

/*
    see header file for a description of this function
*/
int fiducial120_id_from_symbol_sequence( int *sequence, int length )
{
    int id = 0;
    int factorial[8] = { 1, 1, 2, 6, 24, 120, 720, 5040 };
    int mapping[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    int i, j;

    assert( sequence[0] == 0 );
    assert( length <= 8 );

    /* ignore the first slot, which should be zero */
    sequence = sequence + 1;
    length = length - 1;

    for( i=0; i < length; ++i ){
        id += mapping[ (sequence[i]-1) ] * factorial[(length-1) - i];
        for( j = (sequence[i]-1) + 1; j < length; ++j ){
            mapping[j]--;
        }
    }

    return id;
}


static int extract_region_points( RegionPoint *dest, int max_count,
        Region *container, int level )
{
    int result = 0;
    int i;
    int j = 0;

    for( i=0; i < container->adjacent_region_count; ++i ){
        Region *r = container->adjacent_regions[i];
        if( r->level == level ){
            dest[j].r = r;
            /* compute container region centers (center of aligned bounding box) */
            dest[j].x = (float)(r->left + r->right) / 2.0f;
            dest[j].y = (float)(r->top + r->bottom) / 2.0f;
            ++result;
            assert( result <= max_count );
            ++j;
        }
    }

    return result;
}

#define SWAP( T, a, b ) { T temp___xxuusds = a; a = b; b = temp___xxuusds; }

static void compute_location_angle_and_id_for_6_symbol_fiducial( Fiducial120 *f, Region *r )
{
    RegionPoint points[6];
    RegionPoint points2[6];
    RegionPoint *i;
    RegionPoint *start1, *middle1, *end1, *start2, *middle2, *end2;
    int jj;

    assert( r->adjacent_region_count == 7 );

    /* load container region ptrs into points array */

    jj = extract_region_points( points, 6, r, FID_LEAF_CONTAINER_LEVEL );

    /* expect 6 container regions */
	assert( jj == 6 );

    /* find first line */
    find_straightest_line_of_3( &points[0], &points[6], &start1, &middle1, &end1 );

    /* remove first line from points list */
    {
        jj = 0;
        for( i = &points[0]; i != &points[6]; ++i ){
            if( i != start1 && i != middle1 && i != end1 )
                points2[jj++] = *i;
        }

        assert( jj == 3 );
    }

    /* find second line */
    find_straightest_line_of_3( &points2[0], &points2[3], &start2, &middle2, &end2 );


    /* reorder points so that start1 contains the 1 symbol (adjacent region count == 2) */

#define SYMBOL_1_ADJACENT_REGION_COUNT (2)

    if( start1->r->adjacent_region_count == SYMBOL_1_ADJACENT_REGION_COUNT ){

        /* nothing */

    }else if( end1->r->adjacent_region_count == SYMBOL_1_ADJACENT_REGION_COUNT ){

        SWAP( RegionPoint*, start1, end1 );

    }else{

        SWAP( RegionPoint*, start1, start2 );
        SWAP( RegionPoint*, middle1, middle2 );
        SWAP( RegionPoint*, end1, end2 );

        if( start1->r->adjacent_region_count == SYMBOL_1_ADJACENT_REGION_COUNT ){

            /* nothing */

        }else if( end1->r->adjacent_region_count == SYMBOL_1_ADJACENT_REGION_COUNT ){

            SWAP( RegionPoint*, start1, end1 );
        }
    }

    /* make start2 the point adjacent to end1 so that the codes can be read
        in clockwise order */    
    if( intersects( start1, end2, start2, end1 ) )
        SWAP( RegionPoint*, start2, end2 );

#if 0
    /* simple fiducial center calculation using only the middle point of each line */
    f->x = (middle1->x + middle2->x) / 2.0f;
    f->y = (middle1->y + middle2->y) / 2.0f;
#else
    /* center calculation using all points */
    f->x = ((start1->x + middle1->x + end1->x) + (start2->x + middle2->x + end2->x)) / 6.0f;
    f->y = ((start1->y + middle1->y + end1->y) + (start2->y + middle2->y + end2->y)) / 6.0f;
#endif


#if 0
    /* simple angle calculation using only the line start1->end1 */
    {
        double dx = end1->x - start1->x;
        double dy = -(end1->y - start1->y);
        f->angle = (float)calculate_angle( dx, dy );
    }
#else
    /* angle calculation by averaging most available angles */
    {
        /* vectors parallel to longer side of the fiducial */

        double dx = end1->x - start1->x;
        double dy = -(end1->y - start1->y);

        dx += middle1->x - start1->x;
        dy += -(middle1->y - start1->y);

        dx += end1->x - middle1->x;
        dy += -(end1->y - middle1->y);

        dx += start2->x - end2->x;
        dy += -(start2->y - end2->y);

        dx += middle2->x - end2->x;
        dy += -(middle2->y - end2->y);

        dx += start2->x - middle2->x;
        dy += -(start2->y - middle2->y);

        /* vectors parallel to the shorter side of the fiducial, rotated 90 degrees */

        dx += start1->y - end2->y;
        dy += start1->x - end2->x;

        dx += middle1->y - middle2->y;
        dy += middle1->x - middle2->x;

        dx += end1->y - start2->y;
        dy += end1->x - start2->x;

        dx /= 9.;
        dy /= 9.;

        f->angle = (float)calculate_angle( dx, dy );
    }
#endif

    /* compute fiducial id */

    assert( MAX_FIDUCIAL120_SYMBOLS >= 6 );

    /* convert adjacent_region_count to 0 based symbol id by subtracting
	2 -- 1 for the root container, and 1 because there is always one
	leaf to encode a symbol (eg it is 1 based)
    */

    f->symbol_sequence[0] = start1->r->adjacent_region_count - 2;
    f->symbol_sequence[1] = middle1->r->adjacent_region_count - 2;
    f->symbol_sequence[2] = end1->r->adjacent_region_count - 2;
    f->symbol_sequence[3] = start2->r->adjacent_region_count - 2;
    f->symbol_sequence[4] = middle2->r->adjacent_region_count - 2;
    f->symbol_sequence[5] = end2->r->adjacent_region_count - 2;
    f->sequence_length = 6;

    if( f->symbol_sequence[0] == 0 && has_all_symbols_once( f->symbol_sequence, f->sequence_length ) )        
        f->id = fiducial120_id_from_symbol_sequence( f->symbol_sequence, f->sequence_length );
    else
        f->id = INVALID_FIDUCIAL120_ID;
}

/* -------------------------------------------------------------------------- */

int find_fiducials120( Fiducial120 *fiducials, int count,
        PartialSegmentTopology *pst, Segmenter *segments )
{
    int i = 0;
    Region *next;

    initialize_segment_topology( pst, segments, 6 );

    next = pst->root_regions_head.next;
    while( next != &pst->root_regions_head ){

        compute_location_angle_and_id_for_6_symbol_fiducial( &fiducials[i], next );

        next = next->next;
        ++i;
        if( i >= count )
            return i;
    }

    return i;
}
