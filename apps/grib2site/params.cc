/*
 *	Copyright 1991 University Corporation for Atmospheric Research
 *	         All copies to include this notice.
 */
/* "$Id: params.cc,v 1.23 2017/06/22 20:00:02 dicast Exp $" */

#include <stdlib.h>
#include "log/log.hh"
#include <string.h>
#include "params.h"
#include "g21params.h"

#ifdef __STDC__
static void init_lookup(void);
#endif

#ifndef STREQ
#define STREQ(a, b) (*(a) == *(b) && strcmp((a), (b)) == 0)
#endif

extern Log *logFile;

struct param_table {
    int code;			/* GRIB code for parameter */
    char* name;			/* parameter name for netCDF files */
    char* units;		/* GRIB units as understood by udunits(3) */
};

struct param_table ptable[] = {
    {PARM_RESERVED,	(char *)"reserved",	(char *)"none"},
    {PARM_PRESSURE,	(char *)"P",		(char *)"Pa"},
    {PARM_PMSL,		(char *)"P_msl",	(char *)"Pa"},
    {PARM_PTND,		(char *)"Ptend",	(char *)"Pa/s"},
    {PARM_ICAHT,	(char *)"icaht",	(char *)"m"},
    {PARM_GPT,		(char *)"gpt",		(char *)"m2/s2"},
    {PARM_GPT_HGT,	(char *)"Z",		(char *)"gp m"},
    {PARM_GEOM_HGT,	(char *)"alt",		(char *)"m"},
    {PARM_HSTDV,	(char *)"hstdv",	(char *)"m"},
    {PARM_TOZNE,	(char *)"totoz",	(char *)"Dobson"},
    {PARM_TEMP,		(char *)"T",		(char *)"degK"},
    {PARM_VTEMP,	(char *)"Tv",		(char *)"degK"},
    {PARM_POT_TEMP,	(char *)"theta",	(char *)"degK"},
    {PARM_APOT_TEMP,	(char *)"thpa",		(char *)"degK"},
    {PARM_MAX_TEMP,	(char *)"Tmax",		(char *)"degK"},
    {PARM_MIN_TEMP,	(char *)"Tmin",		(char *)"degK"},
    {PARM_DP_TEMP,	(char *)"TD",		(char *)"degK"},
    {PARM_DP_DEP,	(char *)"T_TD",		(char *)"degK"},
    {PARM_LAPSE,	(char *)"dTdz",		(char *)"degK/m"},

    {PARM_VIS,		(char *)"vis",		(char *)"m"},

    {PARM_RAD1,		(char *)"radspec_df",	(char *)"none"},
    {PARM_RAD2,		(char *)"radspec_dr",	(char *)"none"},
    {PARM_RAD3,		(char *)"radspec_rr",	(char *)"none"},
    {PARM_PLI,		(char *)"pli",		(char *)"K"},
    {PARM_TANOM,	(char *)"Tdev",		(char *)"degK"},
    {PARM_PANOM,	(char *)"Pdev",		(char *)"Pa"},
    {PARM_ZANOM,	(char *)"Zdev",		(char *)"gp m"},
    {PARM_WAV1,		(char *)"wavspec_df",	(char *)"none"},
    {PARM_WAV2,		(char *)"wavspec_dr",	(char *)"none"},
    {PARM_WAV3,		(char *)"wavspec_rr",	(char *)"none"},
    {PARM_WND_DIR,	(char *)"DIR",		(char *)"degrees_true"},
    {PARM_WND_SPEED,	(char *)"SPD",		(char *)"m/s"},
    {PARM_U_WIND,	(char *)"u",		(char *)"m/s"},
    {PARM_V_WIND,	(char *)"v",		(char *)"m/s"},
    {PARM_STRM_FUNC,	(char *)"strm_func",	(char *)"m2/s"},
    {PARM_VPOT,		(char *)"velpot",	(char *)"m2/s"},

    {PARM_MNTSF,	(char *)"mntsf",	(char *)"m2/s2"},

    {PARM_SIG_VEL,	(char *)"sigvvel",	(char *)"1/s"},
    {PARM_VERT_VEL,	(char *)"omega",	(char *)"Pa/s"},
    {PARM_GEOM_VEL,	(char *)"w",		(char *)"m/s"},
    {PARM_ABS_VOR,	(char *)"absvor",	(char *)"1/s"},
    {PARM_ABS_DIV,	(char *)"absdiv",	(char *)"1/s"},
    {PARM_REL_VOR,	(char *)"relvor",	(char *)"1/s"},
    {PARM_REL_DIV,	(char *)"reldiv",	(char *)"1/s"},
    {PARM_U_SHR,	(char *)"dudz",		(char *)"1/s"},
    {PARM_V_SHR,	(char *)"dvdz",		(char *)"1/s"},
    {PARM_CRNT_DIR,	(char *)"crnt_dir",	(char *)"degrees_true"},
    {PARM_CRNT_SPD,	(char *)"crnt_spd",	(char *)"m/s"},
    {PARM_U_CRNT,	(char *)"u_crnt",	(char *)"m/s"},
    {PARM_V_CRNT,	(char *)"v_crnt",	(char *)"m/s"},
    {PARM_SPEC_HUM,	(char *)"spec_hum",	(char *)"kg/kg"},
    {PARM_REL_HUM,	(char *)"RH",		(char *)"percent"},
    {PARM_HUM_MIX,	(char *)"hum_mix",	(char *)"kg/kg"},
    {PARM_PR_WATER,	(char *)"pr_water",	(char *)"kg/m2"},
    {PARM_VAP_PR,	(char *)"E",		(char *)"Pa"},
    {PARM_SAT_DEF,	(char *)"sat_def",	(char *)"Pa"},
    {PARM_EVAP,		(char *)"evap",		(char *)"kg/m2"},

    {PARM_C_ICE,	(char *)"c_ice",	(char *)"kg/m2"},

    {PARM_PRECIP_RT,	(char *)"precip_rt",	(char *)"kg/(m2 s)"},
    {PARM_THND_PROB,	(char *)"thnd_prob",	(char *)"percent"},
    {PARM_PRECIP_TOT,	(char *)"PRECIP",	(char *)"kg/m2"},
    {PARM_PRECIP_LS,	(char *)"precip_ls",	(char *)"kg/m2"},
    {PARM_PRECIP_CN,	(char *)"precip_cn",	(char *)"kg/m2"},
    {PARM_SNOW_RT,	(char *)"snow_rt",	(char *)"kg/m2/s"},
    {PARM_SNOW_WAT,	(char *)"snow_wat",	(char *)"kg/m2"},
    {PARM_SNOW,		(char *)"snow",		(char *)"m"},
    {PARM_MIXED_DPTH,	(char *)"mixed_dpth",	(char *)"m"},
    {PARM_TT_DEPTH,	(char *)"tt_depth",	(char *)"m"},
    {PARM_MT_DEPTH,	(char *)"mt_depth",	(char *)"m"},
    {PARM_MTD_ANOM,	(char *)"mtd_anom",	(char *)"m"},
    {PARM_CLOUD,	(char *)"N",		(char *)"percent"},
    {PARM_CLOUD_CN,	(char *)"Nc",		(char *)"percent"},
    {PARM_CLOUD_LOW,	(char *)"Nl",		(char *)"percent"},
    {PARM_CLOUD_MED,	(char *)"Nm",		(char *)"percent"},
    {PARM_CLOUD_HI,	(char *)"Nh",		(char *)"percent"},
    {PARM_CLOUD_WAT,	(char *)"cloud_wat",	(char *)"kg/m2"},
    {PARM_BLI,		(char *)"bli",		(char *)"K"},

