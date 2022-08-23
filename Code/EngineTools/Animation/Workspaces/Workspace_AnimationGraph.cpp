#include "Workspace_AnimationGraph.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Definition.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_Parameters.h"
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
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"
#include "System/FileSystem/FileSystemUtils.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    EE_RESOURCE_WORKSPACE_FACTORY( AnimationGraphWorkspaceFactory, GraphDefinition, AnimationGraphWorkspace );
    EE_RESOURCE_WORKSPACE_FACTORY( AnimationGraphVariationWorkspaceFactory, GraphVariation, AnimationGraphWorkspace );

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

    bool AnimationGraphWorkspace::DebugTarget::IsValid() const
    {
        switch ( m_type )
        {
            case DebugTargetType::MainGraph:
            {
                return m_pComponentToDebug != nullptr;
            }
            break;

            case DebugTargetType::ChildGraph:
            {
                return m_pComponentToDebug != nullptr && m_childGraphID.IsValid();
            }
            break;

            case DebugTargetType::ExternalGraph:
            {
                return m_pComponentToDebug != nullptr && m_externalSlotID.IsValid();
            }
            break;

            default: return true; break;
        }
    }

    //-------------------------------------------------------------------------

    AnimationGraphWorkspace::AnimationGraphWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID )
        : TWorkspace<GraphDefinition>( pToolsContext, pWorld, Variation::GetGraphResourceID( resourceID ), false)
        , m_editorContext( *pToolsContext )
        , m_propertyGrid( m_pToolsContext )
    {
        // Load graph from descriptor
        //-------------------------------------------------------------------------

        m_graphFilePath = GetFileSystemPath( m_pResource.GetResourcePath() );
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
                EE_LOG_ERROR( "Animation", "Graph Editor", "Failed to load graph definition: %s", m_graphFilePath.c_str() );
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

        //-------------------------------------------------------------------------

        auto OnVariationSwitched = [this] ()
        {
            if ( IsDebugging() )
            { 
                StopDebugging();
            }
        };

        m_variationSwitchedEventBindingID = m_editorContext.OnVariationSwitched().Bind( OnVariationSwitched );

        //-------------------------------------------------------------------------

        if ( resourceID.GetResourceTypeID() == GraphVariation::GetStaticResourceTypeID() )
        {
            m_editorContext.TrySetSelectedVariation( Variation::GetVariationNameFromResourceID( resourceID ) );
        }
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

        m_editorContext.OnVariationSwitched().Unbind( m_variationSwitchedEventBindingID );
    }

    bool AnimationGraphWorkspace::IsEditingResource( ResourceID const& resourceID ) const
    {
        ResourceID const actualResourceID = Variation::GetGraphResourceID( resourceID );
        return actualResourceID == m_descriptorID;
    }

    void AnimationGraphWorkspace::Initialize( UpdateContext const& context )
    {
        TWorkspace<GraphDefinition>::Initialize( context );

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

    void AnimationGraphWorkspace::Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused )
    {
        m_nodeContext.m_currentVariationID = m_editorContext.GetSelectedVariationID();
        m_nodeContext.m_pVariationHierarchy = &m_editorContext.GetVariationHierarchy();

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
                auto pReferenceNode = TryCast<GraphNodes::ParameterReferenceToolsNode>( pSelectedNode );
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

        m_compilationLogViewer.UpdateAndDraw( context, &m_nodeContext, pWindowClass, m_compilationLogWindowName.c_str() );

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
                StartDebugging( context, DebugTarget() );
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

        ImGui::BeginDisabled( IsDebugging() );

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
        TWorkspace<GraphDefinition>::DrawViewportOverlayElements( context, pViewport );

        // Check if we have a target parameter selected
        GraphNodes::TargetControlParameterToolsNode* pSelectedTargetControlParameter = nullptr;
        if ( !m_editorContext.GetSelectedNodes().empty() )
        {
            auto pSelectedNode = m_editorContext.GetSelectedNodes().back().m_pNode;
            pSelectedTargetControlParameter = TryCast<GraphNodes::TargetControlParameterToolsNode>( pSelectedNode );

            // Handle reference nodes
            if ( pSelectedTargetControlParameter == nullptr )
            {
                auto pReferenceNode = TryCast<GraphNodes::ParameterReferenceToolsNode>( pSelectedNode );
                if ( pReferenceNode != nullptr && pReferenceNode->GetParameterValueType() == GraphValueType::Target && pReferenceNode->IsReferencingControlParameter() )
                {
                    pSelectedTargetControlParameter = TryCast<GraphNodes::TargetControlParameterToolsNode>( pReferenceNode->GetReferencedControlParameter() );
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
            auto const definitionExtension = GraphDefinition::GetStaticResourceTypeID().ToString();
            EE_ASSERT( m_graphFilePath.IsValid() && m_graphFilePath.MatchesExtension( definitionExtension.c_str() ) );

            // Delete all variation descriptors for this graph
            auto const variationExtension = GraphVariation::GetStaticResourceTypeID().ToString();
            TVector<FileSystem::Path> allVariationFiles;
            if ( FileSystem::GetDirectoryContents( m_graphFilePath.GetParentDirectory(), allVariationFiles, FileSystem::DirectoryReaderOutput::OnlyFiles, FileSystem::DirectoryReaderMode::DontExpand, { variationExtension.c_str() } ) )
            {
                String const variationPathPrefix = Variation::GenerateResourceFilePathPrefix( m_graphFilePath );
                for ( auto const& variationFilePath : allVariationFiles )
                {
                    if ( variationFilePath.GetFullPath().find( variationPathPrefix.c_str() ) != String::npos )
                    {
                        FileSystem::EraseFile( variationFilePath );
                    }
                }
            }

            // Generate the variation descriptors
            auto const& variations = m_editorContext.GetVariationHierarchy();
            for ( auto const& variation : variations.GetAllVariations() )
            {
                GraphVariationResourceDescriptor resourceDesc;
                resourceDesc.m_graphPath = GetResourcePath( m_graphFilePath );
                resourceDesc.m_variationID = variation.m_ID;

                String const variationPathStr = Variation::GenerateResourceFilePath( m_graphFilePath, variation.m_ID );
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
        TWorkspace<GraphDefinition>::PreUndoRedo( operation );

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

        TWorkspace<GraphDefinition>::PostUndoRedo( operation, pAction );
    }

    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::PreUpdateWorld( EntityWorldUpdateContext const& updateContext )
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
            // Check if the entity we are debugging still exists
            auto pDebuggedEntity = m_pToolsContext->TryFindEntityInAllWorlds( m_debuggedEntityID );
            if ( pDebuggedEntity == nullptr )
            {
                StopDebugging();
                return;
            }

            // Check that the component still exists on the entity
            if ( pDebuggedEntity->FindComponent( m_debuggedComponentID ) == nullptr )
            {
                StopDebugging();
                return;
            }

            // Ensure we still have a valid graph instance
            if ( !m_pDebugGraphComponent->HasGraphInstance() )
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
    // Debugging
    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::StartDebugging( UpdateContext const& context, DebugTarget target )
    {
        EE_ASSERT( !IsDebugging() );
        EE_ASSERT( m_pPreviewEntity == nullptr );
        EE_ASSERT( m_pDebugGraphComponent == nullptr && m_pDebugGraphInstance == nullptr );
        EE_ASSERT( target.IsValid() );

        bool const isPreviewDebugSessionRequested = ( target.m_type == DebugTargetType::None );

        // Try to compile the graph
        //-------------------------------------------------------------------------

        GraphDefinitionCompiler definitionCompiler;
        bool const graphCompiledSuccessfully = definitionCompiler.CompileGraph( *m_editorContext.GetGraphDefinition() );
        m_compilationLogViewer.UpdateCompilationResults( definitionCompiler.GetLog() );

        // Compilation failed, stop preview attempt
        if ( !graphCompiledSuccessfully )
        {
            pfd::message( "Compile Error!", "The graph failed to compile! Please check the compilation log for details!", pfd::choice::ok, pfd::icon::error ).result();
            ImGui::SetWindowFocus( m_compilationLogWindowName.c_str() );
            return;
        }

        // Create preview entity
        //-------------------------------------------------------------------------

        m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );

        // Set debug component data
        //-------------------------------------------------------------------------

        // If previewing, create the component
        if ( isPreviewDebugSessionRequested )
        {
            // Save the graph changes
            // Ensure that we save the graph and re-generate the dataset on preview
            Save();

            // Create Preview Graph Component
            String const variationPathStr = Variation::GenerateResourceFilePath( m_graphFilePath, m_editorContext.GetSelectedVariationID() );
            ResourceID const graphVariationResourceID( GetResourcePath( variationPathStr.c_str() ) );

            m_pDebugGraphComponent = EE::New<AnimationGraphComponent>( StringID( "Animation Component" ) );
            m_pDebugGraphComponent->SetGraphVariation( graphVariationResourceID );
            m_pDebugGraphInstance = nullptr; // This will be set later when the component initializes

            m_pPreviewEntity->AddComponent( m_pDebugGraphComponent );
            m_pPreviewEntity->CreateSystem<AnimationSystem>();

            m_debuggedEntityID = m_pPreviewEntity->GetID();
            m_debuggedComponentID = m_pDebugGraphComponent->GetID();
        }
        else // Use the supplied target
        {
            m_pDebugGraphComponent = target.m_pComponentToDebug;
            m_debuggedEntityID = m_pDebugGraphComponent->GetEntityID();
            m_debuggedComponentID = m_pDebugGraphComponent->GetID();

            switch ( target.m_type )
            {
                case DebugTargetType::MainGraph:
                {
                    m_pDebugGraphInstance = m_pDebugGraphComponent->GetDebugGraphInstance();
                }
                break;

                case DebugTargetType::ChildGraph:
                {
                    auto pPrimaryInstance = m_pDebugGraphComponent->GetDebugGraphInstance();
                    m_pDebugGraphInstance = const_cast<GraphInstance*>( pPrimaryInstance->GetChildGraphDebugInstance( target.m_childGraphID ) );
                    EE_ASSERT( m_pDebugGraphInstance != nullptr );
                }
                break;

                case DebugTargetType::ExternalGraph:
                {
                    m_debugExternalGraphSlotID = target.m_externalSlotID;
                    auto pPrimaryInstance = m_pDebugGraphComponent->GetDebugGraphInstance();
                    m_pDebugGraphInstance = const_cast<GraphInstance*>( pPrimaryInstance->GetExternalGraphDebugInstance( m_debugExternalGraphSlotID ) );
                    EE_ASSERT( m_pDebugGraphInstance != nullptr );
                }
                break;

                default: 
                {
                    EE_UNREACHABLE_CODE();
                }
                break;
            }

            // Switch to the correct variation
            StringID const debuggedInstanceVariationID = m_pDebugGraphInstance->GetVariationID();
            if ( m_editorContext.GetSelectedVariationID() != debuggedInstanceVariationID )
            {
                m_editorContext.SetSelectedVariation( debuggedInstanceVariationID );
            }
        }

        // Try Create Preview Mesh Component
        //-------------------------------------------------------------------------

        auto pVariation = m_editorContext.GetVariation( m_editorContext.GetSelectedVariationID() );
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
                m_pPreviewEntity->AddComponent( m_pDebugMeshComponent );
            }
        }

        // Add preview entity to the World
        //-------------------------------------------------------------------------

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
                    auto pNode = TryCast<GraphNodes::BoolControlParameterToolsNode>( pControlParameter );
                    m_pDebugGraphComponent->SetControlParameterValue( parameterIdx, pNode->GetPreviewStartValue() );
                }
                break;

                case GraphValueType::ID:
                {
                    auto pNode = TryCast<GraphNodes::IDControlParameterToolsNode>( pControlParameter );
                    m_pDebugGraphComponent->SetControlParameterValue( parameterIdx, pNode->GetPreviewStartValue() );
                }
                break;

                case GraphValueType::Int:
                {
                    auto pNode = TryCast<GraphNodes::IntControlParameterToolsNode>( pControlParameter );
                    m_pDebugGraphComponent->SetControlParameterValue( parameterIdx, pNode->GetPreviewStartValue() );
                }
                break;

                case GraphValueType::Float:
                {
                    auto pNode = TryCast<GraphNodes::FloatControlParameterToolsNode>( pControlParameter );
                    m_pDebugGraphComponent->SetControlParameterValue( parameterIdx, pNode->GetPreviewStartValue() );
                }
                break;

                case GraphValueType::Vector:
                {
                    auto pNode = TryCast<GraphNodes::VectorControlParameterToolsNode>( pControlParameter );
                    m_pDebugGraphComponent->SetControlParameterValue( parameterIdx, pNode->GetPreviewStartValue() );
                }
                break;

                case GraphValueType::Target:
                {
                    auto pNode = TryCast<GraphNodes::TargetControlParameterToolsNode>( pControlParameter );
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
            if ( IsDebugging() && m_pDebugGraphInstance != nullptr && m_pDebugGraphInstance->IsInitialized() )
            {
                ImGuiX::TextSeparator( "Pose Tasks" );
                AnimationDebugView::DrawGraphActiveTasksDebugView( m_pDebugGraphInstance );

                ImGui::NewLine();
                ImGuiX::TextSeparator( "Root Motion" );
                AnimationDebugView::DrawRootMotionDebugView( m_pDebugGraphInstance );

                ImGui::NewLine();
                ImGuiX::TextSeparator( "Events" );
                AnimationDebugView::DrawGraphSampledEventsView( m_pDebugGraphInstance );
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
        bool hasTargets = false;
        auto const debugWorlds = m_pToolsContext->GetAllWorlds();
        for ( auto pWorld : m_pToolsContext->GetAllWorlds() )
        {
            auto pWorldSystem = pWorld->GetWorldSystem<AnimationWorldSystem>();
            auto const& graphComponents = pWorldSystem->GetRegisteredGraphComponents();

            for ( AnimationGraphComponent* pGraphComponent : graphComponents )
            {
                // Check main instance
                GraphInstance const* pGraphInstance = pGraphComponent->GetDebugGraphInstance();
                if ( pGraphInstance->GetDefinitionResourceID() == m_pResource.GetResourceID() )
                {
                    Entity const* pEntity = pWorld->FindEntity( pGraphComponent->GetEntityID() );
                    InlineString const targetName( InlineString::CtorSprintf(), "Entity: '%s', Component: '%s'", pEntity->GetName().c_str(), pGraphComponent->GetName().c_str() );
                    if ( ImGui::MenuItem( targetName.c_str() ) )
                    {
                        DebugTarget target;
                        target.m_type = DebugTargetType::MainGraph;
                        target.m_pComponentToDebug = pGraphComponent;
                        StartDebugging( context, target );
                    }

                    hasTargets = true;
                }

                // Check child graph instances
                TVector<GraphInstance::DebuggableChildGraph> childGraphs;
                pGraphInstance->GetChildGraphsForDebug( childGraphs );
                for ( auto const& childGraph : childGraphs )
                {
                    if ( childGraph.m_pInstance->GetDefinitionResourceID() == m_pResource.GetResourceID() )
                    {
                        Entity const* pEntity = pWorld->FindEntity( pGraphComponent->GetEntityID() );
                        InlineString const targetName( InlineString::CtorSprintf(), "Entity: '%s', Component: '%s', Path: '%s'", pEntity->GetName().c_str(), pGraphComponent->GetName().c_str(), childGraph.m_pathToInstance.c_str() );
                        if ( ImGui::MenuItem( targetName.c_str() ) )
                        {
                            DebugTarget target;
                            target.m_type = DebugTargetType::ChildGraph;
                            target.m_pComponentToDebug = pGraphComponent;
                            target.m_childGraphID = childGraph.GetID();
                            StartDebugging( context, target );
                        }

                        hasTargets = true;
                    }
                }

                // Check external graph instances
                auto const& externalGraphs = pGraphInstance->GetExternalGraphsForDebug();
                for ( auto const& externalGraph : externalGraphs )
                {
                    if ( externalGraph.m_pInstance->GetDefinitionResourceID() == m_pResource.GetResourceID() )
                    {
                        Entity const* pEntity = pWorld->FindEntity( pGraphComponent->GetEntityID() );
                        InlineString const targetName( InlineString::CtorSprintf(), "Entity: '%s', Component: '%s', Slot: '%s'", pEntity->GetName().c_str(), pGraphComponent->GetName().c_str(), externalGraph.m_slotID.c_str() );
                        if ( ImGui::MenuItem( targetName.c_str() ) )
                        {
                            DebugTarget target;
                            target.m_type = DebugTargetType::ExternalGraph;
                            target.m_pComponentToDebug = pGraphComponent;
                            target.m_externalSlotID = externalGraph.m_slotID;
                            StartDebugging( context, target );
                        }

                        hasTargets = true;
                    }
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