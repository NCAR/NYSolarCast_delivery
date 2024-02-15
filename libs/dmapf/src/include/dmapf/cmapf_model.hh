/*
 *   Module: cmapf_model.hh
 *
 *   Author: Jason Craig
 *
 *   Date:   04/05/2005
 *
 *   Description: Clas to use NCEP models that have been converted
 *   to NetCDF with cmapf, a map projection conversion library.
 *   NetCDF file must have grid_type_code and appropriate variables
 *   to describe map projection.

 *  Needed variables for each grid_type_code:
 *  0 = "latitude/longitude",  La1, Lo1, Di, Dj, Ni, Nj
 *  3 = "lambert conformal",   La1, Lo1, Lov, Latin1, Latin2, Dx or Dy
 *  5 = "polar stereographic", La1, Lo1, Lov, Dx or Dy, (reflat is assumed 60)
 *
 */

#ifndef CMAPF_MODEL_HH
#define CMAPF_MODEL_HH

#include <map>
#include <string>
#include <ncf/ncf.hh>
#include "cmapf.h"

using namespace std;

typedef map<string, int> gridTypeMap;

class CmapfModel
{
public:

  CmapfModel(NcFile *ncf) ;
  ~CmapfModel() {};

  // Check after constructor to see if map projection was loaded.
  inline int error() {return (_errString.length() > 0);}
  // If error > 0 returns message indicating why
  inline string errString() {return (_errString);}
  // Prints the Navigation information used for Dmapf
  void printNav();

  // Converts from lat,lon to x,y in double form
  int ll2xy(double lat, double longit, double *x, double *y);
  int xy2ll(double x, double y, double *lat, double *longit);

  // Converts from lat,lon to x,y in float form
  int ll2xy(float lat, float longit, float *x, float *y);
  int xy2ll(float x, float y, float *lat, float *longit);

private:

  void clear() {_errString = "";};

  int otherModelTypes(NcFile *ncf);

  int getNavVal(NcFile *ncf, const char *var_name, int *val);
  int getNavVal(NcFile *ncf, const char *var_name, short *val);
  int getNavVal(NcFile *ncf, const char *var_name, float *val);
  int getNavVal(NcFile *ncf, const char *var_name, char **val, long *len);
  int getAtt(NcFile *ncf, const char *att_name, char **att);
  int getAtt(NcFile *ncf, const char *att_name, float *val);
  int getAtt(NcFile *ncf, const char *att_name, int *val);
  int getDim(NcFile *ncf, const char *dim_name, int *val);

  gridTypeMap gmap;

  maparam _stcpm;
  string _errString;
  int _type;
  int _global, _nx, _ny;
  float _la1, _lo1, _delx, _dely;
  float _lov, _latin1, _latin2;
  float _iref, _jref;
  float _erad;
};

#endif /* CMAPF_MODEL_HH */
