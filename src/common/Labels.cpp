#include "common/Objects.h"

#include <slag/CommandBuffer.h>

namespace slag {

#ifdef SLAG_DISABLE_LABELS

void CommandBuffer::beginLabel(const char*, const float*) {}
void CommandBuffer::endLabel() {}
void CommandBuffer::insertLabel(const char*, const float*) {}

#else

void CommandBuffer::beginLabel(const char* name, const float* color)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::beginLabel");
    auto* self = static_cast<CommandBufferCommon*>(this);
    SLAG_VALIDATE(self->device->instance, self->state == CommandBufferState::Recording, "CommandBuffer::beginLabel",
                  "not recording");
    SLAG_VALIDATE(self->device->instance, name, "CommandBuffer::beginLabel", "name is null");
    ++self->labelDepth;
    SLAG_DISPATCH(self, cmdBeginLabel)(self, name ? name : "", color);
}

void CommandBuffer::endLabel()
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::endLabel");
    auto* self = static_cast<CommandBufferCommon*>(this);
    SLAG_VALIDATE(self->device->instance, self->state == CommandBufferState::Recording, "CommandBuffer::endLabel",
                  "not recording");
    // A label opened in an earlier list of a split render pass may close here.
    SLAG_VALIDATE(self->device->instance, self->labelDepth > 0 || self->resumesPass, "CommandBuffer::endLabel",
                  "no label is open");
    --self->labelDepth;
    SLAG_DISPATCH(self, cmdEndLabel)(self);
}

void CommandBuffer::insertLabel(const char* name, const float* color)
{
    SLAG_CHECK_LIVE_THIS(CommandBufferCommon, "CommandBuffer", "CommandBuffer::insertLabel");
    auto* self = static_cast<CommandBufferCommon*>(this);
    SLAG_VALIDATE(self->device->instance, self->state == CommandBufferState::Recording, "CommandBuffer::insertLabel",
                  "not recording");
    SLAG_VALIDATE(self->device->instance, name, "CommandBuffer::insertLabel", "name is null");
    SLAG_DISPATCH(self, cmdInsertLabel)(self, name ? name : "", color);
}

#endif

} // namespace slag
