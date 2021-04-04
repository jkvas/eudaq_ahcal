#project(eudaq_ahcal VERSION 1.0.0 LANGUAGES CXX)
project(eudaq_ahcal)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/INSTALL CACHE PATH "Default prefix of install directories" FORCE)
endif()

SET(AHCAL_LIB_VERSION 1)

cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
include(CMakeDependentOption)
#find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 11)
set_property(GLOBAL PROPERTY CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)


if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
  message(STATUS "Setting build type to '${CMAKE_BUILD_TYPE}' as none was specified.")
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif(NOT CMAKE_BUILD_TYPE)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(CMAKE_INSTALL_RPATH "@loader_path/../lib;@loader_path/../extern/lib")
else()
  set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib:\$ORIGIN/../extern/lib")
endif()
set(CMAKE_SKIP_BUILD_RPATH  FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


if(WIN32)
  # add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  # #set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  # add_definitions("/wd4251")
  # add_definitions("/wd4996")
  # add_definitions("/wd4251")

  if(NOT MSVC)
    message(FATAL_ERROR "only VC++ is support on Windows")
  endif()
elseif(APPLE)
  if(NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    message(FATEL ERROR "only Clang is support on MacOS")
  endif()
  list(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,-undefined,error")
else()
  if(NOT(CMAKE_COMPILER_IS_GNUCC))
    message(FATEL ERROR "only GCC is support On Linux")
  endif()
  list(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--no-undefined")
endif()


add_subdirectory(external)
option(AHCAL_BUILD_ONLY_EXTERNAL "build only the external projects" OFF)
if(AHCAL_BUILD_ONLY_EXTERNAL)
  message("configured to build only the external projects")
  return()
endif()

find_package(eudaq REQUIRED
  COMPONENTS eudaq::core
  OPTIONAL_COMPONENTS eudaq::lcio
  CONFIG
  PATHS ${CMAKE_INSTALL_PREFIX}
  NO_DEFAULT_PATH
  )

add_subdirectory(user)



export(PACKAGE ahcal)

list(APPEND CONF_AHCAL_LIBRARIES  )

configure_file("${PROJECT_SOURCE_DIR}/cmake/ahcalConfig.cmake.in"  "${PROJECT_BINARY_DIR}/ahcalConfig.cmake" @ONLY)
configure_file("${PROJECT_SOURCE_DIR}/cmake/ahcalConfigVersion.cmake.in"  "${PROJECT_BINARY_DIR}/ahcalConfigVersion.cmake" @ONLY)
install(FILES
  "${PROJECT_BINARY_DIR}/ahcalConfig.cmake"
  "${PROJECT_BINARY_DIR}/ahcalConfigVersion.cmake"
  DESTINATION cmake
  COMPONENT dev)

#install(EXPORT ahcalTargets DESTINATION cmake COMPONENT dev)


