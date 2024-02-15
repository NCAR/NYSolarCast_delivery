/*
 *	Copyright 1993, University Corporation for Atmospheric Research.
 */

/* 
 * Decodes GRIB products into netCDF files. This version decodes GRIB 1 or 2
 * products into a point-based netcdf file using the list of sites provided
 * on input. This is a copy of gribtonc with appropriate changes made to add
 * GRIB 2 decoding, and to output values at site locations rather than on a
 * grid.
 *
 * Jim Cowie/RAL 12/09/05
 * Jim Cowie/RAL 10/21/06 Modified to handle grid "tiles".
 * Jim Cowie/RAL 12/14/06 Modified to unpack only the grids we need
 *
 */

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>		/* _POSIX_PATH_MAX */
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "mkdirs_open.h"

#include "nc.h"
#include "centers.h"
#include "models.h"
#include "params.h"
#include "levels.h"
#include "timeunits.h"
#include "grib1.h"
#include "product_data.h"
#include "quasi.h"
#include "units.h"
#include "site_list.h"
#include "log/log.hh"

#ifdef NO_ATEXIT
#include <atexit.h>
#endif


extern void print_grib(product_data *gp, int prec);
extern void print_grib_line(product_data *gp);


// Global thingies.

unsigned long num_wmo_messages; /* for statistics on exit */
unsigned long num_gribs_unpacked;
unsigned long num_gribs_written;
unsigned long num_degribbed;
int listing;
Log *logFile;         // log object
int match_filetime;   // to force the data reftime to match the filename


/*
 * Timeout in seconds.  If no input is received for this interval, process
 * closes output file and exits.
 */
#define DEFAULT_TIMEOUT  600


#define DEFAULT_PRECISION 7


/*
 * Called at exit.
 * This callback routine registered by atexit().
 */
static void
cleanup()
{

    logFile->write_time("Info: %lu GRIB msgs, %lu fields unpacked, %lu written\n",
	  num_wmo_messages, num_gribs_unpacked, num_gribs_written);
    //if (!listing)
    //nccleanup();		/* close open netCDF files, if any */

    logFile->write_time("Ending.\n");

    if (logFile)
      delete logFile;

    utTerm();
}


/*
 * Called upon receipt of signals.
 * This callback routine registered in set_sigactions() .
 */
static void
signal_handler(
     int sig
	)
{
#ifdef SVR3SIGNALS
    /* 
     * Some systems reset handler to SIG_DFL upon entry to handler.
     * In that case, we reregister our handler.
     */
    (void) signal(sig, signal_handler) ;
#endif
    switch(sig) {
      case SIGHUP :
	logFile->write_time("Info: SIGHUP\n") ;
	return ;
      case SIGINT :
	logFile->write_time("Info: Interrupt\n") ;
	exit(0) ;
      case SIGTERM :
	logFile->write_time("Info: SIGTERM\n") ;
	exit(0) ;
      case SIGUSR1 :
	logFile->write_time("Info: SIGUSR1\n") ;
	return ;
      case SIGUSR2 :
	//if (toggleulogpri(LOG_INFO))
	  logFile->write_time("Info: Going verbose\n") ;
	//else
	  logFile->write_time("Info: Going silent\n") ;
	return ;
      case SIGPIPE :
	logFile->write_time("Info: SIGPIPE\n") ;
	exit(0) ;
    }
    logFile->write_time("Info: signal_handler: unhandled signal: %d\n", sig) ;
}


static void
set_sigactions()
{
#ifndef NO_POSIXSIGNALS
    struct sigaction sigact ;
    
    sigact.sa_handler = signal_handler;
    sigemptyset(&sigact.sa_mask) ;
    sigact.sa_flags = 0 ;
#ifdef SA_RESTART	/* SVR4, 4.3+ BSD */
    /* usually, restart system calls */
    sigact.sa_flags |= SA_RESTART ;
#endif
    
    (void) sigaction(SIGHUP, &sigact, NULL) ;
    (void) sigaction(SIGINT, &sigact, NULL) ;
    (void) sigaction(SIGTERM, &sigact, NULL) ;
    (void) sigaction(SIGUSR1, &sigact, NULL) ;
    (void) sigaction(SIGUSR2, &sigact, NULL) ;
    (void) sigaction(SIGPIPE, &sigact, NULL) ;
#else
    (void) signal(SIGHUP, signal_handler) ;
    (void) signal(SIGINT, signal_handler) ;
    (void) signal(SIGTERM, signal_handler) ;
    (void) signal(SIGUSR1, signal_handler) ;
    (void) signal(SIGUSR2, signal_handler) ;
    (void) signal(SIGPIPE, signal_handler) ;
#endif
}


