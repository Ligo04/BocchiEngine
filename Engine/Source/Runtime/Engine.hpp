#include "Luna/RHI/SwapChain.hpp"
#include "Luna/Runtime/Path.hpp"
#include "Luna/Runtime/Ref.hpp"
#include <Luna/Window/Window.hpp>
namespace Bocchi
{
    using namespace Luna;
    class BocchiEngine
    {
            static const float k_fps_alpha;

        public:
            BocchiEngine() {}
            ~BocchiEngine() {}

        public:
            Path                 m_project_path;
            Ref<Window::IWindow> m_window;
            Ref<RHI::ISwapChain> m_swap_chain;
    };
} // namespace Bocchi