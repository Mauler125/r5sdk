cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "libimgui" "" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Core"
	"imconfig.h"
	"imgui.cpp"
	"imgui.h"
	"imgui_draw.cpp"
	"imgui_internal.h"
	"imgui_tables.cpp"
	"imgui_widgets.cpp"
	"imstb_rectpack.h"
	"imstb_textedit.h"
	"imstb_truetype.h"
)

add_sources( SOURCE_GROUP "Backends"
	"backends/imgui_impl_dx11.cpp"
	"backends/imgui_impl_dx11.h"
	"backends/imgui_impl_win32.cpp"
	"backends/imgui_impl_win32.h"
	"backends/imgui_impl_sdl2.cpp"
	"backends/imgui_impl_sdl2.h"
	"backends/imgui_impl_opengl2.cpp"
	"backends/imgui_impl_opengl2.h"
)

add_sources( SOURCE_GROUP "Misc"
	"misc/imgui_editor.cpp"
	"misc/imgui_editor.h"
	"misc/imgui_logger.cpp"
	"misc/imgui_logger.h"
	"misc/imgui_plotter.cpp"
	"misc/imgui_plotter.h"
	"misc/imgui_plotter_items.cpp"
	"misc/imgui_plotter_internal.h"
	"misc/imgui_snapshot.cpp"
	"misc/imgui_snapshot.h"
	"misc/imgui_style.cpp"
	"misc/imgui_style.h"
	"misc/imgui_utility.cpp"
	"misc/imgui_utility.h"
	"misc/imgui_wrapper.cpp"
	"misc/imgui_wrapper.h"
	"misc/cpp/imgui_stdlib.cpp"
	"misc/cpp/imgui_stdlib.h"
)

add_sources( SOURCE_GROUP "Demos"
	"demos/imgui_demo.cpp"
	"demos/imgui_plotter_demo.cpp"
)

end_sources()
thirdparty_suppress_warnings()

target_compile_definitions( ${PROJECT_NAME} PRIVATE BUILDING_LIBIMGUI )

target_include_directories( ${PROJECT_NAME} PRIVATE "${THIRDPARTY_SOURCE_DIR}/imgui/" )
target_include_directories( ${PROJECT_NAME} PRIVATE "${THIRDPARTY_SOURCE_DIR}/imgui/misc/" )
target_include_directories( ${PROJECT_NAME} PRIVATE "${THIRDPARTY_SOURCE_DIR}/sdl/include/" )
