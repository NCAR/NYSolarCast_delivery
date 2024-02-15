/*
 *   Copyright 1995, University Corporation for Atmospheric Research.
 *   See ../COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: product_data.h,v 1.6 2012/06/12 19:12:06 cowie Exp $ */

/*
 * Easy-to-access representation for a decoded GRIB product.  In this
 * structure, numbers have been converted to ints and floats, units have
 * been conventionalized, pole values have been replicated, etc.
 */

#ifndef PRODUCT_DATA_H_
#define PRODUCT_DATA_H_

namespace GRIB2 {
#include "grib2c/grib2_int.h"
}

#include "gdes.h"		/* GDS info in a more accessible grid
				   description structure */
#include "gbytem.h"		/* Bitmap Section info in a more accessible
				   structure */
#include "gbds.h"		/* Binary Data Section info in a more
				   accessible structure */
#include "ens.h"                /* For ensemble grid information */

/*
 * Structure for GRIB product data, including the stuff of interest from the
 * Indicator Section, Product Definition Section, Grid Description Section
 * (manufactured, if necessary), and a bytemap (manufactured, if necessary).
 * We trade space for access ease in this structure. In particular, 1-, 2-,
 * and 3-byte sized numbers become ints.
 * "Missing" integers are set to GDES_INT_MISSING.
 * "Missing" floats are set to GDES_FLOAT_MISSING.
 */
struct product_data {
    char            delim[4];	/* "GRIB", indicator block */
    int		    edition;	/* 0 or 1, so far */
    int             center;	/* NMC is 7 */
    int             subcenter;	/* 0 if none */
    int             model;	/* allocated by the originating center */
    int             grid;	/* Grid ID, may be center-specific */
    int		    param_vers;	/* parameter table version */
    int             param;	/* Indicator of parameter */
    int             der_flg;    /* indicator of type of derived variable */
    int             pctl_flg;   /* indicator of percentile variable */
    int             level_flg ;	/* Indicator of type of level */
    int             level[2] ;
    int		    century;	/* ref time of data, =20 until 1 Jan 2001 */
    int             year;	/* of century */
    int             month;
    int             day;
    int             hour;
    int             minute;
    int             tunit;	/* Indicator of forecast time unit */
    int             tr[2];	/* Time range */
    int             tr_flg;	/* Time range flag */
    int             avg ;	/* when tr_flg indictes an average */
    int		    missing;	/* # missing from averages, accumulations */
    int		    has_gds;	/* 1 if a Grid Description Section was
                                   supplied, 0 otherwise */
    int		    has_bms;	/* 1 if a a Bit Map Section was supplied, 0
                                   otherwise */
    int		    scale10;	/* decimal scale factor exponent */
    int             cols;	/* 'columns' per 'row' */
    int             npts ;	/* # of floating point values */
    int             bits ;	/* # of bits used in GRIB encoding */
    ens             *ensemble;  /* Used if grid is a member of an ensemble */
    char            *header;	/* WMO header (or manufactured product ID) */
    gdes	    *gd;	/* grid description */
    gbytem	    *bm;	/* byte map of values */
    gbds	    *bd;	/* binary data parameters */
    float           *data ;	/* unpacked data values */
};

typedef struct product_data product_data;


#ifdef __cplusplus
/* Fillin preallocated product_data from raw grib1, grib2 */
extern "C" int make_grib1_pdata(grib1 *g, int unpack, product_data *pd);
extern "C" int make_grib2_pdata(char *id, GRIB2::gribfield *g, product_data *pd);
/* Allocate a new product_data and fill in from raw grib1, grib2 */
extern "C" product_data* new_grib1_pdata(grib1*, int);
extern "C" product_data* new_grib2_pdata(char *id, GRIB2::gribfield*);
/* Free product_data */
extern "C" void free_product_data(product_data*);
/* Modify product_data for ECMWF */
extern "C" void modify_ecmwf_pdata(product_data *out, pds *pdsp);
/* Modify product_data for BOM */
extern "C" void modify_bom_pdata(int table, int parm,  pds *pdsp,
				   product_data *out);
#elif defined(__STDC__)
/* Fillin preallocated product_data from raw grib1, grib2 */
extern int make_grib1_pdata(grib1*, int, product_data *);
extern int make_grib2_pdata(char *, GRIB2::gribfield*, product_data *);
/* Allocate a new product_data and fill in from raw grib1, grib2 */
extern product_data* new_grib1_pdata(grib1*, int);
extern product_data* new_grib2_pdata(char *id, GRIB2::gribfield*);
/* Free product_data */
extern void free_product_data(product_data*);
/* Modify product_data for ECMWF */
extern void modify_ecmwf_pdata(product_data*, pds*);
/* Modify product_data for BOM */
extern void modify_bom_pdata(int, int, pds*, product_data*);
#else
/* Fillin preallocated product_data from raw grib1, grib2 */
extern int make_grib1_pdata( /* grib1*, int product_data * */);
extern int make_grib2_pdata( /* char*, GRIB2::gribfield*, product_data* */);
/* Allocate a new product_data and fill in from raw grib1, grib1 */
extern product_data* new_grib1_pdata( /* grib1*, int */ );
extern product_data* new_grib2_pdata( /* char* GRIB2::gribfield* */ );
/* Free product_data */
extern void free_product_data( /* product_data* */ );
/* Modify product_data for ECMWF */
extern void modify_ecmwf_pdata(/* product_data*, pds* */);
/* Modify product_data for BOM */
extern void modify_bom_pdata(/* int, int, pds*, product_data* */);
#endif

#endif /* PRODUCT_DATA_H_ */
