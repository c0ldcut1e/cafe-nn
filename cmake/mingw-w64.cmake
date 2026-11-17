# Cross-compiles for 64-bit Windows with mingw-w64. When wine is installed, the tests run under it so a Windows build is checked too.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(NEX_MINGW_PREFIX x86_64-w64-mingw32 CACHE STRING "Target triple of the mingw-w64 toolchain")
set(CMAKE_C_COMPILER ${NEX_MINGW_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${NEX_MINGW_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${NEX_MINGW_PREFIX}-windres)
set(CMAKE_FIND_ROOT_PATH /usr/${NEX_MINGW_PREFIX})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Static runtime libraries, so the test binaries start under wine without the mingw DLLs on its path
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static -static-libgcc -static-libstdc++")

find_program(NEX_WINE wine)
if(NEX_WINE)
    set(CMAKE_CROSSCOMPILING_EMULATOR ${NEX_WINE})
endif()