    {PARM_SNO_C,	(char *)"sno_c",	(char *)"kg/m2"},
    {PARM_SNO_L,	(char *)"sno_l",	(char *)"kg/m2"},

    {PARM_SEA_TEMP,	(char *)"SST",		(char *)"degK"},
    {PARM_LAND_MASK,	(char *)"land_mask",	(char *)"1"},
    {PARM_SEA_MEAN,	(char *)"sea_mean",	(char *)"m"},
    {PARM_SRF_RN,	(char *)"srf_rn",	(char *)"m"},
    {PARM_ALBEDO,	(char *)"albedo",	(char *)"percent"},
    {PARM_SOIL_TEMP,	(char *)"T_soil",	(char *)"degK"},
    {PARM_SOIL_MST,	(char *)"soil_mst",	(char *)"kg/m2"},
    {PARM_VEG,		(char *)"veg",		(char *)"percent"},
    {PARM_SAL,		(char *)"sal",		(char *)"kg/kg"},
    {PARM_DENS,		(char *)"dens",		(char *)"kg/m3"},

    {PARM_WATR,		(char *)"watr",		(char *)"kg/m2"},

    {PARM_ICE_CONC,	(char *)"ice_conc",	(char *)"1"},
    {PARM_ICE_THICK,	(char *)"ice_thick",	(char *)"m"},
    {PARM_ICE_DIR,	(char *)"ice_dir",	(char *)"degrees_true"},
    {PARM_ICE_SPD,	(char *)"ice_spd",	(char *)"m/s"},
    {PARM_ICE_U,	(char *)"ice_u",	(char *)"m/s"},
    {PARM_ICE_V,	(char *)"ice_v",	(char *)"m/s"},
    {PARM_ICE_GROWTH,	(char *)"ice_growth",	(char *)"m"},
    {PARM_ICE_DIV,	(char *)"ice_div",	(char *)"1/s"},

    {PARM_SNO_M,	(char *)"sno_m",	(char *)"kg/m2"},

    {PARM_WAVE_HGT,	(char *)"wave_hgt",	(char *)"m"},
    {PARM_SEA_DIR,	(char *)"sea_dir",	(char *)"degrees_true"},
    {PARM_SEA_HGT,	(char *)"sea_hgt",	(char *)"m"},
    {PARM_SEA_PER,	(char *)"sea_per",	(char *)"s"},
    {PARM_SWELL_DIR,	(char *)"swell_dir",	(char *)"degrees_true"},
    {PARM_SWELL_HGT,	(char *)"swell_hgt",	(char *)"m"},
    {PARM_SWELL_PER,	(char *)"swell_per",	(char *)"s"},
    {PARM_WAVE_DIR,	(char *)"wave_dir",	(char *)"degrees_true"},
    {PARM_WAVE_PER,	(char *)"wave_per",	(char *)"s"},
    {PARM_WAVE2_DIR,	(char *)"wave2_dir",	(char *)"degrees_true"},
    {PARM_WAVE2_PER,	(char *)"wave2_per",	(char *)"s"},
    {PARM_RDN_SWSRF,	(char *)"rdn_swsrf",	(char *)"W/m2"},
    {PARM_RDN_LWSRF,	(char *)"rdn_lwsrf",	(char *)"W/m2"},
    {PARM_RDN_SWTOP,	(char *)"rdn_swtop",	(char *)"W/m2"},
    {PARM_RDN_LWTOP,	(char *)"rdn_lwtop",	(char *)"W/m2"},
    {PARM_RDN_LW,	(char *)"rdn_lw",	(char *)"W/m2"},
    {PARM_RDN_SW,	(char *)"rdn_sw",	(char *)"W/m2"},
    {PARM_RDN_GLBL,	(char *)"rdn_glbl",	(char *)"W/m2"},
    {PARM_BRTMP,	(char *)"brtmp",	(char *)"K"},
    {PARM_LWRAD,	(char *)"lwrad",	(char *)"W/srm2"},
    {PARM_SWRAD,	(char *)"swrad",	(char *)"W/srm2"},
    {PARM_LAT_HT,	(char *)"lat_ht",	(char *)"W/m2"},
    {PARM_SEN_HT,	(char *)"sen_ht",	(char *)"W/m2"},
    {PARM_BL_DISS,	(char *)"bl_diss",	(char *)"W/m2"},

    {PARM_U_FLX,	(char *)"u_flx",	(char *)"N/m2"},
    {PARM_V_FLX,	(char *)"v_flx",	(char *)"N/m2"},
    {PARM_WMIXE,	(char *)"wmixe",	(char *)"J"},

    {PARM_IMAGE,	(char *)"image",	(char *)"none"},
    {PARM_MSLSA,	(char *)"Psl_sa",	(char *)"Pa"},
    {PARM_PM,		(char *)"Pm"	,	(char *)"Pa"},
    {PARM_MSLET,	(char *)"Psl_et",	(char *)"Pa"},
    {PARM_LIFT_INDX,	(char *)"LI",		(char *)"degK"},
    {PARM_LIFT_INDX4,	(char *)"LI4",		(char *)"degK"},
    {PARM_K_INDX,	(char *)"Kind",		(char *)"degK"},
    {PARM_SW_INDX,	(char *)"sweat",	(char *)"degK"},
    {PARM_HM_DIV,	(char *)"mois_div",	(char *)"kg/kg/s"},
    {PARM_VERT_SSHR,	(char *)"vert_sshr",	(char *)"1/s"},
    {PARM_TSLSA,	(char *)"tslsa",	(char *)"Pa/s"},
    {PARM_BVF_2,	(char *)"bvf_2",	(char *)"1/s2"},
    {PARM_PV_MW,	(char *)"pv_mw",	(char *)"1/s/m"},
    {PARM_CRAIN,	(char *)"crain",	(char *)"1"},
    {PARM_CFRZRN,	(char *)"cfrzrn",	(char *)"1"},
    {PARM_CICEPL,	(char *)"cicepl",	(char *)"1"},
    {PARM_CSNOW,	(char *)"csnow",	(char *)"1"},

    {PARM_SOILW,	(char *)"soilw",	(char *)"1"},
    {PARM_PEVPR,	(char *)"pevpr", 	(char *)"W/m2"},
    {PARM_CWORK,	(char *)"cwork",	(char *)"J/kg"},
    {PARM_U_GWD,	(char *)"u_gwd",	(char *)"N/m2"},
    {PARM_V_GWD,	(char *)"v_gwd",	(char *)"N/m2"},
    {PARM_PVORT,	(char *)"pvort",	(char *)"m2/s/kg"},

