cmake_minimum_required( VERSION 3.16 )
add_module( "lib" "libsdl2" "" ${FOLDER_CONTEXT} FALSE TRUE )

start_sources()

add_sources( SOURCE_GROUP "Core"
    "src/hidapi/SDL_hidapi_c.h"
    "src/SDL.c"
    "src/SDL_assert.c"
    "src/SDL_assert_c.h"
    "src/SDL_dataqueue.c"
    "src/SDL_dataqueue.h"
    "src/SDL_error.c"
    "src/SDL_error_c.h"
    "src/SDL_hints.c"
    "src/SDL_hints_c.h"
    "src/SDL_internal.h"
    "src/SDL_log.c"
)

add_sources( SOURCE_GROUP "API Headers"
    "include/begin_code.h"
    "include/close_code.h"
    "include/SDL.h"
    "include/SDL_assert.h"
    "include/SDL_atomic.h"
    "include/SDL_audio.h"
    "include/SDL_bits.h"
    "include/SDL_blendmode.h"
    "include/SDL_clipboard.h"
    "include/SDL_config.h"
    "include/SDL_config_windows.h"
    "include/SDL_copying.h"
    "include/SDL_cpuinfo.h"
    "include/SDL_egl.h"
    "include/SDL_endian.h"
    "include/SDL_error.h"
    "include/SDL_events.h"
    "include/SDL_filesystem.h"
    "include/SDL_gamecontroller.h"
    "include/SDL_gesture.h"
    "include/SDL_haptic.h"
    "include/SDL_hidapi.h"
    "include/SDL_hints.h"
    "include/SDL_joystick.h"
    "include/SDL_keyboard.h"
    "include/SDL_keycode.h"
    "include/SDL_loadso.h"
    "include/SDL_locale.h"
    "include/SDL_log.h"
    "include/SDL_main.h"
    "include/SDL_messagebox.h"
    "include/SDL_metal.h"
    "include/SDL_misc.h"
    "include/SDL_mouse.h"
    "include/SDL_mutex.h"
    "include/SDL_name.h"
    "include/SDL_opengl.h"
    "include/SDL_opengl_glext.h"
    "include/SDL_opengles.h"
    "include/SDL_opengles2.h"
    "include/SDL_opengles2_gl2.h"
    "include/SDL_opengles2_gl2ext.h"
    "include/SDL_opengles2_gl2platform.h"
    "include/SDL_opengles2_khrplatform.h"
    "include/SDL_pixels.h"
    "include/SDL_platform.h"
    "include/SDL_power.h"
    "include/SDL_quit.h"
    "include/SDL_rect.h"
    "include/SDL_render.h"
    "include/SDL_revision.h"
    "include/SDL_rwops.h"
    "include/SDL_scancode.h"
    "include/SDL_sensor.h"
    "include/SDL_shape.h"
    "include/SDL_stdinc.h"
    "include/SDL_surface.h"
    "include/SDL_system.h"
    "include/SDL_syswm.h"
    "include/SDL_test.h"
    "include/SDL_test_assert.h"
    "include/SDL_test_common.h"
    "include/SDL_test_compare.h"
    "include/SDL_test_crc32.h"
    "include/SDL_test_font.h"
    "include/SDL_test_fuzzer.h"
    "include/SDL_test_harness.h"
    "include/SDL_test_images.h"
    "include/SDL_test_log.h"
    "include/SDL_test_md5.h"
    "include/SDL_test_memory.h"
    "include/SDL_test_random.h"
    "include/SDL_thread.h"
    "include/SDL_timer.h"
    "include/SDL_touch.h"
    "include/SDL_types.h"
    "include/SDL_version.h"
    "include/SDL_video.h"
    "include/SDL_vulkan.h"
)

add_sources( SOURCE_GROUP "Atomic"
    "src/atomic/SDL_atomic.c"
    "src/atomic/SDL_spinlock.c"
)

