/* *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=* */
/* ** Copyright UCAR (c) 1992 - 2012 */
/* ** University Corporation for Atmospheric Research(UCAR) */
/* ** National Center for Atmospheric Research(NCAR) */
/* ** Research Applications Laboratory(RAL) */
/* ** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA */
/* ** 2012/9/18 16:58:54 */
/* *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=* */
/*
 * Module: test_log.cc
 *
 * Author: Gerry Wiener
 *
 * Date:   5/6/99
 *
 * Description:
 *     
 */

/* System include files / Local include files */

/* Constant definitions / Macro definitions / Type definitions */

/* External global variables / Non-static global variables / Static globals */

/* External functions / Internal global functions / Internal static functions */


#include <cstdio>
#include "../include/log/log.hh"
using namespace std;

main(int argc, char **argv)
{
  Log log("test_log"); 

  if (log.error())
    {
      printf("Error %s occurred.\n", log.error());
      return(-1);
    }

  {
    Log log_asst("asst_test");
    log_asst.write("asst testing 1, 2, 3\n");
    log_asst = log;
    log_asst.write("asst testing 1, 2, 3\n");

    Log log_asst1 = log;
    log_asst1.write("asst1 testing 1, 2, 3\n");

    Log log_asst2;
  }

  log.write("standard testing 1,2,3\n");

  log.write_time("test message with time\n");
  log.write("test message without time\n");

  Log log1("");
  log1.write_time("log1 test message with time\n");
  log1.write("log1 test message without time\n");

  Log log2("");
  log2.write_time("log2 test message with time\n");
  log2.write("log2 test message without time\n");

  Log log3 = log;
  log3.write_time("log3 test message with time\n");
  log3.write("log3 test message without time\n");
  
  Log log4;
  log4.write_time("log4 test message with time\n");
  log4.write("log4 test message without time\n");
  log4.set_debug(3);
  log4.write(-2, "dl = -2, debug_level = 3, log4 this should appear\n");
  log4.write(-1, "dl = -1, debug_level = 3, log4 this should appear\n");
  log4.write(0, "dl = 0, debug_level = 3, log4 this should appear\n");
  log4.write(1, "dl = 1, debug_level = 3, log4 this should appear\n");
  log4.write(2, "dl = 2, debug_level = 3, log4 this should appear\n");
  log4.write(3, "dl = 3, debug_level = 3, log4 this should appear\n");
  log4.write(4, "dl = 4, debug_level = 3, log4 this shouldn't appear\n");
  log4.set_debug(0);
  log4.write_time(4, "dl = 4, debug_level = 0, log4 this shouldn't appear\n");
  log4.write_time(0, "dl = 0, debug_level = 0, log4 this should appear\n");
  log4.write_time("no dl, log4 this should appear\n");
  
  Log log5;
  log5.set_debug_low(2);
  log5.set_debug(3);
  log5.write(1, "dl = 1, debug_low = 3, log5 this shouldn't appear\n");
  log5.write(2, "dl = 2, debug_low = 2, debug_high = 3, log5 this should appear\n");
  log5.write(3, "dl = 3, debug_low = 3, debug_high = 3, log5 this should appear\n");
  log5.write(4, "dl = 4, debug_low = 3, log5 this shouldn't appear\n");

  return(0);
}

