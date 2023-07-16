#pragma once
#include <compare>
#include <cstdint>
#include <nvrhi/nvrhi.h>
namespace bocchi
{
    enum class RenderGraphRescourceType : uint8_t
    {
        Buffer,
        Texture
    };

    enum class RenderGraphMode : uint8_t
    {
        CopySrc,
        CopyDst,
        IndirectArgs,
        Vertex,
        Index,
        Constant
    };

    struct RenderGraphResourceId
    {
        inline constexpr static uint32_t invalid_id = static_cast<uint32_t>(-1);

        RenderGraphResourceId() : id(invalid_id) {}
        RenderGraphResourceId(RenderGraphResourceId const&);

        explicit RenderGraphResourceId(const size_t id) : id(static_cast<uint32_t>(id)) {}

        void Invalidate() { id = invalid_id; }

        [[nodiscard]] bool IsValid() const { return id != invalid_id; }

        auto operator<=>(const RenderGraphResourceId&) const=default;

        uint32_t id;
    };

    template<RenderGraphRescourceType ResourceType>
    struct TypeRenderGraphResourceId : RenderGraphResourceId
    {
        using RenderGraphResourceId::RenderGraphResourceId;
    };

    using RenderGraphBufferId  = TypeRenderGraphResourceId<RenderGraphRescourceType::Buffer>;
    using RenderGraphTextureId = TypeRenderGraphResourceId<RenderGraphRescourceType::Texture>;

    template<RenderGraphMode Mode>
    struct RenderGraphTextureModeId : RenderGraphTextureId
    {
        using RenderGraphTextureId::RenderGraphTextureId;

    private:
        explicit RenderGraphTextureModeId(RenderGraphTextureId const& id) : RenderGraphTextureId(id) {}
    };

    template<RenderGraphMode Mode>
    struct RenderGraphBufferModeId : RenderGraphBufferId
    {
        using RenderGraphBufferId::RenderGraphBufferId;

    private:
        explicit RenderGraphBufferModeId(RenderGraphBufferId const& id) : RenderGraphBufferId(id) {}
    };

    using RenderTextureCopySrcId = RenderGraphTextureModeId<RenderGraphMode::CopySrc>;
    using RenderTextureCopyDstId = RenderGraphTextureModeId<RenderGraphMode::CopyDst>;

    using RenderBufferCopySrcid      = RenderGraphBufferModeId<RenderGraphMode::CopySrc>;
    using RenderBufferCopyDstId      = RenderGraphBufferModeId<RenderGraphMode::CopyDst>;
    using RenderBufferIndirectArgsId = RenderGraphBufferModeId<RenderGraphMode::IndirectArgs>;
    using RenderBufferVertexId       = RenderGraphBufferModeId<RenderGraphMode::Vertex>;
    using RenderBufferIndexId        = RenderGraphBufferModeId<RenderGraphMode::Index>;
    using RenderbufferConstantId     = RenderGraphBufferModeId<RenderGraphMode::Constant>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    struct RenderGraphResourceDscriptorId
    {
        inline constexpr static uint64_t invalid_id = static_cast<uint64_t>(-1);

        RenderGraphResourceDscriptorId() : id(invalid_id) {}
        RenderGraphResourceDscriptorId(const size_t view_id, const RenderGraphResourceId& resource_handle) :
            id(invalid_id)
        {
            const uint32_t resource_id = resource_handle.id;
            id                         = (view_id << 32) | resource_id;
        }

        [[nodiscard]] size_t GetViewId() const { return id >> 32; }

        [[nodiscard]] size_t GetResourceId() const { return static_cast<uint32_t>(id); }

        RenderGraphResourceId operator*() const { return RenderGraphResourceId(GetResourceId()); }

        void               Invalidate() { id = invalid_id; }
        [[nodiscard]] bool IsValid() const { return id != invalid_id; }
        auto               operator<=>(RenderGraphResourceDscriptorId const&) const = default;

        uint64_t id;
    };

    enum class RenderGraphDscriptorType : uint8_t
    {
        ReadOnly,
        ReadWrite,
        RenderTarget,
        DepthStencil
    };

    template<RenderGraphRescourceType ResourceType, RenderGraphDscriptorType ResourceViewType>
    struct TypedRenderGraphResourceDescritorId : RenderGraphResourceDscriptorId
    {
        using RenderGraphResourceDscriptorId::RenderGraphResourceDscriptorId;
        using RenderGraphResourceDscriptorId::operator*;

