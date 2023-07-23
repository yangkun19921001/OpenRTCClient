# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\p2ps_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\p2ps_autogen.dir\\ParseCache.txt"
  "p2ps_autogen"
  )
endif()
