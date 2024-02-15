/*
 *   Module: cmapf_model.cc
 *
 *   Author: Jason Craig
 *
 *   Date:   04/05/2005
 *
 *   Description: Clas to use NCEP models that have been converted
 *   to NetCDF with cmapf, a map projection conversion library.
 *   NetCDF file must have grid_type_code and appropriate variables
 *   to describe map projection.
 *
 */

#include <string.h>
#include <ctype.h>
#include "../include/dmapf/cmapf_model.hh"

// Constructor loads needed information from NetCDF file
// and sets up dmapf library 
// Certain variables are needed for each grid_type_code:
//  0 = "latitude/longitude",  La1, Lo1, Di, Dj, Ni, Nj
//  3 = "lambert conformal",   La1, Lo1, Lov, Latin1, Latin2, Dx or Dy
//  5 = "polar stereographic", La1, Lo1, Lov, Dx or Dy, (reflat is assumed 60)

CmapfModel::CmapfModel(NcFile *ncf)
{
  gridTypeMap::iterator iter;

  clear();

  // Sets NetCDF file option non-fatal errors and no error messages
  //ncopts = 0;

  // Sets NetCDF file option non-fatal errors
  //ncopts = NC_VERBOSE;

  // gmap defines a mapping between known projection names and the grid type
  // code (0-5) that they are known by in the mpc lib.  This list covers the
  // names seen to date in our model netcdf files. Leave names in lower case.

  gmap["latitude/longitude"] = 0;
  gmap["mercator"] = 1;
  gmap["cartesian"] = 2;
  gmap["lambert conformal"] = 3;
  gmap["cylindrical"] = 4;
  gmap["polar stereographic"] = 5;

  // Get the grid type code. If not there, then get the grid type name
  // and see if it maps to any of the names in gmap above.

  if (getNavVal(ncf, "grid_type_code", &_type) < 0)
  {
    char *gtype;
    long len; 
    if (getNavVal(ncf, "grid_type", &gtype, &len) < 0)
    {
      // otherModelTypes function allows defined models that do not follow
      // standred convention to be read in.  If it fails then we have no idea
      // what kind of model this file is.
      if(otherModelTypes(ncf) < -1) {
	// Format unknown
	_errString = "NetCDF model file is in unknown format. grid_type_code not found.";
	//_errString = "Unable to read grid_type_code or grid_type";
      }
      return;
    }
    
    // Convert to  lower case
    for (int i=0; i<len; i++)
      gtype[i] = (char) tolower((int)gtype[i]);
    
    // Remove trailing spaces
    len--;
    while (! isalpha((int)gtype[len]))
    {
      gtype[len] = '\0';
      len--;
    }

    // Make a string
    string gstr(gtype);
    delete [] gtype;

    // Find the string in the map
    iter = gmap.find(gstr);
    if (iter != gmap.end())
      _type = (*iter).second;
    else
    {
      _errString = "Unknown grid_type name: '" + gstr + "'";
      return;
    }
  }

  // Grid Type 0
  if(_type == gmap["latitude/longitude"])
  {

    if (getNavVal(ncf, "La1", &_la1) < 0)
    {
      _errString = "Unable to read La1";
      return;
    }
    if (getNavVal(ncf, "Lo1", &_lo1) < 0)
    {
      _errString = "Unable to read Lo1";
      return;
    }
    if (getNavVal(ncf, "Di", &_delx) < 0)
    {
      _errString = "Unable to read Di";
      return;
    }
    if (getNavVal(ncf, "Dj", &_dely) < 0)
    {
      _errString = "Unable to read Dj";
      return;
    }
    if (getNavVal(ncf, "Ni", &_nx) < 0)
    {
      _errString = "Unable to read Ni";
      return;
    }
    if (getNavVal(ncf, "Nj", &_ny) < 0)
    {
      _errString = "Unable to read Nj";
      return;
    }
    _iref = 0.0;
    _jref = 0.0;
    // Determine if the grid is global; ie, that it wraps around longitudinally.
    _global = 0;
    if ((_nx * _delx) == 360.)
      _global = 1;

    // Lat Lon grid currently does not use cmapf

  // Grid Type 1
  } else if(_type == gmap["mercator"])
    {
      // Unable to test this grid type so not defined
      _errString = "Unable to process mercator grid";
      return;

  // Grid Type 2
    } else if(_type == gmap["cartesian"])
      {
      // Unable to test this grid type so not defined
	_errString = "Unable to process cartesian grid";
	return;

  // Grid Type 3
      } else if(_type == gmap["lambert conformal"])
	{

	  if (getNavVal(ncf, "La1", &_la1) < 0)
	  {
	    _errString = "Unable to read La1";
	    return;
	  }
	  if (getNavVal(ncf, "Lo1", &_lo1) < 0)
	  {
	    _errString = "Unable to read Lo1";
	    return;
	  }
	  if (getNavVal(ncf, "Lov", &_lov) < 0)
	  {
	    _errString = "Unable to read Lov";
	    return;
	  }
	  if (getNavVal(ncf, "Latin1", &_latin1) < 0)
	  {
	    _errString = "Unable to read Latin1";
	    return;
	  }
	  if (getNavVal(ncf, "Latin2", &_latin2) < 0)
	  {
	    _errString = "Unable to read Latin2";
	    return;
	  }
	  if (getNavVal(ncf, "Dx", &_delx) < 0)
	  {
	    if (getNavVal(ncf, "Dy", &_delx) < 0)
	    {
	      _errString = "Unable to read Dx or Dy";
	      return;
	    }
	  }
	  _delx = _delx / 1000;

	  stlmbr(&_stcpm, eqvlat(_latin1, _latin2), _lov);
	  if (getNavVal(ncf, "Earth_radius", &_erad) == 0)
	    cstrad(&_stcpm, (double)_erad);
	  stcm1p(&_stcpm, 0.0, 0.0, _la1, _lo1, _latin1, _lov, _delx, 0);

  // Grid Type 4
	} else if(_type == gmap["cylindrical"])
	  {
	    // Unable to test this grid type so not defined
	    _errString = "Unable to process cylindrical grid";
	    return;

  // Grid Type 5
	  } else if(_type == gmap["polar stereographic"])
	    {

	      if (getNavVal(ncf, "La1", &_la1) < 0)
	      {
		_errString = "Unable to read La1";
		return;
	      }
	      
	      if (getNavVal(ncf, "Lo1", &_lo1) < 0)
	      {
		_errString = "Unable to read Lo1";
		return;
	      }
	      if (getNavVal(ncf, "Lov", &_lov) < 0)
	      {
		_errString = "Unable to read Lov";
		return;
	      }
	      if (getNavVal(ncf, "Dx", &_delx) < 0)
	      {
		if (getNavVal(ncf, "Dy", &_delx) < 0)
		{
		  _errString = "Unable to read Dx or Dy";
		  return;
		}
	      }
	      _delx = _delx / 1000;
	      sobstr(&_stcpm, 90., 0.);
	      if (getNavVal(ncf, "Earth_radius", &_erad) == 0)
		cstrad(&_stcpm, (double)_erad);
	      stcm1p(&_stcpm, 0.0, 0.0, _la1, _lo1, 60., _lov, _delx, 0);

	    }

}

