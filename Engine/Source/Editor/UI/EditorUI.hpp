#pragma once
#include "Editor/UI/WindowUI.hpp"
#include "Luna/Runtime/String.hpp"
#include "Luna/Runtime/UnorderedMap.hpp"

namespace Bocchi
{
    class EditorUI: public WindowUI
    {
        public:
            void Initialize() override;
            void perRender() override;

        private:
            Luna::UnorderedMap<Luna::String, Luna::Function<void(Luna::String, void *)>> m_editor_ui_creator;
            Luna::UnorderedMap<Luna::String, Luna::u32>                                  m_new_object_index_map;

            bool m_editor_menu_window_open{ true };
            bool m_gmal_engine_window_open{ true };
            bool m_file_content_window_open{ true };
            bool m_assert_window_open{ true };
            bool m_detail_window_open{ true };
    };
} //namespace Bocchi