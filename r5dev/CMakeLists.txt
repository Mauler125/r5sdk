cmake_minimum_required( VERSION 3.16 )
project( sdk )

add_subdirectory( vpc ) # Must be the first as this creates the shared PCH!
add_subdirectory( tier0 )
add_subdirectory( tier1 )
add_subdirectory( tier2 )
add_subdirectory( vpklib )
add_subdirectory( vscript )
add_subdirectory( vstdlib )
add_subdirectory( vphysics )
add_subdirectory( vguimatsurface )
add_subdirectory( vgui )

add_subdirectory( thirdparty/detours )
add_subdirectory( thirdparty/cppnet )
add_subdirectory( thirdparty/lzham )
add_subdirectory( thirdparty/fastlz )
add_subdirectory( thirdparty/imgui )
add_subdirectory( thirdparty/curl )
add_subdirectory( thirdparty/protobuf )
add_subdirectory( thirdparty/spdlog )
add_subdirectory( thirdparty/sdl )
add_subdirectory( thirdparty/recast )

add_subdirectory( sdklauncher )
add_subdirectory( rtech )
add_subdirectory( protoc )


add_subdirectory( networksystem )
add_subdirectory( pluginsystem )
add_subdirectory( pluginsdk )

add_subdirectory( mathlib )
add_subdirectory( netconsole )
add_subdirectory( naveditor )


add_subdirectory( materialsystem )

add_subdirectory( localize )
add_subdirectory( inputsystem )
add_subdirectory( game )

add_subdirectory( datacache )
add_subdirectory( filesystem )
add_subdirectory( ebisusdk )
add_subdirectory( codecs )
add_subdirectory( engine )

add_subdirectory( core )
add_subdirectory( appframework  )