static void
usage(
      char *av0  /* arg list */
      )
{
  fprintf(stderr,
	  "Usage: %s [options] [CDL_file site_file netCDF_file] < GRIB_file(s)\n", av0);
  fprintf(stderr,
	  "Options:\n");
  fprintf(stderr,
	  "-b\t\twrite brief product information to stdout (no netcdf output)\n") ;
  fprintf(stderr,
	  "-h\t\twrite header information to stdout (no netcdf output)\n") ;
  fprintf(stderr,
	  "-f\t\twrite full header and data to stdout (no netcdf output)\n") ;
  fprintf(stderr,
	  "-d debug_level\tlog at a higher debug level\n") ;
  fprintf(stderr,
	  "-l logbase\tbase name of log file to use (default = stdout)\n") ;
  fprintf(stderr,
	  "-m\t\tdo not force model reftime to match the date/time found in the output filename\n") ;
  fprintf(stderr,
	  "-t timeout\tif no input, exit after \"timeout\" seconds (default %d)\n",
	  DEFAULT_TIMEOUT) ;
  fprintf(stderr,
	  "-e errfile\tappend bad GRIB products to this file\n") ;
  fprintf(stderr,
	  "CDL_file\tCDL template, when netCDF output file does not exist\n") ;
  fprintf(stderr,
	  "site_file\tfile containing list of site locations\n") ;
  fprintf(stderr,
	  "netCDF_file\tnetCDF output file\n") ;
  fprintf(stderr,
	  "GRIB_file(s)\tGRIB data on standard input\n") ;
  fprintf(stderr,
	  "\nThis application decodes GRIB version 1 or 2 messages supplied on stdin.\n");
  fprintf(stderr,
	  "If a brief or full listing of products is desired, use the -b or -f options.\n");
    fprintf(stderr,
	  "Use of these options suspends the netcdf output, hence CDL_file, site_file\n");
  fprintf(stderr,
	  "and netCDF_file (if present) are ignored. If -b and -f are not used, GRIB\n");
  fprintf(stderr,
	  "messages are decoded, values are extracted at the locations specified in\n");
  fprintf(stderr,
	  "site_file, and output is written to netCDF_file. If the netCDF file exists,\n");
  fprintf(stderr,
	  "decoded GRIB data are added to it and CDL_file and site_file are ignored.\n");
  fprintf(stderr, "Grid tiles are supported since only data for sites on the tile are updated.\n");

  exit(2);
}


/*
 * Parse raw product bytes into product_data structure.  Returns 0 if
 * failed.  User should call free_product_data() on result when done with
 * it.  Also expands quasi-regular grids to full rectangular grids if qmeth
 * is non-null. Can handle grib 1 or 2 products. Grib 2 messages may contain
 * multiple fields. Field_num is used to specify which field to return, and
 * is also used to flag that the last of the fields has been returned.
 * The caller can choose to unpack the grid data or not.
 */
