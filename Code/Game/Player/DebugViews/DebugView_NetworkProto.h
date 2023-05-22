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

        // HACK
        void ProcessRecording( int32_t simulatedJoinInProgressFrame = -1 );
        void ResetRecordingData();

    private:

        EntityWorld const*                          m_pWorld = nullptr;
        PlayerManager*                              m_pPlayerManager = nullptr;
        bool                                        m_isActionDebugWindowOpen = false;
        bool                                        m_isCharacterControllerDebugWindowOpen = true;

        // HACK
        Animation::GraphComponent*                  m_playerGraphComponent = nullptr;
        Animation::GraphRecorder                    m_graphRecorder;
        int32_t                                     m_updateFrameIdx = InvalidIndex;
        bool                                        m_isRecording = false;
        Animation::GraphInstance*                   m_pActualInstance = nullptr;
        Animation::GraphInstance*                   m_pReplicatedInstance = nullptr;
        TVector<Animation::Pose>                    m_actualPoses;
        TVector<Animation::Pose>                    m_replicatedPoses;
        // END HACK
    };
}
#endif