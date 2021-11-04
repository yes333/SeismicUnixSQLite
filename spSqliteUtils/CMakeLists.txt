add_library(SPSqliteUtils STATIC SPTable.cpp)

target_include_directories(SPSqliteUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(SPSqliteUtils PUBLIC SPFramework)


# dependancies to SeismicUnix

target_include_directories(SPSqliteUtils PUBLIC $ENV{CWPROOT}/include)

target_link_directories(SPSqliteUtils PUBLIC $ENV{CWPROOT}/lib)

target_link_libraries(SPSqliteUtils PUBLIC su par cwp)


# installation

install(TARGETS SPSqliteUtils DESTINATION lib)

install(FILES SPTable.hh DESTINATION include)
