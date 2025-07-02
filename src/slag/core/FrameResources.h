#ifndef SLAG_FRAMERESOURCES_H
#define SLAG_FRAMERESOURCES_H

namespace slag
{
    ///Additional data that is provided to each frame
    class FrameResources
    {
    public:
        virtual ~FrameResources()=default;
        ///Stall the CPU until the resources are finished being used for the frame
        virtual bool waitForResourcesToFinish()=0;
        ///Are the resources for the frame still in use or not
        virtual bool isFinished()=0;
    };
}
#endif //SLAG_FRAMERESOURCES_H