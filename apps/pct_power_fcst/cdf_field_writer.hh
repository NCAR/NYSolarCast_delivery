//==============================================================================
//
//   (c) Copyright, 2013 University Corporation for Atmospheric Research (UCAR).
//       All rights reserved. 
//
//       File: $RCSfile: cdf_field_writer.hh,v $
//       Version: $Revision: 1.4 $  Dated: $Date: 2013/08/30 16:27:05 $
//
//==============================================================================

/**
 * @file cdf_field_writer.hh
 *
 * @class cdf_field_writer
 *
 *  <file/class description>
 *  
 * @date 8/20/13
 */

#ifndef CDF_FIELD_WRITER_HH
#define CDF_FIELD_WRITER_HH


#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <netcdf>
using namespace netCDF;
using namespace netCDF::exceptions;

using std::string;
using std::unordered_map;
using std::vector;


/**
 * @class cdf_field_writer
*/
class cdf_field_writer
{
public:
  /** Constructor */
  cdf_field_writer(const string &cdl_file_name, const string &file_name);

  /** Constructor */
  cdf_field_writer(const string &file_name, const std::unordered_map<string, size_t> dimension_map);

  /** Destructor */
  virtual ~cdf_field_writer();

  string error() { return error_;}

  /* 
  int put_field(const string &field_name, const vector<char> &field, const string &dimension_name, string &error);
  int put_fields(const vector<string> &field_names, vector< vector<char> > &field_vector, vector< vector<string> > &field_dimension_names, string &error);
  */
  
  const std::multimap<string, netCDF::NcDim> &get_dimension_map() const
  {
    return dimension_map_;
  }

  const std::multimap<string, netCDF::NcVar> &get_var_map() const
  {
    return var_map_;
  }

  int add_field(const string &field_name, netCDF::NcType nc_type, const vector<string> &field_dimension_names, const string &long_name, const string &units, double missing, string &error);

  int add_fields(const vector<string> &field_names, const vector<netCDF::NcType> &nc_type, const vector< vector<string> > &field_dimension_names, const vector<string> &long_names, const vector<string> &units, vector<double> missing, string &error);

  int put_field(const string &field_name, const vector<char> &field, string &error);
  int put_fields(const vector<string> &field_names, const vector< vector<char> > &field_vector, string &error);

  int put_field(const string &field_name, const vector<string> &field, string &error);
  int put_field(const vector <string> &field_names, const vector<vector<string> > &field_vector, string &error);

  int put_field(const string &field_name, const vector<short> &field, string &error);
  int put_fields(const vector<string> &field_names, const vector< vector<short> > &field_vector, string &error);

  int put_field(const string &field_name, const vector<int> &field, string &error);
  int put_fields(const vector<string> &field_names, const vector< vector<int> > &field_vector, string &error);

  int put_field(const string &field_name, const vector<float> &field, string &error);
  int put_fields(const vector<string> &field_names, const vector< vector<float> > &field_vector, string &error);

  int put_field(const string &field_name, const vector<double> &field, string &error);
  int put_fields(const vector<string> &field_names, const vector< vector<double> > &field_vector, string &error);

private:
  std::multimap<string, netCDF::NcVar> var_map_;
  std::multimap<string, netCDF::NcDim> dimension_map_;
  string cdl_file_name_;
  string file_name_;
  string error_;
  string units_name_;
  string missing_name_;
  netCDF::NcFile *data_file_;
};

#endif /* CDF_FIELD_WRITER_HH */
