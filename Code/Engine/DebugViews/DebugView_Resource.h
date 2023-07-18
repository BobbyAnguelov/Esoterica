#pragma once

#include "DebugView.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Resource
{
    class ResourceSystem;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ResourceDebugView : public DebugView
    {
        EE_REFLECT_TYPE( ResourceDebugView );

    public:

        static void DrawRequestHistory( ResourceSystem* pResourceSystem );
        static void DrawResourceSystemOverview( ResourceSystem* pResourceSystem );

    public:

        ResourceDebugView() : DebugView( "System/Resource" ) {}

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawMenu( EntityWorldUpdateContext const& context ) override;

    private:

        ResourceSystem*         m_pResourceSystem = nullptr;
    };
}
#endif