int CmapfModel::ll2xy(double lat, double longit, double *x, double *y)
{

  if(_type == gmap["latitude/longitude"])
  {
    *x = (longit - _lo1)/_delx + _iref;
    *y = (lat - _la1)/_dely + _jref;
    
    if (_global) {
      while (*x < 0.)
	*x += _nx;
      
      while (*y < 0.)
	*y += _ny;
      
      *x = fmod(*x, _nx);
      *y = fmod(*y, _ny);
    } 
    else if ((rint(*x) < 0) || (rint(*x) >= _nx) || (rint(*y) < 0) || (rint(*y) >= _ny))
      return -1;

  } else {
    cll2xy(&_stcpm, lat, longit, x, y);

    if ((rint(*x) < 0) || (rint(*x) >= _nx) || (rint(*y) < 0) || (rint(*y) >= _ny))
      return -1;
  }
  return 0;
}

int CmapfModel::ll2xy(float lat, float longit, float *x, float *y)
{

  if(_type == gmap["latitude/longitude"])
  {
    *x = (longit - _lo1)/_delx + _iref;
    *y = (lat - _la1)/_dely + _jref;
    
    if (_global) {
      while (*x < 0.)
	*x += _nx;
      
      while (*y < 0.)
	*y += _ny;
      
      *x = fmod(*x, _nx);
      *y = fmod(*y, _ny);
    } 
    else if ((rint(*x) < 0) || (rint(*x) >= _nx) || (rint(*y) < 0) || (rint(*y) >= _ny))
      return -1;

  } else {
    double _x, _y;
    cll2xy(&_stcpm, (double)lat, (double)longit, &_x, &_y);

    *x = (float) _x;
    *y = (float) _y;

    if ((rint(*x) < 0) || (rint(*x) >= _nx) || (rint(*y) < 0) || (rint(*y) >= _ny))
      return -1;
  }
  return 0;
}

