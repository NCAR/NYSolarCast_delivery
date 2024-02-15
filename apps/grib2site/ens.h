/*
 *   Copyright 1995, University Corporation for Atmospheric Research
 *   See ../COPYRIGHT file for copying and redistribution conditions.
 */

#ifndef ENS_H_
#define ENS_H_

namespace GRIB2 {
#include "grib2c/grib2_int.h"
}

typedef struct ens {
  int member_num;      /* Ensemble member number */
  int total_members;   /* Total number of members in the ensemble, including
                          control */
  int is_control;      /* 1 if the control run, 0 otherwise */
} ens;


#ifdef __cplusplus
extern "C" ens* mkens_from_grib(int center, unsigned char *loc); /* make GRIB ensemble data structure */
extern "C" ens* mkens_from_grib2(GRIB2::g2int igdtnum, GRIB2::g2int *igdtmpl); /* make GRIB2 ensemble data structure */
extern "C" void free_ens(int center, unsigned char *loc); /* free ensemble data structure */
#elif defined(__STDC__)
extern ens* mkens_from_grib(int center, unsigned char *loc); /* make GRIB ensemble data structure */
extern ens* mkens_from_grib2(GRIB2::g2int igdtnum, GRIB2::g2int *igdtmpl); /* make GRIB2 ensemble data structure */
extern void free_ens(int center, unsigned char *loc); /* free ensemble data structure */
#else
extern ens* mkens_from_grib( /* int center, unsigned char *loc */ ); /* make GRIB ensemble data structure */
extern ens* mkens_from_grib2( /* GRIB2::g2int igdtnum, GRIB2::g2int *igdtmpl */);   /* make GRIB2 ensemble data structure */
extern void free_ens( /* int center, unsigned char *loc */ ); /* free ensemble data structure */
#endif


#endif /* ENS_H_ */
