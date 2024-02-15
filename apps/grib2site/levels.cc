/*
 *   Copyright 1996 University Corporation for Atmospheric Research
 *   See ../COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: levels.cc,v 1.8 2017/06/22 19:50:54 dicast Exp $ */

#include <stdlib.h>
#include <stdint.h>
#include "log/log.hh"
#include "levels.h"
#include <math.h>


#define float_near(x,y)	((float)((y) + 0.1*fabs((x)-(y))) == (float)(y)) /* true if x is "close to" y */

extern Log *logFile;

/*
 * Atmospheric level in mb from two 8-bit integers in GRIB product
 */
double
mblev(
     int* levels)
{
    return 256.*levels[0] + levels[1];
}

/*
 * Search in table for specified level.  Returns index
 * of value in table, or -1 if the value was not found.
 */
long
level_index(
     double level,	/* level sought */
     float* levels,	/* table of levels */
     long nlevels	/* number of levels in table */
	)
{
    int nn = 0;

    while (nlevels--) {
	float ll;

	ll = *levels++;
	if (float_near(level, ll)) /* true if level "is close to" ll */
	    break;
	nn++;
    }
    return nlevels==-1 ? -1 : nn;
}


/*
 * Search in table for specified layer.  Returns index
 * of value in table, or -1 if the value was not found.
 */
long
layer_index(
     double top,		/* top of layer sought */
     double bot,		/* bottom of layer sought */
     float* tops,		/* table of layer tops */
     float* bots,		/* table of layer bottoms */
     long nlayers		/* number of layers in table */
	)
{
    int nn = 0;

    while (nlayers--) {
	if (float_near(top, *tops) && float_near(bot, *bots))
	    break;
	tops++;
	bots++;
	nn++;
    }
    return nlayers==-1 ? -1 : nn;
}


/*
 * Return name for GRIB level, given GRIB level code.
 */
char *
levelname(
     int ii)
{
    switch(ii){
    case LEVEL_SURFACE: 
	return (char *)"Surface";
    case LEVEL_CLOUD_BASE: 
	return (char *)"Cloud Base";
    case LEVEL_CLOUD_TOP: 
	return (char *)"Cloud Top";
    case LEVEL_ISOTHERM: 
	return (char *)"0 Isotherm";
    case LEVEL_ADIABAT: 
	return (char *)"Adiabatic Condensation";
    case LEVEL_MAX_WIND: 
	return (char *)"Maximum Wind";
    case LEVEL_TROP: 
	return (char *)"Tropopause";
    case LEVEL_TOP:
	return (char *)"Top of Atmosphere";
    case LEVEL_SEABOT:
	return (char *)"Sea Bottom";
    case LEVEL_TMPL:
	return (char *)"Temperature in 1/100 K";
    case LEVEL_ISOBARIC: 
	return (char *)"Isobaric";
    case LEVEL_LISO: 
	return (char *)"Layer Between Two Isobaric";
    case LEVEL_MEAN_SEA: 
	return (char *)"Mean Sea";
    case LEVEL_FH: 
	return (char *)"Fixed Height";
    case LEVEL_LFHM: 
	return (char *)"Layer Between Two Heights Above MSL";
    case LEVEL_FHG: 
	return (char *)"Fixed Height Above Ground";
    case LEVEL_LFHG: 
	return (char *)"Layer Between Two Fixed Heights Above Ground";
    case LEVEL_SIGMA: 
	return (char *)"Sigma";
    case LEVEL_LS: 
	return (char *)"Layer Between Two Sigma";
    case LEVEL_HY:
	return (char *)"Hybrid level";
    case LEVEL_LHY:
	return (char *)"Layer between 2 hybrid levels";
    case LEVEL_Bls: 
	return (char *)"Below Land Surface";
    case LEVEL_LBls: 
	return (char *)"Layer Between Two Depths Below Land Surface";
    case LEVEL_ISEN:
	return (char *)"Isentropic (theta) level";
    case LEVEL_LISEN:
	return (char *)"Layer between 2 isentropic (theta) levels";
    case LEVEL_PDG:
	return (char *)"level at specified pressure difference from ground to level";
    case LEVEL_LPDG:
	return (char *)"layer between 2 levels at specified pressure differences from ground to levels";
    case LEVEL_PV:
	return (char *)"potential vorticity";
    case LEVEL_ETAL:
	return (char *)"ETA level";
    case LEVEL_LETA: 
	return (char *)"Layer between two ETA levels";
    case LEVEL_LISH: 
	return (char *)"Layer Between Two Isobaric Surfaces, High Precision";
    case LEVEL_FHGH:
	return (char *)"Height level above ground (high precision)";
    case LEVEL_LSH: 
	return (char *)"Layer Between Two Sigma Levels, High Precision";
    case LEVEL_LISM: 
	return (char *)"Layer Between Two Isobaric Surfaces, Mixed Precision";
    case LEVEL_DBS: 
	return (char *)"Depth Below Sea";
    case LEVEL_ATM:
	return (char *)"Entire atmosphere considered as a single layer";
    case LEVEL_OCEAN:
	return (char *)"Entire ocean considered as a single layer";
    case LEVEL_HTFL:
	return (char *)"Highest tropospheric freezing level";
    case LEVEL_BCY:
	return (char *)"Boundary layer cloud layer";
    case LEVEL_LCBL:
	return (char *)"Low cloud bottom level";
    case LEVEL_LCTL:
	return (char *)"Low cloud top level";
    case LEVEL_LCY:
	return (char *)"Low cloud layer";
    case LEVEL_CEILING:
	return (char *)"Cloud ceiling";
    case LEVEL_MCBL:
	return (char *)"Middle cloud bottom level";
    case LEVEL_MCTL:
	return (char *)"Middle cloud top level";
    case LEVEL_MCY:
	return (char *)"Middle cloud layer";
    case LEVEL_HCBL:
	return (char *)"High cloud bottom level";
    case LEVEL_HCTL:
	return (char *)"High cloud top level";
    case LEVEL_HCY:
	return (char *)"Highcloud layer";
    case LEVEL_CCBL:
	return (char *)"Convective cloud bottom level";
    case LEVEL_CCTL:
	return (char *)"Convective cloud top level";
    case LEVEL_CCY:
	return (char *)"Convective cloud layer";
    case LEVEL_FL:
 	return (char *)"flight_level";
    }
    /* default */
    logFile->write_time(1, "Error: unknown level: %d\n", ii);
    return (char *)"reserved or unknown";
}


