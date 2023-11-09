#pragma once

#include <memory>
#include <nvrhi/nvrhi.h>

namespace bocchi
{
    class RenderGraphResourcePool
    {
        struct PooledTexture
        {
            std::unique_ptr<nvrhi::ITexture> texture;
            uint64_t                         last_used_frame;
        };

        struct PooledBuffer
        {
            std::unique_ptr<nvrhi::IBuffer> buffer;
            uint64_t                        last_used_frame;
        };

    public:
        explicit RenderGraphResourcePool(nvrhi::DeviceHandle device) : m_device_handle_(std::move(device)) {}

        void Tick()
        {
            for (size_t i = 0; i < m_texture_pool_.size();)
            {
                auto& [texture, last_used_frame] = m_texture_pool_[i].first;
                if (bool activate = m_texture_pool_[i].second; !activate && last_used_frame + 4 < m_frame_index_)
                {
                    std::swap(m_texture_pool_[i], m_texture_pool_.back());
                    m_texture_pool_.pop_back();
                }
                else
                {
                    ++i;
                }
            }
            ++m_frame_index_;
        }

        nvrhi::ITexture* AllocateTexture(nvrhi::TextureDesc const& texture_desc)
        {
            for (auto& [pool_texture, actiavte] : m_texture_pool_)
            {
                if (!actiavte && pool_texture.texture->getDesc().IsCompatible(texture_desc))
                {
                    pool_texture.last_used_frame = m_frame_index_;
                    actiavte                     = true;
                    return pool_texture.texture.get();
                }
            }
            auto& texture =
                m_texture_pool_
                    .emplace_back(std::pair {PooledTexture {std::make_unique<nvrhi::ITexture>(), m_frame_index_}, true})
                    .first.texture;
            return texture.get();
        }

        void ReleaseTexture(const nvrhi::ITexture* texture)
        {
            for (auto& [pooled_texture, activate] : m_texture_pool_)
            {
                auto& texture_ptr = pooled_texture.texture;
                if (activate && texture_ptr.get() == texture)
                {
                    activate = false;
                }
            }
        }

        nvrhi::IBuffer* AllocateBuffer(nvrhi::BufferDesc const& buffer_desc)
        {
            for (auto& [pooled_buffer, actiavte] : m_buffer_pool_)
            {
                if (!actiavte && pooled_buffer.buffer->getDesc() == buffer_desc)
                {
                    pooled_buffer.last_used_frame = m_frame_index_;
                    actiavte                      = true;
                    return pooled_buffer.buffer.get();
                }
            }
            auto& buffer =
                m_buffer_pool_
                    .emplace_back(std::pair {PooledBuffer {std::make_unique<nvrhi::IBuffer>(), m_frame_index_}, true})
                    .first.buffer;
            return buffer.get();
        }

        void ReleaseTexture(const nvrhi::IBuffer* buffer)
        {
            for (auto& [pooled_buffer, activate] : m_buffer_pool_)
            {
                auto& buffer_ptr = pooled_buffer.buffer;
                if (activate && buffer_ptr.get() == buffer)
                {
                    activate = false;
                }
            }
        }
        nvrhi::DeviceHandle GetDevice() const { return m_device_handle_; }

    private:
        nvrhi::DeviceHandle                         m_device_handle_ = nullptr;
        uint64_t                                    m_frame_index_   = 0;
        std::vector<std::pair<PooledTexture, bool>> m_texture_pool_;
        std::vector<std::pair<PooledBuffer, bool>>  m_buffer_pool_;
        
    };
} // namespace bocchi
