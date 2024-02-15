/*
 *   Copyright 1995, University Corporation for Atmospheric Research.
 *   See ../COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: ncfloat.cc,v 1.6 2013/11/22 19:07:49 dicast Exp $ */

/*
 * Harry Edmon's nc_float package, adapted for use in gribtonc with additions
 * that provide a wrapper for ncvarputg calls.
 */
  
#include <limits.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <netcdf.h>
#include "log/log.hh"

#include "units.h"
#include "ncfloat.h"
#include "emalloc.h"

extern Log *logFile;
extern int ncerr;

/*
  This routine calculates the slope and intercept necessary to convert from 
  the units of the given netCDF variable to the specified units.  If units
  is blank, the netCDF units are returned and the slope is set to one and 
  the intercept to 0.
  */

int
nc_units(
    int ncid,			/* input netCDF id */
    int varid,			/* input netCDF variable id */
    char *units,		/* units specifier, returned if blank */
    double *slope,		/* output units conversion factor */
    double *intercept		/* output units conversion additive offset */
	)
{
    size_t ulen;
    char nc_units[100];
    utUnit funits, tunits;

    *slope = 1.0; *intercept = 0.0;
    if (units == NULL) return(0);
    if (nc_inq_att(ncid, varid, UNITS_NAME, NULL, &ulen) != NC_NOERR ||
	nc_get_att_text(ncid, varid, UNITS_NAME, nc_units) != NC_NOERR) {
	logFile->write_time("Error: Cannot get data units\n");
	return -1;
    }
    nc_units[ulen] = '\0';
/*
  Prepare for possible units conversion.
  */
    if (units[0]) {
/*
  Prepare udunits.
  */
	if (init_udunits()) {
	    logFile->write_time("Error: Cannot initialize udunits library\n");
	    return -1;
	}
	if (utScan(units, &tunits) == 0 
	    && utScan(nc_units, &funits) == 0) {
	    int ret = utConvert(&funits, &tunits, slope, intercept);
	    if (ret == UT_ECONVERT) {
		logFile->write_time("Error: Units `%s' and `%s' are not conformable\n", units, nc_units);
		return -1;
	    }
	}
    }
    else if (units) {
/*
  No units specified - return units in the netCDF file.
  */
	strcpy(units, nc_units);
    }
    return(0);
}

/*
  This routine is an interface to ncvarget that converts the data from 
  the netCDF to floating point with the given missing value and units.

  ** it has been reduced substantially for netcdf-3 API. JRC

*/

int
nc_float(
    int ncid,
    int varid,
    size_t *corn,
    size_t *edge,
    float *data,
    float missing,
    double slope,
    double intercept
	)
{

  int i;
  int ndims;
  size_t size = 1;

  // Get ndims
  if (nc_inq_varndims(ncid, varid, &ndims) != NC_NOERR) {
    logFile->write_time("Error: Could not get ndims from ncfile\n");
    return -1;
  }

  for (i=0; i < ndims; ++i) {
    (i == 0) ? size = edge[0]: size *= edge[i];
  }

  // Read the data    
  if (nc_get_vara_float(ncid, varid, corn, edge, data) != NC_NOERR) {
    logFile->write_time("Error: Could not get data from nc file\n");
    return -1;
  }

  // Convert to original units
  if (slope != 1.0) {
    if (intercept != 0.) {
      for(i=0; i<(int)size; i++)
	if (data[i] != missing)
	  data[i] = slope * data[i] + intercept;
    }
    else
      for(i=0; i<(int)size; i++)
	if (data[i] != missing)
	  data[i] *= slope;
  }
  else if (intercept != 0.)
    for (i=0; i < (int)size; i++)
      if (data[i] != missing)
	data[i] += intercept;

  return(0);

}


/*
  This routine is an interface to ncvarput that converts the data to
  the netCDF from floating point with the given missing value and units.
*/
int
float_nc(
    int ncid,
    int varid,
    size_t *corn,
    size_t *edge,
    float *data,
    double slope,
    double intercept,
    float missing
	)
{
  
  int i;
  int ndims;
  size_t size = 1;
    
  // Get ndims
  if (nc_inq_varndims(ncid, varid, &ndims) != NC_NOERR) {
    logFile->write_time("Error: Could not get ndims from ncfile\n");
    return -1;
  }

  for (i=0; i < ndims; ++i) {
    (i == 0) ? size = edge[0]: size *= edge[i];
  }

  // Convert the units.
  if (slope != 1.0 || intercept != 0.0) {
    if (slope != 1.0) {
      if (intercept != 0.) {
	for(i=0; i<(int)size; i++) {
	  if (data[i] == missing) data[i] = data[i];
	  else data[i] = slope * data[i] + intercept;
	}
      }
      else
	for(i=0; i<(int)size; i++) {
	  if (data[i] == missing) data[i] = data[i];
	  else data[i] = data[i] * slope;
	}
    }
    else {
      for (i=0; i < (int)size; i++) {
	if (data[i] == missing) data[i] = data[i];
	else data[i] = data[i] + intercept;
      }
    }
  }
	
  // put the data
  if (nc_put_vara_float(ncid, varid, corn, edge, data) != NC_NOERR) {
    logFile->write_time("Error: nc_put_vara_float failed\n");
    return -1;
  }
  return 0;
}