    {PARM_COVMZ,	(char *)"covmz",	(char *)"m2/s2"},
    {PARM_COVTZ,	(char *)"covtz",	(char *)"K*m/s"},
    {PARM_COVTM,	(char *)"covtm",	(char *)"K*m/s"},
    {PARM_CLWMR,	(char *)"clwmr",	(char *)"kg/kg"},
    {PARM_O3MR,		(char *)"o3mr",		(char *)"kg/kg"},
    {PARM_GFLUX,	(char *)"gflux",	(char *)"W/m2"},
    {PARM_CIN,		(char *)"cin",		(char *)"J/kg"},
    {PARM_CAPE,		(char *)"cape",		(char *)"J/kg"},
    {PARM_TKE,		(char *)"tke",		(char *)"J/kg"},
    {PARM_CONDP,	(char *)"condp",	(char *)"Pa"},
    {PARM_CSUSF,	(char *)"csusf",	(char *)"W/m2"},
    {PARM_CSDSF,	(char *)"csdsf",	(char *)"W/m2"},
    {PARM_CSULF,	(char *)"csulf",	(char *)"W/m2"},
    {PARM_CSDLF,	(char *)"csdlf",	(char *)"W/m2"},
    {PARM_CFNSF,	(char *)"cfnsf",	(char *)"W/m2"},
    {PARM_CFNLF,	(char *)"cfnlf",	(char *)"W/m2"},
    {PARM_VBDSF,	(char *)"vbdsf",	(char *)"W/m2"},
    {PARM_VDDSF,	(char *)"vddsf",	(char *)"W/m2"},
    {PARM_NBDSF,	(char *)"nbdsf",	(char *)"W/m2"},
    {PARM_NDDSF,	(char *)"nddsf",	(char *)"W/m2"},
    {PARM_RWMR,		(char *)"rwmr",		(char *)"kg/kg"},
    {PARM_SNMR,		(char *)"snmr",		(char *)"kg/kg"},
    {PARM_M_FLX,	(char *)"m_flx",	(char *)"N/m2"},
    {PARM_LMH,		(char *)"lmh",		(char *)"1"},
    {PARM_LMV,		(char *)"lmv",		(char *)"1"},
    {PARM_MLYNO,	(char *)"mlyno",	(char *)"1"},
    {PARM_NLAT,		(char *)"nlat",		(char *)"deg"},
    {PARM_ELON,		(char *)"elon",		(char *)"deg"},
    {PARM_ICMR,		(char *)"icmr",		(char *)"kg/kg"},
    {PARM_GRMR,		(char *)"grmr",		(char *)"kg/kg"},

    {PARM_MAX_GUST,     (char *)"max_gust",     (char *)"m/s"},
    {PARM_GUST,         (char *)"gust",         (char *)"m/s"},

    {PARM_LPS_X,	(char *)"lps_x",	(char *)"1/m"},
    {PARM_LPS_Y,	(char *)"lps_y",	(char *)"1/m"},
    {PARM_HGT_X,	(char *)"hgt_x",	(char *)"m/m"},
    {PARM_HGT_Y,	(char *)"hgt_y",	(char *)"m/m"},

    {PARM_TIPD,         (char *)"tipd",         (char *)"none"},

    {PARM_RDRIP,        (char *)"rdrip",        (char *)"none"},

    {PARM_VPTMP,	(char *)"vptmp",	(char *)"K"},
    {PARM_HLCY,		(char *)"helc",		(char *)"m2/s2"},
    {PARM_PROB,		(char *)"prob",		(char *)"percent"},
    {PARM_PROBN,	(char *)"probn",	(char *)"percent"},
    {PARM_POP,		(char *)"pop",		(char *)"percent"},
    {PARM_CPOFP,	(char *)"cpofp",	(char *)"percent"},
    {PARM_CPOZP,	(char *)"cpozp",	(char *)"percent"},
    {PARM_USTM,		(char *)"ustm",		(char *)"m/s"},
    {PARM_VSTM,		(char *)"vstm",		(char *)"m/s"},
    {PARM_NOICE_WAT,	(char *)"noice_wat",	(char *)"percent"},

    {PARM_DSWRF,	(char *)"dswrf",	(char *)"W/m2"},
    {PARM_DLWRF,	(char *)"dlwrf",	(char *)"W/m2"},
    {PARM_UVPI,		(char *)"uvpi",		(char *)"W/m2"},

    {PARM_MSTR_AVL,	(char *)"mstr_avl",	(char *)"%"},
    {PARM_XCHG_COF,	(char *)"xchg_cof",	(char *)"kg/m2/s"},
    {PARM_NMIX_LYRS,	(char *)"nmix_lyrs",	(char *)"1"},

    {PARM_USWRF,	(char *)"uswrf",	(char *)"W/m2"},
    {PARM_ULWRF,	(char *)"ulwrf",	(char *)"W/m2"},

    {PARM_CLOUD_NCN,	(char *)"cloud_ncn",	(char *)"%"},

    {PARM_CPRAT,	(char *)"cprat",	(char *)"kg/m2/s"},
    {PARM_TTDIA,	(char *)"ttdia",	(char *)"K/s"},

    {PARM_RDN_TTND,	(char *)"rdn_ttnd",	(char *)"degK/s"},

    {PARM_TTPHY,	(char *)"ttphy",	(char *)"K/s"},
    {PARM_PREIX,	(char *)"preix",	(char *)""},

    //{PARM_TSD1D,	(char *)"tsd1d",	(char *)"K"},
    {PARM_CLOUD_HGT,	(char *)"cloud",	(char *)"m"}, // NBM
    {PARM_CBH,	        (char *)"cbh",	        (char *)"m"}, // cloud-base-height

    {PARM_LN_PRES,	(char *)"ln_pres",	(char *)"ln(kPa)"},
    {PARM_HPBL,		(char *)"hpbl",		(char *)"m"},
    {PARM_GPT_HGT5,	(char *)"gpt_hgt5",	(char *)"gp m"},

    {PARM_C_WAT,	(char *)"c_wat",	(char *)"kg/m2"},
    {PARM_BMIXL,	(char *)"bmixl",	(char *)"m"},
    {PARM_AMIXL,	(char *)"amixl",	(char *)"m"},
    {PARM_PEVAP,	(char *)"pevap",	(char *)"kg/m2"},
    {PARM_GPT_HGT5A,	(char *)"gpt_hgt5_anom", (char *)"gp m"},
    {PARM_SNOHF,	(char *)"snohf",	(char *)"W/m2"},
    {PARM_MFLUX,	(char *)"mflux",	(char *)"Pa/s"},
    {PARM_DTRF,		(char *)"dtrf",		(char *)"W/m2"},
    {PARM_UTRF,		(char *)"utrf",		(char *)"W/m2"},
    {PARM_BGRUN,	(char *)"bgrun",	(char *)"kg/m2"},
    {PARM_SSRUN,	(char *)"ssrun",	(char *)"kg/m2"},
    {PARM_03TOT,	(char *)"03tot",	(char *)"Kg/m2"},
    {PARM_SNO_CVR,	(char *)"sno_cvr",	(char *)"percent"},
    {PARM_SNO_T,	(char *)"sno_t",	(char *)"K"},
    {PARM_LRGHR,	(char *)"lrghr",	(char *)"K/s"},
    {PARM_CNVHR,	(char *)"cnvhr",	(char *)"K/s"},
    {PARM_CNVMR,	(char *)"cnvmr",	(char *)"kg/kg/s"},
    {PARM_SHAHR,	(char *)"shahr",	(char *)"K/s"},
    {PARM_SHAMR,	(char *)"shamr",	(char *)"kg/kg/s"},
    {PARM_VDFHR,	(char *)"vdfhr",	(char *)"K/s"},
    {PARM_VDFUA,	(char *)"vdfua",	(char *)"m/s2"},
    {PARM_VDFVA,	(char *)"vdfva",	(char *)"m/s2"},
    {PARM_VDFMR,	(char *)"vdfmr",	(char *)"kg/kg/s"},
    {PARM_SWHR,		(char *)"swhr",		(char *)"K/s"},
    {PARM_LWHR,		(char *)"lwhr",		(char *)"K/s"},
    {PARM_CD,		(char *)"cd",		(char *)"non-dim"},
    {PARM_FRICV,	(char *)"fricv",	(char *)"m/s"},
    {PARM_RI,		(char *)"ri",		(char *)"non-dim."},

