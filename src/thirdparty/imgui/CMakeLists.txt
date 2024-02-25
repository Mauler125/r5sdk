cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "libimgui" "" ${FOLDER_CONTEXT} TRUE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Core"
	"imconfig.h"
	"imgui.cpp"
	"imgui.h"
	"imgui_demo.cpp"
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
)

add_sources( SOURCE_GROUP "Misc"
	"misc/imgui_editor.cpp"
	"misc/imgui_editor.h"
	"misc/imgui_logger.cpp"
	"misc/imgui_logger.h"
	"misc/imgui_snapshot.cpp"
	"misc/imgui_snapshot.h"
	"misc/imgui_utility.cpp"
	"misc/imgui_utility.h"
	"misc/cpp/imgui_stdlib.cpp"
	"misc/cpp/imgui_stdlib.h"
)

end_sources()
thirdparty_suppress_warnings()
target_compile_definitions( ${PROJECT_NAME} PRIVATE BUILDING_LIBIMGUI )
