/*
 *   Copyright 1996 University Corporation for Atmospheric Research
 *   See ../COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: nc.cc,v 1.33 2015/10/12 18:42:59 dicast Exp $ */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <netcdf.h>

#include "mkdirs_open.h"
#include "nc.h"
#include "nuwg.h"
#include "emalloc.h"
#include "params.h"
#include "units.h"
#include "levels.h"
#include "centers.h"
#include "timeunits.h"
#include "gbds.h"		/* only for FILL_VAL */
#include "gdes.h"
#include "recs.h"
#include "site_list.h"
#include "ncfloat.h"
#include "log/log.hh"

#ifndef FILL_NAME
#define FILL_NAME	"_FillValue"
#endif

#ifndef INTERP_METHOD_NAME
#define INTERP_METHOD_NAME	"interpolation_method"
#endif

extern Log *logFile;


typedef struct levels_table {
    int id;			/* dimension id of netCDF dimension */
    float *vals;		/* levels */
    size_t num;			/* number of levels */
    utUnit *bunitp;		/* level units */
} levels_table;


typedef struct layers_table {
    int id;			/* dimension id of netCDF dimension */
    float *tops;		/* top levels of layers  */
    float *bots;		/* bottom levels of layers  */
    size_t num;			/* number of layers */
    utUnit *bunitp;		/* top (and bottom) level units */
} layers_table;


typedef struct tris_table {
    int id;			/* dimension id of netCDF dimension */
    float *starts;		/* starts of time ranges */
    float *ends;		/* ends of time ranges */
    size_t num;			/* number of ranges */
} tris_table;


#ifdef __STDC__
static ncdim* new_dim(int dimid);
static ncvar* new_var(char* ncname, int varid);
static void free_var(ncvar* var);
static void free_dim(ncdim* dim);
static char* parmname(ncfile* nc, product_data* pp);
static int make_ncfile(char* ncname, ncfile* out);
static levels_table* getlevtab(ncfile* nc, ncvar* var);
static layers_table* getlaytab(ncfile* nc, ncvar* var);
static long getlev(product_data* pp, ncfile* nc,
		   ncvar* var);
static int make_var(char* ncname, int varid, ncvar* out);
#ifdef DONT_NEED_FOR_SITE_DATA
static int var_as_int(ncfile* nc, enum ncpart comp, int* val);
static int var_as_float(ncfile* nc, enum ncpart comp, float* val);
static int var_as_lset(ncfile* nc, enum ncpart comp, lset* list);
static void varerr(ncfile* nc, enum ncpart comp);
static int make_navgrid(ncfile* nc, navinfo* nav);
static int make_navinfo(ncfile* nc, navinfo* nav);
static void free_navinfo(navinfo* np);
static navinfo* new_navinfo(ncfile* nc);
static int gd_fne_err(product_data* pp, ncfile* nc, enum ncpart comp, float pval, float nval);
static int gd_ine_err(product_data* pp, ncfile* nc, enum ncpart comp, int pval, int nval);
static int gd_igt_err(product_data* pp, ncfile* nc, enum ncpart comp, int pval, int nval);
static int subgrid(ncfile* nc, product_data* pp, long* ix0, long* ix1);
#endif
int float_nc(int ncid, int varid, size_t *corn, size_t *edge, float *data,
	     double slope, double intercept, float missing);
int nc_float(int ncid, int varid, size_t *corn, size_t *edge, float *data,
	     float missing, double slope, double intercept);
#endif


int ncid;
			/* netCDF id of open netCDF output file.
			 * This is at file scope so routine registered
			 * with atexit() can get it to close file. */

static ncdim *
new_dim(int dimid)
{
    char dimname[NC_MAX_NAME];
    size_t size;
    ncdim *out = (ncdim *)emalloc(sizeof(ncdim));
    
    if (nc_inq_dim(ncid, dimid, dimname, &size) != NC_NOERR) {
	return 0;
    }
    out->name = estrdup(dimname);
    out->id = dimid;
				/* We don't cache size because it may change,
				   e.g. adding records, and we don't want to
				   have to keep cached value up to date.  To
				   get size, use ncdiminq(). */

    return out;
}


static int
make_var(
    char *ncname,		/* netCDF pathanme, only used in error msg */
    int varid,			/* variable ID */
    ncvar *out			/* place to put constructed ncvar */
    )
{
    char varname[NC_MAX_NAME];
    nc_type type;
    int ndims;
    int dims[NC_MAX_VAR_DIMS];
    int id;
    
    if(nc_inq_var(ncid, varid, varname, &type, &ndims, dims, (int *)0) != NC_NOERR) {
	return -1;
    }

    out->id = varid;
    out->name = estrdup(varname);
    out->type = type;
    out->ndims = ndims;
    out->dims = (int *)emalloc(ndims * sizeof(int));
    for (id = 0; id < ndims; id++)
	out->dims[id] = dims[id];

    /* get value of _FillValue attribute, if any, as a float */

    nc_type atttype;
    size_t attlen;

    if (nc_inq_att(ncid, varid, FILL_NAME, &atttype, &attlen) != NC_NOERR) {
      out->fillval = 0;           /* no fill-value attribute */
    }
    else {

      out->fillval = (float *) emalloc(sizeof(float));
      nc_get_att_float(ncid, varid, FILL_NAME, out->fillval);

    }

    if (get_units(ncid, varid, &out->bunitp) == -1) {
      logFile->write_time("Error: can't get units attribute for variable %s\n",
			  varname);
      return -1;
    }

    if (out->bunitp && grib_pcode(varname) != -1) {
	out->uc = uconv(varname, out->bunitp);
    } else {
	out->uc = 0;
    }
    return 0;
}


static void
free_var(ncvar *var)
{
    if (var) {
	if(var->name)
	    free(var->name);
	if(var->dims)
	    free(var->dims);
	if(var->bunitp) {
	  utFree(var->bunitp);
	  free(var->bunitp);
	}
	if(var->fillval)
	    free(var->fillval);
	free(var);
    }
}


/*
 * Creates a new ncvar structure and fills it in with the information from
 * the open netCDF file whose handle is ncid.  A pointer to the structure is
 * returned, or 0 on failure.
 */
static ncvar *
new_var(char *ncname, int varid)
{
    ncvar *out;

    if (varid == -1)		/* handle common failure case with message
				   at higher level */
	return 0;

    out = (ncvar *)emalloc(sizeof(ncvar));
    if (make_var(ncname, varid, out) != 0) {
	free_var(out);
	return 0;
    }

    return out;
}


static void
free_dim(ncdim *dim)
{
    if (dim) {
	if(dim->name)
	    free(dim->name);
	free(dim);
    }
}


/*
 * Name of environment variable containing directory in which to search for
 * CDL files if not found relative to directory from which this process is
 * invoked.
 */
#define LDM_ETCDIR	"LDM_ETCDIR"

/*
 * Checks to see if netCDF file with specified name exists.  If not,
 * makes netCDF file from CDL template file.
 * Returns netCDF file ID on success, or -1 on error.
 */
int
cdl_netcdf (
     char *cdlname,	/* CDL file specifying netCDF structure */
     char *ncname	/* filename of netcdf file to be created */
     )
{
    char cmnd[2*_POSIX_PATH_MAX+20];
    char *cdlfile = cdlname;
    static char *cdldir = 0;
    char envcdl[_POSIX_PATH_MAX];


    /* turn off netCDF error messages from
       library, means we will have to check all
       netCDF status returns and interpret */

    //ncopts = 0;


    if (access(ncname, (R_OK|W_OK)) != 0) { /* no writable ncname exists */
      if (cdlfile == 0) {
	logFile->write_time("Error: %s doesn't exist, and didn't specify a CDL filename\n",
			    ncname);
	return -1;
      }

	/* Try to create any directories in output path that don't exist */
	if (diraccess(ncname,  (R_OK | W_OK), !0) == -1) {
	  logFile->write_time("Error: can't access directories leading to %s\n", ncname);
	  return -1;
	}

	/* If CDL file not found, look in environment variable LDM_ETCDIR */
	if (access(cdlname, R_OK) == -1) { /* no CDL file or unreadable */
	    if (cdldir == 0) {	/* executed once, when cdldir first needed */
		char *ldm_etcdir = getenv(LDM_ETCDIR);
		int slen;
		
		if (ldm_etcdir == 0) {
		    logFile->write_time("Error: CDL file %s not found & LDM_ETCDIR not in environment\n", cdlname);
		    return -1;
		}
		slen = strlen(ldm_etcdir);
		cdldir = (char *)emalloc((slen+2) * sizeof(char));
		strcpy(cdldir, ldm_etcdir);
		if (cdldir[slen-1] != '/') { /* append "/" to dir name */
		    strcat(cdldir, "/");
		}
	    }
	    strcat(envcdl,cdldir);
	    strcat(envcdl,cdlname);
	    if (access(envcdl, R_OK) == -1) {
	      logFile->write_time("Error: can't find CDL file %s, or unreadable\n", envcdl);
      return -1;
	    }
	    cdlfile = envcdl;
	}
	
	(void) strcpy(cmnd, "ncgen");
	(void) strcat(cmnd, " -b ");
	(void) strcat(cmnd, " -o ");
	(void) strcat(cmnd, ncname);
	(void) strcat(cmnd , " ");
	(void) strcat(cmnd, cdlfile);
	
	logFile->write_time("Info: Executing: %s\n", cmnd);

	if (system(cmnd) != 0) {
	    logFile->write_time("Error: can't run \"%s\"\n", cmnd);
	    return -1;
	}
    }

    int id;
    if (nc_open(ncname, NC_WRITE, &id) == NC_NOERR) {
      return (id);
    }
    else {
      return -1;
    }
}


