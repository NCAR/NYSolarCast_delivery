#include <stdio.h>
#include <string.h>
#include <netcdf.h>
#include "dmapf/cmapf.h"
#include "log/log.hh"
#include "emalloc.h"
#include "site_list.h"
#include "product_data.h"

extern Log *logFile;

/* Names for site-related variables in output file */
const char *MAX_SITE_NUM_NAME = "max_site_num";
const char *NUM_SITES_NAME = "num_sites";
const char *SITE_LIST_NAME = "site_list";
const char *LATITUDE_NAME = "lat";
const char *LONGITUDE_NAME = "lon";
const char *ELEVATION_NAME = "elev";


//
// Function to check corner points to see if any are missing which we
// cannot allow for bi-lin interpolation or gradient calculations.
//
// Returns 1 if any missing, 0 if none missing
//
int corner_missing(float vals[2][2], float fillval)
{
  // Check for missing data
  if (vals[0][0] == fillval || vals[1][0] == fillval ||
      vals[0][1] == fillval || vals[1][1] == fillval)
    return(1);

  return(0);
}


//
// Reads site list (ASCII) file, writes ID list and location info to 
// output file, then returns lat and lon position arrays for subsequent
// use. Returns 1 on success, 0 failure.
// 

int process_sites(char *sitefile, int ncid, float **lat_arr, float **lon_arr, int *ns)
{
  FILE *fp;
  const int MAX_LINE = 256;
  char in_line[MAX_LINE];
  int i, ret;
  int num_sites;
  size_t max_sites;
  char icao_id[11];
  //char name[40];
  char name[140];
  char state[3];
  char country[30];
  int *id;
  int wmo_id, region;
  float *elev, *lat, *lon;
  int varid, dimid;
  size_t corner[1];
  size_t count[1];


  /* Determine if site list has already been written. If so, read lat & lon
     arrays from output file */

  if (nc_inq_varid(ncid, NUM_SITES_NAME, &varid) != NC_NOERR)
    {
      logFile->write_time("Error: getting id for variable %s\n",
			  NUM_SITES_NAME);
      return(0);
    }

  corner[0] = 0;
  if (nc_get_var1_int(ncid, varid, corner, &num_sites) != NC_NOERR)
    {
      logFile->write_time("Error: getting variable %s\n", NUM_SITES_NAME);
      return(0);
    }

  if (num_sites != NC_FILL_INT)
    {
      corner[0] = 0;
      count[0] = num_sites;
      lat = (float *) emalloc(num_sites*sizeof(float));
      lon = (float *) emalloc(num_sites*sizeof(float));

      if (nc_inq_varid(ncid, LATITUDE_NAME, &varid) != NC_NOERR)
	{
	  logFile->write_time("Error: getting variable %s\n", LATITUDE_NAME);
	  return(0);
	}
      if (nc_get_vara_float(ncid, varid, corner, count, lat) != NC_NOERR)
	{
	  logFile->write_time("Error: getting variable %s\n", LATITUDE_NAME);
	  return(0);
	}
      
      if (nc_inq_varid(ncid, LONGITUDE_NAME, &varid) != NC_NOERR)
	{
	  logFile->write_time("Error: getting variable %s\n", LONGITUDE_NAME);
	  return(0);
	}
      if (nc_get_vara_float(ncid, varid, corner, count, lon) != NC_NOERR) 
	{
	  logFile->write_time("Error: getting variable %s\n", LONGITUDE_NAME);
	  return(0);
	}
      *lat_arr = lat;
      *lon_arr = lon;
      *ns = num_sites;
      return(1);
    }

  fp = fopen(sitefile, "r");
  if (!fp)
    {
      logFile->write_time("Error: could not open site file %s\n", sitefile);
      return(0);
    }

  /* Read entire file and count non-comment lines */

  num_sites = 0;
  while ((fgets(in_line, MAX_LINE, fp)) != NULL)
    if (in_line[0] != '#')
      num_sites++;


  /* Allocate space for site list struct and separate site list vars */

  id = (int *) emalloc(num_sites*sizeof(int));
  lat = (float *) emalloc(num_sites*sizeof(float));
  lon = (float *) emalloc(num_sites*sizeof(float));
  elev = (float*) emalloc(num_sites*sizeof(float));

  rewind(fp);

  /* Read each line into the site list array */

  i = 0;
  while ((fgets(in_line, MAX_LINE, fp)) != NULL)
    {
      if (in_line[0] != '#')
	{
	  ret = sscanf(in_line,"%d;%d;%10[^;];%f;%f;%f;%d;%[^;];%[^;];%[^\n]",
		       &id[i], &wmo_id, icao_id, &lat[i], &lon[i],
		       &elev[i], &region, name, state, country);
	  
	  if (ret != 10) {
	    logFile->write_time("Error: On site list entry %s\n", in_line);
	    return(0);
	  }
	  i++;
	}
    }

  fclose(fp);


  /* Get max allowed size of site list and compare to num_sites */

  if (nc_inq_dimid(ncid, MAX_SITE_NUM_NAME, &dimid) != NC_NOERR)
    {
      logFile->write_time("Error: getting dimension size for %s\n",
			  MAX_SITE_NUM_NAME);
      return(0);
    }
  ret = nc_inq_dimlen(ncid, dimid, &max_sites);

  if (num_sites > (int)max_sites)
    {
      logFile->write_time("Error: %s (%d) exceeds %s (%d)\n", NUM_SITES_NAME,
			  num_sites, MAX_SITE_NUM_NAME, max_sites);
      return(0);
  }

  /* Write num_sites and site_list */

  if (nc_inq_varid(ncid, NUM_SITES_NAME, &varid) != NC_NOERR)
    {
      logFile->write_time("Error: getting id for variable %s\n",
			  NUM_SITES_NAME);
      return(0);
    }
  if (nc_put_var_int(ncid, varid, &num_sites) != NC_NOERR)
    {
      logFile->write_time("Error: writing variable %s\n", NUM_SITES_NAME);
      return(0);
    }

  if (nc_inq_varid(ncid, SITE_LIST_NAME, &varid) != NC_NOERR)
    {
      logFile->write_time("Error: getting variable %s\n", SITE_LIST_NAME);
      return(0);
    }
  
  corner[0] = 0;
  count[0] = num_sites;

  if (nc_put_vara_int(ncid, varid, corner, count, id) != NC_NOERR)
    {
      logFile->write_time("Error: writing variable %s\n", SITE_LIST_NAME);
      return(0);
    }

  /* Write out lat, lon and elevation */

  if (nc_inq_varid(ncid, LATITUDE_NAME, &varid) != NC_NOERR)
    {
      logFile->write_time("Error: getting variable %s\n", LATITUDE_NAME);
      return(0);
    }
  if (nc_put_vara_float(ncid, varid, corner, count, lat) != NC_NOERR)
    {
      logFile->write_time("Error: writing variable %s\n", LATITUDE_NAME);
      return(0);
    }

  if (nc_inq_varid(ncid, LONGITUDE_NAME, &varid) != NC_NOERR)
    {
      logFile->write_time("Error: getting variable %s\n", LONGITUDE_NAME);
      return(0);
    }
  if (nc_put_vara_float(ncid, varid, corner, count, lon) != NC_NOERR) 
    {
      logFile->write_time("Error: writing variable %s\n", LONGITUDE_NAME);
      return(0);
    }

  if (nc_inq_varid(ncid, ELEVATION_NAME, &varid) != NC_NOERR)
    {
      logFile->write_time("Error: getting variable %s\n", ELEVATION_NAME);
      return(0);
    }
  if (nc_put_vara_float(ncid, varid, corner, count, elev) != NC_NOERR) 
    {
      logFile->write_time("Error: writing variable %s\n", ELEVATION_NAME);
      return(0);
    }

  free(id);
  free(elev);

  *lat_arr = lat;
  *lon_arr = lon;
  *ns = num_sites;

  return(1);
}


