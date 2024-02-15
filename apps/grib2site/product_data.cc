/*
 *   Copyright 1994 University Corporation for Atmospheric Research
 *   See ../COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: product_data.cc,v 1.23 2018/07/25 21:35:59 dicast Exp $ */

#include <iostream>
#include <stdlib.h>
#include "log/log.hh"
#include <string.h>
#include "emalloc.h"
#include "product_data.h"
#include "gbds.h"		/* for unpackbds() */
#include "centers.h"
#include "params.h"
#include "levels.h"
#include "timeunits.h"
#include "units.h"

extern Log *logFile;

/*
 * Free product_data
 */
void
free_product_data(
    product_data *pd)
{
    if (pd) {
	if(pd->header)
	    free(pd->header);
	if(pd->gd)
	  free_gdes(pd->gd);
	if (pd->bm)
	  free_gbytem(pd->bm);
	if (pd->bd)
	  free_gbds(pd->bd);
	if(pd->ensemble)
	  free(pd->ensemble);
	if(pd->data)
	    free(pd->data);
	free(pd);	
    }
}


/*
 * Create product_data structure from raw GRIB 1 structure.
 * If there is no grid description section, we manufacture one.
 * We manufacture a bytemap, even if there is no bitmap section.
 * "unpack" variable dictates whether data is unpacked or not.
 * Returns 0 if successful.
 */
