//==============================================================================
//
//   (c) Copyright, 2009 University Corporation for Atmospheric Research (UCAR).
//       All rights reserved.
//
//       File: $RCSfile: Arguments.cc,v $
//       Version: $Revision: 1.1 $  Dated: $Date: 2014/10/15 13:55:20 $
//
//==============================================================================

/**
 * @file arguments.cc
 *   Implementation of class that parses command line arguments.
 */

// Include files 
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <unistd.h>
#include <iostream>
#include "Arguments.hh"

using std::ifstream;
using std::cerr;
using std::endl;

// Constant and macros

// Types, structures and classes

// Global variables 

// Functions

void get_command_string(int argc, char **argv, string &command_string)
{
  for (int i=0; i<argc-1; i++)
    {
      command_string += string(argv[i]) + string(" ");
    }

  command_string += string(argv[argc-1]);
}

Arguments::Arguments(int argc, char **argv)
{
  //
  // Output "help" comments if no command line args 
  //
  if  (argc == 1)
  {
    usage(argv[0]);

    exit(1);
  }

  //
  // Set some default variable values
  //
  fcstStartTime = -1;
  
  subsetFcst = false;

  error = "";

  logDir = "";

  debugLevel = 0;

  bool errflg = false;

  int c; 

  while ((c = getopt(argc, argv, "d:hl:m:o:s:t:")) != EOF)
    switch (c)
      {
      case 'd':
	debugLevel = atoi(optarg);
	break;

      case 'm':
         nwpFilesStr =  optarg;
         parseCommaDelimStr(nwpFilesStr, nwpFiles);	
         break;          	
      
      case 'h':
	usage(argv[0]);
	exit(2);

      case 'l':
	logDir = optarg;
	break;

      case 'o':
	obsFilesStr = optarg;
	parseCommaDelimStr(obsFilesStr, obsFiles);
	break;
  
      case 's':
        subsetFcst = true; 
        fcstLeadTimesMinsStr = optarg;
        parseCommaDelimStr(fcstLeadTimesMinsStr, fcstLeadsSubset); 
        break; 

      case 't':
        fcstStartTime = atol(optarg); 
        break;
 
      case '?':
	errflg = 1;
	break;
      }

  if (errflg)
    {
      error = "options error";

      return;
    }

  if (argc - optind < 6)
  {
    error = "There are not enough arguments. Arguments include: siteIdFile "
            " fcstLeadsDelta fcstLeadsNum cubistModelBaseName outputCdlFile "
            " outputDir";
    return;
  }

  if ( (int)obsFiles.size() == 0 || (int)nwpFiles.size() == 0)
  {
     error = "Input is empty for observations or NWP forecast files. "
             "Both are needed.";
     return;
  }
  
  programName = string(argv[0]);

  fcstLeadsDelta = atoi(argv[optind++]);

  fcstLeadsNum = atoi(argv[optind++]);

  siteIdFile = string(argv[optind++]);
 
  cubistModel = string(argv[optind++]);

  cdlFile = string(argv[optind++]);

  outputDir = string(argv[optind++]);

  get_command_string(argc, argv, commandString);
}

void Arguments::usage(char *programName)
{
  fprintf(stderr, "\n\nusage:  %s [options] "
                  "<fcstLeadsDelta(in minutes)> "
                  "<fcstLeadsNum(integer number of fcsts to process)> "
                  "<siteIdFile> "
                  "<cubistModelBaseName> "
                  "<outputCdlFile> "
                  "<outputDir>\n\n", programName);
  fprintf(stderr, "%s options:\n", programName);
  fprintf(stderr, "\t-d  <debug level>\n");
  fprintf(stderr, "\t-m <NWP model forecast files> (a comma delimited list)\n");
  fprintf(stderr, "\t-h  help\n");
  fprintf(stderr, "\t-l  <log direcotry>\n");
  fprintf(stderr, "\t-o  <meteorological observations file>\n");
  fprintf(stderr, "\t-s  <single forecast lead in minutes>\n");
  fprintf(stderr, "\t-t  <unix time of first forecast>\n"); 
}

void Arguments::print()
{
  fprintf(stderr, "  forecast leads delta %d\n", fcstLeadsDelta);
  fprintf(stderr, "  number of forecasts leads to be processed: %d\n", 
                     fcstLeadsNum);
  fprintf(stderr, "  statistical model base: %s\n",cubistModel.c_str());
  fprintf(stderr, "  cdlFile: %s\n", cdlFile.c_str());
  fprintf(stderr, "  outputDir:  %s\n", outputDir.c_str()); 
  
  if ((int) obsFiles.size() > 0)
  {
    fprintf(stderr, "  Observation files: \n");
    for (int i = 0; i < (int) obsFiles.size(); i++)
    {
       fprintf(stderr, "    Observation file %d: %s\n", i, obsFiles[i].c_str());
    }
  }
  
  if ((int) nwpFiles.size() > 0)
  {
    fprintf(stderr, "  NWP files: \n");
    for (int i = 0; i < (int) nwpFiles.size(); i++)
    {
       fprintf(stderr, "    NWP file %d: %s\n", i, nwpFiles[i].c_str());
    }

  }

  if (logDir != "")
    fprintf(stderr,"  logDir: %s\n", logDir.c_str());

}

void Arguments::parseCommaDelimStr(string &commaStr, vector <string> &vec)
{
  //
  // Parse comma delimited list of files and push on to vector of file strings
  //
  int pos;
  pos = commaStr.find(",");

  while ( (pos = commaStr.find(",")) != (int) std::string::npos )
  {
    vec.push_back(commaStr.substr(0,pos));
    
    commaStr =  commaStr.substr(pos+1);
  }
  // last file
  vec.push_back(string(commaStr));
}

void Arguments::parseCommaDelimStr(string &commaStr, vector <int> &vec)
{
  //
  // Parse comma delimited list of files and push on to vector of file strings
  //
  int pos;
  pos = commaStr.find(",");

  while ( (pos = commaStr.find(",")) != (int) std::string::npos )
  {
    vec.push_back(atoi(commaStr.substr(0,pos).c_str()));
    
    commaStr =  commaStr.substr(pos+1);
  }
  // last file
  vec.push_back(atoi(commaStr.c_str()));
}
