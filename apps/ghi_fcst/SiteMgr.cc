/**
 *
 * @file SiteMgr.cc
 * 
 *   Implementation for simple class that parses site IDs and locations file and serves
 *   that information to the ForecastProcessor
 * 
 **/



// Include files 
#include <string>
#include <fstream>
#include <sstream>
#include <utility>
#include <iostream>

using std::cerr;
using std::endl;
using std::string;
using std::ifstream;
using std::istream;
using std::getline;
using std::stringstream;

#include "SiteMgr.hh"

// Constant and macros
   
// Types, structures and classes

// Global variables 

// Functions


SiteMgr::SiteMgr(const string siteFile)
{
   siteIdFile = siteFile; 
}

int SiteMgr::parse()
{
   //
   // Open file
   //
   ifstream infile( siteIdFile); 

   if(!infile.is_open())
   {
      cerr << "Cannot open " << siteIdFile.c_str() << "\n"; 
      return 1;
   }

   string delimiter(",");

   while (infile)
   {
      string line;
      
      //
      // Get a line from the input file
      //  
      getline(infile, line);

      //
      // Skip comment lines or empty lines
      //
      if (!line.empty() && line.find("#") == string::npos)
      {
         // 
         // Sample lines :
         // ADDI, 1
         // ANDE, 2 ....
         //
         // Get the string 
         //
         string name = line.substr(0, line.find(delimiter));

         siteNames.push_back(name);

         //
         // Get the site integer ID and convert string to int and store
         //
         int siteNum = stoi(line.substr(line.find(delimiter)+1));

         siteIds.push_back(siteNum);
      }
   }
   //
   // Check that we have sites and that the number of site call letters
   // and integer IDs is the same. 
   //
   if ( siteIds.size() > 0 && ((int) (siteIds.size()) == (int)(siteNames.size())))
      return 0;
   else
      return 1;
}
