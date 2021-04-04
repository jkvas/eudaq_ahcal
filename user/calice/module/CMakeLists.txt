include_directories(${EUDAQ_INCLUDE_DIRS})
include_directories(include)
aux_source_directory(src MODULE_SRC)

if(TARGET eudaq::lcio)
  message(STATUS "Find eudaq::lcio, lcio converter is enabled in module eudaq_module_altel")
else()
  list(FILTER MODULE_SRC EXCLUDE REGEX "(.+)LCEventConverter(.+)")  
  message(STATUS "Unable to find eudaq::lcio, lcio converter is disabled in module eudaq_module_calice")
endif()

# if(NOT EUDAQ_LCIO_LIBRARY)
#   list(REMOVE_ITEM MODULE_SRC src/AHCalRawEvent2LCEventConverter.cc)
#   list(REMOVE_ITEM MODULE_SRC src/AhcalLCFileWriter.cc)
# endif()

if(NOT EUDAQ_TTREE_LIBRARY)
  list(REMOVE_ITEM MODULE_SRC src/AHCALDesyRawEvent2TTreeEventConverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/AHCALRawEvent2TTreeEventConverter.cc)
endif()

if(TARGET eudaq::lcio)
  set(EUDAQ_LIBRARIES eudaq::core eudaq::lcio)
  get_target_property(MY_EUDAQ_LCIO_INCLUDE_DIRS  eudaq::lcio INTERFACE_INCLUDE_DIRECTORIES)
  message(${MY_EUDAQ_LCIO_INCLUDE_DIRS})
else()
  set(EUDAQ_LIBRARIES eudaq::core)
endif()


add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_LIBRARIES})


install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
