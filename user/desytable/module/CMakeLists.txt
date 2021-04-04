#include_directories(${EUDAQ_INCLUDE_DIRS})
include_directories(include ${EUDAQ_INCLUDE_DIRS})
aux_source_directory(src MODULE_SRC)

find_package(eudaq CONFIG
  COMPONENTS eudaq::core eudaq::lcio
  REQUIRED NO_DEFAULT_PATH)

list(FIND EUDAQ_LIBRARIES "eudaq_lcio" LCIO_FIND_POS)
if(${LCIO_FIND_POS} EQUAL -1)
  list(FILTER MODULE_SRC EXCLUDE REGEX "(.+)LCEventConverter(.+)")
endif()

add_library(eudaq_module_desytable SHARED ${MODULE_SRC})
# if(EUDAQ_LCIO_LIBRARY)
#   target_link_libraries(${EUDAQ_MODULE} eudaq::core eudaq::lcio)
# else()
#   target_link_libraries(${EUDAQ_MODULE} eudaq::core)
# endif()
target_link_libraries(eudaq_module_desytable eudaq::core eudaq::lcio)



install(TARGETS
  eudaq_module_desytable
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
