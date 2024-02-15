/*
 *   Copyright 1996 University Corporation for Atmospheric Research
 *   See ../COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: timeunits.cc,v 1.2 2010/02/04 05:24:54 dicast Exp $ */

#include <stdlib.h>
#include "log/log.hh"
#include "timeunits.h"
#include "product_data.h"

extern Log *logFile;

/*
 * Name used in printed dumps
 */
char *
tunitsname(
    int ii)
{
    switch (ii) {
      case TUNITS_MIN : return (char *)"Minute";
      case TUNITS_HOUR : return (char *)"Hour";
      case TUNITS_DAY : return (char *)"Day";
      case TUNITS_MONTH : return (char *)"Month";
      case TUNITS_YEAR : return (char *)"Year";
      case TUNITS_DECADE : return (char *)"Decade";
      case TUNITS_NORMAL : return (char *)"Normal (30 Years)";
      case TUNITS_CENTURY : return (char *)"Century";
      case TUNITS_3HR : return (char *)"3 hours";
      case TUNITS_6HR : return (char *)"6 hours";
      case TUNITS_12HR : return (char *)"12 hours";
      case TUNITS_SECOND : return (char *)"Second";
    }
    /* default */
    return (char *)"Unknown" ;
}


/*
 * Name used in units conversions with udunits library
 */
char *
tunits(
    int ii)
{
    switch (ii) {
      case TUNITS_MIN : return (char *)"minute";
      case TUNITS_HOUR : return (char *)"hour";
      case TUNITS_DAY : return (char *)"day";
      case TUNITS_MONTH : return (char *)"year/12"; /* ??? */
      case TUNITS_YEAR : return (char *)"year";
      case TUNITS_DECADE : return (char *)"10 year";
      case TUNITS_NORMAL : return (char *)"30 year";
      case TUNITS_CENTURY : return (char *)"100 year";
      case TUNITS_3HR : return (char *)"3 hour";
      case TUNITS_6HR : return (char *)"6 hour";
      case TUNITS_12HR : return (char *)"12 hour";
      case TUNITS_SECOND : return (char *)"second";
    }
    /* default */
    return (char *)"Unknown" ;
}


char *
triname(			/* Return time range indicator */
    int ii)
{
    switch (ii) {
      case TRI_P1: return	(char *)"Reference Time + P1" ;
      case TRI_IAP: return	(char *)"Initialized Analysis Product (P1=0)" ;
      case TRI_P12: return	(char *)"Valid from P1 to P2" ;
      case TRI_Ave: return	(char *)"Average from P1 to P2" ;
      case TRI_Acc: return	(char *)"Accumulation from P1 to P2" ;
      case TRI_Dif: return	(char *)"Difference from P2 to P1" ;
      case TRI_LP1: return	(char *)"Reference Time + Long P1" ;
      case TRI_AvgN: return	(char *)"Special average Algorithm 0" ;
      case TRI_AccN: return	(char *)"Special accumulation Algorithm 0" ;
      case TRI_AvgN1: return	(char *)"Special average Algorithm 1" ;
      case TRI_AccN1: return	(char *)"Special accumulation Algorithm 1" ;
      case TRI_AvgN2: return	(char *)"Special average Algorithm 2" ;
      case TRI_VarN: return 	(char *)"Temporal (co)variance";
      case TRI_SdN: return 	(char *)"Standard deviation";
      case TRI_AvgN3: return	(char *)"Special average Algorithm 3" ;
      case TRI_AccN3: return	(char *)"Special accumulation Algorithm 3" ;
    }
    /* default */
    return (char *)"Unknown" ;
}


char *
trisuffix(			/* Time range indicator suffix */
    int ii)
{
    switch (ii) {
      case TRI_P1: return	(char *)"" ;
      case TRI_IAP: return	(char *)"init_times" ;
      case TRI_P12: return	(char *)"valid_times" ;
      case TRI_Ave: return	(char *)"average_times" ;
      case TRI_Acc: return	(char *)"accum_times" ;
      case TRI_Dif: return	(char *)"diff_times" ;
      case TRI_LP1: return	(char *)"" ;
      case TRI_AvgN: return	(char *)"average0" ;
      case TRI_AccN: return	(char *)"accum0" ;
      case TRI_AvgN1: return	(char *)"average1" ;
      case TRI_AccN1: return	(char *)"accum1" ;
      case TRI_AvgN2: return	(char *)"average2" ;
      case TRI_VarN: return 	(char *)"var";
      case TRI_SdN: return 	(char *)"stdev";
      case TRI_AvgN3: return	(char *)"average3" ;
      case TRI_AccN3: return	(char *)"accum3" ;
    }
    /* default */
    return (char *)"Unknown" ;
}


int
trinum(			/* Number of time range indicators */
    int ii)
{
    switch (ii) {
      case TRI_P1:
      case TRI_IAP:
      case TRI_LP1:
	  return 1;

      case TRI_P12:
      case TRI_Ave:
      case TRI_Acc:
      case TRI_Dif:
      case TRI_AvgN:
      case TRI_AccN:
      case TRI_AvgN1:
      case TRI_AccN1:
      case TRI_AvgN2:
      case TRI_VarN:
      case TRI_SdN:
      case TRI_AvgN3:
      case TRI_AccN3:
	  return 2;
    }
    /* default */
    return 0 ;
}


/*
 * Return product valid time increment from product reference time.  This is
 * not well-defined for some accumulations and averages, so for these we
 * just return 0.
 */
int
frcst_time(
    product_data *pp)
{
    switch(pp->tr_flg) {
    case TRI_P1:
    case TRI_IAP:
	return pp->tr[0];
    case TRI_LP1:
	return 256 * pp->tr[0] + pp->tr[1];

    case TRI_P12:
	return pp->tr[1];
    case TRI_Ave:
	return pp->tr[1];
    case TRI_Acc:
    case TRI_Dif:
	return pp->tr[1];
    case TRI_AvgN:
    case TRI_AccN:
    case TRI_AvgN1:
    case TRI_AccN1:
    case TRI_AvgN2:
    case TRI_VarN:
    case TRI_SdN:	
    case TRI_AvgN3:
    case TRI_AccN3:
	return 0;
    }
				/*     default: */
    logFile->write_time("Error: unknown time range flag %d\n", pp->tr_flg);
    return 0;
}
