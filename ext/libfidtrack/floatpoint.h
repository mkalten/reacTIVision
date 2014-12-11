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

#ifndef INCLUDED_FLOATPOINT_H
#define INCLUDED_FLOATPOINT_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


typedef struct FloatPoint{
    float x, y;

}FloatPoint;

typedef struct DoublePoint{
    short x, y;

}DoublePoint;

typedef struct IntPoint{
    short x, y;

}IntPoint;

typedef struct ShortPoint{
    short x, y;

}ShortPoint;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* INCLUDED_FLOATPOINT_H */
