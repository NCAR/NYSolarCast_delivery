#include <stdlib.h>
#include <stdio.h>
#include "cmapf.h"

#define NARR(arr) (sizeof(arr)/sizeof(arr[0]))


main(){
lat_lon ll[]={ {40.,-179.}, {41.,-179.}, {41.,179.}, {40.,179.}, {40.,-179.},
               {40.,-178.} };
x_y xy[NARR(ll)];
int index[(NARR(ll)+1)];
x_y jump[4];
maparam stcprm;
int k,l,m,nstr,resp;
   stlmbr(& stcprm, 0.,0.);
   stcm1p(&stcprm, 0.,0., 0.,0.,  0.,0.,  100., 0.);
   nstr =  kcllxy ( & stcprm, ll, xy, NARR(ll), index, NARR(index) ) ;
   for (k=0;k<nstr-1;k++) {
     printf("%d\n",index[k]);
     for (l=index[k];l<index[k+1];l++) {
       printf("%d ; from %f,%f to %f,%f\n",l,ll[l].lat,ll[l].lng,xy[l].x,xy[l].y);
     }
     resp = kcwrap (& stcprm,& ll[index[k+1]-1], jump );
     if (resp != 0) {
       printf("%d ",resp);
       printf(";from %f,%f to %f,%f\n jump\n",jump[0].x,jump[0].y,jump[1].x,jump[1].y);
       printf("; from %f,%f to %f,%f\n jump\n",jump[2].x,jump[2].y,jump[3].x,jump[3].y);
       printf("\n");
     } else {
       printf("resp = 0\n");
     }
   }
   printf("%d\n",index[nstr-1]);
   fgetc(stdin);
   return 0;
}