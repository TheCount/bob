set(BOB_LIB ${PROJECT_NAME})
set(BOB_LIB_STATIC ${BOB_LIB})

set(BOB_LIB_SOURCES
  bob.c
  config.c
)
include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(bob SHARED ${BOB_LIB_SOURCES})
set_target_properties(bob PROPERTIES
  VERSION ${LIBBOB_VERSION}
  SOVERSION ${LIBBOB_SOVERSION}
  OUTPUT_NAME ${BOB_LIB}
)
add_library(bob_static STATIC ${BOB_LIB_SOURCES})
set_target_properties(bob_static PROPERTIES OUTPUT_NAME ${BOB_LIB_STATIC})

# Install
install(TARGETS bob bob_static DESTINATION ${PKGCONFIG_LIBDIR} )
install(FILES ${PROJECT_DIR}/include/${PROJECT_NAME}.h
  DESTINATION "${PKGCONFIG_INCLUDEDIR}"
)
