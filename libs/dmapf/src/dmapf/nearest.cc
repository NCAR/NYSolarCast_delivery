//----------------------------------------------------------------------
// Module: nearest.cc
//
// Author: Gerry Wiener
//
// Date:   8/26/2005
//
// Description:
//     Read short site file with RUC20 coordinates. Output file marking closest
//     
//----------------------------------------------------------------------

// Include files 
#include <math.h>
#include <stdio.h>
#include <vector>
#include <string>

using namespace std;

// Constant, macro and type definitions 

// Global variables 

// Functions and objects

int MAX_LINE = 132;
int NX = 301;
int NY = 225;

main(int argc, char **argv)
{
  int i;
  double lat;
  double lon;
  char line[MAX_LINE];
  char site[MAX_LINE];
  vector<string> sites;
  vector<double> lats;
  vector<double> lons;
  double x;
  double y;
  vector<double> xc;
  vector<double> yc;

  if (argc < 4)
    {
      printf("usage: %s in_file out_file reject_file\n", argv[0]);
      return(2);
    }

  char *in_file = argv[1];
  char *out_file = argv[2];
  char *reject_file = argv[3];

  FILE *in_fp = fopen(in_file, "r");
  if (in_fp == NULL)
    {
      printf("Error: could not open %s\n", in_file);
      return(1);
    }

  FILE *out_fp = fopen(out_file, "w");
  if (out_fp == NULL)
    {
      printf("Error: could not open %s\n", out_file);
      return(1);
    }

  FILE *rej_fp = fopen(reject_file, "w");
  if (rej_fp == NULL)
    {
      printf("Error: could not open %s\n", reject_file);
      return(1);
    }

  while (1)
    {
      int ret;
      char  *pret;
      
      pret = fgets(line, MAX_LINE-1, in_fp);
      if (pret == NULL)
	break;

      ret = sscanf(line, "%s%lg%lg%lg%lg", site, &lat, &lon, &x, &y);
      if (ret == 5)
	{
	  sites.push_back(site);
	  lats.push_back(lat);
	  lons.push_back(lon);
	  xc.push_back(x);
	  yc.push_back(y);
	}
      else
	break;
    }
  fclose(in_fp);

  double closest_dist[NX * NY];
  int closest_ind[NX * NY];
  int closest_list[lats.size()];
  vector<int> grid_loc;

  for (int i=0; i<lats.size(); i++)
    {
      closest_list[i] = -1;
    }

  for (int i=0; i<NX * NY; i++)
    {
      closest_dist[i] = NX * NY;
      closest_ind[i] = -1;
    }

  for (int i=0; i<lats.size(); i++)
    {
      // Closest RUC pt
      double rx = round(xc[i]);
      double ry = round(yc[i]);

      if (0 <= rx && rx < NX && 0 <= ry && ry < NY)
	{
	  int loc = int(ry * NX + rx);
	  double dist = hypot(rx - xc[i], ry - yc[i]);

	  if (dist < closest_dist[loc])
	    {
	      if (closest_dist[loc] == NX * NY)
		{
		  grid_loc.push_back(loc);
		}
	      else
		{
		  int ind = closest_ind[loc];
		  fprintf(rej_fp, "%s %g %g %g %g %g %g %g\n", sites[ind].c_str(), lats[ind], lons[ind], xc[ind], yc[ind], round(xc[ind]), round(yc[ind]), dist);
		}

	      closest_dist[loc] = dist;
	      closest_ind[loc] = i;
	    }
	  else
	    {
	      fprintf(rej_fp, "%s %g %g %g %g %g %g %g\n", sites[i].c_str(), lats[i], lons[i], xc[i], yc[i], round(xc[i]), round(yc[i]), dist);
	    }
	}
      else
	{
	  printf("%s %g %g %g %g %g %g outside_grid\n", sites[i].c_str(), lats[i], lons[i], xc[i], yc[i], round(xc[i]), round(yc[i]));
	}
    }

  for (int i=0; i<grid_loc.size(); i++)
    {
      int loc = grid_loc[i];
      int ind = closest_ind[loc];
      fprintf(out_fp, "%s %g %g %g %g %g %g %g\n", sites[ind].c_str(), lats[ind], lons[ind], xc[ind], yc[ind], round(xc[ind]), round(yc[ind]), closest_dist[loc]);
      closest_list[ind] = 1;
    }

#ifdef NOTNOW
  for (int i=0; i<lats.size(); i++)
    {
      if (closest_list[i] == 1)
	{
	}
    }
#endif
}


