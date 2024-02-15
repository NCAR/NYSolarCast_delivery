/*
 *	Copyright 1994, University Corporation for Atmospheric Research.
 *	All copies to include this notice.
 */
/* $Id: models.cc,v 1.3 2012/05/22 20:13:50 cowie Exp $ */

/* GRIB models */

#include <stdio.h>
#include <string.h>
#include "centers.h"
#include "models.h"

char *
modelname(int center, int model)
{
    switch(center) {
    case CENTER_BOM:
	switch(model) {
	case MODEL_ACCESS_G:
	  return (char *)"ACCESS-G Global 80km";
	case MODEL_ACCESS_T:
	  return (char *)"ACCESS-T Tropical 37.5km";
	case MODEL_ACCESS_R:
	  return (char *)"ACCESS-R Regional 37.5km";
	case MODEL_ACCESS_A:
	  return (char *)"ACCESS-A Australia 12km";
	case MODEL_ACCESS_VT:
	  return (char *)"ACCESS-VT VICTAS 5km";
	case MODEL_ACCESS_SY:
	  return (char *)"ACCESS-SY Sydney 5km";
	case MODEL_ACCESS_BN:
	  return (char *)"ACCESS-BN Brisbane 5km";
	case MODEL_ACCESS_AD:
	  return (char *)"ACCESS-AD Adelaide 5km";
	case MODEL_ACCESS_PH:
	  return (char *)"ACCESS-PH Perth 5km";
	case MODEL_ACCESS_TC:
	  return (char *)"ACCESS-TC Tropical Cyclone 12km";
	case MODEL_ACCESS_O3:
	  return (char *)"ACCESS-O3 Global Ozone 80km";
	default: {
	    static char bom_model[25];
	    sprintf(bom_model, "unknown BOM model %d", model);
	    return bom_model;
	    }
	}
    case CENTER_ECMWF:
	switch(model) {		/* does anyone know names for these? */
	case MODEL_T213:
	  return (char *)"t213 31-level forecast model";
	default: {
	    static char ecmwf_model[] = "ECMWF model xxxxxxxxxxxx";
	    
	    sprintf(ecmwf_model, "ECMWF model %d", model);
	    return ecmwf_model;
	    }
	}
    case CENTER_NMC:
	switch(model){
	case MODEL_UVPI:
	    return (char *)"Ultra Violet Potential Index Model";
	case MODEL_SAT:
	    return (char *)"Satellite Derived Precipitation and temperatures, from IR";
	case MODEL_SWF:
	    return (char *)"Global Wind-Wave Forecast Model";
	case MODEL_LFM:
	    return (char *)"Limited-area Fine Mesh (LFM)";
	case MODEL_SCA:
	    return (char *)"Snow Cover Analysis";
	case MODEL_NGM:
	    return (char *)"Nested Grid forecast Model (NGM)";
	case MODEL_GOI:
	    return (char *)"Global Optimum Interpolation Analysis (GOI)";
	case MODEL_FGOI:
	    return (char *)"Final Global Optimum Interpolation Analysis (GOI)";
	case MODEL_SST:
	    return (char *)"Sea Surface Temperature Analysis";
	case MODEL_COC:
	    return (char *)"Coastal Ocean Circulation Model";
	case MODEL_LFM4:
	    return (char *)"LFM-Fourth Order Forecast Model";
	case MODEL_ROI:
	    return (char *)"Regional Optimum Interpolation Analysis";
	case MODEL_AVN:
	    return (char *)"80 Wave, 18 Layer Spectral Model Aviation Run";
	case MODEL_MRF:
	    return (char *)"80 Wave, 18 Layer Spectral Model Medium Range Forecast";
	case MODEL_QLM:
	    return (char *)"Quasi-Lagrangian Hurricane Model (QLM)";
	case MODEL_FOG:
	    return (char *)"Fog Forecast model - Ocean Prod. Center";
	case MODEL_GMW:
	    return (char *)"Gulf of Mexico Wind/Wave";
	case MODEL_GAW:
	    return (char *)"Gulf of Alaska Wind/Wave";
	case MODEL_MRFB:
	    return (char *)"Bias Corrected Medium Range Forecast";
	case MODEL_AVN1:
	    return (char *)"126 Wave, 28 Layer Spectral Model Aviation Run";
	case MODEL_MRF1:
	    return (char *)"126 Wave, 28 Layer Spectral Model Medium Range Forecast";
	case MODEL_BCK:
	    return (char *)"Backup from Previous Run of AVN";
	case MODEL_T62:
	    return (char *)"62 wave triangular, 18 layer Spectral Model";
	case MODEL_ASSI:
	    return (char *)"Aviation Spectral Statistical Interpolation";
	case MODEL_FSSI:
	    return (char *)"Final Spectral Statistical Interpolation";
	case MODEL_ETA:
	    return (char *)"ETA mesoscale forecast model 80km res";
	case MODEL_ETA40:
	    return (char *)"ETA Model - 40 km version";
	case MODEL_ETA30:
	    return (char *)"ETA Model - 30 km version";
	case MODEL_RUC:
	    return (char *)"MAPS Model, FSL (Isentropic; 60km at 40N)";
	case MODEL_ENSMB:
	    return (char *)"CAC Ensemble Forecasts from Spectral (ENSMB)";
	case MODEL_PWAV:
	    return (char *)"Ocean Wave model with additional physics (PWAV)";
	case MODEL_ETA48:
	    return (char *)"ETA Model - 48 km version";
	case MODEL_NWSRFS:
	    return (char *)"NWS River Forecast System (NWSRFS)";
	case MODEL_NWSFFGS:
	    return (char *)"NWS Flash Flood Guidance System (NWSFFGS)";
	case MODEL_W2PA:
	    return (char *)"WSR-88D Stage II Precipitation Analysis";
	case MODEL_W3PA:
	    return (char *)"WSR-88D Stage III Precipitation Analysis";
	case MODEL_T213:
	    return (char *)"T213 31-level Forecast Model";
	default:
	    return (char *)"unknown NMC model";
	}
    default:
	return (char *)"model for unknown center";
    }
}
