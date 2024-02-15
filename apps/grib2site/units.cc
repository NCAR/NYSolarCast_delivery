/*
 *	Copyright 1992 University Corporation for Atmospheric Research
 */
/* $Id: units.cc,v 1.11 2015/10/08 16:00:44 dicast Exp $ */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "log/log.hh"
#include "units.h"
#include "timeunits.h"
#include "product_data.h"
#include "nc.h"
#include "params.h"
#include "emalloc.h"

using namespace std;

extern Log *logFile;

/*
 * Initialize udunits library.  Reads in units table.  Returns a non-zero value
 * on failure, 0 on success.  Only tries to read in units table once; if that
 * fails, it will continue to return failure code on subsequent calls.
 *
 * If the environment variable UDUNITS_PATH exists, that is used as the path
 * of the units table; otherwise whatever was compiled into the installed
 * udunits library is used.
 */
int
init_udunits() {
    static int first = 1;
    static int stat = 0;

    if (first) {
	first = 0;

	// Quiet error logging
	ut_set_error_message_handler (ut_ignore);

	stat = utInit((char *) 0);
	if (stat != 0)
	  logFile->write_time("Error: Initializing udunits: %d\n", stat);
	else
	  logFile->write_time(1,"Info: Succussfully initialized udunits\n");
    }

    return stat;
}


/*
 * The NUWG conventions require time as a double in units such as "hours
 * since 1992-1-1".  This function converts the GRIB product reference time
 * and forecast time to the reftime and valtime required in the NUWG
 * conventions, using whatever units are associated with the reftime and
 * valtime variables.  It also fills in a structure of human-readable times
 * corresponding to the reftime and (valtime-reftime) offset.  Returns 0 on
 * success, nonzero on failure.
 */
int
rvhours(
    product_data *gp,		/* input GRIB product structure */
    ncfile *ncp,		/* input netCDF data structure */
    double *reftimep,		/* output, reference time */
    double *valtimep,		/* output, valid time */
    humtime *htp		/* human-comprehensible reftime/valtime */
	)
{
    utUnit *refunitsp = ncp->vars[ncp->reftimeid]->bunitp;
    utUnit *valunitsp = ncp->vars[ncp->valtimeid]->bunitp;
    utUnit hourunit;	/* unit for GRIB valid time offset */
    double tdiff;
    utUnit tdiffunits;
    double slope, intercept;
    //int ret;

    htp->year = gp->year+(gp->century - (gp->year==0 ? 0 : 1))*100;
    htp->month = gp->month;
    htp->day = gp->day;
    htp->hour = gp->hour;
    htp->minute = gp->minute;
    htp->second = 0;

    /* compute reftime in target units */
    utInvCalendar(htp->year, htp->month, htp->day,
			 htp->hour, htp->minute, htp->second,
			 refunitsp, reftimep);

    /* tdiff value is function of tr[0] and tr[1] depending on tr_flg */
    tdiff = frcst_time(gp);

    /* Get tdiff in hours */
    char *hour = (char *)"hour";
    memset(&hourunit, 0, sizeof(utUnit));
    if (utScan(hour, &hourunit) != 0) {
      logFile->write_time("Error: utScan() error, hour\n");
      return -3;
    }
    if (gp->tunit != TUNITS_HOUR) {
	if (utScan((char *) tunits(gp->tunit), &tdiffunits) != 0) {
	    logFile->write_time("Error: GRIB %s: bad forecast time unit %s\n",
				gp->header, tunits(gp->tunit) );
	    return -4;
	}
	if (utConvert(&tdiffunits, &hourunit, &slope, &intercept) != 0) {
	    logFile->write_time("Error: GRIB %s: bad forecast time conversion\n", gp->header);
	    return -5;
	}
	htp->valoffset = slope * tdiff + intercept;
    } else {
        tdiffunits = hourunit;
        htp->valoffset = tdiff;
    }
    utFree(&hourunit);

    /* compute valtime = reftime + tdiff */

    // udunits2 changes 'tdiffunits' from hours to seconds and that
    // screws up the slope. So use the valoffset (in hours) to add to
    // reftime to get valtime. JRC

    //if (utConvert(&tdiffunits, refunitsp, &slope, &intercept) != 0) {
    //logFile->write_time("Error: GRIB %s: bad forecast time conversion\n", gp->header);
    //return -5;
    //}
    /* ignore intercept when adding interval */
    //*valtimep = *reftimep + slope * tdiff;

    *valtimep = *reftimep + htp->valoffset;

    /* convert result to valtime units */
    if (utConvert(refunitsp, valunitsp, &slope, &intercept) != 0) {
      logFile->write_time("Error: GRIB %s: bad reftime to valtime units conversion\n", gp->header);
      return -5;
    }
    *valtimep = slope * *valtimep + intercept;

    return 0;
}


