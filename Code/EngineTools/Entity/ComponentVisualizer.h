#pragma once
#include "EngineTools/_Module/API.h"
#include "Engine/UpdateContext.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Types/Function.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ToolsContext;
    class EntityComponent;
    class EntityWorld;
    namespace Render { class Viewport; }

    //-------------------------------------------------------------------------
    // Component Visualizer Context
    //-------------------------------------------------------------------------

    struct ComponentVisualizerContext
    {
        ComponentVisualizerContext( UpdateContext const& context, Render::Viewport const* pViewport, EntityWorld* pWorld, TFunction<void( EntityComponent* pComponent )>&& preEditFunc, TFunction<void( EntityComponent* pComponent )>&& postEditFunc )
            : m_context( context )
            , m_pViewport( pViewport )
            , m_pWorld( pWorld )
            , m_preComponentEditFunction( preEditFunc )
            , m_postComponentEditFunction( postEditFunc )
        {}

    public:

        UpdateContext const&                                    m_context;
        
        Render::Viewport const* const                           m_pViewport;

        EntityWorld* const                                      m_pWorld = nullptr;

        // This function is called before any changes to component state start
        TFunction<void( EntityComponent* pComponent )> const    m_preComponentEditFunction;

        // This function is called after any changes to component state complete
        TFunction<void( EntityComponent* pComponent )> const    m_postComponentEditFunction;
    };

    //-------------------------------------------------------------------------
    // Component Visualizer
    //-------------------------------------------------------------------------
    // Provides a toolbar and 3D visualization of a selected component

    class EE_ENGINETOOLS_API ComponentVisualizer : public IReflectedType
    {
        EE_REFLECT_TYPE( ComponentVisualizer );

    public:

        ComponentVisualizer() = default;
        virtual ~ComponentVisualizer() = default;

        // does this visualizer support this component type
        virtual TypeSystem::TypeID GetSupportedType() const = 0;

        // Update the currently visualized component
        virtual void UpdateVisualizedComponent( EntityComponent* pComponent );

        // Get the component we are currently visualizing
        EntityComponent const* GetVisualizedComponent() const { return m_pComponent; }

        // Does this visualizer provide a toolbar
        virtual bool HasToolbar() const { return false; }

        // Draw the toolbar
        virtual void DrawToolbar( ComponentVisualizerContext const& context ) {}

        // Visualize the component in the scene
        virtual void Visualize( ComponentVisualizerContext const& context ) {}

    protected:

        EntityComponent* m_pComponent = nullptr;
    };
}