void
setncid(int id)
{
    ncid = id;
}


int
getncid()
{
    return ncid;
}


/* Close open netCDF file, if any */
void
nccleanup()
{
  nc_close(ncid);
}


/*
 * Creates and returns a pointer to a one-dimensional table of levels
 * from the currently-open netCDF file (handle ncid).  Returns 0 on
 * failure.
 */
static levels_table*
getlevtab(
    ncfile *nc,			/* currently open netCDF file */
    ncvar *var			/* level variable */
    )
{
    int did;
    levdim *lp = nc->levdims;
    
    /* See if appropriate levels table already exists for this file */
    //if (var->ndims < 3) {
    if (var->ndims < 2) {
	logFile->write_time("Error: variable %s has too few dimensions for a level\n", var->name);
	return 0;
    }
    if (var->dims[0] != nc->recid)  /* no record dimension */
      did = var->dims[0];
    else
      did = var->dims[1];	/* level dimension */

    while (lp) {
	if(did == lp->levtab->id) {	/* found it */
	    return lp->levtab;
	}
	lp = lp->next;
    }
    /* Not there, so we must create it */
    lp = (levdim *)emalloc(sizeof(levdim));
    lp->levtab = (levels_table *)emalloc(sizeof(levels_table));
    lp->levtab->id = did;
    lp->next = nc->levdims;

    /* Initialize array of levels */
    {
	levels_table *out = lp->levtab;
	char levname[NC_MAX_NAME];
	int levvarid;		/* variable id of level variable */
	ncvar *lev;		/* level variable */
	size_t start = 0;

        /* get number of levels */
	if (nc_inq_dim(ncid, out->id, levname, &out->num) != NC_NOERR) {
	    logFile->write_time("Error: can't get number of %s levels\n", var->name);
	    return 0;
	}
	out->vals = (float *) emalloc(out->num * sizeof(float));
	int ret;
	ret = nc_inq_varid(ncid, levname,&levvarid);
	if(ret != NC_NOERR) {
	    logFile->write_time("Error: No %s coordinate variable for %s level\n",
		   levname, var->name);
	    return 0;
	}
	lev = nc->vars[levvarid];
				/* Check consistency of lev variable */
	if (strcmp(lev->name, levname) != 0 ||
	    lev->type != NC_FLOAT ||
	    lev->ndims != 1 ||
	    lev->dims[0] != out->id) {
	    logFile->write_time("Error: variable %s must be float %s(%s)\n",
		   levname, lev->name, lev->name);
	    return 0;
	}

	if(get_units(ncid, levvarid, &out->bunitp) == -1) {
	  logFile->write_time("Error: error getting units attribute for %s\n",
			      levname);
	  return 0;
	}

	if(nc_get_vara_float(ncid, levvarid, &start, &out->num,
			     out->vals) != NC_NOERR) {
	  logFile->write_time("Error: no %s variable for level\n", levname);
	  return 0;
	}
    }

    nc->levdims = lp;		/* if all goes well */
    return lp->levtab;
}


/*
 * Creates and returns a pointer to a one-dimensional table of layers
 * from the currently-open netCDF file (handle ncid).  Returns 0 on
 * failure.
 */
static layers_table*
getlaytab(
    ncfile *nc,			/* currently open netCDF file */
    ncvar *var			/* layer variable */
	)
{
    int did;
    laydim *lp = nc->laydims;
    
    /* See if appropriate layers table already exists for this file */
    if (var->ndims < 3) {
	logFile->write_time("Error: variable %s has too few dimensions for a layer\n", var->name);
	return 0;
    }
    did = var->dims[1];	/* layer dimension */

    while (lp) {
	if(did == lp->laytab->id) {	/* found it */
	    return lp->laytab;
	}
	lp = lp->next;
    }
    /* Not, there, so we must create it */
    lp = (laydim *)emalloc(sizeof(laydim));
    lp->laytab = (layers_table *)emalloc(sizeof(layers_table));
    lp->laytab->id = did;
    lp->next = nc->laydims;

    /* Initialize array of layers */
    {
	layers_table *out = lp->laytab;
	char layname[NC_MAX_NAME];
	char topname[NC_MAX_NAME];
	char botname[NC_MAX_NAME];
	int topvarid;		/* variable id of layer top variable */
	int botvarid;		/* variable id of layer bottom variable */
	ncvar *top;		/* layer top variable */
	ncvar *bot;		/* layer bottom variable */
	size_t start = 0;

        /* get number of layers */
	if (nc_inq_dim(ncid, out->id, layname, &out->num) != NC_NOERR) {
	    logFile->write_time("Error: can't get number of %s layers\n", var->name);
	    return 0;
	}
	if (strlen(layname)  +  strlen("_top") > (size_t) NC_MAX_NAME) {
	    logFile->write_time("Error: name of layer dimension too long (%s)\n", layname);
	    return 0;
	}
	out->tops = (float *) emalloc(out->num * sizeof(float));
	strcpy(topname, layname);
	strcat(topname, "_top");
	int ret;
	ret = nc_inq_varid(ncid, topname,&topvarid);
	if(ret != NC_NOERR) {
	    logFile->write_time("Error: no %s coordinate variable for %s layer top\n", layname, var->name);
	    return 0;
	}
	top = nc->vars[topvarid];
				/* Check consistency of top variable */
	if (strcmp(top->name, topname) != 0 ||
	    top->type != NC_FLOAT ||
	    top->ndims != 1 ||
	    top->dims[0] != out->id) {
	    logFile->write_time("Error: variable %s must be float %s(%s)\n", layname, top->name, top->name);
	    return 0;
	}

	if(get_units(ncid, topvarid, &out->bunitp) == -1) {
	    logFile->write_time("Error: getting units attribute for %s\n", topname);
	    return 0;
	}

	if(nc_get_vara_float(ncid, topvarid, &start, &out->num,
			     out->tops) != NC_NOERR) {
	    logFile->write_time("Error: no %s variable for top of layer\n", topname);
	    return 0;
	}
	out->bots = (float *) emalloc(out->num * sizeof(float));
	strcpy(botname, layname);
	strcat(botname, "_bot");
	ret = nc_inq_varid(ncid, botname,&botvarid);
	if(botvarid == -1) {
	    logFile->write_time("Error: no %s coordinate variable for %s layer bot\n", layname, var->name);
	    return 0;
	}
	bot = nc->vars[botvarid];
				/* Check consistency of bot variable */
	if (strcmp(bot->name, botname) != 0 ||
	    bot->type != NC_FLOAT ||
	    bot->ndims != 1 ||
	    bot->dims[0] != out->id) {
	    logFile->write_time("Error: variable %s must be float %s(%s)\n", layname, bot->name, bot->name);
	    return 0;
	}

	if(nc_get_vara_float(ncid, botvarid, &start, &out->num,
			     out->bots) != NC_NOERR) {
	    logFile->write_time("Error: no %s variable for bottom of layer\n", botname);
	    return 0;
	}
    }

    nc->laydims = lp;		/* if all goes well */
    return lp->laytab;
}


/*
 * Handle levels
 */