        [[nodiscard]] auto GetResourceId() const
        {
            if constexpr (ResourceType == RenderGraphRescourceType::Buffer)
                return RenderGraphBufferId(RenderGraphResourceDscriptorId::GetResourceId());
            else if constexpr (ResourceType == RenderGraphRescourceType::Texture)
                return RenderGraphTextureId(RenderGraphResourceDscriptorId::GetResourceId());
        }

        auto operator*() const
        {
            if constexpr (ResourceType == RenderGraphRescourceType::Buffer)
                return RenderGraphBufferId(RenderGraphResourceDscriptorId::GetResourceId());
            else if constexpr (ResourceType == RenderGraphRescourceType::Texture)
                return RenderGraphTextureId(RenderGraphResourceDscriptorId::GetResourceId());
        }
    };

    using RenderGraphRenderTargetId =
        TypedRenderGraphResourceDescritorId<RenderGraphRescourceType::Texture, RenderGraphDscriptorType::RenderTarget>;
    using RenderGraphDepthStencilId =
        TypedRenderGraphResourceDescritorId<RenderGraphRescourceType::Texture, RenderGraphDscriptorType::DepthStencil>;
    using RenderGraphTextureReadOnlyId =
        TypedRenderGraphResourceDescritorId<RenderGraphRescourceType::Texture, RenderGraphDscriptorType::ReadOnly>;
    using RenderGraphTextureReadWriteId =
        TypedRenderGraphResourceDescritorId<RenderGraphRescourceType::Texture, RenderGraphDscriptorType::ReadWrite>;

    using RenderGraphBufferReadOnlyId =
        TypedRenderGraphResourceDescritorId<RenderGraphRescourceType::Buffer, RenderGraphDscriptorType::ReadOnly>;
    using RenderGraphBufferReadWriteId =
        TypedRenderGraphResourceDescritorId<RenderGraphRescourceType::Buffer, RenderGraphDscriptorType::ReadWrite>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////

    struct RenderGraphAllocationId : RenderGraphResourceId
    {
        using RenderGraphResourceId::RenderGraphResourceId;
    };

} // namespace bocchi

namespace std
{
    template<>
    struct hash<bocchi::RenderGraphTextureId>
    {
        size_t operator()(bocchi::RenderGraphTextureId const& h) const noexcept { return hash<decltype(h.id)>()(h.id); }
    };

    template<>
    struct hash<bocchi::RenderGraphBufferId>
    {
        size_t operator()(bocchi::RenderGraphBufferId const& h) const noexcept { return hash<decltype(h.id)>()(h.id); }
    };

    template<>
    struct hash<bocchi::RenderGraphTextureReadOnlyId>
    {
        size_t operator()(bocchi::RenderGraphTextureReadOnlyId const& h) const noexcept
        {
            return hash<decltype(h.id)>()(h.id);
        }
    };

    template<>
    struct hash<bocchi::RenderGraphTextureReadWriteId>
    {
        size_t operator()(bocchi::RenderGraphTextureReadWriteId const& h) const noexcept
        {
            return hash<decltype(h.id)>()(h.id);
        }
    };

    template<>
    struct hash<bocchi::RenderGraphRenderTargetId>
    {
        size_t operator()(bocchi::RenderGraphRenderTargetId const& h) const noexcept
        {
            return hash<decltype(h.id)>()(h.id);
        }
    };

    template<>
    struct hash<bocchi::RenderGraphDepthStencilId>
    {
        size_t operator()(bocchi::RenderGraphDepthStencilId const& h) const noexcept
        {
            return hash<decltype(h.id)>()(h.id);
        }
    };

    template<>
    struct hash<bocchi::RenderGraphBufferReadOnlyId>
    {
        size_t operator()(bocchi::RenderGraphBufferReadOnlyId const& h) const noexcept
        {
            return hash<decltype(h.id)>()(h.id);
        }
    };

    template<>
    struct hash<bocchi::RenderGraphBufferReadWriteId>
    {
        size_t operator()(bocchi::RenderGraphBufferReadWriteId const& h) const noexcept
        {
            return hash<decltype(h.id)>()(h.id);
        }
    };

} // namespace std
