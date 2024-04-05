cmake_minimum_required( VERSION 3.16 )

set( FOLDER_CONTEXT "Foundation" )
add_subdirectory( vpc ) # VPC and Tier0 must be the first as this creates the shared PCH!
add_subdirectory( tier0 )
add_subdirectory( tier1 )
add_subdirectory( tier2 )
add_subdirectory( launcher )
add_subdirectory( appframework  )

set( FOLDER_CONTEXT "Libraries" )
add_subdirectory( mathlib )
add_subdirectory( vpklib )
add_subdirectory( vstdlib )
add_subdirectory( vphysics )
add_subdirectory( ebisusdk )
add_subdirectory( codecs )
add_subdirectory( geforce )

set( FOLDER_CONTEXT "Protocols" )
add_subdirectory( protoc )

set( FOLDER_CONTEXT "Respawn" )
add_subdirectory( rtech )

set( FOLDER_CONTEXT "Thirdparty" )
add_subdirectory( thirdparty/cppnet )
add_subdirectory( thirdparty/curl )
add_subdirectory( thirdparty/sdl )
add_subdirectory( thirdparty/imgui )
add_subdirectory( thirdparty/spdlog )
add_subdirectory( thirdparty/jwt )
add_subdirectory( thirdparty/lzham )
add_subdirectory( thirdparty/fastlz )

set( FOLDER_CONTEXT "Thirdparty/ARM" )
add_subdirectory( thirdparty/mbedtls )

set( FOLDER_CONTEXT "Thirdparty/Google" )
add_subdirectory( thirdparty/protobuf )

set( FOLDER_CONTEXT "Thirdparty/Microsoft" )
add_subdirectory( thirdparty/detours )

set( FOLDER_CONTEXT "Thirdparty/Recast" )
add_subdirectory( thirdparty/recast )

set( FOLDER_CONTEXT "Tools" )
add_subdirectory( sdklauncher )
add_subdirectory( netconsole )
add_subdirectory( naveditor )

set( FOLDER_CONTEXT "System" )
add_subdirectory( networksystem )
add_subdirectory( pluginsystem )
add_subdirectory( materialsystem )
add_subdirectory( inputsystem )
add_subdirectory( filesystem )
add_subdirectory( datacache )
add_subdirectory( studiorender )
add_subdirectory( localize )
add_subdirectory( engine )
add_subdirectory( vguimatsurface )
add_subdirectory( vgui )
add_subdirectory( loader )

set( FOLDER_CONTEXT "Plugins" )
add_subdirectory( pluginsdk )

set( FOLDER_CONTEXT "Game" )
add_subdirectory( vscript )
add_subdirectory( game )

set( FOLDER_CONTEXT "/" )
add_subdirectory( core )