int CmapfModel::xy2ll(double x, double y, double *lat, double *longit)
{

  if(_type == gmap["latitude/longitude"])
  {
    if (_global) {
      while (x < 0.)
	x += _nx;
      
      while (y < 0.)
	y += _ny;
      
      x = fmod(x, _nx);
      y = fmod(y, _ny);
    }
    else if ((x < 0) || (x >= _nx) || (y < 0) || (y >= _ny))
      return -1;
    
    *lat = (y - _jref)*_dely + _la1;
    *longit = (x - _iref)*_delx + _lo1;

    if (*longit < 0.0) *longit += 360.0;
    if (*longit > 360.0) *longit -= 360;

    if (*longit > 180.0) *longit -= 360.0;

  } else {
    cxy2ll(&_stcpm, x, y, lat, longit);
  }
  return 0;
}

int CmapfModel::xy2ll(float x, float y, float *lat, float *longit)
{

  if(_type == gmap["latitude/longitude"])
  {
    if (_global) {
      while (x < 0.)
	x += _nx;
      
      while (y < 0.)
	y += _ny;
      
      x = fmod(x, _nx);
      y = fmod(y, _ny);
    }
    else if ((x < 0) || (x >= _nx) || (y < 0) || (y >= _ny))
      return -1;
    
    *lat = (y - _jref)*_dely + _la1;
    *longit = (x - _iref)*_delx + _lo1;

    if (*longit < 0.0) *longit += 360.0;
    if (*longit > 360.0) *longit -= 360;

    if (*longit > 180.0) *longit -= 360.0;

  } else {
    double _lat, _longit;
    cxy2ll(&_stcpm, x, y, &_lat, &_longit);
    
    *lat = (float)_lat;
    *longit = (float)_longit;
  }
  return 0;
}


void CmapfModel::printNav()
{
  cout << "-------------------------------" << endl;
  cout << "Grid map projection information" << endl;
  cout << "type\t" << _type << endl;
  if(_type == gmap["latitude/longitude"])
  {
    cout << "\tlatitude/longitude" << endl;
    cout << "reflat\t" << _la1 << endl;
    cout << "reflon\t" << _lo1 << endl;
    cout << "iref\t0" << endl;
    cout << "jref\t0" << endl;
    cout << "deltaX\t" << _delx << endl;
    cout << "deltaY\t" << _dely << endl;
    cout << "Nx\t" << _nx << endl;
    cout << "Ny\t" << _ny << endl;
    cout << "Global = " << _global << endl;
  } else if(_type == gmap["mercator"])
    {
    } else if(_type == gmap["cartesian"])
      {
      } else if(_type == gmap["lambert conformal"])
	{
	  cout << "\tlambert conformal" << endl;
	  cout << "tangetlat\t" << eqvlat(_latin1, _latin2) << endl;
	  cout << "tangetlon\t" << _lov << endl;
	  cout << "reflat\t" << _la1 << endl;
	  cout << "reflon\t" << _lo1 << endl;
	  cout << "iref\t0" << endl;
	  cout << "jref\t0" << endl;
	  cout << "stdlat\t" << _latin1 << endl;
	  cout << "stdlon\t" << _lov << endl;
	  cout << "delta\t" << _delx << endl;
	} else if(_type == gmap["cylindrical"])
	  {
	  } else if(_type == gmap["polar stereographic"])
	    {
	      cout << "\tpolar stereographic" << endl;
	      cout << "tangetlat\t90.0" << endl;
	      cout << "tangetlon\t0.0" << endl;
	      cout << "reflat\t" << _la1 << endl;
	      cout << "reflon\t" << _lo1 << endl;
	      cout << "iref\t0" << endl;
	      cout << "jref\t0" << endl;
	      cout << "stdlat\t60.0"<< endl;
	      cout << "stdlon\t" << _lov << endl;
	      cout << "delta\t" << _delx << endl;
	    }
  cout << "-------------------------------" << endl;
}

