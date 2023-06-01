#pragma once

#include "Engine/Entity/EntityWorldDebugView.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Recording.h"
#include "Engine/Animation/AnimationPose.h"

//-------------------------------------------------------------------------

namespace EE { class PlayerManager; }

namespace EE::Animation
{
    class GraphComponent;
    class GraphInstance;
    class TaskSystem;
}

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Player
{
    class PlayerController;

    //-------------------------------------------------------------------------

    class NetworkProtoDebugView : public EntityWorldDebugView
    {
        EE_REFLECT_TYPE( NetworkProtoDebugView );

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;
        virtual void DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass ) override;

        virtual void BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToReload, TVector<ResourceID> const& resourcesToBeReloaded ) override;

        void ProcessRecording( int32_t simulatedJoinInProgressFrame = -1 );
        void ResetRecordingData();
        void GenerateTaskSystemPose();

    private:

        EntityWorld const*                          m_pWorld = nullptr;
        PlayerManager*                              m_pPlayerManager = nullptr;
        bool                                        m_isActionDebugWindowOpen = false;
        bool                                        m_isCharacterControllerDebugWindowOpen = true;

        //-------------------------------------------------------------------------

        Animation::GraphComponent*                  m_pPlayerGraphComponent = nullptr;
        Animation::GraphRecorder                    m_graphRecorder;
        int32_t                                     m_updateFrameIdx = InvalidIndex;
        bool                                        m_isRecording = false;
        TVector<float>                              m_serializedTaskSizes;
        TVector<float>                              m_serializedTaskDeltas;
        float                                       m_minSerializedTaskDataSize;
        float                                       m_maxSerializedTaskDataSize;
        Animation::GraphInstance*                   m_pActualInstance = nullptr;
        Animation::GraphInstance*                   m_pReplicatedInstance = nullptr;
        Animation::TaskSystem*                      m_pTaskSystem = nullptr;
        Animation::Pose*                            m_pGeneratedPose = nullptr;
        TVector<Animation::Pose>                    m_actualPoses;
        TVector<Animation::Pose>                    m_replicatedPoses;
    };
}
#endif