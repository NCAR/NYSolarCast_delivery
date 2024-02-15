local log_inc_dir = os.getenv("LOG_INC")
local log_lib_dir = os.getenv("LOG_LIB")

solution "log"
   configurations { "Debug", "Release" }
 
   -- A project defines one build target
   project "log"
      kind "StaticLib"
      language "C++"
      targetdir(log_lib_dir)
      files {
    "log/log.cc",
    "include/log/log.hh"
 }
 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
	 postbuildcommands { "mkdir -p $(LOG_INC)" } 
	 postbuildcommands { "cp include/log/log.hh $(LOG_INC)/" } 

      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }    
	 postbuildcommands { "mkdir -p $(LOG_INC)" } 
	 postbuildcommands { "cp include/log/log.hh $(LOG_INC)/" } 
