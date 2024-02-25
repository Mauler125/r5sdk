#ifndef IMGUI_SYSTEM_H
#define IMGUI_SYSTEM_H

extern bool ImguiSystem_IsInitialized();

extern bool ImguiSystem_Init();
extern void ImguiSystem_Shutdown();

extern void ImguiSystem_SwapBuffers();

extern void ImguiSystem_SampleFrame();
extern void ImguiSystem_RenderFrame();

#endif // IMGUI_SYSTEM_H
