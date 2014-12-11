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

#include "threshold.h"

#include <math.h>
#include <stdlib.h>

void simple_threshold( const unsigned char *source, int source_stride,
        unsigned char *dest,
        int width, int height,
        int threshold )
{
    const unsigned char *p = source;
    int count = width * height;
    int i;
    
    for( i=0; i < count; ++i ){

        if( *p > threshold )
            dest[i] =  255;
        else
            dest[i] = 0;

        p += source_stride;
    }                            
}

void simple_adaptive_threshold( const unsigned char *source, int source_stride,
        unsigned char *dest,
        int width, int height,
        int tile_size )
{
    int x_step = tile_size;
    int y_step = tile_size;
    int y, x;
    
    for( y = 0; y < height; y += y_step ){
        for( x = 0; x < width; x += x_step ){
            int xxmax, yymax, sum, count;
            int yy, xx;
            int mean;
            
            xxmax = x + x_step;
            if( xxmax > width ) xxmax = width;
            
            yymax = y + y_step;
            if( yymax > height ) yymax = height;

            sum = 0;
            count = 0;
            for( yy = y; yy < yymax; ++yy ){
                for( xx = x; xx < xxmax; ++xx ){
                    sum += source[ (yy * width + xx) * source_stride ];
                    ++count;
                }
            }

            mean = sum / count;

/*
            double fmean = (double)sum / (double)count;

            double sd = 0;
            for( int yy = y; yy < yymax; ++yy ){
                for( int xx = x; xx < xxmax; ++xx ){
                    double z = source[ (yy * width + xx) * source_stride ] - fmean;
                    sd += z*z;
                }
            }

            double fsd = (double)sd / (double)count;
            fsd = sqrt( fsd );

            if( fsd < 10 )
                //mean = 128;
                mean += 20;
*/

            for( yy = y; yy < yymax; ++yy ){
                for( xx = x; xx < xxmax; ++xx ){
                    if( source[ (yy * width + xx) * source_stride ] > mean ){
                        dest[(yy * width + xx)] = 255;
                    }else{
                        dest[(yy * width + xx)] = 0;
                    }
                }
            }
        }
    }
}

#if 1
void overlapped_adaptive_threshold( const unsigned char *source, int source_stride,
        unsigned char *dest,
        int width, int height,
        int tile_size )
{
/*
    this could be optimized by only summing each quadrant of each tile once
    and reusing these sums for every half step. eg by maintaining an array
    of sums for the current and previous half-tile rows.
*/

    int x_step = tile_size;
    int y_step = tile_size;
    int x, y;

    for( y = 0; y < height; y += y_step / 2 ){
        for( x = 0; x < width; x += x_step / 2 ){
            int xxmax, yymax, sum, count, yy, xx;
            int mean;
            double fmean, sd, fsd;
            int ys, ymax, xs, xmax;
            
            xxmax = x + x_step;
            if( xxmax > width ) xxmax = width;
            
            yymax = y + y_step;
            if( yymax > height ) yymax = height;

            sum = 0;
            count = (yymax - y) * (xxmax - x );
            for( yy = y; yy < yymax; ++yy ){
                const unsigned char *p = source + (yy * width + x) * source_stride;
                for( xx = x; xx < xxmax; ++xx ){
                    sum += *p;
                    //++count;
                    p += source_stride;
                }
            }			

            mean = sum / count;

            fmean = (double)sum / (double)count;

            sd = 0;
            for( yy = y; yy < yymax; ++yy ){
                const unsigned char *p = source + (yy * width + x) * source_stride;
                for( xx = x; xx < xxmax; ++xx ){
                    double z = *p - fmean;
                    sd += z*z;
                    p += source_stride;
                }
            }

            fsd = (double)sd / (double)count;
            //fsd = sqrt( fsd );

            if( fsd < 100 )
                //mean = 128;
                mean -= 10;


            
            ys = y + y_step / 4;
            ymax = (y + y_step) - y_step / 4;
            if( ymax > yymax )
                ymax = yymax;
			if( y == 0 )
				ys = 0;

            xs = x + x_step / 4;
            xmax = (x + x_step) - x_step / 4;
            if( xmax > xxmax )
                xmax = xxmax;
			if( x == 0 )
				xs = 0;

            for( yy = ys; yy < ymax; ++yy ){
                const unsigned char *p = source + (yy * width + xs) * source_stride;
                for( xx = xs; xx < xmax; ++xx ){
                    if( *p > mean ){
                        dest[(yy * width + xx)] = 255;
                    }else{
                        dest[(yy * width + xx)] = 0;
                    }
                    p += source_stride;
                }
            }
        }
    }
}
#endif

