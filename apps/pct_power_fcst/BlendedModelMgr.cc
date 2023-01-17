//==============================================================================
//
//   (c) Copyright, 2009 University Corporation for Atmospheric Research (UCAR).
//       All rights reserved.
//       Do not copy or distribute without authorization.
//
//       File: $RCSfile: BlendedModelMgr.cc,v $
//       Version: $Revision: 1.1 $  Dated: $Date: 2014/10/15 13:55:20 $
//
//==============================================================================

/**
 *
 * @file BlendedModelMgr.cc  Source code for BlendedModelMgr class
 *
 * @date 07/28/2021
 *
 */

// Include files 

#include <iostream>
#include "BlendedModelMgr.hh"

using std::cerr;
using std::endl;

BlendedModelMgr::BlendedModelMgr()
{
 
}

BlendedModelMgr::BlendedModelMgr( BlendedModelReader *blendedModelFile)
{
  _modelFiles.push_back(blendedModelFile);
}

BlendedModelMgr::~BlendedModelMgr()
{
  //
  // cleanup
  //
  for (int i = 0; i <  (int)_modelFiles.size(); i++)
    delete  _modelFiles[i];

  _modelFiles.erase( _modelFiles.begin(), _modelFiles.end());
}

void BlendedModelMgr::add( BlendedModelReader *modelFile)
{ //
  // Push files on to vector in creation time order
  //
  if ( _modelFiles.empty() )
  {
    _modelFiles.push_back(modelFile);
  }
  else
  {
    int i = 0;
    while ( i < (int) _modelFiles.size() && 
	    modelFile->getGenTime() < _modelFiles[i]->getGenTime())
    {  
      i++;
    }
    _modelFiles.insert(_modelFiles.begin() + i, modelFile);
  }
}

const int BlendedModelMgr::getBlendedModelFileIndex(const double fcstTime) const
{
  //
  // Files are ordered by creation time, with first file in vector
  // having the most recent creation time. Several files may have data values at
  // the input fcstTime but the file index of the file with the most recent
  // creation time is returned. If a file is not found then -1 is returned.
  //
  bool gotIndex = false;

  int index = 0;

  while( !gotIndex && index < (int)_modelFiles.size())
  {
    gotIndex =  _modelFiles[index]->haveData(fcstTime);
    if(!gotIndex)
    {
      index++;
    }
  }

  if (gotIndex && index < (int)_modelFiles.size() )
  {
    return index;
  }
  else
  {
    return -1;
  }                       
}

const int BlendedModelMgr::getClimateZone( const int siteId) 
{
  // 
  // climate zones are static across files, we will get the value from the first file
  //  
  return _modelFiles[0]->getClimateZone(siteId);
}

const float BlendedModelMgr::getGHI( const int siteId, const double fcstTime)    
{
  int i = getBlendedModelFileIndex(fcstTime);

  if(i >= 0)
  {
    return _modelFiles[i]->getGHI(siteId, fcstTime);
  }
  else
  {
    return BlendedModelReader::MISSING;
  }
}

const float BlendedModelMgr::getRh( const int siteId, const double fcstTime)    
{
  int i = getBlendedModelFileIndex(fcstTime);

  if(i >= 0)
  {
    return _modelFiles[i]->getRh(siteId, fcstTime);
  }
  else
  {
    return BlendedModelReader::MISSING;
  }
}

const float BlendedModelMgr::getTemp( const int siteId, const double fcstTime)    
{
  int i = getBlendedModelFileIndex(fcstTime);

  if(i >= 0)
  {
    return _modelFiles[i]->getTemp(siteId, fcstTime);
  }
  else
  {
    return BlendedModelReader::MISSING;
  }
}