static long
levaux(
    product_data *pp,	/* decoded GRIB data to be written */
    ncfile *nc,		/* netCDF file to be written */
    ncvar *var		/* netCDF variable to be written */
	)
{
    int level_flg = pp->level_flg;
    char *varname = var->name;

    /*
      Assumes level dimension is second dimension of a variable, and that
      there is a coordinate variable associated with a level, values of
      which get stored in the level table.
    */

    double lev;
    long levix;

    levels_table *levtab = getlevtab(nc, var);

    if (levtab == 0) {	/* initialize table of levels */
	return -1;
    }

    lev = level1(pp->level_flg, pp->level);

    /* Must convert to units of level table */
    {
	utUnit bfunit;
	memset (&bfunit, 0, sizeof(utUnit));
	char *funits = levelunits(level_flg);
	double slope=1.;
	double intercept=0.;
	if(utScan(funits, &bfunit) != 0) { /* "from" unit */
	    logFile->write_time("Error: parsing unit `%s' for level %s\n", funits, varname);
	    return -1;
	}
	if (levtab->bunitp) {
	  int utStat = utConvert(&bfunit, levtab->bunitp, &slope, &intercept);
	  if(utStat == UT_ECONVERT) {
	    logFile->write_time("Error: units `%s' not conformable with variable %s:units\n",
				funits, varname);
	    return -1;
	  }
	}
	lev = slope * lev + intercept;
	utFree(&bfunit);
    }
    levix = level_index(lev, levtab->vals, levtab->num);
    if (levix == -1) {
	logFile->write_time(1, "Warning: GRIB %s: In %s, no %f level for %s\n",
	       pp->header, nc->ncname, lev, varname);
	return -1;
    }
    return levix;
}


/*
 * Handle layers
 */
static long
layaux(
    product_data *pp,	/* decoded GRIB data to be written */
    ncfile *nc,		/* netCDF file to be written */
    ncvar *var		/* netCDF variable to be written */
	)
{
    int layer_flg = pp->level_flg;
    char *varname = var->name;

    /*
	Assumes layer dimension is second dimension of a variable, and that
	there are _top and _bot levels associated with each layer, values of
	which which get stored in the layer table.
      */

    double top;
    double bot;
    long layix;

    layers_table *laytab = getlaytab(nc, var);

    if (laytab == 0) {
	return -1;
    }

    top = pp->level[0];
    bot = pp->level[1];
    /* Must convert top,bot to units of layer tables */
    {
	utUnit bfunit;
	memset (&bfunit, 0, sizeof(utUnit));
	char *funits = levelunits(layer_flg);
	double slope=1.;
	double intercept=0.;
	if(utScan(funits, &bfunit) != 0) { /* "from" unit */
	    logFile->write_time("Error: parsing unit `%s' for level %s\n", funits, varname);
	    return -1;
	}
	if(laytab->bunitp) {
	  //printf("var: %s, layer units: %s, top %f, bot %f\n", varname, funits, top, bot);
	    if(utConvert(&bfunit, laytab->bunitp, &slope, &intercept) ==
	       UT_ECONVERT) {
		logFile->write_time("Error: units `%s' not conformable with variable %s:units\n",
		       funits, varname);
		return -1;
	    }
	}
	top = slope * top + intercept;
	bot = slope * bot + intercept;
	utFree(&bfunit);
    }
    layix = layer_index(top, bot, laytab->tops, laytab->bots, laytab->num);
    if (layix == -1) {
	logFile->write_time(1, "Warning: GRIB %s: In %s, no (%g,%g) level for %s\n",
	       pp->header, nc->ncname, top, bot, varname);
	return -1;
    }
    return layix;
}


/*
 * Return netCDF level dimension index appropriate for decoded GRIB
 * product.  Returns -2 if no level dimension appropriate (e.g. for surface
 * variables) or -1 in case of failure.
 */
static long
getlev(
    product_data *pp,	/* decoded GRIB data to be written */
    ncfile *nc,		/* netCDF file to be written */
    ncvar *var		/* netCDF variable to be written */
	)
{
    switch (pp->level_flg) {
    /* Levels */
    case LEVEL_ISOBARIC:
    case LEVEL_FHG:
    case LEVEL_SIGMA:
    case LEVEL_HY:
    case LEVEL_FH:
    case LEVEL_Bls:
    case LEVEL_ISEN:
    case LEVEL_PDG:
    case LEVEL_FHGH:
    case LEVEL_DBS:
    case LEVEL_FL:
    case LEVEL_ETAL:
	return levaux(pp, nc, var);

    /* Layers */
    case LEVEL_LBls:
    case LEVEL_LFHG:
    case LEVEL_LFHM:
    case LEVEL_LHY:
    case LEVEL_LISEN:
    case LEVEL_LISH:
    case LEVEL_LISM:
    case LEVEL_LISO:
    case LEVEL_LPDG:
    case LEVEL_LS:
    case LEVEL_LSH:
	return layaux(pp, nc, var);

    /* Special levels, just one so no dimension needed */
    case LEVEL_SURFACE:
    case LEVEL_CLOUD_BASE:
    case LEVEL_CLOUD_TOP:
    case LEVEL_ISOTHERM:
    case LEVEL_ADIABAT:
    case LEVEL_MAX_WIND:
    case LEVEL_TROP:
    case LEVEL_TOP:
    case LEVEL_SEABOT:
    case LEVEL_MEAN_SEA:
    case LEVEL_ATM:
    case LEVEL_OCEAN:
    case LEVEL_CEILING:
    case LEVEL_LCY:
    case LEVEL_MCY:
    case LEVEL_HCY:
    case LEVEL_CCY:
    case LEVEL_BCY:
    case LEVEL_CCBL:
    case LEVEL_CCTL:
    case LEVEL_HCBL:
    case LEVEL_HCTL:
    case LEVEL_LCBL:
    case LEVEL_LCTL:
    case LEVEL_MCBL:
    case LEVEL_MCTL:
    case LEVEL_HTFL:
	return -2;
    }
    /* default: */
    return -1;
}


/*
 * Return netCDF ensemble member dimension index appropriate for decoded GRIB
 * product.  Returns -2 if no ensemble dimension needed or possible, -1 for
 * error.
 */
static long
getens(
       long lev,                 /* level dimension, for alignment */
       product_data *pp,	/* decoded GRIB data to be written */
       ncfile *nc,		/* netCDF file to be written */
       ncvar *var		/* netCDF variable to be written */
	)
{

  int ret;

  //
  // Check on the existence of the ensemble dimension
  //
  if ((var->ndims < 3) || (var->ndims == 3 && lev >= 0))
    return -2;

  //
  // If this is not an emsemble product at this piont, it is an error
  //
  if (pp->ensemble == 0) {
    logFile->write_time("Error: too many dimensions for %s, product is not an emsemble member\n", var->name);
    return -1;
  }

  //
  // Get size of ensemble dimension and the list of values
  //
  int ensdim = var->ndims - 2; // always the dimension to the left of site
  size_t size;
  char name[NC_MAX_NAME];
  if (nc_inq_dim(ncid, var->dims[ensdim], name, &size) != NC_NOERR) {
    logFile->write_time("Error: can't get number of %s ensemble\n", var->name);
    return -1;
  }
  float *values = (float *) emalloc(size * sizeof(float));
  size_t start = 0;
  int ensvarid;
  ret = nc_inq_varid(ncid, name,&ensvarid);
  if (ret != NC_NOERR) {
    logFile->write_time("Error: no %s variable for ensemble\n", name);
    return -1;
  }

  if (nc_get_vara_float(ncid, ensvarid, &start, &size, values) != NC_NOERR) { 
    logFile->write_time("Error: can't get ensemble member numbers\n");
    return -1;
  }
 
  //
  // Loop over the list and find the index for this member
  //
  int index = -1;
  for (int i=0; i<(int)size; i++) {
    if (pp->ensemble->member_num == (int)values[i]) {
      index = i;
      break;
    }
  }      
  
  if (index < 0)
    logFile->write_time(1, "Warning: GRIB %s: In %s, no (%d) ensemble member for %s\n", pp->header, nc->ncname, pp->ensemble->member_num, var->name);
  
  free(values);
  return index;
}

/*
 * Get conventional netcdf variable name with level indicator appended if
 * appropriate. The name is a pointer to a static string, so should be
 * copied if needed beyond the next call to parmname.
 */