/*
 * Returns level suffix, used in netCDF names for variables on special
 * levels and in (char *)"gribdump -b" level abbreviations.
 */
char *
levelsuffix(
    int lev)
{
				/* Note: If any suffixes are added or
				   changed, they must be added or changed in
				   the function grib_pcode() as well. */
    switch(lev) {
    case LEVEL_SURFACE: return (char *)"sfc"; /* surface of the Earth */
    case LEVEL_CLOUD_BASE: return (char *)"clbs"; /* cloud base level */
    case LEVEL_CLOUD_TOP: return (char *)"cltp"; /* cloud top level */
    case LEVEL_ISOTHERM: return (char *)"frzlvl"; /* 0 degree isotherm level */
    case LEVEL_ADIABAT: return (char *)"adcn"; /* adiabatic condensation level */
    case LEVEL_MAX_WIND: return (char *)"maxwind"; /* maximium wind speed level */
    case LEVEL_TROP: return (char *)"trop"; /* at the tropopause */
    case LEVEL_TOP: return (char *)"topa"; /* nominal top of atmosphere */
    case LEVEL_SEABOT: return (char *)"sbot"; /* sea bottom */
    case LEVEL_TMPL: return (char *)"tmpl"; /* temperature in 1/100 K */
    case LEVEL_ISOBARIC: return (char *)""; /* isobaric level */
    case LEVEL_LISO: return (char *)"liso"; /* layer between two isobaric levels */
    case LEVEL_MEAN_SEA: return (char *)"msl"; /* mean sea level */
    case LEVEL_FH: return (char *)"fh";	/* fixed height level */
    case LEVEL_LFHM: return (char *)"lfhm"; /* layer between 2 height levels above MSL */
    case LEVEL_FHG: return (char *)"fhg"; /* fixed height above ground */
    case LEVEL_LFHG: return (char *)"lfhg"; /* layer between 2 height levels above ground */
    case LEVEL_SIGMA: return (char *)"sigma"; /* sigma level */
    case LEVEL_LS: return (char *)"ls";	/* layer between 2 sigma levels */
    case LEVEL_HY: return (char *)"hybr"; /* Hybrid level */
    case LEVEL_LHY: return (char *)"lhyb"; /* Layer between 2 hybrid levels */
    case LEVEL_Bls: return (char *)"bls"; /* Depth below land surface */
    case LEVEL_LBls: return (char *)"lbls"; /* Layer between 2 depths below land surface */
    case LEVEL_ISEN: return (char *)"isen"; /* Isentropic (theta) level */
    case LEVEL_LISEN: return (char *)"lisn"; /* Layer between 2 isentropic (theta) levels */
    case LEVEL_PDG: return (char *)"pdg"; /* level at specified pressure difference from ground */
    case LEVEL_LPDG: return (char *)"lpdg"; /* layer between levels at specif. pressure diffs from ground */
    case LEVEL_PV: return (char *)"pv"; /* level of specified potential vorticity */
    case LEVEL_ETAL: return (char *)"etal"; /* ETA level */
    case LEVEL_LETA: return (char *)"leta";	/* layer between 2 ETA levels */
    case LEVEL_LISH: return (char *)"lish"; /* layer between 2 isobaric surfaces (high precision) */
    case LEVEL_FHGH: return (char *)"fhgh"; /* height level above ground (high precision) */
    case LEVEL_LSH: return (char *)"lsh"; /* layer between 2 sigma levels (high precision) */
    case LEVEL_LISM: return (char *)"lism"; /* layer between 2 isobaric surfaces (mixed precision) */
    case LEVEL_DBS: return (char *)"dbs"; /* depth below sea level */
    case LEVEL_ATM: return (char *)"atm"; /* entire atmosphere considered as a single layer */
    case LEVEL_OCEAN: return (char *)"ocn"; /* entire ocean considered as a single layer */
    case LEVEL_HTFL: return (char *)"htfl"; /* Highest tropospheric freezing level */
    case LEVEL_BCY: return (char *)"bcy"; /* Boundary layer cloud layer */
    case LEVEL_LCBL: return (char *)"lcbl"; /* Low cloud bottom level */
    case LEVEL_LCTL: return (char *)"lctl"; /* Low cloud top level */
    case LEVEL_LCY: return (char *)"lcy"; /* Low cloud layer */
    case LEVEL_CEILING: return (char *)"ceil"; /* Cloud ceiling */
    case LEVEL_MCBL: return (char *)"mcbl"; /* Middle cloud bottom level */
    case LEVEL_MCTL: return (char *)"mctl"; /* Middle cloud top level */
    case LEVEL_MCY: return (char *)"mcy"; /* Middle cloud layer */
    case LEVEL_HCBL: return (char *)"hcbl"; /* High cloud bottom level */
    case LEVEL_HCTL: return (char *)"hctl"; /* High cloud top level */
    case LEVEL_HCY: return (char *)"hcy"; /* Highcloud layer */
    case LEVEL_CCBL: return (char *)"ccbl"; /* Convective cloud bottom level */
    case LEVEL_CCTL: return (char *)"cctl"; /* Convective cloud top level */
    case LEVEL_CCY: return (char *)"ccy"; /* Convective cloud layer */
    case LEVEL_FL: return (char *)"fl";	/* FAA flight level */
    }
				/* default: */
    logFile->write_time(1, "Error: bad level flag: %d\n", lev);
    return (char *)"";
}

