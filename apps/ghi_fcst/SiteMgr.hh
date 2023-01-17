/**
 *
 *  @file SiteMgr.hh
 *  @class SiteMgr 
 *  @brief Class for parsing site IDs and feeding that data to the forecast 
 *         processor. 
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
   * Constructor 
   * @param[in] siteFile  Path to site config file
   */
  SiteMgr(const string siteFile);

  /**
   * Parse the site file: record the site call letters and integer ids.
   * Store data in vectors
   */
  int parse();

  /**
   * Get the number of sites in the site config file
   * @return number of sites to be processed (length of siteId vector)
   */
  const int getNumSites() {return (int)siteIds.size(); }

  /**
   * Get the ith sitesID 
   * @return integer siteID
   */
  const int getSiteId(const int i) const {return siteIds[i];}

  /**
   * Get the ith site call letters
   * @return site call letters string 
   */
  const string getSiteName(const int i ) const { return siteNames[i]; }
 
  
private:
  /**
   * Path of file containing site call letters and integer IDs 
   */ 
  string siteIdFile;

  /**
   * Integer IDs of sites
   */
  vector<int> siteIds;

  /**
   * Call letters of each site 
   */
  vector<string> siteNames;
  
};

#endif /* SITEMGR_HH */