static char*
parmname(
    ncfile *nc,			/* netCDF file */
    product_data *pp		/* product_data struct */
	)
{
    int parm = pp->param;	/* parameter code from GRIB product */
    int level = pp->level_flg;	/* level flag from GRIB product */
    int tr_flg = pp->tr_flg;	/* time range indicator flag */
    int der_flg = pp->der_flg;  /* derived variable indicator flag */
    int pctl_flg = pp->pctl_flg;  /* percentile variable value */
    
    char *varname = grib_pname(parm); /* netcdf variable base name */
    char *suffix;
    static char string[NC_MAX_NAME];
    char *name = string;

    if (!varname) {
      // This log message moved to product_data.cc
      //logFile->write_time("Error: unrecognized GRIB parameter code %d\n", parm);
      return 0;
    }

    strcpy(name, varname);

    /* add derived variable descriptor, if any */
    if (der_flg == 2) {
      strcat(name, "_stdev");   // standard deviation w/ respect to cluster mean
    }
    else if (der_flg == 3) {
      strcat(name, "_Nstdev");  // Normalized standard deviation
    }      
    else if (der_flg == 7) {
      strcat(name, "_iqr");     // IterQuartile Range (25%-75% quantile)
    }
    else if (der_flg == 8) {
      strcat(name, "_ens_min"); // minimum of all ensemble members
    }      
    else if (der_flg == 9) {
      strcat(name, "_ens_max"); // maximum of all ensemble members
    }      
    else if (der_flg > 0) {
      logFile->write_time(1, "Warning: un-handled derived forecast code: %d\n", der_flg);
    }

    /* add percentil value, if any */
    if (pctl_flg >= 0) {
      char pctl[10];
      sprintf(pctl, "_%dpctl", pctl_flg);
      strcat(name, pctl);
    }

    /* Get level modifier, if appropriate */
    suffix = (char *)"";
    suffix = levelsuffix(level);

    /* The "_sfc", "_msl", and "_liso" suffixes are redundant for some
       parameters, so we explicitly exclude those here */
    if((level == LEVEL_SURFACE && sfcparam(parm)) ||
       (level == LEVEL_MEAN_SEA && mslparam(parm)) ||
       (level == LEVEL_LISO && lisoparam(parm))) {
      suffix = (char *)"";
    }

    if (suffix[0] != '\0') {
      strcat(name, "_");
      strcat(name, suffix);
    }

    //
    // Special case for accumulated quantities: Add the accumulation interval to the name
    //
    if (tr_flg == TRI_Acc) {

        char time_range[10];
        int hrs = abs(pp->tr[1] - pp->tr[0]);

	switch (pp->tunit)
	  {
	  case TUNITS_MIN:
	    hrs /= 60;
	    break;
	  case TUNITS_HOUR:
	    break;
	  case TUNITS_3HR:
	    hrs *= 3;
	    break;
	  case TUNITS_6HR:
	    hrs *= 6;
	    break;
	  case TUNITS_12HR:
	    hrs *= 12;
	    break;
	  default:
	    logFile->write_time("Error: unusual time unit for accumulation: %d\n", pp->tunit);
	    return 0;
	  }
	
	sprintf(time_range, "%d", hrs);
	strcpy(name, varname);
	strcat(name, time_range);
	if (suffix[0] != '\0')
	  {
	    strcat(name, "_");
	    strcat(name, suffix);
	  }

	//
	// see if the variable exists, if not, use the base name and see if the first
	// time is 0 which would indicate an accumulation from time 0 to the tr[1].
	//
	int varid;
	int ret;
	ret = nc_inq_varid(ncid,name,&varid);

	if (ret != NC_NOERR && pp->tr[0] == 0)
	  {
	    strcpy(name, varname);
	    if (suffix[0] != '\0')
	      {
		strcat(name, "_");
		strcat(name, suffix);
	      }
	  }
    }

    //
    // If an average field, add av_ prefix.
    //
    else if (tr_flg == TRI_Ave) {
      strcpy(name, "av_");
      strcat(name, varname);
      if (suffix[0] != '\0') {
	strcat(name, "_");
	strcat(name, suffix);
      }
    }


    return name;
}


#ifdef DONT_NEED_FOR_SITE_DATA
/*
 * Stores value of a netCDF variable identified by a NUWG conventional id.
 * In case of failure, returns -1.  The value is converted from
 * whatever type is used for the netCDF variable.
 */
static int
var_as_int(
	   ncfile *nc,
	   enum ncpart comp,
	   int *val			/* where to store the resulting value */
	   )
{
    ncvar *var = (ncvar *)emalloc(sizeof(ncvar));
    long start[] = {0};
    long count[] = {1};
    double buf[1];		/* generic data buffer */
    
    if(make_var(nc->ncname, nuwg_getvar(ncid, comp), var) == -1) {
	free_var(var);
	return -1;
    }
    if (ncvarget(ncid, var->id, start, count, (void *)buf) == -1) {
	free_var(var);
	return -1;
    }
    switch (var->type) {	/* return the value as an int, no
				   matter how it is stored */
      case NC_BYTE:
	*val = *(unsigned char *) buf;
	break;
      case NC_CHAR:
	*val = *(char *) buf;
	break;
      case NC_SHORT:
	*val = *(short *) buf;
	break;
      case NC_LONG:
	*val = *(nclong *) buf;
	break;
      case NC_FLOAT:
	*val = (int)*(float *) buf;
	break;
      case NC_DOUBLE:
	*val = (int)*(double *) buf;
	break;
      default:
	break;
    }
    free_var(var);
    return 0;
}


/*
 * Stores value of a netCDF variable identified by a NUWG conventional id.
 * In case of failure, returns -1.  The value is converted from
 * whatever type is used for the netCDF variable.
 */
static int
var_as_float(
    ncfile *nc,
    enum ncpart comp,
    float *val
	)
{
    ncvar *var = (ncvar *)emalloc(sizeof(ncvar));
    long start[] = {0};
    long count[] = {1};
    double buf[1];		/* generic data buffer */
    
    if(make_var(nc->ncname, nuwg_getvar(ncid, comp), var) == -1) {
	return -1;
    }
    if (ncvarget(ncid, var->id, start, count, (void *)buf) == -1) {
	free_var(var);
	return -1;
    }
    switch (var->type) {	/* return the value as a float, no
				   matter how it is stored */
      case NC_BYTE:
	*val = *(unsigned char *) buf;
	break;
      case NC_CHAR:
	*val = *(char *) buf;
	break;
      case NC_SHORT:
	*val = *(short *) buf;
	break;
      case NC_LONG:
	*val = *(nclong *) buf;
	break;
      case NC_FLOAT:
	*val = *(float *) buf;
	break;
      case NC_DOUBLE:
	*val = *(double *) buf;
	break;
      default:
	break;
    }
    free_var(var);
    return 0;
}


/*
 * Stores values of a netCDF variable (of longs) identified by a NUWG
 * conventional id.  Values are just stored in a list of longs, which can be
 * used as a set in which values are looked up.  In case of failure, returns
 * -1.
 */
static int
var_as_lset(
    ncfile *nc,
    enum ncpart comp,
    lset *list			/* where to store the resulting list */
	)
{
    ncvar *var = (ncvar *)emalloc(sizeof(ncvar));
    static long start[NC_MAX_VAR_DIMS];
    static long count[NC_MAX_VAR_DIMS];
    long prod;
    int i;
    
    if(make_var(nc->ncname, nuwg_getvar(ncid, comp), var) == -1) {
	return -1;
    }
    if (var->type != NC_LONG) {
	logFile->write_time("Error: variable %s must be of type long\n", nuwg_name(comp));
	free_var(var);
	return -1;
    }
    prod=1;
    for (i=0; i<var->ndims; i++) {
	start[i] = 0;
	if (nc_inq_dim(ncid, var->dims[i], (char *)0, &count[i]) != NC_NOERR) {
	    logFile->write_time("Error: can't get size of dimension for %s\n", nuwg_name(comp));
	    free_var(var);
	    return -1;
	}
	prod *= count[i];
    }
    list->n = prod;
    list->vals = (nclong *)emalloc(sizeof(nclong) * prod);
    if (ncvarget(ncid, var->id, start, count, (void *)list->vals) == -1) {
	logFile->write_time("Error: can't get values for %s\n", nuwg_name(comp));
	free_var(var);
	free(list->vals);
	return -1;
    }
    free_var(var);
    return 0;
}


static void
varerr(ncfile *nc, enum ncpart comp)
{
    logFile->write_time("Error: no variable for %s\n", nuwg_name(comp));
}


/*
 *  Returns 0 on success, -1 on failure.
 */
