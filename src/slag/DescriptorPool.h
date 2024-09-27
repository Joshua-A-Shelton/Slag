#ifndef SLAG_DESCRIPTORPOOL_H
#define SLAG_DESCRIPTORPOOL_H

namespace slag
{

    class DescriptorPool
    {
    public:
        virtual ~DescriptorPool()=default;
        virtual void reset()=0;
    };

} // slag

#endif //SLAG_DESCRIPTORPOOL_H
