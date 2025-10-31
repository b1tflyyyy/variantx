function (create_test name)
    file(GLOB_RECURSE TESTS ${CMAKE_CURRENT_SOURCE_DIR}/*pass.cpp)
    add_executable(${name} ${TESTS})

    target_link_libraries(${name} GTest::gtest_main)
    target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR})

    add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}>)
endfunction()
