#pragma once
#include <nvrhi/nvrhi.h>
#include <optional>
#include <unordered_set>

#include "runtime/core/utils/enum_utils.h"
#include "runtime/function/render/rendergraph/render_graph_context.h"

namespace bocchi
{

    enum class RenderGraphPassType : uint8_t
    {
        Graphics,
        Compute,
        ComputAsync,
        Copy
    };

    // TODO: how to set flags
    enum class RenderPassFlags : uint32_t
    {
        None                    = 0x00,
        ForceNocull             = 0x01, // RGPass cannot be culled by render graph
        AllowUauWrite           = 0x02, // Allow uav write,only makes sense if LegacyRenderPassEnabled is disabled
        SkipAutoRenderPass      = 0x04, // RGpass will manages render targets by himself
        LegacyRenderPass        = 0x08, //
        ActAsCreatorWhenWriting = 0x10, // RGPass will be treated as creator when writing to a resource
    };
    DEFINE_ENUM_BIT_OPERATORS(RenderPassFlags);

    enum RenderGarphReadAccess : uint8_t
    {
        ReadAccessPixelShader,
        ReadAccessNonPixelShader,
        ReadAccessAllshader
    };

    enum class RenderGarphLoadAccessOp : uint8_t
    {
        Discard,
        Preserve,
        Clear,
        NoAccess
    };

    enum class RenderGraphStoreAccessOp : uint8_t
    {
        Discard,
        Preserve,
        Resolve,
        NoAccess
    };

    constexpr uint8_t CombineAccessOps(RenderGarphLoadAccessOp load_op, RenderGraphStoreAccessOp store_op)
    {
        return static_cast<uint8_t>(load_op) << 2 | static_cast<uint8_t>(store_op);
    }

    enum class RenderGraphLoadStoreAccessOp : uint8_t
    {
        DiscordWithDiscord   = CombineAccessOps(RenderGarphLoadAccessOp::Discard, RenderGraphStoreAccessOp::Discard),
        DiscordWithPreserve  = CombineAccessOps(RenderGarphLoadAccessOp::Discard, RenderGraphStoreAccessOp::Preserve),
        ClearWithPreserve    = CombineAccessOps(RenderGarphLoadAccessOp::Clear, RenderGraphStoreAccessOp::Preserve),
        PreserveWithPerserve = CombineAccessOps(RenderGarphLoadAccessOp::Preserve, RenderGraphStoreAccessOp::Preserve),
        ClearWithResolve     = CombineAccessOps(RenderGarphLoadAccessOp::Clear, RenderGraphStoreAccessOp::Resolve),
        PreserveWithResolve  = CombineAccessOps(RenderGarphLoadAccessOp::Preserve, RenderGraphStoreAccessOp::Resolve),
        DiscordWithResolve   = CombineAccessOps(RenderGarphLoadAccessOp::Discard, RenderGraphStoreAccessOp::Resolve),
        NoAccessWithNoAccess = CombineAccessOps(RenderGarphLoadAccessOp::NoAccess, RenderGraphStoreAccessOp::NoAccess),
    };

    constexpr void SplitAccessOp(RenderGraphLoadStoreAccessOp load_store_op,
                                 RenderGarphLoadAccessOp&     load_op,
                                 RenderGraphStoreAccessOp&    store_op)
    {
        store_op = static_cast<RenderGraphStoreAccessOp>(static_cast<uint8_t>(load_store_op) & 0b11);
        load_op  = static_cast<RenderGarphLoadAccessOp>(static_cast<uint8_t>(load_store_op) >> 2 & 0b11);
    }

    class RenderGraph;
    class RenderGraphBuilder;

    class RenderGraphPassBase
    {
        friend RenderGraph;
        friend RenderGraphBuilder;

        struct RendeeTargetInfo
        {
            RenderGraphRenderTargetId    render_target_handle;
            RenderGraphLoadStoreAccessOp render_target_access;
        };

        struct DepthStencilInfo
        {
            RenderGraphDepthStencilId    depth_stencil_handle;
            RenderGraphLoadStoreAccessOp depth_access;
            RenderGraphLoadStoreAccessOp stencil_access;
            bool                         read_only;
        };

    public:
        explicit RenderGraphPassBase(const char*         name,
                                     RenderGraphPassType pass_type = RenderGraphPassType::Graphics,
                                     RenderPassFlags     pass_flag = RenderPassFlags::None) :
            m_name_(name),
            m_pass_type_(pass_type), m_pass_flags_(pass_flag)
        {}
        virtual ~RenderGraphPassBase() = default;

    protected:
        virtual void Setup(RenderGraphBuilder&)                                    = 0;
        virtual void Execute(RenderGraphContext&, nvrhi::CommandListHandle) const = 0;

