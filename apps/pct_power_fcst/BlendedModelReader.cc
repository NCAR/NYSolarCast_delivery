//=============================================================================
//
//   (c) Copyright, 2009 University Corporation for Atmospheric Research (UCAR).
//       All rights reserved.
//       Do not copy or distribute without authorization.
//
//       File: $RCSfile: BlendedModelReader.cc,v $
//       Version: $Revision: 1.1 $  Dated: $Date: 2014/10/15 13:55:20 $
//
//==============================================================================

/**
 *
 * @file BlendedModelReader.cc
 *       Reads Blended NWP and StatCast forecast data
 *
 * @note <brief note description> [optional]
 *
 * @date 05/18/2021 
 */

// Include files 

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include "ncfc/ncfc.hh"
#include "BlendedModelReader.hh"

namespace fs = boost::filesystem;
using std::find;
using std::cerr;
using std::endl;

const float BlendedModelReader::MISSING = NC_FILL_FLOAT;

BlendedModelReader::BlendedModelReader(string &dicastFile): 
  inputFile(dicastFile)
{
}

int BlendedModelReader::parse()
{
  error = string("");

  fs::path filePath(inputFile);
  if (!fs::exists(filePath))
    {
      error = string("Error: cdf file") + inputFile +  "does not exist";
      return 1;
    }
    
  //
  // NetCDF file dimensions
  //
  vector<string> dimNames;
  dimNames.push_back("max_site_num");
  dimNames.push_back("fcst_times");
  dimNames.push_back("name_strlen");

  //
  // NetCDF file variable names
  // 
  vector<string> varNames;
  varNames.push_back("gen_time");
  varNames.push_back("valid_time");
  varNames.push_back("num_sites");
  varNames.push_back("siteId");
  varNames.push_back("ClimateZone");
  varNames.push_back("ghi");
  varNames.push_back("RH");
  varNames.push_back("T2");

  //
  // Create a netCDF reader object
  //
  Var_input varInput(inputFile.c_str(), varNames, dimNames);
  int ret = varInput.error_status();
  if (ret != 0)
    {
      error = string("Error: Var_input constructor failed, error: ") + 
              varInput.error() + string(", return: ") + 
              string(nc_strerror(ret)) + string("\n");
      return 1;
    }

  //
  // Get the netCDF data
  //
  const vector<string> inVarNames = varInput.get_var_names();
  const vector<long int> inVarSizes = varInput.get_var_sizes();
  const vector<void *> inVarPtrs = varInput.get_var_ptrs();
  const vector<size_t *> inVarDims = varInput.get_var_dims();

  //
  // Create map from variable names to variable dimension lists 
  //
  map<string, size_t> fileDimMap;

  varInput.get_file_dim_map(fileDimMap);

  //
  // Create map from variable names to index of variable in file
  //
  map<string, uint> varIndexMap;

  varInput.get_var_index_map(varIndexMap);

  //
  // Get variables
  //
  int ind = varIndexMap["num_sites"];

  numSites =  ((int *)inVarPtrs[ind])[0];


  ind = varIndexMap["valid_time"];

  creationTime = ((double *)inVarPtrs[ind])[0];

  // 
  // Copy site list (integer ids) to vector
  //
  ind = varIndexMap["siteId"];

  int numSites = inVarSizes[ind];

  for (int i=0; i<numSites; i++)
    {
      int site = ((int *)inVarPtrs[ind])[i];

      siteList.push_back(site);
    }

  //
  // Get valid times
  //
  ind = varIndexMap["valid_time"];
  
  int numTimes = inVarSizes[ind];
  
  for (int i=0; i<numTimes; i++)
  {
     double t = ((double *)inVarPtrs[ind])[i];
  
     validTime.push_back(t);
  }
 
  //
  // Record the last valid time in file
  //
  if ( (int) validTime.size() > 0)
  {
    lastFcstTime = validTime[ (int) validTime.size() - 1];
  }
  else
  {
     error = string("Error: Empty valid_time array for ") + inputFile;

     return 1;
  }

  //
  // Set the forecast lead time resolution 
  //
  if ( (int) validTime.size() > 1)
  {
     fcst_time_resolution = validTime[1] - validTime[0];

     if (fcst_time_resolution <= 0)
     {
        error = string("Error: expecting valid time resolution > 0");
      
        return 1;
     } 
  }
  
  //
  // Copy climateZone to vector
  //
  ind = varIndexMap["ClimateZone"];
  int numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    int val = ((int *)inVarPtrs[ind])[i];
    climateZone.push_back(val);
  }

  //
  // Copy GHI to vector
  //
  ind = varIndexMap["ghi"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    ghi.push_back(val);
  }

  //
  // Copy relative humidity to vector
  //
  ind = varIndexMap["RH"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    rh.push_back(val);
  }
  
  //
  // Copy temperature to vector
  //
  ind = varIndexMap["T2"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    temp.push_back(val);
  }

  //
  // Map siteIds to integer indices
  //
  for (int i = 0; i < numSites; i++)
  {
    siteIdIndexMap[siteList[i]] = i;
  }

  //
  // Map climate zone to sites 
  //
  for (int i = 0; i < numSites; i++)
  {
    siteClimateZoneMap[siteList[i]] = climateZone[i];
  }

  return 0;
}

const bool BlendedModelReader::haveData(double fcstTime) const
{
  //
  // Check to see if fcstTime is in [firstFcstTime,lastFcstTime] 
  //
  if ( fcstTime >= validTime[0] && 
       fcstTime <= lastFcstTime && (int)fcstTime % fcst_time_resolution == 0 )
  {
    return true;
  }
  else
  {
    return false;
  }
}

const int BlendedModelReader::getSiteIndex(const int site) 
{
  
  if ( siteIdIndexMap.find(site) != siteIdIndexMap.end() )
  {
    return siteIdIndexMap[site];
  }
  else
  {
    return -1;
  }
}

const int BlendedModelReader::getArrayOffset( const int siteId, double fcstTime) 
{

  //
  // Use the site index and forecast index to get the array offset of data 
  //
  int fcstIndex = (fcstTime - validTime[0])/fcst_time_resolution ;    

  int siteIndex = getSiteIndex(siteId);
  
  //
  // Calculate array offset to the data at fcstIndex
  //
  int arrayOffset;

  if (siteIndex >= 0 && fcstIndex < (int) validTime.size())
  {
    arrayOffset =  siteIndex* (int)validTime.size() + fcstIndex;
    
     return arrayOffset;
  }
  else
  {
    return -1;
  }
}

const int BlendedModelReader::getClimateZone( const int siteId)
{

  return siteClimateZoneMap[siteId];
}

const float BlendedModelReader::getGHI( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return ghi[arrayOffset];
  }
  else
  {
    return MISSING;
  }
}

const float BlendedModelReader::getRh( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return rh[arrayOffset];
  }
  else
  {
    return MISSING;
  }
}

const float BlendedModelReader::getTemp( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return temp[arrayOffset];
  }
  else
  {
    return MISSING;
  }
}
