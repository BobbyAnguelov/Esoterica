#pragma once

#include "Engine/DebugViews/DebugView.h"
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

    class NetworkProtoDebugView : public DebugView
    {
        EE_REFLECT_TYPE( NetworkProtoDebugView );

    private:

        virtual void Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld ) override;
        virtual void Shutdown() override;

        virtual void Update( EntityWorldUpdateContext const& context ) override;
        virtual void HotReload_UnloadResources( TInlineVector<Resource::ResourceRequesterID, 20> const& usersToReload, TInlineVector<ResourceID, 20> const& resourcesToBeReloaded ) override;

        void ProcessRecording( int32_t simulatedJoinInProgressFrame = -1, bool useLayerInitInfo = false );
        void ResetRecordingData();
        void UpdateActualGraphInstance();
        void GenerateTaskSystemPose();

        void DrawWindow( EntityWorldUpdateContext const& context );

    private:

        PlayerManager*                              m_pPlayerManager = nullptr;
        bool                                        m_isMeshHidden = false;

        //-------------------------------------------------------------------------

        Animation::GraphComponent*                  m_pPlayerGraphComponent = nullptr;
        Animation::GraphRecorder                    m_graphRecorder;
        int32_t                                     m_updateFrameIdx = InvalidIndex;
        int32_t                                     m_joinInProgressFrameIdx = InvalidIndex;
        bool                                        m_isRecording = false;
        
        TVector<Blob>                               m_serializedParameterData;
        TVector<float>                              m_serializedParameterSizes;
        TVector<float>                              m_serializedParameterDeltas;
        float                                       m_minSerializedParameterDataSize;
        float                                       m_maxSerializedParameterDataSize;

        TVector<float>                              m_serializedTaskSizes;
        TVector<float>                              m_serializedTaskSizeDeltas;
        TVector<float>                              m_serializedTaskSharedByteDeltas;
        float                                       m_minSerializedTaskDataSize;
        float                                       m_maxSerializedTaskDataSize;

        Animation::GraphInstance*                   m_pActualInstance = nullptr;
        Animation::GraphInstance*                   m_pReplicatedInstance = nullptr;
        Animation::TaskSystem*                      m_pTaskSystem = nullptr;
        Animation::Pose*                            m_pGeneratedPose = nullptr;
        TVector<Animation::Pose>                    m_actualPoses;
        TVector<Animation::Pose>                    m_replicatedPoses;
        bool                                        m_showParameterPose = true;
        bool                                        m_showTaskPose = false;
    };
}
#endif