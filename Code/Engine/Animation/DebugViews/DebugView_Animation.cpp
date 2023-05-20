#include "DebugView_Animation.h"
#include "Engine/Animation/Systems/WorldSystem_Animation.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Instance.h"
#include "Engine/Animation/Components/Component_AnimationGraph.h"
#include "Engine/Animation/AnimationEvent.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "System/Imgui/ImguiX.h"
#include "System/Math/MathStringHelpers.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Animation
{
    void AnimationDebugView::DrawGraphControlParameters( GraphInstance* pGraphInstance )
    {
        if ( ImGui::BeginTable( "ControlParametersTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable ) )
        {
            ImGui::TableSetupColumn( "Parameter", ImGuiTableColumnFlags_WidthStretch, 0.5f );
            ImGui::TableSetupColumn( "Value", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableHeadersRow();

            int32_t const numControlParameters = pGraphInstance->GetNumControlParameters();
            for ( int16_t i = 0; i < numControlParameters; i++ )
            {
                StringID const nodeID = pGraphInstance->GetControlParameterID( i );
                GraphValueType const valueType = pGraphInstance->GetControlParameterType( i );

                InlineString stringValue;
                switch ( valueType )
                {
                    case GraphValueType::Bool:
                    {
                        if ( pGraphInstance->GetControlParameterValue<bool>( i ) )
                        {
                            stringValue = "True";
                        }
                        else
                        {
                            stringValue = "False";
                        }
                    }
                    break;

                    case GraphValueType::ID:
                    {
                        StringID const value = pGraphInstance->GetControlParameterValue<StringID>( i );
                        if ( value.IsValid() )
                        {
                            stringValue.sprintf( "%s", value.c_str() );
                        }
                        else
                        {
                            stringValue = "Unset";
                        }
                    }
                    break;

                    case GraphValueType::Int:
                    {
                        stringValue.sprintf( "%d", pGraphInstance->GetControlParameterValue<int32_t>( i ) );
                    }
                    break;

                    case GraphValueType::Float:
                    {
                        stringValue.sprintf( "%.3f", pGraphInstance->GetControlParameterValue<float>( i ) );
                    }
                    break;

                    case GraphValueType::Vector:
                    {
                        Vector const value = pGraphInstance->GetControlParameterValue<Vector>( i );
                        stringValue = Math::ToString( value );
                    }
                    break;

                    case GraphValueType::Target:
                    {
                        Target const value = pGraphInstance->GetControlParameterValue<Target>( i );
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
                    }
                    break;

                    default:
                    {
                        EE_UNREACHABLE_CODE();
                    }
                    break;
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex( 0 );
                ImGui::TextColored( ImGuiX::ToIm( GetColorForValueType( valueType ) ), "%s", nodeID.c_str() );

                ImGui::TableSetColumnIndex( 1 );
                ImGui::Text( stringValue.c_str() );
            }

            ImGui::EndTable();
        }
    }

    //-------------------------------------------------------------------------

    void AnimationDebugView::DrawTaskTreeRow( TaskSystem* pTaskSystem, TaskIndex currentTaskIdx )
    {
        static const char* const stageLabels[] = { "ERROR!", "Pre-Physics", "Post-Physics" };

        auto pTask = pTaskSystem->m_tasks[currentTaskIdx];
        InlineString const rowLabel( InlineString::CtorSprintf(), "%s - %s", stageLabels[(int32_t) pTask->GetActualUpdateStage()], pTask->GetDebugText().c_str() );

        ImGui::SetNextItemOpen( true );
        ImGui::PushStyleColor( ImGuiCol_Text, pTask->GetDebugColor().ToFloat4() );
        if ( ImGui::TreeNode( rowLabel.c_str() ) )
        {
            for ( auto taskIdx : pTask->GetDependencyIndices() )
            {
                DrawTaskTreeRow( pTaskSystem, taskIdx );
            }

            ImGui::TreePop();
        }
        ImGui::PopStyleColor();
    }

    void AnimationDebugView::DrawGraphActiveTasksDebugView( GraphInstance* pGraphInstance )
    {
        if ( pGraphInstance == nullptr || !pGraphInstance->IsInitialized() )
        {
            ImGui::Text( "No animation task debug to Show!" );
            return;
        }

        // Always use the task system from the context as this is guaranteed to be set
        auto pTaskSystem = pGraphInstance->m_graphContext.m_pTaskSystem;
        if ( !pTaskSystem->HasTasks() )
        {
            ImGui::Text( "No Active Tasks" );
            return;
        }

        TaskIndex const finalTask = (TaskIndex) pTaskSystem->m_tasks.size() - 1;
        DrawTaskTreeRow( pTaskSystem, finalTask );
    }

    //-------------------------------------------------------------------------

    void AnimationDebugView::DrawRootMotionRow( GraphInstance* pGraphInstance, RootMotionDebugger const* pRootMotionRecorder, int16_t currentActionIdx )
    {
        static char const* const actionTypes[] = { "Error", "Sample", "External Graph", "Modify", "Blend" };

        RootMotionDebugger::RecordedAction const* pAction = nullptr;

        InlineString rowLabel;
        if ( currentActionIdx != InvalidIndex )
        {
            pAction = &pRootMotionRecorder->GetRecordedActions()[currentActionIdx];
            String const& nodePath = pGraphInstance->m_pGraphVariation->GetDefinition()->GetNodePath( pAction->m_nodeIdx );
            rowLabel.sprintf( "%s - %s", actionTypes[(int32_t) pAction->m_actionType], nodePath.c_str() );
        }
        else
        {
            rowLabel = "No root motion action registered";
        }

        //-------------------------------------------------------------------------

        ImGui::SetNextItemOpen( true );
        ImGuiX::PushFont( ImGuiX::Font::MediumBold );
        if ( ImGui::TreeNode( rowLabel.c_str() ) )
        {
            ImGui::PopFont();

            ImGui::Indent( 20.0f );
            ImGuiX::PushFont( ImGuiX::Font::Small );
            if ( pAction != nullptr )
            {
                Float3 vR = pAction->m_rootMotionDelta.GetRotation().ToEulerAngles().GetAsDegrees();
                InlineString const tR( InlineString::CtorSprintf(), "x=%.3f, y=%.3f, z=%.3f", vR.m_x, vR.m_y, vR.m_z );

                Vector vT = pAction->m_rootMotionDelta.GetTranslation();
                InlineString const tT( InlineString::CtorSprintf(), "x=%f, y=%f, z=%f", vT.GetX(), vT.GetY(), vT.GetZ() );

                ImGui::Text( "R: %s, T: %s", tR.c_str(), tT.c_str() );
            }
            else
            {
                ImGui::Text( "No Root Motion" );
            }

            ImGui::PopFont();
            ImGui::Unindent( 20.0f );

            //-------------------------------------------------------------------------

            if ( pAction != nullptr )
            {
                for ( auto taskIdx : pAction->m_dependencies )
                {
                    DrawRootMotionRow( pGraphInstance, pRootMotionRecorder, taskIdx );
                }
            }

            ImGui::TreePop();
        }
    }

    void AnimationDebugView::DrawRootMotionDebugView( GraphInstance* pGraphInstance )
    {
        if ( pGraphInstance == nullptr || !pGraphInstance->IsInitialized() )
        {
            ImGui::Text( "No root motion debug to Show!" );
            return;
        }

        RootMotionDebugger const* pRootMotionRecorder = pGraphInstance->GetRootMotionDebugger();
        if ( pRootMotionRecorder->HasRecordedActions() )
        {
            DrawRootMotionRow( pGraphInstance, pRootMotionRecorder, pRootMotionRecorder->GetLastActionIndex() );
        }
        else
        {
            ImGui::Text( "No root motion actions this frame!" );
        }
    }

    //-------------------------------------------------------------------------

    void AnimationDebugView::DrawSampledAnimationEventsView( GraphInstance* pGraphInstance )
    {
        if ( pGraphInstance == nullptr || !pGraphInstance->IsInitialized() )
        {
            ImGui::Text( "Nothing to Show!" );
            return;
        }

        if ( pGraphInstance->GetSampledEvents().GetNumAnimationEventsSampled() == 0 )
        {
            ImGui::Text( "No Animation Events Sampled!" );
            return;
        }

        //-------------------------------------------------------------------------

        if ( ImGui::BeginTable( "AnimEventsTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable ) )
        {
            ImGui::TableSetupColumn( "Branch", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 18 );
            ImGui::TableSetupColumn( "Ignored", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 18 );
            ImGui::TableSetupColumn( "Weight", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40 );
            ImGui::TableSetupColumn( "%", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40 );
            ImGui::TableSetupColumn( "Event", ImGuiTableColumnFlags_WidthStretch, 0.5f );
            ImGui::TableSetupColumn( "Source", ImGuiTableColumnFlags_WidthStretch, 0.5f );
            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            bool hasAnimEvent = false;
            for ( SampledEvent const& sampledEvent : pGraphInstance->GetSampledEvents() )
            {
                if ( sampledEvent.IsStateEvent() )
                {
                    continue;
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextRow();
                hasAnimEvent = true;

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                if ( sampledEvent.IsFromActiveBranch() )
                {
                    ImGui::TextColored( Colors::Lime.ToFloat4(), EE_ICON_SOURCE_BRANCH_CHECK );
                    ImGuiX::TextTooltip( "Active Branch" );
                }
                else
                {
                    ImGui::TextColored( Colors::LightGray.ToFloat4(), EE_ICON_SOURCE_BRANCH_REMOVE );
                    ImGuiX::TextTooltip( "Inactive Branch" );
                }

                ImGui::TableNextColumn();
                if ( sampledEvent.IsIgnored() )
                {
                    ImGui::TextColored( Colors::LightGray.ToFloat4(), EE_ICON_CLOSE );
                    ImGuiX::TextTooltip( "Ignored" );
                }
                else
                {
                    ImGui::TextColored( Colors::Lime.ToFloat4(), EE_ICON_CHECK );
                    ImGuiX::TextTooltip( "Valid Event" );
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::Text( "%.2f", sampledEvent.GetWeight() );

                ImGui::TableNextColumn();
                ImGui::Text( "%.1f", sampledEvent.GetPercentageThrough().ToFloat() * 100 );

                ImGui::TableNextColumn();
                Event const* pEvent = sampledEvent.GetEvent();
                InlineString const debugString = pEvent->GetDebugText();
                ImGui::Text( debugString.c_str() );
                ImGuiX::TextTooltip( debugString.c_str() );

                ImGui::TableNextColumn();
                String const& nodePath = pGraphInstance->m_pGraphVariation->GetDefinition()->GetNodePath( sampledEvent.GetSourceNodeIndex() );
                ImGui::Text( nodePath.c_str() );
                ImGuiX::TextTooltip( nodePath.c_str() );
            }

            ImGui::EndTable();

            if ( !hasAnimEvent )
            {
                ImGui::Text( "No Animation Events" );
            }
        }
    }

    void AnimationDebugView::DrawSampledStateEventsView( GraphInstance* pGraphInstance )
    {
        if ( pGraphInstance == nullptr || !pGraphInstance->IsInitialized() )
        {
            ImGui::Text( "Nothing to Show!" );
            return;
        }

        if ( pGraphInstance->GetSampledEvents().GetNumStateEventsSampled() == 0 )
        {
            ImGui::Text( "No State Events Sampled!" );
            return;
        }

        //-------------------------------------------------------------------------

        if ( ImGui::BeginTable( "StateEventsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable ) )
        {
            ImGui::TableSetupColumn( "Branch", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 18 );
            ImGui::TableSetupColumn( "Ignored", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 18 );
            ImGui::TableSetupColumn( "Weight", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40 );
            ImGui::TableSetupColumn( "Event", ImGuiTableColumnFlags_WidthStretch, 0.5f );
            ImGui::TableSetupColumn( "Source", ImGuiTableColumnFlags_WidthStretch, 0.5f );
            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            bool hasStateEvent = false;
            for ( SampledEvent const& sampledEvent : pGraphInstance->GetSampledEvents() )
            {
                if ( sampledEvent.IsAnimationEvent() )
                {
                    continue;
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextRow();
                hasStateEvent = true;

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                if ( sampledEvent.IsFromActiveBranch() )
                {
                    ImGui::TextColored( Colors::Lime.ToFloat4(), EE_ICON_SOURCE_BRANCH_CHECK );
                    ImGuiX::TextTooltip( "Active Branch" );
                }
                else
                {
                    ImGui::TextColored( Colors::LightGray.ToFloat4(), EE_ICON_SOURCE_BRANCH_REMOVE );
                    ImGuiX::TextTooltip( "Inactive Branch" );
                }

                ImGui::TableNextColumn();
                if ( sampledEvent.IsIgnored() )
                {
                    ImGui::TextColored( Colors::LightGray.ToFloat4(), EE_ICON_CLOSE );
                    ImGuiX::TextTooltip( "Ignored" );
                }
                else
                {
                    ImGui::TextColored( Colors::Lime.ToFloat4(), EE_ICON_CHECK );
                    ImGuiX::TextTooltip( "Valid Event" );
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::Text( "%.2f", sampledEvent.GetWeight() );

                ImGui::TableNextColumn();
                ImGui::Text( sampledEvent.GetStateEventID().c_str() );
                ImGuiX::TextTooltip( sampledEvent.GetStateEventID().c_str() );

                ImGui::TableNextColumn();
                String const& nodePath = pGraphInstance->m_pGraphVariation->GetDefinition()->GetNodePath( sampledEvent.GetSourceNodeIndex() );
                ImGui::Text( nodePath.c_str() );
                ImGuiX::TextTooltip( nodePath.c_str() );
            }

            if ( !hasStateEvent )
            {
                ImGui::Text( "No State Events" );
            }

            ImGui::EndTable();
        }
    }

    void AnimationDebugView::DrawCombinedSampledEventsView( GraphInstance* pGraphInstance )
    {
        if ( pGraphInstance == nullptr || !pGraphInstance->IsInitialized() )
        {
            ImGui::Text( "Nothing to Show!" );
            return;
        }

        //-------------------------------------------------------------------------

        DrawSampledAnimationEventsView( pGraphInstance );
        DrawSampledStateEventsView( pGraphInstance );
    }

    //-------------------------------------------------------------------------

    AnimationDebugView::AnimationDebugView()
    {
        m_menus.emplace_back( DebugMenu( "Engine/Animation", [this] ( EntityWorldUpdateContext const& context ) { DrawMenu( context ); } ) );
    }

    void AnimationDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        m_pWorld = pWorld;
        m_pAnimationWorldSystem = pWorld->GetWorldSystem<AnimationWorldSystem>();
    }

    void AnimationDebugView::Shutdown()
    {
        m_pAnimationWorldSystem = nullptr;
        m_pWorld = nullptr;
    }

    //-------------------------------------------------------------------------

    AnimationDebugView::ComponentDebugState* AnimationDebugView::GetDebugState( ComponentID ID )
    {
        for ( auto& debuggedComponent : m_componentRuntimeSettings )
        {
            if ( debuggedComponent.m_ID == ID )
            {
                return &debuggedComponent;
            }
        }

        return &m_componentRuntimeSettings.emplace_back( ID );
    }

    void AnimationDebugView::DestroyDebugState( ComponentID ID )
    {
        for ( int32_t i = 0; i < (int32_t) m_componentRuntimeSettings.size(); i++ )
        {
            if ( m_componentRuntimeSettings[i].m_ID == ID )
            {
                m_componentRuntimeSettings.erase_unsorted( m_componentRuntimeSettings.begin() + i );
                break;
            }
        }

        EE_UNREACHABLE_CODE();
    }

    //-------------------------------------------------------------------------

    void AnimationDebugView::DrawMenu( EntityWorldUpdateContext const& context )
    {
        InlineString componentName;
        for ( AnimationGraphComponent* pGraphComponent : m_pAnimationWorldSystem->m_graphComponents )
        {
            EntityID const entityID = pGraphComponent->GetEntityID();
            auto pEntity = m_pWorld->FindEntity( entityID );
            EE_ASSERT( pEntity != nullptr );

            auto pRuntimeSettings = GetDebugState( pGraphComponent->GetID() );

            //-------------------------------------------------------------------------

            componentName.sprintf( "%s (%s)", pGraphComponent->GetNameID().c_str(), pEntity->GetNameID().c_str() );
            if ( ImGui::BeginMenu( componentName.c_str() ) )
            {
                ImGuiX::TextSeparator( "Graph" );

                if ( ImGui::MenuItem( "Show Control Parameters" ) )
                {
                    pRuntimeSettings->m_drawControlParameters = true;
                }

                if ( ImGui::MenuItem( "Show Sampled Events" ) )
                {
                    pRuntimeSettings->m_drawSampledEvents = true;
                }

                //-------------------------------------------------------------------------

                ImGuiX::TextSeparator( "Root Motion debug" );
                {
                    RootMotionDebugMode const debugMode = pGraphComponent->GetRootMotionDebugMode();

                    bool const isRootVisualizationOff = debugMode == RootMotionDebugMode::Off;
                    if ( ImGui::RadioButton( "No Root Motion Visualization##RootMotion", isRootVisualizationOff ) )
                    {
                        pGraphComponent->SetRootMotionDebugMode( RootMotionDebugMode::Off );
                    }

                    bool const isRootVisualizationOn = debugMode == RootMotionDebugMode::DrawRoot;
                    if ( ImGui::RadioButton( "Draw Root", isRootVisualizationOn ) )
                    {
                        pGraphComponent->SetRootMotionDebugMode( RootMotionDebugMode::DrawRoot );
                    }

                    bool const isRootMotionRecordingEnabled = debugMode == RootMotionDebugMode::DrawRecordedRootMotion;
                    if ( ImGui::RadioButton( "Draw Recorded Root Motion", isRootMotionRecordingEnabled ) )
                    {
                        pGraphComponent->SetRootMotionDebugMode( RootMotionDebugMode::DrawRecordedRootMotion );
                    }

                    bool const isAdvancedRootMotionRecordingEnabled = debugMode == RootMotionDebugMode::DrawRecordedRootMotionAdvanced;
                    if ( ImGui::RadioButton( "Draw Advanced Recorded Root Motion", isAdvancedRootMotionRecordingEnabled ) )
                    {
                        pGraphComponent->SetRootMotionDebugMode( RootMotionDebugMode::DrawRecordedRootMotionAdvanced );
                    }
                }

                //-------------------------------------------------------------------------

                ImGuiX::TextSeparator( "Tasks" );

                if ( ImGui::MenuItem( "Show Active Tasks" ) )
                {
                    pRuntimeSettings->m_drawActiveTasks = true;
                }

                //-------------------------------------------------------------------------

                ImGuiX::TextSeparator( "Pose Debug" );
                {
                    TaskSystemDebugMode const debugMode = pGraphComponent->GetTaskSystemDebugMode();

                    bool const isVisualizationOff = debugMode == TaskSystemDebugMode::Off;
                    if ( ImGui::RadioButton( "No Pose Visualization##Tasks", isVisualizationOff ) )
                    {
                        pGraphComponent->SetTaskSystemDebugMode( TaskSystemDebugMode::Off );
                    }

                    bool const isFinalPoseEnabled = debugMode == TaskSystemDebugMode::FinalPose;
                    if ( ImGui::RadioButton( "FinalPose", isFinalPoseEnabled ) )
                    {
                        pGraphComponent->SetTaskSystemDebugMode( TaskSystemDebugMode::FinalPose );
                    }

                    bool const isPoseTreeEnabled = debugMode == TaskSystemDebugMode::PoseTree;
                    if ( ImGui::RadioButton( "Pose Tree", isPoseTreeEnabled ) )
                    {
                        pGraphComponent->SetTaskSystemDebugMode( TaskSystemDebugMode::PoseTree );
                    }

                    bool const isDetailedPoseTreeEnabled = debugMode == TaskSystemDebugMode::DetailedPoseTree;
                    if ( ImGui::RadioButton( "Detailed Pose Tree", isDetailedPoseTreeEnabled ) )
                    {
                        pGraphComponent->SetTaskSystemDebugMode( TaskSystemDebugMode::DetailedPoseTree );
                    }
                }

                ImGui::EndMenu();
            }
        }
    }

    void AnimationDebugView::DrawWindows( EntityWorldUpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        InlineString title;

        for ( int32_t i = (int32_t) m_componentRuntimeSettings.size() - 1; i >= 0; i-- )
        {
            bool stopDebug = false;
            auto ppFoundComponent = m_pAnimationWorldSystem->m_graphComponents.FindItem( m_componentRuntimeSettings[i].m_ID );
            if ( ppFoundComponent != nullptr )
            {
                auto pGraphComponent = *ppFoundComponent;

                //-------------------------------------------------------------------------

                EntityID const entityID = pGraphComponent->GetEntityID();
                auto pEntity = m_pWorld->FindEntity( entityID );
                EE_ASSERT( pEntity != nullptr );

                //-------------------------------------------------------------------------

                if ( m_componentRuntimeSettings[i].m_drawControlParameters )
                {
                    bool keepOpen = true;
                    title.sprintf( "Control Parameters: %s (%s)", pGraphComponent->GetNameID().c_str(), pEntity->GetNameID().c_str() );
                    ImGui::SetNextWindowSize( ImVec2( 600, 700 ), ImGuiCond_FirstUseEver );
                    if ( ImGui::Begin( title.c_str(), &keepOpen, ImGuiWindowFlags_NoSavedSettings) )
                    {
                        DrawGraphControlParameters( pGraphComponent->m_pGraphInstance );
                    }
                    ImGui::End();

                    if ( !keepOpen )
                    {
                        m_componentRuntimeSettings[i].m_drawControlParameters = false;
                    }
                }

                //-------------------------------------------------------------------------

                if ( m_componentRuntimeSettings[i].m_drawActiveTasks )
                {
                    bool keepOpen = true;
                    title.sprintf( "Active Tasks: %s (%s)", pGraphComponent->GetNameID().c_str(), pEntity->GetNameID().c_str() );
                    ImGui::SetNextWindowSize( ImVec2( 600, 700 ), ImGuiCond_FirstUseEver );
                    if ( ImGui::Begin( title.c_str(), &keepOpen, ImGuiWindowFlags_NoSavedSettings ) )
                    {
                        DrawGraphActiveTasksDebugView( pGraphComponent->m_pGraphInstance );
                    }
                    ImGui::End();

                    if ( !keepOpen )
                    {
                        m_componentRuntimeSettings[i].m_drawActiveTasks = false;
                    }
                }

                //-------------------------------------------------------------------------

                if ( m_componentRuntimeSettings[i].m_drawSampledEvents )
                {
                    bool keepOpen = true;
                    title.sprintf( "Sampled Events: %s (%s)", pGraphComponent->GetNameID().c_str(), pEntity->GetNameID().c_str() );
                    ImGui::SetNextWindowSize( ImVec2( 600, 700 ), ImGuiCond_FirstUseEver );
                    if ( ImGui::Begin( title.c_str(), &keepOpen, ImGuiWindowFlags_NoSavedSettings ) )
                    {
                        DrawCombinedSampledEventsView( pGraphComponent->m_pGraphInstance );
                    }
                    ImGui::End();

                    if ( !keepOpen )
                    {
                        m_componentRuntimeSettings[i].m_drawSampledEvents = false;
                    }
                }
            }
            else // Erase invalid component ID
            {
                stopDebug = true;
            }

            //-------------------------------------------------------------------------

            if ( stopDebug )
            {
                m_componentRuntimeSettings.erase_unsorted( m_componentRuntimeSettings.begin() + i );
            }
        }
    }
}
#endif