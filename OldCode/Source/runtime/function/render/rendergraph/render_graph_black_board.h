#pragma once
#include <type_traits>
#include <typeindex>
#include "runtime/core/base/macro.h"

namespace bocchi
{
    class RenderGraphBlackBoard
    {
    public:
        RenderGraphBlackBoard()  = default;
        ~RenderGraphBlackBoard() = default;

        // default 
        RenderGraphBlackBoard(RenderGraphBlackBoard const&)            = delete;
        RenderGraphBlackBoard& operator=(RenderGraphBlackBoard const&) = delete;

        template<typename T>
        T& Add(T&& data)
        {
            return Create<std::remove_cvref_t<T>>(std::forward<T>(data));
        }

        template<typename T, typename... Args>
        requires std::is_constructible_v<T, Args...> T& Create(Args&&... args)
        {
            // check the type is trivial and standard layout
            static_assert(std::is_trivial_v<T> && std::is_standard_layout_v<T>);
            ASSERT(m_board_data_.find(typeid(T) == m_board_data_.end()) &&
                   "Cannot create same type more than once in blackboard!");
            m_board_data_[typeid(T)] = std::make_unique<uint8_t[]>(sizeof(T));
            T* data_entry            = reinterpret_cast<T*>(m_board_data_[typeid(T)].get());
            *data_entry              = T {std::forward<Args>(args)...};
            return *data_entry;
        }

        template<typename T>
        T const* Get() const
        {
            if (auto it = m_board_data_.find(typeid(T)); it != m_board_data_.end())
            {
                return reinterpret_cast<T const>(it->second.get());
            }
            return nullptr;
        }

        template<typename T>
        T const& GetChecked() const
        {
            T const* p_data = Get<T>();
            ASSERT(p_data != nullptr);
            return *p_data;
        }

        void Clear() { m_board_data_.clear(); }

    private:
        std::unordered_map<std::type_index, std::unique_ptr<uint8_t[]>> m_board_data_;
    };
} // namespace bocchi
