option(DESYTABLE_BUILD_EUDAQ_MODULE "Build DesyTable producer against EUDAQ2" ON)
if(NOT DESYTABLE_BUILD_EUDAQ_MODULE)
  return()
endif()

include_directories(include)
aux_source_directory(src MODULE_SRC)

# Define interface library for all EUDAQ modules:
include_directories(${EUDAQ_INCLUDE_DIRS})
if(TARGET eudaq::lcio)
  message(STATUS "Find eudaq::lcio, lcio converter is enabled in module eudaq_mogdule_desytable")
else()
  list(FILTER MODULE_SRC EXCLUDE REGEX "(.+)LCEventConverter(.+)")  
  message(STATUS "Unable to find eudaq::lcio, lcio converter is disabled in module eudaq_module_ahcal")
endif()

add_library(eudaq_module_desytable SHARED ${MODULE_SRC})
if(TARGET eudaq::lcio)
  set(EUDAQ_LIBRARIES eudaq::core eudaq::lcio)
  get_target_property(MY_EUDAQ_LCIO_INCLUDE_DIRS  eudaq::lcio INTERFACE_INCLUDE_DIRECTORIES)
  message(${MY_EUDAQ_LCIO_INCLUDE_DIRS})
else()
  set(EUDAQ_LIBRARIES eudaq::core)
endif()

target_link_libraries(eudaq_module_desytable ${EUDAQ_LIBRARIES})
set_property(TARGET eudaq_module_desytable APPEND PROPERTY INSTALL_RPATH ${EUDAQ_LIBRARIES_DIRS})


install(TARGETS
  eudaq_module_desytable
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
  
#get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
#set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

# option(USER_DESYTABLE_BUILD "build DESY table producer" ON)

# if(NOT USER_DESYTABLE_BUILD)
#   message("disable building of user/desytable  (USER_DESYTABLE_BUILD=OFF)")
#   return()
# endif()

#add_subdirectory(module)
