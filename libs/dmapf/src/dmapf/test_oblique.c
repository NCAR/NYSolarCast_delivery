#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <values.h>
#include <dmapf/cmapf.h>

void prmprt(maparam * stcprm) {
int k,l;
  printf("gamma = %lf\n",stcprm->gamma);
  for (k=0;k<3;k++) {
	 printf("line %d: ",k);
	 for (l=0;l<3;l++) {
		printf(" %10.6lf ",stcprm->rotate[l] [k]);
	 }
	 printf("\n");
  }
  printf("x0 = %g, y0 = %g\n",stcprm->x0,stcprm->y0);
  printf("cos(th) = %g, sin(th) = %g\n",stcprm->crotate,stcprm->srotate);
  printf("gridsize = %g\n\n",stcprm->gridszeq);
}

int answer_prompt(char * prompt,char * flags) {
char instr[82];
int result;
  printf(prompt);
  gets(instr);
  result = strcspn(flags,instr);
  if(result < strlen(flags)) return (result + 2)>>1;
  else return 0;
}


void main() {
maparam stcprm;
/* double pole_lat = 30. , pole_lon = -90.; */
double latit,longit,x,y,oldx,oldy;
double latit1,latit2,longit1,longit2;
double x1,y1,x2,y2;
double reflat,reflong;
double gridsize,orient;
double enx,eny,enz;
double gx,gy;
double ue,vn,ug,vg;
int another_point;
int choice;
char instr[82];
  do {
	 printf("O - Oblique Stereographic\nL - Lambert Polar\n");
	 printf("T - Transverse Mercator\n");
	 printf("U - Oblique Mercator\n");
	 printf("C - Oblique Lambert\n");
	 choice = answer_prompt("Enter Choice: ","OoLlTtUuCc");
	 switch (choice) {
	 case 1:
		printf("Enter pole latitude and longitude: ");
		gets(instr);
		sscanf(instr,"%lf, %lf",&latit,&longit);
		sobstr(&stcprm, latit,longit);
		break;
	 case 2:
		printf("Enter two reference latitudes and a longitude: ");
		gets(instr);
		sscanf(instr,"%lf, %lf, %lf",&latit1,&latit2,&longit);
		printf("Lat1 = %lf, Lat2 = %lf\n",latit1,latit2);
		printf ("Eqvlat = %lf\n",eqvlat(latit1,latit2));
		stlmbr(&stcprm, eqvlat(latit1,latit2), longit);
		break;
	 case 3:
		printf("Enter latitude and longitude: ");
		gets(instr);
		sscanf(instr,"%lf, %lf",&latit,&longit);
		stvmrc(&stcprm, latit, longit);
		break;
	 case 4:
		printf("Enter Central latitude and longitude: ");
		gets(instr);
		sscanf(instr,"%lf, %lf",&latit1,&longit1);
		printf("Enter Secondary latitude and longitude: ");
		gets(instr);
		sscanf(instr,"%lf, %lf",&latit2,&longit2);
		sobmrc(&stcprm, latit1, longit1, latit2, longit2);
		break;
	 case 5:
		printf("Enter Central latitude and longitude: ");
		gets(instr);
		sscanf(instr,"%lf, %lf",&latit,&longit);
		printf("Enter lat & lon of second point on circle: ");
		gets(instr);
		sscanf(instr,"%lf, %lf",&latit1,&longit1);
		printf("Enter lat & lon of third point on circle: ");
		gets(instr);
		sscanf(instr,"%lf, %lf",&latit2,&longit2);
		soblmbr(&stcprm, latit1, longit1, latit,longit, latit2, longit2);
		break;
	 }
	 choice = answer_prompt("1-point or 2-point scaling? ","1o2t");
	 switch (choice) {
	 case 1:
		printf("Enter x,y of anchor point: ");
		gets(instr);sscanf(instr,"%lf, %lf",&x,&y);
		printf("Enter lat,long of anchor point: ");
		gets(instr);sscanf(instr,"%lf, %lf",&latit,&longit);
		printf("Enter lat,long of reference point: ");
		gets(instr);sscanf(instr,"%lf, %lf",&reflat,&reflong);
		printf("Enter grid size at reference point: ");
		gets(instr);sscanf(instr,"%lf",&gridsize);
		printf("Enter y-axis orientation at reference point: ");
		gets(instr);sscanf(instr,"%lf",&orient);
		stcm1p(&stcprm, x,y, latit,longit, reflat,reflong, gridsize,orient);
		break;
	 case 2:
		printf("Enter x,y of first anchor point: ");
		gets(instr);sscanf(instr,"%lf, %lf",&x1,&y1);
		printf("Enter lat,long of first anchor point: ");
		gets(instr);sscanf(instr,"%lf, %lf",&latit1,&longit1);
		printf("Enter x,y of second anchor point: ");
		gets(instr);sscanf(instr,"%lf, %lf",&x2,&y2);
		printf("Enter lat,long of second anchor point: ");
		gets(instr);sscanf(instr,"%lf, %lf",&latit2,&longit2);
		stcm2p(&stcprm,x1,y1,latit1,longit1, x2,y2,latit2,longit2);
		break;
	 }
	 prmprt(&stcprm);
	 do {
		another_point = answer_prompt("Translate x,y point? (y/n) ","yY");
		if (another_point) {
		  printf("Enter x,y: ");
		  gets(instr);
		  sscanf(instr,"%lf, %lf",&x,&y);
		  cxy2ll(&stcprm, x,y, &latit,&longit);
		  cll2xy(&stcprm, latit,longit,&oldx,&oldy);
		  printf("x,y = (%lf,%lf), lat,long = (%lf,%lf).\n",
				oldx,oldy,latit,longit);
		  printf("gridsize(x,y) = %lf, gridsize(l,l) = %lf\n",
			 cgszxy(&stcprm,x,y), cgszll(&stcprm,latit,longit));
		  cpolxy(&stcprm, x,y, &enx,&eny,&enz);
		  printf("Polar axis from x,y = (%lf, %lf, %lf)\n",enx,eny,enz);
		  cpolll(&stcprm, latit,longit, &enx,&eny,&enz);
		  printf("Polar axis from lat,long = (%lf, %lf, %lf)\n",enx,eny,enz);
		  cgrnxy(&stcprm, x,y, &enx,&eny,&enz);
		  printf("Greenwich axis from x,y = (%lf, %lf, %lf)\n",enx,eny,enz);
		  cgrnll(&stcprm, latit,longit, &enx,&eny,&enz);
		  printf("Greenwich axis from lat,long = (%lf, %lf, %lf)\n",enx,eny,enz);
		  cc2gxy(&stcprm, x,y, 0.,10., &ug,&vg);
		  cg2cxy(&stcprm, x,y, ug,vg, &ue,&vn);
		  printf("x,y winds from (E,N) to (Ug,Vg):(%lf,%lf) to (%lf,%lf)\n",
				ue,vn,ug,vg);
		  cc2gll(&stcprm, latit,longit, 0.,10., &ug,&vg);
		  cg2cll(&stcprm, latit,longit, ug,vg, &ue,&vn);
		  printf("l,l winds from (E,N) to (Ug,Vg):(%lf,%lf) to (%lf,%lf)\n",
				ue,vn,ug,vg);
		  ccrvxy(&stcprm, x,y, &gx,&gy);
		  printf("x,y curvature vector (gx,gy):(%lf,%lf)\n",gx,gy);
		  ccrvll(&stcprm, latit,longit, &gx,&gy);
		  printf("lat,long curvature vector (gx,gy):(%lf,%lf)\n",gx,gy);
		}
	 } while (another_point);
	 choice = answer_prompt("Another Projection? (y/n) ","yY");
  } while (choice);
}
