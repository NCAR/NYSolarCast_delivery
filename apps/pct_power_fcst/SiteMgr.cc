/**
 *
 * @file SiteMgr.cc
 * 
 *   Implementation for class that parses site IDs serves
 *   that infomtion
 */


// Include files 
#include <stdexcept>
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
         int start = 0; 
         int pos;
         //
         // If csv file, find all siteIds before delimiter  
         //
         while ( (pos = line.find(delimiter, start)) != string::npos )
         {
           //
           // Convert string to int and store
           //
           try {
              int siteNum = stoi(line.substr(start, pos));
              siteIds.push_back(siteNum);
              if ( start < line.length()-1)
                 start = pos + 1;
           }
           catch (std::invalid_argument)
           { 
           } 
 
        }
        //
        // Get the last element without a delimiter after if there is one
        //
        if ( !line.substr(start).empty())
        {
           //
           // Convert string to int and store
           //
           try {
              int siteNum = stoi(line.substr(start));

              siteIds.push_back(siteNum);
           }
           catch (std::invalid_argument) {}
        }  
     }
   }
   if ( siteIds.size() > 0 )
   {
      return 0;
   }
   else
   {
      return 1;
   }
}
