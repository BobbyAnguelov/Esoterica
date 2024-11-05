#include "DebugView_Animation.h"
#include "Engine/Animation/Systems/WorldSystem_Animation.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Instance.h"
#include "Engine/Animation/Components/Component_AnimationGraph.h"
#include "Engine/Animation/AnimationEvent.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Math/MathUtils.h"
#include "Base/Utils/TreeLayout.h"

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

                    case GraphValueType::Float:
                    {
                        stringValue.sprintf( "%.3f", pGraphInstance->GetControlParameterValue<float>( i ) );
                    }
                    break;

                    case GraphValueType::Vector:
                    {
                        Float3 const value = pGraphInstance->GetControlParameterValue<Float3>( i );
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
                ImGui::TextColored( ImVec4( GetColorForValueType( valueType ) ), "%s", nodeID.c_str() );

                ImGui::TableSetColumnIndex( 1 );
                ImGui::Text( stringValue.c_str() );
            }

            ImGui::EndTable();
        }
    }

    void AnimationDebugView::DrawGraphActiveTasksDebugView( GraphInstance* pGraphInstance, DebugPath const& filterPath, NavigateToSourceFunc const& navigateToNodeFunc )
    {
        if ( pGraphInstance == nullptr || !pGraphInstance->WasInitialized() )
        {
            ImGui::Text( "No animation task debug to Show!" );
            return;
        }

        // Always use the task system from the context as this is guaranteed to be set
        auto pTaskSystem = pGraphInstance->m_graphContext.m_pTaskSystem;
        if ( pTaskSystem == nullptr )
        {
            return;
        }

        if ( !pTaskSystem->HasTasks() )
        {
            ImGui::Text( "No Active Tasks" );
            return;
        }

        //-------------------------------------------------------------------------

        ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold );

        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 4, 0 ) );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 4 ) );

        constexpr static float const itemVerticalSpacing = 16;
        constexpr static float const itemHorizontalSpacing = 8;
        float const itemWindowHeight = ( ImGui::GetTextLineHeight() * 3 ) + ( ImGui::GetStyle().ItemSpacing.y * 2 ) + ( ImGui::GetStyle().WindowPadding.y * 2 );

        // Layout the nodes
        //-------------------------------------------------------------------------

        int32_t const numTasks = (int32_t) pTaskSystem->m_tasks.size();
        TreeLayout layout;
        layout.m_nodes.resize( numTasks );

        for ( int32_t i = 0; i < numTasks; i++ )
        {
            auto pTask = pTaskSystem->m_tasks[i];
            layout.m_nodes[i].m_pItem = pTask;
            layout.m_nodes[i].m_width = itemHorizontalSpacing + Math::Max( ImGui::CalcTextSize( pTask->GetDebugName() ).x, ImGui::CalcTextSize( pTask->GetDebugTextInfo().c_str() ).x ) + ( ImGui::GetStyle().WindowPadding.x * 2 );
            layout.m_nodes[i].m_height = itemWindowHeight;

            int32_t const numDependencies = pTask->GetNumDependencies();
            if ( numDependencies > 0 )
            {
                for ( int32_t j = 0; j < numDependencies; j++ )
                {
                    layout.m_nodes[i].m_children.emplace_back( &layout.m_nodes[pTask->GetDependencyIndices()[j]] );
                }
            }
        }

        layout.PerformLayout( Float2( 6.0f, itemVerticalSpacing ), Float2::Zero );

        // Draw Visualization
        //-------------------------------------------------------------------------

        ImVec2 const windowSize( Math::Max( ImGui::GetContentRegionAvail().x, layout.m_dimensions.m_x ), layout.m_dimensions.m_y + itemWindowHeight );
        if ( ImGui::BeginChild( "TL", windowSize, ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_ResizeY, ImGuiWindowFlags_HorizontalScrollbar ) )
        {
            ImVec2 const offset = ImVec2( windowSize.x / 2, itemWindowHeight / 2 );
            ImVec2 const windowPos = ImGui::GetWindowPos();
            ImVec2 const windowScrollOffset = ImVec2( ImGui::GetScrollX(), ImGui::GetScrollY() );
            ImVec2 const windowPosWithAllOffsets = windowPos + offset - windowScrollOffset;
            ImDrawList* pDrawList = ImGui::GetWindowDrawList();

            // Resolve debug paths
            //-------------------------------------------------------------------------

            TInlineVector<DebugPath, 20> resolvedPaths;
            for ( int8_t i = 0; i < numTasks; i++ )
            {
                resolvedPaths.emplace_back( pGraphInstance->ResolveTaskDebugPath( i ) );
            }

            // Draw lines
            //-------------------------------------------------------------------------

            for ( TreeLayout::Node_t& node : layout.m_nodes )
            {
                ImVec2 const nodeScreenPos = windowPosWithAllOffsets + node.m_position;
                int32_t const numDependencies = node.GetNumChildren();
                for ( int32_t j = 0; j < numDependencies; j++ )
                {
                    int32_t taskIdx = layout.GetNodeIndex( node.m_children[j] );
                    EE_ASSERT( taskIdx != InvalidIndex );

                    bool wasCreatedByCurrentDebugInstance = true;
                    if ( filterPath.IsValid() && !filterPath.IsParentOf( resolvedPaths[taskIdx] ) )
                    {
                        wasCreatedByCurrentDebugInstance = false;
                    }

                    ImVec2 const childScreenPos = windowPosWithAllOffsets + node.m_children[j]->m_position;
                    pDrawList->AddLine( nodeScreenPos, childScreenPos, wasCreatedByCurrentDebugInstance ? Colors::Lime : Colors::Gray, 2.0f );
                }
            }

            // Draw items
            //-------------------------------------------------------------------------

            ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 5 );

            for ( int8_t i = 0; i < numTasks; i++ )
            {
                auto const& node = layout.m_nodes[i];
;               auto pTask = ( Task const* ) node.m_pItem;

                InlineString const sourcePath = resolvedPaths[i].GetFlattenedPath();

                // Is this item from the current debug instance?
                bool wasCreatedByCurrentDebugInstance = true;
                if ( filterPath.IsValid() && !filterPath.IsParentOf( resolvedPaths[i] ) )
                {
                    wasCreatedByCurrentDebugInstance = false;
                }

                // Calculate item size
                //-------------------------------------------------------------------------

                InlineString const debugInfo = pTask->GetDebugTextInfo();
                float const itemWindowWidth = Math::Max( ImGui::CalcTextSize( pTask->GetDebugName() ).x, ImGui::CalcTextSize( debugInfo.c_str() ).x ) + ( ImGui::GetStyle().WindowPadding.x * 2 );
                ImVec2 const itemSize( itemWindowWidth, itemWindowHeight );
                ImVec2 const itemHalfSize( itemSize / 2 );

                // Draw item
                //-------------------------------------------------------------------------

                ImGui::SetCursorPos( offset - itemHalfSize + node.m_position );
                ImGui::PushID( &node );

                ImGui::PushStyleColor( ImGuiCol_ChildBg, ImGuiX::Style::s_colorGray2 );
                ImGui::PushStyleColor( ImGuiCol_Border, wasCreatedByCurrentDebugInstance ? Colors::Lime : Colors::Gray );
                ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 1.0f );
                bool drawChild = ImGui::BeginChild( "child", itemSize, ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_Borders );
                ImGui::PopStyleVar();
                ImGui::PopStyleColor( 2 );

                if ( drawChild )
                {
                    auto pLocalDrawList = ImGui::GetWindowDrawList();

                    // Draw Task Name
                    //-------------------------------------------------------------------------

                    ImGui::TextColored( pTask->GetDebugColor().ToFloat4(), pTask->GetDebugName() );

                    // Draw Debug Info
                    //-------------------------------------------------------------------------

                    if ( !debugInfo.empty() )
                    {
                        ImGui::Text( pTask->GetDebugTextInfo().c_str() );
                    }

                    // Draw weight/progress
                    //-------------------------------------------------------------------------

                    float const weight = pTask->GetDebugProgressOrWeight();
                    constexpr static float const textMargins = 2.0f;
                    ImVec2 const barStartPos = ImGui::GetWindowPos() + ImGui::GetCursorPos() + ImVec2( 0, 2 );
                    ImVec2 const barEndPos = barStartPos + ImVec2( ImGui::GetContentRegionAvail().x * weight, ImGui::GetTextLineHeight() - 2 );
                    Color const startColor = Color::EvaluateRedGreenGradient( 0.0f, false );
                    Color const color = Color::EvaluateRedGreenGradient( weight, false );
                    pLocalDrawList->AddRectFilled( barStartPos, barEndPos, color );

                    ImGui::SetCursorPosX( ImGui::GetCursorPosX() + textMargins );
                    ImGui::TextColored( Colors::Black.ToFloat4(), "%.2f", pTask->GetDebugProgressOrWeight());

                    // Navigation
                    //-------------------------------------------------------------------------

                    if ( ImGui::IsWindowHovered() )
                    {
                        ImGui::SetTooltip( sourcePath.c_str() );

                        if ( navigateToNodeFunc != nullptr )
                        {
                            ImGui::SetMouseCursor( ImGuiMouseCursor_Hand );
                            if ( wasCreatedByCurrentDebugInstance && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) )
                            {
                                navigateToNodeFunc( resolvedPaths[i] );
                            }
                        }
                    }
                }
                ImGui::EndChild();
                ImGui::PopID();
            }

            ImGui::PopStyleVar();
        }
        ImGui::EndChild();

        //-------------------------------------------------------------------------

        ImGui::PopStyleVar( 2 );
    }

    //-------------------------------------------------------------------------

    void AnimationDebugView::DrawRootMotionRow( GraphInstance* pGraphInstance, DebugPath const& filterPath, RootMotionDebugger const* pRootMotionRecorder, int16_t currentActionIdx, NavigateToSourceFunc const& navigateToNodeFunc )
    {
        RootMotionDebugger::RecordedAction const* pAction = nullptr;
        if ( currentActionIdx != InvalidIndex )
        {
            pAction = &pRootMotionRecorder->GetRecordedActions()[currentActionIdx];
        }

        //-------------------------------------------------------------------------

        ImGui::TableNextRow();

        if ( pAction != nullptr )
        {
            DebugPath const path = pGraphInstance->ResolveRootMotionDebugPath( currentActionIdx );
            InlineString const sourcePath = path.GetFlattenedPath();

            bool wasSampledFromDebuggedInstance = true;
            if ( filterPath.IsValid() && !filterPath.IsParentOf( path ) )
            {
                wasSampledFromDebuggedInstance = false;
            }

            //-------------------------------------------------------------------------

            ImGui::TableNextColumn();
            InlineString const nodeName( InlineString::CtorSprintf(), "%s##%d_%s", RootMotionDebugger::s_actionTypeLabels[(int32_t) pAction->m_actionType], currentActionIdx, sourcePath.c_str());
            ImGui::AlignTextToFramePadding();
            uint32_t const nodeFlags = ( pAction->m_dependencies.empty() ? ImGuiTreeNodeFlags_Bullet : 0 );
            ImGui::SetNextItemOpen( true );
            ImGui::PushStyleColor( ImGuiCol_Text, RootMotionDebugger::s_actionTypeColors[(int32_t) pAction->m_actionType] );
            bool isRowOpen = ImGui::TreeNodeEx( nodeName.c_str(), nodeFlags );
            ImGui::PopStyleColor();

            //-------------------------------------------------------------------------

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            Float3 vR = pAction->m_rootMotionDelta.GetRotation().ToEulerAngles().GetAsDegrees();
            ImGuiX::DrawFloat3( vR );

            //-------------------------------------------------------------------------

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            Vector vT = pAction->m_rootMotionDelta.GetTranslation();
            ImGuiX::DrawFloat3( vT );

            //-------------------------------------------------------------------------

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            float const length = vT.GetLength3();
            ImGui::Text( "%.4f", length );

            //-------------------------------------------------------------------------

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();

            if ( wasSampledFromDebuggedInstance )
            {
                if ( navigateToNodeFunc != nullptr )
                {
                    ImGui::PushID( currentActionIdx );
                    if ( ImGuiX::FlatButton( EE_ICON_MAGNIFY_SCAN ) )
                    {
                        navigateToNodeFunc( path );
                    }
                    ImGuiX::ItemTooltip( sourcePath.c_str() );
                    ImGui::PopID();
                }
            }
            else
            {
                ImGui::SameLine( 14.0f );
                ImGui::Text( EE_ICON_INFORMATION );
                ImGuiX::TextTooltip( "Sampled from another instance: %s", sourcePath.c_str() );
            }

            //-------------------------------------------------------------------------

            if ( isRowOpen )
            {
                for ( int16_t dependencyIdx : pAction->m_dependencies )
                {
                    DrawRootMotionRow( pGraphInstance, filterPath, pRootMotionRecorder, dependencyIdx, navigateToNodeFunc );
                }

                ImGui::TreePop();
            }
        }
        else
        {
            ImGui::TableNextColumn();
            ImGui::Text( "No Root Motion" );
        }
    }

    void AnimationDebugView::DrawRootMotionDebugView( GraphInstance* pGraphInstance, DebugPath const& filterPath, NavigateToSourceFunc const& navigateToNodeFunc )
    {
        if ( pGraphInstance == nullptr || !pGraphInstance->WasInitialized() )
        {
            ImGui::Text( "No root motion debug to Show!" );
            return;
        }

        RootMotionDebugger const* pRootMotionRecorder = pGraphInstance->GetRootMotionDebugger();
        if ( pRootMotionRecorder->HasRecordedActions() )
        {
            static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
            ImGui::PushStyleVar( ImGuiStyleVar_IndentSpacing, 20 );
            if ( ImGui::BeginTable( "RM Debug", 5, flags ) )
            {
                ImGui::TableSetupColumn( "Operation", ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupColumn( "Rotation", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 250 );
                ImGui::TableSetupColumn( "Translation", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 250 );
                ImGui::TableSetupColumn( "Length", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 60 );
                ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 30 );
                ImGui::TableHeadersRow();

                DrawRootMotionRow( pGraphInstance, filterPath, pRootMotionRecorder, pRootMotionRecorder->GetLastActionIndex(), navigateToNodeFunc );

                ImGui::EndTable();
            }
            ImGui::PopStyleVar();
        }
        else
        {
            ImGui::Text( "No root motion actions this frame!" );
        }
    }

    //-------------------------------------------------------------------------

    void AnimationDebugView::DrawSampledAnimationEventsView( GraphInstance* pGraphInstance, DebugPath const& filterPath, NavigateToSourceFunc const& navigateToNodeFunc )
    {
        if ( pGraphInstance == nullptr || !pGraphInstance->WasInitialized() )
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
            ImGui::TableSetupColumn( "Status", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 18 );
            ImGui::TableSetupColumn( "Weight", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40 );
            ImGui::TableSetupColumn( "%", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40 );
            ImGui::TableSetupColumn( "Event", ImGuiTableColumnFlags_WidthStretch, 0.5f );
            ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 30 );
            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            bool hasAnimEvent = false;
            SampledEventsBuffer const& sampledEvents = pGraphInstance->GetSampledEvents();
            for ( int32_t i = 0; i < sampledEvents.GetNumSampledEvents(); i++ )
            {
                SampledEvent const& sampledEvent = sampledEvents.GetEvent( i );
                if ( sampledEvent.IsStateEvent() )
                {
                    continue;
                }

                DebugPath const path = pGraphInstance->ResolveSampledEventDebugPath( i );
                bool wasSampledFromDebuggedInstance = true;
                if ( filterPath.IsValid() && !filterPath.IsParentOf( path ) )
                {
                    wasSampledFromDebuggedInstance = false;
                }

                ImGui::PushID( i );

                //-------------------------------------------------------------------------

                ImGui::TableNextRow();
                hasAnimEvent = true;

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
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
                    ImGui::SameLine( 4.0f );
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
                ImGui::TextColored( Color::EvaluateRedGreenGradient( sampledEvent.GetWeight() ).ToFloat4(), "%.2f", sampledEvent.GetWeight() );

                ImGui::TableNextColumn();
                ImGui::TextColored( Color::EvaluateRedGreenGradient( sampledEvent.GetPercentageThrough().ToFloat() ).ToFloat4(), "%.1f", sampledEvent.GetPercentageThrough().ToFloat() * 100 );

                ImGui::TableNextColumn();
                Event const* pEvent = sampledEvent.GetEvent();
                InlineString const debugString = pEvent->GetDebugText();
                ImGui::Text( debugString.c_str() );
                ImGuiX::TextTooltip( debugString.c_str() );

                ImGui::TableNextColumn();
                InlineString const sourcePath = path.GetFlattenedPath();

                if ( wasSampledFromDebuggedInstance )
                {
                    if ( navigateToNodeFunc != nullptr )
                    {
                        if ( ImGuiX::FlatButton( EE_ICON_MAGNIFY_SCAN ) )
                        {
                            navigateToNodeFunc( path );
                        }
                        ImGuiX::ItemTooltip( sourcePath.c_str() );
                    }
                    else
                    {
                        ImGui::SameLine( 14.0f );
                        ImGui::Text( EE_ICON_INFORMATION );
                        ImGuiX::TextTooltip( sourcePath.c_str() );
                    }
                }
                else
                {
                    ImGui::SameLine( 14.0f );
                    ImGui::Text( EE_ICON_INFORMATION );
                    ImGuiX::TextTooltip( "Sampled from another instance: %s", sourcePath.c_str() );
                }

                //-------------------------------------------------------------------------

                ImGui::PopID();
            }

            ImGui::EndTable();

            if ( !hasAnimEvent )
            {
                ImGui::Text( "No Animation Events" );
            }
        }
    }

    void AnimationDebugView::DrawSampledStateEventsView( GraphInstance* pGraphInstance, DebugPath const& filterPath, NavigateToSourceFunc const& navigateToNodeFunc )
    {
        if ( pGraphInstance == nullptr || !pGraphInstance->WasInitialized() )
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
            ImGui::TableSetupColumn( "Status", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 18 );
            ImGui::TableSetupColumn( "Weight", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40 );
            ImGui::TableSetupColumn( "Event", ImGuiTableColumnFlags_WidthStretch, 0.5f );
            ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 30 );
            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            bool hasStateEvent = false;
            SampledEventsBuffer const& sampledEvents = pGraphInstance->GetSampledEvents();
            for ( int32_t i = 0; i < sampledEvents.GetNumSampledEvents(); i++ )
            {
                SampledEvent const& sampledEvent = sampledEvents.GetEvent( i );

                if ( sampledEvent.IsAnimationEvent() )
                {
                    continue;
                }

                DebugPath const path = pGraphInstance->ResolveSampledEventDebugPath( i );
                bool wasSampledFromDebuggedInstance = true;
                if ( filterPath.IsValid() && !filterPath.IsParentOf( path ) )
                {
                    wasSampledFromDebuggedInstance = false;
                }

                ImGui::PushID( i );

                //-------------------------------------------------------------------------

                ImGui::TableNextRow();
                hasStateEvent = true;

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
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
                ImGui::TextColored( Color::EvaluateRedGreenGradient( sampledEvent.GetWeight() ).ToFloat4(), "%.2f", sampledEvent.GetWeight() );

                ImGui::TableNextColumn();
                {
                    static Color const stateEventTypeColors[] = { Colors::Gold, Colors::Lime, Colors::OrangeRed, Colors::DodgerBlue };

                    ImGui::PushStyleColor( ImGuiCol_Text, stateEventTypeColors[ (uint8_t) sampledEvent.GetStateEventType()] );
                    ImGui::Text( "[%s]", GetNameForStateEventType( sampledEvent.GetStateEventType() ) );
                    ImGui::PopStyleColor();
                    ImGui::SameLine();
                    ImGui::Text( sampledEvent.GetStateEventID().c_str() );
                    ImGuiX::TextTooltip( sampledEvent.GetStateEventID().c_str() );
                }

                ImGui::TableNextColumn();
                InlineString const sourcePath = path.GetFlattenedPath();
                
                if ( wasSampledFromDebuggedInstance )
                {
                    if ( navigateToNodeFunc != nullptr )
                    {
                        if ( ImGuiX::FlatButton( EE_ICON_MAGNIFY_SCAN ) )
                        {
                            navigateToNodeFunc( path );
                        }
                        ImGuiX::ItemTooltip( sourcePath.c_str() );
                    }
                    else
                    {
                        ImGui::SameLine( 14.0f );
                        ImGui::Text( EE_ICON_INFORMATION );
                        ImGuiX::TextTooltip( sourcePath.c_str() );
                    }
                }
                else
                {
                    ImGui::SameLine( 14.0f );
                    ImGui::Text( EE_ICON_INFORMATION );
                    ImGuiX::TextTooltip( "Sampled from another instance: %s", sourcePath.c_str() );
                }

                //-------------------------------------------------------------------------

                ImGui::PopID();
            }

            if ( !hasStateEvent )
            {
                ImGui::Text( "No State Events" );
            }

            ImGui::EndTable();
        }
    }

    void AnimationDebugView::DrawCombinedSampledEventsView( GraphInstance* pGraphInstance, DebugPath const& filterPath, NavigateToSourceFunc const& navigateToNodeFunc )
    {
        if ( pGraphInstance == nullptr || !pGraphInstance->WasInitialized() )
        {
            ImGui::Text( "Nothing to Show!" );
            return;
        }

        //-------------------------------------------------------------------------

        DrawSampledAnimationEventsView( pGraphInstance, filterPath, navigateToNodeFunc );
        DrawSampledStateEventsView( pGraphInstance, filterPath, navigateToNodeFunc );
    }

    //-------------------------------------------------------------------------

    void AnimationDebugView::Initialize( SystemRegistry const& systemRegistry, EntityWorld const* pWorld )
    {
        DebugView::Initialize( systemRegistry, pWorld );
        m_pAnimationWorldSystem = pWorld->GetWorldSystem<AnimationWorldSystem>();
    }

    void AnimationDebugView::Shutdown()
    {
        m_pAnimationWorldSystem = nullptr;
        DebugView::Shutdown();
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
        static StringID const controlParameterWindowID( "ControlParam" );
        static StringID const tasksWindowID( "Tasks" );
        static StringID const eventsWindowID( "Events" );

        //-------------------------------------------------------------------------


        InlineString componentName;
        for ( GraphComponent* pGraphComponent : m_pAnimationWorldSystem->m_graphComponents )
        {
            EntityID const entityID = pGraphComponent->GetEntityID();
            auto pEntity = m_pWorld->FindEntity( entityID );
            EE_ASSERT( pEntity != nullptr );

            auto pRuntimeSettings = GetDebugState( pGraphComponent->GetID() );

            //-------------------------------------------------------------------------

            componentName.sprintf( "%s (%s)", pGraphComponent->GetNameID().c_str(), pEntity->GetNameID().c_str() );
            if ( ImGui::BeginMenu( componentName.c_str() ) )
            {
                ImGui::SeparatorText( "Graph" );

                if ( ImGui::MenuItem( "Show Control Parameters" ) )
                {
                    auto pTaskWindowForComponent = GetDebugWindow( controlParameterWindowID, pGraphComponent->GetID().m_value );
                    if ( pTaskWindowForComponent != nullptr )
                    {
                        pTaskWindowForComponent->m_isOpen = true;
                    }
                    else
                    {
                        InlineString windowName( InlineString::CtorSprintf(), "Control Parameters: %s", componentName.c_str() );
                        m_windows.emplace_back( windowName.c_str(), [this] ( EntityWorldUpdateContext const& context, bool isFocused, uint64_t userData ) { DrawControlParameterWindow( context, isFocused, userData ); } );
                        m_windows.back().m_typeID = controlParameterWindowID;
                        m_windows.back().m_userData = pGraphComponent->GetID().m_value;
                        m_windows.back().m_isOpen = true;
                    }
                }

                if ( ImGui::MenuItem( "Show Sampled Events" ) )
                {
                    auto pTaskWindowForComponent = GetDebugWindow( eventsWindowID, pGraphComponent->GetID().m_value );
                    if ( pTaskWindowForComponent != nullptr )
                    {
                        pTaskWindowForComponent->m_isOpen = true;
                    }
                    else
                    {
                        InlineString windowName( InlineString::CtorSprintf(), "Sampled Events: %s", componentName.c_str() );
                        m_windows.emplace_back( windowName.c_str(), [this] ( EntityWorldUpdateContext const& context, bool isFocused, uint64_t userData ) { DrawEventsWindow( context, isFocused, userData ); } );
                        m_windows.back().m_typeID = eventsWindowID;
                        m_windows.back().m_userData = pGraphComponent->GetID().m_value;
                        m_windows.back().m_isOpen = true;
                    }
                }

                //-------------------------------------------------------------------------

                ImGui::SeparatorText( "Root Motion debug" );
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

                ImGui::SeparatorText( "Tasks" );

                if ( ImGui::MenuItem( "Show Active Tasks" ) )
                {
                    auto pTaskWindowForComponent = GetDebugWindow( tasksWindowID, pGraphComponent->GetID().m_value );
                    if ( pTaskWindowForComponent != nullptr )
                    {
                        pTaskWindowForComponent->m_isOpen = true;
                    }
                    else
                    {
                        InlineString windowName( InlineString::CtorSprintf(), "Active Tasks: %s", componentName.c_str() );
                        m_windows.emplace_back( windowName.c_str(), [this] ( EntityWorldUpdateContext const& context, bool isFocused, uint64_t userData ) { DrawTasksWindow( context, isFocused, userData ); } );
                        m_windows.back().m_typeID = tasksWindowID;
                        m_windows.back().m_userData = pGraphComponent->GetID().m_value;
                        m_windows.back().m_isOpen = true;
                    }
                }

                //-------------------------------------------------------------------------

                ImGui::SeparatorText( "Pose Debug" );
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

    void AnimationDebugView::Update( EntityWorldUpdateContext const& context )
    {
        // Delete windows for missing components
        for ( int32_t i = (int32_t) m_windows.size() - 1; i >= 0; i-- )
        {
            auto ppFoundComponent = m_pAnimationWorldSystem->m_graphComponents.FindItem( ComponentID( m_windows[i].m_userData ) );
            if ( ppFoundComponent == nullptr )
            {
                m_windows.erase_unsorted( m_windows.begin() + i );
            }
        }
    }

    void AnimationDebugView::DrawControlParameterWindow( EntityWorldUpdateContext const& context, bool isFocused, uint64_t userData )
    {
        auto ppFoundComponent = m_pAnimationWorldSystem->m_graphComponents.FindItem( ComponentID( userData ) );
        EE_ASSERT( ppFoundComponent != nullptr );
        auto pGraphComponent = *ppFoundComponent;
        DrawGraphControlParameters( pGraphComponent->m_pGraphInstance );
    }

    void AnimationDebugView::DrawTasksWindow( EntityWorldUpdateContext const& context, bool isFocused, uint64_t userData )
    {
        auto ppFoundComponent = m_pAnimationWorldSystem->m_graphComponents.FindItem( ComponentID( userData ) );
        EE_ASSERT( ppFoundComponent != nullptr );
        auto pGraphComponent = *ppFoundComponent;
        DrawGraphActiveTasksDebugView( pGraphComponent->m_pGraphInstance );
    }

    void AnimationDebugView::DrawEventsWindow( EntityWorldUpdateContext const& context, bool isFocused, uint64_t userData )
    {
        auto ppFoundComponent = m_pAnimationWorldSystem->m_graphComponents.FindItem( ComponentID( userData ) );
        EE_ASSERT( ppFoundComponent != nullptr );
        auto pGraphComponent = *ppFoundComponent;
        DrawCombinedSampledEventsView( pGraphComponent->m_pGraphInstance );
    }
}
#endif