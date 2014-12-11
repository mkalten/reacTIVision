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
#ifndef INCLUDED_DUMP_GRAPH_H
#define INCLUDED_DUMP_GRAPH_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#include "segment.h"


/*
write the segmentation graph to the file fileName in AT&T Graphvis DOT format

white regions are green in the DOT graph,
each node is labeled with its address and the following qualifiers:
    _F_ -- fragmented
    _S_ -- saturated
    _AR_ -- adjacent to root
*/

void dump_graph( const char *fileName, Segmenter *s );


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* INCLUDED_DUMP_GRAPH_H */