/*
 * Returns int for first level (if 2 levels) or level (if only one level)
 */
int
level1(
    int flag,			/* GRIB level flag */
    int *ii			/* GRIB levels */
	)
{
    switch(flag){
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
    case LEVEL_HTFL:
    case LEVEL_BCY:
    case LEVEL_LCBL:
    case LEVEL_LCTL: 
    case LEVEL_LCY:
    case LEVEL_CEILING:
    case LEVEL_MCBL:
    case LEVEL_MCTL:
    case LEVEL_MCY:
    case LEVEL_HCBL:
    case LEVEL_HCTL:
    case LEVEL_HCY:
    case LEVEL_CCBL:
    case LEVEL_CCTL:
    case LEVEL_CCY:
    case LEVEL_FL:
	return 0;
    case LEVEL_TMPL:
    case LEVEL_ISOBARIC: 
    case LEVEL_FH: 
    case LEVEL_FHG: 
    case LEVEL_SIGMA: 
    case LEVEL_HY:
    case LEVEL_Bls: 
    case LEVEL_ISEN:
    case LEVEL_PDG:
    case LEVEL_PV:
    case LEVEL_ETAL:
    case LEVEL_FHGH:
    case LEVEL_DBS: 
	return 256*ii[0]+ii[1];	/* 2-octet level */
    case LEVEL_LISO: 
    case LEVEL_LFHM: 
    case LEVEL_LFHG: 
    case LEVEL_LS: 
    case LEVEL_LHY:
    case LEVEL_LBls: 
    case LEVEL_LISEN:
    case LEVEL_LPDG:
    case LEVEL_LETA: 
    case LEVEL_LISH: 
    case LEVEL_LSH: 
    case LEVEL_LISM: 
	return ii[0];		/* 1-octet level */
    }
    /* default */
    logFile->write_time(1, "Error: unknown level: %d\n", ii);
    return -1;
}


