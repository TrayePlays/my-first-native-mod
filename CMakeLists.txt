cmake_minimum_required(VERSION 3.31)
project(my_first_native_mod)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

include(cmake/CPM.cmake)

CPMAddPackage("gh:BasedInc/libhat@0.9.0")
CPMAddPackage("gh:TsudaKageyu/minhook@1.3.4")

add_library(my_first_native_mod SHARED dllmain.cpp)
target_link_libraries(my_first_native_mod PRIVATE
        libhat::libhat
        minhook)

#cmake_minimum_required(VERSION 4.1)
#project(my_first_native_mod)
#
#set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
#
#include(cmake/CPM.cmake)
#
#CPMAddPackage("gh:BasedInc/libhat@0.9.0")
#CPMAddPackage("gh:TsudaKageyu/minhook@1.3.4")
#set(KIERO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
#set(KIERO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
#
#CPMAddPackage(
#        NAME kiero
#        GITHUB_REPOSITORY BasedInc/kiero
#        GIT_TAG 269860311ac04b0ee89e68f68bca55abd02a3390
#        OPTIONS
#        "KIERO_INCLUDE_D3D11 ON"
#        "KIERO_INCLUDE_D3D12 ON"
#)
#
#
#
## Add ImGui
#CPMAddPackage(
#        NAME imgui
#        GITHUB_REPOSITORY ocornut/imgui
#        GIT_TAG v1.91.0
#)
#
#
#set(IMGUI_BACKENDS
#        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
#        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
#        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
#)
#
#add_library(my_first_native_mod SHARED dllmain.cpp)
#
#
#target_sources(my_first_native_mod PRIVATE
#        ${imgui_SOURCE_DIR}/imgui.cpp
#        ${imgui_SOURCE_DIR}/imgui_draw.cpp
#        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
#        ${imgui_SOURCE_DIR}/imgui_tables.cpp
#        ${IMGUI_BACKENDS}
#)
#
#target_include_directories(my_first_native_mod PRIVATE
#        ${imgui_SOURCE_DIR}
#        ${imgui_SOURCE_DIR}/backends
#)
#
#target_link_libraries(my_first_native_mod PRIVATE
#        libhat::libhat
#        minhook
#        kiero
#)
