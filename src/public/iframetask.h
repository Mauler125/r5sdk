#ifndef TIER0_IFRAMETASK_H
#define TIER0_IFRAMETASK_H

struct QueuedTasks_s
{
    unsigned int m_nDelayedFrames;
    std::function<void()> m_rFunctor;

    QueuedTasks_s(unsigned int frames, std::function<void()> functor)
    {
        m_nDelayedFrames = frames;
        m_rFunctor = functor;
    }
};

abstract_class IFrameTask
{
public:
    virtual ~IFrameTask() {}

    virtual void RunFrame() = 0;
    virtual bool IsFinished() const = 0;
};

#endif // TIER0_IFRAMETASK_H
