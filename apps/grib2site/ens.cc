/*
 *   Copyright 1995, University Corporation for Atmospheric Research
 *   See ../COPYRIGHT file for copying and redistribution conditions.
 */

#include <stdlib.h>			/* for free(), ... */
#include <stdio.h>                      /* for printf().. */
#include "emalloc.h"
#include "gribtypes.h"
#include "centers.h"
#include "ens.h"


/*
 * Make ensemble data structure from local-use GRIB extension data.
 * Returns 0 if memory cannot be allocated. User should call free_ens()
 * when done with it.
 *
 */
ens*
mkens_from_grib(int center, unsigned char *local)
{
    ens *ret = (ens *) emalloc(sizeof(ens));

    int lcode = g1i(local[0]);

    switch (center)
      {
      case CENTER_ECMWF:
	switch (lcode)  // Local definition code
	  {
	  case 1:
	  case 2:
	  case 5:
	  case 18:
	  case 26:
	  case 30:
	  case 36:
	    ret->member_num = g1i(local[9]);
	    ret->total_members = g1i(local[10]);
	    if (ret->member_num == 0)
	      ret->is_control = 1;
	    else
	      ret->is_control = 0;
	    break;
	  default:
	    free(ret);
	    ret = 0;
	  }
	break;
      case CENTER_NMC:     // not tested
	if (g1i(local[1]) == 1)
	  ret->is_control = 1;
	else
	  ret->is_control = 0;
	ret->member_num = g1i(local[2]);
	ret->total_members = 0;  // can we determine total?
	break;
      default:
	free(ret);
	ret = 0;
      }

    return ret;
}


/*
 * Make ensemble data structure from GRIB2 product definition template data.
 * Returns 0 if memory cannot be allocated. User should call free_ens()
 * when done with it.
 *
 * (NB: has not been testing with very many examples.)
 *
 * Note: NCEPs ensemble files don't seem to add up. For the GEFS, there
 * are 20 members, all allegedly positve perturbations, but the total
 * number is declared as 10. We just leave it as-is. (JC)
 *
 */
ens*
mkens_from_grib2(GRIB2::g2int ipdtnum, GRIB2::g2int *igdtmpl)
{
    ens *ret = (ens *) emalloc(sizeof(ens));

    switch (ipdtnum)
      {
      case 1:
      case 11:
	ret->member_num = igdtmpl[16];
	ret->total_members = igdtmpl[17];
	if (igdtmpl[15] <= 1)
	  ret->is_control = 1;
	else
	  ret->is_control = 0;
	break;
      default:
	free(ret);
	ret = 0;
	break;
      }
    return ret;
}

void
free_ens(		/* free ensemble data structure */
    ens* en
	)
{
    if(en)
	free(en);
}
