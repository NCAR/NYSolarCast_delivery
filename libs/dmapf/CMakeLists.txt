add_library(dmapf
        src/dmapf/basegm.c
        src/dmapf/cmapf.c
        src/dmapf/cmapf_model.cc
        src/dmapf/eqvlat.c
        src/dmapf/geog_ll.c
        src/dmapf/kcllxy.c
        src/dmapf/limmath.c
        src/dmapf/ll_geog.c
        src/dmapf/lmbrt.c
        src/dmapf/map_xy.c
        src/dmapf/mapstart.c
        src/dmapf/obmrc.c
        src/dmapf/obqlmbrt.c
        src/dmapf/obstr.c
        src/dmapf/proj_3d.c
        src/dmapf/stcm1p.c
        src/dmapf/stcm2p.c
        src/dmapf/trnsmrc.c
        src/dmapf/vector_3.c
        src/dmapf/xy_map.c
        )

target_include_directories(dmapf PRIVATE
        ${DICAST_LIB_DIR}/ncf/src/include
        ${DICAST_LIB_DIR}/netcdf_c++/src/include
        src/include
        )
