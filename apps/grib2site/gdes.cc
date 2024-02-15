/*
 *   Copyright 1995, University Corporation for Atmospheric Research
 *   See ../COPYRIGHT file for copying and redistribution conditions.
 */
/* $Id: gdes.cc,v 1.9 2012/02/17 15:24:30 cowie Exp $ */

#include <stdlib.h>			/* for malloc(), free(), ... */
#include <stdio.h>

#include "log/log.hh"
#include "emalloc.h"
#include "gdes.h"
#include "gbytem.h"
#include "grib1.h"
#include "centers.h"
#include "quasi.h"

extern Log *logFile;

#ifdef __STDC__
static gdes* gds_to_gdes(gds * gdsp);
static gdes* nmc_1(void);
static gdes* nmc_100(void);
static gdes* nmc_101(void);
static gdes* nmc_104(void);
static gdes* nmc_105(void);
static gdes* nmc_2(void);
static gdes* nmc_207(void);
static gdes* nmc_21(void);
static gdes* nmc_211(void);
static gdes* nmc_212(void);
static gdes* nmc_22(void);
static gdes* nmc_23(void);
static gdes* nmc_24(void);
static gdes* nmc_25(void);
static gdes* nmc_26(void);
static gdes* nmc_27(void);
static gdes* nmc_28(void);
static gdes* nmc_3(void);
static gdes* nmc_5(void);
static gdes* nmc_50(void);
static gdes* nmc_6(void);
static gdes* nmc_61(void);
static gdes* nmc_62(void);
static gdes* nmc_63(void);
static gdes* nmc_64(void);
static gdes* synth_gdes(int centerid, int gridid);
static int fill_albers(grid_albers * raw, gdes * gd);
static int fill_gau(grid_gau * raw, gdes * gd);
static int fill_gnomon(grid_gnomon * raw, gdes * gd);
static int fill_lambert(grid_lambert * raw, gdes * gd);
static int fill_ll(grid_ll *raw, gdes *gd);
static int fill_mercator(grid_mercator * raw, gdes * gd);
static int fill_olambert(grid_olambert * raw, gdes * gd);
static int fill_polars(grid_polars * raw, gdes * gd);
static int fill_rgau(grid_rgau * raw, gdes * gd);
static int fill_rll(grid_rll *raw, gdes *gd);
static int fill_rsph(grid_rsph * raw, gdes * gd);
static int fill_sgau(grid_sgau * raw, gdes * gd);
static int fill_sll(grid_sll * raw, gdes * gd);
static int fill_spacev(grid_spacev * raw, gdes * gd);
static int fill_sph(grid_sph * raw, gdes * gd);
static int fill_srgau(grid_srgau * raw, gdes * gd);
static int fill_srll(grid_srll * raw, gdes * gd);
static int fill_srsph(grid_srsph * raw, gdes * gd);
static int fill_ssph(grid_ssph * raw, gdes * gd);
#endif


static int
fill_ll(grid_ll *raw, gdes *gd)
{
    gdes_ll *cooked = &gd->grid.ll;

    cooked->ni = g2i(raw->ni);
    cooked->nj = g2i(raw->nj);
    cooked->la1 = g3si(raw->la1)*.001; /* millidegrees to degrees */
    cooked->lo1 = g3si(raw->lo1)*.001;
    cooked->la2 = g3si(raw->la2)*.001;
    cooked->lo2 = g3si(raw->lo2)*.001;
    cooked->di = g2i(raw->di)*.001;
    cooked->dj = g2i(raw->dj)*.001;
    cooked->rot=0;
    cooked->strch=0;

    if(cooked->ni == G2I_MISSING && g2i(raw->di) == G2I_MISSING) {
	cooked->ni = GDES_INT_MISSING;
	cooked->di = GDES_FLOAT_MISSING;
	gd->quasi = QUASI_ROWS;	/* A quasi-regular grid with varying rows */
    } else if (cooked->nj == G2I_MISSING && g2i(raw->dj) == G2I_MISSING) {
	cooked->nj = GDES_INT_MISSING;
	cooked->dj = GDES_FLOAT_MISSING;
	gd->quasi = QUASI_COLS;	/* A quasi-regular grid with varying cols */
    }
    gd->ncols = cooked->ni;
    gd->nrows = cooked->nj;
    gd->npts = gd->ncols*gd->nrows;
    gd->res_flags = raw->res_flags;
    gd->scan_mode = raw->scan_mode;
    return 0;
}

static int
fill_rll(grid_rll *raw, gdes *gd)
{
    gdes_ll *cooked = &gd->grid.ll;
    cooked->ni = g2i(raw->ni);
    cooked->nj = g2i(raw->nj);
    cooked->la1 = g3si(raw->la1)*.001; /* millidegrees to degrees */
    cooked->lo1 = g3si(raw->lo1)*.001;
    cooked->la2 = g3si(raw->la2)*.001;
    cooked->lo2 = g3si(raw->lo2)*.001;
    cooked->di = g2i(raw->di)*.001;
    cooked->dj = g2i(raw->dj)*.001;
    cooked->rot=(rotated *)emalloc(sizeof(rotated));
    cooked->rot->lat = g3si(raw->lapole)*.001;
    cooked->rot->lon = g3si(raw->lopole)*.001;
    cooked->rot->angle = g4f(raw->angrot);
    cooked->strch=0;

    if(cooked->ni == G2I_MISSING && g2i(raw->di) == G2I_MISSING) {
	cooked->ni = GDES_INT_MISSING;
	cooked->di = GDES_FLOAT_MISSING;
	gd->quasi = QUASI_ROWS;	/* A quasi-regular grid with varying rows */
    } else if (cooked->nj == G2I_MISSING && g2i(raw->dj) == G2I_MISSING) {
	cooked->nj = GDES_INT_MISSING;
	cooked->dj = GDES_FLOAT_MISSING;
	gd->quasi = QUASI_COLS;	/* A quasi-regular grid with varying cols */
    }
    gd->ncols = cooked->ni;
    gd->nrows = cooked->nj;
    gd->npts = gd->ncols*gd->nrows;
    gd->res_flags = raw->res_flags;
    gd->scan_mode = raw->scan_mode;
    return 0;
}

static int
fill_sll(grid_sll *raw, gdes *gd)
{
    gdes_ll *cooked = &gd->grid.ll;
    cooked->ni = g2i(raw->ni);
    cooked->nj = g2i(raw->nj);
    cooked->la1 = g3si(raw->la1)*.001; /* millidegrees to degrees */
    cooked->lo1 = g3si(raw->lo1)*.001;
    cooked->la2 = g3si(raw->la2)*.001;
    cooked->lo2 = g3si(raw->lo2)*.001;
    cooked->di = g2i(raw->di)*.001;
    cooked->dj = g2i(raw->dj)*.001;
    cooked->rot=0;
    cooked->strch=(stretched *)emalloc(sizeof(stretched));
    cooked->strch->lat = g3si(raw->lastr)*.001;
    cooked->strch->lon = g3si(raw->lostr)*.001;
    cooked->strch->factor = g4f(raw->stretch);

    if(cooked->ni == G2I_MISSING && g2i(raw->di) == G2I_MISSING) {
	cooked->ni = GDES_INT_MISSING;
	cooked->di = GDES_FLOAT_MISSING;
	gd->quasi = QUASI_ROWS;	/* A quasi-regular grid with varying rows */
    } else if (cooked->nj == G2I_MISSING && g2i(raw->dj) == G2I_MISSING) {
	cooked->nj = GDES_INT_MISSING;
	cooked->dj = GDES_FLOAT_MISSING;
	gd->quasi = QUASI_COLS;	/* A quasi-regular grid with varying cols */
    }
    gd->ncols = cooked->ni;
    gd->nrows = cooked->nj;
    gd->npts = gd->ncols*gd->nrows;
    gd->res_flags = raw->res_flags;
    gd->scan_mode = raw->scan_mode;
    return 0;
}

static int
fill_srll(grid_srll *raw, gdes *gd)
{
    gdes_ll *cooked = &gd->grid.ll;
    cooked->ni = g2i(raw->ni);
    cooked->nj = g2i(raw->nj);
    cooked->la1 = g3si(raw->la1)*.001; /* millidegrees to degrees */
    cooked->lo1 = g3si(raw->lo1)*.001;
    cooked->la2 = g3si(raw->la2)*.001;
    cooked->lo2 = g3si(raw->lo2)*.001;
    cooked->di = g2i(raw->di)*.001;
    cooked->dj = g2i(raw->dj)*.001;
    cooked->rot=(rotated *)emalloc(sizeof(rotated));
    cooked->rot->lat = g3si(raw->lapole)*.001;
    cooked->rot->lon = g3si(raw->lopole)*.001;
    cooked->rot->angle = g4f(raw->angrot);
    cooked->strch=(stretched *)emalloc(sizeof(stretched));
    cooked->strch->lat = g3si(raw->lastr)*.001;
    cooked->strch->lon = g3si(raw->lostr)*.001;
    cooked->strch->factor = g4f(raw->stretch);

    if(cooked->ni == G2I_MISSING && g2i(raw->di) == G2I_MISSING) {
	cooked->ni = GDES_INT_MISSING;
	cooked->di = GDES_FLOAT_MISSING;
	gd->quasi = QUASI_ROWS;	/* A quasi-regular grid with varying rows */
    } else if (cooked->nj == G2I_MISSING && g2i(raw->dj) == G2I_MISSING) {
	cooked->nj = GDES_INT_MISSING;
	cooked->dj = GDES_FLOAT_MISSING;
	gd->quasi = QUASI_COLS;	/* A quasi-regular grid with varying cols */
    }
    gd->ncols = cooked->ni;
    gd->nrows = cooked->nj;
    gd->npts = gd->ncols*gd->nrows;
    gd->res_flags = raw->res_flags;
    gd->scan_mode = raw->scan_mode;
    return 0;
}

