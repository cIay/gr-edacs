INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_EDACS edacs)

FIND_PATH(
    EDACS_INCLUDE_DIRS
    NAMES edacs/api.h
    HINTS $ENV{EDACS_DIR}/include
        ${PC_EDACS_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    EDACS_LIBRARIES
    NAMES gnuradio-edacs
    HINTS $ENV{EDACS_DIR}/lib
        ${PC_EDACS_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(EDACS DEFAULT_MSG EDACS_LIBRARIES EDACS_INCLUDE_DIRS)
MARK_AS_ADVANCED(EDACS_LIBRARIES EDACS_INCLUDE_DIRS)

