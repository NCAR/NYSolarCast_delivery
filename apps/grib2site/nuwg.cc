/*
 *   Copyright 1995, University Corporation for Atmospheric Research.
 *   See ../COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: nuwg.cc,v 1.3 2013/11/22 19:07:49 dicast Exp $ */


/*
 * Permits some customization of which netCDF names are used in netCDF files
 * for gridded data that follow the NUWG convention.  Although a specific
 * default name is used, this interface allows a user to check other names
 * or aliases for NUWG dimension or variable names.  Ultimately this should
 * be table-driven, with the table parsed to initialize the tests done here,
 * so that recompilation is not necessary to change names.
 */


#include <netcdf.h>
#include "log/log.hh"
#include "nuwg.h"

extern Log *logFile;

/*
 * Returns name for specified NUWG component, or -1 if there is no
 * such component.
 */
char *
nuwg_name(
    enum ncpart which)
{
    switch(which) {
    case DIM_FHG:
	return (char *)"fhg";
    case DIM_LEVEL:
	return (char *)"level";
    case DIM_NAV:
	return (char *)"nav";

    case DIM_SIGMA:
	return (char *)"sigma";
    case DIM_HY:
	return (char *)"hyb";
    case DIM_FH:
	return (char *)"fh";
    case DIM_BLS:
	return (char *)"bls";
    case DIM_ISEN:
	return (char *)"isen";
    case DIM_PDG:
	return (char *)"pdg";
    case DIM_FHGH:
	return (char *)"fhgh";
    case DIM_DBS:
	return (char *)"dbs";
    case DIM_FL:
 	return (char *)"fl";

    case DIM_LBLS:
	return (char *)"lbls";
    case DIM_LFHG:
	return (char *)"lfhg";
    case DIM_LFHM:
	return (char *)"lfhm";
    case DIM_LHY:
	return (char *)"lhy";
    case DIM_LISEN:
	return (char *)"lisen";
    case DIM_LISH:
	return (char *)"lish";
    case DIM_LISM:
	return (char *)"lism";
    case DIM_LISO:
	return (char *)"liso";
    case DIM_LPDG:
	return (char *)"lpdg";
    case DIM_LSH:
	return (char *)"lsh";
    case DIM_LS:
	return (char *)"ls";

    case DIM_NGRIDS:
	return (char *)"ngrids";
    case VAR_REFTIME:
	return (char *)"reftime";
    case VAR_VALTIME:
	return (char *)"valtime";
    case VAR_DATETIME:
	return (char *)"datetime";
    case VAR_VALOFFSET:
	return (char *)"valtime_offset";
    case VAR_GRID_TYPE_CODE:
	return (char *)"grid_type_code";
    case VAR_GRID_CENTER:
	return (char *)"grid_center";
    case VAR_GRID_NUMBER:
	return (char *)"grid_number";
    case VAR_RESCOMP:
	return (char *)"ResCompFlag";
    case VAR_MODELID:
	return (char *)"model_id";
    case VAR_DI:
	return (char *)"Di";
    case VAR_DJ:
	return (char *)"Dj";
    case VAR_DX:
	return (char *)"Dx";
    case VAR_DY:
	return (char *)"Dy";
    case VAR_J:
	return (char *)"J";
    case VAR_K:
	return (char *)"K";
    case VAR_LA1:
	return (char *)"La1";
    case VAR_LA2:
	return (char *)"La2";
    case VAR_LAP:
	return (char *)"Lap";
    case VAR_LATIN:
	return (char *)"Latin";
    case VAR_LATIN1:
	return (char *)"Latin1";
    case VAR_LATIN2:
	return (char *)"Latin2";
    case VAR_LO1:
	return (char *)"Lo1";
    case VAR_LO2:
	return (char *)"Lo2";
    case VAR_LOP:
	return (char *)"Lop";
    case VAR_LOV:
	return (char *)"Lov";
    case VAR_M:
	return (char *)"M";
    case VAR_MODE:
	return (char *)"Mode";
    case VAR_N:
	return (char *)"N";
    case VAR_NI:
	return (char *)"Ni";
    case VAR_NJ:
	return (char *)"Nj";
    case VAR_NR:
	return (char *)"Nr";
    case VAR_NX:
	return (char *)"Nx";
    case VAR_NY:
	return (char *)"Ny";
    case VAR_ORIENTATION:
	return (char *)"Orientation";
    case VAR_PROJFLAG:
	return (char *)"ProjFlag";
    case VAR_ROTANGLE:
	return (char *)"RotAngle";
    case VAR_ROTLAT:
	return (char *)"RotLat";
    case VAR_ROTLON:
	return (char *)"RotLon";
    case VAR_SPLAT:
	return (char *)"SpLat";
    case VAR_SPLON:
	return (char *)"SpLon";
    case VAR_STRETCHFACTOR:
	return (char *)"StretchFactor";
    case VAR_STRETCHLAT:
	return (char *)"StretchLat";
    case VAR_STRETCHLON:
	return (char *)"StretchLon";
    case VAR_TYPE:
	return (char *)"Type";
    case VAR_XO:
	return (char *)"Xo";
    case VAR_XP:
	return (char *)"Xp";
    case VAR_YO:
	return (char *)"Yo";
    case VAR_YP:
	return (char *)"Yp";
    default:
	logFile->write_time("Error: nuwg_name() called for bad component: %d\n", which);
	break;
    }
    /* default */
    return 0;
}


/*
 * Returns dimension ID for specified NUWG dimension, or -1 if there is no
 * such dimension.
 */
int
nuwg_getdim(
    int ncid,
    enum ncpart which
	)
{
  int dimid;
  if (nc_inq_dimid(ncid, nuwg_name(which), &dimid) != NC_NOERR)
    return -1;
  else
    return (dimid);
}


/*
 * Returns variable ID for specified NUWG variable, or -1 if there is no
 * such variable.
 */
int
nuwg_getvar(
    int ncid,
    enum ncpart which
	)
{
  int varid;
  if (nc_inq_varid(ncid, nuwg_name(which), &varid) != NC_NOERR)
    return -1;
  else
    return (varid);

}