int
make_grib1_pdata(
		 grib1 *gp,
		 int unpack,
		 product_data *out)
{
    ids *idsp = gp->idsp;	/* indicator section */
    pds *pdsp = gp->pdsp;	/* product description section */
    bms *bmsp = gp->bmsp;	/* may be 0 if optional bitmap section missing */
    gds *gdsp = gp->gdsp;	/* may be 0 if optional GDS section missing */
    bds *bdsp = gp->bdsp;	/* binary data section */

    if(out == 0)
	return -1;

    /*
     * These will be allocated later, but zero them now to make freeing easier
     * if something goes wrong 
     */
    out->gd = 0;
    out->bm = 0;
    out->bd = 0;
    out->data = 0;
    out->ensemble = 0;
    
    out->delim[0] = 'G' ;	/* signature for decoded binary GRIB */
    out->delim[1] = 'R' ; 
    out->delim[2] = 'I' ; 
    out->delim[3] = 'B' ; 

    out->header = (char *)emalloc(strlen(gp->hdr)+1);
    strcpy(out->header, gp->hdr);

    out->edition = g1i(idsp->edition) ;
    out->center = g1i(pdsp->center) ;
    out->subcenter = g1i(pdsp->subcenter) ;
    out->model = g1i(pdsp->model) ;
    out->grid = g1i(pdsp->grid) ;
    out->param_vers = g1i(pdsp->table_version) ;
    out->der_flg = 0;
    out->pctl_flg = -1;

    if (out->edition == 0)	/* Fix this before 2087, please */
	out->century = ( g1i(pdsp->year) >= 87 || g1i(pdsp->year) == 0)
	    ? 20 : 21;
    else
	out->century = g1i(pdsp->century) ;	/* Note: =20 for 1901-2000 */

    out->year = g1i(pdsp->year) ;
    out->month = g1i(pdsp->month) ;
    out->day = g1i(pdsp->day) ;
    out->hour = g1i(pdsp->hour) ;
    out->minute = g1i(pdsp->minute) ;
    out->tunit = g1i(pdsp->tunit) ;
    out->tr[0] = g1i(pdsp->tr[0]) ;
    out->tr[1] = g1i(pdsp->tr[1]) ;
    out->tr_flg = g1i(pdsp->tr_flg) ;
    out->avg = g2i(pdsp->avg) ;
    out->missing = g1i(pdsp->missing) ;
    if (out->edition == 0)
        out->scale10 = 0 ;      /* not in edition 0, so manufacture it */
    else
        out->scale10 = g2si(pdsp->scale10) ;

    /*
     * Handle grids from specific centers here. Some need to be mapped into
     * the internal parameter and level name space.
     */
    if (out->center == CENTER_ECMWF){
      out->param = param_code_e(out->edition, out->param_vers, g1i(pdsp->param)) ;
      modify_ecmwf_pdata(out, pdsp);
    } else if (out->center == CENTER_BOM) {
      out->param = param_code_bom(out->param_vers, g1i(pdsp->param)) ;
      modify_bom_pdata(out->param_vers, g1i(pdsp->param), pdsp, out) ;
    } else if (out->center == CENTER_UKMET) {
      out->param = param_code_ukmet(g1i(pdsp->param)) ;
      out->level_flg = g1i(pdsp->level_flg) ;
      out->level[0] = g1i(pdsp->level.levs[0]) ;
      out->level[1] = g1i(pdsp->level.levs[1]) ;

      // Fix odd time units
      if (out->tunit == 255) {
	out->tunit = TUNITS_HOUR;
	out->tr[0] = out->tr[0] - 256;
	if (out->minute == 255)
	  out->minute = 0 ;
      }
    } else {
      out->param = param_code(out->edition, g1i(pdsp->param)) ;
      out->level_flg = g1i(pdsp->level_flg) ;
      out->level[0] = g1i(pdsp->level.levs[0]) ;
      out->level[1] = g1i(pdsp->level.levs[1]) ;
    }

    // Check for unknown parameter
    if (out->param == -1) {
      logFile->write_time(1, "Info: Unknown GRIB-1 parameter: %d\n", g1i(pdsp->param));
      return -1;
    }

    // Handle ensemble grid info
    if (g1i(pdsp->reserved2[0]) > 0) {
      out->ensemble = mkens_from_grib(out->center, pdsp->reserved2);
    }

    out->bits = g1i(bdsp->bits) ;
    out->has_gds = ((pdsp->db_flg & HAS_GDS) != 0) ;
    out->has_bms = ((pdsp->db_flg & HAS_BMS) != 0) ;

    out->gd = make_gdes(gp);
    if(!out->gd) {
	if(out->has_gds)
	    logFile->write_time("Error: GRIB %s has bad GDS, skipping\n",
		   out->header);
	else
	    logFile->write_time("Error: %s: can't make a GDS for center=%d ,grid=%d\n",
		   out->header, out->center, out->grid);
	return -2;
    }

    out->cols = out->gd->ncols;
    out->npts = out->gd->npts;

    out->bm = make_gbytem(bmsp, pdsp, gdsp, out->npts);
    if(!out->bm) {
	logFile->write_time("Error: in GRIB %s, can't make byte map structure, skipping\n",
	       out->header);
	return -3;
    }
    out->bd = make_gbds(bdsp);
    if(!out->bd) {
	logFile->write_time("Error: in GRIB %s, can't make binary data structure, skipping\n",
	       out->header);
	return -4;
    }
    if (unpack) {
      out->data = unpackbds(out->bd, out->bm, out->npts, out->scale10);
      if(out->data == 0) {
	logFile->write_time("Error: in GRIB %s, can't unpack binary data, skipping\n",
			    out->header);
	return -5;
      }
    }

    return 0;
}

/*
 * Similar to the grib1 function, but reads data out of the grib2 struct
 * "gribfield", which contains the grid definition template. The grib2
 * structure has more information, so we pull out only what we need. The
 * cnvgrib program from NCEP was useful in determining what was needed and
 * from where.
 *
 * Returns 0 if successful.
 */
