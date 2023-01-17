//==============================================================================
//
//   (c) Copyright, 2009 University Corporation for Atmospheric Research (UCAR).
//       All rights reserved.
//       Do not copy or distribute without authorization.
//
//       File: $RCSfile: NwpMgr.cc,v $
//       Version: $Revision: 1.1 $  Dated: $Date: 2014/10/15 13:55:20 $
//
//==============================================================================

/**
 *
 * @file NwpMgr.cc  Source code for NwpMgr class
 *
 * @date 07/28/2019
 *
 */

// Include files 

#include <iostream>
#include "NwpMgr.hh"

using std::cerr;
using std::endl;

NwpMgr::NwpMgr()
{
 
}

NwpMgr::NwpMgr( NwpReader *nwpFile)
{
  _nwpFiles.push_back(nwpFile);
}

NwpMgr::~NwpMgr()
{
  for (int i = 0; i <  (int)_nwpFiles.size(); i++)
  {
    delete  _nwpFiles[i];
  }
  _nwpFiles.erase( _nwpFiles.begin(), _nwpFiles.end());
}

void NwpMgr::add( NwpReader *nwpFile)
{
  // 
  // Push files on to vector in creation time order
  //
  if ( _nwpFiles.empty() )
  {
    _nwpFiles.push_back(nwpFile);
  }
  else
  {
    int i = 0;
    while ( i < (int) _nwpFiles.size() && 
	    nwpFile->getGenTime() < _nwpFiles[i]->getGenTime())
    {  
      i++;
    }
    _nwpFiles.insert(_nwpFiles.begin() + i, nwpFile);
  }
}

const int NwpMgr::getNwpFileIndex(const double fcstTime) const
{
  //
  // Files are ordered by creation time, with first file in vector
  // having the most recent creation time. Several files may have data values at
  // the input fcstTime but the file index of the file with the most recent
  // creation time is returned. If a file is not found then -1 is returned.
  //
  bool gotIndex = false;

  int index = 0;

  while( !gotIndex && index < (int)_nwpFiles.size())
  {
    gotIndex =  _nwpFiles[index]->haveData(fcstTime);
    if(!gotIndex)
    {
      index++;
    }
  }

  if (gotIndex && index < (int)_nwpFiles.size() )
  {
    return index;
  }
  else
  {
    return -1;
  }                       
}

const float NwpMgr::getAzimuth( const int siteId, const double fcstTime) 
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {   
    return _nwpFiles[i]->getAzimuth(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }    
}

const float NwpMgr::getCloudFrac( const int siteId, const double fcstTime)    
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getCloudFrac(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getDHI( const int siteId, const double fcstTime)    
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getDHI(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getDNI( const int siteId, const double fcstTime)    
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getDNI(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getElevation( const int siteId, const double fcstTime)    
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getElevation(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getGHI( const int siteId, const double fcstTime)    
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getGHI(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getKt( const int siteId, const double fcstTime)    
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getKt(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getMixingRatio( const int siteId, const double fcstTime)    
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getMixingRatio(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getPsfc( const int siteId, const double fcstTime)    
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getPsfc(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getRh( const int siteId, const double fcstTime)    
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getRh(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getTauQcTot( const int siteId, const double fcstTime)    
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getTauQcTot(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getTauQiTot( const int siteId, const double fcstTime)    
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getTauQiTot(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getTauQs( const int siteId, const double fcstTime)    
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getTauQs(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getTaod5502d(const int siteId, const double fcstTime)    
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getTaod5502d(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getToa( const int siteId, const double fcstTime)    
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getToa(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getTemp( const int siteId, const double fcstTime)    
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getTemp(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getWindDir( const int siteId, const double fcstTime)    
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getWindDir(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getWindSpeed( const int siteId, const double fcstTime)    
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getWindSpeed(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getWpTot( const int siteId, const double fcstTime)    
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getWpTot(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getWvp( const int siteId, const double fcstTime)    
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getWvp(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getWrfKt2( const int siteId, const double fcstTime)
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getWrfKt2(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}

const float NwpMgr::getWrfToa2( const int siteId, const double fcstTime)
{
  int i = getNwpFileIndex(fcstTime);

  if(i >= 0)
  {
    return _nwpFiles[i]->getWrfToa2(siteId, fcstTime);
  }
  else
  {
    return NwpReader::NWP_MISSING;
  }
}