#if 0
void overlapped_adaptive_threshold( const unsigned char *source, int source_stride,
        unsigned char *dest,
        int width, int height,
        int tile_size )
{
    int x, y, xstart, ystart, yend, xend;
    int diff;
    
    /*
        within each tile
            - get the maximum gradient
                if it's above a threshold
                    - use the average of max and min as the tile threshold
                else
                    - use a global threshold or zero the tile
    */


    for( ystart = 0; ystart < height; ystart += tile_size ){
        for( xstart = 0; xstart < width; xstart += tile_size ){
            int maxGradient = 0;
            int min = 255;
            int max = 0;
            
            yend = ystart + tile_size;
            xend = xstart + tile_size;

            for( y = ystart; y < yend; ++y ){
                for( x = xstart; x < xend; ++x ){
                    int hgradient, vgradient;
                    int src = source[ ( y * width + x ) * source_stride ];
                    if( src > max )
                        max = src;
                    if( src < min )
                        min = src;
                    /*
                    hgradient = src - source[ ( y * width + x + 1) * source_stride ];   // fixme -- reads 1 pixel past end

                    if( hgradient > maxGradient )
                        maxGradient = hgradient;
                    else if( -hgradient > maxGradient )
                        maxGradient = -hgradient;

                    vgradient = src - source[ ( (y+1) * width + x) * source_stride ];   // fixme -- reads 1 pixel past end
                    if( vgradient > maxGradient )
                        maxGradient = vgradient;
                    else if( -vgradient > maxGradient )
                        maxGradient = -vgradient;
                    */
                }
            }


            diff = min - max;
            if( diff < 0 )
                diff = -diff;
            //diff = abs( min - max );
            if( diff  > 30 ){
            //if( maxGradient > 20 ){
                int threshold = (min + max) / 2;
                for( y = ystart; y < yend; ++y ){
                    for( x = xstart; x < xend; ++x ){

                        if( source[ ( y * width + x ) * source_stride ] > threshold ){
                            dest[ y * width + x ] = 255;
                        }else{
                            dest[ y * width + x ] = 0;
                        }
                    }
                }
            }else{
                for( y = ystart; y < yend; ++y ){
                    for( x = xstart; x < xend; ++x ){
                        dest[ y * width + x ] = 255;
                    }
                }        
            }
        }
    }
}
#endif

#if 0 
void overlapped_adaptive_threshold( const unsigned char *source, int source_stride,
        unsigned char *dest,
        int width, int height,
        int tile_size )
{
    int x, y, xstart, ystart, yend, xend;
    int diff;
    
    /*
        within each tile
            - get the maximum gradient
                if it's above a threshold
                    - use the average of max and min as the tile threshold
                else
                    - use a global threshold or zero the tile
    */


    for( ystart = 0; ystart < height; ystart += tile_size ){
        for( xstart = 0; xstart < width; xstart += tile_size ){
            int maxGradient = 0;
            int min = 255;
            int max = 0;
            
            yend = ystart + tile_size;
            xend = xstart + tile_size;

            for( y = ystart; y < yend; ++y ){
                for( x = xstart; x < xend; ++x ){
                    int hgradient, vgradient;
                    int src = source[ ( y * width + x ) * source_stride ];
                    if( src > max )
                        max = src;
                    if( src < min )
                        min = src;

                    hgradient = src - source[ ( y * width + x + 1) * source_stride ];   // fixme -- reads 1 pixel past end

                    if( hgradient > maxGradient )
                        maxGradient = hgradient;
                    //else if( -hgradient > maxGradient )
                    //    maxGradient = -hgradient;

                    vgradient = src - source[ ( (y+1) * width + x) * source_stride ];   // fixme -- reads 1 pixel past end
                    if( vgradient > maxGradient )
                        maxGradient = vgradient;
                    //else if( -vgradient > maxGradient )
                    //    maxGradient = -vgradient;

                }
            }


            //diff = min - max;
            //if( diff < 0 )
            //    diff = -diff;
            //diff = abs( min - max );
            //if( diff  > 30 ){
            if( maxGradient > 20 ){
                int threshold = (min + max) / 2;
                for( y = ystart; y < yend; ++y ){
                    for( x = xstart; x < xend; ++x ){

                        if( source[ ( y * width + x ) * source_stride ] > threshold ){
                            dest[ y * width + x ] = 255;
                        }else{
                            dest[ y * width + x ] = 0;
                        }
                    }
                }
            }else{
                for( y = ystart; y < yend; ++y ){
                    for( x = xstart; x < xend; ++x ){
                        dest[ y * width + x ] = 255;
                    }
                }        
            }
        }
    }
}
#endif


