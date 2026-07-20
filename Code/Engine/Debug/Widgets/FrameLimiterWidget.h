#pragma once

#include "Engine/_Module/API.h"
#include "Base/Esoterica.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class UpdateContext;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API FrameLimiterWidget
    {

    public:

        FrameLimiterWidget( float width = -1 );
        void Draw( UpdateContext& context ) const;

    public:

        float m_width = 0;
    };
}
#endif