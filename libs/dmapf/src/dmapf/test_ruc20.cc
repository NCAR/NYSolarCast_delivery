//----------------------------------------------------------------------
// Module: ruc40.cc
//
// Author: Gerry Wiener
//
// Date:   12/19/01
//
// Description:
//     Test dmap library.
//     Compile with      g++ -I../include ruc40.cc ../libdmapf.a -o ruc40
//     Check output against ruclatlon.asc
//     
//----------------------------------------------------------------------

// Include files 
#include <stdio.h>
#include <dmapf/cmapf.h>

// Constant, macro and type definitions 

// Global variables 

// Functions and objects

/*
La1 = 16.281; latitude of lower left corner of grid
 Lo1 = -126.138; longitude of lower left corner of grid
 Lov = -95.0; median aligned with cartesian y-axis
 Dx = 20318 ;
 Dy = 20318 ;
 Latin1 = 25.0 ; latitude at lambert conformal projection is true
 Latin2 = 25.0 ;
 ProjFlag = 0;
 SpLat = 0.0;
 SpLon = 0.0;
 ScanS2N = 1;
 */

int main(int argc, char **argv)
{
  int i;
  maparam stcpm1;
  maparam stcpm2;
  double lat;
  double lon;
  double x1;
  double x2;
  double y1;
  double y2;

  // Uses latitude where lambert conformal projection is true and
  // median aligned with cartesian y-axis
  stlmbr(&stcpm1, 25.0, -95);
  stlmbr(&stcpm2, 25.0, -95);

  // Use either stcm2p or stcm1p to set up the projection. The funciton stcm2p uses two points while
  // stcm1p uses one point and a latitude where the grid size is known

  // The next invocation uses two points. First specify their
  // coordinates in the grid and the corresponding latl-longs.
  stcm1p(&stcpm1, 0.0, 0.0, 16.281, -126.138, 25.0, -95, 20.318, 0);
  stcm2p(&stcpm2, 0., 0., 16.281, -126.138, 300., 224., 55.481, -57.38); 

  int bigi = 0;
  int bigj = 0;
  double dist = 0;
  double maxdist = 0;
  double maxx = 0;
  double maxy = 0;
  double avgx = 0;
  double avgy = 0;
  double dx;
  double dy;

  // The next invocation uses one point, a latitude where the grid size is known
  for (int j=0; j<225; j++)
    for (i=0; i<301; i++)
      {
	cxy2ll(&stcpm2, i, j, &lat, &lon);
	cll2xy(&stcpm1, lat, lon, &x1, &y1);
	cll2xy(&stcpm2, lat, lon, &x2, &y2);
	printf("lat, lon, x1, y1, x2, y2: %g %g %g %g %g %g\n", lat, lon, x1, y1, x2, y2);
	dx = fabs(x1-x2);
	dy = fabs(y1-y2);
	avgx += dx;
	avgy += dy;

	dist = hypot(dx,dy);
	if (dist > maxdist)
	  {
	    maxx = dx;
	    maxy = dy;
	    bigi = i;
	    bigj = j;
	    maxdist = dist;
	  }
      }

  printf("bigi %d, bigj %d, maxx %g, maxy %g, maxdist %g, avgx %g, avgy %g\n", bigi, bigj, maxx, maxy, maxdist, avgx/(301*225), avgy/(301*225));
  return 0;
}


