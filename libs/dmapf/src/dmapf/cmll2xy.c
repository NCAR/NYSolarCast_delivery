#include <math.h>
#include <values.h> 
#include <dmapf/cmapf.h>

int cml2xy(maparam * stcprm,double lat[],double longit[],
           int size_l,
           double x[],double y[]) {
vector_3d old_geog,new_geog,mid_geog;
double fact,mid_mag;
double xi,eta;
int count;
int npoints = (size_l>=0? size_l : -size_l);
int oldquad,newquad;
#define ABSDIFF(A,B) ((A)<(B)?(B)-(A):(A)-(B))
/* Given a string of latitudes and longitudes, returns a string
 * of x-y values.  In the event that the *first* segment crosses the
 * 'cut' of the map, from side a to side b, the first x-y returned
 * will be the side b version of the cut-crossing point.  (If size_l<0,
 * then only the first part of the segment, on side a, will be returned.)
 *  If *any other* segment crosses the cut, this ends the transfer, the
 * last x-y value will be the near side of cut, and the returned value
 * will be negative.  Returns an int, the absolute value of which is
 * the number of points transformed, and the sign of which indicates
 * (if negative) that more points remain to be transformed.*/
  if(size_l == 0) return 0;
 /* Leading segment of output string.
  */
  old_geog = basegtom(stcprm,ll_geog(lat[0],longit[0]));
  oldquad=n_quad(old_geog);
  count=1;
  map_xy(stcprm, old_geog, &x[0], &y[0]);
  if (npoints == 1) return 1;
  /* If only one point provided, it is returned.*/
  if (size_l > 0) {
  /* In this case, must determine whether first line
   * segment crosses the cut.  If not, first two points are leading segment
   * of output.  If so, second point determines proper side of cut for
   * midpoint, and midpoint and second point are leading segment of output.
   */
    new_geog = basegtom(stcprm,ll_geog(lat[count],longit[count]));
    newquad = n_quad(new_geog);
    if (ABSDIFF(oldquad,newquad) > 1) {
/* In this case, the cut *may* have been crossed.  If it has, replace
 * xy-point in location 0 with interpolated value*/
    fact = new_geog.v[1]/(new_geog.v[1]-old_geog.v[1]);
    if (fact >=0 && fact <= 1.0) {
/* Unless this is true, segment doesn't even cross the x-axis.*/
      mid_geog.v[1]=0;
      mid_geog.v[0]=new_geog.v[0]+fact*(old_geog.v[0]-new_geog.v[0]);
      mid_geog.v[2]=new_geog.v[2]+fact*(old_geog.v[2]-new_geog.v[2]);
      if (mid_geog.v[0] <= 0. ) {
      /* only in this case is there a crossing of the cut.*/
        mid_mag = hypot(mid_geog.v[0],mid_geog.v[2]);
        if (mid_mag > 0.) {
          mid_geog.v[0] /= mid_mag;
          mid_geog.v[2] /= mid_mag;
        } else mid_geog.v[0]=-1.;
        /* Replace point in x[0],y[0] with interpolated point in same
         * quadrant as new_geog, which will go into x[1],y[1]
         */
        map_xe(stcprm, mid_geog, &xi, &eta, newquad);
        xe_xy(stcprm, xi,eta, &x[0],&y[0]);
      }
    }}
/* First point is either given or interpolated.  In either case, second point
 * is partly calculated; finish calculation and store in string. Advance
 * count. */
    map_xy(stcprm, new_geog, &x[count], &y[count]);
    old_geog = new_geog;
    oldquad = newquad;
    count++;
  }
  /* End of case size_l >0.  In contrary case, Leading part of output
   * would be first point (not segment)*/
/* geog values indicate v[0] "Toward lat0,lon0", v[1] East @0,0,
 * v[2] North @0,0.  Thus cut half-plane is v[1]=0, v[0]<=0.
 * Interpolate.*/
  for (;count < npoints;count++) {
    new_geog = basegtom(stcprm,ll_geog(lat[count],longit[count]));
    newquad = n_quad(new_geog);
    if (ABSDIFF(oldquad,newquad) > 1) {
/* Place interpolated value in location count and exit*/
/*If cut crossed, replace xy-point in location 0 with interpolated value*/
    fact = new_geog.v[1]/(new_geog.v[1]-old_geog.v[1]);
    if ( fact >= 0. && fact <= 1.) {
      mid_geog.v[1]=0;
      mid_geog.v[0]=new_geog.v[0]+fact*(old_geog.v[0]-new_geog.v[0]);
      mid_geog.v[2]=new_geog.v[2]+fact*(old_geog.v[2]-new_geog.v[2]);
      if (mid_geog.v[0] <= 0. ) {
      /* only in this case is there a crossing of the cut.*/
        mid_mag = hypot(mid_geog.v[0],mid_geog.v[2]);
        if (mid_mag > 0.) {
          mid_geog.v[0] /= mid_mag;
          mid_geog.v[2] /= mid_mag;
        } else mid_geog.v[0]=-1.;
        map_xe(stcprm, mid_geog, &xi, &eta, oldquad);
#if 1==0
        if (old_geog.v[1]>0) {
          map_xe(stcprm, mid_geog, &xi, &eta, 'e');
        } else {
          map_xe(stcprm, mid_geog, &xi, &eta, 'w');
        }
#endif
        xe_xy(stcprm, xi,eta, &x[count],&y[count]);
        count++;
        return - count;
      }
    }}
    map_xy(stcprm, new_geog, &x[count], &y[count]);
    old_geog = new_geog;
    oldquad  = newquad ;
  }
  return count;
}