static product_data *
grib_decode(
     prod *prodp,		/* input raw product bytes */
     quas *quasp,		/* if non-null, method used to expand
				   quasi-regular "grids" */
     int *field_num,            /* On input, the field number to process.
				   On output, the value is set to 0 if the
				   requested field was the last one available.
				   This is really only needed for grib 2
				   products since there is only one field in
				   a grib 1 product. */
     int unpack                 /* flag indicating whether data should be
				   unpacked or not (1=yes, 0=no). There are
				   situations where we don't need to unpack
				   all the data and this saves time. */
     )
{

  product_data *pdp = 0;

  // Determine the GRIB edition so that we can handle them differently
  int grib_edition = *(prodp->bytes+7);
  
  switch (grib_edition)
    {
    case 0:
    case 1:
      {
	*field_num = 0;               /* No more fields for grib1 */
      
	grib1 *gp = new_grib1(prodp); /* overlay raw bits on raw grib1 struct */
	if (gp == 0)
	  break;

	pdp = new_grib1_pdata(gp, unpack); /* compute cooked product structure, with
					 GDS (manufactured, if necessary) and
					 bytemap */
	free_grib1(gp);	
	
	if (pdp && pdp->gd->quasi && quasp) {
	  int ret = expand_quasi(quasp, pdp) ; /* Changes *pdp */
	  if (!ret)
	    logFile->write_time("Error: can't expand quasi-regular grid\n");
	}
	break;
      }
    case 2:
      {
	GRIB2::g2int sec0[3], sec1[13], nlocal, nfields, ierr, expand;
	GRIB2::gribfield *g2fld;
	
	ierr = GRIB2::g2_info(prodp->bytes, sec0, sec1, &nfields, &nlocal);
	if (ierr != 0 || *field_num > nfields) {
	  *field_num = 0;
	  break;
	}
	expand = unpack;
	ierr = GRIB2::g2_getfld(prodp->bytes, *field_num, unpack, expand, &g2fld);
	if (ierr != 0) {
	  *field_num = 0;
	  break;
	}

	pdp = new_grib2_pdata(prodp->id, g2fld);
	
	GRIB2::g2_free(g2fld);
	
	if (*field_num == nfields)
	  *field_num = 0;
	
	break;
      }
    }
  
  return pdp;
}


static int
do_nc (
    FILE *ep,			/* if non-null, where to append bad GRIBs */
    int timeout,		/* exit if no data in this many seconds */
    quas *quasp,		/* if non-null, specification for how
				   quasi-regular "grids" are to be expanded */
    char *cdlname,		/* Pathname of CDL template file to be used to
				   create netCDF file, if it doesn't exist */
    char *sitename,		/* Pathname of site list file */
    char *ncname		/* Pathname of netCDF output file */
    )
{
    struct prod the_prod;	/* raw bits of GRIB message, length, id */
    struct product_data *gribp;	/* decoded GRIB product structure */
    float *lat_arr, *lon_arr;	/* arrays of lat/lon locations */
    FILE *fp = stdin;		/* input */
    ncfile *ncp = 0;
    int ncid = 0;
    int ret;
    int num_sites;
    int field_num, last_field;
    int unpack;


    if (!listing) {
      ncid = cdl_netcdf(cdlname, ncname);	/* get netCDF file handle */
      if (ncid == -1) {
	logFile->write_time("Error: can't create output netCDF file %s\n", 
			    ncname);
	return(1);
      }
      setncid(ncid);	/* store ncid so can be closed if interrupt */
    }

    num_wmo_messages = 0;
    num_gribs_unpacked = 0;

    if (init_udunits() != 0) {
	logFile->write_time("Error: can't initialize udunits library\n");
	return(1);
    }

    // Set up netcdf output file
    if (!listing) {
      ncp = new_ncfile(ncname);
      if (!ncp) {
	logFile->write_time("Error: can't create output netCDF file %s\n", ncname);
	return(1);
      }

      /* Process site file */
      if (!(process_sites(sitename, ncid, &lat_arr, &lon_arr, &num_sites))) {
	return(1);
      }
    }
    else if (listing == 1) {
      printf("grb cnt mdl grd prm    lvlf  lev1 lev2  trf tr0 tr1  pack bms gds   npts header\n");
    }

    while(1) {			/* usual exit is timeout in get_prod() */
	int bytes = get_prod(fp, timeout, &the_prod);
	if (bytes == 0)
	  break;
	else if (bytes < 0)
	  return(1);	  
	else
	  num_wmo_messages++;
	
	field_num = 1;
	while(field_num > 0) {

	  unpack = 0;
	  if (listing >= 2) unpack = 1; // Unpack full listing only

	  /* decode message into a grib product structure */
	  last_field = field_num;
	  gribp = grib_decode(&the_prod, quasp, &field_num, unpack);


	  // GRIB message will either be written to an "error file",
	  // listed to stdout, or written to a netcdf file.

	  /* Write 'bad' products to error file */
	  if (gribp == 0) {
	    if (ep && fwrite(the_prod.bytes, the_prod.len, 1, ep) == 0) {
	      logFile->write_time(1, "Info: writing bad GRIB to error file\n");
	    }
	  }

	  /* Dump brief or full grib info to stdout */
	  else if(listing == 1) {
            print_grib_line(gribp);
	  }
	  else if (listing == 2) {
            print_grib(gribp, -1);
	  }
	  else if (listing == 3) {
            print_grib(gribp, DEFAULT_PRECISION);
	  }

	  /* Write to netcdf file. First check that the variable exists in
	     the netcdf file, then unpack the product data and store it. */
	  else if (nc_check(gribp, ncp) == 0) {
	      free_product_data(gribp);
	      gribp = grib_decode(&the_prod, quasp, &last_field, 1);
	      ret = nc_write(gribp, ncp, lat_arr, lon_arr, num_sites);
	      if (ret < 0)
		return (1);
	      num_gribs_written = num_gribs_written + ret;
	      num_gribs_unpacked++;
	  }

	  
	  free_product_data(gribp);

	  /* If more fields are available, increment field_num */
	  if (field_num > 0)
	    field_num++;

	}

	if (the_prod.id)
	  free(the_prod.id);

    }

    if (!listing) {
      free(lat_arr);
      free(lon_arr);
      if (ncp != 0)
	free_ncfile(ncp);
      // close nc file
      if (ncid != 0)
	nc_close(ncid);
    }

    /* free the product buffer */
    get_prod(0, timeout, &the_prod);

    return(0);
}


