#ifndef TIER0_FRAMETASK_H
#define TIER0_FRAMETASK_H

#include "public/iframetask.h"

class CFrameTask : public IFrameTask
{
public:
    virtual ~CFrameTask() {}
    virtual void RunFrame();
    virtual bool IsFinished() const;

    void AddFunc(std::function<void()> functor, int frames);

private:
    std::mutex m_Mutex;
    std::list<DelayedCall_s> m_DelayedCalls;
};

extern std::list<IFrameTask*> g_FrameTasks;
extern CFrameTask* g_DelayedCallTask;

#endif // TIER0_FRAMETASK_H
