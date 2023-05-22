#include "DebugView_NetworkProto.h"
#include "Engine/Animation/Components/Component_AnimationGraph.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntitySystem.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Player/Systems/WorldSystem_PlayerManager.h"
#include "Engine/UpdateContext.h"
#include "System/Imgui/ImguiX.h"
#include "System/Math/MathStringHelpers.h"
#include "System/Resource/ResourceSystem.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Player
{
    void NetworkProtoDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        m_pWorld = pWorld;
        m_pPlayerManager = pWorld->GetWorldSystem<PlayerManager>();
    }

    void NetworkProtoDebugView::Shutdown()
    {
        ResetRecordingData();
        m_pPlayerManager = nullptr;
        m_pWorld = nullptr;
    }

    void NetworkProtoDebugView::BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToReload, TVector<ResourceID> const& resourcesToBeReloaded )
    {
        if ( VectorContains( usersToReload, Resource::ResourceRequesterID( m_playerGraphComponent->GetEntityID().m_value ) ) )
        {
            ResetRecordingData();
        }

        if ( VectorContains( resourcesToBeReloaded, m_graphRecorder.m_graphID ) )
        {
            ResetRecordingData();
        }
    }

    void NetworkProtoDebugView::DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        EE_ASSERT( m_pWorld != nullptr );

        PlayerController* pPlayerController = nullptr;
        if ( m_isActionDebugWindowOpen || m_isCharacterControllerDebugWindowOpen )
        {
            if ( m_pPlayerManager->HasPlayer() )
            {
                auto pPlayerEntity = m_pWorld->GetPersistentMap()->FindEntity( m_pPlayerManager->GetPlayerEntityID() );
                for ( auto pComponent : pPlayerEntity->GetComponents() )
                {
                    m_playerGraphComponent = TryCast<Animation::GraphComponent>( pComponent );
                    if ( m_playerGraphComponent != nullptr )
                    {
                        break;
                    }
                }
            }
        }

        if ( m_playerGraphComponent == nullptr )
        {
            ResetRecordingData();
            return;
        }

        //-------------------------------------------------------------------------

        if ( pWindowClass != nullptr ) ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( "Network Proto" ) )
        {
            if ( m_isRecording )
            {
                if ( ImGuiX::IconButton( EE_ICON_STOP, " Stop Recording", ImGuiX::ImColors::White ) )
                {
                    m_playerGraphComponent->GetDebugGraphInstance()->StopRecording();

                    //-------------------------------------------------------------------------

                    m_pActualInstance = EE::New<Animation::GraphInstance>( m_playerGraphComponent->GetDebugGraphInstance()->GetGraphVariation(), 1 );
                    m_pReplicatedInstance = EE::New<Animation::GraphInstance>( m_playerGraphComponent->GetDebugGraphInstance()->GetGraphVariation(), 2 );

                    ProcessRecording();
                    m_updateFrameIdx = 0;
                    m_isRecording = false;
                }
            }
            else
            {
                if ( ImGuiX::IconButton( EE_ICON_RECORD, " Start Recording", ImGuiX::ImColors::Red ) )
                {
                    ResetRecordingData();
                    m_playerGraphComponent->GetDebugGraphInstance()->StartRecording( &m_graphRecorder );
                    m_isRecording = true;
                }
            }

            //-------------------------------------------------------------------------

            if ( m_graphRecorder.HasRecordedData() )
            {
                // Timeline
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Recorded Frame: " );
                ImGui::SameLine();
                ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x - 200 );
                if ( ImGui::SliderInt( "##timeline", &m_updateFrameIdx, 0, m_graphRecorder.GetNumRecordedFrames() - 1 ) )
                {
                    // Do Nothing
                }

                // Join in progress
                ImGui::BeginDisabled( m_updateFrameIdx < 0 );
                ImGui::SameLine();
                if ( ImGui::Button( "Fake Join In Progress##SIMJIP", ImVec2( 200, 0 ) ) )
                {
                    ProcessRecording( m_updateFrameIdx );
                }
                ImGui::EndDisabled();

                //-------------------------------------------------------------------------

                if ( m_updateFrameIdx != InvalidIndex )
                {
                    // Draw sync range
                    //-------------------------------------------------------------------------

                    auto const& frameData = m_graphRecorder.m_recordedData[m_updateFrameIdx];
                    ImGui::Text( "Sync Range: ( %d, %.2f%% ) -> (%d, %.2f%%)", frameData.m_updateRange.m_startTime.m_eventIdx, frameData.m_updateRange.m_startTime.m_percentageThrough.ToFloat(), frameData.m_updateRange.m_endTime.m_eventIdx, frameData.m_updateRange.m_endTime.m_percentageThrough.ToFloat() );

                    // Draw Control Parameters
                    //-------------------------------------------------------------------------

                    if ( ImGui::CollapsingHeader( "Parameters" ) )
                    {
                        if ( ImGui::BeginTable( "Params", 2, ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable ) )
                        {
                            ImGui::TableSetupColumn( "Label", ImGuiTableColumnFlags_WidthFixed, 300.0f );
                            ImGui::TableSetupColumn( "Value", ImGuiTableColumnFlags_WidthStretch );

                            int32_t const numParameters = m_pActualInstance->GetNumControlParameters();
                            for ( int32_t i = 0; i < numParameters; i++ )
                            {
                                ImGui::TableNextRow();

                                ImGui::TableNextColumn();
                                ImGui::Text( m_pActualInstance->GetControlParameterID( (int16_t) i ).c_str() );

                                ImGui::TableNextColumn();
                                switch ( m_pActualInstance->GetControlParameterType( (int16_t) i ) )
                                {
                                    case Animation::GraphValueType::Bool:
                                    {
                                        ImGui::Text( frameData.m_parameterData[i].m_bool ? "True" : "False" );
                                    }
                                    break;

                                    case Animation::GraphValueType::ID:
                                    {
                                        if ( frameData.m_parameterData[i].m_ID.IsValid() )
                                        {
                                            ImGui::Text( frameData.m_parameterData[i].m_ID.c_str() );
                                        }
                                        else
                                        {
                                            ImGui::Text( "" );
                                        }
                                    }
                                    break;

                                    case Animation::GraphValueType::Int:
                                    {
                                        ImGui::Text( "%d", frameData.m_parameterData[i].m_int );
                                    }
                                    break;

                                    case Animation::GraphValueType::Float:
                                    {
                                        ImGui::Text( "%f", frameData.m_parameterData[i].m_float );
                                    }
                                    break;

                                    case Animation::GraphValueType::Vector:
                                    {
                                        ImGui::Text( Math::ToString( frameData.m_parameterData[i].m_vector ).c_str() );
                                    }
                                    break;

                                    case Animation::GraphValueType::Target:
                                    {
                                        InlineString stringValue;
                                        Animation::Target const value = frameData.m_parameterData[i].m_target;
                                        if ( !value.IsTargetSet() )
                                        {
                                            stringValue = "Unset";
                                        }
                                        else if ( value.IsBoneTarget() )
                                        {
                                            stringValue.sprintf( "Bone: %s", value.GetBoneID().c_str() );
                                        }
                                        else
                                        {
                                            stringValue = "TODO";
                                        }
                                        ImGui::Text( stringValue.c_str() );
                                    }
                                    break;

                                    default:
                                    break;
                                }
                            }

                            ImGui::EndTable();
                        }
                    }
                }
            }

            //-------------------------------------------------------------------------

            if ( m_updateFrameIdx != InvalidIndex )
            {
                int32_t const nextFrameIdx = ( m_updateFrameIdx < m_graphRecorder.GetNumRecordedFrames() - 1 ) ? m_updateFrameIdx + 1 : m_updateFrameIdx;
                auto const& nextRecordedFrameData = m_graphRecorder.m_recordedData[nextFrameIdx];

                auto drawContext = context.GetDrawingContext();
                m_actualPoses[m_updateFrameIdx].DrawDebug( drawContext, nextRecordedFrameData.m_characterWorldTransform, Colors::LimeGreen, 4.0f );
                m_replicatedPoses[m_updateFrameIdx].DrawDebug( drawContext, nextRecordedFrameData.m_characterWorldTransform, Colors::HotPink, 4.0f );
            }
        }
        ImGui::End();
    }

    void NetworkProtoDebugView::ResetRecordingData()
    {
        m_graphRecorder.Reset();
        m_isRecording = false;
        m_updateFrameIdx = InvalidIndex;
        EE::Delete( m_pActualInstance );
        EE::Delete( m_pReplicatedInstance );
        m_actualPoses.clear();
        m_replicatedPoses.clear();
    }

    void NetworkProtoDebugView::ProcessRecording( int32_t simulatedJoinInProgressFrame )
    {
        m_actualPoses.clear();
        m_replicatedPoses.clear();

        // Actual recording
        //-------------------------------------------------------------------------

        m_graphRecorder.m_initialState.PrepareForReading();
        m_pActualInstance->SetToRecordedState( m_graphRecorder.m_initialState );

        for ( auto i = 0; i < m_graphRecorder.GetNumRecordedFrames(); i++ )
        {
            auto const& frameData = m_graphRecorder.m_recordedData[i];
            int32_t const nextFrameIdx = ( i < m_graphRecorder.GetNumRecordedFrames() - 1 ) ? i + 1 : i;
            auto const& nextRecordedFrameData = m_graphRecorder.m_recordedData[nextFrameIdx];

            // Set parameters
            m_pActualInstance->SetRecordedFrameUpdateData( frameData );
            m_pActualInstance->EvaluateGraph( frameData.m_deltaTime, frameData.m_characterWorldTransform, nullptr, false );
            m_pActualInstance->ExecutePrePhysicsPoseTasks( nextRecordedFrameData.m_characterWorldTransform );
            m_pActualInstance->ExecutePostPhysicsPoseTasks();

            auto& pose = m_actualPoses.emplace_back( Animation::Pose( m_pActualInstance->GetPose()->GetSkeleton() ) );
            pose.CopyFrom( *m_pActualInstance->GetPose() );
        }

        // Replicated Recording
        //-------------------------------------------------------------------------

        bool const isSimulatingJoinInProgress = simulatedJoinInProgressFrame > 0;
        int32_t startFrameIdx = 0;
        if ( isSimulatingJoinInProgress )
        {
            EE_ASSERT( simulatedJoinInProgressFrame < ( m_graphRecorder.GetNumRecordedFrames() - 1 ) );

            for ( int32_t i = 0; i < simulatedJoinInProgressFrame; i++ )
            {
                m_replicatedPoses.emplace_back( Animation::Pose( m_pReplicatedInstance->GetPose()->GetSkeleton(), Animation::Pose::Type::ReferencePose ) );
            }

            startFrameIdx = simulatedJoinInProgressFrame;
        }

        // Set graph parameters before reset
        auto const& startFrameData = m_graphRecorder.m_recordedData[startFrameIdx];
        m_pReplicatedInstance->SetRecordedFrameUpdateData( startFrameData );
        m_pReplicatedInstance->ResetGraphState( startFrameData.m_updateRange.m_startTime );

        // Evaluate subsequent frames
        for ( auto i = startFrameIdx; i < m_graphRecorder.GetNumRecordedFrames(); i++ )
        {
            auto const& frameData = m_graphRecorder.m_recordedData[i];
            int32_t const nextFrameIdx = ( i < m_graphRecorder.GetNumRecordedFrames() - 1 ) ? i + 1 : i;
            auto const& nextRecordedFrameData = m_graphRecorder.m_recordedData[nextFrameIdx];

            // Set parameters
            m_pReplicatedInstance->SetRecordedFrameUpdateData( frameData );
            m_pReplicatedInstance->EvaluateGraph( frameData.m_deltaTime, frameData.m_characterWorldTransform, nullptr );
            m_pReplicatedInstance->ExecutePrePhysicsPoseTasks( nextRecordedFrameData.m_characterWorldTransform );
            m_pReplicatedInstance->ExecutePostPhysicsPoseTasks();

            auto& pose = m_replicatedPoses.emplace_back( Animation::Pose( m_pReplicatedInstance->GetPose()->GetSkeleton() ) );
            pose.CopyFrom( *m_pReplicatedInstance->GetPose() );
        }
    }
}
#endif