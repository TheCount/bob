cmake_minimum_required(VERSION 3.1)
project(bob)
enable_testing()

# Version
set(BOB_VERSION_MAJOR 0) # increases with incompatible API changes
set(BOB_VERSION_MINOR 0) # increases with backwards compatible API changes
set(BOB_VERSION_PATCH 0) # increases with each non-API-changing release
set(BOB_VERSION "${BOB_VERSION_MAJOR}.${BOB_VERSION_MINOR}.${BOB_VERSION_PATCH}")
set(LIBBOB_SOVERSION 0) # increases with incompatible ABI changes
set(LIBBOB_MINOR 0) # increases with backwards compatible ABI changes
set(LIBBOB_RELEASE 0) # increases with code changes that do not affect the ABI
set(LIBBOB_VERSION "${LIBBOB_SOVERSION}.${LIBBOB_MINOR}.${LIBBOB_RELEASE}")

# pkg-config
include(GNUInstallDirs)
set(PKGCONFIG_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(PKGCONFIG_EXEC_PREFIX "${CMAKE_INSTALL_BINDIR}")
set(PKGCONFIG_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
set(PKGCONFIG_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
set(PKGCONFIG_NAME "${PROJECT_NAME}")
set(PKGCONFIG_DESC "BOB — binary object files")
set(PKGCONFIG_VERSION "${BOB_VERSION}")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${PKGCONFIG_NAME}.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/${PKGCONFIG_NAME}.pc" @ONLY)
set(PKGCONFIG_DIR "${PKGCONFIG_LIBDIR}/pkgconfig")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PKGCONFIG_NAME}.pc" DESTINATION ${PKGCONFIG_DIR})

# Compiler settings
set(CMAKE_C_STANDARD 11)
add_definitions("-D_GNU_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64")

# Build subdirs
add_subdirectory(src)
add_subdirectory(test)
