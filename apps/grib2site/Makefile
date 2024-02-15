
###########################################################################
#
# Makefile for grib2site module
#
# Jim Cowie, RAP, NCAR, Boulder, Co, USA, 80307
#
# Dec 2005
#
###########################################################################


include $(RAP_MAKE_INC_DIR)/rap_make_macros

LOC_INCLUDES = $(NETCDF4_INCS)
LOC_CPPC_CFLAGS = -Wall
LOC_LDFLAGS = $(NETCDF4_LDFLAGS)
LOC_LIBS = -lgrib2c -ldmapf -llog -lnetcdf -ludunits2 -lexpat -ljasper -lpng -lm \
	        -lhdf5_hl -lhdf5 -lz -lsz -ldl -lcurl

TARGET_FILE = grib2site
MODULE_TYPE = progcpp

HDRS =

CPPC_SRCS =	 	\
	centers.cc	\
	dump.cc		\
	emalloc.cc	\
	ens.cc		\
	gbds.cc		\
	gbytem.cc	\
	gdes.cc		\
	get_prod.cc	\
	grib1.cc	\
	grib2site.cc	\
	gribtypes.cc	\
	levels.cc	\
	mkdirs_open.cc	\
	models.cc	\
	nc.cc		\
	ncfloat.cc	\
	nuwg.cc		\
	params.cc	\
	product_data.cc	\
	quasi.cc	\
	recs.cc		\
	site_list.cc	\
	timeunits.cc	\
	units.cc



include $(RAP_MAKE_INC_DIR)/rap_make_targets

# local targets

depend: depend_generic

# DO NOT DELETE THIS LINE -- make depend depends on it.