    {PARM_MISSING,	(char *)"missing",	(char *)"none"},

    /* GRIB Edition 0 */
    {PARM_VERT_SHR,	(char *)"vert_shr",	(char *)"m/sec/km"},
    {PARM_CON_PRECIP,	(char *)"con_precip",	(char *)"mm h2o/g"},
    {PARM_PRECIP,	(char *)"PRECIP",	(char *)"mm h2o/g"},
    {PARM_NCON_PRECIP,	(char *)"ncon_precip",	(char *)"mm h2o/g"},
    {PARM_SST_WARM,	(char *)"sst_warm",	(char *)"degC"},
    {PARM_UND_ANOM,	(char *)"und_anom",	(char *)"degC"},
    {PARM_SEA_TEMP_0,	(char *)"SST",		(char *)"0.1 degC"},
    {PARM_PRESSURE_D,	(char *)"pressure_d",	(char *)"10 pascals"},
    {PARM_GPT_THICK,	(char *)"gpt_thick",	(char *)"gp m"},
    {PARM_GPT_HGT_D,	(char *)"gpt_hgt_d",	(char *)"gp m"},
    {PARM_GEOM_HGT_D,	(char *)"geom_hgt_d",	(char *)"m"},
    {PARM_TEMP_D,	(char *)"temp_d",	(char *)"0.1 degC"},
    {PARM_REL_HUM_D,	(char *)"rel_hum_d",	(char *)"0.1 percent"},
    {PARM_LIFT_INDX_D,	(char *)"lift_indx_d",	(char *)"0.1 degC"},
    {PARM_REL_VOR_D,	(char *)"rel_vor_d",	(char *)"10**-6/sec"},
    {PARM_ABS_VOR_D,	(char *)"abs_vor_d",	(char *)"10**-6/sec"},
    {PARM_VERT_VEL_D,	(char *)"omega",	(char *)"10 pascals/sec"},
    {PARM_SEA_TEMP_D,	(char *)"sea_temp_d",	(char *)"0.01 degC"},
    {PARM_SST_ANOM,	(char *)"sst_anom",	(char *)"0.1 degC"},
    {PARM_QUAL_IND,	(char *)"qual_ind",	(char *)"none"},
    {PARM_GPT_DEP,	(char *)"gpt_dep",	(char *)"gp m"},
    {PARM_PRESSURE_DEP,	(char *)"pressure_dep",	(char *)"100 pascals"},

    {PARM_ECMWF_PRECIP_TOT, (char *)"ecmwf_PRECIP",	(char *)"m"},
    {PARM_ECMWF_PRECIP_CN,  (char *)"ecmwf_precip_cn",	(char *)"m"},
    {PARM_ECMWF_DSWRF,	    (char *)"ecmwf_dswrf",	(char *)"W/m2-s"},

    {PARM_SEA_ICE,      (char *)"sea_ice",      (char *)"1"},
    {PARM_AREA_CLOUDF,  (char *)"cld_area_fract", (char *)"1"},
    {PARM_LIQ_CLOUDF,   (char *)"liq_cld_fract",(char *)"1"},
    {PARM_ICE_CLOUDF,   (char *)"ice_cld_fract",(char *)"1"},
    {PARM_EVAP_SEA,     (char *)"evap_sea",     (char *)"kg/m2"},
    {PARM_FOG_FRACT,    (char *)"fog_fract",    (char *)"1"},
    {PARM_VIS_PROB,     (char *)"vis_prob",     (char *)"1"},
    {PARM_PW_CODE,      (char *)"pw_code",      (char *)"none"},
    {PARM_CSRAT,        (char *)"csrat",        (char *)"kg/m2/s"},
    {PARM_SCLLWC,       (char *)"scllwc",       (char *)"kg/kg"},
    {PARM_SCILWC,       (char *)"scliwc",       (char *)"kg/kg"},
    
    {PARM_REFC,		(char *)"radar_cref",	(char *)"BZ"},

    {PARM_PTYPE,	(char *)"precip_type",	(char *)"none"},

    {PARM_LAST_ENTRY,   (char *)"reserved",     (char *)"none"}
};

#define PARM_TABLE_SIZE sizeof(ptable)/sizeof(ptable[0])

static int lookup[PARM_LAST_ENTRY + 1];	/* private lookup table for speed */
static int lookup_initialized = 0; /* gets set to 1 when lookup initialized */

static void
init_lookup() {			/* initialize fast lookup table from the
				 * parameter table */
    if (! lookup_initialized) {
	unsigned int i;
	for (i=0; i < PARM_TABLE_SIZE; i++)
	  lookup[ptable[i].code] = i;
	lookup_initialized = 1;
    }
}


/*
 * Given the parameter code from grib data, returns a netCDF variable
 * name chosen for this parameter.  Returns NULL if not found.
 */
char *
grib_pname(
     int param			/* parameter code from GRIB data */
	)
{
    if (! lookup_initialized)
	init_lookup();

    if (param < 0 || param >= PARM_LAST_ENTRY)
      return 0;

    return ptable[lookup[param]].name;
}

/*
 * Given the netCDF variable name for a GRIB parameter, returns the GRIB
 * parameter code (the inverse of the grib_pname() mapping).  Since we only do
 * this rarely, it uses a linear search.  Returns -1 if name doesn't match any
 * GRIB parameter names.
 * Comparison is made *without* any level suffixes we have may added
 * (e.g. "sfc", "trop", "maxwind").
 */