add_sources( SOURCE_GROUP "Audio"
    "src/audio/SDL_audio.c"
    "src/audio/SDL_audio_c.h"
    "src/audio/SDL_audiocvt.c"
    "src/audio/SDL_audiodev.c"
    "src/audio/SDL_audiodev_c.h"
    "src/audio/SDL_audiotypecvt.c"
    "src/audio/SDL_mixer.c"
    "src/audio/SDL_sysaudio.h"
    "src/audio/SDL_wave.c"
    "src/audio/SDL_wave.h"
)

add_sources( SOURCE_GROUP "Audio/DirectSound"
    "src/audio/directsound/SDL_directsound.c"
    "src/audio/directsound/SDL_directsound.h"
)
source_group("audio\\directsound" FILES ${audio__directsound})

add_sources( SOURCE_GROUP "Audio/Disk"
    "src/audio/disk/SDL_diskaudio.c"
    "src/audio/disk/SDL_diskaudio.h"
)

add_sources( SOURCE_GROUP "Audio/Dummy"
    "src/audio/dummy/SDL_dummyaudio.c"
    "src/audio/dummy/SDL_dummyaudio.h"
)

add_sources( SOURCE_GROUP "Audio/WasAPI"
    "src/audio/wasapi/SDL_wasapi.c"
    "src/audio/wasapi/SDL_wasapi.h"
    "src/audio/wasapi/SDL_wasapi_win32.c"
)

add_sources( SOURCE_GROUP "Audio/WinMM"
    "src/audio/winmm/SDL_winmm.c"
    "src/audio/winmm/SDL_winmm.h"
)

add_sources( SOURCE_GROUP "Core/Windows"
    "src/core/windows/SDL_directx.h"
    "src/core/windows/SDL_hid.c"
    "src/core/windows/SDL_hid.h"
    "src/core/windows/SDL_windows.c"
    "src/core/windows/SDL_windows.h"
    "src/core/windows/SDL_xinput.c"
    "src/core/windows/SDL_xinput.h"
)

add_sources( SOURCE_GROUP "CpuInfo"
    "src/cpuinfo/SDL_cpuinfo.c"
)

add_sources( SOURCE_GROUP "DynAPI"
    "src/dynapi/SDL_dynapi.c"
    "src/dynapi/SDL_dynapi.h"
    "src/dynapi/SDL_dynapi_overrides.h"
    "src/dynapi/SDL_dynapi_procs.h"
)

add_sources( SOURCE_GROUP "Events"
    "src/events/blank_cursor.h"
    "src/events/default_cursor.h"
    "src/events/scancodes_windows.h"
    "src/events/SDL_clipboardevents.c"
    "src/events/SDL_clipboardevents_c.h"
    "src/events/SDL_displayevents.c"
    "src/events/SDL_displayevents_c.h"
    "src/events/SDL_dropevents.c"
    "src/events/SDL_dropevents_c.h"
    "src/events/SDL_events.c"
    "src/events/SDL_events_c.h"
    "src/events/SDL_gesture.c"
    "src/events/SDL_gesture_c.h"
    "src/events/SDL_keyboard.c"
    "src/events/SDL_keyboard_c.h"
    "src/events/SDL_mouse.c"
    "src/events/SDL_mouse_c.h"
    "src/events/SDL_quit.c"
    "src/events/SDL_sysevents.h"
    "src/events/SDL_touch.c"
    "src/events/SDL_touch_c.h"
    "src/events/SDL_windowevents.c"
    "src/events/SDL_windowevents_c.h"
)

add_sources( SOURCE_GROUP "File"
    "src/file/SDL_rwops.c"
)

add_sources( SOURCE_GROUP "FileSystem/Windows"
    "src/filesystem/windows/SDL_sysfilesystem.c"
)

add_sources( SOURCE_GROUP "Haptic"
    "src/haptic/SDL_haptic.c"
    "src/haptic/SDL_haptic_c.h"
    "src/haptic/SDL_syshaptic.h"
)