int
make_grib2_pdata(
		 char *id,
		 GRIB2::gribfield *g2fld,
		 product_data *out)
{

  if(out == 0)
    return -1;

  /* These will be allocated later, but zero them now to make freeing easier if
     something goes wrong */
  
  out->header = 0;
  out->gd = 0;
  out->bm = 0;
  out->bd = 0;
  out->data = 0;
  out->ensemble = 0;
  out->der_flg = 0;
  out->pctl_flg = -1;

  out->delim[0] = 'G' ;	/* signature for decoded binary GRIB */
  out->delim[1] = 'R' ; 
  out->delim[2] = 'I' ; 
  out->delim[3] = 'B' ; 

  out->header = (char *)emalloc(strlen(id)+1);
  strcpy(out->header, id);

  out->edition = g2fld->version ;
  out->center = g2fld->idsect[0] ;
  out->subcenter = g2fld->idsect[1] ;
  out->model = g2fld->ipdtmpl[4];
  out->grid = 255;

  // Split 4-digit year into century and year of century
  out->century = (g2fld->idsect[5]/100)+1;
  out->year = g2fld->idsect[5] - (g2fld->idsect[5]/100)*100;
  out->month = g2fld->idsect[6] ;
  out->day = g2fld->idsect[7] ;
  out->hour = g2fld->idsect[8] ;
  out->minute = g2fld->idsect[9] ;

  // Handle ensemble grid info
  if (g2fld->idsect[12] >= 3 && g2fld->idsect[12] <=5) {
    out->ensemble = mkens_from_grib2(g2fld->ipdtnum, g2fld->ipdtmpl);
  }

  // Handle derived forecasts
  if ((g2fld->ipdtnum > 1 && g2fld->ipdtnum < 5) || (g2fld->ipdtnum > 11 && g2fld->ipdtnum < 15))
    out->der_flg = g2fld->ipdtmpl[15];

  // Handle probability forecasts
  if (g2fld->ipdtnum == 5 || g2fld->ipdtnum == 9)
    {
      logFile->write_time(1, "Info: PDT Number (%ld) indicates a probability forecast, currently unsupported\n", g2fld->ipdtnum);
      return -2;
      //printf("PDT Number %ld, prob number %ld, total probs %ld, prob_type %ld, scale factor lower %ld, scale value lower %ld, scale factor upper %ld, scale value upper %ld\n", g2fld->ipdtnum, g2fld->ipdtmpl[15], g2fld->ipdtmpl[16], g2fld->ipdtmpl[17], g2fld->ipdtmpl[18], g2fld->ipdtmpl[19], g2fld->ipdtmpl[20], g2fld->ipdtmpl[21]);
    }

  // Handle percentile forecasts
  if (g2fld->ipdtnum == 6 || g2fld->ipdtnum == 10)
    out->pctl_flg = g2fld->ipdtmpl[15];

  // Map grib2 parameter numbers to grib1.
  if (param_g21(out->header, g2fld->ipdtnum, g2fld->discipline, g2fld->ipdtmpl[0],
		g2fld->ipdtmpl[1], &out->param_vers, &out->param) != 0) {
    return -5;
  }

  // Map grib2 levels to grib1.
  if (level_g21(out->header, g2fld->ipdtmpl, &out->level_flg, out->level) != 0) {
    return -4;
  }

  // Process the time unit. Grib2 tunit values are the same as grib1 except
  // for seconds (code 13).
  out->tunit = g2fld->ipdtmpl[7];
  if (out->tunit == 13)
    out->tunit = 254;

  out->tr[0] = g2fld->ipdtmpl[8];
  out->tr[1] = 0;

  // Handle fields valid at point in time
  if (g2fld->ipdtnum <= 7) {
    out->missing = 0 ;
    out->tr_flg = 0;
    if (out->tr[0] == 0) out->tr_flg = 1;       // Analysis

    // For models with fcst times > 256, pack time into tr[] and flag it
    if (out->model == 77 || out->model == 80 ||
	out->model == 81 || out->model == 82 ||
	out->model == 96) {
      out->tr[1] = out->tr[0] & 255;
      out->tr[0] = out->tr[0] >> 8;
      out->tr_flg = 10;
    }
  }
  else {
    // Non-continuous times (averages, accumulations, etc): Add the time
    // range to the first time. The position depends on the product template
    // number.
    int offset;
    switch (g2fld->ipdtnum)
      {
      case(8):  offset = 23; break;
      case(9):  offset = 30; break;
      case(10): offset = 24; break;
      case(11): offset = 26; break;
      case(12): offset = 25; break;
      case(13): offset = 39; break;
      case(14): offset = 38; break;
      default:
	{
	  logFile->write_time("Error: Unsupported time type: %d\n", g2fld->ipdtnum);
	  return -2;
	}
      }

    int tinc = g2fld->ipdtmpl[offset+3];

    // convert time increment if time units are different
    if (g2fld->ipdtmpl[offset+2] != out->tunit) 
      {
	utUnit tunit1, tunit2;
	double slope, intercept;
	int ret = -1;
	if (utScan(tunits(out->tunit), &tunit1) == 0
	    && utScan(tunits(g2fld->ipdtmpl[offset+2]), &tunit2) == 0)
	  ret = utConvert(&tunit2, &tunit1, &slope, &intercept);

	if (ret != 0)
	  {
	    logFile->write_time("Error: Could not convert time unit %d to %d\n",
				g2fld->ipdtmpl[offset+2], out->tunit);
	    return -2;
	  }

	tinc = slope * tinc + intercept;
      }
    out->tr[1] = out->tr[0] + tinc;

    switch (g2fld->ipdtmpl[offset])
      {
      case(255): out->tr_flg = 2; break;
      case(0):   out->tr_flg = 3; break;
      case(1):   out->tr_flg = 4; break;
      case(2):   out->tr_flg = 2; break;
      case(3):   out->tr_flg = 2; break;
      case(4):   out->tr_flg = 5; break;
      }
    out->missing = g2fld->ipdtmpl[offset-1];
  }

  // Change accumulation fields for BOM grids to fix valid time offset
  // problem. Their data has a valid time 3 hrs ahead of what it should be.

  if (out->center == CENTER_BOM && out->tr_flg == 4)
    {
      logFile->write_time(1, "Info: Changing accumulation end time %d to %d for parameter %d\n", out->tr[1], out->tr[1]-3, out->param);
      out->tr[1] = out->tr[1] - 3;
    }

  out->avg = 0;
  out->scale10 = g2fld->idrtmpl[2];
  out->bits = g2fld->idrtnum;  // Use this as the data representation code
  out->has_gds = 1;            // Always has a grid def template

  // Pass on bit map info.
  if (g2fld->ibmap == 0 || g2fld->ibmap == 254)
    out->has_bms = 1;
  else
    out->has_bms = 0;

  // Make a gdes out of the GRIB-2 grid definition template
  out->gd = gdt_to_gdes(g2fld);
  if(!out->gd) {
    logFile->write_time("Error: %s: can't make a GDS for center=%d ,grid=%d\n",
			out->header, out->center, out->grid);
    return -2;
  }

  out->cols = out->gd->ncols;
  out->npts = out->gd->npts;

  if (g2fld->unpacked) {
    // Allocate space for the grid data and copy it over
    int len = sizeof(float) * out->npts;
    out->data = (float *)emalloc(len);
    out->data = (float *)memcpy(out->data, g2fld->fld, len);

    // Set 0 bit-map values to missing
    if (out->has_bms)
      for (int b=0; b<out->npts; b++)
	if (g2fld->bmap[b] == 0)
	  out->data[b] = GDES_FLOAT_MISSING;
 
    // Fix grids with alternating row direction to have rows go in one
    // direction (left->right). (NDFD grids, for example.) Only every other
    // row needs swapping, starting with the second row.
    if (out->gd->scan_mode & 0x10)
      for (int j=1; j<out->gd->nrows; j+=2)
	for (int i=0; i<out->cols; i++)
	  out->data[j*out->cols+i] = g2fld->fld[(j+1)*out->cols-i-1];
  }

  return 0;
}

