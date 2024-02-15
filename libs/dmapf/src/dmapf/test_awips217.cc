//----------------------------------------------------------------------
// Module: awips217.cc
//
// Author: Gerry Wiener
//
// Date:   12/11/2003
//
// Description:
//     Test dmap library.
//     Compile with      g++ -I../include awips217.cc ../libdmapf.a -o awips217
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
Nx 277
Ny 213
La1 = 30.00; latitude of lower left corner of grid
Lo1 = 187.00; longitude of lower left corner of grid
 Lov = 225.00; median aligned with cartesian y-axis
 Dx = 22500 ;
 Dy = 22500 ;
 Latin1 = 90.0 ; latitude at lambert conformal projection is true
 Latin2 = 90.0 ;
 ProjFlag = 0;
 SpLat = 0.0;
 SpLon = 0.0;
 ScanS2N = 1;
 */

int main(int argc, char **argv)
{
  int i;
  maparam stcpm;
  double lat;
  double lon;

  // Uses latitude where lambert conformal projection is true and
  // median aligned with cartesian y-axis
  stlmbr(&stcpm, 90.0, 225.0);

  // Use either stcm2p or stcm1p to set up the projection. The funciton stcm2p uses two points while
  // stcm1p uses one point and a latitude where the grid size is known

  // The next invocation uses two points. First specify their
  // coordinates in the grid and the corresponding latl-longs.
  //  stcm2p(&stcpm, 0., 0., 30.00, -173.00, 0., 212., 50.454, 143.597); 
  stcm2p(&stcpm, 0., 0., 30.00, -173.00, 276.0, 212.0, 70.111, -62.85); 

  // The next invocation uses one point, a latitude where the grid size is known
  //  stcm1p(&stcpm, 0.0, 0.0, 30.00, -173.00, 90.0, 225, 22.500, 0);
  for (int j=0; j<213; j++)
    for (i=0; i<277; i++)
      {
	cxy2ll(&stcpm, (double)i, (double)j, &lat, &lon);
	printf("[%d %d]: %g %g\n", i, j, lat, lon);
      }

  printf("lats:\n");

  // Print the lats
  for (int j=0; j<213; j++)
    for (i=0; i<277; i+=7)
      {
        int end;

        if (i + 7 >= 277)
          end = 277;
        else
          end = i + 7;

        for (int k=i; k<end; k++)
          {
            cxy2ll(&stcpm, (double)k, (double)j, &lat, &lon);
            printf("%g, ", lat);
          }

        printf("\n");
      }

  printf("lons:\n");

  // Print the lons
  for (int j=0; j<213; j++)
    for (i=0; i<277; i+=7)
      {
        int end;

        if (i + 7 >= 277)
          end = 277;
        else
          end = i + 7;

        for (int k=i; k<end; k++)
          {
            cxy2ll(&stcpm, (double)k, (double)j, &lat, &lon);
            printf("%g, ", lon);
          }
        printf("\n");
      }

  return 0;
}


