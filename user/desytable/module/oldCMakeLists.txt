#include_directories(${EUDAQ_INCLUDE_DIRS})
include_directories(include)
aux_source_directory(src MODULE_SRC)

if(NOT EUDAQ_LCIO_LIBRARY)
  list(REMOVE_ITEM MODULE_SRC src/DesyTable2LCEventConverter.cc)
endif()

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
if(EUDAQ_LCIO_LIBRARY)
  target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_LCIO_LIBRARY} ${LCIO_LIBRARIES})
else()
  target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY})
endif()


install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