//
// Takes a grid and lat/lon arrays and calculates values at each location.
// What is calculated is governed by "calc_type". This can be a type of
// interpolation or a gradient in a particular direction. The product
// structure contains the data (grid) as well as the grid mapping information.
// The site_data array is already populated on input. Only sites that are in
// the grid area are updated on output. This allows processing  of grid
// "tiles". If tiles overlap (ie, a site is located on both grids), then data
// from the final tile are output.
//
// Returns 1 on success, 0 on failure.
//

int make_site_data(product_data *pd, float fillval, char *calc_type, float *lat_arr, float *lon_arr, int num_sites, float *site_data)
{
  int nx, ny;
  float la1, lo1, la2 = -9999, lo2, lov;
  float latin1, latin2;
  float iref, jref;
  float delx, dely;
  double x, y;
  maparam stcpm;
  int i, j, ns;
  float corner_vals[2][2];
  int x_corners[2];
  int y_corners[2];
  int ind;
  float xdist, ydist;
  double dx, dy;
  float interp[2];
  float grad[2];
  double ival;
  int wrap_flag = 0;


  // Set up grid navigation details. Currently, we can handle lat/lon,
  // rotated lat/lon (but only with 0 rotation angle), Lambert, and polar
  // stereographic grids. We use the dmapf library to do the lat-lon to
  // x-y grid coord translations except for lat/lon grids which we 
  // compute ourselves.

  switch(pd->gd->type)
    {
    case GRID_LL:
    case GRID_RLL:
      nx  = pd->gd->grid.ll.ni;
      ny  = pd->gd->grid.ll.nj;
      la1 = pd->gd->grid.ll.la1;
      lo1 = pd->gd->grid.ll.lo1;
      la2 = pd->gd->grid.ll.la2;
      lo2 = pd->gd->grid.ll.lo2;
      delx = pd->gd->grid.ll.di;
      dely = pd->gd->grid.ll.dj;

      // keep lo2 > lo1
      if (lo1 >= 180.) lo1 = lo1 - 360.;
      if (lo2 < lo1) lo2 = lo2 + 360.;

      // check if grid wraps the globe longitudinally
      if ((360.0 - fabs(lo2-lo1)) <= delx)  
	wrap_flag = 1;

      break;

    case GRID_GAU:

      // Treat this as a lat-lon grid. This is an approximation. Gaussian grids
      // have linear longitude spacing but unequal latitude spacing. However,
      // if we treat this as a linear grid, the error is only 1-2 thousandths
      // of a degree which is good enough for us.

      nx  = pd->gd->grid.gau.ni;
      ny  = pd->gd->grid.gau.nj;
      la1 = pd->gd->grid.gau.la1;
      lo1 = pd->gd->grid.gau.lo1;
      la2 = pd->gd->grid.gau.la2;
      lo2 = pd->gd->grid.gau.lo2;
      //delx = pd->gd->grid.gau.di;
      //dely = 90.0 / pd->gd->grid.gau.n;

      // keep lo2 > lo1
      if (lo1 >= 180.) lo1 = lo1 - 360.;
      if (lo2 < lo1) lo2 = lo2 + 360.;

      // Compute grid spacing using first and last points
      delx = fabs(lo2 - lo1) / (nx - 1);
      dely = fabs(la2 - la1) / (ny - 1);

      // check if grid wraps the globe longitudinally
      if ((360.0 - fabs(lo2-lo1)) <= delx)  
	wrap_flag = 1;

      break;

    case GRID_LAMBERT:
      nx  = pd->gd->grid.lambert.nx;
      ny  = pd->gd->grid.lambert.ny;
      la1 = pd->gd->grid.lambert.la1;
      lo1 = pd->gd->grid.lambert.lo1;
      lov = pd->gd->grid.lambert.lov;
      if (lo1 >= 180.) lo1 = lo1 - 360.;
      if (lov >= 180.) lov = lov - 360.;
      delx = pd->gd->grid.lambert.dx;
      dely = pd->gd->grid.lambert.dy;
      latin1 = pd->gd->grid.lambert.latin1;
      latin2 = pd->gd->grid.lambert.latin2;
      delx = delx / 1000; // convert to km
      dely = dely / 1000; // convert to km
      stlmbr(&stcpm, eqvlat(latin1, latin2), lov);
      stcm1p(&stcpm, 0.0, 0.0, la1, lo1, latin1, lov, delx, 0);
      break;

    case GRID_POLARS:
      nx  = pd->gd->grid.polars.nx;
      ny  = pd->gd->grid.polars.ny;
      la1 = pd->gd->grid.polars.la1;
      lo1 = pd->gd->grid.polars.lo1;
      lov = pd->gd->grid.polars.lov;
      if (lo1 >= 180.) lo1 = lo1 - 360.;
      if (lov >= 180.) lov = lov - 360.;
      delx = pd->gd->grid.polars.dx;
      dely = pd->gd->grid.polars.dy;
      latin1 = 60.0;  // The NWS defines this as their reference latitude
      delx = delx / 1000; // convert to km
      dely = dely / 1000; // convert to km
      sobstr(&stcpm, 90., 0.);
      stcm1p(&stcpm, 0.0, 0.0, la1, lo1, latin1, lov, delx, 0);
      break;

    default:
      logFile->write_time("Error: %s, cannot handle grid type %d\n",
	     pd->header, pd->gd->type);
      return(0);
    }

  iref = 0.;
  jref = 0.;

  // Loop over sites, determine x, y grid coordinate and dx, dy from lat, lon

  for (ns=0; ns<num_sites; ns++)
    {
      //printf("ns: %d\n", ns);

      // Handle lat/lon and rotated lat/lon grids here

      if (pd->gd->type == GRID_LL || pd->gd->type == GRID_RLL ||
	  pd->gd->type == GRID_GAU)
        {

	  float lat, lon;

	  // For rotated lat-lon, convert geographic lat-lon to
	  // rotated lat-lon. (RADPDEG, DEGPRAD from dmap library)
	  //
	  // Equations came from:
	  // http://www.emc.ncep.noaa.gov/mmb/research/FAQ-eta.html#rotatedlatlongrid
	  // Note this does not support a non-zero rotation angle. The
	  // script grib2ctl.pl (google it) does do this, but only in the
	  // opposite transform (rotated to geographic coords).

	  if (pd->gd->type == GRID_RLL)
	    {
	      
	      float polelatr = RADPDEG * (90.0 + pd->gd->grid.ll.rot->lat);
	      float polelonr = RADPDEG * pd->gd->grid.ll.rot->lon;

	      float latr = RADPDEG * lat_arr[ns];
	      float lonr = RADPDEG * lon_arr[ns];

	      float X = cos(polelatr) * cos(latr) * cos(lonr - polelonr) +
		sin(polelatr) * sin(latr);

	      float Y = cos(latr) * sin(lonr - polelonr);

	      float Z = -sin(polelatr) * cos(latr) * cos(lonr - polelonr) +
		cos(polelatr) * sin(latr);

	      lat = DEGPRAD * (atan ( Z / sqrt(X*X + Y*Y) ));
	      lon = DEGPRAD * (atan ( Y / X ));

	      if (X < 0) {
		lon = lon +  DEGPRAD * 180.0;
	      }
	    }
	  else
	    {
	      lon = lon_arr[ns];
	      lat = lat_arr[ns];
	    }

	  // Get x coordinate. Handle grid crossing dateline as special case.
	  if (lo1 >= 0. && lon < 0.)
	    x = (lon+360. - lo1)/delx + iref;
	  else
	    x = (lon - lo1)/delx + iref;

	  // Get y coordinate. if la1 < la2, grid is oriented south to north,
	  // if la1 > la2, it is oriented north to south
	  if (la1 < la2)
	    y = (lat - la1)/dely + jref;
	  else
	    y = (la1 - lat)/dely + jref;

	  // Adjust out-of-range points if grid wraps longitudinally
	  // Should not need this ?? (JC)
          //if ((nx * delx) >= 360.)
	  //{
	  //  while (x < 0.)
	  //    x += nx;

	  //  x = fmod(x, nx);
	  //}

	  // Get grid spacing in meters (REARTH, RADPDEG are from dmap library)
	  dy = (REARTH*1000.0) * (RADPDEG*dely);
	  dx = (REARTH*1000.0) * (RADPDEG*delx) * cos(RADPDEG*(lat));

	  // Check for upside-down (top to bottom) grid
	  if (la2 != -9999 && la2 < la1)
	    dy = -dy;

        }
      // All other projections here
      else
        {
          cll2xy(&stcpm, (double)lat_arr[ns], (double)lon_arr[ns], &x, &y);

	  // Get grid spacing in meters
          dx = cgszll(&stcpm, (double)lat_arr[ns], (double)lon_arr[ns]);
	  dx = dx * 1000.0;
	  dy = dx;
        }


      // Check that the location is on the grid. If a grid wraps 
      // the globe, we allow the x coord to go up to nx.
      if ((y < 0.) || (y > ny-1) || (x < 0.) ||
	  (x > nx && wrap_flag) || (x > nx-1 && !wrap_flag))
        {
	  logFile->write_time(3, "Info: site-index(ns): %d, lat %f, lon %f, x %f, y %f, value (off grid)\n", ns, lat_arr[ns], lon_arr[ns], x, y);
          continue;
        }

      // Find values at surrounding grid points.
      x_corners[0] = (int) floor(x);
      x_corners[1] = (int) ceil(x);
      y_corners[0] = (int) floor(y);
      y_corners[1] = (int) ceil(y);

      // Make sure upper corners are still in range
      if (x_corners[1] >= nx)
	x_corners[1] = x_corners[1] - nx;
      if (y_corners[1] >= ny)
	y_corners[1] = y_corners[1] - ny; 


      for (i=0; i<2; i++)
        for (j=0; j<2; j++) {
          ind = y_corners[j] * nx + x_corners[i];
          corner_vals[i][j] = pd->data[ind];
        }


      // Get the fractional distance to the grid point in the direction of
      // the origin
      xdist = (float) modf((double) x, &ival);
      ydist = (float) modf((double) y, &ival);
	  

      // Perform the calculation

      if (strcmp(calc_type, "bilinear") == 0)
	{
	  if (corner_missing(corner_vals, fillval))
	    continue;
	  
	  for (i=0; i<2; i++)
	    interp[i] = (xdist*corner_vals[1][i]) + (1-xdist)*corner_vals[0][i];

	  site_data[ns] = (ydist*interp[1]) + (1-ydist)*interp[0];
	}
      else if (strcmp(calc_type, "nearest_neighbor") == 0)
	{
	  i = 0;
	  j = 0;
	  if (xdist >= 0.5) i = 1;
	  if (ydist >= 0.5) j = 1;
	  site_data[ns] = corner_vals[i][j];
	}
      else if (strcmp(calc_type, "gradx") == 0)
	{
	  if (corner_missing(corner_vals, fillval))
	    continue;
	  
	  for (i=0; i<2; i++)
	    grad[i] = corner_vals[1][i] - corner_vals[0][i];
	  
	  site_data[ns] = ((ydist*grad[1]) + (1-ydist)*grad[0])/dx;
	}
      else if (strcmp(calc_type, "grady") == 0)
	{
	  if (corner_missing(corner_vals, fillval))
	    continue;

	  for (i=0; i<2; i++)
	    grad[i] = corner_vals[i][1] - corner_vals[i][0];
	  
	  site_data[ns] = ((xdist*grad[1]) + (1-xdist)*grad[0])/dy;
	}
      else
	{
	  logFile->write_time("Error: Invalid calc_type: '%s'\n", calc_type);
	  return (0);
	}

      // This can be used for just printing values
      //printf("%f\n", site_data[ns]);
      
      logFile->write_time(3, "Info: site-index (ns): %d, lat %7.2f, lon %7.2f, x %.2f, y %.2f, value %f\n", ns, lat_arr[ns], lon_arr[ns], x, y, site_data[ns]);
      logFile->write_time(4, "\tInfo: x[0] %d, x[1] %d\n", x_corners[0], x_corners[1]);
      logFile->write_time(4, "\tInfo: y[0] %d, y[1] %d\n", y_corners[0], y_corners[1]);
      logFile->write_time(4, "\tInfo: data at [x,y]: [0,1] %f, [1,1] %f\n", corner_vals[0][1], corner_vals[1][1]);
      logFile->write_time(4, "\tInfo: data at [x,y]: [0,0] %f, [1,0] %f\n", corner_vals[0][0], corner_vals[1][0]);
      logFile->write_time(4, "\tInfo: dx %f, dy %f\n", dx, dy);

    }

    return(1);
}
