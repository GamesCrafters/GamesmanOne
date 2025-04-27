cmake_minimum_required(VERSION 3.24)
set(CMAKE_SYSTEM_NAME Darwin)

# ------------------------------------------------------------
# 1.  Detect the Homebrew prefix in the most portable way
# ------------------------------------------------------------
#   • If the user already has   HOMEBREW_PREFIX   in their env
#     (added by   eval "$(/opt/homebrew/bin/brew shellenv)"  )
#     then respect it.
#   • Otherwise fall back to:   brew --prefix
# ------------------------------------------------------------
if(DEFINED ENV{HOMEBREW_PREFIX})
    set(HBREW "$ENV{HOMEBREW_PREFIX}")
else()
    execute_process(
        COMMAND brew --prefix
        OUTPUT_VARIABLE HBRAW
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(HBREW "${HBRAW}")
endif()

# ------------------------------------------------------------
# 2.  Point CMake at the Homebrew LLVM tool-chain
# ------------------------------------------------------------
set(CMAKE_C_COMPILER   "${HBREW}/opt/llvm/bin/clang"  CACHE STRING "")
set(CMAKE_CXX_COMPILER "${HBREW}/opt/llvm/bin/clang++" CACHE STRING "")

# ------------------------------------------------------------
# 3.  Help CMake find headers & libraries for FindOpenMP
# ------------------------------------------------------------
list(APPEND CMAKE_PREFIX_PATH
     "${HBREW}/opt/llvm"
     "${HBREW}/opt/libomp")

# Tell the runtime loader where to find libomp.dylib at execution time
set(CMAKE_INSTALL_RPATH "${HBREW}/opt/libomp/lib")

# ------------------------------------------------------------
# 4.  Default to the host architecture (arm64 or x86_64)
#     Users can still override with  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
# ------------------------------------------------------------
if(NOT CMAKE_OSX_ARCHITECTURES)
    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64")
        set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "")
    else()
        set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "")
    endif()
endif()
