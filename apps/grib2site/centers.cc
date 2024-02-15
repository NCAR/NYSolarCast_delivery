/*
 *   Copyright 1996 University Corporation for Atmospheric Research
 *   See ../COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: centers.cc,v 1.3 2012/05/22 20:13:50 cowie Exp $ */

#include "centers.h"

char *
centername(int center)
{
    switch(center){
    case CENTER_WMO:	
	return (char *)"WMO Secretariat";
    case CENTER_BOM:	
	return (char *)"Australian Bureau of Meteorology - Melbourne";
    case CENTER_NMC:	
	return (char *)"US Weather Service - National Met. Center";
    case CENTER_NWSTG:
	return (char *)"US Weather Service - NWS Telecomms Gateway";
    case CENTER_NWSFS:
	return (char *)"US Weather Service - Field Stations";
    case CENTER_JMA:
	return (char *)"Japanese Meteorological Agency - Tokyo";
    case CENTER_NHC:
	return (char *)"US National Hurricane Center, Miami";
    case CENTER_CMS:
	return (char *)"Canadian Meteorological Service - Montreal";
    case CENTER_USAF:
	return (char *)"US Air Force - Global Weather Center";
    case CENTER_FNOC:
	return (char *)"US Navy  - Fleet Numerical Oceanography Center";
    case CENTER_FSL:
	return (char *)"NOAA Forecast Systems Lab, Boulder CO";
    case CENTER_NCAR:
	return (char *)"National Center for Atmospheric Research (NCAR), Boulder CO";
    case CENTER_UKMET:
	return (char *)"U.K. Met Office - Bracknell";
    case CENTER_FR:
	return (char *)"French Weather Service - Toulouse";
    case CENTER_ESA:
	return (char *)"European Space Agency (ESA)";
    case CENTER_ECMWF:
	return (char *)"European Center for Medium-Range Weather Forecasts - Reading";
    case CENTER_NL:
	return (char *)"DeBilt, Netherlands";
    }
    /* default */
    return (char *)"unknown";
}


char *
subcentername(int center, int sub)
{
    switch(center){
    case CENTER_NMC:	
	switch(sub) {
	case SUBCENTER_NCEP_REANA:
	    return (char *)"NCEP Re-Analysis Project";
	case SUBCENTER_NCEP_EP:
	    return (char *)"NCEP Ensemble Products";
	case SUBCENTER_NCEP_CO:
	    return (char *)"NCEP Central Operations";
	case SUBCENTER_NCEP_EMC:
	    return (char *)"NCEP Environmental Modelling Center";
	case SUBCENTER_NCEP_HPC:
	    return (char *)"NCEP Hydrometeorological Prediction Center";
	case SUBCENTER_NCEP_MPC:
	    return (char *)"NCEP Marine Prediction Center";
	case SUBCENTER_NCEP_CPC:
	    return (char *)"NCEP Climate Prediction Center";
	case SUBCENTER_NCEP_APC:
	    return (char *)"NCEP Aviation Weather Center";
	case SUBCENTER_NCEP_SPC:
	    return (char *)"NCEP Storm Prediction Center";
	case SUBCENTER_NCEP_TPC:
	    return (char *)"NCEP Tropical Prediction Center";
	default:
	    break;
	}
	break;
    case CENTER_NWSFS:
	switch(sub) {
	case SUBCENTER_NWSFS_ABRFC:
	    return (char *)"ABRFC - Arkansas-Red River RFC, Tulsa OK";
	case SUBCENTER_NWSFS_AKFC:
	    return (char *)"Alaska RFC, Anchorage, AK";
	case SUBCENTER_NWSFS_CBRFC:
	    return (char *)"CBRFC - Colorado Basin RFC, Salt Lake City, UT";
	case SUBCENTER_NWSFS_CNRFC:
	    return (char *)"CNRFC - California-Nevada RFC, Sacramento, CA";
	case SUBCENTER_NWSFS_LMRFC:
	    return (char *)"LMRFC - Lower Mississippi RFC, Slidel, LA";
	case SUBCENTER_NWSFS_MARFC:
	    return (char *)"MARFC - Mid Atlantic RFC, State College, PA";
	case SUBCENTER_NWSFS_MBRFC:
	    return (char *)"MBRFC - Missouri Basin RFC, Kansas City, MO";
	case SUBCENTER_NWSFS_NCRFC:
	    return (char *)"NCRFC - North Central RFC, Minneapolis, MN";
	case SUBCENTER_NWSFS_NERFC:
	    return (char *)"NERFC - Northeast RFC, Hartford, CT";
	case SUBCENTER_NWSFS_NWRFC:
	    return (char *)"NWRFC - Northwest RFC, Portland, OR";
	case SUBCENTER_NWSFS_OHRFC:
	    return (char *)"OHRFC - Ohio Basin RFC, Cincinnati, OH";
	case SUBCENTER_NWSFS_SERFC:
	    return (char *)"SERFC - Southeast RFC, Atlanta, GA";
	case SUBCENTER_NWSFS_WGRFC:
	    return (char *)"WGRFC - West Gulf RFC, Fort Worth, TX";
	case SUBCENTER_NWSFS_OUN:
	    return (char *)"OUN - Norman OK WFO";
	default:
	    break;
	}
	break;
    default:
	break;
    }
    return (char *)"unknown";
}