static int
fill_gau(grid_gau *raw, gdes *gd)
{
    gdes_gau *cooked = &gd->grid.gau;
    cooked->ni = g2i(raw->ni);
    cooked->nj = g2i(raw->nj);
    cooked->la1 = g3si(raw->la1)*.001; /* millidegrees to degrees */
    cooked->lo1 = g3si(raw->lo1)*.001;
    cooked->la2 = g3si(raw->la2)*.001;
    cooked->lo2 = g3si(raw->lo2)*.001;
    cooked->di = g2i(raw->di)*.001;
    cooked->n = g2i(raw->n);
    cooked->rot=0;
    cooked->strch=0;

    if(cooked->ni == G2I_MISSING && g2i(raw->di) == G2I_MISSING) {
	cooked->ni = GDES_INT_MISSING;
	cooked->di = GDES_FLOAT_MISSING;
	gd->quasi = QUASI_ROWS;	/* A quasi-regular grid with varying rows */
    }
    gd->ncols = cooked->ni;
    gd->nrows = cooked->nj;
    gd->npts = gd->ncols*gd->nrows;
    gd->res_flags = raw->res_flags;
    gd->scan_mode = raw->scan_mode;
    return 0;
}

static int
fill_rgau(grid_rgau *raw, gdes *gd)
{
    gdes_gau *cooked = &gd->grid.gau;
    cooked->ni = g2i(raw->ni);
    cooked->nj = g2i(raw->nj);
    cooked->la1 = g3si(raw->la1)*.001; /* millidegrees to degrees */
    cooked->lo1 = g3si(raw->lo1)*.001;
    cooked->la2 = g3si(raw->la2)*.001;
    cooked->lo2 = g3si(raw->lo2)*.001;
    cooked->di = g2i(raw->di)*.001;
    cooked->n = g2i(raw->n);
    cooked->rot=(rotated *)emalloc(sizeof(rotated));
    cooked->rot->lat = g3si(raw->lapole)*.001;
    cooked->rot->lon = g3si(raw->lopole)*.001;
    cooked->rot->angle = g4f(raw->angrot);
    cooked->strch = 0;

    if(cooked->ni == G2I_MISSING && g2i(raw->di) == G2I_MISSING) {
	cooked->ni = GDES_INT_MISSING;
	cooked->di = GDES_FLOAT_MISSING;
	gd->quasi = QUASI_ROWS;	/* A quasi-regular grid with varying rows */
    }
    gd->ncols = cooked->ni;
    gd->nrows = cooked->nj;
    gd->npts = gd->ncols*gd->nrows;
    gd->res_flags = raw->res_flags;
    gd->scan_mode = raw->scan_mode;
    return 0;
}

static int
fill_sgau(
    grid_sgau *raw,
    gdes *gd)
{
    gdes_gau *cooked = &gd->grid.gau;
    cooked->ni = g2i(raw->ni);
    cooked->nj = g2i(raw->nj);
    cooked->la1 = g3si(raw->la1)*.001; /* millidegrees to degrees */
    cooked->lo1 = g3si(raw->lo1)*.001;
    cooked->la2 = g3si(raw->la2)*.001;
    cooked->lo2 = g3si(raw->lo2)*.001;
    cooked->di = g2i(raw->di)*.001;
    cooked->n = g2i(raw->n);
    cooked->rot=0;
    cooked->strch=(stretched *)emalloc(sizeof(stretched));
    cooked->strch->lat = g3si(raw->lastr)*.001;
    cooked->strch->lon = g3si(raw->lostr)*.001;
    cooked->strch->factor = g4f(raw->stretch);

    if(cooked->ni == G2I_MISSING && g2i(raw->di) == G2I_MISSING) {
	cooked->ni = GDES_INT_MISSING;
	cooked->di = GDES_FLOAT_MISSING;
	gd->quasi = QUASI_ROWS;	/* A quasi-regular grid with varying rows */
    }
    gd->ncols = cooked->ni;
    gd->nrows = cooked->nj;
    gd->npts = gd->ncols*gd->nrows;
    gd->res_flags = raw->res_flags;
    gd->scan_mode = raw->scan_mode;
    return 0;
}

static int
fill_srgau(
    grid_srgau *raw,
    gdes *gd)
{
    gdes_gau *cooked = &gd->grid.gau;
    cooked->ni = g2i(raw->ni);
    cooked->nj = g2i(raw->nj);
    cooked->la1 = g3si(raw->la1)*.001; /* millidegrees to degrees */
    cooked->lo1 = g3si(raw->lo1)*.001;
    cooked->la2 = g3si(raw->la2)*.001;
    cooked->lo2 = g3si(raw->lo2)*.001;
    cooked->di = g2i(raw->di)*.001;
    cooked->n = g2i(raw->n);
    cooked->rot=(rotated *)emalloc(sizeof(rotated));
    cooked->rot->lat = g3si(raw->lapole)*.001;
    cooked->rot->lon = g3si(raw->lopole)*.001;
    cooked->rot->angle = g4f(raw->angrot);
    cooked->strch=(stretched *)emalloc(sizeof(stretched));
    cooked->strch->lat = g3si(raw->lastr)*.001;
    cooked->strch->lon = g3si(raw->lostr)*.001;
    cooked->strch->factor = g4f(raw->stretch);

    if(cooked->ni == G2I_MISSING && g2i(raw->di) == G2I_MISSING) {
	cooked->ni = GDES_INT_MISSING;
	cooked->di = GDES_FLOAT_MISSING;
	gd->quasi = QUASI_ROWS;	/* A quasi-regular grid with varying rows */
    }
    gd->ncols = cooked->ni;
    gd->nrows = cooked->nj;
    gd->npts = gd->ncols*gd->nrows;
    gd->res_flags = raw->res_flags;
    gd->scan_mode = raw->scan_mode;
    return 0;
}

static int
fill_sph(
    grid_sph *raw,
    gdes *gd)
{
    gdes_sph *cooked = &gd->grid.sph;
    cooked->j = g2i(raw->j);
    cooked->k = g2i(raw->k);
    cooked->m = g2i(raw->m);
    cooked->type = g1i(raw->type);
    cooked->mode = g1i(raw->mode);
    cooked->rot=0;
    cooked->strch=0;
    gd->ncols = cooked->j;	/* *** probably not right ? *** */
    gd->nrows = cooked->k;
    gd->npts = gd->ncols*gd->nrows;
    gd->res_flags = 0;
    gd->scan_mode = 0;
    return 0;
}

static int
fill_rsph(
    grid_rsph *raw,
    gdes *gd)
{
    gdes_sph *cooked = &gd->grid.sph;
    cooked->j = g2i(raw->j);
    cooked->k = g2i(raw->k);
    cooked->m = g2i(raw->m);
    cooked->type = g1i(raw->type);
    cooked->mode = g1i(raw->mode);
    cooked->rot=(rotated *)emalloc(sizeof(rotated));
    cooked->rot->lat = g3si(raw->lapole)*.001;
    cooked->rot->lon = g3si(raw->lopole)*.001;
    cooked->rot->angle = g4f(raw->angrot);
    cooked->strch=0;
    gd->ncols = cooked->j;	/* *** probably not right *** */
    gd->nrows = cooked->k;
    gd->npts = gd->ncols*gd->nrows;
    gd->res_flags = 0;
    gd->scan_mode = 0;
    return 0;
}

static int
fill_ssph(
    grid_ssph *raw,
    gdes *gd)
{
    gdes_sph *cooked = &gd->grid.sph;
    cooked->j = g2i(raw->j);
    cooked->k = g2i(raw->k);
    cooked->m = g2i(raw->m);
    cooked->type = g1i(raw->type);
    cooked->mode = g1i(raw->mode);
    cooked->rot=0;
    cooked->strch=(stretched *)emalloc(sizeof(stretched));
    cooked->strch->lat = g3si(raw->lastr)*.001;
    cooked->strch->lon = g3si(raw->lostr)*.001;
    cooked->strch->factor = g4f(raw->stretch);
    gd->ncols = cooked->j;	/* *** probably not right *** */
    gd->nrows = cooked->k;
    gd->npts = gd->ncols*gd->nrows;
    gd->res_flags = 0;
    gd->scan_mode = 0;
    return 0;
}

static int
fill_srsph(
    grid_srsph *raw,
    gdes *gd)
{
    gdes_sph *cooked = &gd->grid.sph;
    cooked->j = g2i(raw->j);
    cooked->k = g2i(raw->k);
    cooked->m = g2i(raw->m);
    cooked->type = g1i(raw->type);
    cooked->mode = g1i(raw->mode);
    cooked->rot=(rotated *)emalloc(sizeof(rotated));
    cooked->rot->lat = g3si(raw->lapole)*.001;
    cooked->rot->lon = g3si(raw->lopole)*.001;
    cooked->rot->angle = g4f(raw->angrot);
    cooked->strch=(stretched *)emalloc(sizeof(stretched));
    cooked->strch->lat = g3si(raw->lastr)*.001;
    cooked->strch->lon = g3si(raw->lostr)*.001;
    cooked->strch->factor = g4f(raw->stretch);
    gd->ncols = cooked->j;	/* *** probably not right *** */
    gd->nrows = cooked->k;
    gd->npts = gd->ncols*gd->nrows;
    gd->res_flags = 0;
    gd->scan_mode = 0;
    return 0;
}

static int
fill_mercator(
    grid_mercator *raw,
    gdes *gd)
{
    gdes_mercator *cooked = &gd->grid.mercator;
    cooked->ni = g2i(raw->ni);
    cooked->nj = g2i(raw->nj);
    cooked->la1 = g3si(raw->la1)*.001; /* millidegrees to degrees */
    cooked->lo1 = g3si(raw->lo1)*.001;
    cooked->la2 = g3si(raw->la2)*.001;
    cooked->lo2 = g3si(raw->lo2)*.001;
    cooked->latin = g3si(raw->latin)*.001;
    cooked->di = g3i(raw->di);	/* in meters */
    cooked->dj = g3i(raw->dj);	/* in meters */
    gd->ncols = cooked->ni;
    gd->nrows = cooked->nj;
    gd->npts = gd->ncols*gd->nrows;
    gd->res_flags = raw->res_flags;
    gd->scan_mode = raw->scan_mode;
    return 0;
}