        bool IsCulled() const { return CanBeCulled() && m_ref_count_ == 0; }
        bool CanBeCulled() const { return !HasAnyFlag(m_pass_flags_, RenderPassFlags::ForceNocull); };
        bool SkipAutoRenderPassSetup() const
        {
            return !HasAnyFlag(m_pass_flags_, RenderPassFlags::SkipAutoRenderPass);
        };
        bool UseLegacyRenderPasses() const { return !HasAnyFlag(m_pass_flags_, RenderPassFlags::LegacyRenderPass); };
        bool AllowUavWrites() const { return !HasAnyFlag(m_pass_flags_, RenderPassFlags::AllowUauWrite); };
        bool ActAsCreatorWhenWriting() const
        {
            return !HasAnyFlag(m_pass_flags_, RenderPassFlags::ActAsCreatorWhenWriting);
        };

    private:
        const std::string   m_name_;
        size_t              m_ref_count_ = 0ull;
        RenderGraphPassType m_pass_type_;
        RenderPassFlags     m_pass_flags_ = RenderPassFlags::None;

        std::unordered_set<RenderGraphTextureId>                        m_texture_creates_;
        std::unordered_set<RenderGraphTextureId>                        m_texture_reads_;
        std::unordered_set<RenderGraphTextureId>                        m_texture_writes_;
        std::unordered_set<RenderGraphTextureId>                        m_texture_destroys_;
        std::unordered_map<RenderGraphTextureId, nvrhi::ResourceStates> m_texture_state_maps_;

        std::unordered_set<RenderGraphBufferId>                        m_buffer_creates_;
        std::unordered_set<RenderGraphBufferId>                        m_buffer_reads_;
        std::unordered_set<RenderGraphBufferId>                        m_buffer_writes_;
        std::unordered_set<RenderGraphBufferId>                        m_buffer_destroys_;
        std::unordered_map<RenderGraphBufferId, nvrhi::ResourceStates> m_buffer_state_maps_;

        std::vector<RenderGraphRenderTargetId> m_render_target_info_;
        std::optional<DepthStencilInfo>        m_depth_stencil_info_ = std::nullopt;
        uint32_t                               m_viewport_width_ = 0, m_viewport_height_ = 0;
    };

    template<typename PassData>
    class RenderGraphPass final : public RenderGraphPassBase
    {
    public:
        
        using SetupFunc   = std::function<void(PassData&, RenderGraphBuilder&)>;
        using ExecuteFunc = std::function<void(PassData const&, RenderGraphContext&, nvrhi::CommandListHandle)>;

        RenderGraphPass(const char*         name,
                        SetupFunc&&         setup,
                        ExecuteFunc&&       execute,
                        RenderGraphPassType type  = RenderGraphPassType::Graphics,
                        RenderPassFlags     flags = RenderPassFlags::None) :
            RenderGraphPassBase(name, type, flags),
            m_setup_func_(std::move(setup)), m_execute_func_(std::move(execute))
        {}

    private:
        PassData    m_data_;
        SetupFunc   m_setup_func_;
        ExecuteFunc m_execute_func_;

        void Setup(RenderGraphBuilder& builder) override
        {
            ASSERT(m_setup_func_ != nullptr && "setup fuinction is null!");
            m_setup_func_(m_data_, builder);
        }

        void Execute(RenderGraphContext& context, nvrhi::CommandListHandle command_list) const override
        {
            ASSERT(m_setup_func_ != nullptr && "execute funtion is null");
            m_execute_func_(m_data_, context, command_list);
        }
    };

    template<>
    class RenderGraphPass<void> final : public RenderGraphPassBase
    {
    public:
        using SetupFunc   = std::function<void(RenderGraphBuilder&)>;
        using ExecuteFunc = std::function<void(RenderGraphContext&, nvrhi::CommandListHandle)>;

        RenderGraphPass(const char*         name,
                        SetupFunc&&         setup,
                        ExecuteFunc&&       execute,
                        RenderGraphPassType type  = RenderGraphPassType::Graphics,
                        RenderPassFlags     flags = RenderPassFlags::None) :
            RenderGraphPassBase(name, type, flags),
            m_setup_func_(std::move(setup)), m_execute_func_(std::move(execute))
        {}

    private:
        SetupFunc   m_setup_func_;
        ExecuteFunc m_execute_func_;

        void Setup(RenderGraphBuilder& builder) override
        {
            ASSERT(m_setup_func_ != nullptr && "setup fuinction is null!");
            m_setup_func_(builder);
        }

        void Execute(RenderGraphContext& context, nvrhi::CommandListHandle command_list) const override
        {
            ASSERT(m_setup_func_ != nullptr && "execute funtion is null");
            m_execute_func_(context, command_list);
        }
    };
} // namespace bocchi
