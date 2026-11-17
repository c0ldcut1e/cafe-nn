# Every middleware library lives in libs/<name> with its own include/ and src/, and builds into its own lib<name>.a. Some of them only
# make sense on the console, so each one lists the platforms it supports and we skip it everywhere else.

if(NEX_WIIU)
    set(NN_PLATFORM wiiu)
elseif(WIN32)
    set(NN_PLATFORM windows)
elseif(APPLE)
    set(NN_PLATFORM macos)
else()
    set(NN_PLATFORM linux)
endif()

set(NN_LIBRARIES "")

# nn_add_library(<name> PLATFORMS <platform>... [DEPENDS <library>...])
#
# We declare libs/<name> as the static library <name> (exported as nex::<name>), and set NN_HAS_<NAME> so tests and bindings can tell
# whether it was built here. A library whose dependency was skipped is skipped too.
function(nn_add_library name)
    cmake_parse_arguments(ARG "" "" "PLATFORMS;DEPENDS" ${ARGN})
    string(TOUPPER ${name} upper)
    set(supported ON)
    if(NOT NN_PLATFORM IN_LIST ARG_PLATFORMS)
        set(supported OFF)
    endif()
    foreach(dep ${ARG_DEPENDS})
        if(NOT TARGET ${dep})
            set(supported OFF)
        endif()
    endforeach()
    set(NN_HAS_${upper} ${supported} PARENT_SCOPE)
    if(NOT supported)
        message(STATUS "Skipping lib${name}: not supported on ${NN_PLATFORM}")
        return()
    endif()

    set(dir ${PROJECT_SOURCE_DIR}/libs/${name})
    file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${dir}/src/*.cpp)
    add_library(${name} STATIC ${sources})
    add_library(nex::${name} ALIAS ${name})
    target_include_directories(${name} PUBLIC $<BUILD_INTERFACE:${dir}/include> $<INSTALL_INTERFACE:include>)
    target_compile_features(${name} PUBLIC cxx_std_20)
    target_link_libraries(${name} PUBLIC ${ARG_DEPENDS})

    if(MSVC)
        target_compile_options(${name} PRIVATE /W4 /utf-8)
    else()
        target_compile_options(${name} PRIVATE -Wall -Wextra -ffunction-sections -fdata-sections)
    endif()
    if(NEX_WIIU)
        target_link_libraries(${name} PUBLIC wut::wut)
        target_compile_definitions(${name} PUBLIC __WIIU__ __WUT__)
        target_compile_options(${name} PRIVATE -fno-exceptions -fno-rtti)
    endif()

    install(TARGETS ${name} EXPORT nexTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(DIRECTORY ${dir}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    set(NN_LIBRARIES ${NN_LIBRARIES} ${name} PARENT_SCOPE)
endfunction()
