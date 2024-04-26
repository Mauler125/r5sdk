#ifndef TIER0_FRAMETASK_H
#define TIER0_FRAMETASK_H

#include "public/iframetask.h"

//=============================================================================//
// This class is set up to run before each frame, committed tasks are scheduled
// to execute after 'i' frames.
// ----------------------------------------------------------------------------
// A use case for scheduling tasks in the main thread would be (for example)
// performing a web request in a separate thread, and apply the results (such as
// server lists in the browser) onto the imgui panels which are created/drawn in
// the main thread
//=============================================================================//
class CFrameTask : public IFrameTask
{
public:
    virtual ~CFrameTask() {}
    virtual void RunFrame();
    virtual bool IsFinished() const;

    void Dispatch(std::function<void()> functor, unsigned int frames);

private:
    mutable std::mutex m_Mutex;
    std::list<QueuedTasks_s> m_QueuedTasks;
};

extern std::list<IFrameTask*> g_TaskQueueList;
extern CFrameTask g_TaskQueue;

#endif // TIER0_FRAMETASK_H