add_sources( SOURCE_GROUP "Haptic/Dummy"
    "src/haptic/dummy/SDL_syshaptic.c"
)

add_sources( SOURCE_GROUP "Haptic/Windows"
    "src/haptic/windows/SDL_dinputhaptic.c"
    "src/haptic/windows/SDL_dinputhaptic_c.h"
    "src/haptic/windows/SDL_windowshaptic.c"
    "src/haptic/windows/SDL_windowshaptic_c.h"
    "src/haptic/windows/SDL_xinputhaptic.c"
    "src/haptic/windows/SDL_xinputhaptic_c.h"
)

add_sources( SOURCE_GROUP "HidAPI"
    "src/hidapi/SDL_hidapi.c"
    "src/hidapi/hidapi/hidapi.h"
)

add_sources( SOURCE_GROUP "Joystic"
    "src/joystick/controller_type.h"
    "src/joystick/SDL_gamecontroller.c"
    "src/joystick/SDL_gamecontrollerdb.h"
    "src/joystick/SDL_joystick.c"
    "src/joystick/SDL_joystick_c.h"
    "src/joystick/SDL_sysjoystick.h"
    "src/joystick/usb_ids.h"
)

add_sources( SOURCE_GROUP "Joystick/Dummy"
    "src/joystick/dummy/SDL_sysjoystick.c"
)

add_sources( SOURCE_GROUP "Joystick/HidAPI"
    "src/joystick/hidapi/SDL_hidapi_gamecube.c"
    "src/joystick/hidapi/SDL_hidapi_luna.c"
    "src/joystick/hidapi/SDL_hidapi_ps4.c"
    "src/joystick/hidapi/SDL_hidapi_ps5.c"
    "src/joystick/hidapi/SDL_hidapi_rumble.c"
    "src/joystick/hidapi/SDL_hidapi_rumble.h"
    "src/joystick/hidapi/SDL_hidapi_stadia.c"
    "src/joystick/hidapi/SDL_hidapi_switch.c"
    "src/joystick/hidapi/SDL_hidapi_xbox360.c"
    "src/joystick/hidapi/SDL_hidapi_xbox360w.c"
    "src/joystick/hidapi/SDL_hidapi_xboxone.c"
    "src/joystick/hidapi/SDL_hidapijoystick.c"
    "src/joystick/hidapi/SDL_hidapijoystick_c.h"
)

add_sources( SOURCE_GROUP "Joystick/Virtual"
    "src/joystick/virtual/SDL_virtualjoystick.c"
    "src/joystick/virtual/SDL_virtualjoystick_c.h"
)

add_sources( SOURCE_GROUP "Joystick/Windows"
    "src/joystick/windows/SDL_dinputjoystick.c"
    "src/joystick/windows/SDL_dinputjoystick_c.h"
    "src/joystick/windows/SDL_rawinputjoystick.c"
    "src/joystick/windows/SDL_rawinputjoystick_c.h"
    "src/joystick/windows/SDL_windows_gaming_input.c"
    "src/joystick/windows/SDL_windowsjoystick.c"
    "src/joystick/windows/SDL_windowsjoystick_c.h"
    "src/joystick/windows/SDL_xinputjoystick.c"
    "src/joystick/windows/SDL_xinputjoystick_c.h"
)

add_sources( SOURCE_GROUP "LibM"
    "src/libm/e_atan2.c"
    "src/libm/e_exp.c"
    "src/libm/e_fmod.c"
    "src/libm/e_log.c"
    "src/libm/e_log10.c"
    "src/libm/e_pow.c"
    "src/libm/e_rem_pio2.c"
    "src/libm/e_sqrt.c"
    "src/libm/k_cos.c"
    "src/libm/k_rem_pio2.c"
    "src/libm/k_sin.c"
    "src/libm/k_tan.c"
    "src/libm/math_libm.h"
    "src/libm/math_private.h"
    "src/libm/s_atan.c"
    "src/libm/s_copysign.c"
    "src/libm/s_cos.c"
    "src/libm/s_fabs.c"
    "src/libm/s_floor.c"
    "src/libm/s_scalbn.c"
    "src/libm/s_sin.c"
    "src/libm/s_tan.c"
)

