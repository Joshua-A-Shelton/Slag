#include "d3d12/D3D12Backend.h"

#include <cstdarg>
#include <cstdio>
#include <cwchar>

// Crash diagnostics through DRED (ADR 0014): auto-breadcrumbs, breadcrumb
// contexts (label text), and page-fault reporting.

namespace slag::d3d12 {

namespace {

// Appends to a fixed buffer, truncating.
struct ReportWriter
{
    char* text;
    size_t size;
    size_t used = 0;

    void add(const char* format, ...)
    {
        if (used + 1 >= size)
        {
            return;
        }
        va_list args;
        va_start(args, format);
        int written = std::vsnprintf(text + used, size - used, format, args);
        va_end(args);
        if (written > 0)
        {
            used += static_cast<size_t>(written) < size - used ? static_cast<size_t>(written) : size - used - 1;
        }
    }
};

const char* opName(D3D12_AUTO_BREADCRUMB_OP op)
{
    switch (op)
    {
    case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:
        return "SetMarker";
    case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:
        return "BeginEvent";
    case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:
        return "EndEvent";
    case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:
        return "DrawInstanced";
    case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:
        return "DrawIndexedInstanced";
    case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:
        return "ExecuteIndirect";
    case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:
        return "Dispatch";
    case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:
        return "CopyBufferRegion";
    case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:
        return "CopyTextureRegion";
    case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:
        return "CopyResource";
    case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE:
        return "ResolveSubresource";
    case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:
        return "ResourceBarrier";
    case D3D12_AUTO_BREADCRUMB_OP_PRESENT:
        return "Present";
    case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA:
        return "ResolveQueryData";
    case D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION:
        return "BeginSubmission";
    case D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION:
        return "EndSubmission";
    case D3D12_AUTO_BREADCRUMB_OP_BEGIN_COMMAND_LIST:
        return "BeginCommandList";
    case D3D12_AUTO_BREADCRUMB_OP_BARRIER:
        return "Barrier";
    default:
        return nullptr;
    }
}

void addName(ReportWriter& writer, const char* narrow, const wchar_t* wide)
{
    if (narrow)
    {
        writer.add("\"%s\"", narrow);
    }
    else if (wide)
    {
        char utf8[limits::maxDebugNameLength * 3 + 1];
        toUtf8(wide, utf8, sizeof(utf8));
        writer.add("\"%s\"", utf8);
    }
    else
    {
        writer.add("(unnamed)");
    }
}

const char* allocationTypeName(D3D12_DRED_ALLOCATION_TYPE type)
{
    switch (type)
    {
    case D3D12_DRED_ALLOCATION_TYPE_RESOURCE:
        return "resource";
    case D3D12_DRED_ALLOCATION_TYPE_HEAP:
        return "heap";
    case D3D12_DRED_ALLOCATION_TYPE_COMMAND_LIST:
        return "command list";
    case D3D12_DRED_ALLOCATION_TYPE_PIPELINE_STATE:
        return "pipeline";
    case D3D12_DRED_ALLOCATION_TYPE_DESCRIPTOR_HEAP:
        return "descriptor heap";
    default:
        return "object";
    }
}

void addAllocations(ReportWriter& writer, const char* title, const D3D12_DRED_ALLOCATION_NODE1* node)
{
    writer.add("\n  %s:", title);
    if (!node)
    {
        writer.add(" none");
    }
    for (int shown = 0; node && shown < 16; node = node->pNext, ++shown)
    {
        writer.add("\n    %s ", allocationTypeName(node->AllocationType));
        addName(writer, node->ObjectNameA, node->ObjectNameW);
    }
}

} // namespace

// Process-wide, so applied before any device is created (ADR 0014).
void enableDred(const MessageSink& sink)
{
    ID3D12DeviceRemovedExtendedDataSettings1* settings = nullptr;
    if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&settings))))
    {
        logMessage(sink, MessageSeverity::Warning, MessageSource::Slag,
                   "slag: createInstance: DRED is not available; crash diagnostics are off");
        return;
    }
    settings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    settings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    settings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    release(settings);
}

