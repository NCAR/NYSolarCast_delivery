//==============================================================================
//
//   (c) Copyright, 2009 University Corporation for Atmospheric Research (UCAR).
//       All rights reserved. 
//
//       File: $RCSfile: MainGHIFcst.cc,v $
//       Version: $Revision: 1.1 $  Dated: $Date: 2014/10/15 13:55:20 $
//
//==============================================================================

/**
 *
 * @file main.cc
 *
 * Main program for pct_power_fcst 
 *
 */

// Include files 
#include <log/log.hh>
#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>
#include "Arguments.hh"
#include "FcstProcessor.hh"

//
// Global variables for debugging and logging
//
int DebugLevel = 0;

Log *Logg;

//
// Method called when new fails
//
void out_of_store()
{
  int exit_status = 1;

  Logg->write_time("Error: out of store.\n");
 
  Logg->write_time_ending(exit_status);

  exit(exit_status);
}

int main(int argc, char **argv)
{
  //
  // Get command line arguments and store in args
  //
  Arguments args(argc, argv);
  
  if (args.error != string(""))
  {
     fprintf(stderr, "Error: command line arguments problem: %s\n", args.error.c_str());
 
     return 2;
  }

  //
  // Set global Debug_level from args
  //
  DebugLevel = args.debugLevel;

  //
  // Set up logging global Logg
  //
  Logg = new Log(args.logDir.c_str());
 
  Logg->write_time_starting(args.programName.c_str());

  Logg->write_time("Info: executed: %s\n", args.commandString.c_str());

  if (args.error != "")
  {
     Logg->write_time("Error: %s\n", args.error.c_str());

     Logg->write_time_ending(1);
  
     return 1;
  }
  
  //
  // Initialize forecast processing object
  //
  FcstProcessor  fcstProcessor(args);

  // 
  // Check for successful initialization
  //
  if (fcstProcessor.error != string(""))
  {
     Logg->write_time("Error: process initialization failed, %s\n", fcstProcessor.error.c_str());

     Logg->write_time_ending(1);

     return 1;
  }

  //
  // Run forecast processor
  //
  if (fcstProcessor.run() > 0)
  {
     Logg->write_time("Error: processing failed\n");

     Logg->write_time_ending(1);

     return 1;
  }

  //
  // That's all folks!
  // 
  Logg->write_time_ending(0);

  delete Logg;

  return 0;
}