add_sources( SOURCE_GROUP "LoadSO/Windows"
    "src/loadso/windows/SDL_sysloadso.c"
)

add_sources( SOURCE_GROUP "Locale"
    "src/locale/SDL_locale.c"
    "src/locale/SDL_syslocale.h"
)

add_sources( SOURCE_GROUP "Locale/Windows"
    "src/locale/windows/SDL_syslocale.c"
)

add_sources( SOURCE_GROUP "Main/Windows"
    "src/main/windows/SDL_windows_main.c"
)

add_sources( SOURCE_GROUP "Misc"
    "src/misc/SDL_sysurl.h"
    "src/misc/SDL_url.c"
)

add_sources( SOURCE_GROUP "Misc/Windows"
    "src/misc/windows/SDL_sysurl.c"
)

add_sources( SOURCE_GROUP "Power"
    "src/power/SDL_power.c"
    "src/power/SDL_syspower.h"
)

add_sources( SOURCE_GROUP "Power/Windows"
    "src/power/windows/SDL_syspower.c"
)

add_sources( SOURCE_GROUP "Render"
    "src/render/SDL_d3dmath.c"
    "src/render/SDL_d3dmath.h"
    "src/render/SDL_render.c"
    "src/render/SDL_sysrender.h"
    "src/render/SDL_yuv_sw.c"
    "src/render/SDL_yuv_sw_c.h"
)

add_sources( SOURCE_GROUP "Render/Direct3D"
    "src/render/direct3d/SDL_render_d3d.c"
    "src/render/direct3d/SDL_shaders_d3d.c"
    "src/render/direct3d/SDL_shaders_d3d.h"
)

add_sources( SOURCE_GROUP "Render/Direct3D11"
    "src/render/direct3d11/SDL_render_d3d11.c"
    "src/render/direct3d11/SDL_shaders_d3d11.c"
    "src/render/direct3d11/SDL_shaders_d3d11.h"
)

add_sources( SOURCE_GROUP "Render/OpenGL"
    "src/render/opengl/SDL_glfuncs.h"
    "src/render/opengl/SDL_render_gl.c"
    "src/render/opengl/SDL_shaders_gl.c"
    "src/render/opengl/SDL_shaders_gl.h"
)

add_sources( SOURCE_GROUP "Render/OpenGLES2"
    "src/render/opengles2/SDL_gles2funcs.h"
    "src/render/opengles2/SDL_render_gles2.c"
    "src/render/opengles2/SDL_shaders_gles2.c"
    "src/render/opengles2/SDL_shaders_gles2.h"
)

add_sources( SOURCE_GROUP "Render/Software"
    "src/render/software/SDL_blendfillrect.c"
    "src/render/software/SDL_blendfillrect.h"
    "src/render/software/SDL_blendline.c"
    "src/render/software/SDL_blendline.h"
    "src/render/software/SDL_blendpoint.c"
    "src/render/software/SDL_blendpoint.h"
    "src/render/software/SDL_draw.h"
    "src/render/software/SDL_drawline.c"
    "src/render/software/SDL_drawline.h"
    "src/render/software/SDL_drawpoint.c"
    "src/render/software/SDL_drawpoint.h"
    "src/render/software/SDL_render_sw.c"
    "src/render/software/SDL_render_sw_c.h"
    "src/render/software/SDL_rotate.c"
    "src/render/software/SDL_rotate.h"
    "src/render/software/SDL_triangle.c"
    "src/render/software/SDL_triangle.h"
)

add_sources( SOURCE_GROUP "Sensor"
    "src/sensor/SDL_sensor.c"
    "src/sensor/SDL_sensor_c.h"
    "src/sensor/SDL_syssensor.h"
)