static int
fill_polars(
    grid_polars *raw,
    gdes *gd)
{
    gdes_polars *cooked = &gd->grid.polars;
    cooked->nx = g2i(raw->nx);
    cooked->ny = g2i(raw->ny);
    cooked->la1 = g3si(raw->la1)*.001; /* millidegrees to degrees */
    cooked->lo1 = g3si(raw->lo1)*.001;
    cooked->lov = g3si(raw->lov)*.001;
    cooked->dx = g3i(raw->dx);	/* in meters */
    cooked->dy = g3i(raw->dy);	/* in meters */
    cooked->pole = ((raw->pole & 0x80) == 0x80);
    gd->ncols = cooked->nx;
    gd->nrows = cooked->ny;
    gd->npts = gd->ncols*gd->nrows;
    gd->res_flags = raw->res_flags;
    gd->scan_mode = raw->scan_mode;
    return 0;
}

static int
fill_gnomon(
    grid_gnomon *raw,
    gdes *gd)
{
    gdes_polars *cooked = &gd->grid.polars;
    cooked->nx = g2i(raw->nx);
    cooked->ny = g2i(raw->ny);
    cooked->la1 = g3si(raw->la1)*.001; /* millidegrees to degrees */
    cooked->lo1 = g3si(raw->lo1)*.001;
    cooked->lov = g3si(raw->lov)*.001;
    cooked->dx = g3i(raw->dx);	/* in meters */
    cooked->dy = g3i(raw->dy);	/* in meters */
    cooked->pole = ((raw->pole & 0x80) == 0x80);
    gd->ncols = cooked->nx;
    gd->nrows = cooked->ny;
    gd->npts = gd->ncols*gd->nrows;
    gd->res_flags = raw->res_flags;
    gd->scan_mode = raw->scan_mode;
    return 0;
}

static int
fill_lambert(
    grid_lambert *raw,
    gdes *gd)
{
    gdes_lambert *cooked = &gd->grid.lambert;
    cooked->nx = g2i(raw->nx);
    cooked->ny = g2i(raw->ny);
    cooked->la1 = g3si(raw->la1)*.001; /* millidegrees to degrees */
    cooked->lo1 = g3si(raw->lo1)*.001;
    cooked->lov = g3si(raw->lov)*.001;
    cooked->dx = g3i(raw->dx);	/* in meters */
    cooked->dy = g3i(raw->dy);	/* in meters */
    cooked->pole = ((raw->pole & 0x80) == 0x80);
    cooked->centers = ((raw->pole & 0x40) == 0x40) + 1;
    cooked->latin1 = g3si(raw->latin1)*.001;
    cooked->latin2 = g3si(raw->latin2)*.001;
    cooked->splat = g3si(raw->splat)*.001;
    cooked->splon = g3si(raw->splon)*.001;
    gd->ncols = cooked->nx;
    gd->nrows = cooked->ny;
    gd->npts = gd->ncols*gd->nrows;
    gd->res_flags = raw->res_flags;
    gd->scan_mode = raw->scan_mode;
    return 0;
}

static int
fill_olambert(
    grid_olambert *raw,
    gdes *gd)
{
    gdes_lambert *cooked = &gd->grid.lambert;
    cooked->nx = g2i(raw->nx);
    cooked->ny = g2i(raw->ny);
    cooked->la1 = g3si(raw->la1)*.001; /* millidegrees to degrees */
    cooked->lo1 = g3si(raw->lo1)*.001;
    cooked->lov = g3si(raw->lov)*.001;
    cooked->dx = g3i(raw->dx);	/* in meters */
    cooked->dy = g3i(raw->dy);	/* in meters */
    cooked->pole = ((raw->pole & 0x80) == 0x80);
    cooked->centers = ((raw->pole & 0x40) == 0x40) + 1;
    cooked->latin1 = g3si(raw->latin1)*.001;
    cooked->latin2 = g3si(raw->latin2)*.001;
    cooked->splat = g3si(raw->splat)*.001;
    cooked->splon = g3si(raw->splon)*.001;
    gd->ncols = cooked->nx;
    gd->nrows = cooked->ny;
    gd->npts = gd->ncols*gd->nrows;
    gd->res_flags = raw->res_flags;
    gd->scan_mode = raw->scan_mode;
    return 0;
}

static int
fill_albers(
    grid_albers *raw,
    gdes *gd)
{
    gdes_lambert *cooked = &gd->grid.lambert;
    cooked->nx = g2i(raw->nx);
    cooked->ny = g2i(raw->ny);
    cooked->la1 = g3si(raw->la1)*.001; /* millidegrees to degrees */
    cooked->lo1 = g3si(raw->lo1)*.001;
    cooked->lov = g3si(raw->lov)*.001;
    cooked->dx = g3i(raw->dx);	/* in meters */
    cooked->dy = g3i(raw->dy);	/* in meters */
    cooked->pole = ((raw->pole & 0x80) == 0x80);
    cooked->centers = ((raw->pole & 0x40) == 0x40) + 1;
    cooked->latin1 = g3si(raw->latin1)*.001;
    cooked->latin2 = g3si(raw->latin2)*.001;
    cooked->splat = g3si(raw->splat)*.001;
    cooked->splon = g3si(raw->splon)*.001;
    gd->ncols = cooked->nx;
    gd->nrows = cooked->ny;
    gd->npts = gd->ncols*gd->nrows;
    gd->res_flags = raw->res_flags;
    gd->scan_mode = raw->scan_mode;
    return 0;
}

static int
fill_spacev(
    grid_spacev *raw,
    gdes *gd)
{
    gdes_spacev *cooked = &gd->grid.spacev;
    cooked->nx = g2i(raw->nx);
    cooked->ny = g2i(raw->ny);
    cooked->lap = g3si(raw->lap)*.001; /* millidegrees to degrees */
    cooked->lop = g3si(raw->lop)*.001;
    cooked->dx = g3i(raw->dx);	/* in grid-lengths */
    cooked->dy = g3i(raw->dy);	/* in grid-lengths */
    cooked->xp = g2i(raw->xp);
    cooked->yp = g2i(raw->yp);
    cooked->orient = g3si(raw->orient)*.001; /* millidegrees to degrees */
    cooked->nr = g3i(raw->nr);
    cooked->xo = g2i(raw->xo);
    cooked->yo = g2i(raw->yo);
    gd->ncols = cooked->nx;
    gd->nrows = cooked->ny;
    gd->npts = gd->ncols*gd->nrows;
    gd->res_flags = raw->res_flags;
    gd->scan_mode = raw->scan_mode;
    return 0;
}


/*
 * Make a gdes from an existing raw gds.
 * User should call free_gdes() when done to free.
 */
static gdes*
gds_to_gdes(
    gds *gdsp)
{
    int nv = g1i(gdsp->nv);
    int pv = g1i(gdsp->pv);
    int type = g1i(gdsp->type);
    int nerrs = 0;
    gdes *ret = (gdes *) emalloc(sizeof(gdes));

    ret->type = type;
    ret->quasi = QUASI_RECT;	/* ordinary rectangular grid is default */
    ret->nv = 0;
    ret->vc = 0;
    ret->lc = 0;
    ret->keep = 0;
    if(nv != 0 && nv != G1I_MISSING) { /* we have vert. coordinates */
        g4flt *fp = (g4flt *) ((char *)gdsp + pv);
        int i;
        
	ret->nv = nv;
        ret->vc = (float *) emalloc(ret->nv * sizeof(float));
        /* unpack the vertical coords into floats */
        for (i = 0; i < ret->nv; i++) {
            ret->vc[i] = g4f(*fp++);
        }
    }

    switch(type) {
    case GRID_LL:
	nerrs = fill_ll(&gdsp->grid.ll, ret);
	break;
    case GRID_RLL:
	nerrs = fill_rll(&gdsp->grid.rll, ret);
        break;
    case GRID_SLL:
	nerrs = fill_sll(&gdsp->grid.sll, ret);
        break;
    case GRID_SRLL:
	nerrs = fill_srll(&gdsp->grid.srll, ret);
        break;
    case GRID_GAU:
	nerrs = fill_gau(&gdsp->grid.gau, ret);
        break;
    case GRID_RGAU:
	nerrs = fill_rgau(&gdsp->grid.rgau, ret);
        break;
    case GRID_SGAU:
	nerrs = fill_sgau(&gdsp->grid.sgau, ret);
        break;
    case GRID_SRGAU:
	nerrs = fill_srgau(&gdsp->grid.srgau, ret);
        break;
    case GRID_SPH:
	nerrs = fill_sph(&gdsp->grid.sph, ret);
        break;
    case GRID_RSPH:
	nerrs = fill_rsph(&gdsp->grid.rsph, ret);
        break;
    case GRID_SSPH:
	nerrs = fill_ssph(&gdsp->grid.ssph, ret);
        break;
    case GRID_SRSPH:
	nerrs = fill_srsph(&gdsp->grid.srsph, ret);
        break;
    case GRID_MERCAT:
	nerrs = fill_mercator(&gdsp->grid.mercator, ret);
        break;
    case GRID_POLARS:
	nerrs = fill_polars(&gdsp->grid.polars, ret);
        break;
    case GRID_GNOMON:
	nerrs = fill_gnomon(&gdsp->grid.gnomon, ret);
        break;
    case GRID_LAMBERT:
	nerrs = fill_lambert(&gdsp->grid.lambert, ret);
        break;
    case GRID_ALBERS:
	nerrs = fill_albers(&gdsp->grid.albers, ret);
        break;
    case GRID_OLAMBERT:
	nerrs = fill_olambert(&gdsp->grid.olambert, ret);
        break;
    case GRID_SPACEV:
	nerrs = fill_spacev(&gdsp->grid.spacev, ret);
        break;
    case GRID_UTM:
    case GRID_SIMPOL:
    case GRID_MILLER:
    default:
	nerrs++;
    }
    if (nerrs) {
	free_gdes(ret);
	ret = 0;
	return ret;
    }

    if(ret->quasi == QUASI_ROWS) {
        ret->ncols = 1;
	ret->lc = (int *)emalloc((1 + ret->nrows) * sizeof(int));
	{	/* unpack list of row indexes */
	    g2int *ip = (g2int *) ((char *)gdsp + g1i(gdsp->pv)-1 + 4*ret->nv);
	    int i;
	    int n=0;
	    int maxlc = 0;
	    for (i = 0; i < ret->nrows; i++) {
		int nl;
		nl = g2i(*ip++);
		if (nl > maxlc)
		    maxlc = nl;
		ret->lc[i] = n;
		n += nl;
	    }
	    ret->lc[i] = n;
	    ret->npts = n;
	    ret->maxlc = maxlc;
	}
    } else if(ret->quasi == QUASI_COLS) {
        ret->nrows = 1;
	ret->lc = (int *)emalloc((1 + ret->ncols) * sizeof(int));
	{	/* unpack list of col indexes */
	    g2int *ip = (g2int *) ((char *)gdsp + g1i(gdsp->pv)-1 + 4*ret->nv);
	    int i;
	    int n=0;
	    int maxlc = 0;
	    for (i = 0; i < ret->ncols; i++) {
		int nl;
		nl = g2i(*ip++);
		if (nl > maxlc)
		    maxlc = nl;
		ret->lc[i] = n;
		n += nl;
	    }
	    ret->lc[i] = n;
	    ret->npts = n;
	    ret->maxlc = maxlc;
	}
    }
    if (nerrs) {
	free_gdes(ret);
	ret = 0;
    }
    return ret;
}