static int
make_navgrid(
    ncfile *nc,			/* netCDF file */
    navinfo *nav		/* where to put the navinfo */
    )
{

    switch(nav->grid_type_code) {
    case GRID_LL:
    case GRID_RLL:
    case GRID_SLL:
    case GRID_SRLL:
    {
	gdes_ll *gg = &nav->grid.ll;
	
	if (var_as_int(nc, VAR_NI, &gg->ni) == -1) {
	    varerr(nc, VAR_NI);
	    return -1;
	}
	
	if (var_as_int(nc, VAR_NJ, &gg->nj) == -1) {
	    varerr(nc, VAR_NJ);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LA1, &gg->la1) == -1) {
	    varerr(nc, VAR_LA1);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LO1, &gg->lo1) == -1) {
	    varerr(nc, VAR_LO1);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LA2, &gg->la2) == -1) {
	    varerr(nc, VAR_LA2);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LO2, &gg->lo2) == -1) {
	    varerr(nc, VAR_LO2);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_DI, &gg->di) == -1) {
	    varerr(nc, VAR_DI);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_DJ, &gg->dj) == -1) {
	    varerr(nc, VAR_DJ);
	    return -1;
	}
	
	if (nav->grid_type_code == GRID_RLL || nav->grid_type_code == GRID_SRLL) {
	    gg->rot = (rotated *)emalloc(sizeof(rotated));
	
	    if (var_as_float(nc, VAR_ROTLAT, &gg->rot->lat) == -1) {
		varerr(nc, VAR_ROTLAT);
		return -1;
	    }
	    if (var_as_float(nc, VAR_ROTLON, &gg->rot->lon) == -1) {
		varerr(nc, VAR_ROTLON);
		return -1;
	    }
	    if (var_as_float(nc, VAR_ROTANGLE, &gg->rot->angle) == -1) {
		varerr(nc, VAR_ROTANGLE);
		return -1;
	    }
	}
	
	if (nav->grid_type_code == GRID_SLL || nav->grid_type_code == GRID_SRLL) {
	    gg->strch = (stretched *)emalloc(sizeof(stretched));
	
	    if (var_as_float(nc, VAR_STRETCHLAT, &gg->strch->lat) == -1) {
		varerr(nc, VAR_STRETCHLAT);
		return -1;
	    }
	    if (var_as_float(nc, VAR_STRETCHLON, &gg->strch->lon) == -1) {
		varerr(nc, VAR_STRETCHLON);
		return -1;
	    }
	    if (var_as_float(nc, VAR_STRETCHFACTOR, &gg->strch->factor) == -1) {
		varerr(nc, VAR_STRETCHFACTOR);
		return -1;
	    }
	}
    }
    break;
    case GRID_GAU:
    case GRID_RGAU:
    case GRID_SGAU:
    case GRID_SRGAU:
    {
	gdes_gau *gg = &nav->grid.gau;

	if (var_as_int(nc, VAR_NI, &gg->ni) == -1) {
	    varerr(nc, VAR_NI);
	    return -1;
	    }
	
	if (var_as_int(nc, VAR_NJ, &gg->nj) == -1) {
	    varerr(nc, VAR_NJ);
	    return -1;
	    }
	
	if (var_as_float(nc, VAR_LA1, &gg->la1) == -1) {
	    varerr(nc, VAR_LA1);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LO1, &gg->lo1) == -1) {
	    varerr(nc, VAR_LO1);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LA2, &gg->la2) == -1) {
	    varerr(nc, VAR_LA2);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LO2, &gg->lo2) == -1) {
	    varerr(nc, VAR_LO2);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_DI, &gg->di) == -1) {
	    varerr(nc, VAR_DI);
	    return -1;
	}
	
	if (var_as_int(nc, VAR_N, &gg->n) == -1) {
	    varerr(nc, VAR_N);
	    return -1;
	}

	if (nav->grid_type_code == GRID_RLL || nav->grid_type_code == GRID_SRLL) {
	    gg->rot = (rotated *)emalloc(sizeof(rotated));
	
	    if (var_as_float(nc, VAR_ROTLAT, &gg->rot->lat) == -1) {
		varerr(nc, VAR_ROTLAT);
		return -1;
	    }
	    if (var_as_float(nc, VAR_ROTLON, &gg->rot->lon) == -1) {
		varerr(nc, VAR_ROTLON);
		return -1;
	    }
	    if (var_as_float(nc, VAR_ROTANGLE, &gg->rot->angle) == -1) {
		varerr(nc, VAR_ROTANGLE);
		return -1;
	    }
	}
	
	if (nav->grid_type_code == GRID_SLL || nav->grid_type_code == GRID_SRLL) {
	    gg->strch = (stretched *)emalloc(sizeof(stretched));
	
	    if (var_as_float(nc, VAR_STRETCHLAT, &gg->strch->lat) == -1) {
		varerr(nc, VAR_STRETCHLAT);
		return -1;
	    }
	    if (var_as_float(nc, VAR_STRETCHLON, &gg->strch->lon) == -1) {
		varerr(nc, VAR_STRETCHLON);
		return -1;
	    }
	    if (var_as_float(nc, VAR_STRETCHFACTOR, &gg->strch->factor) == -1) {
		varerr(nc, VAR_STRETCHFACTOR);
		return -1;
	    }
	}
    }
    break;
    case GRID_SPH:
    case GRID_RSPH:
    case GRID_SSPH:
    case GRID_SRSPH:
    {
	gdes_sph *gg = &nav->grid.sph;

	if (var_as_int(nc, VAR_J, &gg->j) == -1) {
	    varerr(nc, VAR_J);
	    return -1;
	}
	
	if (var_as_int(nc, VAR_K, &gg->k) == -1) {
	    varerr(nc, VAR_K);
	    return -1;
	}
	
	if (var_as_int(nc, VAR_M, &gg->m) == -1) {
	    varerr(nc, VAR_M);
	    return -1;
	}
	
	if (var_as_int(nc, VAR_TYPE, &gg->type) == -1) {
	    varerr(nc, VAR_TYPE);
	    return -1;
	}
	
	if (var_as_int(nc, VAR_MODE, &gg->mode) == -1) {
	    varerr(nc, VAR_MODE);
	    return -1;
	}
	if (nav->grid_type_code == GRID_RLL || nav->grid_type_code == GRID_SRLL) {
	    gg->rot = (rotated *)emalloc(sizeof(rotated));
	
	    if (var_as_float(nc, VAR_ROTLAT, &gg->rot->lat) == -1) {
		varerr(nc, VAR_ROTLAT);
		return -1;
	    }
	    if (var_as_float(nc, VAR_ROTLON, &gg->rot->lon) == -1) {
		varerr(nc, VAR_ROTLON);
		return -1;
	    }
	    if (var_as_float(nc, VAR_ROTANGLE, &gg->rot->angle) == -1) {
		varerr(nc, VAR_ROTANGLE);
		return -1;
	    }
	}
	
	if (nav->grid_type_code == GRID_SLL || nav->grid_type_code == GRID_SRLL) {
	    gg->strch = (stretched *)emalloc(sizeof(stretched));
	
	    if (var_as_float(nc, VAR_STRETCHLAT, &gg->strch->lat) == -1) {
		varerr(nc, VAR_STRETCHLAT);
		return -1;
	    }
	    if (var_as_float(nc, VAR_STRETCHLON, &gg->strch->lon) == -1) {
		varerr(nc, VAR_STRETCHLON);
		return -1;
	    }
	    if (var_as_float(nc, VAR_STRETCHFACTOR, &gg->strch->factor) == -1) {
		varerr(nc, VAR_STRETCHFACTOR);
		return -1;
	    }
	}
    }
    break;
    case GRID_MERCAT:
    {
	gdes_mercator *gg = &nav->grid.mercator;

	if (var_as_int(nc, VAR_NI, &gg->ni) == -1) {
	    varerr(nc, VAR_NI);
	    return -1;
	}
	
	if (var_as_int(nc, VAR_NJ, &gg->nj) == -1) {
	    varerr(nc, VAR_NJ);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LA1, &gg->la1) == -1) {
	    varerr(nc, VAR_LA1);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LO1, &gg->lo1) == -1) {
	    varerr(nc, VAR_LO1);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LA2, &gg->la2) == -1) {
	    varerr(nc, VAR_LA2);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LO2, &gg->lo2) == -1) {
	    varerr(nc, VAR_LO2);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LATIN, &gg->latin) == -1) {
	    varerr(nc, VAR_LATIN);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_DI, &gg->di) == -1) {
	    varerr(nc, VAR_DI);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_DJ, &gg->dj) == -1) {
	    varerr(nc, VAR_DJ);
	    return -1;
	}
	
    }
    break;
    case GRID_GNOMON:		/* fall through */
    case GRID_POLARS:
    {
	gdes_polars *gg = &nav->grid.polars;

	if (var_as_int(nc, VAR_NX, &gg->nx) == -1) {
	    varerr(nc, VAR_NX);
	    return -1;
	}
	
	if (var_as_int(nc, VAR_NY, &gg->ny) == -1) {
	    varerr(nc, VAR_NY);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LA1, &gg->la1) == -1) {
	    varerr(nc, VAR_LA1);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LO1, &gg->lo1) == -1) {
	    varerr(nc, VAR_LO1);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LOV, &gg->lov) == -1) {
	    varerr(nc, VAR_LOV);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_DX, &gg->dx) == -1) {
	    varerr(nc, VAR_DX);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_DY, &gg->dy) == -1) {
	    varerr(nc, VAR_DY);
	    return -1;
	}
	
	if (var_as_int(nc, VAR_PROJFLAG, &gg->pole) == -1) {
	    varerr(nc, VAR_PROJFLAG);
	    return -1;
	}
    }
    break;
    case GRID_LAMBERT:
    {
	gdes_lambert *gg = &nav->grid.lambert;

	if (var_as_int(nc, VAR_NX, &gg->nx) == -1) {
	    varerr(nc, VAR_NX);
	    return -1;
	}
	
	if (var_as_int(nc, VAR_NY, &gg->ny) == -1) {
	    varerr(nc, VAR_NY);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LA1, &gg->la1) == -1) {
	    varerr(nc, VAR_LA1);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LO1, &gg->lo1) == -1) {
	    varerr(nc, VAR_LO1);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LOV, &gg->lov) == -1) {
	    varerr(nc, VAR_LOV);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_DX, &gg->dx) == -1) {
	    varerr(nc, VAR_DX);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_DY, &gg->dy) == -1) {
	    varerr(nc, VAR_DY);
	    return -1;
	}
	
	if (var_as_int(nc, VAR_PROJFLAG, &gg->pole) == -1) {
	    varerr(nc, VAR_PROJFLAG);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LATIN1, &gg->latin1) == -1) {
	    varerr(nc, VAR_LATIN1);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LATIN2, &gg->latin2) == -1) {
	    varerr(nc, VAR_LATIN2);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_SPLAT, &gg->splat) == -1) {
	    varerr(nc, VAR_SPLAT);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_SPLON, &gg->splon) == -1) {
	    varerr(nc, VAR_SPLON);
	    return -1;
	}
	
    }
    break;
    case GRID_SPACEV:
    {
	gdes_spacev *gg = &nav->grid.spacev;

	if (var_as_int(nc, VAR_NX, &gg->nx) == -1) {
	    varerr(nc, VAR_NX);
	    return -1;
	}
	
	if (var_as_int(nc, VAR_NY, &gg->ny) == -1) {
	    varerr(nc, VAR_NY);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LAP, &gg->lap) == -1) {
	    varerr(nc, VAR_LAP);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_LOP, &gg->lop) == -1) {
	    varerr(nc, VAR_LOP);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_DX, &gg->dx) == -1) {
	    varerr(nc, VAR_DX);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_DY, &gg->dy) == -1) {
	    varerr(nc, VAR_DY);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_XP, &gg->xp) == -1) {
	    varerr(nc, VAR_XP);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_YP, &gg->yp) == -1) {
	    varerr(nc, VAR_YP);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_ORIENTATION, &gg->orient) == -1) {
	    varerr(nc, VAR_ORIENTATION);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_NR, &gg->nr) == -1) {
	    varerr(nc, VAR_NR);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_XO, &gg->xo) == -1) {
	    varerr(nc, VAR_XO);
	    return -1;
	}
	
	if (var_as_float(nc, VAR_YO, &gg->yo) == -1) {
	    varerr(nc, VAR_YO);
	    return -1;
	}
	
    }
    break;
    case GRID_ALBERS:
    case GRID_OLAMBERT:
    case GRID_UTM:
    case GRID_SIMPOL:
    case GRID_MILLER:
    default:
	logFile->write_time("Error: can't handle %s grids\n",
	       gds_typename(nav->grid_type_code));
	return -1;
    }
    return 0;
}


