#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Entity/EntityWorldDebugView.h"
#include "Engine/Entity/EntityIDs.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Animation
{
    class AnimationWorldSystem;
    class AnimationGraphComponent;
    class TaskSystem;
    class RootMotionDebugger;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API AnimationDebugView : public EntityWorldDebugView
    {
        EE_REGISTER_TYPE( AnimationDebugView );

        struct ComponentDebugState
        {
            ComponentDebugState( ComponentID ID )
                : m_ID( ID )
            {}

            ComponentID             m_ID;
            bool                    m_drawControlParameters = false;
            bool                    m_drawActiveTasks = false;
            bool                    m_drawSampledEvents = false;
        };

    public:

        static void DrawGraphControlParameters( AnimationGraphComponent* pGraphComponent );
        static void DrawGraphActiveTasksDebugView( AnimationGraphComponent* pGraphComponent );
        static void DrawGraphSampledEventsView( AnimationGraphComponent* pGraphComponent );

    private:

        static void DrawTaskTreeRow( AnimationGraphComponent* pGraphComponent, TaskSystem* pTaskSystem, TaskIndex currentTaskIdx );
        static void DrawRootMotionRow( AnimationGraphComponent* pGraphComponent, RootMotionDebugger const* pRootMotionRecorder, int16_t currentActionIdx );

    public:

        AnimationDebugView();

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass ) override;
        void DrawMenu( EntityWorldUpdateContext const& context );

        ComponentDebugState* GetDebugState( ComponentID ID );
        void DestroyDebugState( ComponentID ID );

    private:

        EntityWorld const*                      m_pWorld = nullptr;
        AnimationWorldSystem*                   m_pAnimationWorldSystem = nullptr;
        TVector<ComponentDebugState>         m_componentRuntimeSettings;
    };
}
#endif