static void
nmc_21_24(gdes *g)
{
    g->grid.ll.ni = 37;
    g->grid.ll.nj = 37;
    g->grid.ll.di = 5.0;
    g->grid.ll.dj = 2.5;
    g->grid.ll.rot = 0;
    g->grid.ll.strch = 0;
    g->type = GRID_LL;
    g->ncols = 37;
    g->nrows = 37;
    g->npts = g->ncols*g->nrows;
    g->res_flags = RESCMP_DIRINC | RESCMP_UVRES;
    g->scan_mode = SCAN_J_PLUS;
    g->nv = 0;
    g->vc = 0;
    g->quasi = QUASI_RECT;
    g->lc = 0;
    g->keep = 1;
}

static gdes*
nmc_21()
{
    static gdes_ll *ll;
    static gdes g;

    nmc_21_24(&g);
    ll = &g.grid.ll;
    ll->la1 = 0;
    ll->lo1 = 0;
    ll->la2 = 90;
    ll->lo2 = 180;
    return &g;
}

static gdes*
nmc_22()
{
    static gdes_ll *ll;
    static gdes g;

    ll = &g.grid.ll;
    nmc_21_24(&g);
    ll->la1 = 0;
    ll->lo1 = -180;
    ll->la2 = 90;
    ll->lo2 = 0;
    return &g;
}

static gdes*
nmc_23()
{
    static gdes_ll *ll;
    static gdes g;

    ll = &g.grid.ll;
    nmc_21_24(&g);
    ll->la1 = -90;
    ll->lo1 = 0;
    ll->la2 = 0;
    ll->lo2 = 180;
    return &g;
}

static gdes*
nmc_24()
{
    static gdes_ll *ll;
    static gdes g;

    ll = &g.grid.ll;
    nmc_21_24(&g);
    ll->la1 = -90;
    ll->lo1 = -180;
    ll->la2 = 0;
    ll->lo2 = 0;
    return &g;
}

static void
nmc_25_26(gdes *g)
{
    g->grid.ll.ni = 72;
    g->grid.ll.nj = 19;
    g->grid.ll.di = 5.0;
    g->grid.ll.dj = 5.0;
    g->grid.ll.rot = 0;
    g->grid.ll.strch = 0;
    g->type = GRID_LL;
    g->ncols = 72;
    g->nrows = 19;
    g->npts = g->ncols*g->nrows;
    g->res_flags = RESCMP_DIRINC | RESCMP_UVRES;
    g->scan_mode = SCAN_J_PLUS;
    g->nv = 0;
    g->vc = 0;
    g->quasi = QUASI_RECT;
    g->lc = 0;
    g->keep = 1;
}

static gdes*
nmc_25()
{
    static gdes_ll *ll;
    static gdes g;

    ll = &g.grid.ll;
    nmc_25_26(&g);
    ll->la1 = 0;
    ll->lo1 = 0;
    ll->la2 = 90;
    ll->lo2 = 355;
    return &g;
}

static gdes*
nmc_26()
{
    static gdes_ll *ll;
    static gdes g;

    ll = &g.grid.ll;
    nmc_25_26(&g);
    ll->la1 = -90;
    ll->lo1 = 0;
    ll->la2 = 0;
    ll->lo2 = 355;
    return &g;
}

static gdes*
nmc_50()
{
    static gdes_ll *ll;
    static gdes g;

    ll = &g.grid.ll;

    ll->ni = 36;
    ll->nj = 33;
    ll->di = 2.5;
    ll->dj = 1.25;
    ll->la1 = 20;
    ll->lo1 = -140;
    ll->la2 = 60;
    ll->lo2 = -52.5;
    ll->rot = 0;
    ll->strch = 0;
    g.type = GRID_LL;
    g.ncols = 36;
    g.nrows = 33;
    g.npts = g.ncols*g.nrows;
    g.res_flags = RESCMP_DIRINC | RESCMP_UVRES;
    g.scan_mode = SCAN_J_PLUS;
    g.nv = 0;
    g.vc = 0;
    g.quasi = QUASI_RECT;
    g.lc = 0;
    g.keep = 1;
    return &g;
}

static void
nmc_61_64(gdes *g)
{
    g->grid.ll.ni = 91;
    g->grid.ll.nj = 46;
    g->grid.ll.di = 2.0;
    g->grid.ll.dj = 2.0;
    g->grid.ll.rot = 0;
    g->grid.ll.strch = 0;
    g->type = GRID_LL;
    g->ncols = 91;
    g->nrows = 46;
    g->npts = g->ncols*g->nrows;
    g->res_flags = RESCMP_DIRINC | RESCMP_UVRES;
    g->scan_mode = SCAN_J_PLUS;
    g->nv = 0;
    g->vc = 0;
    g->quasi = QUASI_RECT;
    g->lc = 0;
    g->keep = 1;
}

static gdes*
nmc_61()
{
    static gdes_ll *ll;
    static gdes g;

    ll = &g.grid.ll;
    nmc_61_64(&g);
    ll->la1 = 0;
    ll->lo1 = 0;
    ll->la2 = 90;
    ll->lo2 = 180;
    return &g;
}

static gdes*
nmc_62()
{
    static gdes_ll *ll;
    static gdes g;

    ll = &g.grid.ll;
    nmc_61_64(&g);
    ll->la1 = 0;
    ll->lo1 = -180;
    ll->la2 = 90;
    ll->lo2 = 0;
    return &g;
}

static gdes*
nmc_63()
{
    static gdes_ll *ll;
    static gdes g;

    ll = &g.grid.ll;
    nmc_61_64(&g);
    ll->la1 = -90;
    ll->lo1 = 0;
    ll->la2 = 0;
    ll->lo2 = 180;
    return &g;
}

static gdes*
nmc_64()
{
    static gdes_ll *ll;
    static gdes g;

    ll = &g.grid.ll;
    nmc_61_64(&g);
    ll->la1 = -90;
    ll->lo1 = -180;
    ll->la2 = 0;
    ll->lo2 = 0;
    return &g;
}

static gdes*
nmc_1()				/* Tropical strip (Mercator) */
{
    static gdes_mercator *mercator;
    static gdes g;

    mercator = &g.grid.mercator;

    mercator->ni = 73;
    mercator->nj = 23;
    mercator->di = 513669;
    mercator->dj = 513669;
    mercator->la1 = -48.09;
    mercator->lo1 = 0;
    mercator->la2 = 48.09;
    mercator->lo2 = 360.;
    mercator->latin = 22.5;

    g.type = GRID_MERCAT;
    g.ncols = 73;
    g.nrows = 23;
    g.npts = g.ncols*g.nrows;
    g.res_flags = RESCMP_DIRINC | RESCMP_UVRES;
    g.scan_mode = SCAN_J_PLUS;
    g.nv = 0;
    g.vc = 0;
    g.quasi = QUASI_RECT;
    g.lc = 0;
    g.keep = 1;
    return &g;
}

static gdes*
nmc_2()
{
    static gdes_ll *ll;
    static gdes g;

    ll = &g.grid.ll;

    ll->ni = 144;
    ll->nj = 73;
    ll->di = 2.5;
    ll->dj = 2.5;
    ll->la1 = 90;
    ll->lo1 = 0;
    ll->la2 = -90;
    ll->lo2 = 355;
    ll->rot = 0;
    ll->strch = 0;
    g.type = GRID_LL;
    g.ncols = 144;
    g.nrows = 73;
    g.npts = g.ncols*g.nrows;
    g.res_flags = RESCMP_DIRINC | RESCMP_UVRES;
    g.scan_mode = 0;
    g.nv = 0;
    g.vc = 0;
    g.quasi = QUASI_RECT;
    g.lc = 0;
    g.keep = 1;
    return &g;
}

static gdes*
nmc_3()
{
    static gdes_ll *ll;
    static gdes g;

    ll = &g.grid.ll;

    ll->ni = 360;
    ll->nj = 181;
    ll->di = 1.0;
    ll->dj = 1.0;
    ll->la1 = 90;
    ll->lo1 = 0;
    ll->la2 = -90;
    ll->lo2 = 359;
    ll->rot = 0;
    ll->strch = 0;
    g.type = GRID_LL;
    g.ncols = ll->ni;
    g.nrows = ll->nj;
    g.npts = g.ncols*g.nrows;
    g.res_flags = RESCMP_DIRINC ;
    g.scan_mode = 0;
    g.nv = 0;
    g.vc = 0;
    g.quasi = QUASI_RECT;
    g.lc = 0;
    g.keep = 1;
    return &g;
}


static gdes*
nmc_5()			/* N. Hemisphere polar stereographic oriented 105W */
{
    static gdes_polars *polars;
    static gdes g;

    polars = &g.grid.polars;

    polars->nx = 53;
    polars->ny = 57;
    polars->la1 = 7.64713;
    polars->lo1 = -133.443;
    polars->lov = -105;
    polars->dx = 109500.;
    polars->dy = 109500.;
    polars->pole = 0;
    g.type = GRID_POLARS;
    g.ncols = polars->nx;
    g.nrows = polars->ny;
    g.npts = g.ncols*g.nrows;
    g.res_flags = RESCMP_UVRES ;
    g.scan_mode = SCAN_J_PLUS ;
    g.nv = 0;
    g.vc = 0;
    g.quasi = QUASI_RECT;
    g.lc = 0;
    g.keep = 1;
    return &g;
}

