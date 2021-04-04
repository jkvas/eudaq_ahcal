include_directories(include)
aux_source_directory(src MODULE_SRC)

include_directories(${EUDAQ_INCLUDE_DIRS})

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} eudaq::core)
set_property(TARGET ${EUDAQ_MODULE} APPEND PROPERTY INSTALL_RPATH ${EUDAQ_LIBRARIES_DIRS})
set(EUDAQ_LIBRARIES eudaq::core)


install(TARGETS
  ${EUDAQ_MODULE}
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)


# set(sourcefiles src/AHCALProducer.cxx src/AHCALProducer.cc src/ScReader.cc)

# include_directories(./include)
# add_executable(AHCALProducer ${sourcefiles})
# target_link_libraries(AHCALProducer ${EUDAQ_CORE_LIBRARY} ${EUDAQ_THREADS_LIB})

# install(TARGETS AHCALProducer
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib)
