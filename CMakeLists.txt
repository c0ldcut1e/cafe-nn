cmake_minimum_required(VERSION 3.20)
project(cafe-nn VERSION 3.10.0 LANGUAGES C CXX)

option(NEX_BUILD_TESTS "Build the tests and run them as part of the build" ON)
option(NEX_BUILD_LUA "Build the Lua bindings" ON)
option(NEX_SANITIZE "Build host tests with AddressSanitizer and UBSan" OFF)

include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(CMAKE_SYSTEM_NAME STREQUAL "CafeOS" OR DEFINED WUT_ROOT)
    set(NEX_WIIU ON)
else()
    set(NEX_WIIU OFF)
endif()

if(NEX_WIIU)
    # We link wut ourselves so anything that links nex gets it. The devkitPro toolchain already passes wut.specs, and gcc refuses the
    # specs twice, so we only add them for builds that set WUT_ROOT without that toolchain.
    if(NOT TARGET wut::wut)
        add_library(wut::wut STATIC IMPORTED)
        set_target_properties(wut::wut PROPERTIES
            IMPORTED_LOCATION ${WUT_ROOT}/lib/libwut.a
            IMPORTED_LOCATION_DEBUG ${WUT_ROOT}/lib/libwutd.a
            INTERFACE_INCLUDE_DIRECTORIES ${WUT_ROOT}/include)
        if(NOT CMAKE_EXE_LINKER_FLAGS MATCHES "wut\\.specs" AND NOT CMAKE_EXE_LINKER_FLAGS_INIT MATCHES "wut\\.specs")
            set_property(TARGET wut::wut PROPERTY INTERFACE_LINK_OPTIONS "-specs=${WUT_ROOT}/share/wut.specs")
        endif()
    endif()
endif()

include(cmake/NNLibrary.cmake)

# Each library only depends on what it actually uses, so a game can take libpead without dragging in libnex.
nn_add_library(crypto PLATFORMS wiiu linux windows macos)
nn_add_library(pead PLATFORMS wiiu linux windows macos)
nn_add_library(nex PLATFORMS wiiu linux windows macos)
nn_add_library(pia PLATFORMS wiiu linux windows macos DEPENDS nex pead crypto)

if(WIN32)
    target_link_libraries(crypto PUBLIC bcrypt)
    target_link_libraries(nex PUBLIC ws2_32)
endif()

if(NEX_BUILD_LUA)
    include(cmake/Lua.cmake)
    add_subdirectory(lua)
endif()

# Tests only run where the build can execute what it produced, so a Wii U build skips them.
if(NEX_BUILD_TESTS AND NOT NEX_WIIU AND (NOT CMAKE_CROSSCOMPILING OR CMAKE_CROSSCOMPILING_EMULATOR))
    enable_testing()
    add_subdirectory(test)
endif()

install(EXPORT nexTargets NAMESPACE nex:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/nex)
