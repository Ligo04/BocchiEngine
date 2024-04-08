#include "Editor/UI/Window_ui.hpp"
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
    };
} //namespace Bocchi