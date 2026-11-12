cmake_minimum_required(VERSION 3.16)
project(pixel_painter C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(PP_WERROR "Treat compiler warnings as errors" OFF)

# --- raylib 5.5 -------------------------------------------------------------
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(raylib)

# --- Warnings for our own targets only --------------------------------------
function(pp_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
    if(PP_WERROR)
      target_compile_options(${target} PRIVATE /WX)
    endif()
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra)
    if(PP_WERROR)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  endif()
endfunction()

# Logic files: must not include raylib.h (BUILD_INSTRUCTIONS.txt 4.2).
set(PP_LOGIC_SOURCES
  src/app.c
  src/grid.c
  src/history.c
  src/palette.c
  src/hexparse.c
  src/export_png.c
  src/export_bmp.c)

add_library(pp_logic STATIC ${PP_LOGIC_SOURCES})
target_include_directories(pp_logic PUBLIC src)
pp_warnings(pp_logic)

# --- App --------------------------------------------------------------------
add_executable(pixel_painter src/main.c src/render.c src/replay.c)
target_link_libraries(pixel_painter PRIVATE pp_logic raylib)
pp_warnings(pixel_painter)
if(APPLE)
  target_link_libraries(pixel_painter PRIVATE
    "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
endif()

# --- Unit tests (no raylib) -------------------------------------------------
add_executable(pp_tests tests/test_main.c)
target_link_libraries(pp_tests PRIVATE pp_logic)
pp_warnings(pp_tests)

enable_testing()
add_test(NAME pp_tests COMMAND pp_tests)
