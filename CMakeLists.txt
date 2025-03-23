cmake_minimum_required (VERSION 3.20)

project ("Span Lang")

include(FetchContent)
include(FeatureSummary)

set(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
set(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if(MSVC)
    message(STATUS "MSVC version ${MSVC_VERSION}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /EHsc")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /EHsc")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi /EHa")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Od /Zi /EHa")
else()
    if (true)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fno-exceptions")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-3 -fno-exceptions")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -gdwarf-3 -O3 -fno-exceptions")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -gdwarf-3 -fno-exceptions")
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -gdwarf-3 -O3 -fsanitize=undefined,address -fno-exceptions")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -gdwarf-3 -O1 -fsanitize=undefined,address -fno-exceptions")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -gdwarf-3 -O3 -fsanitize=undefined,address -fno-exceptions")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -gdwarf-3 -O1 -fsanitize=undefined,address -fno-exceptions")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined,address -fno-exceptions")
    endif()
endif()

find_package(Git QUIET)

if(MSVC)
    if (C3_LLVM_VERSION STREQUAL "auto")
        set(C3_LLVM_VERSION "18")
    endif()
    FetchContent_Declare(
            LLVM_Windows
            URL https://github.com/c3lang/win-llvm/releases/download/llvm_18_1_8_with_rt/llvm-18.1.8-windows-amd64-msvc17-libcmt.7z
    )
    FetchContent_Declare(
            LLVM_Windows_debug
            URL https://github.com/c3lang/win-llvm/releases/download/llvm_18_1_8_with_rt/llvm-18.1.8-windows-amd64-msvc17-libcmt-dbg.7z
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message("Loading Windows LLVM debug libraries, this may take a while...")
        FetchContent_MakeAvailable(LLVM_Windows_debug)
        set(llvm_dir ${llvm_windows_debug_SOURCE_DIR})
    else()
        message("Loading Windows LLVM libraries, this may take a while...")
        FetchContent_MakeAvailable(LLVM_Windows)
        set(llvm_dir ${llvm_windows_SOURCE_DIR})
    endif()
    set(CMAKE_SYSTEM_PREFIX_PATH ${llvm_dir} ${CMAKE_SYSTEM_PREFIX_PATH})

    find_package(LLVM REQUIRED CONFIG)
    find_package(LLD REQUIRED CONFIG)
else()
    find_package(LLVM REQUIRED CONFIG)
endif()


include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(LLVM_LINK_COMPONENTS
	    AllTargetsAsmParsers
	    AllTargetsCodeGens
	    AllTargetsDescs
	    AllTargetsDisassemblers
	    AllTargetsInfos
	    Analysis
	    AsmPrinter
	    BitReader
	    Core
	    DebugInfoPDB
	    InstCombine
	    IrReader
	    LibDriver
	    Linker
	    LTO
	    MC
	    MCDisassembler
	    native
	    nativecodegen
	    Object
	    Option
	    ScalarOpts
	    Support
	    Target
	    TransformUtils
	    WindowsManifest
	    WindowsDriver
    )

llvm_map_components_to_libnames(llvm_libs ${LLVM_LINK_COMPONENTS})

find_library(LLD_COFF NAMES lldCOFF.lib lldCOFF.a liblldCOFF.a PATHS ${LLVM_LIBRARY_DIRS} NO_DEFAULT_PATH)
find_library(LLD_COMMON NAMES lldCommon.lib lldCommon.a liblldCommon.a PATHS ${LLVM_LIBRARY_DIRS} NO_DEFAULT_PATH)
find_library(LLD_ELF NAMES lldELF.lib lldELF.a liblldELF.a PATHS ${LLVM_LIBRARY_DIRS} NO_DEFAULT_PATH)
find_library(LLD_MACHO NAMES lldMachO.lib lldMachO.a liblldMachO.a PATHS ${LLVM_LIBRARY_DIRS} NO_DEFAULT_PATH)
find_library(LLD_MINGW NAMES lldMinGW.lib lldMinGW.a liblldMinGW.a PATHS ${LLVM_LIBRARY_DIRS} NO_DEFAULT_PATH)
find_library(LLD_WASM NAMES lldWasm.lib lldWasm.a liblldWasm.a PATHS ${LLVM_LIBRARY_DIRS} NO_DEFAULT_PATH)


find_library(LLD_LOONG NAMES libLLVMLoongArchCodeGen.lib libLLVMLoongArchAsmParser.lib libLLVMLoongArchCodeGen.a libLLVMLoongArchAsmParser.a PATHS ${LLVM_LIBRARY_DIRS} NO_DEFAULT_PATH)
set(lld_libs
    ${LLD_COFF}
    ${LLD_COMMON}
    ${LLD_WASM}
    ${LLD_MINGW}
    ${LLD_ELF}
    ${LLD_MACHO}
    )

add_executable(spanLang "src/main.cpp" "src/span.cpp" "src/span.h" "src/utils.cpp" "src/utils.h" "src/token.h" "src/token.cpp"  "src/module.h" "src/module.cpp" "src/types.h" "src/types.cpp")

target_link_libraries(spanLang ${llvm_libs} ${lld_libs})
