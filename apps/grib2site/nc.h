/*
 *   Copyright 1996, University Corporation for Atmospheric Research.
 *   See ../COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: nc.h,v 1.5 2013/11/22 19:08:33 dicast Exp $ */

/* netCDF-specific structures and functions */

#ifndef NC_H_
#define NC_H_

#include <netcdf.h>
#include "netcdf_c++/netcdf.hh"
#include <udunits.h>
#include "product_data.h"
#include "gdes.h"
#include "site_list.h"

typedef struct unitconv {	/* units conversion parameters */
    double slope;		/* for GRIB edition 1 units */
    double intercept;
} unitconv;

typedef struct ncdim {		/* netCDF dimension */
    int id;
    char *name;
} ncdim;

typedef struct ncvar {		/* netCDF var. with extra convenience fields */
    int id;			/* netCDF variable id */
    char *name;			/* name of variable */
    nc_type type;		/* data type */
    int ndims;			/* number of dimensions */
    int *dims;			/* array of ndims dimension ids */
    float *fillval;		/* fill value, if any */
    utUnit *bunitp;		/* units attribute, if any */
    unitconv *uc;		/* units conversion from GRIB needed, if any */
} ncvar;

typedef struct lset {	/* fixed-size list of integers */
    long n;			/* size of list */
    int *vals;		/* integers in list */
  //nclong *vals;		/* integers in list */
} lset;

struct levels_table;		/* defined in nc.c */

struct levdim {			/* list of level dimensions */
    struct levels_table *levtab; /* corresponding level table */
    struct levdim *next;
};

typedef struct levdim levdim;

struct layers_table;		/* defined in nc.c */

struct laydim {			/* list of layer dimensions */
    struct layers_table *laytab; /* corresponding layer table */
    struct laydim *next;
};

typedef struct laydim laydim;

typedef struct navinfo {
    int navid;			/* navigation dimension */
    char *nav_model;		/* navigation parameterization */
    int grid_type_code;		/* GRIB GDS data representation type */
    int grid_center;		/* GRIB originating center ID */
    lset grid_numbers;		/* GRIB center-catalogued grid numbers */
    int rescomp;		/* GRIB resolution and component flags */
    gengrid grid;		/* union of diverse kinds of grids */
} navinfo;

struct rectimes;		/* forward declaration */

typedef struct ncfile {
    char *ncname;		/* file name */
    int ncid;			/* handle */
    int ndims;			/* number of dimensions (uneeded?) */
    int nvars;			/* number of variables */
    ncdim **dims;		/* array of ndims dimensions */
    ncvar **vars;		/* array of nvars variables */
    int recid;			/* record dimension */
    int reftimeid;		/* reference time variable id */
    int valtimeid;		/* valid time variable id */
    lset models;		/* list of model numbers */
    navinfo *nav;		/* navigation information */
    levdim *levdims;		/* list of level dimensions */
    laydim *laydims;		/* list of layer dimensions */
    int datetimeid;		/* datetime variable id, if any */
    int valoffsetid;		/* valoffset variable id, if any */
    struct rectimes *rt;	/* table of reftimes,valtimes,records */
} ncfile;


#ifdef __cplusplus
extern "C" int cdl_netcdf(char *cdlname, char* ncname);
extern "C" void setncid(int ncid);
extern "C" int getncid(void);
extern "C" void nccleanup(void);
extern "C" ncfile *new_ncfile(char *ncname);
extern "C" void free_ncfile(ncfile *nc);
extern "C" int nc_write(product_data *, ncfile *, float *, float *, int);
extern "C" int nc_check(product_data *, ncfile *);
#elif defined(__STDC__)
extern int cdl_netcdf(char *cdlname, char* ncname);
extern void setncid(int ncid);
extern int getncid(void);
extern void nccleanup(void);
extern ncfile *new_ncfile(char *ncname);
extern void free_ncfile(ncfile *nc);
extern int nc_write(product_data *, ncfile *, float *, float *, int ns);
extern int nc_check(product_data *, ncfile *);
#else
extern int cdl_netcdf( /* char *cdlname, char* ncname */ );
extern void setncid( /* int ncid */ );
extern int getncid( /* */ );
extern void nccleanup( /* */ );
extern ncfile *new_ncfile( /* char *ncname */ );
extern void free_ncfile( /*ncfile *nc */ );
extern int nc_write( /* product_data *, ncfile *, float *, float *, int */);
extern int nc_write( /* product_data *, ncfile * */ );
#endif

#endif /* NC_H_ */
