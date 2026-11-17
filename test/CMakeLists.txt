# Each library gets its own test executable from test/<name>, linked against nothing but that library.
set(NEX_TEST_TARGETS "")
foreach(lib ${NN_LIBRARIES})
    file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${lib}/*.cpp)
    if(sources)
        add_executable(${lib}_test ${sources})
        target_link_libraries(${lib}_test PRIVATE ${lib})
        add_test(NAME ${lib}_test COMMAND ${lib}_test)
        list(APPEND NEX_TEST_TARGETS ${lib}_test)
    endif()
endforeach()

if(NEX_BUILD_LUA)
    add_executable(nex_lua_test lua/runner.cpp)
    target_link_libraries(nex_lua_test PRIVATE nex_lua)
    file(GLOB NEX_LUA_TESTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lua/*.lua)
    foreach(script ${NEX_LUA_TESTS})
        get_filename_component(name ${script} NAME_WE)
        add_test(NAME lua_${name} COMMAND nex_lua_test ${script})
    endforeach()
    list(APPEND NEX_TEST_TARGETS nex_lua_test)

    # Editors read lua/meta/nex.lua to know the nex global, so we regenerate it from the real bindings on every build.
    add_custom_target(lua_meta ALL
        COMMAND nex_lua_test ${PROJECT_SOURCE_DIR}/tools/gen_lua_meta.lua ${PROJECT_SOURCE_DIR}/lua/meta
        DEPENDS nex_lua_test
        COMMENT "Generating the LuaLS files in lua/meta")
endif()

if(NEX_SANITIZE AND NOT MSVC)
    foreach(target ${NEX_TEST_TARGETS})
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endforeach()
endif()

# Part of ALL, so a build only succeeds once every test passes.
add_custom_target(check ALL
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS ${NEX_TEST_TARGETS}
    COMMENT "Running tests"
    USES_TERMINAL)