/* 
 * Modify product data structure for certain ECMWF grids. (GRIB1 only)
 */
void modify_ecmwf_pdata(product_data *out, pds *pdsp)
{
  /* Set initial level-type and level-values */
  out->level_flg = g1i(pdsp->level_flg) ;
  out->level[0] = g1i(pdsp->level.levs[0]) ;
  out->level[1] = g1i(pdsp->level.levs[1]) ;
  
  /* Change the level-type and/or level-value
     for certain ECMWF variables*/ 

  if (out->level_flg == LEVEL_SURFACE){
	
    /* Change level type and value for "2 metre temperature"  */
    if (pdsp->param == 167){
      out->level_flg = LEVEL_FHG;
      out->level[1] = 2;
    }
	
    /* Change level type and value for "Maximum temperature at 2 metres since last 6 hours"  */
    if (pdsp->param == 121){
      out->level_flg = LEVEL_FHG;
      out->level[1] = 2;
    }
	
    /* Change level type and value for "Minimum temperature at 2 metres since last 6 hours"  */
    if (pdsp->param == 122){
      out->level_flg = LEVEL_FHG;
      out->level[1] = 2;
    }
	
    /* Change level type and value for "2 metre dewpoint temperature"  */
    if (pdsp->param == 168){
      out->level_flg = LEVEL_FHG;
      out->level[1] = 2;
    }
	
    /* Change level type and value for "10 metre U wind component"  */
    if (pdsp->param == 165){
      out->level_flg = LEVEL_FHG;
      out->level[1] = 10;
    }
	
    /* Change level type and value for "10 metre V wind component"  */
    if (pdsp->param == 166){
      out->level_flg = LEVEL_FHG;
      out->level[1] = 10;
    }
	
    /* Change level type for "Mean sea level pressure"  */
    if (pdsp->param == 151){
      out->level_flg = LEVEL_MEAN_SEA;
    }
    
    /* Change level type for "Total cloud cover"  */
    if (pdsp->param == 164){
      out->level_flg = LEVEL_ATM;
    }
    
    /* Change level type and value for "200 metre U or V wind component"  */
    if (pdsp->param == 239 || pdsp->param == 240 ){
      out->level_flg = LEVEL_FHG;
      out->level[1] = 200;
    }

    /* Change level type and value for "100 metre U or V wind component"  */
    if (pdsp->param == 246 || pdsp->param == 247 ){
      out->level_flg = LEVEL_FHG;
      out->level[1] = 100;
    }
	
  } // end if level-type is orginally LEVEL_SURFACE  
  
}

