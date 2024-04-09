#pragma once
#include "Editor/UI/EditorUI.hpp"
#include "Luna/Runtime/UniquePtr.hpp"
#include "Runtime/Engine.hpp"

namespace Bocchi
{
    class BocchiEngine;
    class BocchiEditor
    {
        public:
        protected:
            Luna::UniquePtr<EditorUI>     m_editor_ui;
            Luna::UniquePtr<BocchiEngine> m_engine;
    };
} //namespace Bocchi