/*
 * Returns int for second level (if 2 levels) or 0 (if only one level)
 */
int
level2(
    int flag,			/* GRIB level flag */
    int *ii			/* GRIB levels */
	)
{
    switch(flag){
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
    case LEVEL_HTFL:
    case LEVEL_BCY:
    case LEVEL_LCBL:
    case LEVEL_LCTL:
    case LEVEL_LCY:
    case LEVEL_CEILING:
    case LEVEL_MCBL:
    case LEVEL_MCTL:
    case LEVEL_MCY:
    case LEVEL_HCBL:
    case LEVEL_HCTL:
    case LEVEL_HCY:
    case LEVEL_CCBL:
    case LEVEL_CCTL:
    case LEVEL_CCY:
    case LEVEL_FL:
    case LEVEL_TMPL: 
    case LEVEL_ISOBARIC: 
    case LEVEL_FH: 
    case LEVEL_FHG: 
    case LEVEL_SIGMA: 
    case LEVEL_HY:
    case LEVEL_Bls: 
    case LEVEL_ISEN:
    case LEVEL_PDG:
    case LEVEL_PV:
    case LEVEL_ETAL:
    case LEVEL_FHGH:
    case LEVEL_DBS: 
	return 0;
    case LEVEL_LISO: 
    case LEVEL_LFHM: 
    case LEVEL_LFHG: 
    case LEVEL_LS: 
    case LEVEL_LHY:
    case LEVEL_LBls: 
    case LEVEL_LISEN:
    case LEVEL_LPDG:
    case LEVEL_LETA: 
    case LEVEL_LISH: 
    case LEVEL_LSH: 
    case LEVEL_LISM: 
	return ii[1];
    }
    /* default */
    logFile->write_time(1, "Error: unknown level: %d\n", ii);
    return -1;
}


/*
 * Return GRIB units (as a string) for various kinds of levels.
 */
char *
levelunits(int ii)
{
    switch(ii){
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
    case LEVEL_HY:
    case LEVEL_LHY:
    case LEVEL_ATM:
    case LEVEL_OCEAN:
    case LEVEL_HTFL:
    case LEVEL_BCY:
    case LEVEL_LCBL:
    case LEVEL_LCTL:
    case LEVEL_LCY:
    case LEVEL_CEILING:
    case LEVEL_MCBL:
    case LEVEL_MCTL:
    case LEVEL_MCY:
    case LEVEL_HCBL:
    case LEVEL_HCTL:
    case LEVEL_HCY:
    case LEVEL_CCBL:
    case LEVEL_CCTL:
    case LEVEL_CCY:
    case LEVEL_FL:
	return (char *)"" ;
    case LEVEL_ISOBARIC: 
    case LEVEL_PDG:
    case LEVEL_LPDG:
    case LEVEL_LISH: 
	return (char *)"hPa";
    case LEVEL_LISO: 
    case LEVEL_LISM: 
	return (char *)"kPa";
    case LEVEL_FH: 
    case LEVEL_FHG: 
    case LEVEL_DBS: 
	return (char *)"meters";
    case LEVEL_LFHM: 
    case LEVEL_LFHG: 
	return (char *)"hm" ;
    case LEVEL_SIGMA: 
	return (char *)".0001";		/* dimensionless */
    case LEVEL_LS: 
    case LEVEL_LETA: 
	return (char *)".01";		/* dimensionless */
    case LEVEL_Bls: 
    case LEVEL_LBls: 
    case LEVEL_FHGH:
	return (char *)"cm";
    case LEVEL_ISEN:
    case LEVEL_LISEN:
	return (char *)"degK";
    case LEVEL_TMPL:
	return (char *)".01 degK";
    case LEVEL_LSH: 
	return (char *)".001";
    case LEVEL_PV:
	return (char *)".000001 K m2/kg/sec";
    case LEVEL_ETAL:
	return (char *)".0001";
    }
    /* default */
    return (char *)"unknown" ;
}