int
grib_pcode(
     char *pname		/* parameter name */
	)
{
    unsigned int i = strlen(pname);
#define PARAM_NAME_LEN_MAX 128
    char pname2[PARAM_NAME_LEN_MAX];		
    static char *suffixes[] = {	/* list of suffixes returned from
				   levelsuffix() function */
	(char *)"_sfc",			/* surface of the Earth */
	(char *)"_clbs",		/* cloud base level */
	(char *)"_cltp",		/* cloud top level */
	(char *)"_frzlvl",		/* 0 degree isotherm level */
	(char *)"_adcn",		/* adiabatic condensation level */
	(char *)"_maxwind",		/* maximium wind speed level */
	(char *)"_trop",		/* at the tropopause */
	(char *)"_topa",		/* nominal top of atmosphere */
	(char *)"_sbot",		/* sea bottom */
	(char *)"_liso",		/* layer between two isobaric levels */
	(char *)"_msl",			/* mean sea level */
	(char *)"_fh",			/* fixed height level */
	(char *)"_lfhm",		/* layer between 2 height levels above MSL */
	(char *)"_fhg",			/* fixed height above ground */
	(char *)"_lfhg",		/* layer between 2 height levels above
				   ground */
	(char *)"_sigma",		/* sigma level */
	(char *)"_ls",			/* layer between 2 sigma levels */
	(char *)"_hybr",		/* Hybrid level */
	(char *)"_lhyb",		/* Layer between 2 hybrid levels */
	(char *)"_bls",			/* Depth below land surface */
	(char *)"_lbls",		/* Layer between 2 depths below land surface */
	(char *)"_isen",		/* Isentropic (theta) level */
	(char *)"_lisn",		/* Layer between 2 isentropic (theta) levels */
	(char *)"_pdg",			/* level at specified pressure difference from
				   ground */
	(char *)"_lpdg",		/* layer between levels at specif. pressure
				   diffs from ground */
	(char *)"_pv",			/* level of specified potential vorticity */
	(char *)"_lish",		/* layer between 2 isobaric surfaces (high
				   precision) */
	(char *)"_fhgh",		/* height level above ground (high
				   precision) */
	(char *)"_lsh",			/* layer between 2 sigma levels (high
				   precision) */
	(char *)"_lism",		/* layer between 2 isobaric surfaces (mixed
				   precision) */
	(char *)"_dbs",			/* depth below sea level */
	(char *)"_atm",			/* entire atmosphere considered as a single
				   layer */
	(char *)"_ocn",			/* entire ocean considered as a single layer */
 	(char *)"_fl",			/* flight level */
   };
    unsigned int suf;
    
    if (STREQ(pname, "P_msl"))	/* exceptional case, "P" and "P_msl" are
				   2 different GRIB parameters */
	return PARM_PMSL;

    strncpy(pname2,pname,PARAM_NAME_LEN_MAX);
    for (suf = 0; suf < sizeof(suffixes)/sizeof(suffixes[0]); suf++) {
	int lensuf = strlen(suffixes[suf]);
	if (strcmp(pname2+i-lensuf, suffixes[suf]) == 0) /* strip off suffix */
	    pname2[i-lensuf] = '\0';
    }

    for(i=0; i < PARM_TABLE_SIZE; i++)
	if (STREQ(pname2, ptable[i].name)) {
	    return ptable[i].code;
	}
    return -1;
}


/*
 * Given the parameter code from grib data, returns GRIB units string for this
 * parameter.  Returns NULL if not found.
 */
char *
grib_units(
     int param			/* parameter code from GRIB data */
	)
{
    if (! lookup_initialized)
	init_lookup();

    if (param < 0 || param >= PARM_LAST_ENTRY)
      return 0;

    return ptable[lookup[param]].units;
}


/*
 * Maps (GRIB edition, parameter code) pair into a unique parameter code,
 * based on the GRIB edition 1 codes.  This is necessary because the meaning
 * of the parameter tables (Table 5 of StackPole's GRIB 0 description, Table
 * 2 of the GRIB 1 description) completely changed between editions.
 */
int
param_code(
     int grib_edition,
     int param
	)
{
    switch (grib_edition) {
      case 1:
	return param;
      case 0:			/* soon to be obsolete ... */
	switch (param) {
	  case 1: return PARM_PRESSURE;
	  case 2: return PARM_GPT_HGT;
	  case 3: return PARM_GEOM_HGT;
	  case 4: return PARM_TEMP;
	  case 5: return PARM_MAX_TEMP;
	  case 6: return PARM_MIN_TEMP;
	  case 8: return PARM_POT_TEMP;
	  case 10: return PARM_DP_TEMP;
	  case 11: return PARM_DP_DEP;
	  case 12: return PARM_SPEC_HUM;
	  case 13: return PARM_REL_HUM;
	  case 14: return PARM_HUM_MIX;
	  case 15: return PARM_LIFT_INDX;
	  case 17: return PARM_LIFT_INDX4;
	  case 21: return PARM_WND_SPEED;
	  case 23: return PARM_U_WIND;
	  case 24: return PARM_V_WIND;
	  case 29: return PARM_STRM_FUNC;
	  case 30: return PARM_REL_VOR;
	  case 31: return PARM_ABS_VOR;
	  case 40: return PARM_VERT_VEL;
	  case 44: return PARM_VERT_SHR;
	  case 47: return PARM_PR_WATER;
	  case 48: return PARM_CON_PRECIP;
	  case 50: return PARM_PRECIP;
	  case 51: return PARM_SNOW;
	  case 55: return PARM_NCON_PRECIP;
	  case 58: return PARM_SST_WARM;
	  case 59: return PARM_UND_ANOM;
	  case 61: return PARM_SEA_TEMP_0;
	  case 64: return PARM_WAVE_HGT;
	  case 65: return PARM_SWELL_DIR;
	  case 66: return PARM_SWELL_HGT;
	  case 67: return PARM_SWELL_PER;
	  case 68: return PARM_SEA_DIR;
	  case 69: return PARM_SEA_HGT;
	  case 70: return PARM_SEA_PER;
	  case 75: return PARM_WAVE_DIR;
	  case 76: return PARM_WAVE_PER;
	  case 77: return PARM_WAVE2_DIR;
	  case 78: return PARM_WAVE2_PER;
	  case 90: return PARM_ICE_CONC;
	  case 91: return PARM_ICE_THICK;
	  case 92: return PARM_ICE_U;
	  case 93: return PARM_ICE_V;
	  case 94: return PARM_ICE_GROWTH;
	  case 95: return PARM_ICE_DIV;
	  case 100: return PARM_PRESSURE_D;
	  case 101: return PARM_GPT_THICK;
	  case 102: return PARM_GPT_HGT_D;
	  case 103: return PARM_GEOM_HGT_D;
	  case 104: return PARM_TEMP_D;
	  case 113: return PARM_REL_HUM_D;
	  case 115: return PARM_LIFT_INDX_D;
	  case 130: return PARM_REL_VOR_D;
	  case 131: return PARM_ABS_VOR_D;
	  case 141: return PARM_VERT_VEL_D;
	  case 162: return PARM_SEA_TEMP_D;
	  case 163: return PARM_SST_ANOM;
	  case 180: return PARM_MIXED_DPTH;
	  case 181: return PARM_TT_DEPTH;
	  case 182: return PARM_MT_DEPTH;
	  case 183: return PARM_MTD_ANOM;
	  case 190: return PARM_QUAL_IND;
	  case 210: return PARM_GPT_DEP;
	  case 211: return PARM_PRESSURE_DEP;
	  default:
	    return -1;
	}
      default:
	return -1;
    }
    /* NOT REACHED */
}

/*
 * Maps ECMWF parameter codes to unique parameter code. Note the original function
 * did not use grib table version because only one version was available at that
 * time. But now we are getting grids from other tables and there can be conflicts
 * so some of these cases are now caught but only in an ad-hoc manner.
 * (JC 8/22)
 */