add_sources( SOURCE_GROUP "Sensor/Dummy"
    "src/sensor/dummy/SDL_dummysensor.c"
    "src/sensor/dummy/SDL_dummysensor.h"
)

add_sources( SOURCE_GROUP "Sensor/Windows"
    "src/sensor/windows/SDL_windowssensor.c"
    "src/sensor/windows/SDL_windowssensor.h"
)

add_sources( SOURCE_GROUP "StdLib"
    "src/stdlib/SDL_crc32.c"
    "src/stdlib/SDL_getenv.c"
    "src/stdlib/SDL_iconv.c"
    "src/stdlib/SDL_malloc.c"
    "src/stdlib/SDL_qsort.c"
    "src/stdlib/SDL_stdlib.c"
    "src/stdlib/SDL_string.c"
    "src/stdlib/SDL_strtokr.c"
)

add_sources( SOURCE_GROUP "Thread"
    "src/thread/SDL_systhread.h"
    "src/thread/SDL_thread.c"
    "src/thread/SDL_thread_c.h"
)

add_sources( SOURCE_GROUP "Thread/Generic"
    "src/thread/generic/SDL_syscond.c"
    "src/thread/generic/SDL_syscond_c.h"
)

add_sources( SOURCE_GROUP "Thread/Windows"
    "src/thread/windows/SDL_syscond_cv.c"
    "src/thread/windows/SDL_sysmutex.c"
    "src/thread/windows/SDL_sysmutex_c.h"
    "src/thread/windows/SDL_syssem.c"
    "src/thread/windows/SDL_systhread.c"
    "src/thread/windows/SDL_systhread_c.h"
    "src/thread/windows/SDL_systls.c"
)

add_sources( SOURCE_GROUP "Timer"
    "src/timer/SDL_timer.c"
    "src/timer/SDL_timer_c.h"
)

add_sources( SOURCE_GROUP "Timer/Windows"
    "src/timer/windows/SDL_systimer.c"
)

add_sources( SOURCE_GROUP "Video"
    "src/video/SDL_blit.c"
    "src/video/SDL_blit.h"
    "src/video/SDL_blit_0.c"
    "src/video/SDL_blit_1.c"
    "src/video/SDL_blit_A.c"
    "src/video/SDL_blit_auto.c"
    "src/video/SDL_blit_auto.h"
    "src/video/SDL_blit_copy.c"
    "src/video/SDL_blit_copy.h"
    "src/video/SDL_blit_N.c"
    "src/video/SDL_blit_slow.c"
    "src/video/SDL_blit_slow.h"
    "src/video/SDL_bmp.c"
    "src/video/SDL_clipboard.c"
    "src/video/SDL_egl.c"
    "src/video/SDL_egl_c.h"
    "src/video/SDL_fillrect.c"
    "src/video/SDL_pixels.c"
    "src/video/SDL_pixels_c.h"
    "src/video/SDL_rect.c"
    "src/video/SDL_rect_c.h"
    "src/video/SDL_RLEaccel.c"
    "src/video/SDL_RLEaccel_c.h"
    "src/video/SDL_shape.c"
    "src/video/SDL_shape_internals.h"
    "src/video/SDL_stretch.c"
    "src/video/SDL_surface.c"
    "src/video/SDL_sysvideo.h"
    "src/video/SDL_video.c"
    "src/video/SDL_vulkan_internal.h"
    "src/video/SDL_vulkan_utils.c"
    "src/video/SDL_yuv.c"
    "src/video/SDL_yuv_c.h"
)

add_sources( SOURCE_GROUP "Video/Dummy"
    "src/video/dummy/SDL_nullevents.c"
    "src/video/dummy/SDL_nullevents_c.h"
    "src/video/dummy/SDL_nullframebuffer.c"
    "src/video/dummy/SDL_nullframebuffer_c.h"
    "src/video/dummy/SDL_nullvideo.c"
    "src/video/dummy/SDL_nullvideo.h"
)

