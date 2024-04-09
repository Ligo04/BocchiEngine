#pragma once
#include <Luna/RHI/RHI.hpp>
#include <Luna/Runtime/Ref.hpp>

namespace Bocchi
{
    using namespace Luna;
    class RHIContent
    {
        private:
            Ref<RHI::IDevice>       m_device;
            Ref<RHI::IAdapter>      m_physics_devices;
            Ref<RHI::ISwapChain>    m_swap_chain;
            Ref<RHI::SwapChainDesc> m_swap_chain_desc;

            struct SwapChainImage
            {
                    Ref<RHI::ITexture>    image;
                    Ref<RHI::TextureDesc> image_view;
            };
            Vector<SwapChainImage>   m_swap_chain_images;
            u32                      m_swap_chain_index = static_cast<u32>(-1);

            u32                      m_graphics_queue;
            u32                      m_compute_queue;
            u32                      m_transfer_queue;

            Ref<RHI::ICommandBuffer> m_command_buffer;
    };
} //namespace Bocchi