/*
 * Make an in-memory structure for netCDF navigation information and
 * initialize it from specified file.  We cache this information because
 * some of it must be consulted for every decoded product, and it won't
 * change.  Returns 0 on success, -1 on failure.
 */
static int
make_navinfo(
    ncfile *nc,			/* netCDF file */
    navinfo *nav		/* where to put the navinfo */
	)
{
    
    nav->navid = nuwg_getdim(ncid, DIM_NAV);

    if (var_as_int(nc, VAR_GRID_TYPE_CODE, &nav->grid_type_code) == -1) {
	varerr(nc, VAR_GRID_TYPE_CODE);
	return -1;
    }

    if (var_as_int(nc, VAR_GRID_CENTER, &nav->grid_center) == -1) {
	varerr(nc, VAR_GRID_CENTER);
	return -1;
    }

    /* Multiple grid numbers allowed, they get stitched together */
    if (var_as_lset(nc, VAR_GRID_NUMBER, &nav->grid_numbers) == -1) {
	varerr(nc, VAR_GRID_NUMBER);
	return -1;
    }

    /* GRIB resolution and component flags */
    if (var_as_int(nc, VAR_RESCOMP, &nav->rescomp) == -1) {
	varerr(nc, VAR_RESCOMP);
	return -1;
    }

    if (make_navgrid(nc, nav) == -1) {
	return -1;
    }

    return 0;
}


static void
free_navinfo(
    navinfo* np)
{
    if (np) {
	if(np->nav_model)
	    free(np->nav_model);
	if(np->grid_numbers.vals)
	    free(np->grid_numbers.vals);
	free(np);
    }
}

static navinfo *
new_navinfo(
    ncfile *nc)
{
    navinfo *out = (navinfo *)emalloc(sizeof(navinfo));

    if (make_navinfo(nc, out) != 0) {
	free_navinfo(out);
	return 0;
    }

    return out;
}
#endif


/*
 * Make an in-memory structure for netCDF information and initialize it from
 * specified file.  We cache this information because some of it must be
 * consulted for every decoded product, and it won't change.
 */
static int
make_ncfile(
    char *ncname,
    ncfile *out)
{
    int ndims;			/* number of dimensions */
    int nvars;			/* number of variables */
    int recid;
    int dimid;
    int varid;

    out->ncname = estrdup(ncname);
    out->ncid = ncid;

    if (nc_inq(ncid, &ndims, &nvars, (int *)0, &recid) != NC_NOERR) {
	logFile->write_time("Error: ncinquire() failed\n");
	return -1;
    }
    out->ndims = ndims;
    out->nvars = nvars;
    out->dims = (ncdim **)emalloc(ndims * sizeof(ncdim *));
    for (dimid = 0; dimid < ndims; dimid++) {
	out->dims[dimid] = new_dim(dimid);
    }
    out->vars = (ncvar **)emalloc(nvars * sizeof(ncvar *));
    for (varid = 0; varid < nvars; varid++) {
      out->vars[varid] = new_var(ncname, varid);
    }

    if (recid == -1) {
	logFile->write_time("Error: no record dimension\n");
	return -1;
    }
    out->recid = recid;
    out->reftimeid = nuwg_getvar(ncid, VAR_REFTIME);
    out->valtimeid = nuwg_getvar(ncid, VAR_VALTIME);
    if(out->reftimeid == -1) {
	logFile->write_time("Error: no reftime variable\n");
	return -1;
    }
    if(out->valtimeid == -1) {
	logFile->write_time("Error: no valtime variable\n");
	return -1;
    }
    out->datetimeid = nuwg_getvar(ncid, VAR_DATETIME);
    out->valoffsetid = nuwg_getvar(ncid, VAR_VALOFFSET);
    if(out->datetimeid == -1) {
	logFile->write_time("Error: no datetimeid variable\n");
	return -1;
    }
    if(out->valoffsetid == -1) {
	logFile->write_time("Error: no valoffsetid variable\n");
	return -1;
    }

    /* initialize reftime,valtime,record table */
    if (new_recs(out) == -1) {
	logFile->write_time("Error: can't initialize reftime,valtime table\n");
	return -1;
    }

#ifdef DONT_NEED_FOR_SITE_DATA   
    /* Multiple model numbers allowed, e.g. for initialization */
    if (var_as_lset(out, VAR_MODELID, &out->models) == -1) {
	varerr(out, VAR_MODELID);
	return -1;
    }

    out->nav = new_navinfo(out);
    if (!out->nav) {		/* get navigation information */
	logFile->write_time("Error: can't get navigation information\n");
	return -1;
    }
#endif

    out->levdims = 0;		/* only add level dimensions as needed */
    out->laydims = 0;		/* only add layer dimensions as needed */
    
    return 0;
}


void
free_ncfile(
    ncfile* np)
{
    if (np) {
	if(np->ncname)
	    free(np->ncname);

	if(np->dims) {
	    int dimid;
	    for (dimid = 0; dimid < np->ndims; dimid++) {
		free_dim(np->dims[dimid]);
	    }
	    free(np->dims);
	}

	if(np->vars) {
	    int varid;
	    for (varid = 0; varid < np->nvars; varid++) {
	      if (np->vars[varid])
		free_var(np->vars[varid]);
	    }
	    free(np->vars);
	}
	if(np->rt)
	    free_recs(np->rt);

#ifdef DONT_NEED_FOR_SITE_DATA
	if(np->models.vals)
	  free(np->models.vals);

	if(np->nav)
	  free_navinfo(np->nav);
#endif

	if(np->levdims)
	  {
	    levdim *lp = np->levdims;

	    while (lp)
	      {
		if (lp->levtab->vals)
		  free(lp->levtab->vals);

		if (lp->levtab->bunitp) {
		  utFree(lp->levtab->bunitp);
		  free(lp->levtab->bunitp);
		}

		free(lp->levtab);

		levdim *p = lp;
		lp = p->next;
		if (p)
		  free(p);
	      }
	  }

	if(np->laydims)
	  {
	    laydim *lp = np->laydims;

	    while (lp)
	      {
		if (lp->laytab->tops)
		  free(lp->laytab->tops);

		if (lp->laytab->bots)
		  free(lp->laytab->bots);

		if (lp->laytab->bunitp) {
		  utFree(lp->laytab->bunitp);
		  free(lp->laytab->bunitp);
		}

		free(lp->laytab);

		laydim *p = lp;
		lp = p->next;
		if (p)
		  free(p);
	      }
	  }

	free(np);
    }
}