int
param_code_e( /* For the ECMWF */
     int grib_edition,
     int grib_table_version,
     int param
	)
{
    switch (grib_edition) {
      case 1:
	switch (param) {
	  case  20: return PARM_VIS;
	  case  23: return PARM_CBH;
	  case  28: return PARM_MAX_GUST;
	  case  29: return PARM_GUST;
	  // case 123: return PARM_GUST;
	  case 129:
	    switch (grib_table_version)
	      {
	      case 128: return PARM_GPT_HGT;
	      default: return -1;
	      }
	  case 130: return PARM_TEMP;
	  case 131: return PARM_U_WIND;
	  case 132: return PARM_V_WIND;
	  case 133: return PARM_SPEC_HUM;
	  case 135: return PARM_VERT_VEL;
	  case 151: return PARM_PMSL;
	  case 134: return PARM_PRESSURE;
	  case 156: return PARM_GPT_HGT;
	  case 157: return PARM_REL_HUM;
	  case 159: return PARM_HPBL;
	  case 164: return PARM_CLOUD;
	  case 165: return PARM_U_WIND;
	  case 166: return PARM_V_WIND;
	  case 167: return PARM_TEMP;
	  case 168: return PARM_DP_TEMP;
	  case 186: return PARM_CLOUD_LOW;   
          case 187: return PARM_CLOUD_MED;   
          case 188: return PARM_CLOUD_HI;   
          case 228: return PARM_ECMWF_PRECIP_TOT;
	  case 143: return PARM_ECMWF_PRECIP_CN;
	  case 239: return PARM_U_WIND;
	  case 240: return PARM_V_WIND;
	  case 246: return PARM_U_WIND;
	  case 247: return PARM_V_WIND;
	  case  59: return PARM_CAPE;
          case 169: return PARM_ECMWF_DSWRF;
          case 144: return PARM_SNOW;
          case 121: return PARM_MAX_TEMP;
	  case 122: return PARM_MIN_TEMP;
	  default:
	    return -1;
	}
      case 0:			/* soon to be obsolete ... */
	switch (param) {
	  case 129: return PARM_GPT;
	  case 130: return PARM_TEMP;
	  case 131: return PARM_U_WIND;
	  case 132: return PARM_V_WIND;
	  case 133: return PARM_SPEC_HUM;
	  case 134: return PARM_PRESSURE;
	  case 135: return PARM_VERT_VEL;
	  case 137: return PARM_PRECIP_TOT;
	  case 138: return PARM_ABS_VOR;
	  case 139: return PARM_TEMP;
	  case 140: return PARM_SOIL_MST;
	  case 141: return PARM_SNOW;
	  case 142: return PARM_PRECIP_LS;
	  case 143: return PARM_PRECIP_CN;
	  case 144: return PARM_SNOW;
	  case 145: return PARM_BL_DISS;
	  case 146: return PARM_SEN_HT;
	  case 147: return PARM_LAT_HT;
	  case 151: return PARM_PMSL;
	  case 152: return PARM_LN_PRES;
	  case 155: return PARM_ABS_DIV;
	  case 156: return PARM_GPT_HGT;
	  case 157: return PARM_REL_HUM;
	  case 158: return PARM_PTND;
	  case 165: return PARM_U_WIND;
	  case 166: return PARM_V_WIND;
	  case 167: return PARM_TEMP;
	  case 168: return PARM_DP_TEMP;
	  case 170: return PARM_SOIL_TEMP;
	  case 171: return PARM_SOIL_MST;
	  case 172: return PARM_LAND_MASK;
	  case 173: return PARM_SRF_RN;
	  case 174: return PARM_ALBEDO;
	  case 176: return PARM_RDN_SWSRF;
	  case 177: return PARM_RDN_LWSRF;
	  case 178: return PARM_RDN_SWTOP;
	  case 179: return PARM_RDN_LWTOP;
	  case 182: return PARM_EVAP;
	  default:
	    return -1;
	}
      default:
	return -1;
    }
    /* NOT REACHED */
}

/*
 * Maps certain UKMET parameter codes to unique code. Only some are defined
 * here based on what has been seen so far. Look here for further definitions:
 *
 * http://research.metoffice.gov.uk/research/nwp/numerical/operational/products/GRIB1_Catalogue_Spring2009.pdf
 *
 */
int
param_code_ukmet( /* For the UKMET */
     int param
	)
{
  // Map only values > 128.
  if (param < 128)
    return param;

  switch (param)
    {
    case 138: return PARM_FOG_FRACT;
    case 140: return PARM_PRECIP_CN;
    case 143: return PARM_PRECIP_RT;
    case 144: return PARM_CPRAT;
    case 146: return PARM_SNOW_RT;
    case 147: return PARM_CSRAT;
    case 148: return PARM_GEOM_HGT;
    case 149: return PARM_GUST;
    case 150: return PARM_PW_CODE;
    default:
      logFile->write_time(1, "Warning: Unknown UKMET parameter code %d\n", param);
      return -1;
  }
    /* NOT REACHED */
}



/*
 * Maps Australian BOM parameter codes to unique parameter code. Needs
 * parameter version table since they use several.
 */
int
param_code_bom( /* For BOM grids */
     int table,
     int param
	)
{
    switch (table)
      {
      case 128:
	switch (param)
	  {
	  case 31: return PARM_SEA_ICE;
	  case 49: return PARM_GUST;  
	  case 54: return PARM_PRESSURE;
	  case 55: return PARM_PRESSURE;
	  case 81: return PARM_SPEC_HUM;
	  case 82: return PARM_AREA_CLOUDF;
	  case 83: return PARM_LIQ_CLOUDF;
	  case 84: return PARM_ICE_CLOUDF;
	  case 86: return PARM_SOIL_MST;
	  case 87: return PARM_C_WAT;
	  case 88: return PARM_DENS;
	  case 89: return PARM_PRESSURE;
	  case 90: return PARM_PRESSURE;
	  case 101: return PARM_REL_HUM;
	  case 109: return PARM_SRF_RN;
	  case 125: return PARM_TEMP;
	  case 130: return PARM_TEMP;
	  case 131: return PARM_U_WIND;
	  case 132: return PARM_V_WIND;
	  case 134: return PARM_PRESSURE;
	  case 135: return PARM_VERT_VEL;
	  case 137: return PARM_PR_WATER;
	  case 139: return PARM_SOIL_TEMP;
	  case 140: return PARM_SOIL_MST;
	  case 146: return PARM_SEN_HT;
	  case 147: return PARM_LAT_HT;
	  case 151: return PARM_PMSL;
	  case 156: return PARM_GPT_HGT;
	  case 157: return PARM_REL_HUM;
	  case 159: return PARM_HPBL;
	  case 164: return PARM_CLOUD;
	  case 165: return PARM_U_WIND;
	  case 166: return PARM_V_WIND;
	  case 167: return PARM_TEMP;
	  case 168: return PARM_DP_TEMP;
	  case 170: return PARM_SOIL_TEMP;
	  case 171: return PARM_SOIL_MST;
	  case 180: return PARM_U_GWD;
	  case 181: return PARM_V_GWD;
	  case 183: return PARM_SOIL_TEMP;
	  case 184: return PARM_SOIL_MST;
	  case 185: return PARM_CLOUD_CN;
	  case 186: return PARM_CLOUD_LOW;
	  case 187: return PARM_CLOUD_MED;
	  case 188: return PARM_CLOUD_HI;
	  case 201: return PARM_MAX_TEMP;
	  case 202: return PARM_MIN_TEMP;
	  case 236: return PARM_SOIL_TEMP;
	  case 237: return PARM_SOIL_MST;
	  case 246: return PARM_CLWMR;
	  case 247: return PARM_ICMR;
	  default:
	    return -1;
	}
      case 228:
	switch (param)
	  {
	  case 17: return PARM_DP_TEMP;
	  case 47: return PARM_VDDSF;
	  case 57: return PARM_EVAP;
	  case 58: return PARM_EVAP_SEA;
	  case 61: return PARM_PRECIP_TOT;
	  case 115: return PARM_VBDSF;
	  case 126: return PARM_FRICV;
	  case 133: return PARM_SPEC_HUM;
	  case 134: return PARM_GEOM_VEL;
	  case 139: return PARM_GEOM_HGT;
	  case 142: return PARM_PRECIP_LS;
	  case 143: return PARM_PRECIP_CN;
	  case 156: return PARM_GEOM_HGT;
	  case 172: return PARM_LAND_MASK;
	  case 200: return PARM_TEMP;
	  case 202: return PARM_PMSL;
	  case 211: return PARM_RDN_SWSRF;
	  case 212: return PARM_RDN_LWSRF;
	  case 213: return PARM_DLWRF;
	  case 214: return PARM_DSWRF;
	  case 215: return PARM_ULWRF;
	  case 216: return PARM_DSWRF;
	  case 217: return PARM_CLOUD;
	  case 221: return PARM_LAT_HT;
	  case 222: return PARM_SEN_HT;
	  case 224: return PARM_U_GWD;
	  case 225: return PARM_V_GWD;
	  case 233: return PARM_U_WIND;
	  case 234: return PARM_V_WIND;
	  case 239: return PARM_SNO_C;
	  case 240: return PARM_SNO_L;
	  case 253: return PARM_SPEC_HUM;
	  default:
	    return -1;
	  }
      case 229:
	switch (param)
	  {
	  case 20: return PARM_VIS;
	  case 210: return PARM_FOG_FRACT;
	  case 211: return PARM_VIS_PROB;
	  default:
	    return -1;
	  }
      case 231:
	switch (param)
	  {
	  case 33: return PARM_U_WIND;
	  case 34: return PARM_V_WIND;
	  default:
	    return -1;
	  }
      default:
	return -1;
      }
    /* NOT REACHED */
 
}

