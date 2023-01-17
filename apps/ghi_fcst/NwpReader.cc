//==============================================================================
//
//   (c) Copyright, 2009 University Corporation for Atmospheric Research (UCAR).
//       All rights reserved.
//       Do not copy or distribute without authorization.
//
//       File: $RCSfile: NwpReader.cc,v $
//       Version: $Revision: 1.1 $  Dated: $Date: 2014/10/15 13:55:20 $
//
//==============================================================================

/**
 *
 * @file NwpReader.cc  Source code file for NwpReader class
 *
 * @date 07/29/2021
 *
 * @todo [optional]
 *
 */

// Include files 

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include "ncfc/ncfc.hh"
#include "NwpReader.hh"

namespace fs = boost::filesystem;
using std::find;
using std::cerr;
using std::endl;
const float NwpReader::NWP_MISSING = NC_FILL_FLOAT;
const int NwpReader::FCST_TIME_RESOLUTION = 900;

NwpReader::NwpReader(string &nwpFile): 
  inputFile(nwpFile)
{
}

int NwpReader::parse()
{

  //
  // Note that the netcdf format and its contents are assumed to be known
  //

  error = string("");

  fs::path filePath(inputFile);
  if (!fs::exists(filePath))
    {
      error = string("Error: cdf file") + inputFile +  "does not exist";
      return 1;
    }
    
  vector<string> dimNames;
  dimNames.push_back("max_site_num");
  dimNames.push_back("fcst_times");
  dimNames.push_back("name_strlen");

  vector<string> varNames;
  varNames.push_back("creation_time");
  varNames.push_back("valid_time");
  varNames.push_back("num_sites");
  varNames.push_back("StationName");
  varNames.push_back("StationID");
  varNames.push_back("Q2");
  varNames.push_back("SWDDNI");
  varNames.push_back("SWDDIF");
  varNames.push_back("SWDOWN");
  varNames.push_back("TAOD5502D");
  varNames.push_back("CLDFRAC2D");
  varNames.push_back("WVP");
  varNames.push_back("WP_TOT_SUM");
  varNames.push_back("TAU_QC_TOT");
  varNames.push_back("TAU_QI_TOT");
  varNames.push_back("TAU_QS");
  varNames.push_back("T2");
  varNames.push_back("PSFC");
  varNames.push_back("CLRNIDX");
  varNames.push_back("TOA");
  varNames.push_back("WSPD10");
  varNames.push_back("WDIR10");
  varNames.push_back("custom_TOA");
  varNames.push_back("apparent_elevation");
  varNames.push_back("azimuth");
  varNames.push_back("custom_KT");

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

  ind = varIndexMap["creation_time"];

  creationTime = ((double *)inVarPtrs[ind])[0];

  // 
  // Copy site list (integer ids) to vector
  //
  ind = varIndexMap["StationID"];

  int numIds = inVarSizes[ind];

  for (int i=0; i<numIds; i++)
    {
      int site = ((int *)inVarPtrs[ind])[i];

      siteList.push_back(site);
    }

 //
 // Copy station name strings to vector
 //
  ind = varIndexMap["StationName"];

  int numNames = inVarSizes[ind];

  for (int i=0; i<numNames; i++)
    { 
      char name[16];
      for (int j = 0; j < (int) fileDimMap["name_strlen"]; j++)
      {
         name[j] = ((char *)inVarPtrs[ind])[i+j];
      }
      siteNames.push_back(string(name));
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
  // Copy mixing ratio to vector
  //
  ind = varIndexMap["Q2"];
  int numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    mixingRatio.push_back(val);
  }

  //
  // Copy DNI to vector
  //
  ind = varIndexMap["SWDDNI"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    dni.push_back(val);
  }

  //
  // Copy diffuse irradiance to vector
  //
  ind = varIndexMap["SWDDIF"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    dhi.push_back(val);
  }

  //
  // Copy GHI to vector
  //
  ind = varIndexMap["SWDOWN"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    ghi.push_back(val);
  }

  //
  // Copy total aerosol optical depth to vector
  //
  ind = varIndexMap["TAOD5502D"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    taod5502d.push_back(val);
  }
  
  //
  // Copy cloud fraction to vector
  //
  ind = varIndexMap["CLDFRAC2D"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    cloudFrac.push_back(val);
  }

  //
  // Copy water vapor path to vector
  //
  ind = varIndexMap["WVP"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    wvp.push_back(val);
  }

  //
  // Copy total water path ( liquid+ice+snow) to vector 
  //
  ind = varIndexMap["WP_TOT_SUM"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    wpTot.push_back(val);
  }
  
  //
  // Copy mass weighted liquid cloud optical thickness to vector 
  //
  ind = varIndexMap["TAU_QC_TOT"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    tauQcTot.push_back(val);
  }
 
  //
  // Copy mass weighted ice optical thickness to vector 
  //
  ind = varIndexMap["TAU_QI_TOT"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    tauQiTot.push_back(val);
  }

  //
  // Copy mass weighted snow optical thickness to vector 
  //
  ind = varIndexMap["TAU_QS"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    tauQs.push_back(val);
  }

  //
  // Copy temperature data to vector 
  //
  ind = varIndexMap["T2"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
   temp.push_back(val);
  }

  //
  // Copy pressure data to vector 
  //
  ind = varIndexMap["PSFC"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    pSfc.push_back(val);
  }

  //
  // Clearness index computed with WRF TOA
  //
  ind = varIndexMap["CLRNIDX"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    wrfKt2.push_back(val);
  }

  
  //
  // WRF Top of atmosphere irradiance data to vector 
  //
  ind = varIndexMap["TOA"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    wrfToa2.push_back(val);
  }

  //
  // Copy wind speed data to vector 
  //
  ind = varIndexMap["WSPD10"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    windSpeed.push_back(val);
  }

  //
  // Copy wind direction data to vector 
  //
  ind = varIndexMap["WDIR10"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    windDir.push_back(val);
  }

  //
  // Copy custom TOA data to vector
  //https://pvpmc.sandia.gov/modeling-steps/1-weather-design-inputs/irradiance-and-insolation-2/extraterrestrial-radiation/
  // 
  ind = varIndexMap["custom_TOA"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    toa.push_back(val);
  }
 
  //
  // Copy solar elevation angle data to vector
  //
  ind = varIndexMap["apparent_elevation"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    elevation.push_back(val);
  }

  //
  // Copy solar azimuth angle data to vector
  //
  ind = varIndexMap["azimuth"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  {
    float val = ((float *)inVarPtrs[ind])[i];
    azimuth.push_back(val);
  }

  //
  // Copy WrfGHI/customTOA to vector
  //
  ind = varIndexMap["custom_KT"];
  numObs = inVarSizes[ind];
  for (int i=0; i<numObs; i++)
  { 
    float val = ((float *)inVarPtrs[ind])[i];
    kt.push_back(val);
  }

  //
  // Map siteIds to integer indices
  //
  for (int i = 0; i < numSites; i++)
  {
    siteIdIndexMap[siteList[i]] = i;
  }

  //
  // Map siteIds to integer indices
  //
  for (int i = 0; i < numSites; i++)
  {
    siteNamesMap[siteList[i]] = siteNames[i];
  }

  return 0;
}

const bool NwpReader::haveData(double fcstTime) const
{
  //
  // Check to see if fcstTime is in [firstFcstTime,lastFcstTime] 
  //
  if ( fcstTime >= validTime[0] && 
       fcstTime <= lastFcstTime && (int)fcstTime % FCST_TIME_RESOLUTION == 0 )
  {
    return true;
  }
  else
  {
    return false;
  }
}

const int NwpReader::getSiteIndex(const int site) 
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

const int NwpReader::getArrayOffset( const int siteId, double fcstTime) 
{
  int fcstIndex = (fcstTime - validTime[0])/FCST_TIME_RESOLUTION ;    

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

const float NwpReader::getAzimuth( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return azimuth[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getCloudFrac( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return cloudFrac[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getDHI( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return dhi[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getDNI( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return dni[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getElevation( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return elevation[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getGHI( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return ghi[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getKt( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return kt[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getMixingRatio(const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return mixingRatio[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getPsfc( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return pSfc[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getRh( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return rh[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getTaod5502d( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return taod5502d[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getTauQcTot( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return tauQcTot[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getTauQiTot( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return tauQiTot[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getTauQs( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return tauQs[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getTemp( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return temp[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getToa( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return toa[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getWindDir( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return windDir[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getWindSpeed( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return windSpeed[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getWpTot( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return wpTot[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getWvp( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return wvp[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getWrfKt2( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return wrfKt2[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}

const float NwpReader::getWrfToa2( const int siteId, const double fcstTime)
{
  int arrayOffset =  getArrayOffset(siteId, fcstTime);

  if ( arrayOffset >= 0)
  {
    return wrfToa2[arrayOffset];
  }
  else
  {
    return NWP_MISSING;
  }
}