add_sources( SOURCE_GROUP "Video/Khronos/Vulkan"
    "src/video/khronos/vulkan/vk_icd.h"
    "src/video/khronos/vulkan/vk_layer.h"
    "src/video/khronos/vulkan/vk_platform.h"
    "src/video/khronos/vulkan/vk_sdk_platform.h"
    "src/video/khronos/vulkan/vulkan.h"
    "src/video/khronos/vulkan/vulkan.hpp"
    "src/video/khronos/vulkan/vulkan_android.h"
    "src/video/khronos/vulkan/vulkan_beta.h"
    "src/video/khronos/vulkan/vulkan_core.h"
    "src/video/khronos/vulkan/vulkan_directfb.h"
    "src/video/khronos/vulkan/vulkan_fuchsia.h"
    "src/video/khronos/vulkan/vulkan_ggp.h"
    "src/video/khronos/vulkan/vulkan_ios.h"
    "src/video/khronos/vulkan/vulkan_macos.h"
    "src/video/khronos/vulkan/vulkan_metal.h"
    "src/video/khronos/vulkan/vulkan_vi.h"
    "src/video/khronos/vulkan/vulkan_wayland.h"
    "src/video/khronos/vulkan/vulkan_win32.h"
    "src/video/khronos/vulkan/vulkan_xcb.h"
    "src/video/khronos/vulkan/vulkan_xlib.h"
    "src/video/khronos/vulkan/vulkan_xlib_xrandr.h"
)

add_sources( SOURCE_GROUP "Video/Windows"
    "src/video/windows/SDL_msctf.h"
    "src/video/windows/SDL_vkeys.h"
    "src/video/windows/SDL_windowsclipboard.c"
    "src/video/windows/SDL_windowsclipboard.h"
    "src/video/windows/SDL_windowsevents.c"
    "src/video/windows/SDL_windowsevents.h"
    "src/video/windows/SDL_windowsframebuffer.c"
    "src/video/windows/SDL_windowsframebuffer.h"
    "src/video/windows/SDL_windowskeyboard.c"
    "src/video/windows/SDL_windowskeyboard.h"
    "src/video/windows/SDL_windowsmessagebox.c"
    "src/video/windows/SDL_windowsmessagebox.h"
    "src/video/windows/SDL_windowsmodes.c"
    "src/video/windows/SDL_windowsmodes.h"
    "src/video/windows/SDL_windowsmouse.c"
    "src/video/windows/SDL_windowsmouse.h"
    "src/video/windows/SDL_windowsopengl.c"
    "src/video/windows/SDL_windowsopengl.h"
    "src/video/windows/SDL_windowsopengles.c"
    "src/video/windows/SDL_windowsopengles.h"
    "src/video/windows/SDL_windowsshape.c"
    "src/video/windows/SDL_windowsshape.h"
    "src/video/windows/SDL_windowstaskdialog.h"
    "src/video/windows/SDL_windowsvideo.c"
    "src/video/windows/SDL_windowsvideo.h"
    "src/video/windows/SDL_windowsvulkan.c"
    "src/video/windows/SDL_windowsvulkan.h"
    "src/video/windows/SDL_windowswindow.c"
    "src/video/windows/SDL_windowswindow.h"
    "src/video/windows/wmmsg.h"
)

add_sources( SOURCE_GROUP "Video/Yuv2RGB"
    "src/video/yuv2rgb/yuv_rgb.c"
    "src/video/yuv2rgb/yuv_rgb.h"
    "src/video/yuv2rgb/yuv_rgb_sse_func.h"
    "src/video/yuv2rgb/yuv_rgb_std_func.h"
)

end_sources()

target_include_directories( ${PROJECT_NAME} PRIVATE
    "${ENGINE_SOURCE_DIR}/thirdparty/sdl/include/"
)

thirdparty_suppress_warnings()
