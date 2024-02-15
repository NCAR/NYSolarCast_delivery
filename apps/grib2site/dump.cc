/*
 *	Copyright 1993, University Corporation for Atmospheric Research.
 */
/* $Id: dump.cc,v 1.6 2015/10/08 14:56:09 dicast Exp $ */

/* 
 * decodes GRIB products into ASCII
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "centers.h"
#include "models.h"
#include "params.h"
#include "levels.h"
#include "timeunits.h"
#include "grib1.h"
#include "product_data.h"
#include "quasi.h"

static void
print_floats(float *ff,			/* array of floats to print */
	     int cols, 			/* number of columns of data */
	     int rows, 			/* number of rows of data */
	     int pp			/* precision to use in printing */
	     )
{
    int ii;
    int jj;
    int per_line = 8;
    static char format[] = "%.10g%s";
    
    sprintf(format, "%%.%dg%%s", pp);

    
    for(ii = 0; ii < rows; ii++) {
	printf("Row %d:\n\t",ii);

	for(jj = 0; jj < cols;jj++)
	    printf(format, ff[ii*cols+jj],
		   jj%per_line+1 == per_line ? "\n\t" : " ");

	if ((jj-1)%per_line+1 != per_line)
	    printf("\n");
    }
}


static int
is_ixg(			/* is an international exchange grid? */
     int id
     )
{
    switch (id) {
      case 21:
      case 22:
      case 23:
      case 24:
      case 25:
      case 26:
      case 37:
      case 38:
      case 39:
      case 40:
      case 41:
      case 42:
      case 43:
      case 44:
      case 50:
      case 61:
      case 62:
      case 63:
      case 64:
	return 1;
    }
    /* default: */
    return 0;
}

