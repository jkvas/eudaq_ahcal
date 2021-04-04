# if(WIN32)
#   message("user/calice AHCAL is not to be built on windows (fixme)")
#   return()
# endif()

cmake_dependent_option(USER_CALICE_BUILD_AHCAL "build user/calice AHCAL" ON
  "USER_CALICE_BUILD" OFF)

get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE eudaq_module_${EUDAQ_USERNAME}) 

#include_directories(${EUDAQ_INCLUDE_DIRS})

if(NOT USER_CALICE_BUILD_AHCAL)
  message("user/calice AHCAL is not to be built")
  return()
endif()

add_subdirectory(module)


