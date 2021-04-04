get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(USER_DESYTABLE_BUILD "build DESY table producer" ON)

if(NOT USER_DESYTABLE_BUILD)
  message("disable building of user/desytable  (USER_DESYTABLE_BUILD=OFF)")
  return()
endif()

add_subdirectory(module)