/*
 * Get value of units attribute associated with a netCDF variable, if any.
 * Returns 0 if OK, or if no units attribute.
 * Returns -1 if error accessing units attribute.
 */
int
get_units(
    int ncid,			/* netCDF ID */
    int varid,			/* variable ID */
    utUnit **bunitp		/* returned pointer to malloced binary unit */
	)
{
    nc_type atttype;
    size_t attlen;
#define MAX_UNIT_LEN 100
    char units[MAX_UNIT_LEN];

    *bunitp = 0;
    
    if (nc_inq_att(ncid, varid, UNITS_NAME, &atttype, &attlen) != NC_NOERR) {
	return 0;		/* no units attribute */
    }
    /* else */
    if (atttype == NC_CHAR) {

      if (attlen+1 > MAX_UNIT_LEN) {
	logFile->write_time("Error: units attribute too long: %d\n", attlen);
	return -1;
      }
      if(nc_get_att_text(ncid, varid, UNITS_NAME, units) != NC_NOERR) {
	logFile->write_time("Error: getting units attribute (char)\n");
	return -1;
      }
      units[attlen] = '\0'; /* ncattget doesn't null-terminate */
    }
#ifdef NC_STRING // NetCDF-4 type
    else if (atttype == NC_STRING) {
      char *units_arr[1];
      if(nc_get_att_string(ncid, varid, UNITS_NAME, units_arr) != NC_NOERR) {
	logFile->write_time("Error: getting units attribute (string)\n");
	return -1;
      }
      strcpy(units, units_arr[0]);
      attlen = strlen(units);
    }
#endif // NC_STRING
    else {
      logFile->write_time("Error: Attribute units not of type char or string\n");
      return -1;
    }

    *bunitp = (utUnit *)emalloc(sizeof(utUnit));
    memset(*bunitp, 0, sizeof(utUnit));
    /* Don't bother to parse psuedo-units, such as "(allocated by center)" */
    if(units[0] != '(' && utScan(units, *bunitp) != 0) {
	logFile->write_time("Error: parsing units: %s\n", units);
	return -1;
    }
    return 0;
}



/*
 * Return units conversion structure (with slope and intercept) for
 * converting from standard GRIB units to specified units for specified
 * netCDF variable corresponding to a GRIB parameter.
 */
unitconv *
uconv(
    char *varname,		/* netCDF variable name */
    utUnit *btunitp		/* netCDF variable units to which GRIB data
				   should be converted */
	)
{
    int param;			/* GRIB parameter code */
    char *funits;		/* units to convert from, if GRIB 1 product */
    double slope=1., intercept=0.;
    utUnit bfunit;
    unitconv *out;

    memset(&bfunit, 0, sizeof(utUnit));

    param = grib_pcode(varname);
    if (param == -1)		/* not a GRIB parameter */
	return 0;
    if (!btunitp)		/* no units to convert to */
	return 0;
    funits = grib_units(param);
    if(!funits)			/* GRIB1 units not available for this parameter */
	return 0;

    if(funits != 0) { 
	if(utScan(funits, &bfunit) != 0) { /* "from" unit */
	    logFile->write_time("Error: parsing GRIB units '%s' for variable\n", funits, varname);
	    return 0;
	}
	if(utConvert(&bfunit, btunitp, &slope, &intercept) == UT_ECONVERT) {
	    logFile->write_time("Error: GRIB units `%s' not conformable with variable %s:units\n",
		   funits, varname);
	    return 0;
	}
    } else {
	slope = 1.0;
	intercept = 0.0;
    }
    utFree(&bfunit);
    if (slope == 1.0 && intercept == 0.0) /* trivial conversion */
	return 0;
    out = (unitconv *)emalloc(sizeof(unitconv));
    out->slope = slope;
    out->intercept = intercept;
    return out;
}