#ifdef DONT_NEED_FOR_SITE_DATA

/*
  This routine is an interface to ncvarputg that converts the data to
  the netCDF from floating point with the given missing value and units.
*/
int
float_ncg(
    int ncid,
    int varid,
    long *corn,
    long *edge,
    long *stride,
    long *imap,
    float *data,
    double slope,
    double intercept,
    float missing
	)
{
    int ndims, i, prod=1, rangerr=0, check_miss=0;
    nc_type datatype;
    void *dd;
    char *c, *cf;
    short *s, *sf;
    long *l, *lf;
    float *f, *ff, *datac;
    double *d, miss;
    
    if (ncvarinq(ncid, varid, NULL, &datatype, &ndims, NULL, NULL) == -1) {
	logFile->write_time("Error: Cannot get data type\n");
	return -1;
    }
    for (i=0; i < ndims; ++i) prod *= edge[i];
/*
  Convert the units.
  */
    if (slope == 1.0 && intercept == 0.0) datac = data;
    else {
	datac = (float *) emalloc(prod * sizeof(float));
	if (slope != 1.0) {
	    if (intercept != 0.) {
		for(i=0; i<prod; i++) {
		    if (data[i] == missing) datac[i] = data[i];
		    else datac[i] = slope * data[i] + intercept;
		}
	    }
	    else
		for(i=0; i<prod; i++) {
		    if (data[i] == missing) datac[i] = data[i];
		    else datac[i] = data[i] * slope;
	    }
	}
	else {
	    for (i=0; i < prod; i++) {
		if (data[i] == missing) datac[i] = data[i];
		else datac[i] = data[i] + intercept;
	    }
	}
    }
	
/*
  Create temporary array and store
  */
    if (ncattget(ncid, varid, _FillValue, &miss) == -1) check_miss++;
    switch (datatype) {

    case NC_CHAR: 
    case NC_BYTE:
	cf = (char *) &miss;
	c=(char *) emalloc(prod*sizeof(char));
	for (i=0; i < prod; ++i) {
	    if (check_miss && datac[i] == missing) c[i] = *cf;
	    else if (data[i] < CHAR_MIN || data[i] > CHAR_MAX) {
		rangerr = 1;
		c[i] = *cf;
	    }
	    else c[i] = nint(datac[i]);
	}
	dd = (void *) c;
	break;
    case NC_SHORT:
	sf = (short *) &miss;
	s=(short *) emalloc(prod*sizeof(short));
	for (i=0; i < prod; ++i) {
	    if (check_miss && datac[i] == missing) s[i] = *sf;
	    else if (data[i] < SHRT_MIN || data[i] > SHRT_MAX) {
		rangerr = 1;
		s[i] = *sf;
	    }
	    else s[i] = nint(datac[i]);
	}
	dd = (void *) s;
	break;
    case NC_LONG:
	lf = (long *) &miss;
	l=(long *) emalloc(prod*sizeof(long));
	for (i=0; i < prod; ++i) {
	    if (check_miss && datac[i] == missing) l[i] = *lf;
	    else if (data[i] < LONG_MIN || data[i] > LONG_MAX) {
		rangerr = 1;
		l[i] = *lf;
	    }
	    else l[i] = nint(datac[i]);
	}
	dd = (void *) l;
	break;
    case NC_FLOAT:
	ff = (float *) &miss;
	if (!check_miss || missing == *ff) {
	    dd = (void *) datac;
	    break;
	}
	f=(float *) emalloc(prod*sizeof(float));
	for (i=0; i < prod; ++i) {
	    if (datac[i] == missing) f[i] = *ff;
	    else f[i] = datac[i];
	}
	dd = (void *) f;
	break;
    case NC_DOUBLE:
	d=(double *) emalloc(prod*sizeof(double));
	for (i=0; i < prod; ++i) {
	    if (check_miss && datac[i] == missing) d[i] = miss;
	    else d[i] = datac[i];
	}
	dd = (void *) d;
	break;

    default:
      break;

    }
    if (data!=datac && (void *)datac!=dd) free(datac);
    if (rangerr)
	logFile->write_time("Error: Range error - out of bounds data set to missing\n");
    i = ncvarputg(ncid, varid, corn, edge, stride, imap, dd);
    if (dd != (void *) data) free(dd);
    if (i == -1) {
	logFile->write_time("Error: ncvarputg failed\n");
	return -1;
    }
    return 0;
}



static
int nint(
    double d)
{
    if (d >= 0) return (int)(d + .5);
    return (int)(d - .5);
}



#endif
