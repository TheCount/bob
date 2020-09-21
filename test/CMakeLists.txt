include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src
)

link_directories(${CMAKE_BINARY_DIR}/src)

# Test macro
macro(test TESTNAME)
        add_executable(${TESTNAME}-test ${TESTNAME}.c)
        target_link_libraries(${TESTNAME}-test ${PROJECT_NAME})
        add_test(${TESTNAME}-Test ${TESTNAME}-test)
endmacro(test)

# Tests
test(varint)
