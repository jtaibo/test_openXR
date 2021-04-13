
set( SRC_FILES
	"main.cpp"
	"xrapp.cpp"
	"xrapp.h"
	"glsystem.cpp"
	"glsystem.h"
	"gfxwrapper_opengl.c"
	"gfxwrapper_opengl.h"
)

find_package(OpenGL REQUIRED)

#set( OPENXR_INCLUDE_DIRS "D:/devel/OpenXR-SDK/include" )
#set( OPENXR_loader_LIBRARY "D:/devel/OpenXR-SDK/build/win64/src/loader/Release/openxr_loader.lib" )
find_package(OpenXR)

add_executable( ${PROJECT_NAME} ${SRC_FILES} )

target_include_directories( ${PROJECT_NAME} PUBLIC ${OPENXR_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR} "${CMAKE_SOURCE_DIR}/external/include" )
target_link_libraries( ${PROJECT_NAME} ${OPENXR_loader_LIBRARY} pathcch)
