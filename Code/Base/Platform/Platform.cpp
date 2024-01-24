#include "Platform.h"
#include "Base/Esoterica.h"

//-------------------------------------------------------------------------

namespace EE::Platform
{
    namespace Internals
    {
        void* g_pMainWindowHandle = nullptr;
    }

    // Windowing
    //-------------------------------------------------------------------------

    void* GetMainWindowHandle()
    {
        return Internals::g_pMainWindowHandle;
    }

    void SetMainWindowHandle( void* pWindowHandle )
    {
        EE_ASSERT( Internals::g_pMainWindowHandle == nullptr );
        Internals::g_pMainWindowHandle = pWindowHandle;
    }

    void ClearMainWindowHandle()
    {
        EE_ASSERT( Internals::g_pMainWindowHandle != nullptr );
        Internals::g_pMainWindowHandle = nullptr;
    }
}