add_library(SPFramework STATIC SPAccessors.cpp SPBaseUtil.cpp SPProcessor.cpp)

target_include_directories(SPFramework PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


# dependancies to SeismicUnix

target_include_directories(SPFramework PUBLIC $ENV{CWPROOT}/include)

target_link_directories(SPFramework PUBLIC $ENV{CWPROOT}/lib)

target_link_libraries(SPFramework PUBLIC su par cwp)


# installation

install(TARGETS SPFramework DESTINATION lib)

install(FILES SPAccessors.hh SPBaseUtil.hh SPProcessor.hh DESTINATION include)
