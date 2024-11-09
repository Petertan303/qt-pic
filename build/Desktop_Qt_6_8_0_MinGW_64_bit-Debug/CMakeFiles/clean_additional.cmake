# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\t4-2_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\t4-2_autogen.dir\\ParseCache.txt"
  "t4-2_autogen"
  )
endif()