#if 1
// adaptive gradient tracking using sobel gradient approximation
// http://www.cee.hw.ac.uk/hipr/html/sobel.html

// hardwired dimensions for now
#define TILE_ARRAY_HEIGHT   (480/16)
#define TILE_ARRAY_WIDTH    (640/16)

int sobel_tiles[ TILE_ARRAY_HEIGHT * TILE_ARRAY_WIDTH ];
unsigned char max_tiles[ TILE_ARRAY_HEIGHT * TILE_ARRAY_WIDTH ];
unsigned char min_tiles[ TILE_ARRAY_HEIGHT * TILE_ARRAY_WIDTH ];

// comment out the following line to use contrast thresholding instead
#define USE_GRADIENT_THRESHOLDING

void overlapped_adaptive_threshold2( const unsigned char *source, int source_stride,
        unsigned char *dest,
        int width, int height,
        int tile_size, int gradient_threshold )
{
    int x, y, xstart, ystart, yend, xend;
    //int a, b;
    
    int xtiles = width / tile_size;
    int ytiles = height / tile_size;
    int xt, yt;
    const unsigned char *pp1, *pp4, *pp7;
    unsigned char *pdest;
    int linestep, destlinestep;
    unsigned char max, min;
    int threshold;
    
    int approx_gradient_mag;
    int max_approx_gradient_mag;
    
#define REQUIRED_EXTRA	1 // 2 for sobel

    // calculate full 3x3 sobel for all but the right hand vertical,
    // and bottom horizontal tile strips.
    for( yt = 0; yt < ytiles; ++yt ){
        for( xt = 0; xt < xtiles; ++xt ){
        
            xstart = xt * tile_size;
            xend = xstart + tile_size;
			if( xend > width - REQUIRED_EXTRA )
				xend = width - REQUIRED_EXTRA;
			
			ystart = yt * tile_size;
            yend = ystart + tile_size;
			if( yend > height - REQUIRED_EXTRA )
				yend = height - REQUIRED_EXTRA;

            linestep = (width - (xend - xstart)) * source_stride;

            pp1 = source + (ystart * width + xstart) * source_stride;
            pp4 = pp1 + width * source_stride;
            pp7 = pp1 + width * source_stride;

            max_approx_gradient_mag = 0;
            max = 0;
            min = 255;
            
            for( y = ystart; y < yend; ++y ){
                for( x = xstart; x < xend; ++x ){

                    unsigned char p1 = pp1[0];

                /////////
				// this is just a simple edge detector
#ifdef USE_GRADIENT_THRESHOLDING
                    unsigned char p2 = pp1[source_stride];
                    unsigned char p4 = pp4[0];

                    approx_gradient_mag = abs( p2 - p1 ) + abs( p4 - p1 );

                    if( approx_gradient_mag > max_approx_gradient_mag )
                        max_approx_gradient_mag = approx_gradient_mag;

                    pp4 += source_stride;
#endif /* USE_GRADIENT_THRESHOLDING */
                /////////


                /*
				// this is the full sobel
                    unsigned char p2 = pp1[source_stride];
                    unsigned char p3 = pp1[source_stride * 2];

                    unsigned char p4 = pp4[0];
                    //unsigned char p5 = pp4[source_stride];
                    unsigned char p6 = pp4[source_stride * 2];

                    unsigned char p7 = pp7[0];
                    unsigned char p8 = pp7[source_stride];
                    unsigned char p9 = pp7[source_stride * 2];

                    approx_gradient_mag = abs( (p1 + 2 * p2 + p3)-(p7 + 2 * p8 + p9) )
                            + abs( (p3 + 2 * p6 + p9)-(p1 + 2 * p4 + p7) );

                    if( approx_gradient_mag > max_approx_gradient_mag )
                        max_approx_gradient_mag = approx_gradient_mag;

                    pp4 += source_stride;
                    pp7 += source_stride;
                */

                    if( p1 > max )
                        max = p1;
                    else if( p1 < min )
                        min = p1;

                    /*
                    if( approx_gradient_mag > 255 )
                        approx_gradient_mag = 255;

                    dest[ y * width + x ] = approx_gradient_mag;
                    */
                    pp1 += source_stride;

                }
                pp1 += linestep;
#ifdef USE_GRADIENT_THRESHOLDING
                pp4 += linestep;
                pp7 += linestep;
#endif /* USE_GRADIENT_THRESHOLDING */
            }

#ifdef USE_GRADIENT_THRESHOLDING
            sobel_tiles[ yt * TILE_ARRAY_WIDTH + xt ] = max_approx_gradient_mag;
#endif /* USE_GRADIENT_THRESHOLDING */

            max_tiles[ yt * TILE_ARRAY_WIDTH + xt ] = max;
            min_tiles[ yt * TILE_ARRAY_WIDTH + xt ] = min;
        }
    }
	
	// top left quater cell
	xstart = 0;
    xend = xstart + (tile_size/2);
    ystart = 0;
    yend = ystart + (tile_size/2);
    linestep = (width - (xend - xstart)) * source_stride;
    destlinestep = width - (xend - xstart);
    
    pp1 = source + (ystart * width + xstart) * source_stride;
    pdest = dest + (ystart * width + xstart);

#ifdef USE_GRADIENT_THRESHOLDING
    max_approx_gradient_mag = sobel_tiles[ 0 ];
    if( max_approx_gradient_mag > gradient_threshold )
#else
        if(1)
#endif
    {
    
        max = max_tiles[ 0];
        min = min_tiles[ 0 ];

#ifndef USE_GRADIENT_THRESHOLDING
        if( (max - min) > gradient_threshold )
#else
        if(1)
#endif
        {
            threshold = (max + min) / 2;

            for( y = ystart; y < yend; ++y ){
                for( x = xstart; x < xend; ++x ){

                    unsigned char p1 = pp1[0];
                    if( p1 > threshold )
                        *pdest = 255;
                    else
                        *pdest = 0;

                    pp1 += source_stride;
                    ++pdest;
                }
                pp1 += linestep;
                pdest += destlinestep;
            }
        }else{
            for( y = ystart; y < yend; ++y ){
                for( x = xstart; x < xend; ++x ){

                    *pdest = 0;
                
                    ++pdest;
                }
                pdest += destlinestep;
            }
        }

    }else{
        for( y = ystart; y < yend; ++y ){
            for( x = xstart; x < xend; ++x ){

                *pdest = 0;
                
                ++pdest;
            }
            pdest += destlinestep;
        }
    }


	

	/// top row of half tiles
    for( xt = 0; xt < xtiles - 1; ++xt ){

        xstart = xt * tile_size + (tile_size/2);
        xend = xstart + tile_size;
        ystart = 0;
        yend = ystart + (tile_size/2);
        linestep = (width - (xend - xstart)) * source_stride;
        destlinestep = width - (xend - xstart);
        
        pp1 = source + (ystart * width + xstart) * source_stride;
        pdest = dest + (ystart * width + xstart);

#ifdef USE_GRADIENT_THRESHOLDING
        max_approx_gradient_mag = sobel_tiles[ xt ];
        if( sobel_tiles[ xt + 1 ] > max_approx_gradient_mag )
            max_approx_gradient_mag = sobel_tiles[ xt + 1 ];

        if( max_approx_gradient_mag > gradient_threshold )
#else
        if(1)
#endif
        {
        
            max = max_tiles[ xt];
            if( max_tiles[ xt + 1 ] > max )
                max = max_tiles[ xt +  1 ];

            min = min_tiles[ xt ];
            if( min_tiles[ xt +  1 ] < min )
                min = min_tiles[ xt + 1 ];

#ifndef USE_GRADIENT_THRESHOLDING
            if( (max - min) > gradient_threshold )
#else
        if(1)
#endif
            {
                threshold = (max + min) / 2;

                for( y = ystart; y < yend; ++y ){
                    for( x = xstart; x < xend; ++x ){

                        unsigned char p1 = pp1[0];
                        if( p1 > threshold )
                            *pdest = 255;
                        else
                            *pdest = 0;

                        pp1 += source_stride;
                        ++pdest;
                    }
                    pp1 += linestep;
                    pdest += destlinestep;
                }
            }else{
                for( y = ystart; y < yend; ++y ){
                    for( x = xstart; x < xend; ++x ){

                        *pdest = 0;
                    
                        ++pdest;
                    }
                    pdest += destlinestep;
                }
            }

        }else{
            for( y = ystart; y < yend; ++y ){
                for( x = xstart; x < xend; ++x ){

                    *pdest = 0;
                    
                    ++pdest;
                }
                pdest += destlinestep;
            }
        }
    }


	// top right quater cell
	xstart = width - (tile_size/2) ;
    xend = xstart + (tile_size/2);
    ystart = 0;
    yend = ystart + (tile_size/2);
    linestep = (width - (xend - xstart)) * source_stride;
    destlinestep = width - (xend - xstart);
    
    pp1 = source + (ystart * width + xstart) * source_stride;
    pdest = dest + (ystart * width + xstart);

#ifdef USE_GRADIENT_THRESHOLDING
    max_approx_gradient_mag = sobel_tiles[ xtiles - 1 ];
    if( max_approx_gradient_mag > gradient_threshold )
#else
        if(1)
#endif
    {
    
        max = max_tiles[ xtiles - 1 ];
        min = min_tiles[ xtiles - 1 ];

#ifndef USE_GRADIENT_THRESHOLDING
        if( (max - min) > gradient_threshold )
#else
        if(1)
#endif
        {
            threshold = (max + min) / 2;

            for( y = ystart; y < yend; ++y ){
                for( x = xstart; x < xend; ++x ){

                    unsigned char p1 = pp1[0];
                    if( p1 > threshold )
                        *pdest = 255;
                    else
                        *pdest = 0;

                    pp1 += source_stride;
                    ++pdest;
                }
                pp1 += linestep;
                pdest += destlinestep;
            }
        }else{
            for( y = ystart; y < yend; ++y ){
                for( x = xstart; x < xend; ++x ){

                    *pdest = 0;
                
                    ++pdest;
                }
                pdest += destlinestep;
            }
        }

    }else{
        for( y = ystart; y < yend; ++y ){
            for( x = xstart; x < xend; ++x ){

                *pdest = 0;
                
                ++pdest;
            }
            pdest += destlinestep;
        }
    }


    for( yt = 0; yt < ytiles - 1; ++yt ){

		// first half tile
		xstart = 0;
        xend = (tile_size/2);
		ystart = yt * tile_size + (tile_size / 2);
        yend = ystart + tile_size;
		linestep = (width - (xend - xstart)) * source_stride;
        destlinestep = width - (xend - xstart);
            
        pp1 = source + (ystart * width + xstart) * source_stride;
        pdest = dest + (ystart * width + xstart);

#ifdef USE_GRADIENT_THRESHOLDING
		max_approx_gradient_mag = sobel_tiles[ yt * TILE_ARRAY_WIDTH ];
        if( sobel_tiles[ (yt + 1) * TILE_ARRAY_WIDTH ] > max_approx_gradient_mag )
            max_approx_gradient_mag = sobel_tiles[ (yt + 1) * TILE_ARRAY_WIDTH ];

		if( max_approx_gradient_mag > gradient_threshold )
#else
        if(1)
#endif
        {
        
            max = max_tiles[ yt* TILE_ARRAY_WIDTH ];
            if( max_tiles[ (yt + 1)* TILE_ARRAY_WIDTH ] > max )
                max = max_tiles[ (yt + 1)* TILE_ARRAY_WIDTH ];

            min = min_tiles[ yt * TILE_ARRAY_WIDTH ];
            if( min_tiles[ (yt + 1)* TILE_ARRAY_WIDTH ] < min )
                min = min_tiles[ (yt + 1)* TILE_ARRAY_WIDTH ];

#ifndef USE_GRADIENT_THRESHOLDING
            if( (max - min) > gradient_threshold )
#else
        if(1)
#endif
            {
                threshold = (max + min) / 2;

                for( y = ystart; y < yend; ++y ){
                    for( x = xstart; x < xend; ++x ){

                        unsigned char p1 = pp1[0];
                        if( p1 > threshold )
                            *pdest = 255;
                        else
                            *pdest = 0;

                        pp1 += source_stride;
                        ++pdest;
                    }
                    pp1 += linestep;
                    pdest += destlinestep;
                }
            }else{
                for( y = ystart; y < yend; ++y ){
                    for( x = xstart; x < xend; ++x ){

                        *pdest = 0;
                    
                        ++pdest;
                    }
                    pdest += destlinestep;
                }
            }

        }else{
            for( y = ystart; y < yend; ++y ){
                for( x = xstart; x < xend; ++x ){

                    *pdest = 0;
                    
                    ++pdest;
                }
                pdest += destlinestep;
            }
        }

		// row of full tiles
        for( xt = 0; xt < xtiles - 1; ++xt ){

            xstart = xt * tile_size + (tile_size/2);
            xend = xstart + tile_size;
            ystart = yt * tile_size + (tile_size/2);
            yend = ystart + tile_size;
            linestep = (width - (xend - xstart)) * source_stride;
            destlinestep = width - (xend - xstart);
            
            pp1 = source + (ystart * width + xstart) * source_stride;
            pdest = dest + (ystart * width + xstart);

#ifdef USE_GRADIENT_THRESHOLDING
            max_approx_gradient_mag = sobel_tiles[ yt * TILE_ARRAY_WIDTH + xt ];
            if( sobel_tiles[ (yt + 1) * TILE_ARRAY_WIDTH + xt ] > max_approx_gradient_mag )
                max_approx_gradient_mag = sobel_tiles[ (yt + 1) * TILE_ARRAY_WIDTH + xt ];
            if( sobel_tiles[ (yt + 1) * TILE_ARRAY_WIDTH + xt + 1 ] > max_approx_gradient_mag )
                max_approx_gradient_mag = sobel_tiles[ (yt + 1) * TILE_ARRAY_WIDTH + xt + 1 ];
            if( sobel_tiles[ yt * TILE_ARRAY_WIDTH + xt + 1 ] > max_approx_gradient_mag )
                max_approx_gradient_mag = sobel_tiles[ yt * TILE_ARRAY_WIDTH + xt + 1 ];
            if( max_approx_gradient_mag > gradient_threshold )
#else
        if(1)
#endif
            {
            
                max = max_tiles[ yt* TILE_ARRAY_WIDTH + xt ];
                if( max_tiles[ (yt + 1)* TILE_ARRAY_WIDTH + xt ] > max )
                    max = max_tiles[ (yt + 1)* TILE_ARRAY_WIDTH +  xt ];
                if( max_tiles[ (yt + 1)* TILE_ARRAY_WIDTH +  xt + 1 ] > max )
                    max = max_tiles[ (yt + 1)* TILE_ARRAY_WIDTH +  xt + 1 ];
                if( max_tiles[ yt * TILE_ARRAY_WIDTH + xt + 1] > max )
                    max = max_tiles[ yt * TILE_ARRAY_WIDTH +  xt + 1 ];

                min = min_tiles[ yt * TILE_ARRAY_WIDTH +  xt ];
                if( min_tiles[ (yt + 1)* TILE_ARRAY_WIDTH +  xt ] < min )
                    min = min_tiles[ (yt + 1)* TILE_ARRAY_WIDTH +  xt ];
                if( min_tiles[ (yt + 1)* TILE_ARRAY_WIDTH +  xt + 1 ] < min )
                    min = min_tiles[ (yt + 1)* TILE_ARRAY_WIDTH +  xt + 1 ];
                if( min_tiles[ yt * TILE_ARRAY_WIDTH +  xt + 1] < min )
                    min = min_tiles[ yt * TILE_ARRAY_WIDTH +  xt + 1 ];

#ifndef USE_GRADIENT_THRESHOLDING
                if( (max - min) > gradient_threshold )
#else
        if(1)
#endif
                {
                  threshold = (max + min) / 2;

                  for( y = ystart; y < yend; ++y ){
                      for( x = xstart; x < xend; ++x ){

                          unsigned char p1 = pp1[0];
                          if( p1 > threshold )
                              *pdest = 255;
                          else
                              *pdest = 0;

                          pp1 += source_stride;
                          ++pdest;
                      }
                      pp1 += linestep;
                      pdest += destlinestep;
                  }
                }else{
                    for( y = ystart; y < yend; ++y ){
                        for( x = xstart; x < xend; ++x ){

                            *pdest = 0;
                        
                            ++pdest;
                        }
                        pdest += destlinestep;
                    }
                }

            }else{
                for( y = ystart; y < yend; ++y ){
                    for( x = xstart; x < xend; ++x ){

                        *pdest = 0;
                        
                        ++pdest;
                    }
                    pdest += destlinestep;
                }
            }
        }


		// last half tile
		xstart = width - (tile_size/2);
        xend = width;
		ystart = yt * tile_size + (tile_size / 2);
        yend = ystart + tile_size;
		linestep = (width - (xend - xstart)) * source_stride;
        destlinestep = width - (xend - xstart);
            
        pp1 = source + (ystart * width + xstart) * source_stride;
        pdest = dest + (ystart * width + xstart);

#ifdef USE_GRADIENT_THRESHOLDING
		max_approx_gradient_mag = sobel_tiles[ yt * TILE_ARRAY_WIDTH + (xtiles - 1)  ];
        if( sobel_tiles[ (yt + 1) * TILE_ARRAY_WIDTH + (xtiles - 1) ] > max_approx_gradient_mag )
            max_approx_gradient_mag = sobel_tiles[ (yt + 1) * TILE_ARRAY_WIDTH + (xtiles - 1)  ];

		if( max_approx_gradient_mag > gradient_threshold )
#else
        if(1)
#endif
        {
        
            max = max_tiles[ yt* TILE_ARRAY_WIDTH + (xtiles - 1)  ];
            if( max_tiles[ (yt + 1)* TILE_ARRAY_WIDTH + (xtiles - 1)  ] > max )
                max = max_tiles[ (yt + 1)* TILE_ARRAY_WIDTH + (xtiles - 1)  ];

            min = min_tiles[ yt * TILE_ARRAY_WIDTH + (xtiles - 1)  ];
            if( min_tiles[ (yt + 1)* TILE_ARRAY_WIDTH + (xtiles - 1)  ] < min )
                min = min_tiles[ (yt + 1)* TILE_ARRAY_WIDTH + (xtiles - 1)  ];

#ifndef USE_GRADIENT_THRESHOLDING
            if( (max - min) > gradient_threshold )
#else
        if(1)
#endif
            {
                threshold = (max + min) / 2;

                for( y = ystart; y < yend; ++y ){
                    for( x = xstart; x < xend; ++x ){

                        unsigned char p1 = pp1[0];
                        if( p1 > threshold )
                            *pdest = 255;
                        else
                            *pdest = 0;

                        pp1 += source_stride;
                        ++pdest;
                    }
                    pp1 += linestep;
                    pdest += destlinestep;
                }
            }else{
                for( y = ystart; y < yend; ++y ){
                    for( x = xstart; x < xend; ++x ){

                        *pdest = 0;
                    
                        ++pdest;
                    }
                    pdest += destlinestep;
                }
            }

        }else{
            for( y = ystart; y < yend; ++y ){
                for( x = xstart; x < xend; ++x ){

                    *pdest = 0;
                    
                    ++pdest;
                }
                pdest += destlinestep;
            }
        }
    }



	// bottom left quater cell
	xstart = 0;
    xend = xstart + (tile_size/2);
    ystart = (ytiles - 1) * tile_size + (tile_size / 2);
    yend = ystart + (tile_size/2);
    linestep = (width - (xend - xstart)) * source_stride;
    destlinestep = width - (xend - xstart);
    
    pp1 = source + (ystart * width + xstart) * source_stride;
    pdest = dest + (ystart * width + xstart);

#ifdef USE_GRADIENT_THRESHOLDING
    max_approx_gradient_mag = sobel_tiles[ ((ytiles - 1) * TILE_ARRAY_WIDTH) ];

    if( max_approx_gradient_mag > gradient_threshold )
#else
        if(1)
#endif
    {
    
        max = max_tiles[ ((ytiles - 1) * TILE_ARRAY_WIDTH)];
        min = min_tiles[ ((ytiles - 1) * TILE_ARRAY_WIDTH) ];

#ifndef USE_GRADIENT_THRESHOLDING
        if( (max - min) > gradient_threshold )
#else
        if(1)
#endif
        {
            threshold = (max + min) / 2;

            for( y = ystart; y < yend; ++y ){
                for( x = xstart; x < xend; ++x ){

                    unsigned char p1 = pp1[0];
                    if( p1 > threshold )
                        *pdest = 255;
                    else
                        *pdest = 0;

                    pp1 += source_stride;
                    ++pdest;
                }
                pp1 += linestep;
                pdest += destlinestep;
            }
        }else{
            for( y = ystart; y < yend; ++y ){
                for( x = xstart; x < xend; ++x ){

                    *pdest = 0;
                
                    ++pdest;
                }
                pdest += destlinestep;
            }
        }

    }else{
        for( y = ystart; y < yend; ++y ){
            for( x = xstart; x < xend; ++x ){

                *pdest = 0;
                
                ++pdest;
            }
            pdest += destlinestep;
        }
    }


	/// bottom row of half tiles
    for( xt = 0; xt < xtiles - 1; ++xt ){

        xstart = xt * tile_size + (tile_size/2);
        xend = xstart + tile_size;
		ystart = (ytiles - 1) * tile_size + (tile_size / 2);
        yend = ystart + (tile_size/2);
        linestep = (width - (xend - xstart)) * source_stride;
        destlinestep = width - (xend - xstart);


        pp1 = source + (ystart * width + xstart) * source_stride;
        pdest = dest + (ystart * width + xstart);

#ifdef USE_GRADIENT_THRESHOLDING
        max_approx_gradient_mag = sobel_tiles[ ((ytiles - 1) * TILE_ARRAY_WIDTH) + xt ];
        if( sobel_tiles[ ((ytiles - 1) * TILE_ARRAY_WIDTH) + xt + 1 ] > max_approx_gradient_mag )
            max_approx_gradient_mag = sobel_tiles[ ((ytiles - 1) * TILE_ARRAY_WIDTH) + xt + 1 ];

        if( max_approx_gradient_mag > gradient_threshold )
#else
        if(1)
#endif
        {
        
            max = max_tiles[ ((ytiles - 1) * TILE_ARRAY_WIDTH) + xt];
            if( max_tiles[ ((ytiles - 1) * TILE_ARRAY_WIDTH) + xt + 1 ] > max )
                max = max_tiles[ ((ytiles - 1) * TILE_ARRAY_WIDTH) + xt +  1 ];

            min = min_tiles[ ((ytiles - 1) * TILE_ARRAY_WIDTH) + xt ];
            if( min_tiles[ ((ytiles - 1) * TILE_ARRAY_WIDTH) + xt +  1 ] < min )
                min = min_tiles[ ((ytiles - 1) * TILE_ARRAY_WIDTH) + xt + 1 ];

#ifndef USE_GRADIENT_THRESHOLDING
            if( (max - min) > gradient_threshold )
#else
        if(1)
#endif
            {
                threshold = (max + min) / 2;

                for( y = ystart; y < yend; ++y ){
                    for( x = xstart; x < xend; ++x ){

                        unsigned char p1 = pp1[0];
                        if( p1 > threshold )
                            *pdest = 255;
                        else
                            *pdest = 0;

                        pp1 += source_stride;
                        ++pdest;
                    }
                    pp1 += linestep;
                    pdest += destlinestep;
                }
            }else{
                for( y = ystart; y < yend; ++y ){
                    for( x = xstart; x < xend; ++x ){

                        *pdest = 0;
                    
                        ++pdest;
                    }
                    pdest += destlinestep;
                }
            }

        }else{
            for( y = ystart; y < yend; ++y ){
                for( x = xstart; x < xend; ++x ){

                    *pdest = 0;
                    
                    ++pdest;
                }
                pdest += destlinestep;
            }
        }
    }


	// bottom right quater cell
	xstart = width - (tile_size/2) ;
    xend = xstart + (tile_size/2);
    ystart = (ytiles - 1) * tile_size + (tile_size / 2);
    yend = ystart + (tile_size/2);
    linestep = (width - (xend - xstart)) * source_stride;
    destlinestep = width - (xend - xstart);
    
    pp1 = source + (ystart * width + xstart) * source_stride;
    pdest = dest + (ystart * width + xstart);

#ifdef USE_GRADIENT_THRESHOLDING
    max_approx_gradient_mag = sobel_tiles[ ((ytiles - 1) * TILE_ARRAY_WIDTH) +  xtiles - 1 ];

    if( max_approx_gradient_mag > gradient_threshold )
#else
        if(1)
#endif
    {
    
        max = max_tiles[ ((ytiles - 1) * TILE_ARRAY_WIDTH) + xtiles - 1 ];
        min = min_tiles[ ((ytiles - 1) * TILE_ARRAY_WIDTH) + xtiles - 1 ];

#ifndef USE_GRADIENT_THRESHOLDING
        if( (max - min) > gradient_threshold )
#else
        if(1)
#endif
        {
            threshold = (max + min) / 2;

            for( y = ystart; y < yend; ++y ){
                for( x = xstart; x < xend; ++x ){

                    unsigned char p1 = pp1[0];
                    if( p1 > threshold )
                        *pdest = 255;
                    else
                        *pdest = 0;

                    pp1 += source_stride;
                    ++pdest;
                }
                pp1 += linestep;
                pdest += destlinestep;
            }
        }else{
            for( y = ystart; y < yend; ++y ){
                for( x = xstart; x < xend; ++x ){

                    *pdest = 0;
                
                    ++pdest;
                }
                pdest += destlinestep;
            }
        }

    }else{
        for( y = ystart; y < yend; ++y ){
            for( x = xstart; x < xend; ++x ){

                *pdest = 0;
                
                ++pdest;
            }
            pdest += destlinestep;
        }
    }


}
#endif



