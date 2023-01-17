/**
 *
 *  @file SiteMgr.hh
 *  @class SiteMgr 
 *  @brief Class for parsing site ID configuration file. 
 */

#ifndef SITEMGR_HH
#define SITEMGR_HH

#include <string>
#include <vector>
#include <utility>

using std::string;
using std::vector;
using std::pair;
/** 
 * SiteMgrclass 
 */
class SiteMgr 
{
public:
  /** 
   *Constructor 
   */
  SiteMgr(const string siteFile);

  /**
   * Read in the site IDs to process
   */
  int parse();

  /**
   * Get the number of sites to process
   * @return integer number of sites in the siteId container 
   */
  const int getNumSites() {return (int)siteIds.size(); }

  /**
   * Get the ith siteID 
   * @param[in] integer index
   * @return integer siteID
   */ 
  const int getSiteId(const int i) const {return siteIds[i];}
  
private:
  
  /**
   * Path of siteId config file
   */
  string siteIdFile;

  /**
   * Container for siteIDs
   */
  vector<int> siteIds;
};

#endif /* SITEMGR_HH */
