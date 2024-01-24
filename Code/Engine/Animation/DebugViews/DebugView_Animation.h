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
    struct SampledEventDebugPath;

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
        static void DrawGraphActiveTasksDebugView( GraphInstance* pGraphInstance );
        static void DrawGraphActiveTasksDebugView( TaskSystem* pTaskSystem );
        static void DrawRootMotionDebugView( GraphInstance* pGraphInstance );
        static void DrawSampledAnimationEventsView( GraphInstance* pGraphInstance, TFunction<void( SampledEventDebugPath const& )> const& navigateToNodeFunc = TFunction<void( SampledEventDebugPath const& )>() );
        static void DrawSampledStateEventsView( GraphInstance* pGraphInstance, TFunction<void( SampledEventDebugPath const&)> const& navigateToNodeFunc = TFunction<void( SampledEventDebugPath const& )>() );
        static void DrawCombinedSampledEventsView( GraphInstance* pGraphInstance, TFunction<void( SampledEventDebugPath const& )> const& navigateToNodeFunc = TFunction<void( SampledEventDebugPath const& )>() );

    private:

        static void DrawTaskTreeRow( TaskSystem* pTaskSystem, TaskIndex currentTaskIdx );
        static void DrawRootMotionRow( GraphInstance* pGraphInstance, RootMotionDebugger const* pRootMotionRecorder, int16_t currentActionIdx );

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