/*
 * Reads GRIB data from standard input.
 * GRIB data may be contained in WMO envelope or not.
 * Decodes GRIB data to in-memory structure.
 * Writes decoded data that conforms to one specified netCDF output file.
 * May be used as an in-line LDM decoder, invoked by pqact(1).
 */
int
main(
     int ac,
     char *av[]
     )
{
    char *logfname = 0 ;	/* log file name, default uses syslogd */
    char *ofile = 0 ;		/* output netCDF file name */
    char *cdlfile = 0 ;		/* CDL template file name */
    char *sitefile = 0 ;	/* site list file name */
    FILE *ep = 0;		/* file handle for bad GRIBS output, when
				   -e badfname used */
    int timeo = DEFAULT_TIMEOUT ; /* timeout */
    quas *quasp = 0;		/* default, don't expand quasi-regular grids */

    int debugLevel = 0;
    int ret;

    
    {
	extern int optind;
	extern int opterr;
	extern char *optarg;
	int ch;
	int errflg = 0;

	listing = 0;
	match_filetime = 1;
	
	opterr = 1;
	
	while ((ch = getopt(ac, av, "bhfd:l:t:me:")) != EOF) {
	    switch (ch) {
	    case 'b':
		listing = 1;
		break;
	    case 'h':
		listing = 2;
		break;
	    case 'f':
		listing = 3;
		break;
	    case 'd':
		debugLevel = atoi(optarg);
		break;
	    case 'l':
		logfname = optarg ;
		break;
	    case 't':
		timeo = atoi(optarg) ;
		if(timeo < 1) {
		    fprintf(stderr, "%s: invalid timeout %s",
			    av[0], optarg) ;
		    errflg++;
		}
		break;
	    case 'm':
	        match_filetime = 0;
		break;
	    case 'e':
		ep = fopen(optarg, "w");
		if(!ep) {
		  //serror("can't open %s", optarg);
		    errflg++;
		}
		break;
	    case 'q':
		quasp = qmeth_parse(optarg);
		if(!quasp) {
		    fprintf(stderr,
			    "%s: invalid quasi-regular expansion method %s",
			    av[0], optarg) ;
		    errflg++;
		}
		break;
	    case '?':
		errflg++;
		break;
	    }
	}
	
	// If no listings requested, get command line args
	if (listing == 0) {
	  
	  if ((ac - optind) == 3) {
	    cdlfile = av[optind] ;
	    sitefile = av[optind+1] ;
	    ofile = av[optind+2] ;
	  }
	  else {
	    errflg++;
	  }
	}

	if(errflg)
	  usage(av[0]);

    }


    /*
     * register exit handler
     */
    if(atexit(cleanup) != 0)
      {
	//serror("atexit") ;
	  exit(1) ;
      }
    
    /*
     * set up signal handlers
     */
    set_sigactions() ;
        
    /*
     * initialize logger
     */
    if (logfname)
      logFile = new Log(logfname);
    else
      logFile = new Log("");

    logFile->set_debug(debugLevel);

    logFile->write_time("Starting %s\n", av[0]) ;

    ret = do_nc(ep, timeo, quasp, cdlfile, sitefile, ofile);

    exit(ret);
    
}
