local dmapf_inc_dir = os.getenv("DMAPF_INC")
local dmapf_lib_dir = os.getenv("DMAPF_LIB")
local ncf_inc_dir = os.getenv("NCF_INC")
local ncf_lib_dir = os.getenv("NCF_LIB")
local netcdf_inc_dir = os.getenv("NETCDF_INC")
local netcdf_lib_dir = os.getenv("NETCDF_LIB")

solution "dmapf"
   configurations { "Debug", "Release" }
 
   -- A project defines one build target
   project "dmapf"
      kind "StaticLib"
      language "C++"
      includedirs {dmapf_inc_dir, ncf_inc_dir, netcdf_inc_dir}
      targetdir(dmapf_lib_dir)
      files {
    "dmapf/cmapf_model.cc",
    "dmapf/basegm.c",
    "dmapf/cmapf.c",
    "dmapf/eqvlat.c",
    "dmapf/geog_ll.c",
    "dmapf/kcllxy.c",
    "dmapf/limmath.c",
    "dmapf/ll_geog.c",
    "dmapf/lmbrt.c",
    "dmapf/map_xy.c",
    "dmapf/mapstart.c",
    "dmapf/obmrc.c",
    "dmapf/obqlmbrt.c",
    "dmapf/obstr.c",
    "dmapf/proj_3d.c",
    "dmapf/stcm1p.c",
    "dmapf/stcm2p.c",
    "dmapf/trnsmrc.c",
    "dmapf/xy_map.c",
    "dmapf/vector_3.c",
    "include/dmapf/cmapf.h",
    "include/dmapf/cmapf_model.hh",
    "include/dmapf/limmath.h",
    "include/dmapf/vector_3.h"
 }
 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
	 postbuildcommands { "mkdir -p $(DMAPF_INC)" } 
	 postbuildcommands { "cp include/dmapf/*.h* $(DMAPF_INC)/" } 

      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }    
	 postbuildcommands { "mkdir -p $(DMAPF_INC)" } 
	 postbuildcommands { "cp include/dmapf/*.h* $(DMAPF_INC)/" } 
