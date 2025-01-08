#ifndef SLAG_FRAMERESOURCES_H
#define SLAG_FRAMERESOURCES_H

namespace slag
{

    class FrameResources
    {
    public:
        virtual ~FrameResources()=default;
        virtual void waitForResourcesToFinish()=0;
        virtual bool isFinished()=0;
    };

} // slag

#endif //SLAG_FRAMERESOURCES_H
