/*
 * Structure for a site list. Contains the site ID, and other identifying info.
 * DICAST format.
 */

#include "product_data.h"

#ifndef SITE_LIST_H
#define SITE_LIST_H

int process_sites(char *sitename, int ncid, float **lat, float **lon, int *ns);
int make_site_data(product_data *pp, float fillval, char *calc_type, float *lat, float *lon, int ns, float *site_data);

#endif