/*
 * Creates a new ncfile structure and fills it in with the information from
 * the open netCDF file whose handle is ncid.  A pointer to the structure is
 * returned, or 0 on failure.
 */
ncfile *
new_ncfile(
    char *ncname)
{
    ncfile *out = (ncfile *)emalloc(sizeof(ncfile));

    if (make_ncfile(ncname, out) != 0) {
	logFile->write_time("Error: make_ncfile failed\n");
	free_ncfile(out);
	return 0;
    }

    return out;
}

#ifdef USED
/*
 * Print an error message if two specified floating-point values are not
 * sufficiently close.  Returns 1 if not close, zero otherwise.
 */
static int
gd_fne_err(
    product_data *pp,		/* decoded GRIB product */
    ncfile *nc,			/* netCDF file */
    enum ncpart comp,		/* name-independent id of component */
    float pval,			/* first value, from GRIB product */
    float nval			/* second value, from netCDF file */
	)
{
#define float_near(x,y)	((float)((y) + 0.1*fabs((x)-(y))) == (float)(y)) /* true if x is "close to" y */
    if (! float_near(pval, nval)) {
	logFile->write_time("Error: GRIB %s: %s nav. mismatch %s: %g != %g\n",
	       pp->header, nc->ncname, nuwg_name(comp), pval, nval);
	return 1;
    }
    return 0;    
}

/*
 * Print an error message if two specified floating-point values are not
 * integer close.  Returns 1 if not close, zero otherwise.
 */
static int
gd_fnei_err(
    product_data *pp,		/* decoded GRIB product */
    ncfile *nc,			/* netCDF file */
    enum ncpart comp,		/* name-independent id of component */
    float pval,			/* first value, from GRIB product */
    float nval			/* second value, from netCDF file */
	)
{
    if ((int) (pval+0.5) != (int) (nval+0.5)) {
	logFile->write_time("Error: GRIB %s: %s nav. mismatch %s: %d != %d\n",
	       pp->header, nc->ncname, nuwg_name(comp), (int) (pval+0.5), 
	       (int) (nval+0.5));
	return 1;
    }
    return 0;    
}


/*
 * Print an error message if two specified int values are not
 * equal.  Return 1 if not equal, zero otherwise.
 */
static int
gd_ine_err(
    product_data *pp,		/* decoded GRIB product */
    ncfile *nc,			/* netCDF file */
    enum ncpart comp,		/* name-independent id of component */
    int pval,			/* first value, from GRIB product */
    int nval			/* second value, from netCDF file */
	)
{
    if (pval != nval) {
	logFile->write_time("Error: GRIB %s: %s nav. mismatch %s: %d != %d\n",
	       pp->header, nc->ncname, nuwg_name(comp), pval, nval);
	return 1;
    }
    return 0;    
}


/*
 * Print an error message if first int value is greater than second.
 * Return 1 if not equal, zero otherwise.
 */
static int
gd_igt_err(
    product_data *pp,		/* decoded GRIB product */
    ncfile *nc,			/* netCDF file */
    enum ncpart comp,		/* name-independent id of component */
    int pval,			/* first value, from GRIB product */
    int nval			/* second value, from netCDF file */
	)
{
    if (pval > nval) {
	logFile->write_time("Error: GRIB %s: %s nav. mismatch %s: %d > %d\n",
	       pp->header, nc->ncname, nuwg_name(comp), pval, nval);
	return 1;
    }
    return 0;    
}

/*
 * Figure out subgrid location from netCDF navigation information and
 * product Grid Description Section information.  Returns 0 on success, -1
 * on failure.
 */
static int
subgrid(
    ncfile *nc,
    product_data *pp,
    long *ix0,
    long *ix1
	)
{
    gdes_ll *gll;
    float plon;

    *ix0 = 0;
    *ix1 = 0;

    switch (pp->gd->type) {
    case GRID_LL:
    case GRID_RLL:
    case GRID_SLL:
    case GRID_SRLL:

      if((pp->gd->scan_mode & 0x20) == 1 || /*adjacent points in j direction */
	 (pp->gd->scan_mode & 0x80) == 1 ) { /* points scan in -i direction */
	    logFile->write_time("Error: GRIB %s: can't handle scan mode of %x\n",
		   pp->header,pp->gd->scan_mode);
	    return -1;
	}

	/* If scanning mode flag indicates North to South scan, we
	   reverse the rows so we can always assume South to North scan */
	if((pp->gd->scan_mode & 0x40) == 0) { /* north to south */
	    int row;
	    int nrows = pp->gd->nrows;
	    int ncols = pp->gd->ncols;
	    float tmp;
	    
	    for (row = 0; row < nrows/2; row++) {
		int col;
		float *upper = pp->data + row * ncols;
		float *lower = pp->data + (nrows - 1 - row) * ncols;
		/* swap row (upper) and nrows-1-row (lower) */
		for (col = 0; col < ncols; col++) {
		    tmp = *upper;
		    *upper = *lower;
		    *lower = tmp;
		    upper++;
		    lower++;
		}
	    }
	    gll = &pp->gd->grid.ll;
	    tmp = gll->la1;
	    gll->la1 = gll->la2;
	    gll->la2 = tmp;
	    pp->gd->scan_mode |= 0x40;
	}	

        /* Compare pp->gdes->grid.ll->la1 to value of La1, La2 netCDF
	   variables and similarly for lo1, lo2. */
	gll = &pp->gd->grid.ll;
	plon = gll->lo1;
	while (plon < nc->nav->grid.ll.lo1)
	    plon += 360.0 ;
        /* handle case where right edge is same as left edge */
	if (plon == nc->nav->grid.ll.lo2 &&
	    nc->nav->grid.ll.lo1 + 360 ==  nc->nav->grid.ll.lo2) {
	    plon = nc->nav->grid.ll.lo1;
	}
	*ix0 = (long)((gll->la1 - nc->nav->grid.ll.la1) /nc->nav->grid.ll.dj + 0.5);
	*ix1 = (long)((plon - nc->nav->grid.ll.lo1) /nc->nav->grid.ll.di + 0.5);
    }
    /* default: */
    return 0;
}

#endif // USED



/*
 * Handle writing extra time-range indicator information, if any, in
 * auxilliary variables.  For example, accumulation interval for
 * precipitation variables would be handled here.

 * For now, we do this in an ad hoc way, until we have a mechanism for
 * writing general auxilliary GRIB info associated with a variable.
 */
static int
triaux(
    product_data *pp,	/* decoded GRIB data to be written */
    ncfile *nc,		/* netCDF file to be written */
    ncvar *var,		/* netCDF variable to be written */
    size_t *start	/* index where variable to be written */
	)
{
    char tri_name[NC_MAX_NAME];
    char *suf;
    //ncvar *trivar;
    int trivarid;
    size_t ix[2];
    size_t count[2];
    float trivals[2];
    int ret;

    if(pp->tr_flg == TRI_P1 || pp->tr_flg == TRI_LP1)
	return 0;      /* usually time range info is in valid_time variable */

    /* check if auxilliary time-range variable exists */
    suf = trisuffix(pp->tr_flg);
    if (strlen(var->name) + 1 + strlen(suf) > (size_t) NC_MAX_NAME) {
	logFile->write_time("Error: name of %s TRI variable too long (%s)\n",
	       suf, var->name);
	return -1;
    }

    strcpy(tri_name, var->name);
    strcat(tri_name, "_");
    strcat(tri_name, suf);
    ret = nc_inq_varid(ncid, tri_name,&trivarid);
    if(ret != NC_NOERR) {
	return 0;		/* not an error, since optional whether file
				 * has auxilliary time-range variable */
    }

    /* *** should check units of _accum_len variable and convert to
       those, use float_nc() *** */
    //trivar = nc->vars[trivarid];

    switch (trinum(pp->tr_flg)) {
    case 2:
	ix[0]=start[0];		/* record number */
	ix[1]=0;
	count[0] = 1;
	count[1] = 2;		/* two values for time range */

        switch (pp->tunit) {

          case TUNITS_HOUR:
	    trivals[0] = pp->tr[0];
	    trivals[1] = pp->tr[1];
            break;
          case TUNITS_3HR:
	    trivals[0] = pp->tr[0] * 3;
	    trivals[1] = pp->tr[1] * 3;
            break;
          case TUNITS_6HR:
	    trivals[0] = pp->tr[0] * 6;
	    trivals[1] = pp->tr[1] * 6;
            break;
          case TUNITS_12HR:
	    trivals[0] = pp->tr[0] * 12;
	    trivals[1] = pp->tr[1] * 12;
            break;
          default:
            logFile->write_time("Error: unusual time unit for accumulation: %d\n", pp->tunit);
            return 0;
        }
	if (nc_put_vara_float(ncid, trivarid, ix, count, trivals) != NC_NOERR) {
	    logFile->write_time("Error: can't write accum_len variable for (%s)\n", var->name);
	    return -1;
	}
	break;
    case 1:
	ix[0]=start[0];
	trivals[0] = frcst_time(pp);
	if (nc_put_var1_float(ncid, trivarid, ix, trivals) != NC_NOERR) {
	    logFile->write_time("Error: can't write accum_len variable for (%s)\n", var->name);
	    return -1;
	}
	break;
    default:
	    logFile->write_time("Error: can't handle time flag %d for variable (%s)\n", pp->tr_flg, var->name);
	    return -1;
    }
    return 0;
}