static gdes*
nmc_6()			/* N. Hemisphere polar stereographic oriented 105W */
{
    static gdes_polars *polars;
    static gdes g;

    polars = &g.grid.polars;

    polars->nx = 53;
    polars->ny = 45;
    polars->la1 = 7.64713;
    polars->lo1 = -133.443;
    polars->lov = -105;
    polars->dx = 109500.;
    polars->dy = 109500.;
    polars->pole = 0;
    g.type = GRID_POLARS;
    g.ncols = polars->nx;
    g.nrows = polars->ny;
    g.npts = g.ncols*g.nrows;
    g.res_flags = RESCMP_UVRES ;
    g.scan_mode = SCAN_J_PLUS ;
    g.nv = 0;
    g.vc = 0;
    g.quasi = QUASI_RECT;
    g.lc = 0;
    g.keep = 1;
    return &g;
}

static gdes*
nmc_27()		/* N. Hemisphere polar stereographic oriented 80W */
{
    static gdes_polars *polars;
    static gdes g;

    polars = &g.grid.polars;

    polars->nx = 65;
    polars->ny = 65;
    polars->la1 = -20.8255;
    polars->lo1 = -125.;
    polars->lov = -80;
    polars->dx = 381000.;
    polars->dy = 381000.;
    polars->pole = 0;
    g.type = GRID_POLARS;
    g.ncols = polars->nx;
    g.nrows = polars->ny;
    g.npts = g.ncols*g.nrows;
    g.res_flags = RESCMP_UVRES ;
    g.scan_mode = SCAN_J_PLUS ;
    g.nv = 0;
    g.vc = 0;
    g.quasi = QUASI_RECT;
    g.lc = 0;
    g.keep = 1;
    return &g;
}

static gdes*
nmc_28()		/* S. Hemisphere polar stereographic oriented 100E */
{
    static gdes_polars *polars;
    static gdes g;

    polars = &g.grid.polars;

    polars->nx = 65;
    polars->ny = 65;
    polars->la1 = 20.8255;
    polars->lo1 = 145.;
    polars->lov = 100.;
    polars->dx = 381000.;
    polars->dy = 381000.;
    polars->pole = 1;
    g.type = GRID_POLARS;
    g.ncols = polars->nx;
    g.nrows = polars->ny;
    g.npts = g.ncols*g.nrows;
    g.res_flags = RESCMP_UVRES ;
    g.scan_mode = SCAN_J_PLUS ;
    g.nv = 0;
    g.vc = 0;
    g.quasi = QUASI_RECT;
    g.lc = 0;
    g.keep = 1;
    return &g;
}

static gdes*
nmc_100()		/* N. Hemisphere polar stereographic oriented 105W */
{
    static gdes_polars *polars;
    static gdes g;

    polars = &g.grid.polars;

    polars->nx = 83;
    polars->ny = 83;
    polars->la1 = 17.1101;
    polars->lo1 = -129.296;
    polars->lov = -105;
    polars->dx = 91452.;
    polars->dy = 91452.;
    polars->pole = 0;
    g.type = GRID_POLARS;
    g.ncols = polars->nx;
    g.nrows = polars->ny;
    g.npts = g.ncols*g.nrows;
    g.res_flags = RESCMP_UVRES ;
    g.scan_mode = SCAN_J_PLUS ;
    g.nv = 0;
    g.vc = 0;
    g.quasi = QUASI_RECT;
    g.lc = 0;
    g.keep = 1;
    return &g;
}

static gdes*
nmc_101()		/* N. Hemisphere polar stereographic oriented 105W */
{
    static gdes_polars *polars;
    static gdes g;

    polars = &g.grid.polars;

    polars->nx = 113;
    polars->ny = 91;
    polars->la1 = 10.52797;
    polars->lo1 = -137.146;
    polars->lov = -105;
    polars->dx = 91452.;
    polars->dy = 91452.;
    polars->pole = 0;
    g.type = GRID_POLARS;
    g.ncols = polars->nx;
    g.nrows = polars->ny;
    g.npts = g.ncols*g.nrows;
    g.res_flags = RESCMP_UVRES ;
    g.scan_mode = SCAN_J_PLUS ;
    g.nv = 0;
    g.vc = 0;
    g.quasi = QUASI_RECT;
    g.lc = 0;
    g.keep = 1;
    return &g;
}

static gdes*
nmc_104()		/* N. Hemisphere polar stereographic oriented 105W */
{
    static gdes_polars *polars;
    static gdes g;

    polars = &g.grid.polars;

    polars->nx = 147;
    polars->ny = 110;
    polars->la1 = -0.268327;
    polars->lo1 = -139.475;
    polars->lov = -105;
    polars->dx = 90754.64;
    polars->dy = 90754.64;
    polars->pole = 0;
    g.type = GRID_POLARS;
    g.ncols = polars->nx;
    g.nrows = polars->ny;
    g.npts = g.ncols*g.nrows;
    g.res_flags = RESCMP_UVRES ;
    g.scan_mode = SCAN_J_PLUS ;
    g.nv = 0;
    g.vc = 0;
    g.quasi = QUASI_RECT;
    g.lc = 0;
    g.keep = 1;
    return &g;
}

static gdes*
nmc_105()			/* N. Hemisphere polar stereographic oriented 105W */
{
    static gdes_polars *polars;
    static gdes g;

    polars = &g.grid.polars;

    polars->nx = 83;
    polars->ny = 83;
    polars->la1 = 17.529;
    polars->lo1 = -129.296;
    polars->lov = -105;
    polars->dx = 90754.64;
    polars->dy = 90754.64;
    polars->pole = 0;
    g.type = GRID_POLARS;
    g.ncols = polars->nx;
    g.nrows = polars->ny;
    g.npts = g.ncols*g.nrows;
    g.res_flags = RESCMP_UVRES ;
    g.scan_mode = SCAN_J_PLUS ;
    g.nv = 0;
    g.vc = 0;
    g.quasi = QUASI_RECT;
    g.lc = 0;
    g.keep = 1;
    return &g;
}

static gdes*
nmc_207()			/* Regional - Alaska (polar stereographic) */
{
    static gdes_polars *polars;
    static gdes g;

    polars = &g.grid.polars;

    polars->nx = 49;
    polars->ny = 35;
    polars->la1 = 42.085;
    polars->lo1 = -175.641;
    polars->lov = -150;
    polars->dx = 95250;
    polars->dy = 95250;
    polars->pole = 0;
    g.type = GRID_POLARS;
    g.ncols = polars->nx;
    g.nrows = polars->ny;
    g.npts = g.ncols*g.nrows;
    g.res_flags = RESCMP_UVRES ;
    g.scan_mode = SCAN_J_PLUS ;
    g.nv = 0;
    g.vc = 0;
    g.quasi = QUASI_RECT;
    g.lc = 0;
    g.keep = 1;
    return &g;
}

static gdes*
nmc_211()			/* Regional - CONUS (Lambert Conformal) */
{
    static gdes_lambert *lambert;
    static gdes g;

    lambert = &g.grid.lambert;

    lambert->nx = 93;
    lambert->ny = 65;
    lambert->la1 = 12.190;
    lambert->lo1 = -133.459;
    lambert->lov = -95;
    lambert->dx = 81270.5;
    lambert->dy = 81270.5;
    lambert->pole = 0;
    lambert->centers = 1;	/* not bipolar */
    lambert->latin1 = 25.0;
    lambert->latin2 = 25.0;	/* tangent cone */
    g.type = GRID_LAMBERT;
    g.ncols = lambert->nx;
    g.nrows = lambert->ny;
    g.npts = g.ncols*g.nrows;
    g.res_flags = RESCMP_UVRES ;
    g.scan_mode = SCAN_J_PLUS ;
    g.nv = 0;
    g.vc = 0;
    g.quasi = QUASI_RECT;
    g.lc = 0;
    g.keep = 1;
    return &g;
}

static gdes*
nmc_212()			/* Regional - CONUS (Lambert Conformal) */
{
    static gdes_lambert *lambert;
    static gdes g;

    lambert = &g.grid.lambert;

    lambert->nx = 185;
    lambert->ny = 129;
    lambert->la1 = 12.190;
    lambert->lo1 = -133.459;
    lambert->lov = -95;
    lambert->dx = 40635;
    lambert->dy = 40635;
    lambert->pole = 0;
    lambert->centers = 1;	/* not bipolar */
    lambert->latin1 = 25.0;
    lambert->latin2 = 25.0;	/* tangent cone */
    g.type = GRID_LAMBERT;
    g.ncols = lambert->nx;
    g.nrows = lambert->ny;
    g.npts = g.ncols*g.nrows;
    g.res_flags = RESCMP_UVRES ;
    g.scan_mode = SCAN_J_PLUS ;
    g.nv = 0;
    g.vc = 0;
    g.quasi = QUASI_RECT;
    g.lc = 0;
    g.keep = 1;
    return &g;
}


/*
 * Sythesize a gdes from the center ID and grid ID.
 * Returns 0 if failed.
 */
