cmake_minimum_required(VERSION 3.1)
project(imgcomp)

add_compile_options( "-fexceptions" "-fopenmp" "-DFLTK_USE_SVG" "-DANIMGIF" )

#set (CMAKE_CXX_STANDARD 11)
#set (CMAKE_BUILD_TYPE "Debug" CACHE STRING "blah" FORCE)
#set (CMAKE_CXX_FLAGS "-fpermissive")

INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR} /home/kevin/proj/fltk/fltk )
#INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR} /home/kevin/proj/fltk/fltk ${PROJECT_SOURCE_DIR}/imgComp-fltkZ/bit7z/include)
LINK_DIRECTORIES(/home/kevin/FLTK14/build/debug/lib)

add_executable( imgcomp main.cpp ViewWin.cpp Webp.cpp filedata.cpp Fl_Image_Display.cxx procThread.cpp
                logging.cpp MostRecentPaths.cpp SharedImageExt.cpp Fl_Anim_GIF_Image.cpp)

#add_executable( imgcompZ imgComp-fltkZ/main.cpp Fl_Image_Display.cxx logging.cpp MostRecentPaths.cpp procThread.cpp
#        SharedImageExt.cpp ViewWin.cpp Webp.cpp imgComp-fltkZ/ArchiveData.cpp imgComp-fltkZ/Extractor.cpp
#        imgComp-fltkZ/filedata.cpp imgComp-fltkZ/FL_Tree_Columns.cpp)

find_library(FLTK fltk /home/kevin/proj/fltk/fltk/lib)
find_library(FLTK_IMG fltk_images /home/kevin/proj/fltk/fltk/lib)
# Using own [modified] PNG library to prevent output to console
find_library(FLTK_PNG fltk_png /home/kevin/proj/fltk/fltk/lib)
find_library(FLTK_JPG fltk_jpeg    /home/kevin/fltk/build/lib)

#set(LINK_FLAGS -no-pie -fopenmp -lX11 -lXext -lm -ldl -lXinerama -lXcursor -lXi
#        -lXrender -lXfixes -ljpeg -lz -lXft -lfontconfig -pthread
#        -lpangocairo-1.0 -lpango-1.0 -lcairo -lpangoft2-1.0 -lgobject-2.0
#        -lglib-2.0 -lharfbuzz -lpangoxft-1.0 -lpthread -lwebp -lwebpdemux)
set(LINK_FLAGS -no-pie -fopenmp -lX11 -lXext -lm -ldl -lXinerama -lXcursor
        -lXrender -lXfixes -lz -lXft -lfontconfig -pthread
        -lpthread -lwebp -lwebpdemux
        )

target_link_libraries(imgcomp LINK_PUBLIC ${FLTK} ${FLTK_IMG} ${FLTK_PNG} ${FLTK_JPG} ${LINK_FLAGS} )

install(TARGETS imgcomp RUNTIME DESTINATION bin)
