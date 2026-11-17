file(GLOB NEX_LUA_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# The bindings for a library that wasn't built on this platform are left out, and Module.cpp skips opening them.
foreach(lib crypto pead)
    string(TOUPPER ${lib} upper)
    if(NN_HAS_${upper})
        list(APPEND NEX_LUA_DEFINES NN_HAS_${upper})
    else()
        string(SUBSTRING ${lib} 0 1 first)
        string(TOUPPER ${first} first)
        string(SUBSTRING ${lib} 1 -1 rest)
        list(REMOVE_ITEM NEX_LUA_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/${first}${rest}.cpp)
    endif()
endforeach()
add_library(nex_lua STATIC ${NEX_LUA_SOURCES})
add_library(nex::lua ALIAS nex_lua)
target_include_directories(nex_lua PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(nex_lua PUBLIC ${NN_LIBRARIES} lua54)
target_compile_definitions(nex_lua PRIVATE ${NEX_LUA_DEFINES})

# Lua errors unwind as C++ exceptions, so the binding needs them even on Wii U where the core goes without.
if(MSVC)
    target_compile_options(nex_lua PRIVATE /W4 /utf-8 /EHsc)
else()
    target_compile_options(nex_lua PRIVATE -Wall -Wextra -fexceptions)
endif()

install(TARGETS nex_lua lua54 EXPORT nexTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