// Allows models that do not follow standred convention to be read in.
// If it fails then we have no idea what kind of model this file is.
// Return code of -1 means we had an indicator of model but not all variables were present
// Return code of -2 means unknown file type.
int CmapfModel::otherModelTypes(NcFile *ncf)
{
  char *attribute = new char[200];
  int projIndex, wdt_version;

  //
  // Maple NetCDF format for WDT tot precip data
  if (getAtt(ncf, "projName", &attribute) == 0 && strncmp(attribute, "LATLON", 6) == 0)
    if (getAtt(ncf, "projIndex", &projIndex) == 0 && projIndex == 8)
      {
	if(getAtt(ncf, "lat00", &_la1) < 0) {
	  _errString = "Unable to read lat00";
	  return -1;
	}
	if(getAtt(ncf, "lon00", &_lo1) < 0) {
	  _errString = "Unable to read lon00";
	  return -1;
	}
	if (getAtt(ncf, "dxKm", &_delx) < 0) {
	  _errString = "Unable to read dxKm";
	  return -1;
	}
	if (getAtt(ncf, "dyKm", &_dely) < 0) {
	  _errString = "Unable to read dyKm";
	  return -1;
	}
	if (getDim(ncf, "x", &_nx) < 0) {
	  _errString = "Unable to read x";
	  return -1;
	}
	if (getDim(ncf, "y", &_ny) < 0) {
	  _errString = "Unable to read y";
	  return -1;
	}

	_iref = 0.0;
	_jref = 0.0;
	// Determine if the grid is global; ie, that it wraps around longitudinally.
	_global = 0;
	if ((_nx * _delx) == 360.)
	  _global = 1;

	_type = gmap["latitude/longitude"];

	return 0;
      }
  //
  // Maple NetCDF format for WDT precip type data
  if (getAtt(ncf, "projName", &attribute) == 0 && strncmp(attribute, "LATLON", 6) == 0)
    if (getAtt(ncf, "wdt_version", &wdt_version) == 0 && wdt_version == 1)
      {
	if(getAtt(ncf, "lat00", &_la1) < 0) {
	  _errString = "Unable to read lat00";
	  return -1;
	}
	if(getAtt(ncf, "lon00", &_lo1) < 0) {
	  _errString = "Unable to read lon00";
	  return -1;
	}
	if (getAtt(ncf, "dxKm", &_delx) < 0) {
	  _errString = "Unable to read dxKm";
	  return -1;
	}
	if (getAtt(ncf, "dyKm", &_dely) < 0) {
	  _errString = "Unable to read dyKm";
	  return -1;
	}
	if (getDim(ncf, "x", &_nx) < 0) {
	  _errString = "Unable to read x";
	  return -1;
	}
	if (getDim(ncf, "y", &_ny) < 0) {
	  _errString = "Unable to read y";
	  return -1;
	}

	_iref = 0.0;
	_jref = 0.0;
	// Determine if the grid is global; ie, that it wraps around longitudinally.
	_global = 0;
	if ((_nx * _delx) == 360.)
	  _global = 1;

	_type = gmap["latitude/longitude"];

	return 0;
      }
  //
  // Really messed up Maple NetCDF format for WDT tot precip data 
  // I've hard coded some values, since global attribute definition is ambiguous.
  if (getAtt(ncf, "wdt_version", &wdt_version) == 0 && wdt_version == 1)
    if (getAtt(ncf, "proj_name", &attribute) == 0 && strncmp(attribute, "LATLON", 6) == 0)
    {
      if (getDim(ncf, "nx", &_nx) < 0) {
	_errString = "Unable to read x";
	return -1;
      }
      if (getDim(ncf, "ny", &_ny) < 0) {
	_errString = "Unable to read y";
	return -1;
      }
      /*
      if(getAtt(ncf, "ref_lat", &_la1) < 0) {
	_errString = "Unable to read lat00";
	return -1;
      }
      if(getAtt(ncf, "ref_lon", &_lo1) < 0) {
	_errString = "Unable to read lon00";
	return -1;
      }
      if (getAtt(ncf, "dx_km", &_delx) < 0) {
	_errString = "Unable to read dxKm";
	return -1;
      }
      if (getAtt(ncf, "dy_km", &_dely) < 0) {
	_errString = "Unable to read dyKm";
	return -1;
      }
      */
      if(_nx == 5445 && _ny == 4226) 
	{
	  _la1 = 20.0;
	  _lo1 = -128.0;
	  _delx = .01157025;
	  _dely = .00899195;
	  _iref = 0.0;
	  _jref = 0.0;
	  // Determine if the grid is global; ie, that it wraps around longitudinally.
	  _global = 0;
	  if ((_nx * _delx) == 360.)
	    _global = 1;
	  
	  _type = gmap["latitude/longitude"];
	  
	  return 0;
	}	
    }
  return -2;
}