static gdes*
synth_gdes(
    int centerid,
    int gridid)
{
    /* If it's an international exchange grid, it doesn't matter what center
       it's from */
    switch(gridid) {
    case 21: return nmc_21();
    case 22: return nmc_22();
    case 23: return nmc_23();
    case 24: return nmc_24();
    case 25: return nmc_25();
    case 26: return nmc_26();
    case 50: return nmc_50();
    case 61: return nmc_61();
    case 62: return nmc_62();
    case 63: return nmc_63();
    case 64: return nmc_64();
    }

    switch(centerid) {
    case CENTER_NMC:
	switch(gridid) {

         case  1: return nmc_1();
         case  2: return nmc_2();
         case  3: return nmc_3(); /* and so on ..., but we don't see these on
				     HRS, so we'll finish them later */
/*         case  4: return nmc_4(); */
         case  5: return nmc_5();
         case  6: return nmc_6();
         case  27: return nmc_27();
         case  28: return nmc_28();
/*         case  29: return nmc_29(); */
/*         case  30: return nmc_30(); */
/*         case  33: return nmc_33(); */
/*         case  34: return nmc_34(); */
/*         case  45: return nmc_45(); */
/*         case  55: return nmc_55(); */
/*         case  56: return nmc_56(); */
/*         case  67: return nmc_67(); */
/*         case  68: return nmc_68(); */
/*         case  69: return nmc_69(); */
/*         case  70: return nmc_70(); */
/*         case  71: return nmc_71(); */
/*         case  72: return nmc_72(); */
/*         case  73: return nmc_73(); */
/*         case  74: return nmc_74(); */
/*         case  75: return nmc_75(); */
/*         case  76: return nmc_76(); */
/*         case  77: return nmc_77(); */
/*         case  85: return nmc_85(); */
/*         case  86: return nmc_86(); */
/*         case  87: return nmc_87(); */
/*         case  90: return nmc_90(); */
/*         case  91: return nmc_91(); */
/*         case  92: return nmc_92(); */
/*         case  93: return nmc_93(); */
/*         case  94: return nmc_94(); */
/*         case  95: return nmc_95(); */
/*         case  98: return nmc_98(); */
        case  100: return nmc_100();
        case  101: return nmc_101();
/*         case  103: return nmc_103(); */
        case  104: return nmc_104();
	case  105: return nmc_105();
/*         case  106: return nmc_106(); */
/*         case  107: return nmc_107(); */
/*         case  126: return nmc_126(); */
/*         case  201: return nmc_201(); */
/*         case  202: return nmc_202(); */
/*         case  203: return nmc_203(); */
/*         case  204: return nmc_204(); */
/*         case  205: return nmc_205(); */
/*         case  206: return nmc_206(); */
        case  207: return nmc_207();
/*         case  208: return nmc_208(); */
/*         case  209: return nmc_209(); */
/*         case  210: return nmc_210(); */
        case  211: return nmc_211();
        case  212: return nmc_212();
/*         case  213: return nmc_213(); */
/*         case  214: return nmc_214(); */
	default:
	    logFile->write_time("Error: unrecognized NMC grid id %d\n", gridid );
	    return 0;
	}
    case CENTER_FNOC:
	switch (gridid) {
/*         case  220: return fnoc_220(); */
/*         case  221: return fnoc_221(); */
/*         case  223: return fnoc_223(); */
	default:
	    logFile->write_time("Error: unrecognized FNOC grid id %d\n", gridid );
	    return 0;
	}
    case CENTER_ECMWF:
	switch (gridid) {	/* These come with Grid Description Sections */
/*         case 1: return ecmwf_1(); */
/*         case 2: return ecmwf_2(); */
/*         case 3: return ecmwf_3(); */
/*         case 4: return ecmwf_4(); */
/*         case 5: return ecmwf_5(); */
/*         case 6: return ecmwf_6(); */
/*         case 7: return ecmwf_7(); */
/*         case 8: return ecmwf_8(); */
/*         case 9: return ecmwf_9(); */
/*         case 10: return ecmwf_10(); */
/*         case 11: return ecmwf_11(); */
/*         case 12: return ecmwf_12(); */
/*         case 13: return ecmwf_13(); */
/*         case 14: return ecmwf_14(); */
/*         case 15: return ecmwf_15(); */
/*         case 16: return ecmwf_16(); */
	default:
	    logFile->write_time("Error: unrecognized ECMWF grid id %d\n", gridid );
	    return 0;
	}
    default:
	logFile->write_time("Error: unrecognized (center,grid) combination: (%d,%d)\n",
	       centerid, gridid);
	return 0;
    }
}



/*
 * Make a gdes from raw gds or pds.  Returns 0 on failure.
 */
gdes*
make_gdes(
    grib1 *gb)
{

    if (gb->gdsp) {		/* If GDS exists, use it */
	return gds_to_gdes(gb->gdsp);
    } else if(gb->pdsp->grid == NONCATALOGED_GRID) {
	logFile->write_time("Error: grid id = 255, but no Grid Description Section\n");
	return 0;
    } else {			/* Otherwise, manufacture from PDS center,
				   model */
	return synth_gdes(g1i(gb->pdsp->center), g1i(gb->pdsp->grid));
    }
}

/*
 * Free gdes, unless statically allocated
 */
void
free_gdes(
    gdes *gd)
{

    if (gd) {
	if(gd->keep)
	    return;
	if (gd->vc)
	    free(gd->vc);
	if (gd->lc)
	    free(gd->lc);
	switch(gd->type) {	/* free type-specific stuff */
	case GRID_LL:
	    break;
	case GRID_RLL:
	    if(gd->grid.ll.rot)
		free(gd->grid.ll.rot);
	    break;
	case GRID_SLL:
	    if(gd->grid.ll.strch)
		free(gd->grid.ll.strch);
	    break;
	case GRID_SRLL:
	    if(gd->grid.ll.rot)
		free(gd->grid.ll.rot);
	    if(gd->grid.ll.strch)
		free(gd->grid.ll.strch);
	    break;
	case GRID_GAU:
	    break;
	case GRID_RGAU:
	    if(gd->grid.gau.rot)
		free(gd->grid.gau.rot);
	    break;
	case GRID_SGAU:
	    if(gd->grid.gau.strch)
		free(gd->grid.gau.strch);
	    break;
	case GRID_SRGAU:
	    if(gd->grid.gau.rot)
		free(gd->grid.gau.rot);
	    if(gd->grid.gau.strch)
		free(gd->grid.gau.strch);
	    break;
	case GRID_SPH:
	    break;
	case GRID_RSPH:
	    if(gd->grid.sph.rot)
		free(gd->grid.sph.rot);
	    break;
	case GRID_SSPH:
	    if(gd->grid.sph.strch)
		free(gd->grid.sph.strch);
	    break;
	case GRID_SRSPH:
	    if(gd->grid.sph.rot)
		free(gd->grid.sph.rot);
	    if(gd->grid.sph.strch)
		free(gd->grid.sph.strch);
	    break;
	case GRID_MERCAT:
	case GRID_POLARS:
	case GRID_GNOMON:
	case GRID_LAMBERT:
	case GRID_ALBERS:
	case GRID_OLAMBERT:
	case GRID_SPACEV:
	case GRID_UTM:
	case GRID_SIMPOL:
	case GRID_MILLER:
	default:
	    break;
	}
	free(gd);
    }
}


/*
 * return string describing type of grid projection
 */
char*
gds_typename(int type)
{
	switch(type) {
	case GRID_LL:
	    return (char *)"Latitude/Longitude";
	case GRID_RLL:
	    return (char *)"Rotated latitude/longitude";
	case GRID_SLL:
	    return (char *)"Stretched latitude/longitude";
	case GRID_SRLL:
	    return (char *)"Stretched and rotated latitude/longitude";
	case GRID_GAU:
	    return (char *)"Gaussian latitude/longitude";
	case GRID_RGAU:
	    return (char *)"Rotated Gaussian latitude/longitude";
	case GRID_SGAU:
	    return (char *)"Stretched Gaussian latitude/longitude";
	case GRID_SRGAU:
	    return (char *)"Stretched and rotated Gaussian latitude/longitude";
	case GRID_SPH:
	    return (char *)"Spherical harmonic coefficients";
	case GRID_RSPH:
	    return (char *)"Rotated spherical harmonics";
	case GRID_SSPH:
	    return (char *)"Stretched spherical harmonics";
	case GRID_SRSPH:
	    return (char *)"Stretched and rotated spherical harmonics";
	case GRID_MERCAT:
	    return (char *)"Mercator projection";
	case GRID_POLARS:
	    return (char *)"Polar stereographic projection";
	case GRID_GNOMON:
	    return (char *)"Gnomonic projection";
	case GRID_LAMBERT:
	    return (char *)"Lambert conformal projection";
	case GRID_ALBERS:
	    return (char *)"Albers equal-area projection";
	case GRID_OLAMBERT:
	    return (char *)"Oblique Lambert projection";
	case GRID_SPACEV:
	    return (char *)"Space view";
	case GRID_UTM:
	    return (char *)"Universal Transverse Mercator (UTM) projection";
	case GRID_SIMPOL:
	    return (char *)"Simple polyconic projection";
	case GRID_MILLER:
	    return (char *)"Miller's cylindrical projection";
	default:
	    return (char *)"Unknown GRIB GDS data representation type";
	}
}


/*
 * Create a gdes from a GRIB-2 gdt. Information for this translation
 * was taken from the NCEP cnvgrib program.
 */

