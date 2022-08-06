#include "Workspace_AnimationGraph.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_Definition.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Animation_EditorGraph_Compilation.h"
#include "EngineTools/Animation/GraphEditor/EditorGraph/Nodes/Animation_EditorGraphNode_ControlParameters.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationGraph.h"
#include "Engine/Camera/Components/Component_DebugCamera.h"
#include "Engine/Animation/Systems/EntitySystem_Animation.h"
#include "Engine/Animation/Components/Component_AnimationGraph.h"
#include "Engine/Animation/Systems/WorldSystem_Animation.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Physics/PhysicsSystem.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Engine/Animation/DebugViews/DebugView_Animation.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    EE_RESOURCE_WORKSPACE_FACTORY( AnimationGraphEditorFactory, GraphDefinition, AnimationGraphWorkspace );

    //-------------------------------------------------------------------------

    static InlineString GenerateFilePathForVariation( FileSystem::Path const& graphPath, StringID variationID )
    {
        FileSystem::Path const parentDirectory = graphPath.GetParentDirectory();
        String const filenameNoExtension = graphPath.GetFileNameWithoutExtension();

        InlineString variationPathStr;
        variationPathStr.sprintf( "%s%s_%s.agv", parentDirectory.c_str(), filenameNoExtension.c_str(), variationID.c_str() );
        return variationPathStr;
    }

    //-------------------------------------------------------------------------

    class GraphUndoableAction final : public IUndoableAction
    {
    public:

        GraphUndoableAction( AnimationGraphWorkspace* pWorkspace )
            : m_pWorkspace( pWorkspace )
        {
            EE_ASSERT( m_pWorkspace != nullptr );
        }

        virtual void Undo() override
        {
            Serialization::JsonArchiveReader archive;
            archive.ReadFromString( m_valueBefore.c_str() );
            m_pWorkspace->m_editorContext.LoadGraph( archive.GetDocument() );
        }

        virtual void Redo() override
        {
            Serialization::JsonArchiveReader archive;
            archive.ReadFromString( m_valueAfter.c_str() );
            m_pWorkspace->m_editorContext.LoadGraph( archive.GetDocument() );
        }

        void SerializeBeforeState()
        {
            if ( m_pWorkspace->IsDebugging() )
            {
                m_pWorkspace->StopDebugging();
            }

            Serialization::JsonArchiveWriter archive;
            m_pWorkspace->m_editorContext.SaveGraph( *archive.GetWriter() );
            m_valueBefore.resize( archive.GetStringBuffer().GetSize() );
            memcpy( m_valueBefore.data(), archive.GetStringBuffer().GetString(), archive.GetStringBuffer().GetSize() );
        }

        void SerializeAfterState()
        {
            Serialization::JsonArchiveWriter archive;
            m_pWorkspace->m_editorContext.SaveGraph( *archive.GetWriter() );
            m_valueAfter.resize( archive.GetStringBuffer().GetSize() );
            memcpy( m_valueAfter.data(), archive.GetStringBuffer().GetString(), archive.GetStringBuffer().GetSize() );
        }

    private:

        AnimationGraphWorkspace*            m_pWorkspace = nullptr;
        String                              m_valueBefore;
        String                              m_valueAfter;
    };

    //-------------------------------------------------------------------------

    AnimationGraphWorkspace::AnimationGraphWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID )
        : TResourceWorkspace<GraphDefinition>( pToolsContext, pWorld, resourceID, false )
        , m_editorContext( *pToolsContext )
        , m_propertyGrid( m_pToolsContext )
    {
        // Load graph from descriptor
        //-------------------------------------------------------------------------

        m_graphFilePath = GetFileSystemPath( resourceID.GetResourcePath() );
        if ( m_graphFilePath.IsValid() )
        {
            bool graphLoadFailed = false;

            // Try read JSON data
            Serialization::JsonArchiveReader archive;
            if ( !archive.ReadFromFile( m_graphFilePath ) )
            {
                graphLoadFailed = true;
            }

            // Try to load the graph from the file
            if ( !m_editorContext.LoadGraph( archive.GetDocument() ) )
            {
                EE_LOG_ERROR( "Animation", "Failed to load graph definition: %s", m_graphFilePath.c_str() );
            }

            m_graphEditor.NavigateTo( m_editorContext.GetRootGraph() );
        }

        EE_ASSERT( m_editorContext.GetGraphDefinition() != nullptr );

        // Gizmo
        //-------------------------------------------------------------------------

        m_gizmo.SetTargetTransform( &m_gizmoTransform );
        m_gizmo.SetCoordinateSystemSpace( CoordinateSpace::Local );
        m_gizmo.SetOption( ImGuiX::Gizmo::Options::DrawManipulationPlanes, true );
        m_gizmo.SetOption( ImGuiX::Gizmo::Options::AllowScale, false );
        m_gizmo.SetOption( ImGuiX::Gizmo::Options::AllowCoordinateSpaceSwitching, false );
        m_gizmo.SwitchMode( ImGuiX::Gizmo::GizmoMode::Translation );

        // Bind events
        //-------------------------------------------------------------------------

        auto OnBeginGraphModification = [this] ( VisualGraph::BaseGraph* pRootGraph )
        {
            if ( pRootGraph == m_editorContext.GetRootGraph() )
            {
                EE_ASSERT( m_pActiveUndoableAction == nullptr );

                m_pActiveUndoableAction = EE::New<GraphUndoableAction>( this );
                m_pActiveUndoableAction->SerializeBeforeState();
            }
        };

        auto OnEndGraphModification = [this] ( VisualGraph::BaseGraph* pRootGraph )
        {
            if ( pRootGraph == m_editorContext.GetRootGraph() )
            {
                EE_ASSERT( m_pActiveUndoableAction != nullptr );

                m_pActiveUndoableAction->SerializeAfterState();
                m_undoStack.RegisterAction( m_pActiveUndoableAction );
                m_pActiveUndoableAction = nullptr;
                m_editorContext.MarkDirty();
            }
        };

        m_rootGraphBeginModificationBindingID = VisualGraph::BaseGraph::OnBeginModification().Bind( OnBeginGraphModification );
        m_rootGraphEndModificationBindingID = VisualGraph::BaseGraph::OnEndModification().Bind( OnEndGraphModification );

        //-------------------------------------------------------------------------

        m_preEditEventBindingID = m_propertyGrid.OnPreEdit().Bind( [this] ( PropertyEditInfo const& info ) { m_editorContext.GetRootGraph()->BeginModification(); } );
        m_postEditEventBindingID = m_propertyGrid.OnPostEdit().Bind( [this] ( PropertyEditInfo const& info ) { m_editorContext.GetRootGraph()->EndModification(); } );

        //-------------------------------------------------------------------------

        m_navigateToNodeEventBindingID = m_editorContext.OnNavigateToNode().Bind( [this] ( VisualGraph::BaseNode* pNode ) { m_graphEditor.NavigateTo( pNode ); } );
        m_navigateToGraphEventBindingID = m_editorContext.OnNavigateToGraph().Bind( [this] ( VisualGraph::BaseGraph* pGraph ) { m_graphEditor.NavigateTo( pGraph ); } );
    }

    AnimationGraphWorkspace::~AnimationGraphWorkspace()
    {
        if ( IsDebugging() )
        {
            StopDebugging();
        }

        m_propertyGrid.OnPreEdit().Unbind( m_preEditEventBindingID );
        m_propertyGrid.OnPostEdit().Unbind( m_postEditEventBindingID );

        VisualGraph::BaseGraph::OnBeginModification().Unbind( m_rootGraphBeginModificationBindingID );
        VisualGraph::BaseGraph::OnEndModification().Unbind( m_rootGraphEndModificationBindingID );

        m_editorContext.OnNavigateToNode().Unbind( m_navigateToNodeEventBindingID );
        m_editorContext.OnNavigateToGraph().Unbind( m_navigateToGraphEventBindingID );
    }

    void AnimationGraphWorkspace::Initialize( UpdateContext const& context )
    {
        TResourceWorkspace<GraphDefinition>::Initialize( context );

        m_controlParametersWindowName.sprintf( "Control Parameters##%u", GetID() );
        m_graphViewWindowName.sprintf( "Graph View##%u", GetID() );
        m_propertyGridWindowName.sprintf( "Details##%u", GetID() );
        m_variationEditorWindowName.sprintf( "Variation Editor##%u", GetID() );
        m_compilationLogWindowName.sprintf( "Compilation Log##%u", GetID() );
        m_debuggerWindowName.sprintf( "Debugger##%u", GetID() );
    }

    void AnimationGraphWorkspace::InitializeDockingLayout( ImGuiID dockspaceID ) const
    {
        ImGuiID topLeftDockID = 0, bottomLeftDockID = 0, centerDockID = 0, rightDockID = 0, bottomRightDockID;

        ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.2f, &topLeftDockID, &centerDockID );
        ImGui::DockBuilderSplitNode( topLeftDockID, ImGuiDir_Down, 0.33f, &bottomLeftDockID, &topLeftDockID );
        ImGui::DockBuilderSplitNode( centerDockID, ImGuiDir_Left, 0.66f, &centerDockID, &rightDockID );
        ImGui::DockBuilderSplitNode( rightDockID, ImGuiDir_Down, 0.66f, &bottomRightDockID, &rightDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetViewportWindowID(), rightDockID );
        ImGui::DockBuilderDockWindow( m_debuggerWindowName.c_str(), bottomRightDockID );
        ImGui::DockBuilderDockWindow( m_controlParametersWindowName.c_str(), topLeftDockID );
        ImGui::DockBuilderDockWindow( m_propertyGridWindowName.c_str(), bottomLeftDockID );
        ImGui::DockBuilderDockWindow( m_graphViewWindowName.c_str(), centerDockID );
        ImGui::DockBuilderDockWindow( m_variationEditorWindowName.c_str(), centerDockID );
        ImGui::DockBuilderDockWindow( m_compilationLogWindowName.c_str(), bottomLeftDockID );
    }

    void AnimationGraphWorkspace::UpdateWorkspace( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused )
    {
        m_nodeContext.m_currentVariationID = m_selectedVariationID;

        // Control Parameters
        //-------------------------------------------------------------------------

        if ( m_controlParameterEditor.UpdateAndDraw( context, &m_nodeContext, pWindowClass, m_controlParametersWindowName.c_str() ) )
        {
            auto pVirtualParameterToEdit = m_controlParameterEditor.GetVirtualParameterToEdit();
            if ( pVirtualParameterToEdit != nullptr )
            {
                m_graphEditor.NavigateTo( pVirtualParameterToEdit->GetChildGraph() );
            }
        }

        // Variation Editor
        //-------------------------------------------------------------------------

        m_variationEditor.UpdateAndDraw( context, pWindowClass, m_variationEditorWindowName.c_str() );

        // Graph Editor
        //-------------------------------------------------------------------------

        m_graphEditor.UpdateAndDraw( context, &m_nodeContext, pWindowClass, m_graphViewWindowName.c_str() );

        // Property Grid
        //-------------------------------------------------------------------------

        auto const& selection = m_editorContext.GetSelectedNodes();
        if ( selection.empty() )
        {
            m_propertyGrid.SetTypeToEdit( nullptr );
        }
        else
        {
            auto pSelectedNode = selection.back().m_pNode;
            if ( m_propertyGrid.GetEditedType() != pSelectedNode )
            {
                // Handle control parameters as a special case
                auto pReferenceNode = TryCast<GraphNodes::ParameterReferenceEditorNode>( pSelectedNode );
                if ( pReferenceNode != nullptr && pReferenceNode->IsReferencingControlParameter() )
                {
                    m_propertyGrid.SetTypeToEdit( pReferenceNode->GetReferencedControlParameter() );
                }
                else
                {
                    m_propertyGrid.SetTypeToEdit( pSelectedNode );
                }
            }
        }

        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_propertyGridWindowName.c_str() ) )
        {
            if ( !selection.empty() )
            {
                ImGui::Text( "Node: %s", selection.back().m_pNode->GetName());
            }

            m_propertyGrid.DrawGrid();
        }
        ImGui::End();

        // Compilation Log
        //-------------------------------------------------------------------------

        m_graphCompilationLog.UpdateAndDraw( context, &m_nodeContext, pWindowClass, m_compilationLogWindowName.c_str() );

        // Debugger
        //-------------------------------------------------------------------------

        ImGui::SetNextWindowClass( pWindowClass );
        DrawDebuggerWindow( context );
    }

    void AnimationGraphWorkspace::DrawWorkspaceToolbarItems( UpdateContext const& context )
    {
        ImVec2 const menuDimensions = ImGui::GetContentRegionMax();
        float buttonDimensions = 130;
        ImGui::SameLine( menuDimensions.x / 2 - buttonDimensions / 2 );

        if ( IsDebugging() )
        {
            if ( ImGuiX::FlatIconButton( EE_ICON_STOP, "Stop Debugging", Colors::Red, ImVec2( buttonDimensions, 0 ) ) )
            {
                StopDebugging();
            }
        }
        else
        {
            if ( ImGuiX::FlatIconButton( EE_ICON_PLAY, "Preview Graph", Colors::Lime, ImVec2( buttonDimensions, 0 ) ) )
            {
                StartDebugging( context, nullptr );
            }
        }

        // Gap
        //-------------------------------------------------------------------------

        constexpr float const gapWidth = 320;
        float const availableX = ImGui::GetContentRegionAvail().x;
        if ( availableX > gapWidth )
        {
            ImGui::Dummy( ImVec2( availableX - gapWidth, 0 ) );
        }

        // Live Game Debugging
        //-------------------------------------------------------------------------

        ImGui::BeginDisabled( !m_pToolsContext->IsGameRunning() || IsDebugging() );

        ImGui::PushStyleColor( ImGuiCol_Text, ImGuiX::ConvertColor( Colors::LimeGreen ).Value );
        bool const drawMenu = ImGui::BeginMenu( EE_ICON_CONNECTION"##DebugTargetsMenu" );
        ImGui::PopStyleColor();

        if ( drawMenu )
        {
            DrawLiveDebugTargetsMenu( context );
            ImGui::EndMenu();
        }
        ImGuiX::ItemTooltip( "Attach to Game" );
        ImGui::EndDisabled();

        // Debug + Preview Options
        //-------------------------------------------------------------------------

        ImGuiX::VerticalSeparator( ImVec2( 20, 0 ) );

        if ( ImGui::BeginMenu( EE_ICON_COG" Debug Options" ) )
        {
            ImGuiX::TextSeparator( "Graph Debug" );

            bool isGraphDebugEnabled = ( m_graphDebugMode == GraphDebugMode::On );
            if ( ImGui::Checkbox( "Enable Graph Debug", &isGraphDebugEnabled ) )
            {
                m_graphDebugMode = isGraphDebugEnabled ? GraphDebugMode::On : GraphDebugMode::Off;
            }

            ImGuiX::TextSeparator( "Root Motion Debug" );

            bool const isRootVisualizationOff = m_rootMotionDebugMode == RootMotionDebugMode::Off;
            if ( ImGui::RadioButton( "No Visualization##Root", isRootVisualizationOff ) )
            {
                m_rootMotionDebugMode = RootMotionDebugMode::Off;
            }

            bool const isRootVisualizationOn = m_rootMotionDebugMode == RootMotionDebugMode::DrawRoot;
            if ( ImGui::RadioButton( "Draw Root", isRootVisualizationOn ) )
            {
                m_rootMotionDebugMode = RootMotionDebugMode::DrawRoot;
            }

            bool const isRootMotionRecordingEnabled = m_rootMotionDebugMode == RootMotionDebugMode::DrawRecordedRootMotion;
            if ( ImGui::RadioButton( "Draw Recorded Root Motion", isRootMotionRecordingEnabled ) )
            {
                m_rootMotionDebugMode = RootMotionDebugMode::DrawRecordedRootMotion;
            }

            bool const isAdvancedRootMotionRecordingEnabled = m_rootMotionDebugMode == RootMotionDebugMode::DrawRecordedRootMotionAdvanced;
            if ( ImGui::RadioButton( "Draw Advanced Recorded Root Motion", isAdvancedRootMotionRecordingEnabled ) )
            {
                m_rootMotionDebugMode = RootMotionDebugMode::DrawRecordedRootMotionAdvanced;
            }

            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "Pose Debug" );

            bool const isVisualizationOff = m_taskSystemDebugMode == TaskSystemDebugMode::Off;
            if ( ImGui::RadioButton( "No Visualization##Tasks", isVisualizationOff ) )
            {
                m_taskSystemDebugMode = TaskSystemDebugMode::Off;
            }

            bool const isFinalPoseEnabled = m_taskSystemDebugMode == TaskSystemDebugMode::FinalPose;
            if ( ImGui::RadioButton( "Final Pose", isFinalPoseEnabled ) )
            {
                m_taskSystemDebugMode = TaskSystemDebugMode::FinalPose;
            }

            bool const isPoseTreeEnabled = m_taskSystemDebugMode == TaskSystemDebugMode::PoseTree;
            if ( ImGui::RadioButton( "Pose Tree", isPoseTreeEnabled ) )
            {
                m_taskSystemDebugMode = TaskSystemDebugMode::PoseTree;
            }

            bool const isDetailedPoseTreeEnabled = m_taskSystemDebugMode == TaskSystemDebugMode::DetailedPoseTree;
            if ( ImGui::RadioButton( "Detailed Pose Tree", isDetailedPoseTreeEnabled ) )
            {
                m_taskSystemDebugMode = TaskSystemDebugMode::DetailedPoseTree;
            }

            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "Physics" );
            ImGui::BeginDisabled( !IsDebugging() );
            if ( m_pPhysicsSystem != nullptr && m_pPhysicsSystem->IsConnectedToPVD() )
            {
                if ( ImGui::Button( "Disconnect From PVD", ImVec2( -1, 0 ) ) )
                {
                    m_pPhysicsSystem->DisconnectFromPVD();
                }
            }
            else
            {
                if ( ImGui::Button( "Connect To PVD", ImVec2( -1, 0 ) ) )
                {
                    m_pPhysicsSystem->ConnectToPVD();
                }
            }
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------

            ImGui::EndMenu();
        }

        // Preview Options
        //-------------------------------------------------------------------------

        ImGuiX::VerticalSeparator( ImVec2( 20, 0 ) );

        if ( ImGui::BeginMenu( EE_ICON_TELEVISION_PLAY" Preview Options" ) )
        {
            ImGuiX::TextSeparator( "Preview Settings" );
            ImGui::Checkbox( "Start Paused", &m_startPaused );

            if ( ImGui::Checkbox( "Camera Follows Character", &m_isCameraTrackingEnabled ) )
            {
                if ( m_isCameraTrackingEnabled && IsDebugging() )
                {
                    CalculateCameraOffset();
                }
            }

            ImGuiX::TextSeparator( "Start Transform" );
            ImGuiX::InputTransform( "StartTransform", m_previewStartTransform, 250.0f );

            if ( ImGui::Button( "Reset Start Transform", ImVec2( -1, 0 ) ) )
            {
                m_previewStartTransform = Transform::Identity;
            }

            ImGui::EndMenu();
        }
    }

    void AnimationGraphWorkspace::DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        TResourceWorkspace<GraphDefinition>::DrawViewportOverlayElements( context, pViewport );

        // Check if we have a target parameter selected
        GraphNodes::TargetControlParameterEditorNode* pSelectedTargetControlParameter = nullptr;
        if ( !m_editorContext.GetSelectedNodes().empty() )
        {
            auto pSelectedNode = m_editorContext.GetSelectedNodes().back().m_pNode;
            pSelectedTargetControlParameter = TryCast<GraphNodes::TargetControlParameterEditorNode>( pSelectedNode );

            // Handle reference nodes
            if ( pSelectedTargetControlParameter == nullptr )
            {
                auto pReferenceNode = TryCast<GraphNodes::ParameterReferenceEditorNode>( pSelectedNode );
                if ( pReferenceNode != nullptr && pReferenceNode->GetParameterValueType() == GraphValueType::Target && pReferenceNode->IsReferencingControlParameter() )
                {
                    pSelectedTargetControlParameter = TryCast<GraphNodes::TargetControlParameterEditorNode>( pReferenceNode->GetReferencedControlParameter() );
                }
            }
        }

        // Allow for in-viewport manipulation of the parameter preview value
        if ( pSelectedTargetControlParameter != nullptr )
        {
            bool drawGizmo = m_gizmo.IsManipulating();
            if ( !m_gizmo.IsManipulating() )
            {
                if ( IsDebugging() )
                {
                    if ( m_pDebugGraphComponent->HasGraphInstance() )
                    {
                        int16_t const parameterIdx = m_pDebugGraphComponent->GetControlParameterIndex( StringID( pSelectedTargetControlParameter->GetParameterName() ) );
                        EE_ASSERT( parameterIdx != InvalidIndex );
                        Target const targetValue = m_pDebugGraphComponent->GetControlParameterValue<Target>( parameterIdx );
                        if ( targetValue.IsTargetSet() && !targetValue.IsBoneTarget() )
                        {
                            m_gizmoTransform = targetValue.GetTransform();
                            drawGizmo = true;
                        }
                    }
                }
                else
                {
                    if ( !pSelectedTargetControlParameter->IsBoneTarget() )
                    {
                        m_gizmoTransform = pSelectedTargetControlParameter->GetPreviewTargetTransform();
                        drawGizmo = true;
                    }
                }
            }

            //-------------------------------------------------------------------------

            if ( drawGizmo )
            {
                auto drawingCtx = GetDrawingContext();

                // Draw scene origin to provide a reference point
                drawingCtx.DrawArrow( Vector( 0, 0, 0.25f ), Vector::Zero, Colors::HotPink, 5.0f );

                //-------------------------------------------------------------------------

                if ( m_isViewportFocused )
                {
                    if ( ImGui::IsKeyPressed( ImGuiKey_Space ) )
                    {
                        m_gizmo.SwitchToNextMode();
                    }
                }

                switch ( m_gizmo.Draw( *pViewport ) )
                {
                    case ImGuiX::Gizmo::Result::StartedManipulating:
                    {
                        if ( IsDebugging() )
                        {
                            // Do Nothing
                        }
                        else
                        {
                            m_editorContext.BeginGraphModification();
                        }
                    }
                    break;

                    case ImGuiX::Gizmo::Result::Manipulating:
                    {
                        // Do Nothing
                    }
                    break;

                    case ImGuiX::Gizmo::Result::StoppedManipulating:
                    {
                        if ( IsDebugging() )
                        {
                            EE_ASSERT( m_pDebugGraphComponent->HasGraphInstance() );
                            int16_t const parameterIdx = m_pDebugGraphComponent->GetControlParameterIndex( StringID( pSelectedTargetControlParameter->GetParameterName() ) );
                            EE_ASSERT( parameterIdx != InvalidIndex );
                            m_pDebugGraphComponent->SetControlParameterValue<Target>( parameterIdx, Target( m_gizmoTransform ) );
                        }
                        else
                        {
                            pSelectedTargetControlParameter->SetPreviewTargetTransform( m_gizmoTransform );
                            m_editorContext.EndGraphModification();
                        }
                    }
                    break;
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    bool AnimationGraphWorkspace::Save()
    {
        EE_ASSERT( m_graphFilePath.IsValid() );
        Serialization::JsonArchiveWriter archive;
        m_editorContext.SaveGraph( *archive.GetWriter() );
        if ( archive.WriteToFile( m_graphFilePath ) )
        {
            EE_ASSERT( m_graphFilePath.IsValid() && m_graphFilePath.MatchesExtension( "ag" ) );

            // Generate the variation descriptors
            auto const& variations = m_editorContext.GetVariationHierarchy();
            for ( auto const& variation : variations.GetAllVariations() )
            {
                GraphVariationResourceDescriptor resourceDesc;
                resourceDesc.m_graphPath = GetResourcePath( m_graphFilePath );
                resourceDesc.m_variationID = variation.m_ID;

                InlineString const variationPathStr = GenerateFilePathForVariation( m_graphFilePath, variation.m_ID );
                FileSystem::Path const variationPath( variationPathStr.c_str() );

                Resource::ResourceDescriptor::TryWriteToFile( *m_pToolsContext->m_pTypeRegistry, variationPath, &resourceDesc );
            }

            m_editorContext.ClearDirty();
            return true;
        }

        return false;
    }

    bool AnimationGraphWorkspace::IsDirty() const
    {
        return m_editorContext.IsDirty();
    }

    void AnimationGraphWorkspace::PreUndoRedo( UndoStack::Operation operation )
    {
        TResourceWorkspace<GraphDefinition>::PreUndoRedo( operation );

        auto pSelectedNode = TryCast<VisualGraph::BaseNode>( m_propertyGrid.GetEditedType() );
        if ( pSelectedNode != nullptr )
        {
            m_selectedNodePreUndoRedo = pSelectedNode->GetID();
        }

        if ( IsDebugging() )
        {
            StopDebugging();
        }
    }

    void AnimationGraphWorkspace::PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        auto pNode = m_editorContext.GetRootGraph()->FindNode( m_selectedNodePreUndoRedo );
        if ( pNode != nullptr )
        {
            m_editorContext.SetSelectedNodes( { VisualGraph::SelectedNode( pNode ) } );
        }

        m_graphEditor.OnUndoRedo();

        TResourceWorkspace<GraphDefinition>::PostUndoRedo( operation, pAction );
    }

    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::UpdateWorld( EntityWorldUpdateContext const& updateContext )
    {
        if ( IsPreviewing() )
        {
            if ( !m_pDebugGraphComponent->IsInitialized() )
            {
                return;
            }

            //-------------------------------------------------------------------------

            if ( m_isFirstPreviewFrame )
            {
                EE_ASSERT( m_pDebugGraphComponent->HasGraphInstance() );

                ReflectInitialPreviewParameterValues( updateContext );
                m_pDebugGraphInstance = m_pDebugGraphComponent->GetDebugGraphInstance();
                m_nodeContext.m_pGraphInstance = m_pDebugGraphInstance;
                m_isFirstPreviewFrame = false;
            }

            //-------------------------------------------------------------------------

            if ( updateContext.GetUpdateStage() == UpdateStage::FrameEnd )
            {
                if ( !updateContext.IsWorldPaused() )
                {
                    Transform const& WT = m_pPreviewEntity->GetWorldTransform();
                    Transform const& RMD = m_pDebugGraphComponent->GetRootMotionDelta();
                    m_characterTransform = RMD * WT;
                    m_pPreviewEntity->SetWorldTransform( m_characterTransform );
                }
            }
        }
        else if ( IsLiveDebugging() )
        {
            // Check if the game we are connected to is still running
            if ( !m_pToolsContext->IsGameRunning() )
            {
                StopDebugging();
                return;
            }

            // Check if the external graph instance is still connected
            if ( m_debugExternalGraphSlotID.IsValid() )
            {
                if ( !m_pDebugGraphComponent->GetDebugGraphInstance()->IsExternalGraphSlotFilled( m_debugExternalGraphSlotID ) )
                {
                    StopDebugging();
                    return;
                }
            }

            m_characterTransform = m_pDebugGraphComponent->GetDebugWorldTransform();

            // Transfer pose to preview entity
            if ( m_pDebugMeshComponent != nullptr && m_pDebugMeshComponent->IsInitialized() )
            {
                m_pDebugMeshComponent->SetPose( m_pDebugGraphComponent->GetPose() );
                m_pDebugMeshComponent->FinalizePose();
                m_pDebugMeshComponent->SetWorldTransform( m_characterTransform );
            }
        }

        //-------------------------------------------------------------------------

        if ( IsDebugging() )
        {
            // Update camera tracking
            // TODO: this is pretty janky so need to try to improve it
            if ( updateContext.GetUpdateStage() == UpdateStage::FrameEnd )
            {
                if ( m_isCameraTrackingEnabled )
                {
                    Transform const currentCameraTransform = GetViewportCameraTransform();
                    Transform const offsetDelta = currentCameraTransform * m_previousCameraTransform.GetInverse();
                    m_cameraOffsetTransform = offsetDelta * m_cameraOffsetTransform;
                    m_previousCameraTransform = m_cameraOffsetTransform * m_characterTransform;
                    SetViewportCameraTransform( m_previousCameraTransform );
                }
            }

            // Reflect debug options
            if ( updateContext.GetUpdateStage() == UpdateStage::FrameEnd )
            {
                auto drawingContext = updateContext.GetDrawingContext();
                m_pDebugGraphComponent->SetGraphDebugMode( m_graphDebugMode );
                m_pDebugGraphComponent->SetRootMotionDebugMode( m_rootMotionDebugMode );
                m_pDebugGraphComponent->SetTaskSystemDebugMode( m_taskSystemDebugMode );
                m_pDebugGraphComponent->DrawDebug( drawingContext );
            }
        }
    }

    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::StartDebugging( UpdateContext const& context, AnimationGraphComponent* pTargetDebugComponent, StringID externalGraphSlotID )
    {
        EE_ASSERT( !IsDebugging() );
        EE_ASSERT( m_pPreviewEntity == nullptr );
        EE_ASSERT( m_pDebugGraphComponent == nullptr && m_pDebugGraphInstance == nullptr );

        bool const isPreviewDebugSessionRequested = pTargetDebugComponent == nullptr;

        // Try to compile the graph
        //-------------------------------------------------------------------------

        GraphDefinitionCompiler definitionCompiler;
        bool const graphCompiledSuccessfully = definitionCompiler.CompileGraph( *m_editorContext.GetGraphDefinition() );
        m_graphCompilationLog.UpdateCompilationResults( definitionCompiler.GetLog() );

        // Compilation failed, stop preview attempt
        if ( !graphCompiledSuccessfully )
        {
            ImGui::SetWindowFocus( m_compilationLogWindowName.c_str() );
            return;
        }

        // Optional: Create preview graph component
        //-------------------------------------------------------------------------

        if ( isPreviewDebugSessionRequested )
        {
            // Save the graph changes
            // Ensure that we save the graph and re-generate the dataset on preview
            Save();

            // Create Preview Graph Component
            InlineString const variationPathStr = GenerateFilePathForVariation( m_graphFilePath, m_selectedVariationID );
            ResourceID const graphVariationResourceID( GetResourcePath( variationPathStr.c_str() ) );

            m_pDebugGraphComponent = EE::New<AnimationGraphComponent>( StringID( "Animation Component" ) );
            m_pDebugGraphComponent->SetGraphVariation( graphVariationResourceID );
            m_pDebugGraphInstance = nullptr; // This will be set later when the component initializes
        }
        else // Use the supplied component
        {
            m_pDebugGraphComponent = pTargetDebugComponent;
            if ( externalGraphSlotID.IsValid() )
            {
                m_debugExternalGraphSlotID = externalGraphSlotID;
                auto pPrimaryInstance = m_pDebugGraphComponent->GetDebugGraphInstance();
                m_pDebugGraphInstance = const_cast<GraphInstance*>( pPrimaryInstance->GetExternalGraphDebugInstance( externalGraphSlotID ) );
                EE_ASSERT( m_pDebugGraphInstance != nullptr );
            }
            else
            {
                m_pDebugGraphInstance = m_pDebugGraphComponent->GetDebugGraphInstance();
            }
        }

        // Try Create Preview Mesh Component
        //-------------------------------------------------------------------------

        auto pVariation = m_editorContext.GetVariation( m_selectedVariationID );
        EE_ASSERT( pVariation != nullptr );
        if ( pVariation->m_pSkeleton.IsValid() )
        {
            // Load resource descriptor for skeleton to get the preview mesh
            FileSystem::Path const resourceDescPath = GetFileSystemPath( pVariation->m_pSkeleton.GetResourcePath() );
            SkeletonResourceDescriptor resourceDesc;
            if ( Resource::ResourceDescriptor::TryReadFromFile( *m_pToolsContext->m_pTypeRegistry, resourceDescPath, resourceDesc ) )
            {
                // Create a preview mesh component
                m_pDebugMeshComponent = EE::New<Render::SkeletalMeshComponent>( StringID( "Mesh Component" ) );
                m_pDebugMeshComponent->SetSkeleton( pVariation->m_pSkeleton.GetResourceID() );
                m_pDebugMeshComponent->SetMesh( resourceDesc.m_previewMesh.GetResourceID() );
            }
        }

        // Create preview entity
        //-------------------------------------------------------------------------

        // Add additional components/systems needed when previewing
        m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
        m_pPreviewEntity->AddComponent( m_pDebugMeshComponent );

        if ( isPreviewDebugSessionRequested )
        {
            m_pPreviewEntity->AddComponent( m_pDebugGraphComponent );
            m_pPreviewEntity->CreateSystem<AnimationSystem>();
        }

        AddEntityToWorld( m_pPreviewEntity );

        // Set up node context
        //-------------------------------------------------------------------------

        m_nodeContext.m_pGraphInstance = m_pDebugGraphInstance;
        m_nodeContext.m_nodeIDtoIndexMap = definitionCompiler.GetIDToIndexMap();

        // Systems
        //-------------------------------------------------------------------------

        EE_ASSERT( m_pPhysicsSystem == nullptr );
        m_pPhysicsSystem = context.GetSystem<Physics::PhysicsSystem>();

        // Set up preview
        //-------------------------------------------------------------------------

        m_isFirstPreviewFrame = true;

        if ( isPreviewDebugSessionRequested )
        {
            SetWorldPaused( m_startPaused );
            m_debugMode = DebugMode::Preview;
            m_characterTransform = m_previewStartTransform;
        }
        else
        {
            m_debugMode = DebugMode::LiveDebug;
            m_characterTransform = m_pDebugGraphComponent->GetDebugWorldTransform();
        }

        if ( m_pDebugMeshComponent != nullptr )
        {
            m_pDebugMeshComponent->SetWorldTransform( m_characterTransform );
        }

        // Adjust Camera
        //-------------------------------------------------------------------------

        OBB const bounds = OBB( m_characterTransform.GetTranslation(), Vector::One );
        m_pCamera->FocusOn( bounds );

        if ( m_isCameraTrackingEnabled )
        {
            CalculateCameraOffset();
        }
    }

    void AnimationGraphWorkspace::StopDebugging()
    {
        EE_ASSERT( m_debugMode != DebugMode::None );

        // Clear physics system
        //-------------------------------------------------------------------------

        EE_ASSERT( m_pPhysicsSystem != nullptr );
        if ( m_pPhysicsSystem->IsConnectedToPVD() )
        {
            m_pPhysicsSystem->DisconnectFromPVD();
        }
        m_pPhysicsSystem = nullptr;

        // Destroy entity and clear debug state
        //-------------------------------------------------------------------------

        EE_ASSERT( m_pPreviewEntity != nullptr );
        DestroyEntityInWorld( m_pPreviewEntity );
        m_pPreviewEntity = nullptr;
        m_pDebugGraphComponent = nullptr;
        m_pDebugMeshComponent = nullptr;
        m_pDebugGraphInstance = nullptr;
        m_debugExternalGraphSlotID.Clear();

        // Reset node context debug info
        //-------------------------------------------------------------------------

        m_nodeContext.m_pGraphInstance = nullptr;
        m_nodeContext.m_nodeIDtoIndexMap.clear();

        // Reset debug mode
        //-------------------------------------------------------------------------

        SetViewportCameraTransform( m_cameraOffsetTransform );
        m_previousCameraTransform = m_cameraOffsetTransform;
        m_debugMode = DebugMode::None;

        ResetCameraView();
    }

    void AnimationGraphWorkspace::ReflectInitialPreviewParameterValues( UpdateContext const& context )
    {
        EE_ASSERT( m_isFirstPreviewFrame && m_pDebugGraphComponent != nullptr && m_pDebugGraphComponent->IsInitialized() );

        for ( auto pControlParameter : m_editorContext.GetControlParameters() )
        {
            int16_t const parameterIdx = m_pDebugGraphComponent->GetControlParameterIndex( StringID( pControlParameter->GetParameterName() ) );

            switch ( pControlParameter->GetValueType() )
            {
                case GraphValueType::Bool:
                {
                    auto pNode = TryCast<GraphNodes::BoolControlParameterEditorNode>( pControlParameter );
                    m_pDebugGraphComponent->SetControlParameterValue( parameterIdx, pNode->GetPreviewStartValue() );
                }
                break;

                case GraphValueType::ID:
                {
                    auto pNode = TryCast<GraphNodes::IDControlParameterEditorNode>( pControlParameter );
                    m_pDebugGraphComponent->SetControlParameterValue( parameterIdx, pNode->GetPreviewStartValue() );
                }
                break;

                case GraphValueType::Int:
                {
                    auto pNode = TryCast<GraphNodes::IntControlParameterEditorNode>( pControlParameter );
                    m_pDebugGraphComponent->SetControlParameterValue( parameterIdx, pNode->GetPreviewStartValue() );
                }
                break;

                case GraphValueType::Float:
                {
                    auto pNode = TryCast<GraphNodes::FloatControlParameterEditorNode>( pControlParameter );
                    m_pDebugGraphComponent->SetControlParameterValue( parameterIdx, pNode->GetPreviewStartValue() );
                }
                break;

                case GraphValueType::Vector:
                {
                    auto pNode = TryCast<GraphNodes::VectorControlParameterEditorNode>( pControlParameter );
                    m_pDebugGraphComponent->SetControlParameterValue( parameterIdx, pNode->GetPreviewStartValue() );
                }
                break;

                case GraphValueType::Target:
                {
                    auto pNode = TryCast<GraphNodes::TargetControlParameterEditorNode>( pControlParameter );
                    m_pDebugGraphComponent->SetControlParameterValue( parameterIdx, pNode->GetPreviewStartValue() );
                }
                break;

                default:
                break;
            }
        }
    }

    void AnimationGraphWorkspace::DrawDebuggerWindow( UpdateContext const& context )
    {
        if ( ImGui::Begin( m_debuggerWindowName.c_str() ) )
        {
            if ( IsDebugging() && m_pDebugGraphComponent->IsInitialized() )
            {
                AnimationDebugView::DrawGraphActiveTasksDebugView( m_pDebugGraphComponent );

                ImGui::NewLine();
                ImGuiX::TextSeparator( "Events" );
                AnimationDebugView::DrawGraphSampledEventsView( m_pDebugGraphComponent );
            }
            else
            {
                ImGui::Text( "Nothing to Debug" );
            }
        }
        ImGui::End();
    }

    void AnimationGraphWorkspace::DrawLiveDebugTargetsMenu( UpdateContext const& context )
    {
        EE_ASSERT( m_pToolsContext->IsGameRunning() );

        auto pGameWorld = m_pToolsContext->GetGameWorld();
        auto pWorldSystem = pGameWorld->GetWorldSystem<AnimationWorldSystem>();
        auto const& graphComponents = pWorldSystem->GetRegisteredGraphComponents();

        bool hasTargets = false;

        for ( AnimationGraphComponent* pGraphComponent : graphComponents )
        {
            // Check main instance
            GraphInstance const* pGraphInstance = pGraphComponent->GetDebugGraphInstance();
            if ( pGraphInstance->GetGraphDefinitionID() == m_pResource.GetResourceID() )
            {
                Entity const* pEntity = pGameWorld->FindEntity( pGraphComponent->GetEntityID() );
                InlineString const targetName( InlineString::CtorSprintf(), "Component: '%s' on Entity: '%s'", pGraphComponent->GetName().c_str(), pEntity->GetName().c_str() );
                if ( ImGui::MenuItem( targetName.c_str() ) )
                {
                    StartDebugging( context, pGraphComponent );
                }

                hasTargets = true;
            }

            // Check external graph instances
            auto const& connectedExternalGraphs = pGraphInstance->GetConnectedExternalGraphsForDebug();
            for ( auto const& connectedExternalGraph : connectedExternalGraphs )
            {
                if ( connectedExternalGraph.m_pInstance->GetGraphDefinitionID() == m_pResource.GetResourceID() )
                {
                    Entity const* pEntity = pGameWorld->FindEntity( pGraphComponent->GetEntityID() );
                    InlineString const targetName( InlineString::CtorSprintf(), "Slot '%s' on Component: '%s' on Entity: '%s'", connectedExternalGraph.m_slotID.c_str(), pGraphComponent->GetName().c_str(), pEntity->GetName().c_str() );
                    if ( ImGui::MenuItem( targetName.c_str() ) )
                    {
                        StartDebugging( context, pGraphComponent, connectedExternalGraph.m_slotID );
                    }

                    hasTargets = true;
                }
            }
        }

        if ( !hasTargets )
        {
            ImGui::Text( "No debug targets" );
        }
    }

    void AnimationGraphWorkspace::CalculateCameraOffset()
    {
        m_previousCameraTransform = GetViewportCameraTransform();
        m_cameraOffsetTransform = m_previousCameraTransform * m_characterTransform.GetInverse();
    }
}