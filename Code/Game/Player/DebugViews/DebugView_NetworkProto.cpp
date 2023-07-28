#include "DebugView_NetworkProto.h"
#include "Engine/Animation/Components/Component_AnimationGraph.h"
#include "Engine/Animation/TaskSystem/Animation_TaskSystem.h"
#include "Engine/Animation/DebugViews/DebugView_Animation.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntitySystem.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Player/Systems/WorldSystem_PlayerManager.h"
#include "Engine/UpdateContext.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Math/MathUtils.h"
#include "Base/Resource/ResourceSystem.h"
#include "Base/ThirdParty/implot/implot.h"
#include "Engine/Render/Components/Component_RenderMesh.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Player
{
    void NetworkProtoDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        DebugView::Initialize( systemRegistry, pWorld );
        m_pPlayerManager = pWorld->GetWorldSystem<PlayerManager>();

        m_windows.emplace_back( "Network Proto", [this] ( EntityWorldUpdateContext const& context, bool isFocused, uint64_t ) { DrawWindow( context ); } );
        m_windows.back().m_isOpen = true;
    }

    void NetworkProtoDebugView::Shutdown()
    {
        ResetRecordingData();
        m_pPlayerManager = nullptr;
        DebugView::Shutdown();
    }

    void NetworkProtoDebugView::BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToReload, TVector<ResourceID> const& resourcesToBeReloaded )
    {
        if ( VectorContains( usersToReload, Resource::ResourceRequesterID( m_pPlayerGraphComponent->GetEntityID().m_value ) ) )
        {
            ResetRecordingData();
        }

        if ( VectorContains( resourcesToBeReloaded, m_graphRecorder.m_graphID ) )
        {
            ResetRecordingData();
        }
    }

    void NetworkProtoDebugView::Update( EntityWorldUpdateContext const& context )
    {
        PlayerController* pPlayerController = nullptr;
        if ( m_pPlayerManager->GetPlayerEntityID().IsValid() )
        {
            auto pPlayerEntity = m_pWorld->GetPersistentMap()->FindEntity( m_pPlayerManager->GetPlayerEntityID() );
            for ( auto pComponent : pPlayerEntity->GetComponents() )
            {
                m_pPlayerGraphComponent = TryCast<Animation::GraphComponent>( pComponent );
                if ( m_pPlayerGraphComponent != nullptr )
                {
                    break;
                }

                if ( auto pMeshComponent = TryCast<Render::MeshComponent>( pComponent ) )
                {
                    pMeshComponent->SetVisible( !m_isMeshHidden );
                }
            }
        }

        if ( m_pPlayerGraphComponent == nullptr )
        {
            ResetRecordingData();
        }

        //-------------------------------------------------------------------------

        m_windows.back().m_isOpen = true;
    }

    void NetworkProtoDebugView::DrawWindow( EntityWorldUpdateContext const& context )
    {
        EE_ASSERT( m_pWorld != nullptr );

        if ( m_pPlayerGraphComponent == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( m_isRecording )
        {
            if ( ImGuiX::IconButton( EE_ICON_STOP, " Stop Recording", Colors::White, ImVec2( 200, 0 ) ) )
            {
                m_pPlayerGraphComponent->GetDebugGraphInstance()->StopRecording();
                m_pPlayerGraphComponent->GetDebugGraphInstance()->DisableTaskSystemSerialization();

                //-------------------------------------------------------------------------

                m_pActualInstance = EE::New<Animation::GraphInstance>( m_pPlayerGraphComponent->GetDebugGraphInstance()->GetGraphVariation(), 1 );
                m_pReplicatedInstance = EE::New<Animation::GraphInstance>( m_pPlayerGraphComponent->GetDebugGraphInstance()->GetGraphVariation(), 2 );
                m_pGeneratedPose = EE::New<Animation::Pose>( m_pPlayerGraphComponent->GetSkeleton() );

                m_pTaskSystem = EE::New<Animation::TaskSystem>( m_pPlayerGraphComponent->GetSkeleton() );
                m_pTaskSystem->EnableSerialization( *context.GetSystem<TypeSystem::TypeRegistry>() );

                ProcessRecording();
                m_updateFrameIdx = 0;
                m_isRecording = false;

                GenerateTaskSystemPose();
            }
        }
        else
        {
            if ( ImGuiX::IconButton( EE_ICON_RECORD, " Start Recording", Colors::Red, ImVec2( 200, 0 ) ) )
            {
                ResetRecordingData();
                m_pPlayerGraphComponent->GetDebugGraphInstance()->EnableTaskSystemSerialization( *context.GetSystem<TypeSystem::TypeRegistry>() );
                m_pPlayerGraphComponent->GetDebugGraphInstance()->StartRecording( &m_graphRecorder );
                m_isRecording = true;
            }
        }

        //-------------------------------------------------------------------------

        ImGui::SameLine();

        ImGuiX::Checkbox( "Hide Mesh", &m_isMeshHidden );

        //-------------------------------------------------------------------------

        if ( m_graphRecorder.HasRecordedData() )
        {
            auto UpdateFrameIndex = [this] ( int32_t newFrameIdx )
            {
                if ( newFrameIdx != m_updateFrameIdx )
                {
                    m_updateFrameIdx = Math::Clamp( newFrameIdx, 0, m_graphRecorder.GetNumRecordedFrames() - 1 );
                    GenerateTaskSystemPose();
                }
            };

            ImGuiX::TextSeparator( "Recorded Data" );

            // Timeline
            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Recorded Frame: " );
            ImGui::SameLine();
            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x - ( ImGui::GetStyle().ItemSpacing.x * 5 ) - 360 - 60 );
            if ( ImGui::SliderInt( "##timeline", &m_updateFrameIdx, 0, m_graphRecorder.GetNumRecordedFrames() - 1 ) )
            {
                GenerateTaskSystemPose();
            }

            ImGui::SameLine();
            if ( ImGui::Button( EE_ICON_STEP_BACKWARD"##Frame--", ImVec2( 30, 0 ) ) )
            {
                UpdateFrameIndex( m_updateFrameIdx - 1 );
            }

            ImGui::SameLine();
            if ( ImGui::Button( EE_ICON_STEP_FORWARD"##Frame++", ImVec2( 30, 0 ) ) )
            {
                UpdateFrameIndex( m_updateFrameIdx + 1 );
            }

            // Join in progress
            ImGui::BeginDisabled( m_updateFrameIdx < 0 );
            ImGui::SameLine();
            if ( ImGui::Button( "Fake Join In Progress##SIMJIP", ImVec2( 180, 0 ) ) )
            {
                ProcessRecording( m_updateFrameIdx, false );
            }

            ImGui::SameLine();
            if ( ImGui::Button( "Fake JIP (Layer)##SIMJIP2", ImVec2( 180, 0 ) ) )
            {
                ProcessRecording( m_updateFrameIdx, true );
            }
            ImGui::EndDisabled();

            // Serialized Parameter Plots
            //-------------------------------------------------------------------------

            if ( !m_isRecording )
            {
                ImGuiX::TextSeparator( "Parameter Data" );

                if ( ImPlot::BeginPlot( "Recorded Parameter Data", ImVec2( -1, 200 ), ImPlotFlags_NoMenus | ImPlotFlags_NoMouseText | ImPlotFlags_NoLegend | ImPlotFlags_NoBoxSelect ) )
                {
                    ImPlot::SetupAxes( "Time", "Bytes", ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoLabel, ImPlotAxisFlags_AutoFit );
                    ImPlot::PlotBars( "Vertical", m_serializedParameterSizes.data(), m_graphRecorder.GetNumRecordedFrames(), 1.0f );
                    double x = (double) m_updateFrameIdx;
                    if ( ImPlot::DragLineX( 0, &x, ImVec4( 1, 0, 0, 1 ), 2, 0 ) )
                    {
                        UpdateFrameIndex( (int32_t) x );
                    }
                    ImPlot::EndPlot();
                }

                if ( ImPlot::BeginPlot( "Recorded Parameter Delta", ImVec2( -1, 200 ), ImPlotFlags_NoMenus | ImPlotFlags_NoMouseText | ImPlotFlags_NoLegend | ImPlotFlags_NoBoxSelect ) )
                {
                    ImPlot::SetupAxes( "Time", EE_ICON_DELTA" Bytes", ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoLabel, ImPlotAxisFlags_AutoFit );
                    ImPlot::PlotBars( "Vertical", m_serializedParameterDeltas.data(), m_graphRecorder.GetNumRecordedFrames(), 1.0f );
                    double x = (double) m_updateFrameIdx;
                    if ( ImPlot::DragLineX( 0, &x, ImVec4( 1, 0, 0, 1 ), 2, 0 ) )
                    {
                        UpdateFrameIndex( (int32_t) x );
                    }
                    ImPlot::EndPlot();
                }
            }

            // Serialized Task Plots
            //-------------------------------------------------------------------------

            if ( !m_isRecording )
            {
                ImGuiX::TextSeparator( "Task Data" );

                if ( ImPlot::BeginPlot( "Recorded Task Data", ImVec2( -1, 200 ), ImPlotFlags_NoMenus | ImPlotFlags_NoMouseText | ImPlotFlags_NoLegend | ImPlotFlags_NoBoxSelect ) )
                {
                    ImPlot::SetupAxes( "Time", "Bytes", ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoLabel, ImPlotAxisFlags_AutoFit );
                    ImPlot::PlotBars( "Vertical", m_serializedTaskSizes.data(), m_graphRecorder.GetNumRecordedFrames(), 1.0f );
                    double x = (double) m_updateFrameIdx;
                    if ( ImPlot::DragLineX( 0, &x, ImVec4( 1, 0, 0, 1 ), 2, 0 ) )
                    {
                        UpdateFrameIndex( (int32_t) x );
                    }
                    ImPlot::EndPlot();
                }

                if ( ImPlot::BeginPlot( "Recorded Task Delta", ImVec2( -1, 200 ), ImPlotFlags_NoMenus | ImPlotFlags_NoMouseText | ImPlotFlags_NoLegend | ImPlotFlags_NoBoxSelect ) )
                {
                    ImPlot::SetupAxes( "Time", EE_ICON_DELTA" Bytes", ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoLabel, ImPlotAxisFlags_AutoFit );
                    ImPlot::PlotBars( "Vertical", m_serializedTaskDeltas.data(), m_graphRecorder.GetNumRecordedFrames(), 1.0f );
                    double x = (double) m_updateFrameIdx;
                    if ( ImPlot::DragLineX( 0, &x, ImVec4( 1, 0, 0, 1 ), 2, 0 ) )
                    {
                        UpdateFrameIndex( (int32_t) x );
                    }
                    ImPlot::EndPlot();
                }
            }

            // Draw frame data info
            //-------------------------------------------------------------------------

            if ( m_updateFrameIdx != InvalidIndex )
            {
                auto const& frameData = m_graphRecorder.m_recordedData[m_updateFrameIdx];

                InlineString str( InlineString::CtorSprintf(), "Frame Data: %d", m_updateFrameIdx );
                ImGuiX::TextSeparator( str.c_str() );

                //-------------------------------------------------------------------------

                ImGui::Text( "Sync Range: ( %d, %.2f%% ) -> (%d, %.2f%%)", frameData.m_updateRange.m_startTime.m_eventIdx, frameData.m_updateRange.m_startTime.m_percentageThrough.ToFloat() * 100, frameData.m_updateRange.m_endTime.m_eventIdx, frameData.m_updateRange.m_endTime.m_percentageThrough.ToFloat() * 100 );

                ImGui::Text( "Serialized Task Size: %d bytes", frameData.m_serializedTaskData.size() );

                ImGui::Text( "Delta from previous frame : % d bytes",  (int32_t) m_serializedTaskDeltas[m_updateFrameIdx] );

                ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 8, 8 ) );
                bool const drawMainTable = ImGui::BeginTable( "RecData", 2, ImGuiTableFlags_Borders );
                ImGui::PopStyleVar();

                if ( drawMainTable )
                {
                   
                    ImGui::TableSetupColumn( "Parameters", ImGuiTableColumnFlags_WidthStretch, 0.5f );
                    ImGui::TableSetupColumn( "Tasks", ImGuiTableColumnFlags_WidthStretch, 0.5f );
                    ImGui::TableHeadersRow();

                    // Draw Control Parameters
                    //-------------------------------------------------------------------------

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();

                    if ( ImGui::BeginTable( "Params", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable ) )
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

                    // Draw task list
                    //-------------------------------------------------------------------------

                    ImGui::TableNextColumn();
                    Animation::AnimationDebugView::DrawGraphActiveTasksDebugView( m_pTaskSystem );

                    ImGui::EndTable();
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

            if ( m_pGeneratedPose != nullptr )
            {
                m_pGeneratedPose->DrawDebug( drawContext, nextRecordedFrameData.m_characterWorldTransform, Colors::Orange, 4.0f );
            }
        }
    }

    void NetworkProtoDebugView::ResetRecordingData()
    {
        m_graphRecorder.Reset();
        m_isRecording = false;
        m_updateFrameIdx = InvalidIndex;
        m_joinInProgressFrameIdx = InvalidIndex;
        EE::Delete( m_pActualInstance );
        EE::Delete( m_pReplicatedInstance );
        EE::Delete( m_pTaskSystem );
        EE::Delete( m_pGeneratedPose );
        m_actualPoses.clear();
        m_replicatedPoses.clear();
        m_serializedTaskSizes.clear();
        m_serializedTaskDeltas.clear();
    }

    void GenerateBitPackedParameterData( Animation::GraphInstance const* pGraphInstance, Animation::RecordedGraphFrameData const& data, Blob& outData )
    {
        Serialization::BitArchive<1280> archive;

        // Serialize parameters
        //-------------------------------------------------------------------------

        auto const& parameterData = data.m_parameterData;

        int32_t numParameters = pGraphInstance->GetNumControlParameters();
        EE_ASSERT( numParameters == parameterData.size() );
        for ( int16_t i = 0; i < numParameters; i++ )
        {
            Animation::GraphValueType type = pGraphInstance->GetControlParameterType( i );
            switch ( type )
            {
                case Animation::GraphValueType::Bool :
                {
                    archive.WriteBool( parameterData[i].m_bool );
                }
                break;

                case Animation::GraphValueType::ID :
                {
                    archive.WriteUInt( parameterData[i].m_ID.ToUint(), 32 );
                }
                break;

                case Animation::GraphValueType::Float :
                {
                    archive.WriteQuantizedFloat( parameterData[i].m_float, -1000, 1000 );
                }
                break;

                case Animation::GraphValueType::Vector :
                {
                    archive.WriteQuantizedFloat( parameterData[i].m_vector.GetX(), -FLT_MAX, FLT_MAX );
                    archive.WriteQuantizedFloat( parameterData[i].m_vector.GetY(), -FLT_MAX, FLT_MAX );
                    archive.WriteQuantizedFloat( parameterData[i].m_vector.GetZ(), -FLT_MAX, FLT_MAX );
                }
                break;

                case Animation::GraphValueType::Target:
                {
                    if ( parameterData[i].m_target.IsBoneTarget() )
                    {
                        archive.WriteUInt( parameterData[i].m_target.GetBoneID().ToUint(), 32 );
                    }
                    else
                    {
                        Quantization::EncodedQuaternion ec( parameterData[i].m_target.GetTransform().GetRotation() );
                        archive.WriteUInt( ec.m_data0, 16 );
                        archive.WriteUInt( ec.m_data1, 16 );
                        archive.WriteUInt( ec.m_data2, 16 );
                        archive.WriteQuantizedFloat( parameterData[i].m_target.GetTransform().GetTranslation().GetX(), -FLT_MAX, FLT_MAX );
                        archive.WriteQuantizedFloat( parameterData[i].m_target.GetTransform().GetTranslation().GetY(), -FLT_MAX, FLT_MAX );
                        archive.WriteQuantizedFloat( parameterData[i].m_target.GetTransform().GetTranslation().GetZ(), -FLT_MAX, FLT_MAX );
                    }
                }
                break;

                default:
                EE_HALT();
                break;
            }
        }

        // Serialize sync data
        //-------------------------------------------------------------------------

        archive.WriteUInt( (uint8_t) data.m_updateRange.m_startTime.m_eventIdx, 8 );
        archive.WriteNormalizedFloat16Bit( data.m_updateRange.m_startTime.m_percentageThrough );
        archive.WriteUInt( (uint8_t) data.m_updateRange.m_endTime.m_eventIdx, 8 );
        archive.WriteNormalizedFloat16Bit( data.m_updateRange.m_endTime.m_percentageThrough );

        uint8_t const numLayerNodes = (uint8_t) data.m_layerUpdateStates.size();
        archive.WriteUInt( numLayerNodes, 8 );

        for ( auto i = 0; i < numLayerNodes; i++ )
        {
            archive.WriteUInt( data.m_layerUpdateStates[i].m_nodeIdx, 16 );

            uint8_t const numLayers = (uint8_t) data.m_layerUpdateStates[i].m_updateRanges.size();
            archive.WriteUInt( numLayerNodes, 8 );

            for ( auto j = 0u; j < numLayers; j++ )
            {
                archive.WriteUInt( (uint8_t) data.m_updateRange.m_startTime.m_eventIdx, 8 );
                archive.WriteNormalizedFloat16Bit( data.m_updateRange.m_startTime.m_percentageThrough );
                archive.WriteUInt( (uint8_t) data.m_updateRange.m_endTime.m_eventIdx, 8 );
                archive.WriteNormalizedFloat16Bit( data.m_updateRange.m_endTime.m_percentageThrough );
            }
        }

        //-------------------------------------------------------------------------

        archive.GetWrittenData( outData );
    }

    void NetworkProtoDebugView::ProcessRecording( int32_t simulatedJoinInProgressFrame, bool useLayerInitInfo )
    {
        m_actualPoses.clear();
        m_replicatedPoses.clear();
        m_serializedTaskSizes.clear();
        m_serializedTaskDeltas.clear();

        m_joinInProgressFrameIdx = simulatedJoinInProgressFrame;

        // Serialized data
        //-------------------------------------------------------------------------

        m_minSerializedParameterDataSize = FLT_MAX;
        m_maxSerializedParameterDataSize = -FLT_MAX;

        m_minSerializedTaskDataSize = FLT_MAX;
        m_maxSerializedTaskDataSize = -FLT_MAX;

        for ( auto i = 0u; i < m_graphRecorder.m_recordedData.size(); i++ )
        {
            Blob& serializedParameterData = m_serializedParameterData.emplace_back();
            GenerateBitPackedParameterData( m_pReplicatedInstance, m_graphRecorder.m_recordedData[i], serializedParameterData );

            // Parameters
            //-------------------------------------------------------------------------

            float size = (float) serializedParameterData.size();
            m_minSerializedParameterDataSize = Math::Min( m_minSerializedParameterDataSize, size );
            m_maxSerializedParameterDataSize = Math::Max( m_maxSerializedParameterDataSize, size );
            m_serializedParameterSizes.emplace_back( size );

            if ( i == 0 )
            {
                m_serializedParameterDeltas.emplace_back( m_serializedParameterSizes[0] );
            }
            else // Calculate delta
            {
                auto const& from = m_serializedParameterData[i - 1];
                auto const& to = m_serializedParameterData[i];

                size_t const numSharedBytes = Math::Min( from.size(), to.size() );
                float delta = (float) to.size() - (float) from.size();

                for ( auto b = 0u; b < numSharedBytes; b++ )
                {
                    if ( from[b] != to[b] )
                    {
                        delta++;
                    }
                }

                m_serializedParameterDeltas.emplace_back( delta );
            }

            // Tasks
            //-------------------------------------------------------------------------

            size = (float) m_graphRecorder.m_recordedData[i].m_serializedTaskData.size();
            m_minSerializedTaskDataSize = Math::Min( m_minSerializedTaskDataSize, size );
            m_maxSerializedTaskDataSize = Math::Max( m_maxSerializedTaskDataSize, size );
            m_serializedTaskSizes.emplace_back( size );

            if ( i == 0 )
            {
                m_serializedTaskDeltas.emplace_back( m_serializedTaskSizes[0] );
            }
            else // Calculate delta
            {
                auto const& from = m_graphRecorder.m_recordedData[i-1].m_serializedTaskData;
                auto const& to = m_graphRecorder.m_recordedData[i].m_serializedTaskData;

                size_t const numSharedBytes = Math::Min( from.size(), to.size() );
                float delta = (float) to.size() - (float) from.size();

                for ( auto b = 0u; b < numSharedBytes; b++ )
                {
                    if ( from[b] != to[b] )
                    {
                        delta++;
                    }
                }

                m_serializedTaskDeltas.emplace_back( delta );
            }
        }

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
        m_pReplicatedInstance->ResetGraphState( startFrameData.m_updateRange.m_startTime, useLayerInitInfo ? &startFrameData.m_layerUpdateStates : nullptr );

        // Evaluate subsequent frames
        for ( auto i = startFrameIdx; i < m_graphRecorder.GetNumRecordedFrames(); i++ )
        {
            auto const& frameData = m_graphRecorder.m_recordedData[i];
            int32_t const nextFrameIdx = ( i < m_graphRecorder.GetNumRecordedFrames() - 1 ) ? i + 1 : i;
            auto const& nextRecordedFrameData = m_graphRecorder.m_recordedData[nextFrameIdx];

            // Set parameters
            m_pReplicatedInstance->SetRecordedFrameUpdateData( frameData );
            m_pReplicatedInstance->EvaluateGraph( frameData.m_deltaTime, frameData.m_characterWorldTransform, nullptr, nullptr );
            m_pReplicatedInstance->ExecutePrePhysicsPoseTasks( nextRecordedFrameData.m_characterWorldTransform );
            m_pReplicatedInstance->ExecutePostPhysicsPoseTasks();

            auto& pose = m_replicatedPoses.emplace_back( Animation::Pose( m_pReplicatedInstance->GetPose()->GetSkeleton() ) );
            pose.CopyFrom( *m_pReplicatedInstance->GetPose() );
        }
    }

    void NetworkProtoDebugView::GenerateTaskSystemPose()
    {
        EE_ASSERT( m_pTaskSystem != nullptr && m_pGeneratedPose != nullptr );
        EE_ASSERT( m_graphRecorder.HasRecordedData() && !m_isRecording );
        EE_ASSERT( m_updateFrameIdx != InvalidIndex );

        if ( m_joinInProgressFrameIdx != InvalidIndex && m_updateFrameIdx < m_joinInProgressFrameIdx )
        {
            m_pGeneratedPose->Reset( Animation::Pose::Type::ReferencePose );
        }
        else
        {
            auto const& frameData = m_graphRecorder.m_recordedData[m_updateFrameIdx];

            TInlineVector<Animation::ResourceLUT const*, 10> LUTs;
            m_pPlayerGraphComponent->GetDebugGraphInstance()->GetResourceLookupTables( LUTs );

            m_pTaskSystem->Reset();
            m_pTaskSystem->DeserializeTasks( LUTs, frameData.m_serializedTaskData );
            m_pTaskSystem->UpdatePrePhysics( frameData.m_deltaTime, frameData.m_characterWorldTransform, frameData.m_characterWorldTransform.GetInverse() );
            m_pTaskSystem->UpdatePostPhysics();
            m_pGeneratedPose->CopyFrom( m_pTaskSystem->GetPose() );
        }
    }
}
#endif