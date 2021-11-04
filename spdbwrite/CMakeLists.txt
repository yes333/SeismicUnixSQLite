
add_executable(spdbwrite spdbwrite.cpp)

target_link_libraries(spdbwrite PUBLIC SPFramework SPSqliteUtils)
target_link_libraries(spdbwrite PUBLIC sqlite3)

install(TARGETS spdbwrite DESTINATION bin)
