# Lua 5.4.7 from the third_party/lua submodule. We leave out the standalone interpreter (lua.c), the single file build (onelua.c)
# and the internal test library (ltests.c).
# We compile it as C++ so lua_error unwinds through the binding with exceptions and runs destructors, which longjmp would skip.
set(LUA_DIR ${PROJECT_SOURCE_DIR}/third_party/lua)
file(GLOB LUA_SOURCES CONFIGURE_DEPENDS ${LUA_DIR}/*.c)
list(REMOVE_ITEM LUA_SOURCES ${LUA_DIR}/lua.c ${LUA_DIR}/onelua.c ${LUA_DIR}/ltests.c)
set_source_files_properties(${LUA_SOURCES} PROPERTIES LANGUAGE CXX)
add_library(lua54 STATIC ${LUA_SOURCES})
target_include_directories(lua54 PUBLIC
    $<BUILD_INTERFACE:${LUA_DIR}>
    $<INSTALL_INTERFACE:include/lua>)
set_target_properties(lua54 PROPERTIES LINKER_LANGUAGE CXX)

if(MSVC)
    target_compile_options(lua54 PRIVATE /TP /EHsc /W0)
else()
    target_compile_options(lua54 PRIVATE -x c++ -fexceptions -w)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(lua54 PUBLIC LUA_USE_LINUX)
    target_link_libraries(lua54 PUBLIC ${CMAKE_DL_LIBS} m)
elseif(APPLE)
    target_compile_definitions(lua54 PUBLIC LUA_USE_MACOSX)
endif()

# devkitPPC's limits.h only defines LLONG_MAX for C99 and newer, which a C++ compile never is, so luaconf.h would think long long is
# missing in every C++ file that includes it. Only <climits> defines it for C++.
if(NEX_WIIU)
    target_compile_options(lua54 PUBLIC -include climits)
endif()
