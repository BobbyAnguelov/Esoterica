#pragma once
#include "Engine/Debug/DebugView.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class EntityWorldManager;
    class Entity;
    class EntityComponent;
    class SpatialEntityComponent;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API EntityWorldTimeControlsWidget
    {
    public:

        EntityWorldTimeControlsWidget( EntityWorld* pWorld );

        void Draw( float width = -1 );

    private:

        EntityWorld* m_pWorld = nullptr;
    };

    //-------------------------------------------------------------------------

    class EntityDebugView : public DebugView
    {
        EE_REFLECT_TYPE( EntityDebugView );

    public:

        virtual Category GetCategory() const override { return Category::Engine; }
        virtual char const* GetMenuPath() const override { return "Entity"; }

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawMenu( EntityWorldUpdateContext const& context ) override;

        void DrawWorldBrowser( EntityWorldUpdateContext const& context );
        void DrawMapLoader( EntityWorldUpdateContext const& context );

        void DrawComponentEntry( EntityComponent const* pComponent );
        void DrawSpatialComponentTree( SpatialEntityComponent const* pComponent );

    private:

        // Browser Data
        TVector<Entity*>        m_entities;
        Entity*                 m_pSelectedEntity = nullptr;
    };
}
#endif