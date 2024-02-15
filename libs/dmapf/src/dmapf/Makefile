###########################################################################
#
# Makefile for dmapf module
#
# Martha Limber, RAP, NCAR, Boulder, Co, USA, 80307
#
# January 2002
#
###########################################################################


include $(RAP_MAKE_INC_DIR)/rap_make_macros

LOC_INCLUDES = $(NETCDF4_INCS) -I../include/dmapf
LOC_CPPC_FLAGS =  -g

TARGET_FILE = ../libdmapf.a
MODULE_TYPE = library

HDRS = ../include/dmapf/cmapf.h

CPPC_SRCS = \
	cmapf_model.cc

SRCS = \
	basegm.c \
	cmapf.c \
	eqvlat.c \
	geog_ll.c \
	kcllxy.c \
	limmath.c \
	ll_geog.c \
	lmbrt.c \
	map_xy.c \
	mapstart.c \
	obmrc.c \
	obqlmbrt.c \
	obstr.c \
	proj_3d.c  \
	stcm1p.c \
	stcm2p.c \
	trnsmrc.c \
	xy_map.c \
	vector_3.c


#
# general targets
#

include $(RAP_MAKE_INC_DIR)/rap_make_lib_module_targets


#
# local targets - test programs
#

test_oblique: test_oblique.o
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) test_oblique.o -g -L.. -ldmapf -lm -o test_oblique

test_oblique.o: test_oblique.c
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) -c -g test_oblique.c 

test_conus5: test_conus5.o
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) test_conus5.o -g -L.. -ldmapf -lm -o test_conus5

test_alaska6: test_alaska6.o
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) test_alaska6.o -g -L.. -ldmapf -lm -o test_alaska6

test_awips217: test_awips217.o
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) test_awips217.o -g -L.. -ldmapf -lm -o test_awips217

test_awips217.o: test_awips217.cc
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) -c -g test_awips217.cc 

test_ruc40: test_ruc40.o
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) test_ruc40.o -g -L.. -ldmapf -lm -o test_ruc40

test_ruc40.o: test_ruc40.cc
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) -c -g test_ruc40.cc 

test_ruc20: test_ruc20.o
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) test_ruc20.o -g -L.. -ldmapf -lm -o test_ruc20

test_ruc20.o: test_ruc20.cc
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) -c -g test_ruc20.cc 

test_ruc5: test_ruc5.o
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) test_ruc5.o -g -L.. -ldmapf -lm -o test_ruc5

test_ruc5.o: test_ruc5.cc
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) -g test_ruc5.cc  

ruc13ll2xy: ruc13ll2xy.o
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) ruc13ll2xy.o -g -L.. -ldmapf -lm -o ruc13ll2xy

ruc13ll2xy.o: ruc13ll2xy.cc
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) -c -g ruc13ll2xy.cc 

ruc20ll2xy: ruc20ll2xy.o
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) ruc20ll2xy.o -g -L.. -ldmapf -lm -o ruc20ll2xy

ruc20ll2xy.o: ruc20ll2xy.cc
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) -c -g ruc20ll2xy.cc 

ruc20xy2ll: ruc20xy2ll.o
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) ruc20xy2ll.o -g -L.. -ldmapf -lm -o ruc20xy2ll

ruc20xy2ll.o: ruc20xy2ll.cc
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) -c -g ruc20xy2ll.cc 

ndfd5kmll2xy.o: ndfd5kmll2xy.cc
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) -c -g ndfd5kmll2xy.cc

ndfd5kmll2xy: ndfd5kmll2xy.o
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) ndfd5kmll2xy.o -g -L.. -ldmapf -lm -o ndfd5kmll2xy

ndfd5kmxy2ll.o: ndfd5kmxy2ll.cc
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) -c -g ndfd5kmxy2ll.cc

ndfd5kmxy2ll: ndfd5kmxy2ll.o
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) ndfd5kmxy2ll.o -g -L.. -ldmapf -lm -o ndfd5kmxy2ll

ndfd5km.o: ndfd5km.cc
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) -c -g ndfd5km.cc

ndfd5km: ndfd5km.o
	$(CPPC) $(LOC_CPPC_CFLAGS) $(LOC_INCLUDES) ndfd5km.o -g -L.. -ldmapf -lm -o ndfd5km


depend: depend_generic

# DO NOT DELETE THIS LINE -- make depend depends on it.
