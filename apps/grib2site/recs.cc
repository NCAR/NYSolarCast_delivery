/*
 *   Copyright 1996, University Corporation for Atmospheric Research.
 *   See ../COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: recs.cc,v 1.5 2015/10/12 18:42:59 dicast Exp $ */

#include <netcdf.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "log/log.hh"
#include "timeunits.h"
#include "nc.h"
#include "recs.h"
#include "emalloc.h"

extern Log *logFile;
extern int match_filetime;


/*
 * Initializes (reftime,valtime) pair table from open netcdf file.
 * Returns -1 on failure.
 */
int
new_recs(
    ncfile *nc)
{
    int ncid = nc->ncid;
    int recid = nc->recid;
    size_t nrecs;
    size_t size;
    char *ncname = nc->ncname;

    /* get number of records */
    if (nc_inq_dim(ncid, recid, (char *)0, &nrecs) == -1) {
	logFile->write_time("Error: %s: can't get number of records\n", ncname);
	return -1;
    }
    nc->rt = (rectimes *) emalloc(sizeof(rectimes));
    nc->rt->nrecs = nrecs;

    if (nrecs == 0) {
#define RECS_INIT_SIZE 32	/* initial size of growable record table */
	size = RECS_INIT_SIZE;
    } else {
	size = 2*nrecs;
    }
    nc->rt->size = size;
    nc->rt->reftimes = (double *) emalloc(size * sizeof(double));
    nc->rt->valtimes = (double *) emalloc(size * sizeof(double));
    
    if (nrecs > 0) {
	int nerrs = 0;
	//int irec;
	size_t start[1];
	size_t count[1];
	start[0] = 0;
	count[0] = nrecs;
	if (nc_get_vara_double(ncid, nc->reftimeid, start, count,
			       nc->rt->reftimes) == -1) {
	    logFile->write_time("Error: %s: can't get reftimes\n", nc->ncname);
	    nerrs++;
	}
	if (nc_get_vara_double(ncid, nc->valtimeid, start, count,
			       nc->rt->valtimes ) == -1) {
	    logFile->write_time("Error: %s: can't get valtimes\n", nc->ncname);
	    nerrs++;
	}
	if(nerrs)
	    return -1;
    }
    return 0;
}


/*
 * Frees memory used by (reftime,valtime) pair table from open netcdf file.
 */
int
free_recs(
    rectimes *rp)
{
    if(rp) {
	if(rp->reftimes)
	    free(rp->reftimes);
	if(rp->valtimes)
	    free(rp->valtimes);
	free(rp);
    }
    return 0;
}


/*
 * Returns record number determined by (reftime,valtime) pair.  If
 * (reftime,valtime) is in table, returns corresponding record.  If
 * (reftime,valtime) is not in table, increments record count, enters
 * (reftime,valtime) for new record in table, updates reftime, valtime in
 * netCDF file, updates human-comprehensible time variables in netCDF file
 * (if any), and returns record.  Returns -1 on failure.
 */
long
getrec(
    ncfile *nc,
    double reftime,
    double valtime,
    humtime *htp
	)
{
  //double *dp;
    double *reftimes = nc->rt->reftimes;
    double *valtimes = nc->rt->valtimes;
    int ncid = nc->ncid;
    int i;

    /* First look in table of existing records */
    for (i=0; i < nc->rt->nrecs; i++) {
	if (reftime == reftimes[i] && valtime == valtimes[i])
	    return i;
    }

    // Verify the model reftime matches the filename date/time
    if (match_filetime)  // this is turned off using -m option
      {
	char match_datetime[15];
	sprintf(match_datetime, "%.4d%.2d%.2d.i%02d%02d",
		htp->year, htp->month, htp->day,
		htp->hour, htp->minute);

	//printf("reftime %s, filename %s\n", match_datetime, nc->ncname);
	if (strstr(nc->ncname, match_datetime) == NULL)
	  {
	    logFile->write_time("Error: The model reftime (%s) does not match the date/time pattern in the output filename (%s).\n", match_datetime, nc->ncname);
	    logFile->write_time("Info: Use the -m option to bypass this date/time matching enforcement.\n");
	    return -1;
	  }
      }

    /* Didn't find it, so create a new record */
    if (nc->rt->nrecs+1 == nc->rt->size) {/* no room for another record,
					     double size of table before
					     adding */
	nc->rt->size *= 2;
	nc->rt->reftimes = (double *) erealloc(nc->rt->reftimes,
		     nc->rt->size * sizeof(double));
	nc->rt->valtimes = (double *) erealloc(nc->rt->valtimes,
		     nc->rt->size * sizeof(double));
	reftimes = nc->rt->reftimes;
	valtimes = nc->rt->valtimes;
    }
    reftimes[nc->rt->nrecs] = reftime;
    valtimes[nc->rt->nrecs] = valtime;

    {				/* Update reftimes and valtimes in ncfile */
	size_t ix[2];
	size_t count[2];

	ix[0] = nc->rt->nrecs;
	count[0] = 1;

	if (nc_put_var1_double(ncid, nc->reftimeid, ix, &reftime) == -1 ||
	    nc_put_var1_double(ncid, nc->valtimeid, ix, &valtime) == -1) {
	    logFile->write_time("Error: %s: failed to add new reftime, valtime\n", nc->ncname);
	    return -1;
	}
	/* update humtime record variables, if any, from htp */
	ix[1] = 0;
	if (nc->datetimeid > 0) {
	    char datetime[100];
	    sprintf(datetime, "%.4d-%.2d-%.2d %02d:%02d:%02.0fZ",
		    htp->year, htp->month, htp->day,
		    htp->hour, htp->minute, htp->second);
	    count[1] = strlen(datetime)+1; /* include terminating null */
	    nc_put_vara_text(ncid, nc->datetimeid, ix, count, datetime);
	}
	if (nc->valoffsetid > 0) {
	    nc_put_var1_float(ncid, nc->valoffsetid, ix, &htp->valoffset);
	}
    }
    nc->rt->nrecs++;
    return nc->rt->nrecs-1;
}