/*
 * Takes a GRIB2 product definition template array and maps the GRIB2 level
 * information to the GRIB1 equivalent, returning what's needed. This mapping
 * was taken from the module makepds.ffrom the FORTRAN program 'cnvgrib'
 * (v1.4.0) available from NCEP.
 */

int level_g21(char *header, GRIB2::g2int *ipdtmpl, int *level_flg, int *levels)
{
  int ltype1, ltype2;
  float scale1, scale2;

  *level_flg = 255;

  ltype1 = ipdtmpl[9];
  ltype2 = ipdtmpl[12];
  scale1 = powf(10., (float)-ipdtmpl[10]);
  scale2 = powf(10., (float)-ipdtmpl[13]);

  //printf("g-2 ltype1 %d, ltype2 %d, scale1 %d, scale2 %d, l1 %d, l2 %d\n",ltype1, ltype2, ipdtmpl[10], ipdtmpl[13], ipdtmpl[11], ipdtmpl[14]);

  if (ltype1 == 10 && ltype2 == 255) {             // Special case for NCEP HRRR to get total-cloud-cover (Surface to top of Atm). LEVEL_ATM = 200
    *level_flg = LEVEL_ATM;
    levels[0] = 0;
    levels[1] = 0;
  }
  else if (ltype1 < 100 && ltype2 == 255) {        // Special levels
    *level_flg = ltype1;
    levels[0] = 0;
    levels[1] = 0;
  }
  else if (ltype1 == 1 && ltype2 == 8) {           // Surface to top of Atm
    *level_flg = ltype1;
    levels[0] = 0;
    levels[1] = 0;
  }
  else if (ltype1 == 2 && ltype2 == 8) {           // Cld base to top of Atm
    *level_flg = ltype1;
    levels[0] = 0;
    levels[1] = 0;
  }
  else if (ltype1 >= 200 && ltype2 == 255) {        // NCEP special levels
    *level_flg = ltype1;
    levels[0] = 0;
    levels[1] = 0;
  }
  else if (ltype1 == 100 && ltype2 == 255) {       // Isobaric level
    *level_flg = 100;
    levels[0] = 0;
    levels[1] = (int)(ipdtmpl[11]*scale1/100.+.5);
  }
  else if (ltype1 == 100 && ltype2 == 100) {      // Isobaric layer
    *level_flg = 101;
    levels[0] = (int)(ipdtmpl[11]*scale1/1000.+.5);
    levels[1] = (int)(ipdtmpl[14]*scale2/1000.+.5);
  }
  else if (ltype1 == 101 && ltype2 == 255) {      // Mean Sea Level
    *level_flg = 102;
    levels[0] = 0;
    levels[1] = 0;
  }
  else if (ltype1 == 102 && ltype2 == 255) {      // Altitude above MSL
    *level_flg = 103;
    levels[0] = 0;
    levels[1] = (int)(ipdtmpl[11]*scale1+.5);
  }
  else if (ltype1 == 102 && ltype2 == 102) {      // Altitude above MSL layer
    *level_flg = 104;
    levels[0] = (int)(ipdtmpl[11]*scale1/100+.5);
    levels[1] = (int)(ipdtmpl[14]*scale2/100+.5);
  }
  else if (ltype1 == 103 && ltype2 == 255) {      // Height above ground
    *level_flg = 105;
    levels[0] = 0;
    levels[1] = (int)(ipdtmpl[11]*scale1+.5);
  }
  else if (ltype1 == 103 && ltype2 == 103) {      // Height above ground layer
    *level_flg = 106;
    levels[0] = (int)(ipdtmpl[11]*scale1/100.+.5);
    levels[1] = (int)(ipdtmpl[14]*scale2/100.+.5);
  }
  else if (ltype1 == 104 && ltype2 == 255) {      // Sigma level
    *level_flg = 107;
    levels[0] = 0;
    levels[1] = (int)(ipdtmpl[11]*scale1*10000.+.5);
  }
  else if (ltype1 == 104 && ltype2 == 104) {      // Sigma layer
    *level_flg = 108;
    levels[0] = (int)(ipdtmpl[11]*scale1*100.+.5);
    levels[1] = (int)(ipdtmpl[14]*scale2*100.+.5);
  }
  else if (ltype1 == 105 && ltype2 == 255) {      // Hybrid level
    *level_flg = 109;
    levels[0] = 0;
    levels[1] = ipdtmpl[11];
  }
  else if (ltype1 == 105 && ltype2 == 105) {      // Hybrid layer
    *level_flg = 110;
    levels[0] = (int)(ipdtmpl[11]*scale1+.5);
    levels[1] = (int)(ipdtmpl[14]*scale2+.5);
  }
  else if (ltype1 == 106 && ltype2 == 255) {      // Depth below land surface
    *level_flg = 111;
    levels[0] = 0;
    levels[1] = (int)(ipdtmpl[11]*scale1*100.+.5);
  }
  else if ((ltype1 == 1 || ltype1 == 106) &&
	   ltype2 == 106) {                      // Layer depth below surface
    *level_flg = 112;
    levels[0] = (int)(ipdtmpl[11]*scale1*100.+.5);
    levels[1] = (int)(ipdtmpl[14]*scale2*100.+.5);
  }
  else if (ltype1 == 107 && ltype2 == 255) {      // Isentropic level
    *level_flg = 113;
    levels[0] = 0;
    levels[1] = (int)(ipdtmpl[11]*scale1+.5);
  }
  else if (ltype1 == 107 && ltype2 == 107) {      // Isentropic layer
    *level_flg = 114;
    levels[0] = 475-(int)(ipdtmpl[11]*scale1+.5);
    levels[1] = 475-(int)(ipdtmpl[14]*scale2+.5);
  }    
  else if (ltype1 == 108 && ltype2 == 255) {      // Level at specified press
    *level_flg = 115  ;                            // difference from ground to
    levels[0] = 0;
    levels[1] = (int)(ipdtmpl[11]*scale1/100.+.5);
  }
  else if (ltype1 == 108 && ltype2 == 108) {      // Layer at specified press
    *level_flg = 116;                              // difference from ground to
    levels[0] = (int)(ipdtmpl[11]*scale1/100.+.5);
    levels[1] = (int)(ipdtmpl[14]*scale2/100.+.5);
  }  
  else if (ltype1 == 109 && ltype2 == 255) {      // Potential vorticity sfc
    *level_flg = 117;                             // (can be negative)
    levels[0] = 0;
    if (ipdtmpl[11] >= 0)
      levels[1] = (int)(ipdtmpl[11]*scale1*1000000000.+.5);
    else
      levels[1] = (int)(ipdtmpl[11]*scale1*1000000000.-.5);
  }
  else if (ltype1 == 111 && ltype2 == 255) {      // Eta level
    *level_flg = 119;
    levels[0] = 0;
    levels[1] = (int)(ipdtmpl[11]*scale1*10000.+.5);
  }
  else if (ltype1 == 111 && ltype2 == 111) {      // Eta layer
    *level_flg = 120;
    levels[0] = (int)(ipdtmpl[11]*scale1*100.+.5);
    levels[1] = (int)(ipdtmpl[14]*scale2*100.+.5);
  }
  else if (ltype1 == 160 && ltype2 == 255) {      // Depth below sea level
    *level_flg = 160;
    levels[0] = 0;
    levels[1] = (int)(ipdtmpl[11]*scale1+.5);
  }
  else {
    *level_flg = 255;
    levels[0] = 0;
    levels[1] = 0;
    logFile->write_time(1, "Error: GRIB %s: Can't process GRIB2 level type(s): %d, %d, scale(s) %f, %f\n", header, ltype1, ltype2, scale1, scale2);
    return (-1);
  }

  // Byte shift values as needed to conform to grib1 notation.
  if (levels[1] > 255 && levels[0] == 0) {
    levels[0] = levels[1] >> 8;
    levels[1] = levels[1] & 255;
  }

  // The values in the levels array should not be > 255.
  if (levels[0] > 255 || levels[1] > 255) {
    logFile->write_time(1, "Error: GRIB %s: Processing GRIB-2 level type %d, %d\n", header, ltype1, ltype2);
    return (-1);
  }

  return (0);

}
