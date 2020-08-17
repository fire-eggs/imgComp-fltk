cmake_minimum_required(VERSION 3.1)
project(imgcomp)

set (CMAKE_CXX_STANDARD 11)
set (CMAKE_BUILD_TYPE "Debug" CACHE STRING "blah" FORCE)
set (CMAKE_CXX_FLAGS "-fpermissive")

INCLUDE_DIRECTORIES( /home/kevin/FLTK14 /home/kevin/FLTK14/build/debug /home/kevin/FLTK14 )
LINK_DIRECTORIES(/home/kevin/FLTK14/build/debug/lib)

add_executable( imgcomp main.cpp ViewWin.cpp filedata.cpp Fl_Image_Display.cxx )

TARGET_LINK_LIBRARIES (imgcomp fltk fltk_images fltk_jpeg png z dl Xrender Xext Xft fontconfig pthread m X11)

install(TARGETS imgcomp RUNTIME DESTINATION bin)
