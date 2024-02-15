//----------------------------------------------------------------------
// Module: ruc13ll2xy.cc
//
// Author: Gerry Wiener
//
// Date:   8/26/2005
//
// Description:
//     Convert RUC13 lat-longs to xy coordinates in RUC13 grid
//     Compile with      g++ -I../include ruc13ll2xy.cc ../libdmapf.a -o ruc13ll2xy
//     
//----------------------------------------------------------------------

// Include files 
#include <stdio.h>
#include <dmapf/cmapf.h>
#include <string>
#include <vector>

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

using namespace std;

int MAX_LINE = 132;

main(int argc, char **argv)
{
  int i;
  double lat;
  double lon;
  char line[MAX_LINE];
  char site[MAX_LINE];
  maparam stcpm;
  double x;
  double y;
  vector<string> sites;
  vector<double> lats;
  vector<double> lons;

  if (argc < 2)
    {
      printf("usage: rucll2xy in_file\n");
      return(2);
    }

  char *in_file = argv[1];

  FILE *in_fp = fopen(in_file, "r");
  if (in_fp == NULL)
    {
      printf("Error: could not open %s\n", in_file);
      return(1);
    }

  while (1)
    {
      int ret;
      char  *pret;
      
      pret = fgets(line, MAX_LINE-1, in_fp);
      if (pret == NULL)
	break;

      ret = sscanf(line, "%s%lg%lg", site, &lat, &lon);
      if (ret == 3)
	{
	  sites.push_back(site);
	  lats.push_back(lat);
	  lons.push_back(lon);
	}
      else
	break;
    }
  fclose(in_fp);

  // Uses latitude where lambert conformal projection is true and
  // median aligned with cartesian y-axis
  stlmbr(&stcpm, 25.0, -95);

  // Use either stcm2p or stcm1p to set up the projection. The function stcm2p uses two points while
  // stcm1p uses one point and a latitude where the grid size is known

  // The next invocation uses two points. First specify their
  // coordinates in the grid and the corresponding latl-longs.
  //stcm2p(&stcpm, 0., 0., 16.281, -126.138, 450., 336., 55.481, -57.38); 

  // The next invocation uses one point, a latitude where the grid size is known
  stcm1p(&stcpm, 0.0, 0.0, 16.281, -126.138, 25.0, -95, 13.545, 0);

  for (int i=0; i<lats.size(); i++)
    {
	cll2xy(&stcpm, lats[i], lons[i], &x, &y);
	printf("%s %g %g %g %g\n", sites[i].c_str(), lats[i], lons[i], x, y);
    }

#ifdef NOTNOW  
  for (int j=0; j<225; j++)
    for (i=0; i<301; i++)
      {
	cll2xy(&stcpm, lat, lon, (double)i, (double)j, &lat, &lon);
	printf("[%d %d]: %g %g\n", j, i, lat, lon);
      }
#endif
}


