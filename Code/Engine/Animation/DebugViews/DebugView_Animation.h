#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/DebugViews/DebugView.h"
#include "Engine/Entity/EntityIDs.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Animation
{
    class AnimationWorldSystem;
    class GraphInstance;
    class TaskSystem;
    class RootMotionDebugger;
    struct DebugPath;

    //-------------------------------------------------------------------------

    using NavigateToSourceFunc = TFunction<void( DebugPath const& )>;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API AnimationDebugView : public DebugView
    {
        EE_REFLECT_TYPE( AnimationDebugView );

        struct ComponentDebugState
        {
            ComponentDebugState( ComponentID ID )
                : m_ID( ID )
            {}

            ComponentID                         m_ID;
            bool                                m_drawControlParameters = false;
            bool                                m_drawActiveTasks = false;
            bool                                m_drawSampledEvents = false;
        };

    public:

        static void DrawGraphControlParameters( GraphInstance* pGraphInstance );
        static void DrawGraphActiveTasksDebugView( GraphInstance* pGraphInstance, DebugPath const& filterPath = DebugPath(), NavigateToSourceFunc const& navigateToNodeFunc = NavigateToSourceFunc());
        static void DrawRootMotionDebugView( GraphInstance* pGraphInstance, DebugPath const& filterPath = DebugPath(), NavigateToSourceFunc const& navigateToNodeFunc = NavigateToSourceFunc() );

        static void DrawSampledAnimationEventsView( GraphInstance* pGraphInstance, DebugPath const& filterPath = DebugPath(), NavigateToSourceFunc const& navigateToNodeFunc = NavigateToSourceFunc() );
        static void DrawSampledStateEventsView( GraphInstance* pGraphInstance, DebugPath const& filterPath = DebugPath(), NavigateToSourceFunc const& navigateToNodeFunc = NavigateToSourceFunc() );
        static void DrawCombinedSampledEventsView( GraphInstance* pGraphInstance, DebugPath const& filterPath = DebugPath(), NavigateToSourceFunc const& navigateToNodeFunc = NavigateToSourceFunc() );

    private:

        static void DrawRootMotionRow( GraphInstance* pGraphInstance, DebugPath const& filterPath, RootMotionDebugger const* pRootMotionRecorder, int16_t currentActionIdx, NavigateToSourceFunc const& navigateToNodeFunc );

    public:

        AnimationDebugView() : DebugView( "Engine/Animation" ) {}

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawMenu( EntityWorldUpdateContext const& context ) override;
        virtual void Update( EntityWorldUpdateContext const& context ) override;

        ComponentDebugState* GetDebugState( ComponentID ID );
        void DestroyDebugState( ComponentID ID );

        void DrawControlParameterWindow( EntityWorldUpdateContext const& context, bool isFocused, uint64_t userData );
        void DrawTasksWindow( EntityWorldUpdateContext const& context, bool isFocused, uint64_t userData );
        void DrawEventsWindow( EntityWorldUpdateContext const& context, bool isFocused, uint64_t userData );

    private:

        AnimationWorldSystem*                   m_pAnimationWorldSystem = nullptr;
        TVector<ComponentDebugState>            m_componentRuntimeSettings;
    };
}
#endif