gdes *gdt_to_gdes(GRIB2::gribfield *g2fld)
{

  gdes *gd = (gdes *) emalloc(sizeof(gdes));
  
  // Set some defaults
  gd->nv = 0;
  gd->vc = 0;
  gd->quasi = QUASI_RECT;
  gd->lc = 0;
  gd->maxlc = 0;
  gd->keep = 0;


  // Fill in the grid structures for various grids

  int gtype = g2fld->igdtnum;

  // Lat/Lon Grid ------------------------------------------

  if (gtype == 0) {

    // Grid geometry
    gd->type = 0;
    gd->ncols = g2fld->igdtmpl[7];
    gd->nrows = g2fld->igdtmpl[8];
    gd->npts = g2fld->ngrdpts;
    gd->grid.ll.ni = gd->ncols;
    gd->grid.ll.nj = gd->nrows;
    gd->grid.ll.la1 = g2fld->igdtmpl[11]/1000000.;
    gd->grid.ll.lo1 = g2fld->igdtmpl[12]/1000000.;
    gd->grid.ll.la2 = g2fld->igdtmpl[14]/1000000.;
    gd->grid.ll.lo2 = g2fld->igdtmpl[15]/1000000.;
    gd->grid.ll.di = g2fld->igdtmpl[16]/1000000.;
    gd->grid.ll.dj = g2fld->igdtmpl[17]/1000000.;
    gd->grid.ll.rot = 0;
    gd->grid.ll.strch = 0;

    // Resolution and component flags
    gd->res_flags = 0;
    if (g2fld->igdtmpl[0] == 2)
      gd->res_flags = 64;
    if ((g2fld->igdtmpl[13] & (1<<4)) ||
	(g2fld->igdtmpl[13] & (1<<5))) 
      gd->res_flags = gd->res_flags + 128;
    if ((g2fld->igdtmpl[13] & (1<<3)))
      gd->res_flags = gd->res_flags + 8;

    // Scan mode
    gd->scan_mode = g2fld->igdtmpl[18];
  }

  // Rotated lat/lon -----------------------------------------

  else if (gtype == 1) {

    // Grid geometry
    gd->type = 10;
    gd->ncols = g2fld->igdtmpl[7];
    gd->nrows = g2fld->igdtmpl[8];
    gd->npts = g2fld->ngrdpts;
    gd->grid.ll.ni = gd->ncols;
    gd->grid.ll.nj = gd->nrows;
    gd->grid.ll.la1 = g2fld->igdtmpl[11]/1000000.;
    gd->grid.ll.lo1 = g2fld->igdtmpl[12]/1000000.;
    gd->grid.ll.la2 = g2fld->igdtmpl[14]/1000000.;
    gd->grid.ll.lo2 = g2fld->igdtmpl[15]/1000000.;
    gd->grid.ll.di = g2fld->igdtmpl[16]/1000000.;
    gd->grid.ll.dj = g2fld->igdtmpl[17]/1000000.;
    gd->grid.ll.rot = (rotated *)emalloc(sizeof(rotated));
    gd->grid.ll.rot->lat = g2fld->igdtmpl[19]/1000000.;
    gd->grid.ll.rot->lon = g2fld->igdtmpl[20]/1000000.;
    gd->grid.ll.rot->angle = g2fld->igdtmpl[21]/1000000.;
    gd->grid.ll.strch = 0;

    // Resolution and component flags
    gd->res_flags = 0;
    if (g2fld->igdtmpl[0] == 2)
      gd->res_flags = 64;
    if ((g2fld->igdtmpl[13] & (1<<4)) ||
	(g2fld->igdtmpl[13] & (1<<5))) 
      gd->res_flags = gd->res_flags + 128;
    if ((g2fld->igdtmpl[13] & (1<<3)))
      gd->res_flags = gd->res_flags + 8;

    // Scan mode
    gd->scan_mode = g2fld->igdtmpl[18];

    // Check on a non-zero rotation angle. This is currently not
    // supported. (It needs to be added in make_site_data().)
    if (gd->grid.ll.rot->angle != 0.0) {
      logFile->write_time("Error: Cannot handle rotated lat-lon grid with non-zero angle (%d)\n", gd->grid.ll.rot->angle);
      free(gd);
      gd = 0;
    }
  }

  // Mercator grid -----------------------------------------

  else if (gtype == 10) {

    // Grid geometry
    gd->type = 1;
    gd->ncols = g2fld->igdtmpl[7];
    gd->nrows = g2fld->igdtmpl[8];
    gd->npts = g2fld->ngrdpts;
    gd->grid.mercator.ni = gd->ncols;
    gd->grid.mercator.nj = gd->nrows;
    gd->grid.mercator.la1 = g2fld->igdtmpl[9]/1000000.;
    gd->grid.mercator.lo1 = g2fld->igdtmpl[10]/1000000.;
    gd->grid.mercator.la2 = g2fld->igdtmpl[13]/1000000.;
    gd->grid.mercator.lo2 = g2fld->igdtmpl[14]/1000000.;
    gd->grid.mercator.latin = g2fld->igdtmpl[12]/1000.;
    gd->grid.mercator.di = g2fld->igdtmpl[17]/1000.;
    gd->grid.mercator.dj = g2fld->igdtmpl[18]/1000.;

    // Resolution and component flags
    gd->res_flags = 0;
    if (g2fld->igdtmpl[0] == 2)
      gd->res_flags = 64;
    if ((g2fld->igdtmpl[11] & (1<<4)) ||
	(g2fld->igdtmpl[11] & (1<<5))) 
      gd->res_flags = gd->res_flags + 128;
    if ((g2fld->igdtmpl[11] & (1<<3)))
      gd->res_flags = gd->res_flags + 8;

    // Scan mode
    gd->scan_mode = g2fld->igdtmpl[15];
  }

  // Polar Stereographic grid ------------------------------

  else if (gtype == 20) {

    // Grid geometry
    gd->type = 5;
    gd->ncols = g2fld->igdtmpl[7];
    gd->nrows = g2fld->igdtmpl[8];
    gd->npts = g2fld->ngrdpts;
    gd->grid.polars.nx = gd->ncols;
    gd->grid.polars.ny = gd->nrows;
    gd->grid.polars.la1 = g2fld->igdtmpl[9]/1000000.;
    gd->grid.polars.lo1 = g2fld->igdtmpl[10]/1000000.;
    gd->grid.polars.lov = g2fld->igdtmpl[13]/1000000.;
    gd->grid.polars.dx = g2fld->igdtmpl[14]/1000.;
    gd->grid.polars.dy = g2fld->igdtmpl[15]/1000.;
    gd->grid.polars.pole = g2fld->igdtmpl[16];

    // Resolution and component flags
    gd->res_flags = 0;
    if (g2fld->igdtmpl[0] == 2)
      gd->res_flags = 64;
    if ((g2fld->igdtmpl[11] & (1<<4)) ||
	(g2fld->igdtmpl[11] & (1<<5))) 
      gd->res_flags = gd->res_flags + 128;
    if ((g2fld->igdtmpl[11] & (1<<3)))
      gd->res_flags = gd->res_flags + 8;

    // Scan mode
    gd->scan_mode = g2fld->igdtmpl[17];
  }

  // Lambert Conformal grid --------------------------------

  else if (gtype == 30) {

    // Grid geometry
    gd->type = 3;
    gd->ncols = g2fld->igdtmpl[7];
    gd->nrows = g2fld->igdtmpl[8];
    gd->npts = g2fld->ngrdpts;
    gd->grid.lambert.nx = gd->ncols;
    gd->grid.lambert.ny = gd->nrows;
    gd->grid.lambert.la1 = g2fld->igdtmpl[9]/1000000.;
    gd->grid.lambert.lo1 = g2fld->igdtmpl[10]/1000000.;
    gd->grid.lambert.lov = g2fld->igdtmpl[13]/1000000.;
    gd->grid.lambert.dx = g2fld->igdtmpl[14]/1000.;
    gd->grid.lambert.dy = g2fld->igdtmpl[15]/1000.;
    gd->grid.lambert.pole = g2fld->igdtmpl[16];
    gd->grid.lambert.centers = 1;
    gd->grid.lambert.latin1 = g2fld->igdtmpl[18]/1000000.;
    gd->grid.lambert.latin2 = g2fld->igdtmpl[19]/1000000.;
    gd->grid.lambert.splat = g2fld->igdtmpl[20]/1000000.;
    gd->grid.lambert.splon = g2fld->igdtmpl[21]/1000000.;

    // Resolution and component flags
    gd->res_flags = 0;
    if (g2fld->igdtmpl[0] == 2)
      gd->res_flags = 64;
    if ((g2fld->igdtmpl[11] & (1<<4)) ||
	(g2fld->igdtmpl[11] & (1<<5))) 
      gd->res_flags = gd->res_flags + 128;
    if ((g2fld->igdtmpl[11] & (1<<3)))
      gd->res_flags = gd->res_flags + 8;

    // Scan mode
    gd->scan_mode = g2fld->igdtmpl[17];
  }

  // Gaussian Lat/Lon grid ---------------------------------

  else if (gtype == 40) {

    // Grid geometry
    gd->type = 4;
    gd->ncols = g2fld->igdtmpl[7];
    gd->nrows = g2fld->igdtmpl[8];
    gd->npts = g2fld->ngrdpts;
    gd->grid.gau.ni = gd->ncols;
    gd->grid.gau.nj = gd->nrows;
    gd->grid.gau.la1 = g2fld->igdtmpl[11]/1000000.;
    gd->grid.gau.lo1 = g2fld->igdtmpl[12]/1000000.;
    gd->grid.gau.la2 = g2fld->igdtmpl[14]/1000000.;
    gd->grid.gau.lo2 = g2fld->igdtmpl[15]/1000000.;
    gd->grid.gau.di = g2fld->igdtmpl[16]/1000000.;
    gd->grid.gau.n = g2fld->igdtmpl[17];
    gd->grid.gau.rot = 0;
    gd->grid.gau.strch = 0;

    // Resolution and component flags
    gd->res_flags = 0;
    if (g2fld->igdtmpl[0] == 2)
      gd->res_flags = 64;
    if ((g2fld->igdtmpl[11] & (1<<4)) ||
	(g2fld->igdtmpl[11] & (1<<5))) 
      gd->res_flags = gd->res_flags + 128;
    if ((g2fld->igdtmpl[11] & (1<<3)))
      gd->res_flags = gd->res_flags + 8;

    // Scan mode
    gd->scan_mode = g2fld->igdtmpl[18];
  }

  // Not supported, print an error.
				      
  else {
    logFile->write_time("Error: Cannot handle grid template %d\n", gtype);
    free(gd);
    gd = 0;
  }


  // Process vertical coord parameters

  gd->nv = g2fld->num_coord;
  if (gd->nv != 0) {
    gd->vc = (float *) emalloc(gd->nv * sizeof(float));
    for (int i=0; i<gd->nv; i++) {
      gd->vc[i] = g2fld->coord_list[i];
    }
  }


  // Set up grid geometry for irregular grids

  if (g2fld->num_opt != 0) {

    // Grid with varying rows
    if (gd->ncols == -1) {

      gd->quasi = QUASI_ROWS;
      gd->ncols = 1;
      gd->lc = (int *)emalloc((1 + gd->nrows) * sizeof(int));

      for (int r=0; r<g2fld->num_opt; r++ ) {
	gd->lc[r] = g2fld->list_opt[r];
	if (gd->lc[r] > gd->maxlc)
	  gd->maxlc = gd->lc[r];
      }
    }

    // Grid with varying columns
    else if (gd->nrows == -1) {
      
      gd->quasi = QUASI_COLS;
      gd->nrows = 1;
      gd->lc = (int *)emalloc((1 + gd->ncols) * sizeof(int));

      for (int c=0; c<g2fld->num_opt; c++ ) {
	gd->lc[c] = g2fld->list_opt[c];
	if (gd->lc[c] > gd->maxlc)
	  gd->maxlc = gd->lc[c];
      }
    }
    else {
      logFile->write_time("Error: Irregular grid but nrows (%d) and ncols (%d) != -1\n", gd->nrows, gd->ncols);
      free(gd);
      gd = 0;      
    }

  }

  //gd->keep = 1;
  return (gd);
}



