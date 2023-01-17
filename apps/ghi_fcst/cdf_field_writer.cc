//==============================================================================
//
//   (c) Copyright, 2013 University Corporation for Atmospheric Research (UCAR).
//       All rights reserved.
//       Do not copy or distribute without authorization.
//
//       File: $RCSfile: cdf_field_writer.cc,v $
//       Version: $Revision: 1.4 $  Dated: $Date: 2014/05/13 20:40:59 $
//
//==============================================================================

/**
 *
 * @file cdf_field_writer.cc
 *
 */

// Include files 
#include <iostream>
#include <math.h>
#include <sstream>
#include "cdf_field_writer.hh"

using namespace netCDF;
using namespace netCDF::exceptions;

using std::string;
using std::vector;

// Constant and macros

// Types, structures and classes

// Global variables 

// Functions

cdf_field_writer::cdf_field_writer(const string &cdl_file_name, const string &file_name) : cdl_file_name_(cdl_file_name), file_name_(file_name)
{
  error_ = "";
  units_name_ = "units";
  missing_name_ = "_FillValue";
  data_file_ = 0;

  // ncgen file name
  if (cdl_file_name.substr(cdl_file_name_.size() - 3, 3) != "cdl")
    {
      error_ = "bad cdl name";
      return;
    }

  string ncgen_command = "ncgen " + cdl_file_name + " -o " + file_name_;
  int ret = system(ncgen_command.c_str());
  if (ret != 0)
    {
      error_ = "ncgen failure: " + ncgen_command;
      return;
    }

  try
    {
      // Open the file and check to make sure it's valid.
      data_file_ = new netCDF::NcFile(file_name_, netCDF::NcFile::write);
      dimension_map_ = data_file_->getDims();
      var_map_ = data_file_->getVars();
    }
  catch (netCDF::exceptions::NcException e)
    {
      // Capture exception in error_ by redirecting cout
      error_ = e.what();
    }
}

cdf_field_writer::cdf_field_writer(const string &file_name, const std::unordered_map<string, size_t> dimension_map)
{
  error_ = "";
  units_name_ = "units";
  missing_name_ = "_FillValue";
  data_file_ = 0;

  try
    {
      // Open the file and check to make sure it's valid.
      data_file_ = new netCDF::NcFile(file_name_, netCDF::NcFile::replace);

      // Add dimensions to dimension map
      for (auto itr = dimension_map.begin(); itr != dimension_map.end(); ++itr)
	{
	  NcDim dim = data_file_->addDim(itr->first, itr->second);
	  dimension_map_.insert(std::pair<string, NcDim>(itr->first, dim));
	}
    }
  catch (netCDF::exceptions::NcException e)
    {
      // Capture exception in error_ by redirecting cout
      error_ = e.what();
    }

  return;
}

int cdf_field_writer::add_field(const string &field_name, NcType nc_type, const vector<string> &field_dimension_names, const string &long_name, const string &units, double missing, string &error)
{
  vector<NcDim> dim_array(field_dimension_names.size());
  for (size_t i=0; i<field_dimension_names.size(); i++)
    {
      auto itr = dimension_map_.find(field_dimension_names[i]);
      if (itr != dimension_map_.end())
	{
	  dim_array[i] = itr->second;
	}
      else
	{
	  error = string("dimension name not in dimension map: ") + field_dimension_names[i];
	  return -1;
	}
    }

  // Get the variable
  netCDF::NcVar var;
  var = data_file_->addVar(field_name, nc_type, dim_array);
  if (var.isNull())
    {
      error = string("could not find variable: ") + field_name;
      return -1;
    }
  var_map_.insert(std::pair<string, NcVar>(field_name, var));
  var.putAtt("long_name", long_name);
  var.putAtt("units", units);
  if (nc_type == NC_SHORT)
    {
      var.putAtt("_FillValue", nc_type, static_cast<short>(missing));
    }
  else if (nc_type == NC_INT)
    {
      var.putAtt("_FillValue", nc_type, static_cast<int>(missing));
    }
  else if (nc_type == NC_FLOAT)
    {
      var.putAtt("_FillValue", nc_type, static_cast<float>(missing));
    }
  else if (nc_type == NC_DOUBLE)
    {
      var.putAtt("_FillValue", nc_type, static_cast<double>(missing));
    }
  else if (nc_type == NC_CHAR)
    {
      var.putAtt("_FillValue", nc_type, static_cast<char>(missing));
    }
  return 0;
}

