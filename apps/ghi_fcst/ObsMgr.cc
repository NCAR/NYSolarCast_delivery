//==============================================================================
//
//   (c) Copyright, 2009 University Corporation for Atmospheric Research (UCAR).
//       All rights reserved.
//       Do not copy or distribute without authorization.
//
//       File: $RCSfile: ObsMgr.cc,v $
//       Version: $Revision: 1.2 $  Dated: $Date: 2015/10/29 20:33:20 $
//
//==============================================================================

/**
 *
 * @file ObsMgr.cc  Source code for ObsMgr class
 *
 * @date 07/29/2021
 */

// Include files 

#include <iostream>
#include "ObsMgr.hh"

using std::cerr;
using std::endl;

ObsMgr::ObsMgr()
{
 
}

ObsMgr::ObsMgr(ObsReader *obsFile)
{
  _obsFiles.push_back(obsFile);
}

ObsMgr::~ObsMgr()
{
  for (int i = 0; i <  (int)_obsFiles.size(); i++)
  {
    delete  _obsFiles[i];
  }

  _obsFiles.erase( _obsFiles.begin(), _obsFiles.end());
}

void ObsMgr::add(ObsReader *obsFile)
{
  // 
  // Push files on to vector in creation time order
  //
  if ( _obsFiles.empty() )
  {
    _obsFiles.push_back(obsFile);
  }
  else
  {
    int i = 0;

    
    while ( i < (int) _obsFiles.size() && 
	    obsFile->getStartTime() > _obsFiles[i]->getStartTime())
    {  
      i++;
    }
    _obsFiles.insert(_obsFiles.begin() + i, obsFile);
  }
}

const int ObsMgr::getObsFileIndex(const int siteId, const double obsTime) const
{
  //
  // Files are ordered by observation time, with first file in vector
  // having the most recent observaton time.
  //
  bool gotIndex = false;

  int index = 0;

  while( !gotIndex && index < (int)_obsFiles.size())
  {
    gotIndex =  _obsFiles[index]->haveData(siteId, obsTime);
    
    if(!gotIndex)
    {
      index++;
    }
  }

  if (gotIndex && index < (int)_obsFiles.size() )
  {
    return index;
  }
  else
  {
    return -1;	
  }			  
}

const float ObsMgr::getAzimuth(const int siteId, const double obsTime)
{
  int i = getObsFileIndex(siteId, obsTime);

  if(i >= 0)
  {
    return (_obsFiles[i]->getAzimuth(siteId, obsTime));
  }
  else
  {
    return  ObsReader::OBS_MISSING;
  }
}

const float ObsMgr::getElevation(const int siteId, const double obsTime)
{ 
  int i = getObsFileIndex(siteId, obsTime);
  
  if(i >= 0)
  { 
    return (_obsFiles[i]->getElevation(siteId, obsTime));
  }
  else
  { 
    return  ObsReader::OBS_MISSING;
  }
}

const float ObsMgr::getGHI(const int siteId, const double obsTime)
{
  int i = getObsFileIndex(siteId, obsTime);

  if(i >= 0)
  {
    return (_obsFiles[i]->getGHI(siteId, obsTime));
  }
  else
  {
    return  ObsReader::OBS_MISSING;
  }
}

const float ObsMgr::getKt(const int siteId, const double obsTime)
{
  int i = getObsFileIndex(siteId, obsTime);

  if(i >= 0)
  {
    return (_obsFiles[i]->getKt(siteId, obsTime));
  }
  else
  {
    return  ObsReader::OBS_MISSING;
  }
}

const float ObsMgr::getPressure(const int siteId, const double obsTime)
{
  int i = getObsFileIndex(siteId, obsTime);

  if(i >= 0)
  {
    return (_obsFiles[i]->getPressure(siteId, obsTime));
  }
  else
  {
    return  ObsReader::OBS_MISSING;
  }
}

const float ObsMgr::getRh(const int siteId, const double obsTime)
{
  int i = getObsFileIndex(siteId, obsTime);

  if(i >= 0)
  {
    return (_obsFiles[i]->getRh(siteId, obsTime));
  }
  else
  {
    return  ObsReader::OBS_MISSING;
  }
}


const float ObsMgr::getTemp(const int siteId, const double obsTime) 
{
  int i = getObsFileIndex(siteId, obsTime);

  if(i >= 0)
  {   
    return (_obsFiles[i]->getTemp(siteId, obsTime));
  }
  else
  {
    return  ObsReader::OBS_MISSING;
  }    
}


const float ObsMgr::getToa(const int siteId, const double obsTime)
{
  int i = getObsFileIndex(siteId, obsTime);

  if(i >= 0)
  {   
    return (_obsFiles[i]->getToa(siteId, obsTime));
  }
  else
  {
    return  ObsReader::OBS_MISSING;
  }    
}

const float ObsMgr::getWindDir(const int siteId, const double obsTime)
{
  int i = getObsFileIndex(siteId, obsTime);

  if(i >= 0)
  {
    return (_obsFiles[i]->getWindDir(siteId, obsTime));
  }
  else
  {
    return  ObsReader::OBS_MISSING;
  }
}

const float ObsMgr::getWindSpeed(const int siteId, const double obsTime) 
{
  int i = getObsFileIndex(siteId, obsTime);

  if(i >= 0)
  {   
    return (_obsFiles[i]->getWindSpeed(siteId, obsTime));
  }
  else
  {
    return  ObsReader::OBS_MISSING;
  }    
}

