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
 Dx = 5080 ;
 Dy = 5080 ;
 Latin1 = 25.0 ; latitude at lambert conformal projection is true
 Latin2 = 25.0 ;
 ProjFlag = 0;
 SpLat = 0.0;
 SpLon = 0.0;
 ScanS2N = 1;
 */

main(int argc, char **argv)
{
  int i;
  maparam stcpm;
  double lat;
  double lon;

  // Uses latitude where lambert conformal projection is true and
  // median aligned with cartesian y-axis
  stlmbr(&stcpm, 25.0, -95);

  // Use either stcm2p or stcm1p to set up the projection. The funciton stcm2p uses two points while
  // stcm1p uses one point and a latitude where the grid size is known

  // The next invocation uses two points. First specify their
  // coordinates in the grid and the corresponding latl-longs.
  stcm2p(&stcpm, 0., 0., 16.281, -126.138, 1200., 896., 55.481, -57.38); 

  // The next invocation uses one point, a latitude where the grid size is known
  //stcm1p(&stcpm, 0.0, 0.0, 16.281, -126.138, 25.0, -95, 20.318, 0);
  FILE *fp = 0;

  fp = fopen("ruc5_lats.txt", "w");
  
  int ct = 0;
  for (int j=0; j<897; j++)
    {
      for (i=0; i<1201; i++)
        {
          ct++;
          cxy2ll(&stcpm, (double)i, (double)j, &lat, &lon);
          //printf("[%d %d]: %g %g\n", j, i, lat, lon);
          fprintf(fp, "%g, ", lat);
          if (ct % 8 == 0)
            fprintf(fp, "\n");
        }
    }
  fclose(fp);

  fp = fopen("ruc5_longs.txt", "w");
  ct = 0;
  for (int j=0; j<897; j++)
    for (i=0; i<1201; i++)
      {
        ct++;
	cxy2ll(&stcpm, (double)i, (double)j, &lat, &lon);
	//printf("[%d %d]: %g %g\n", j, i, lat, lon);

        fprintf(fp, "%g, ", lon);
        if (ct % 8 == 0)
          fprintf(fp, "\n");
      }
}

