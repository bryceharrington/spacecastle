file(GLOB spacecastle_SOURCES *.cpp)

add_executable(spacecastle ${spacecastle_SOURCES})
target_link_libraries(spacecastle ${spacecastle_LIBS})