/*
 * Given the parameter code from grib data for a GRIB edition 0 parameter,
 * returns a units string for the units used in GRIB edition 0 for this
 * parameter.  Returns NULL if not found.
 */
char *
grib0_units(
     int param			/* GRIB 1 parameter code from GRIB 0 data */
	)
{
    switch (param) {
      case PARM_PRESSURE: return (char *)"hectopascals" ;
      case PARM_GPT_HGT: return (char *)"geopotential dekameters" ;
      case PARM_GEOM_HGT: return (char *)"10 m" ;
      case PARM_TEMP: return (char *)"celsius" ;
      case PARM_MAX_TEMP: return (char *)"celsius" ;
      case PARM_MIN_TEMP: return (char *)"celsius" ;
      case PARM_POT_TEMP: return (char *)"celsius" ;
      case PARM_DP_TEMP: return (char *)"celsius" ;
      case PARM_DP_DEP: return (char *)"celsius" ;
      case PARM_SPEC_HUM: return (char *)"0.1 g/kg" ;
      case PARM_REL_HUM: return (char *)"percent" ;
      case PARM_HUM_MIX: return (char *)"0.1 g/kg (char *)" ;
      case PARM_LIFT_INDX: return (char *)"celsius" ;
      case PARM_LIFT_INDX4: return (char *)"celsius" ;
      case PARM_WND_SPEED: return (char *)"meters/second" ;
      case PARM_U_WIND: return (char *)"meters/second" ;
      case PARM_V_WIND: return (char *)"meters/second" ;
      case PARM_STRM_FUNC: return (char *)"100000 m2/sec" ;
      case PARM_REL_VOR: return (char *)".00001/sec" ;
      case PARM_ABS_VOR: return (char *)".00001/sec" ;
      case PARM_VERT_VEL: return (char *)"millibars/second" ;
      case PARM_VERT_SHR: return (char *)"meters/second/km" ;
      case PARM_PR_WATER: return (char *)"mm h2o/g" ;
      case PARM_CON_PRECIP: return (char *)"mm h2o/g" ;
      case PARM_PRECIP: return (char *)"mm h2o/g" ;
      case PARM_SNOW: return (char *)"cm" ;
      case PARM_NCON_PRECIP: return (char *)"mm h2o/g" ;
      case PARM_SST_WARM: return (char *)"celsius" ;
      case PARM_UND_ANOM: return (char *)"celsius" ;
      case PARM_SEA_TEMP_0: return (char *)"0.1 celsius" ;
      case PARM_WAVE_HGT: return (char *)"0.5 m" ;
      case PARM_SWELL_DIR: return (char *)"10 degrees" ;
      case PARM_SWELL_HGT: return (char *)"0.5 m" ;
      case PARM_SWELL_PER: return (char *)"second" ;
      case PARM_SEA_DIR: return (char *)"10 degrees" ;
      case PARM_SEA_HGT: return (char *)"0.5 m" ;
      case PARM_SEA_PER: return (char *)"second" ;
      case PARM_WAVE_DIR: return (char *)"10 degrees" ;
      case PARM_WAVE_PER: return (char *)"second" ;
      case PARM_WAVE2_DIR: return (char *)"10 degrees" ;
      case PARM_WAVE2_PER: return (char *)"second" ;
      case PARM_ICE_CONC: return (char *)"1" ;
      case PARM_ICE_THICK: return (char *)"m" ;
      case PARM_ICE_U: return (char *)"km/day" ;
      case PARM_ICE_V: return (char *)"km/day" ;
      case PARM_ICE_GROWTH: return (char *)"0.1 meters" ;
      case PARM_ICE_DIV: return (char *)"1/sec" ;
      case PARM_PRESSURE_D: return (char *)"10 pascals" ;
      case PARM_GPT_THICK: return (char *)"gp m" ;
      case PARM_GPT_HGT_D: return (char *)"gp m" ;
      case PARM_GEOM_HGT_D: return (char *)"m" ;
      case PARM_TEMP_D: return (char *)"0.1 celsius" ;
      case PARM_REL_HUM_D: return (char *)"0.1 percent" ;
      case PARM_LIFT_INDX_D: return (char *)"0.1 celsius" ;
      case PARM_REL_VOR_D: return (char *)".000001/sec" ;
      case PARM_ABS_VOR_D: return (char *)".000001/sec" ;
      case PARM_VERT_VEL_D: return (char *)"10 pascals/sec" ;
      case PARM_SEA_TEMP_D: return (char *)"0.01 celsius" ;
      case PARM_SST_ANOM: return (char *)"0.1 celsius" ;
      case PARM_MIXED_DPTH: return (char *)"cm" ;
      case PARM_TT_DEPTH: return (char *)"cm" ;
      case PARM_MT_DEPTH: return (char *)"cm" ;
      case PARM_MTD_ANOM: return (char *)"cm" ;
      case PARM_QUAL_IND: return (char *)"" ;
      case PARM_PRESSURE_DEP: return (char *)"hectopascals" ;
      default:
	return 0;
    }
}


/*
 * Return true (1) if parameter is implicitly a surface parameter that does
 * not need a surface-level suffix
 */
