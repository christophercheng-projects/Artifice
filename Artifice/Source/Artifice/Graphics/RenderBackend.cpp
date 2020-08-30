#include "RenderBackend.h"

#include "Artifice/Core/Log.h"



RenderBackend::RenderBackend()
{

}

void RenderBackend::Init()
{
    m_Instance.Init();

    m_RenderHandleAllocator = new RenderHandleAllocator<AR_HANDLE_RING_SIZE>();
}

void RenderBackend::CleanUp()
{
    for (Device* device : m_Devices)
    {
        device->CleanUp();
    }

    m_Instance.CleanUp();

    delete m_RenderHandleAllocator;
}

DeviceID RenderBackend::CreateDevice()
{
    uint32 id = BIT(m_Devices.size());
    
    Device* device = new Device();
    device->Init(m_Instance.GetHandle(), m_RenderHandleAllocator);
    m_Devices.push_back(device);
    
    return id;
}

void RenderBackend::BeginFrame()
{
    m_RenderHandleAllocator->Advance();
    for (Device* device : m_Devices)
    {
        device->BeginFrame();
    }
}
void RenderBackend::EndFrame()
{
    for (Device* device : m_Devices)
    {
        device->EndFrame();
    }
}

bool RenderBackend::AcquireSwapchains(std::vector<RenderHandle> swapchains, DeviceID device_id)
{
    bool result = true;

    Device* device = GetDevice(device_id);

    std::vector<RenderHandle> swapchains_to_reacquire;
    bool success = false;
    while (!success)
    {
        success = true;
        std::vector<bool> results = device->AcquireSwapchains(swapchains);

        for (uint32 i = 0; i < swapchains.size(); i++)
        {
            if (!results[i])
            {
                device->WaitIdle();
                device->ResizeSwapchain(swapchains[i], 0, 0);
                swapchains_to_reacquire.push_back(swapchains[i]);

                success = false;
                result = false;
            }
        }

        if (!success)
        {
            swapchains = swapchains_to_reacquire;
            swapchains_to_reacquire.clear();
        }
    }

    return result;
}

bool RenderBackend::PresentSwapchains(QueueType queue, std::vector<RenderHandle> swapchains, DeviceID device_id)
{
    bool result = true;

    Device* device = GetDevice(device_id);

    std::vector<bool> results = device->PresentSwapchains(queue, swapchains);

    for (uint32 i = 0; i < swapchains.size(); i++)
    {
        if (!results[i])
        {
            device->WaitIdle();
            device->ResizeSwapchain(swapchains[i], 0, 0);

            return result;
        }
    }

    return result;
}