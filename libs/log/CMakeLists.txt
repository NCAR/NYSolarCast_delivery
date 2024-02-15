add_library(log
        src/log/log.cc
        )

target_include_directories(log PRIVATE
        src/include)