void
print_grib(
	   product_data *gp,
	   int prec			/* precision to use for float vals */
	   )
{

    char format[80];

    printf("-----------------------------------------------------\n");

    printf("   %24s : %s\n","    Header", gp->header);
    printf("   %24s : %d\n","GRIB Edition", gp->edition);
    printf("   %24s : %d (%s)\n","Originating Center", gp->center,
	   centername(gp->center));
    if(gp->subcenter != 0)
    printf("   %33s : %d (%s)\n","Subcenter", gp->subcenter,
	   subcentername(gp->center, gp->subcenter));
    printf("   %24s : %d (%s)\n","Process", gp->model,
	   modelname(gp->center,gp->model));
    printf("   %24s : %d\n","Grid", gp->grid);
    printf("             Points in grid : %d\n", gp->npts);
    printf("   %24s : %d\n","Parameter Table Ver", gp->param_vers);
    printf("   %24s : %d (%s)\n","Parameter", gp->param,
	   grib_pname(gp->param));
    printf("   %24s : %s\n", "Units", grib_units(gp->param));
    printf("   %24s : %s\n", "Level Type", levelname(gp->level_flg));
    switch (gp->level_flg) {
      case LEVEL_SURFACE:
      case LEVEL_CLOUD_BASE:
      case LEVEL_CLOUD_TOP:
      case LEVEL_CEILING:
      case LEVEL_ISOTHERM:
      case LEVEL_ADIABAT:
      case LEVEL_MAX_WIND:
      case LEVEL_TROP:
      case LEVEL_MEAN_SEA:
	break;
      case LEVEL_FH:
      case LEVEL_FHG:
	printf("   %24s : %f (m)\n", "Level", mblev(gp->level));
	break;
      case LEVEL_FHGH:
	printf("   %24s : %f (cm)\n", "Level", mblev(gp->level));
	break;
      case LEVEL_SIGMA:
	printf("   %24s : %f\n", "Level", (256*gp->level[0]+gp->level[1])/10000.0);
	break;
      case LEVEL_DBS:
      case LEVEL_Bls:
      case LEVEL_ISOBARIC:
	printf("   %24s : %f (Pa)\n", "Level", 100.*mblev(gp->level));
	break;
      case LEVEL_LISO:
      case LEVEL_LFHM:
      case LEVEL_LFHG:
      case LEVEL_LS:
      case LEVEL_LBls:
      case LEVEL_LISH:
      case LEVEL_LSH:
      case LEVEL_LISM:
	printf("   %24s : %f (Pa)\n", "Level 1", gp->level[0]*1000.0);
	printf("   %24s : %f (Pa)\n", "Level 2", gp->level[1]*1000.0);
	break;
    }
    printf("   %24s : %04d/%02d/%02d:%02d:%02d\n","Reference Time", 
				/* century 21 doesn't start until 2001 */
	   gp->year+(gp->century - (gp->year==0 ? 0 : 1))*100,
	   gp->month, gp->day, gp->hour, 
	   gp->minute);
    printf("   %24s : %s\n", "Time Unit", tunitsname(gp->tunit));
    printf("   %24s : %s\n", "Time Range Indicator", triname(gp->tr_flg));
    switch (gp->tr_flg) {
      case TRI_P1:
      case TRI_IAP:
	printf("   %24s : %d\n", "Time 1 (P1)", gp->tr[0]);
	break; 
      case TRI_P12:
      case TRI_Ave:
      case TRI_Acc:
      case TRI_Dif:
      case TRI_LP1:
      case TRI_AvgN:
      case TRI_AvgN1:
      case TRI_AccN1:
      case TRI_AvgN2:
      case TRI_AvgN3:
      case TRI_AccN3:
	printf("   %24s : %d\n", "Time 1 (P1)", gp->tr[0]);
	printf("   %24s : %d\n", "Time 2 (P2)", gp->tr[1]);
	break; 
    }
    if (gp->edition <2) {
      if (gp->bd->is_not_simple == 0)
	printf("   %24s : %s\n", "Packing", "simple");
      else
	printf("   %24s : %s\n", "Packing", "complex or second order");
      printf("   %24s : %d\n", "Decimal Scale Factor", gp->scale10);
      printf("   %24s : %d\n", "Binary Scale Factor", gp->bd->bscale);
      printf("   %24s : %f\n", "Reference Value", gp->bd->ref);
      {				/* all done in float to reproduce FSL value */
	float dscal;
	dscal = gp->scale10;
	dscal = pow(10.0, dscal);
	//sprintf(format, "   %%24s : %%.%dg\n", prec);
	sprintf(format, "   %%24s : %%.%dg\n", 7);
	printf(format, "Minimum Value", (float)(gp->bd->ref / dscal));
      }
      printf("   %24s : %d\n", "Number of Bits", gp->bd->nbits);
    }
    else {
      printf("   %24s : %d\n", "Data Packing Code", gp->bits);
    }
    printf("   %24s : %s\n", "BMS Included", gp->has_bms ? "TRUE" : "FALSE");
    printf("   %24s : %s\n", "GDS Included", gp->has_gds ? "TRUE" : "FALSE");
    printf("   %24s : %s\n", "IsInternationalGrid",
	   is_ixg(gp->grid) ? "TRUE" : "FALSE");
    print_gdes(gp->gd);		/* print Grid Description Section */

    if (prec >= 0) {
      // Dump data values
      printf("                 grid values:\n");
      print_floats(gp->data, gp->cols, gp->npts/gp->cols, prec);
    }

}     


void
print_grib_line(product_data *gp)
{
    char *lev;			/* abbreviation for level flag */
    
    lev = levelsuffix(gp->level_flg);

    if (lev) {
	printf("%3d%4d%4d%4d%4d ",
	       gp->edition, gp->center, gp->model, gp->grid, gp->param);
    	
	if (lev[0] == '\0')
	  lev = (char *)"isob";	/* show isobaric levels explicitly */
	printf("%7s %5d %4d", lev, level1(gp->level_flg, gp->level),
	       level2(gp->level_flg, gp->level));
	
	printf(" %4d%4d%4d %5d%4d%4d %6d %s\n",
	       gp->tr_flg, gp->tr[0], gp->tr[1],
	       gp->bits, gp->has_bms, gp->has_gds, gp->npts, gp->header);
    }
}     
