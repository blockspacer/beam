file(GLOB header_files ${BEAM_INCLUDE_PATH}/Beam/Python/*.hpp)
file(GLOB source_files ${BEAM_SOURCE_PATH}/Python/*.cpp)

include_directories(${PYTHON_INCLUDE_PATH})

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  add_definitions(-DBOOST_PYTHON_NO_LIB)
endif()

add_library(Python SHARED ${header_files} ${source_files})
set_source_files_properties(${header_files} PROPERTIES HEADER_FILE_ONLY TRUE)
set_target_properties(Python PROPERTIES OUTPUT_NAME beam)
add_definitions(-DBEAM_BUILD_DLL)
add_definitions(-DBEAM_ENABLE_STACK_PRINT)

if(MSVC)
  set_target_properties(Python PROPERTIES SUFFIX ".pyd" LINK_FLAGS /SAFESEH:NO
    LINK_FLAGS_RELEASE /LTCG)
endif()
if(UNIX)
  set_target_properties(Python PROPERTIES PREFIX "")
endif()

target_link_libraries(Python
  debug ${CRYPTOPP_LIBRARY_DEBUG_PATH}
  optimized ${CRYPTOPP_LIBRARY_OPTIMIZED_PATH}
  debug ${OPEN_SSL_LIBRARY_DEBUG_PATH}
  optimized ${OPEN_SSL_LIBRARY_OPTIMIZED_PATH}
  debug ${OPEN_SSL_BASE_LIBRARY_DEBUG_PATH}
  optimized ${OPEN_SSL_BASE_LIBRARY_OPTIMIZED_PATH}
  debug ${PYTHON_LIBRARY_DEBUG_PATH}
  optimized ${PYTHON_LIBRARY_OPTIMIZED_PATH}
  debug ${YAML_LIBRARY_DEBUG_PATH}
  optimized ${YAML_LIBRARY_OPTIMIZED_PATH}
  debug ${BOOST_PYTHON_LIBRARY_DEBUG_PATH}
  optimized ${BOOST_PYTHON_LIBRARY_OPTIMIZED_PATH})

if(UNIX)
  target_link_libraries(Python
    debug ${BOOST_CHRONO_LIBRARY_DEBUG_PATH}
    optimized ${BOOST_CHRONO_LIBRARY_OPTIMIZED_PATH}
    debug ${BOOST_CONTEXT_LIBRARY_DEBUG_PATH}
    optimized ${BOOST_CONTEXT_LIBRARY_OPTIMIZED_PATH}
    debug ${BOOST_DATE_TIME_LIBRARY_DEBUG_PATH}
    optimized ${BOOST_DATE_TIME_LIBRARY_OPTIMIZED_PATH}
    debug ${BOOST_THREAD_LIBRARY_DEBUG_PATH}
    optimized ${BOOST_THREAD_LIBRARY_OPTIMIZED_PATH}
    debug ${BOOST_SYSTEM_LIBRARY_DEBUG_PATH}
    optimized ${BOOST_SYSTEM_LIBRARY_OPTIMIZED_PATH})
endif()

install(TARGETS Python CONFIGURATIONS Debug
  DESTINATION ${LIB_INSTALL_DIRECTORY}/Debug)
install(TARGETS Python CONFIGURATIONS Release RelWithDebInfo
  DESTINATION ${LIB_INSTALL_DIRECTORY}/Release)