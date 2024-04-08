
namespace Bocchi
{

    class WindowUI
    {
        public:
            virtual void Initialize() = 0;
            virtual void perRender()  = 0;
    };
} //namespace Bocchi