int sfcparam(
    int param
	)
{
    switch (param) {
    case PARM_PRECIP_RT: 	/* Precipitation rate, kg/m2/s */
    case PARM_THND_PROB: 	/* Thunderstorm probability, % */
    case PARM_PRECIP_TOT: 	/* Total precipitation, kg/m2 */
    case PARM_PRECIP_LS: 	/* Large scale precipitation, kg/m2 */
    case PARM_PRECIP_CN: 	/* Convective precipitation, kg/m2 */
    case PARM_SNOW_RT:		/* Snowfall rate water equivalent, kg/m2s */
    case PARM_SNOW_WAT: 	/* Water equiv. of accum. snow depth, kg/m2 */
    case PARM_SNOW:		/* Snow depth, m */
    case PARM_SNO_C:		/* Convective snow, kg/m2 */
    case PARM_SNO_L:		/* Large scale snow, kg/m2 */
    case PARM_SEA_TEMP:		/* sea temperature in degrees K */
    case PARM_SRF_RN:		/* Surface roughness, m */
    case PARM_SOIL_TEMP: 	/* Soil temperature, deg. K */
    case PARM_SOIL_MST: 	/* Soil moisture content, kg/m2 */
    case PARM_VEG:		/* Vegetation, % */
    case PARM_DENS:		/* Density, kg/m3 */
    case PARM_WATR:		/* Water runoff, kg/m2 */
    case PARM_ICE_CONC: 	/* Ice concentration (ice=l; no ice=O), 1/0 */
    case PARM_ICE_THICK: 	/* Ice thickness, m */
    case PARM_ICE_DIR:		/* Direction of ice drift, deg. true */
    case PARM_ICE_SPD:		/* Speed of ice drift, m/s */
    case PARM_ICE_U:		/* u-component of ice drift, m/s */
    case PARM_ICE_V:		/* v-component of ice drift, m/s */
    case PARM_ICE_GROWTH: 	/* Ice growth, m */
    case PARM_ICE_DIV:		/* Ice divergence, /s */
    case PARM_SNO_M:		/* Snow melt, kg/m2 */
    case PARM_RDN_SWSRF:        /* Net shortwave radiation (surface), W/m2 */
    case PARM_RDN_LWSRF:        /* Net longwave radiation (surface), W/m2 */
    case PARM_LIFT_INDX: 	/* Surface lifted index, Deg. K */
    case PARM_CRAIN:		/* Categorical rain  (yes=1; no=0), non-dim */
    case PARM_CFRZRN:		/* Categorical freezing rain  (yes=1; no=0), non-dim */
    case PARM_CICEPL:		/* Categorical ice pellets  (yes=1; no=0), non-dim */
    case PARM_CSNOW:		/* Categorical snow  (yes=1; no=0), non-dim */
    case PARM_GFLUX:		/* Ground Heat Flux, W/m2 */
    case PARM_NOICE_WAT: 	/* Ice-free water surface, % */
    case PARM_MSTR_AVL: 	/* Moisture availability, % */
    case PARM_NMIX_LYRS: 	/* No. of mixed layers next to surface, integer */
    case PARM_CPRAT:		/* Convective Precipitation rate, kg/m2/s */
    case PARM_PREIX:		/* precip.index(0.0-1.00)(see note), fraction */
    case PARM_LN_PRES:		/* Natural log of surface pressure, ln(kPa) */
    case PARM_C_WAT:		/* Plant canopy surface water, kg/m2 */
    case PARM_SNOHF:		/* Snow phase-change heat flux, W/m2 */
    case PARM_BGRUN:		/* Baseflow-groundwater runoff, kg/m2 */
    case PARM_SSRUN:		/* Storm surface runoff, kg/m2 */
    case PARM_SNO_CVR:		/* Snow cover, percent */
    case PARM_SNO_T:		/* Snow temperature, K */
    case PARM_CON_PRECIP: 	/* convective precip. amount in mm */
    case PARM_PRECIP:		/* precipitation amount in mm */
    case PARM_PTYPE:		/* precipitation type categorical */
    case PARM_NCON_PRECIP: 	/* non-convective precip. amount in mm */
    case PARM_ECMWF_PRECIP_TOT: /* precipitation amount in meters */
    case PARM_ECMWF_PRECIP_CN:  /* convective precip. amount in meters */
    case PARM_SEA_ICE:          /* sea ice cover 0-1 */
    case PARM_AREA_CLOUDF:      /* cloud area fraction 0-1 */
    case PARM_LIQ_CLOUDF:       /* liquid cloud fraction 0-1 */
    case PARM_ICE_CLOUDF:       /* ice cloud fraction 0-1 */
    case PARM_EVAP_SEA:         /* evaporation over open sea in kg/m2 */
    case PARM_FOG_FRACT:        /* fog fraction 0-1 */
    case PARM_VIS_PROB:         /* probability of horizontal visibility < 5km 0-1 */
    case PARM_PW_CODE:          /* present weather code */
    case PARM_CSRAT:		/* Convective snowfall rate, kg/m2/s */
      return 1;
    }
    return 0;
}


/*
 * Return true (1) if parameter is implicitly a MSL parameter that does
 * not need a MSL-level suffix
 */
int mslparam(
    int param
	)
{
    switch (param) {
    case PARM_PMSL:		/* Pressure reduced to MSL, Pa */
    case PARM_MSLSA:		/* Mean sea level pressure (std. atms. reduction), Pa */
    case PARM_MSLET:		/* Mean sea level pressure (ETA model reduction), Pa */
    case PARM_WAVE_HGT:		/* Significant height of combined wind waves and swell, m */
    case PARM_SEA_DIR:		/* Direction of wind waves, deg. true */
    case PARM_SEA_HGT:		/* Significant height of wind waves, m */
    case PARM_SEA_PER:		/* Mean period of wind waves, s */
    case PARM_SWELL_DIR:	/* Direction of swell waves, deg. true */
    case PARM_SWELL_HGT:	/* Significant height of swell waves, m */
    case PARM_SWELL_PER:	/* Mean period of swell waves, s */
    case PARM_WAVE_DIR:		/* Primary wave direction, deg. true */
    case PARM_WAVE_PER:		/* Primary wave mean period, s */
    case PARM_WAVE2_DIR:	/* Secondary wave direction, deg. true */
    case PARM_WAVE2_PER:	/* Secondary wave mean period, s */
	return 1;
    }
    return 0;
}


/*
 * Return true (1) if parameter is implicitly a LISO (layer between two
 * isobaric levels) parameter that does not need a LISO-level suffix
 */
int lisoparam(
    int param
	)
{
    switch (param) {
    case PARM_LIFT_INDX:	/* Surface lifted index, Deg. K */
				/* *** Are there more of these? *** */
	return 1;
    }
    return 0;
}


/*
 * Converts GRIB-2 parameter IDs to GRIB-1 equivalents.
 */
int param_g21(char *header, GRIB2::g2int g2pdtnum, GRIB2::g2int g2mdis,
	      GRIB2::g2int g2pcat,
	      GRIB2::g2int g2pnum, int *g1pver, int *g1pnum)
{

  for (int i=0; i<num_gparams; i++) {

    if (gparams[i].g2mdis == g2mdis && gparams[i].g2pcat == g2pcat &&
        gparams[i].g2pnum == g2pnum) {
      *g1pnum = gparams[i].g1pnum;
      *g1pver = gparams[i].g1pver;

      return (0);
    }
  }

  // Didn't find
  logFile->write_time(1, "Info: GRIB %s: Unknown GRIB-2 parameter: PDT number %d, discipline %d, category %d, number %d\n", header, g2pdtnum, g2mdis, g2pcat, g2pnum);

  return (-1);
}