//---------------------------------------------------------------------------//

// The getNavVal() set of methods which return different netcdf variable types.
// Try reading value as scalar first, if that fails, try getting the first value
// in the vector.


int CmapfModel::getNavVal(NcFile *ncf, const char *var_name, int *val)
{
  int ndim, *vals;
  long size, *dim;

  if (NCF_get_var(ncf, var_name, val) == 0)
    return (0);

  if (NCF_get_var(ncf, var_name, &vals, &dim, &ndim, &size) < 0)
    return (-1);

  *val = vals[0];
  delete [] dim;
  delete [] vals;

  return (0);
}

int CmapfModel::getNavVal(NcFile *ncf, const char *var_name, short *val)
{
  int ndim;
  short *vals;
  long size, *dim;

  if (NCF_get_var(ncf, var_name, val) == 0)
    return (0);

  if (NCF_get_var(ncf, var_name, &vals, &dim, &ndim, &size) < 0)
    return (-1);
  
  *val = vals[0];
  delete [] dim;
  delete [] vals;

  return (0);
}

int CmapfModel::getNavVal(NcFile *ncf, const char *var_name, float *val)
{
  int ndim;
  long size, *dim;
  float *vals;

  if (NCF_get_var(ncf, var_name, val) == 0)
    return (0);

  if(NCF_get_var(ncf, var_name, &vals, &dim, &ndim, &size) < 0)
     return (-1);

  *val = vals[0];
  delete [] dim;
  delete [] vals;

  return (0);
}

int CmapfModel::getNavVal(NcFile *ncf, const char *var_name, char **val, long *size)
{
  int ndim;
  long *dim;
  char *vals;

  if(NCF_get_var(ncf, var_name, &vals, &dim, &ndim, size) < 0)
     return (-1);

  *val = vals;

  delete [] dim;

  return (0);
}

int CmapfModel::getAtt(NcFile *ncf, const char *att_name, char **att)
{
  if(NCF_get_attr(ncf, att_name, att) < 0)
    return (-1);

  return (0);
}

int CmapfModel::getAtt(NcFile *ncf, const char *att_name, float *val)
{
  if(NCF_get_attr(ncf, att_name, val) < 0)
    return (-1);

  return (0);
}

int CmapfModel::getAtt(NcFile *ncf, const char *att_name, int *val)
{
  if(NCF_get_attr(ncf, att_name, val) < 0)
    return (-1);
  
  return (0);
}
  
int CmapfModel::getDim(NcFile *ncf, const char *dim_name, int *val)
{
  *val = NCF_get_dim_size(ncf, dim_name);
  if(*val < 0)
    return (-1);
  return (0);
}