/*
 * Checks that the grid being processed is in the output nc file.
 * Returns 0 if grid is in the output file, -1 if not.
 */
int
nc_check(
    product_data *pp,	/* decoded GRIB product to be written */
    ncfile *nc		/* netCDF file to write */
    )
{
    ncvar *var;
    int varid;
    char *cp = parmname(nc, pp);


    if (!cp) {
	logFile->write_time(1, "Warning: GRIB %s: unrecognized (param,level_flg) combination (%d,%d)\n", pp->header, pp->param, pp->level_flg);
	return(-1);
    }

    /* locate variable in output netCDF file */
    int ret;
    ret = nc_inq_varid(ncid, cp, &varid);
    if (ret != NC_NOERR) {
      logFile->write_time(1, "Warning: GRIB %s: no variable %s in %s\n",
			  pp->header, cp, nc->ncname);
      return(-1);
    }
    var = nc->vars[varid];

    if (!var) {
       logFile->write_time(1, "Warning: GRIB %s: could not handle %s\n", pp->header, cp);
       return(-1);
    }

    /* handle level dimension, if any */
    if (getlev(pp, nc, var) == -1) {
      logFile->write_time(1, "Warning: GRIB %s: could not handle level for %s\n",
			  pp->header, cp);
      return(-1);
    }

    // The variable is in the file!!
    return(0);
}


/*
 * Writes decoded GRIB product to netCDF file open for writing. It is
 * assumed that nc_check() has already been called to verify that the variable
 * is in the output file. Returns the number of things written.
 */
int
nc_write(
    product_data *pp,	/* decoded GRIB product to be written */
    ncfile *nc,		/* netCDF file to write */
    float *lat, 	/* site latitudes */
    float *lon,		/* site longitudes */
    int num_sites      /* Number of sites (lat/lon pairs) */
    )
{
    double reftime, valtime;
    long rec = 0;
    ncvar *var;
    char varname[NC_MAX_NAME];
    int varid;
    char *cp = parmname(nc, pp); /* var to write */
    int dim;		 	 /* which dimension we are dealing with */
    double slope, intercept;     /* Use for units conversion */
    float *site_data;            /* grid values at site locations */
    float fillval;               /* fill value defined in output nc file */
    int nwritten = 0;

#define MAX_PARM_DIMS	4	/* Maximum dimensions for a parameter.
				   X(rec,lev,ens,site) lev, ens optional */

    size_t start[MAX_PARM_DIMS];
    size_t count[MAX_PARM_DIMS];

    //
    // Normally there is one output variable per GRIB message, but we want to
    // be able to create some derived things, such as gradients in a certain
    // direction, so set up a list of additional output variables to look for.
    // For example, for variable T, we might also calculate T_gradx and T_grady,
    // if they exist in the output netCDF file.
    //
    // This is a little awkward but saves having to replicate a lot of this
    // code. In this section we also determine the interpolation method for
    // the non-derived variables from the 'interpolation_method' attribute of
    // the netCDF variable.
    //
    static char *calc_types[] = { (char *)"",
				  (char *)"gradx",
				  (char *)"grady" };

    int num_calc_types = sizeof(calc_types)/sizeof(*calc_types);

    char calc_type[NC_MAX_NAME];


    /* Loop over the calculation type list */

    for (int v=0; v<num_calc_types; v++) {
 
      memset(calc_type, 0, NC_MAX_NAME);
      strcpy(calc_type, calc_types[v]);
      strcpy(varname, cp);

      // Create varname of the form "VAR_xxxx" for 'derived' vars.
      if (strcmp(calc_type, "") != 0) {
	strcat(varname, "_");
	strcat(varname, calc_type);
      }

      /* locate variable in output netCDF file */
      int ret;
      ret = nc_inq_varid(ncid,varname,&varid);
      if (ret != NC_NOERR) {
	continue;
      }

      logFile->write_time(1, "Info: GRIB %s: processing %s\n",
			  pp->header, varname);

      /* Get the interpolation_method attribute if needed. If attribute is
         not defined, default to bilinear. */
      if (strcmp(calc_type, "") == 0) {
	if (nc_get_att_text(ncid, varid, INTERP_METHOD_NAME, calc_type) != NC_NOERR)
	  strcpy(calc_type, "bilinear");
      }

      var = nc->vars[varid];

      dim = 0;
      if (var->dims[0] != nc->recid) { /* no record dimension */
	dim--;
      } else {			/* handle record dimension */
	humtime ht;
	
	rvhours(pp, nc, &reftime, &valtime, &ht);
	
	rec = getrec(nc, reftime, valtime, &ht); /* which record to write */
	if (rec < 0)
	  return (-1);
	
	start[dim] = rec;
	count[dim] = 1;
      }
     
      /* Handle auxilliary time-range indicator information, if any */
      triaux(pp, nc, var, start) ;
      
      /* handle level dimension, if any */
      long lev = getlev(pp, nc, var);    
      if (lev == -1) {
	continue;
      }

      if(lev >= 0) {
	dim++;
	start[dim] = lev;
	count[dim] = 1;
      }

      /* Check for possible ensemble member dimension */
      long member = getens(lev, pp, nc, var);
      if (member == -1) {
	continue;
      }

      if (member >= 0) {
	dim++;
	start[dim] = member;
	count[dim] = 1;
      }
            
      /* always increment dimension for num_sites */
      dim++;
      start[dim] = 0;
      count[dim] = num_sites;
      
      if (var->uc) {		/* units conversion */
	slope = var->uc->slope;
	intercept = var->uc->intercept;
      } else {
	slope = 1.0;
	intercept = 0.0;
      }
      
      /* Allocate space for the site_data */
      site_data = (float*) emalloc(num_sites*sizeof(float));
      
      /* Get the fill value attribute. Use default if not there */
      if (nc_get_att_float(ncid, varid, FILL_NAME, &fillval) != NC_NOERR) {
	fillval = NC_FILL_FLOAT;
      }

      /* Read existing data from netcdf file. (This allows us to process
	 grids in tiles.) Units are reverted back to the original units
	 since we will convert them again on output. */
      nc_float(ncid, varid, start, count, site_data, fillval, 1/slope, -intercept);

      /* Get data values at sites from the grid */
      if (!make_site_data(pp, fillval, calc_type, lat, lon, num_sites, site_data)) {
	free(site_data);
	continue;
      }
      
      /* Write the data */
      if (float_nc(ncid, varid, start, count,
		   site_data, slope, intercept, fillval) == -1) {
	free(site_data);
	logFile->write_time("Error: GRIB %s: writing %s in %s\n",
			    pp->header, varname, nc->ncname);
	return (-1);
	//continue;
      }
      
      //if (nc_sync(ncid) != NC_NOERR) {
      //free(site_data);
      //logFile->write_time("Error: GRIB %s: syncing file %s\n", pp->header,
      //		    nc->ncname);
      //continue;
       //}

      /* Build up string to log what was written to output file */
      nwritten++;
      char log_str[80];
      sprintf(log_str, "Info: GRIB %s: wrote %s(",  pp->header, varname);

      /* record dimension */
      if (var->dims[0] == nc->recid)
	sprintf(&log_str[strlen(log_str)], "%ld,", rec);

      /* level dimension */
      if (lev >= 0)
	sprintf(&log_str[strlen(log_str)], "%ld,", lev);

      /* ensemble member dimension */
      if (member >= 0)
	sprintf(&log_str[strlen(log_str)], "%ld,", member);

      /* finalize and write string */
      sprintf(&log_str[strlen(log_str)], "*) to %s", nc->ncname);
      logFile->write_time(1, "%s\n", log_str);

      free(site_data);
    }

    return(nwritten);
}
