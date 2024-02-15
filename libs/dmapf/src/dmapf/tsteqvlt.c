#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "cmapf.h"

double pairs[][2]={{90.,60.},{60,65},{90.,-90.},{60.,60.}};

main() {
int k;
  for (k=0;k<sizeof(pairs)/sizeof(double)/2;k++) {
  printf("%f %f %f\n",pairs[k][0],pairs[k][1],eqvlat(pairs[k][0],pairs[k][1]));
  }
/*  getc(stdin); */
}
