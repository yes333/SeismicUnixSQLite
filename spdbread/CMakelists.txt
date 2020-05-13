
add_executable(spdbread spdbread.cpp SPParsers.cpp)

target_include_directories(spdbread PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(spdbread PUBLIC SPFramework SPSqliteUtils)
target_link_libraries(spdbread PUBLIC sqlite3)

install(TARGETS spdbread DESTINATION bin)