bool describeDeviceLoss(GraphicsDeviceCommon* common, char* text, size_t size)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    ID3D12DeviceRemovedExtendedData2* dred = nullptr;
    if (!device.device || FAILED(device.device->QueryInterface(IID_PPV_ARGS(&dred))))
    {
        return false;
    }
    ReportWriter writer{text, size};
    text[0] = '\0';
    writer.add("slag: crash report for \"%s\" (DRED): removed reason 0x%08lx", device.info.name,
               static_cast<unsigned long>(device.device->GetDeviceRemovedReason()));
    switch (dred->GetDeviceState())
    {
    case D3D12_DRED_DEVICE_STATE_HUNG:
        writer.add(", the GPU hung");
        break;
    case D3D12_DRED_DEVICE_STATE_FAULT:
        writer.add(", the GPU faulted");
        break;
    case D3D12_DRED_DEVICE_STATE_PAGEFAULT:
        writer.add(", the GPU page-faulted");
        break;
    default:
        break;
    }

    // Breadcrumbs: the command lists DRED tracked, with the operations around
    // where each stopped.
    D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 breadcrumbs = {};
    if (SUCCEEDED(dred->GetAutoBreadcrumbsOutput1(&breadcrumbs)))
    {
        writer.add("\n  command lists:");
        int lists = 0;
        for (const D3D12_AUTO_BREADCRUMB_NODE1* node = breadcrumbs.pHeadAutoBreadcrumbNode; node && lists < 8;
             node = node->pNext)
        {
            UINT done = node->pLastBreadcrumbValue ? *node->pLastBreadcrumbValue : 0;
            ++lists;
            writer.add("\n    command list ");
            addName(writer, node->pCommandListDebugNameA, node->pCommandListDebugNameW);
            writer.add(" on queue ");
            addName(writer, node->pCommandQueueDebugNameA, node->pCommandQueueDebugNameW);
            bool finished = done >= node->BreadcrumbCount;
            if (finished)
            {
                // A fault can surface after the GPU logged the work as done.
                writer.add(": all %u operations done; the last ones:", node->BreadcrumbCount);
                done = node->BreadcrumbCount;
            }
            else
            {
                writer.add(": %u of %u operations done", done, node->BreadcrumbCount);
            }
            // The operations around the failure, and the labels recorded before it.
            UINT first = done > 6 ? done - 6 : 0;
            UINT last = done + 4 < node->BreadcrumbCount ? done + 4 : node->BreadcrumbCount;
            for (UINT i = first; i < last; ++i)
            {
                const char* name = opName(node->pCommandHistory[i]);
                writer.add("\n      %s %u: ", i == done ? "->" : "  ", i);
                if (name)
                {
                    writer.add("%s", name);
                }
                else
                {
                    writer.add("operation %d", static_cast<int>(node->pCommandHistory[i]));
                }
                for (UINT c = 0; c < node->BreadcrumbContextsCount; ++c)
                {
                    if (node->pBreadcrumbContexts[c].BreadcrumbIndex == i &&
                        node->pBreadcrumbContexts[c].pContextString)
                    {
                        writer.add(" ");
                        addName(writer, nullptr, node->pBreadcrumbContexts[c].pContextString);
                    }
                }
            }
        }
        if (lists == 0)
        {
            writer.add(" none");
        }
    }

    D3D12_DRED_PAGE_FAULT_OUTPUT2 pageFault = {};
    if (SUCCEEDED(dred->GetPageFaultAllocationOutput2(&pageFault)) && pageFault.PageFaultVA)
    {
        writer.add("\n  page fault at GPU address 0x%llx", static_cast<unsigned long long>(pageFault.PageFaultVA));
        addAllocations(writer, "objects at that address", pageFault.pHeadExistingAllocationNode);
        addAllocations(writer, "recently freed objects there", pageFault.pHeadRecentFreedAllocationNode);
    }
    else
    {
        writer.add("\n  no page fault reported");
    }
    release(dred);
    return true;
}

} // namespace slag::d3d12
