set(TARGET grib2site)

add_executable(${TARGET}
        centers.cc
        dump.cc
        emalloc.cc
        ens.cc
        gbds.cc
        gbytem.cc
        gdes.cc
        get_prod.cc
        grib1.cc
        grib2site.cc
        gribtypes.cc
        levels.cc
        mkdirs_open.cc
        models.cc
        nc.cc
        ncfloat.cc
        nuwg.cc
        params.cc
        product_data.cc
        quasi.cc
        recs.cc
        site_list.cc
        timeunits.cc
        units.cc
       )

add_dependencies(${TARGET} tdrp_gen)

target_include_directories(${TARGET} PRIVATE
        ${DICAST_LIB_DIR}/dmapf/src/include
        ${DICAST_LIB_DIR}/log/src/include
        ${DICAST_LIB_DIR}/grib2c/g2clib-1.6.4/src/include
        ${DICAST_LIB_DIR}/netcdf_c++/src/include
        )

target_link_libraries(${TARGET} PRIVATE
        dmapf
        grib2c
        log
        netcdf_c++
        udunits2
        )
