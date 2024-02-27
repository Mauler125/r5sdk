// Usage:
//  static ImDrawDataSnapshot snapshot; // Important: make persistent across frames to reuse buffers.
//  snapshot.SnapUsingSwap(ImGui::GetDrawData(), ImGui::GetTime());
//  [...]
//  ImGui_ImplDX11_RenderDrawData(&snapshot.DrawData);
#pragma once

// FIXME: Could store an ID in ImDrawList to make this easier for user.
#include "imgui_internal.h" // ImPool<>, ImHashData

struct ImDrawDataSnapshotEntry
{
    ImDrawList* SrcCopy = NULL;     // Drawlist owned by main context
    ImDrawList* OurCopy = NULL;     // Our copy
    double          LastUsedTime = 0.0;
};

struct ImDrawDataSnapshot
{
    // Members
    ImDrawData                      DrawData;
    ImPool<ImDrawDataSnapshotEntry> Cache;
    float                           MemoryCompactTimer = 20.0f; // Discard unused data after 20 seconds

    // Functions
    ~ImDrawDataSnapshot() { Clear(); }
    void                            Clear();
    void                            SnapUsingSwap(ImDrawData* src, double current_time); // Efficient snapshot by swapping data, meaning "src_list" is unusable.
    //void                          SnapUsingCopy(ImDrawData* src, double current_time); // Deep-copy snapshot

    // Internals
    ImGuiID                         GetDrawListID(ImDrawList* src_list) { return ImHashData(&src_list, sizeof(src_list)); }     // Hash pointer
    ImDrawDataSnapshotEntry* GetOrAddEntry(ImDrawList* src_list) { return Cache.GetOrAddByKey(GetDrawListID(src_list)); }
};
