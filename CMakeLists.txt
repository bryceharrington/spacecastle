cmake_minimum_required(VERSION 2.8)

project(spacecastle)
set(PROJECT_VERSION "0.2")
set(ARCHIVE_NAME ${CMAKE_PROJECT_NAME}-${PROJECT_VERSION})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeScripts/Modules/")

set(spacecastle_LIBS "")
set(spacecastle_LIBS "")
set(spacecastle_INCS "")
set(spacecastle_INCS_SYS "")

list(APPEND spacecastle_INCS
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/src

  # Generated includes
  ${CMAKE_BINARY_DIR}/include
)

# ----------------------------------------------------------------------
# Dependencies
# ----------------------------------------------------------------------

find_package(PkgConfig REQUIRED)

find_package(Popt REQUIRED)
list(APPEND spacecastle_INCS_SYS ${POPT_INCLUDE_DIR})
list(APPEND spacecastle_LIBS ${POPT_LIBRARIES})
add_definitions(${POPT_DEFINITIONS})

find_package(X11 REQUIRED)

pkg_check_modules(GTK2 REQUIRED gtk+-2.0)
list(APPEND spacecastle_INCS_SYS
       ${GTK2_INCLUDE_DIRS}
       ${GTK2_CAIRO_INCLUDE_DIR}
)
list(APPEND spacecastle_LIBS ${GTK2_LIBRARIES})
link_directories(${GTK2_LIBRARY_DIRS})
add_definitions(${GTK2_CFLAGS_OTHER})

find_package(Cairo)
list(APPEND spacecastle_INCS_SYS ${CAIRO_INCLUDE_DIRS})
list(APPEND spacecastle_LIBS ${CAIRO_LIBRARIES})

# ----------------------------------------------------------------------
# Build
# ----------------------------------------------------------------------

include_directories(${spacecastle_INCS})
include_directories(SYSTEM ${spacecastle_INCS_SYS})

add_subdirectory(src)

unset(spacecastle_INCS)
unset(spacecastle_INCS_SYS)

add_custom_target(dist
  COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD
      | bzip2 > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.bz2
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# ----------------------------------------------------------------------
# Testing
# ----------------------------------------------------------------------

# TODO: Move this config stuff into test/subdir
# TODO: Alias 'check' and 'test'

#add_boost_test
#@add_executable()
#target_link_libraries()

enable_testing()
add_subdirectory(test)
add_test(
  NAME score
  COMMAND test_score
  )