/*
 * Dump gdes in text form
 */
void
print_gdes(
    gdes *gd)
{
    printf("   %24s : %d (%s)\n","GDS representation type",
	   gd->type, gds_typename(gd->type));
    printf("   %24s : %d\n","Number of columns",
	   gd->ncols);	
    printf("   %24s : %d\n","Number of rows",
	   gd->nrows);	
    printf("   %24s : %d\n","Number of points",
	   gd->npts);
    printf("   %24s : ","Kind of grid");
    switch(gd->quasi) {
    case QUASI_RECT:
	printf("rectangular\n");
	break;
    case QUASI_ROWS:
	printf("quasi-regular (varying rows)\n");
	printf("   %24s : ","Row lengths");
	{
	    int ii, *ip;
	    for (ii=0, ip=gd->lc; ii < gd->nrows; ii++) {
		printf("%d ", *(ip+1) - *ip);
		ip++;
		if(ii%16 == 15 && ii < gd->nrows-1)
		    printf("\n   %24s   ", "");
	    }
	    printf("\n");
	}
	break;
    case QUASI_COLS:
	printf("quasi-regular (varying columns)\n");
	{
	    int ii, *ip;
	    for (ii=0, ip=gd->lc; ii < gd->ncols; ii++) {
		printf("%d ", *(ip+1) - *ip);
		ip++;
		if(ii%16 == 15 && ii < gd->ncols-1)
		    printf("\n   %24s   ", "");
	    }
	    printf("\n");
	}
	break;
    default:
	printf("invalid code for quasi-regularity, %d\n", gd->quasi);
    }
    printf("   %24s : %#x\n", "GDS res/comp flag", gd->res_flags);
    printf("   %24s : %d\n", "GDS scan mode flag", gd->scan_mode);
    printf("   %24s : %d\n", "GDS no. of vert. coords", gd->nv);
    switch (gd->type) {
    case GRID_LL:		/* fall through */
    case GRID_RLL:		/* fall through */
    case GRID_SLL:		/* fall through */
    case GRID_SRLL:
    {
	gdes_ll *gg = &gd->grid.ll;
	printf("   %24s : %d\n", "GDS Ni", gg->ni);
	printf("   %24s : %d\n", "GDS Nj", gg->nj);
	printf("   %24s : %f\n", "GDS La1", gg->la1);
	printf("   %24s : %f\n", "GDS Lo1", gg->lo1);
	printf("   %24s : %f\n", "GDS La2", gg->la2);
	printf("   %24s : %f\n", "GDS Lo2", gg->lo2);
	printf("   %24s : %f\n", "GDS Di", gg->di);
	printf("   %24s : %f\n", "GDS Dj", gg->dj);
	if (gg->rot) {
	    printf("   %24s : %f\n", "GDS Lat of S. pole of rotation",
		   gg->rot->lat);
	    printf("   %24s : %f\n", "GDS Lon of S. pole of rotation",
		   gg->rot->lon);
	    printf("   %24s : %f\n", "GDS Angle of rotation",
		   gg->rot->angle);
	}
	if (gg->strch) {
	    printf("   %24s : %f\n", "GDS Lat of S. pole of stretching",
		   gg->strch->lat);
	    printf("   %24s : %f\n", "GDS Lon of S. pole of stretching",
		   gg->strch->lon);
	    printf("   %24s : %f\n", "GDS Stretching factor",
		   gg->strch->factor);
	}
    }
	break;
    case GRID_GAU:		/* fall through */
    case GRID_RGAU:		/* fall through */
    case GRID_SGAU:		/* fall through */
    case GRID_SRGAU:		/* fall through */
    {
	gdes_gau *gg = &gd->grid.gau;
	printf("   %24s : %d\n", "GDS Ni", gg->ni);
	printf("   %24s : %d\n", "GDS Nj", gg->nj);
	printf("   %24s : %f\n", "GDS La1", gg->la1);
	printf("   %24s : %f\n", "GDS Lo1", gg->lo1);
	printf("   %24s : %f\n", "GDS La2", gg->la2);
	printf("   %24s : %f\n", "GDS Lo2", gg->lo2);
	printf("   %24s : %f\n", "GDS Di", gg->di);
	printf("   %24s : %d\n", "GDS n", gg->n);
	if (gg->rot) {
	    printf("   %24s : %f\n", "GDS Lat of S. pole of rotation",
		   gg->rot->lat);
	    printf("   %24s : %f\n", "GDS Lon of S. pole of rotation",
		   gg->rot->lon);
	    printf("   %24s : %f\n", "GDS Angle of rotation",
		   gg->rot->angle);
	}
	if (gg->strch) {
	    printf("   %24s : %f\n", "GDS Lat of S. pole of stretching",
		   gg->strch->lat);
	    printf("   %24s : %f\n", "GDS Lon of S. pole of stretching",
		   gg->strch->lon);
	    printf("   %24s : %f\n", "GDS Stretching factor",
		   gg->strch->factor);
	}
    }
    break;
    case GRID_SPH:
    case GRID_RSPH:
    case GRID_SSPH:
    case GRID_SRSPH:
    {
	gdes_sph *gg = &gd->grid.sph;
	printf("   %24s : %d\n", "GDS j", gg->j);
	printf("   %24s : %d\n", "GDS k", gg->k);
	printf("   %24s : %d\n", "GDS m", gg->m);
	printf("   %24s : %d\n", "GDS type", gg->type);
	printf("   %24s : %d\n", "GDS mode", gg->mode);
	if (gg->rot) {
	    printf("   %24s : %f\n", "GDS Lat of S. pole of rotation",
		   gg->rot->lat);
	    printf("   %24s : %f\n", "GDS Lon of S. pole of rotation",
		   gg->rot->lon);
	    printf("   %24s : %f\n", "GDS Angle of rotation",
		   gg->rot->angle);
	}
	if (gg->strch) {
	    printf("   %24s : %f\n", "GDS Lat of S. pole of stretching",
		   gg->strch->lat);
	    printf("   %24s : %f\n", "GDS Lon of S. pole of stretching",
		   gg->strch->lon);
	    printf("   %24s : %f\n", "GDS Stretching factor",
		   gg->strch->factor);
	}
    }
    break;
    case GRID_MERCAT:
    {
	gdes_mercator *gg = &gd->grid.mercator;
	printf("   %24s : %d\n", "GDS Ni", gg->ni);
	printf("   %24s : %d\n", "GDS Nj", gg->nj);
	printf("   %24s : %f\n", "GDS La1", gg->la1);
	printf("   %24s : %f\n", "GDS Lo1", gg->lo1);
	printf("   %24s : %f\n", "GDS La2", gg->la2);
	printf("   %24s : %f\n", "GDS Lo2", gg->lo2);
	printf("   %24s : %.3f\n", "GDS Latin", gg->latin);
	printf("   %24s : %.3f\n", "GDS Di", gg->di);
	printf("   %24s : %.3f\n", "GDS Dj", gg->dj);
    }
    break;
    case GRID_GNOMON:		/* fall through */
    case GRID_POLARS:
    {
	gdes_polars *gg = &gd->grid.polars;
	printf("   %24s : %d\n", "GDS Nx", gg->nx);
	printf("   %24s : %d\n", "GDS Ny", gg->ny);
	printf("   %24s : %f\n", "GDS La1", gg->la1);
	printf("   %24s : %f\n", "GDS Lo1", gg->lo1);
	printf("   %24s : %f\n", "GDS Lov", gg->lov);
	printf("   %24s : %.3f\n", "GDS Dx", gg->dx);
	printf("   %24s : %.3f\n", "GDS Dy", gg->dy);
	printf("   %24s : %s\n", "GDS Pole in proj. plane",
	       gg->pole == 0 ? "North" : "South");
    }
	break;
    case GRID_LAMBERT:
    {
	gdes_lambert *gg = &gd->grid.lambert;
	printf("   %24s : %d\n", "GDS Nx", gg->nx);
	printf("   %24s : %d\n", "GDS Ny", gg->ny);
	printf("   %24s : %f\n", "GDS La1", gg->la1);
	printf("   %24s : %f\n", "GDS Lo1", gg->lo1);
	printf("   %24s : %f\n", "GDS Lov", gg->lov);
	printf("   %24s : %.3f\n", "GDS Dx", gg->dx);
	printf("   %24s : %.3f\n", "GDS Dy", gg->dy);
	printf("   %24s : %s\n", "GDS Pole in proj. plane",
	       gg->pole == 0 ? "North" : "South");
	printf("   %24s : %d\n", "GDS centers", gg->centers);
	printf("   %24s : %f\n", "GDS Latin1", gg->latin1);
	printf("   %24s : %f\n", "GDS Latin2", gg->latin2);
	printf("   %24s : %f\n", "GDS Splat", gg->splat);
	printf("   %24s : %f\n", "GDS SPlon", gg->splon);
    }
	break;
    case GRID_SPACEV:
    {
	gdes_spacev *gg = &gd->grid.spacev;
	printf("   %24s : %d\n", "GDS Nx", gg->nx);
	printf("   %24s : %d\n", "GDS Ny", gg->ny);
	printf("   %24s : %f\n", "GDS Lap", gg->lap);
	printf("   %24s : %f\n", "GDS Lop", gg->lop);
	printf("   %24s : %f\n", "GDS dx", gg->dx);
	printf("   %24s : %f\n", "GDS dy", gg->dy);
	printf("   %24s : %f\n", "GDS Xp", gg->xp);
	printf("   %24s : %f\n", "GDS Yp", gg->yp);
	printf("   %24s : %f\n", "GDS Orientation", gg->orient);
	printf("   %24s : %f\n", "GDS Nr", gg->nr);
	printf("   %24s : %f\n", "GDS Xo", gg->xo);
	printf("   %24s : %f\n", "GDS Yo", gg->yo);
    }
    break;
    case GRID_ALBERS:
    case GRID_OLAMBERT:
    case GRID_UTM:
    case GRID_SIMPOL:
    case GRID_MILLER:
    default:
	break;
    }
}