/* 
 * Modify product data structure for certain BOM grids. (GRIB1 only)
 */
void modify_bom_pdata(int table, int param, pds *pdsp, product_data *out)
{
  
  /* Change level type and value for 50m winds */
  if (table == 231 && (param == 33 || param == 34))
    {
      out->level_flg = LEVEL_FHG;
      out->level[0] = 0;
      out->level[1] = 50;
    }
  else if (table == 128 && param == 89)
    {
      out->level_flg = LEVEL_CCBL;
    } 
  else if (table == 128 && param == 90)
    {
      out->level_flg = LEVEL_CCTL;
    } 
  else if (table == 228 && param == 215)
    {
      out->level_flg = LEVEL_TOP;
    } 
  else if (table == 228 && param == 216)
    {
      out->level_flg = LEVEL_TOP;
    } 
  else /* default - use level info as-is */
    {
      out->level_flg = g1i(pdsp->level_flg) ;
      out->level[0] = g1i(pdsp->level.levs[0]) ;
      out->level[1] = g1i(pdsp->level.levs[1]) ;
    }

  // Change accumulation fields to fix valid time offset problem.
  // Their data has a valid time 3 hrs ahead of what it should be.

  if (out->tr_flg == 4)
    {
      logFile->write_time(1, "Info: Changing accumulation end time %d to %d for parameter %d\n", out->tr[1], out->tr[1]-3, out->param);
      out->tr[1] = out->tr[1] - 3;
    }

}



/*
 * Create newly-allocated product_data structure from raw grib1 structure.
 * If there is no grid description section, we manufacture one.  We
 * manufacture a bytemap, even if there is no bitmap section.  Returns 0 if
 * something goes wrong. Unpack option allows caller to have data unpacked 
 * or not.
 */
product_data *
new_grib1_pdata(
    grib1 *gp, int unpack)
{
    product_data *out = (product_data *)emalloc(sizeof(product_data));

    if (make_grib1_pdata(gp, unpack, out) != 0) {
	free_product_data(out);
	return 0;
    }

    return out;
}

/*
 * Similar to new_grib1_pdata() except used for grib2 data structure,
 * 'gribfield'. Also takes the WMO ID string for consistency.
 * 
 */
product_data *
new_grib2_pdata(
		char *id,
		GRIB2::gribfield *g2fld)
{
    product_data *out = (product_data *)emalloc(sizeof(product_data));
    if (make_grib2_pdata(id, g2fld, out) != 0) {
      free_product_data(out);
      return 0;
    }

    return out;
}
