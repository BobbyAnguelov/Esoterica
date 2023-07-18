#pragma once

//#include "Engine/Entity/EntityWorldDebugView.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    //class EntityWorldManager;
    //class Entity;
    //class EntityComponent;
    //class SpatialEntityComponent;

    ////-------------------------------------------------------------------------

    //class EntityDebugView : public DebugView
    //{
    //    EE_REFLECT_TYPE( EntityDebugView );

    //public:

    //    EntityDebugView();

    //private:

    //    virtual void DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass ) override;

    //    void DrawMenu( EntityWorldUpdateContext const& context );
    //    void DrawWorldBrowser( EntityWorldUpdateContext const& context );
    //    void DrawMapLoader( EntityWorldUpdateContext const& context );

    //    void DrawComponentEntry( EntityComponent const* pComponent );
    //    void DrawSpatialComponentTree( SpatialEntityComponent const* pComponent );

    //private:

    //    bool                    m_isWorldBrowserOpen = false;
    //    bool                    m_isMapLoaderOpen = false;

    //    // Browser Data
    //    TVector<Entity*>        m_entities;
    //    Entity*                 m_pSelectedEntity = nullptr;
    //};
}
#endif