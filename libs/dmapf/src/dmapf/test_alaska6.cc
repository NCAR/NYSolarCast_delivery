//----------------------------------------------------------------------
// Module: test_alaska6.cc
//
// Author: Jim Cowie
//
// Date:   07/17/06
//
// Description:
//     Test dmap library for Alaska 6km (NDFD) polar stereo grid. Set a
//     #define below to produce desired result. Possibilities are: print
//     lat/lons on the grid, print the corners, or determine if a lat/lon 
//     point is on the grid or not. (exit 0 if on grid, -1 if not). You can
//     also choose to do the grid mapping with the one-point method or the
//     two-point method, also determined with a #define.
//
//----------------------------------------------------------------------

// Define what this progarm will do.

//#define PRINT_LATLONS 1
#define PRINT_CORNER_LATLONS 1
//#define IS_POINT_IN_GRID 1

// Define which method is used

#define ONE_POINT_METHOD
//#define TWO_POINT_METHOD

//----------------------------------------------------------------------


// Include files 
#include <stdio.h>
#include <stdlib.h>
#include <dmapf/cmapf.h>

// Constant, macro and type definitions 

// Global variables - grid parameters

int nx = 825;
int ny = 553;
float la1 = 40.530;
float lo1 = -178.571;
float la2 = 63.975; // la2, lo2 came from Duane Carpenter at NWS 
float lo2 = -93.689;
float latin = 90.0;
float lad = 60.0;
float lov = 210.0;
float dx = 5.953000;
float dy = 5.953000;

double earth_radius = 6367.47; // value assumed by GRIB1
//double earth_radius = 6371.2;  // value used for current NDFD grids



// Functions and objects

int main(int argc, char **argv)
{
  int i;
  maparam stcpm;
  double lat, lon;
  double x, y;

  // The Alaska 6km NDFD grid is a polar stereographic projection, 
  // tangent at the North Pole.
  sobstr(&stcpm, latin, lov);

  cstrad(&stcpm, earth_radius);

  // Use either stcm2p or stcm1p to set up the projection. The function
  // stcm2p uses two points while stcm1p uses one point and a latitude
  // where the grid size is known. The details are:

  // call stcm1p(stcprm, p_x, p_y, p_lat, p_lon,
  //             xlatsp, xlonsp, gridsz, orient)
  //
  //      One-Point scale setting routine.  Establishes grid
  //      parameters so that grid point (p_x,p_y) will be pinned to
  //      geographic location (lat,long) = (p_lat,p_lon), while at
  //      the Scaling Point (lat,long) = (xlatsp,xlonsp), one grid unit
  //      is 'gridsz' km., and the y-axis is 'orient' degrees West of
  //       North.

  // call stcm2p(stcprm, p_x1, p_y1, p_lat1, p_lon1,
  //             p_x2, p_y2, p_lat2, p_lon2)
  //
  //      Two-Point scale setting routine. Establishes grid
  //      parameters so that grid point (p_x1,p_y1) will be pinned to
  //      geographic location (lat,long) = (p_lat1,p_lon1), while grid
  //      point (p_x2,P_y2) will be pinned to geographic location
  //      (lat,long) = (p_lat2,p_lon2).

#ifdef ONE_POINT_METHOD
  printf("Using the one-point method\n");
  stcm1p(&stcpm, 0., 0., la1, lo1, lad, lov, dx, 0);
#endif

#ifdef TWO_POINT_METHOD
  printf("Using the two-point method\n");
  stcm2p(&stcpm, 0., 0., la1, lo1, nx-1, ny-1, la2, lo2); 
#endif

#ifdef PRINT_LATLONS
  // Loop over grid points
  int ct = 0;
  printf("lats =\n");
  for (int j=0; j<ny; j++)
    {
      for (i=0; i<nx; i++)
        {
          ct++;
          cxy2ll(&stcpm, (double)i, (double)j, &lat, &lon);
          //printf("[%d %d]: %g %g\n", j, i, lat, lon);
          printf("%g, ", lat);
          if (ct % 8 == 0)
            printf("\n");
        }
    }
  printf(";\n");

  ct = 0;
  printf("\nlons =\n");
  for (int j=0; j<ny; j++)
    for (i=0; i<nx; i++)
      {
        ct++;
	cxy2ll(&stcpm, (double)i, (double)j, &lat, &lon);
	//printf("[%d %d]: %g %g\n", j, i, lat, lon);
        printf("%g, ", lon);
        if (ct % 8 == 0)
          printf("\n");
      }
  printf(";\n");
#endif // PRINT_LATLONS

#ifdef PRINT_CORNER_LATLONS
cxy2ll(&stcpm, (double)0., (double)0., &lat, &lon);
printf("lower left (%d,%d): %g %g\n", 0, 0, lat, lon);
cxy2ll(&stcpm, (double)0., (double)(ny-1), &lat, &lon);
printf("upper left (%d,%d): %g %g\n", ny-1, 0, lat, lon);
cxy2ll(&stcpm, (double)(nx-1), (double)(ny-1), &lat, &lon);
printf("upper right (%d,%d): %g %g\n", nx-1, ny-1, lat, lon);
cxy2ll(&stcpm, (double)(nx-1), (double)0., &lat, &lon);
printf("lower right (%d,%d): %g %g\n", 0, nx-1, lat, lon);
#endif // PRINT_CORNER_LATLONS

#ifdef IS_POINT_IN_GRID
// Note: This option requires lat and lon arguments on the command line
lat = atof(argv[1]);
lon = atof(argv[2]);
cll2xy(&stcpm, lat, lon, &x, &y);
printf("lat %g, lon %g, x, %g, y %g\n", lat, lon, x, y);
 if (x>=0 && x<nx && y>=0 && y<ny)
   exit(0);
 else
   exit(-1);
#endif // IS_POINT_IN_GRID
 return 0;
}
