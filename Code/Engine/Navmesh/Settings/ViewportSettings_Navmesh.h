#pragma once
#include "Engine/Viewport/ViewportSettings.h"
#include "Engine/_Module/API.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Navmesh
{
    class EE_ENGINE_API NavmeshViewportSettings : public IViewportSettings
    {
        EE_REFLECT_TYPE( NavmeshViewportSettings );

    public:

        virtual char const* GetFriendlyName() const override { return "Navmesh"; }
        virtual void DrawMenu( EntityWorld* pWorld ) override;

        void ShowAreas( EntityWorld* pWorld );

    public:

        EE_REFLECT();
        bool m_drawDebug = false;
    };
}
#endif