int cdf_field_writer::add_fields(const vector<string> &field_names, const vector<NcType> &nc_type, const vector< vector<string> > &field_dimension_names, const vector<string> &long_names, const vector<string> &units, vector<double> missing, string &error)
{
  for (size_t i=0; i<field_names.size(); i++)
    {
      int ret = add_field(field_names[i], nc_type[i], field_dimension_names[i], long_names[i], units[i], missing[i], error);
      if (ret != 0)
	{
	  return -1;
	}
    }

  return 0;
}

int cdf_field_writer::put_field(const string &field_name, const vector<short> &field, string &error)
{
  auto itr = var_map_.find(field_name);
  if (itr != var_map_.end())
    {
      NcVar var = itr->second;
      var.putVar(&field[0]);
    }
  else
    {
      error = string("var name not in var map: ") + field_name;
      return -1;
    }
  
  return 0;
}

int cdf_field_writer::put_fields(const vector<string> &field_names, const vector< vector<short> > &field_vector, string &error)
{
  string error_occurred;
  int error_return = 0;

  for (size_t i=0; i<field_names.size(); i++)
    {
      error_return = put_field(field_names[i], field_vector[i], error_occurred);
      if (error_return != 0)
	{
	  error = error_occurred;
	}
    }

  return error_return;
}

int cdf_field_writer::put_field(const string &field_name, const vector<char> &field, string &error)
{
  auto itr = var_map_.find(field_name);
  if (itr != var_map_.end())
    {
      NcVar var = itr->second;
      var.putVar(&field[0]);
    }
  else
    {
      error = string("var name not in var map: ") + field_name;
      return -1;
    }
  
  return 0;
}


int cdf_field_writer::put_fields(const vector<string> &field_names, const vector< vector<char> > &field_vector, string &error)
{
  string error_occurred;
  int error_return = 0;

  for (size_t i=0; i<field_names.size(); i++)
    {
      error_return = put_field(field_names[i], field_vector[i], error_occurred);
      if (error_return != 0)
	{
	  error = error_occurred;
	}
    }

  return error_return;
}


int cdf_field_writer::put_field(const string &field_name, const vector<int> &field, string &error)
{
  auto itr = var_map_.find(field_name);
  if (itr != var_map_.end())
    {
      NcVar var = itr->second;
      var.putVar(&field[0]);
    }
  else
    {
      error = string("var name not in var map: ") + field_name;
      return -1;
    }
  
  return 0;
}

int cdf_field_writer::put_fields(const vector<string> &field_names, const vector< vector<int> > &field_vector, string &error)
{
  string error_occurred;
  int error_return = 0;

  for (size_t i=0; i<field_names.size(); i++)
    {
      error_return = put_field(field_names[i], field_vector[i], error_occurred);
      if (error_return != 0)
	{
	  error = error_occurred;
	}
    }

  return error_return;
}

int cdf_field_writer::put_field(const string &field_name, const vector<float> &field, string &error)
{
  auto itr = var_map_.find(field_name);
  if (itr != var_map_.end())
    {
      NcVar var = itr->second;
      var.putVar(&field[0]);
    }
  else
    {
      error = string("var name not in var map: ") + field_name;
      return -1;
    }
  
  return 0;
}

int cdf_field_writer::put_fields(const vector<string> &field_names, const vector< vector<float> > &field_vector, string &error)
{
  string error_occurred;
  int error_return = 0;

  for (size_t i=0; i<field_names.size(); i++)
    {
      error_return = put_field(field_names[i], field_vector[i], error_occurred);
      if (error_return != 0)
	{
	  error = error_occurred;
	}
    }

  return error_return;
}


int cdf_field_writer::put_field(const string &field_name, const vector<double> &field, string &error)
{
  auto itr = var_map_.find(field_name);
  if (itr != var_map_.end())
    {
      NcVar var = itr->second;
      var.putVar(&field[0]);
    }
  else
    {
      error = string("var name not in var map: ") + field_name;
      return -1;
    }
  
  return 0;
}

int cdf_field_writer::put_fields(const vector<string> &field_names, const vector< vector<double> > &field_vector, string &error)
{
  string error_occurred;
  int error_return = 0;

  for (size_t i=0; i<field_names.size(); i++)
    {
      error_return = put_field(field_names[i], field_vector[i], error_occurred);
      if (error_return != 0)
	{
	  error = error_occurred;
	}
    }

  return error_return;
}

cdf_field_writer::~cdf_field_writer()
{
  delete data_file_;
}
