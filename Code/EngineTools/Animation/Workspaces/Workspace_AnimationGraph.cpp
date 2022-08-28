#include "Workspace_AnimationGraph.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_Parameters.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_EntryStates.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_GlobalTransitions.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_DataSlot.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_StateMachine.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_State.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_StateMachineGraph.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationGraph.h"
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"
#include "Engine/Camera/Components/Component_DebugCamera.h"
#include "Engine/Animation/DebugViews/DebugView_Animation.h"
#include "Engine/Animation/Components/Component_AnimationGraph.h"
#include "Engine/Animation/Systems/EntitySystem_Animation.h"
#include "Engine/Animation/Systems/WorldSystem_Animation.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Physics/PhysicsSystem.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "System/FileSystem/FileSystemUtils.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Imgui/ImguiStyle.h"

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
            m_pWorkspace->m_toolsGraph.LoadFromJson( *m_pWorkspace->m_pToolsContext->m_pTypeRegistry, archive.GetDocument() );
        }

        virtual void Redo() override
        {
            Serialization::JsonArchiveReader archive;
            archive.ReadFromString( m_valueAfter.c_str() );
            m_pWorkspace->m_toolsGraph.LoadFromJson( *m_pWorkspace->m_pToolsContext->m_pTypeRegistry, archive.GetDocument() );
        }

        void SerializeBeforeState()
        {
            if ( m_pWorkspace->IsDebugging() )
            {
                m_pWorkspace->StopDebugging();
            }

            Serialization::JsonArchiveWriter archive;
            m_pWorkspace->m_toolsGraph.SaveToJson( *m_pWorkspace->m_pToolsContext->m_pTypeRegistry, *archive.GetWriter() );
            m_valueBefore.resize( archive.GetStringBuffer().GetSize() );
            memcpy( m_valueBefore.data(), archive.GetStringBuffer().GetString(), archive.GetStringBuffer().GetSize() );
        }

        void SerializeAfterState()
        {
            Serialization::JsonArchiveWriter archive;
            m_pWorkspace->m_toolsGraph.SaveToJson( *m_pWorkspace->m_pToolsContext->m_pTypeRegistry, *archive.GetWriter() );
            m_valueAfter.resize( archive.GetStringBuffer().GetSize() );
            memcpy( m_valueAfter.data(), archive.GetStringBuffer().GetString(), archive.GetStringBuffer().GetSize() );
        }

    private:

        AnimationGraphWorkspace*            m_pWorkspace = nullptr;
        String                              m_valueBefore;
        String                              m_valueAfter;
    };

    //-------------------------------------------------------------------------

    class ControlParameterPreviewState
    {
    public:

        ControlParameterPreviewState( GraphNodes::ControlParameterToolsNode* pParameter ) : m_pParameter( pParameter ) { EE_ASSERT( m_pParameter != nullptr ); }
        virtual ~ControlParameterPreviewState() = default;

        virtual void DrawPreviewEditor( ToolsGraphUserContext* pGraphNodeContext ) = 0;

    public:

        GraphNodes::ControlParameterToolsNode*     m_pParameter = nullptr;
    };

    namespace
    {
        struct BoolParameterState : public ControlParameterPreviewState
        {
            using ControlParameterPreviewState::ControlParameterPreviewState;

            virtual void DrawPreviewEditor( ToolsGraphUserContext* pGraphNodeContext ) override
            {
                int16_t const parameterIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( m_pParameter->GetID() );
                EE_ASSERT( parameterIdx != InvalidIndex );

                auto value = pGraphNodeContext->m_pGraphInstance->GetControlParameterValue<bool>( parameterIdx );
                if ( ImGui::Checkbox( "##bp", &value ) )
                {
                    pGraphNodeContext->m_pGraphInstance->SetControlParameterValue( parameterIdx, value );
                }
            }
        };

        struct IntParameterState : public ControlParameterPreviewState
        {
            using ControlParameterPreviewState::ControlParameterPreviewState;

            virtual void DrawPreviewEditor( ToolsGraphUserContext* pGraphNodeContext ) override
            {
                int16_t const parameterIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( m_pParameter->GetID() );
                EE_ASSERT( parameterIdx != InvalidIndex );

                auto value = pGraphNodeContext->m_pGraphInstance->GetControlParameterValue<int32_t>( parameterIdx );
                ImGui::SetNextItemWidth( -1 );
                if ( ImGui::InputInt( "##ip", &value ) )
                {
                    pGraphNodeContext->m_pGraphInstance->SetControlParameterValue( parameterIdx, value );
                }
            }
        };

        struct FloatParameterState : public ControlParameterPreviewState
        {
            FloatParameterState( GraphNodes::ControlParameterToolsNode* pParameter )
                : ControlParameterPreviewState( pParameter )
            {
                auto pFloatParameter = Cast<GraphNodes::FloatControlParameterToolsNode>( pParameter );
                m_min = pFloatParameter->GetPreviewRangeMin();
                m_max = pFloatParameter->GetPreviewRangeMax();
            }

            virtual void DrawPreviewEditor( ToolsGraphUserContext* pGraphNodeContext ) override
            {
                int16_t const parameterIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( m_pParameter->GetID() );
                EE_ASSERT( parameterIdx != InvalidIndex );

                float value = pGraphNodeContext->m_pGraphInstance->GetControlParameterValue<float>( parameterIdx );

                //-------------------------------------------------------------------------

                auto UpdateLimits = [this, pGraphNodeContext, &value, parameterIdx] ()
                {
                    if ( m_min > m_max )
                    {
                        m_max = m_min;
                    }

                    // Clamp the value to the range
                    value = Math::Clamp( value, m_min, m_max );
                    pGraphNodeContext->m_pGraphInstance->SetControlParameterValue( parameterIdx, value );
                };

                //-------------------------------------------------------------------------

                constexpr float const limitWidth = 40;

                ImGui::SetNextItemWidth( limitWidth );
                if ( ImGui::InputFloat( "##min", &m_min, 0.0f, 0.0f, "%.1f", ImGuiInputTextFlags_EnterReturnsTrue ) )
                {
                    UpdateLimits();
                }

                ImGui::SameLine();
                ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x - limitWidth - ImGui::GetStyle().ItemSpacing.x );
                if ( ImGui::SliderFloat( "##fp", &value, m_min, m_max ) )
                {
                    pGraphNodeContext->m_pGraphInstance->SetControlParameterValue( parameterIdx, value );
                }

                ImGui::SameLine();
                ImGui::SetNextItemWidth( limitWidth );
                if ( ImGui::InputFloat( "##max", &m_max, 0.0f, 0.0f, "%.1f", ImGuiInputTextFlags_EnterReturnsTrue ) )
                {
                    UpdateLimits();
                }
            }

        private:

            float m_min = 0.0f;
            float m_max = 1.0f;
        };

        struct VectorParameterState : public ControlParameterPreviewState
        {
            using ControlParameterPreviewState::ControlParameterPreviewState;

            virtual void DrawPreviewEditor( ToolsGraphUserContext* pGraphNodeContext ) override
            {
                int16_t const parameterIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( m_pParameter->GetID() );
                EE_ASSERT( parameterIdx != InvalidIndex );

                Vector value = pGraphNodeContext->m_pGraphInstance->GetControlParameterValue<Vector>( parameterIdx );
                ImGui::SetNextItemWidth( -1 );
                if ( ImGui::InputFloat4( "##vp", &value.m_x ) )
                {
                    pGraphNodeContext->m_pGraphInstance->SetControlParameterValue( parameterIdx, value );
                }
            }
        };

        struct IDParameterState : public ControlParameterPreviewState
        {
            using ControlParameterPreviewState::ControlParameterPreviewState;

            virtual void DrawPreviewEditor( ToolsGraphUserContext* pGraphNodeContext ) override
            {
                int16_t const parameterIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( m_pParameter->GetID() );
                EE_ASSERT( parameterIdx != InvalidIndex );

                auto value = pGraphNodeContext->m_pGraphInstance->GetControlParameterValue<StringID>( parameterIdx );
                if ( value.IsValid() )
                {
                    strncpy_s( m_buffer, 255, value.c_str(), strlen( value.c_str() ) );
                }
                else
                {
                    memset( m_buffer, 0, 255 );
                }

                ImGui::SetNextItemWidth( -1 );
                if ( ImGui::InputText( "##tp", m_buffer, 255, ImGuiInputTextFlags_EnterReturnsTrue ) )
                {
                    pGraphNodeContext->m_pGraphInstance->SetControlParameterValue( parameterIdx, StringID( m_buffer ) );
                }
            }

        private:

            char m_buffer[256];
        };

        struct TargetParameterState : public ControlParameterPreviewState
        {
            using ControlParameterPreviewState::ControlParameterPreviewState;

            virtual void DrawPreviewEditor( ToolsGraphUserContext* pGraphNodeContext ) override
            {
                int16_t const parameterIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( m_pParameter->GetID() );
                EE_ASSERT( parameterIdx != InvalidIndex );

                Target value = pGraphNodeContext->m_pGraphInstance->GetControlParameterValue<Target>( parameterIdx );

                // Reflect actual state
                //-------------------------------------------------------------------------

                m_isSet = value.IsTargetSet();
                m_isBoneTarget = value.IsBoneTarget();

                if ( m_isBoneTarget )
                {
                    StringID const boneID = value.GetBoneID();
                    if ( boneID.IsValid() )
                    {
                        strncpy_s( m_buffer, 255, boneID.c_str(), strlen( boneID.c_str() ) );
                    }
                    else
                    {
                        memset( m_buffer, 0, 255 );
                    }

                    m_useBoneSpaceOffsets = value.IsUsingBoneSpaceOffsets();
                }

                if ( m_isBoneTarget && value.HasOffsets() )
                {
                    m_rotationOffset = value.GetRotationOffset().ToEulerAngles();
                    m_translationOffset = value.GetTranslationOffset().ToFloat3();
                }

                if ( m_isSet && !m_isBoneTarget )
                {
                    m_transform = value.GetTransform();
                }

                //-------------------------------------------------------------------------

                bool updateValue = false;

                //-------------------------------------------------------------------------

                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Is Set:" );
                ImGui::SameLine();

                if ( ImGui::Checkbox( "##IsSet", &m_isSet ) )
                {
                    updateValue = true;
                }

                //-------------------------------------------------------------------------

                if ( m_isSet )
                {
                    ImGui::SameLine( 0, 8 );
                    ImGui::Text( "Is Bone:" );
                    ImGui::SameLine();

                    if ( ImGui::Checkbox( "##IsBone", &m_isBoneTarget ) )
                    {
                        updateValue = true;
                    }

                    if ( m_isBoneTarget )
                    {
                        if ( ImGui::BeginTable( "BoneSettings", 2 ) )
                        {
                            ImGui::TableSetupColumn( "Label", ImGuiTableColumnFlags_WidthFixed, 45 );
                            ImGui::TableSetupColumn( "Editor", ImGuiTableColumnFlags_WidthStretch );

                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::AlignTextToFramePadding();
                            {
                                ImGuiX::ScopedFont sf( ImGuiX::Font::Small );
                                ImGui::Text( "Bone ID" );
                            }

                            ImGui::TableNextColumn();
                            if ( ImGui::InputText( "##tp", m_buffer, 255, ImGuiInputTextFlags_EnterReturnsTrue ) )
                            {
                                updateValue = true;
                            }

                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::AlignTextToFramePadding();
                            {
                                ImGuiX::ScopedFont sf( ImGuiX::Font::Small );
                                ImGui::Text( "Offset R" );
                            }

                            ImGui::TableNextColumn();
                            Float3 degrees = m_rotationOffset.GetAsDegrees();
                            if ( ImGuiX::InputFloat3( "##RotationOffset", degrees ) )
                            {
                                m_rotationOffset = EulerAngles( degrees );
                                updateValue = true;
                            }

                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::AlignTextToFramePadding();

                            {
                                ImGuiX::ScopedFont sf( ImGuiX::Font::Small );
                                ImGui::Text( "Offset T" );
                            }

                            ImGui::TableNextColumn();
                            if ( ImGuiX::InputFloat3( "##TranslationOffset", m_translationOffset ) )
                            {
                                updateValue = true;
                            }

                            ImGui::EndTable();
                        }
                    }
                    else
                    {
                        if ( ImGuiX::InputTransform( "##Transform", m_transform ) )
                        {
                            updateValue = true;
                        }
                    }
                }

                //-------------------------------------------------------------------------

                if ( updateValue )
                {
                    if ( m_isSet )
                    {
                        if ( m_isBoneTarget )
                        {
                            value = Target( StringID( m_buffer ) );

                            if ( !Quaternion( m_rotationOffset ).IsIdentity() || !Vector( m_translationOffset ).IsNearZero3() )
                            {
                                value.SetOffsets( Quaternion( m_rotationOffset ), m_translationOffset, m_useBoneSpaceOffsets );
                            }
                        }
                        else
                        {
                            value = Target( m_transform );
                        }
                    }
                    else
                    {
                        value = Target();
                    }

                    pGraphNodeContext->m_pGraphInstance->SetControlParameterValue( parameterIdx, value );
                }
            }

        private:

            bool            m_isSet = false;
            bool            m_isBoneTarget = false;
            bool            m_useBoneSpaceOffsets = false;
            char            m_buffer[256] = { 0 };
            EulerAngles     m_rotationOffset;
            Float3          m_translationOffset = Float3::Zero;
            Transform       m_transform = Transform::Identity;
        };
    }

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
        , m_propertyGrid( m_pToolsContext )
        , m_resourcePicker( *m_pToolsContext )
    {
        // Get anim graph type info
        //-------------------------------------------------------------------------

        m_registeredNodeTypes = m_pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( GraphNodes::FlowToolsNode::GetStaticTypeID(), false, false, true );

        for ( auto pNodeType : m_registeredNodeTypes )
        {
            auto pDefaultNode = Cast<GraphNodes::FlowToolsNode const>( pNodeType->m_pDefaultInstance );
            if ( pDefaultNode->IsUserCreatable() )
            {
                m_categorizedNodeTypes.AddItem( pDefaultNode->GetCategory(), pDefaultNode->GetTypeName(), pNodeType );
            }
        }

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
            if ( !m_toolsGraph.LoadFromJson( *m_pToolsContext->m_pTypeRegistry, archive.GetDocument() ) )
            {
                EE_LOG_ERROR( "Animation", "Graph Editor", "Failed to load graph definition: %s", m_graphFilePath.c_str() );
            }
        }

        if ( resourceID.GetResourceTypeID() == GraphVariation::GetStaticResourceTypeID() )
        {
            TrySetSelectedVariation( Variation::GetVariationNameFromResourceID( resourceID ) );
        }

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

        m_rootGraphBeginModificationBindingID = VisualGraph::BaseGraph::OnBeginModification().Bind( [this] ( VisualGraph::BaseGraph* pRootGraph ) { OnBeginGraphModification( pRootGraph ); } );
        m_rootGraphEndModificationBindingID = VisualGraph::BaseGraph::OnEndModification().Bind( [this] ( VisualGraph::BaseGraph* pRootGraph ) { OnEndGraphModification( pRootGraph ); } );

        // Set initial graph view
        //-------------------------------------------------------------------------

        NavigateTo( m_toolsGraph.GetRootGraph() );
    }

    AnimationGraphWorkspace::~AnimationGraphWorkspace()
    {
        if ( IsDebugging() )
        {
            StopDebugging();
        }

        // Unbind Events
        //-------------------------------------------------------------------------

        VisualGraph::BaseGraph::OnBeginModification().Unbind( m_rootGraphBeginModificationBindingID );
        VisualGraph::BaseGraph::OnEndModification().Unbind( m_rootGraphEndModificationBindingID );
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

        //-------------------------------------------------------------------------

        InitializePropertyGrid();
        InitializeUserContext();
        InitializeControlParameterEditor();
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

    void AnimationGraphWorkspace::Shutdown( UpdateContext const& context )
    {
        ShutdownControlParameterEditor();
        ShutdownUserContext();
        ShutdownPropertyGrid();

        TWorkspace<GraphDefinition>::Shutdown( context );
    }

    void AnimationGraphWorkspace::Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused )
    {
        UpdateUserContext();
        DrawVariationEditor( context, pWindowClass );
        DrawControlParameterEditor( context, pWindowClass );
        DrawPropertyGrid( context, pWindowClass );
        DrawCompilationLog( context, pWindowClass );
        DrawDebuggerWindow( context, pWindowClass );
        DrawGraphView( context, pWindowClass );
        DrawDialogs( context );
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
        if ( !m_selectedNodes.empty() )
        {
            auto pSelectedNode = m_selectedNodes.back().m_pNode;
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
                            m_toolsGraph.MarkDirty();
                            m_toolsGraph.GetRootGraph()->BeginModification();
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
                            m_toolsGraph.GetRootGraph()->EndModification();
                        }
                    }
                    break;
                }
            }
        }
    }

    void AnimationGraphWorkspace::DrawDialogs( UpdateContext const& context )
    {
        bool isDialogOpen = m_activeOperation != GraphOperationType::None;

        //-------------------------------------------------------------------------

        switch ( m_activeOperation )
        {
            case GraphOperationType::None:
            break;

            case GraphOperationType::RenameParameter:
            {
                ImGui::OpenPopup( "Rename Parameter" );
                ImGui::SetNextWindowSize( ImVec2( 300, -1 ) );
                if ( ImGui::BeginPopupModal( "Rename Parameter", &isDialogOpen, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize ) )
                {
                    DrawRenameParameterDialogWindow();
                    ImGui::EndPopup();
                }
            }
            break;

            case GraphOperationType::DeleteParameter:
            {
                ImGui::OpenPopup( "Delete Parameter" );
                if ( ImGui::BeginPopupModal( "Delete Parameter", &isDialogOpen, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize ) )
                {
                    DrawDeleteParameterDialogWindow();
                    ImGui::EndPopup();
                }
            }
            break;

            case GraphOperationType::CreateVariation:
            {
                ImGui::OpenPopup( "Create Variation" );
                if ( ImGui::BeginPopupModal( "Create Variation", &isDialogOpen, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize ) )
                {
                    DrawCreateVariationDialogWindow();
                    ImGui::EndPopup();
                }
            }

            break;

            case GraphOperationType::RenameVariation:
            {
                ImGui::OpenPopup( "Rename Variation" );
                if ( ImGui::BeginPopupModal( "Rename Variation", &isDialogOpen, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize ) )
                {
                    DrawRenameVariationDialogWindow();
                    ImGui::EndPopup();
                }
            }
            break;

            case GraphOperationType::DeleteVariation:
            {
                ImGui::OpenPopup( "Delete Variation" );
                if ( ImGui::BeginPopupModal( "Delete Variation", &isDialogOpen, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize ) )
                {
                    DrawDeleteVariationDialogWindow();
                    ImGui::EndPopup();
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

        // If the dialog was closed (i.e. operation cancelled)
        if ( !isDialogOpen )
        {
            m_activeOperation = GraphOperationType::None;
        }
    }

    //-------------------------------------------------------------------------

    bool AnimationGraphWorkspace::Save()
    {
        EE_ASSERT( m_graphFilePath.IsValid() );
        Serialization::JsonArchiveWriter archive;
        m_toolsGraph.SaveToJson( *m_pToolsContext->m_pTypeRegistry, *archive.GetWriter() );
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
            auto const& variations = m_toolsGraph.GetVariationHierarchy();
            for ( auto const& variation : variations.GetAllVariations() )
            {
                GraphVariationResourceDescriptor resourceDesc;
                resourceDesc.m_graphPath = GetResourcePath( m_graphFilePath );
                resourceDesc.m_variationID = variation.m_ID;

                String const variationPathStr = Variation::GenerateResourceFilePath( m_graphFilePath, variation.m_ID );
                FileSystem::Path const variationPath( variationPathStr.c_str() );

                Resource::ResourceDescriptor::TryWriteToFile( *m_pToolsContext->m_pTypeRegistry, variationPath, &resourceDesc );
            }

            m_toolsGraph.ClearDirty();
            return true;
        }

        return false;
    }

    bool AnimationGraphWorkspace::IsDirty() const
    {
        return m_toolsGraph.IsDirty();
    }

    void AnimationGraphWorkspace::PreUndoRedo( UndoStack::Operation operation )
    {
        TWorkspace<GraphDefinition>::PreUndoRedo( operation );

        m_pBreadcrumbPopupContext = nullptr;

        // Stop Debugging
        //-------------------------------------------------------------------------

        if ( IsDebugging() )
        {
            StopDebugging();
        }

        // Record selection
        //-------------------------------------------------------------------------

        m_selectedNodesPreUndoRedo = m_selectedNodes;
        m_selectedNodes.clear();
        m_propertyGrid.SetTypeToEdit( nullptr );
    }

    void AnimationGraphWorkspace::PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        auto pRootGraph = m_toolsGraph.GetRootGraph();

        // Restore Selection
        //-------------------------------------------------------------------------

        m_selectedNodes.clear();
        for ( auto const& selectedNode : m_selectedNodesPreUndoRedo )
        {
            auto pSelectedNode = pRootGraph->FindNode( selectedNode.m_nodeID );
            if ( pSelectedNode != nullptr )
            {
                m_selectedNodes.emplace_back( VisualGraph::SelectedNode( pSelectedNode ) );
            }
        }

        // Ensure that we are viewing the correct graph
        //-------------------------------------------------------------------------
        // This is necessary since undo/redo will change all graph ptrs

        auto pFoundGraph = pRootGraph->FindPrimaryGraph( m_primaryViewGraphID );
        if ( pFoundGraph != nullptr )
        {
            if ( m_primaryGraphView.GetViewedGraph() != pFoundGraph )
            {
                m_primaryGraphView.SetGraphToView( &m_userContext, pFoundGraph );
            }

            UpdateSecondaryViewState();
        }
        else
        {
            NavigateTo( pRootGraph );
        }

        //-------------------------------------------------------------------------

        RefreshControlParameterCache();
    
        //-------------------------------------------------------------------------

        TWorkspace<GraphDefinition>::PostUndoRedo( operation, pAction );
    }

    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::PreUpdateWorld( EntityWorldUpdateContext const& updateContext )
    {
        if ( IsPreviewDebugSession() )
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
                m_userContext.m_pGraphInstance = m_pDebugGraphInstance;
                m_isFirstPreviewFrame = false;
            }
        }
        else if ( IsLiveDebugSession() )
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
    // Graph Operations
    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::OnBeginGraphModification( VisualGraph::BaseGraph* pRootGraph )
    {
        if ( pRootGraph == m_toolsGraph.GetRootGraph() )
        {
            EE_ASSERT( m_pActiveUndoableAction == nullptr );

            m_pActiveUndoableAction = EE::New<GraphUndoableAction>( this );
            m_pActiveUndoableAction->SerializeBeforeState();
        }
    }

    void AnimationGraphWorkspace::OnEndGraphModification( VisualGraph::BaseGraph* pRootGraph )
    {
        if ( pRootGraph == m_toolsGraph.GetRootGraph() )
        {
            EE_ASSERT( m_pActiveUndoableAction != nullptr );

            m_pActiveUndoableAction->SerializeAfterState();
            m_undoStack.RegisterAction( m_pActiveUndoableAction );
            m_pActiveUndoableAction = nullptr;
            MarkDirty();
        }
    }

    //-------------------------------------------------------------------------
    // Debugging
    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::DrawDebuggerWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        ImGui::SetNextWindowClass( pWindowClass );
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
        bool const graphCompiledSuccessfully = definitionCompiler.CompileGraph( m_toolsGraph );
        m_compilationLog = definitionCompiler.GetLog();

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
            String const variationPathStr = Variation::GenerateResourceFilePath( m_graphFilePath, m_selectedVariationID );
            ResourceID const graphVariationResourceID( GetResourcePath( variationPathStr.c_str() ) );

            m_pDebugGraphComponent = EE::New<AnimationGraphComponent>( StringID( "Animation Component" ) );
            m_pDebugGraphComponent->ShouldApplyRootMotionToEntity( true );
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
            if ( m_selectedVariationID != debuggedInstanceVariationID )
            {
                SetSelectedVariation( debuggedInstanceVariationID );
            }
        }

        // Try Create Preview Mesh Component
        //-------------------------------------------------------------------------

        auto pVariation = m_toolsGraph.GetVariation( m_selectedVariationID );
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

        m_userContext.m_pGraphInstance = m_pDebugGraphInstance;
        m_userContext.m_nodeIDtoIndexMap = definitionCompiler.GetIDToIndexMap();

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

        if ( !isPreviewDebugSessionRequested )
        {
            OBB const bounds = OBB( m_characterTransform.GetTranslation(), Vector::One );
            m_pCamera->FocusOn( bounds );
        }

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

        m_userContext.m_pGraphInstance = nullptr;
        m_userContext.m_nodeIDtoIndexMap.clear();

        // Reset debug mode
        //-------------------------------------------------------------------------

        if ( IsLiveDebugSession() )
        {
            SetViewportCameraTransform( m_cameraOffsetTransform );
            m_previousCameraTransform = m_cameraOffsetTransform;
            ResetCameraView();
        }

        m_debugMode = DebugMode::None;
    }

    void AnimationGraphWorkspace::ReflectInitialPreviewParameterValues( UpdateContext const& context )
    {
        EE_ASSERT( m_isFirstPreviewFrame && m_pDebugGraphComponent != nullptr && m_pDebugGraphComponent->IsInitialized() );

        auto pRootGraph = m_toolsGraph.GetRootGraph();
        auto const controlParameters = pRootGraph->FindAllNodesOfType<GraphNodes::ControlParameterToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
        for ( auto pControlParameter : controlParameters )
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

    //-------------------------------------------------------------------------
    // Selection
    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::SetSelectedVariation( StringID variationID )
    {
        EE_ASSERT( m_toolsGraph.IsValidVariation( variationID ) );
        if ( m_selectedVariationID != variationID )
        {
            m_selectedVariationID = variationID;

            if ( IsDebugging() )
            {
                StopDebugging();
            }
        }
    }

    void AnimationGraphWorkspace::TrySetSelectedVariation( String const& variationName )
    {
        auto const& varHierarchy = m_toolsGraph.GetVariationHierarchy();
        for ( auto const& variation : varHierarchy.GetAllVariations() )
        {
            int32_t const result = variationName.comparei( variation.m_ID.c_str() );
            if ( result == 0 )
            {
                SetSelectedVariation( variation.m_ID );
                return;
            }
        }
    }

    //-------------------------------------------------------------------------
    // User Context
    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::InitializeUserContext()
    {
        m_userContext.m_pTypeRegistry = m_pToolsContext->m_pTypeRegistry;
        m_userContext.m_pCategorizedNodeTypes = &m_categorizedNodeTypes.GetRootCategory();
        m_userContext.m_pVariationHierarchy = &m_toolsGraph.GetVariationHierarchy();
        m_userContext.m_pControlParameters = &m_controlParameters;
        m_userContext.m_pVirtualParameters = &m_virtualParameters;

        m_resourceOpenRequestEventBindingID = m_userContext.OnRequestOpenResource().Bind( [this] ( ResourceID const& resourceID ) { m_pToolsContext->TryOpenResource( resourceID ); } );
        m_navigateToNodeEventBindingID = m_userContext.OnNavigateToNode().Bind( [this] ( VisualGraph::BaseNode* pNode ) { NavigateTo( pNode ); } );
        m_navigateToGraphEventBindingID = m_userContext.OnNavigateToGraph().Bind( [this] ( VisualGraph::BaseGraph* pGraph ) { NavigateTo( pGraph ); } );
    }

    void AnimationGraphWorkspace::UpdateUserContext()
    {
        m_userContext.m_selectedVariationID = m_selectedVariationID;
    }

    void AnimationGraphWorkspace::ShutdownUserContext()
    {
        m_userContext.OnNavigateToNode().Unbind( m_navigateToNodeEventBindingID );
        m_userContext.OnNavigateToGraph().Unbind( m_navigateToGraphEventBindingID );
        m_userContext.OnRequestOpenResource().Unbind( m_resourceOpenRequestEventBindingID );

        m_userContext.m_pTypeRegistry = nullptr;
        m_userContext.m_pCategorizedNodeTypes = nullptr;
        m_userContext.m_pVariationHierarchy = nullptr;
        m_userContext.m_pControlParameters = nullptr;
        m_userContext.m_pVirtualParameters = nullptr;
    }

    //-------------------------------------------------------------------------
    // Graph View
    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::NavigateTo( VisualGraph::BaseNode* pNode, bool focusViewOnNode )
    {
        EE_ASSERT( pNode != nullptr );

        // Navigate to the appropriate graph
        auto pParentGraph = pNode->GetParentGraph();
        EE_ASSERT( pParentGraph != nullptr );
        NavigateTo( pParentGraph );

        // Select node
        if ( m_primaryGraphView.GetViewedGraph()->FindNode( pNode->GetID() ) )
        {
            m_primaryGraphView.SelectNode( pNode );
            if ( focusViewOnNode )
            {
                m_primaryGraphView.CenterView( pNode );
            }
        }
        else if ( m_secondaryGraphView.GetViewedGraph() != nullptr && m_secondaryGraphView.GetViewedGraph()->FindNode( pNode->GetID() ) )
        {
            m_secondaryGraphView.SelectNode( pNode );
            if ( focusViewOnNode )
            {
                m_secondaryGraphView.CenterView( pNode );
            }
        }
    }

    void AnimationGraphWorkspace::NavigateTo( VisualGraph::BaseGraph* pGraph )
    {
        // If the graph we wish to navigate to is a secondary graph, we need to set the primary view and selection accordingly
        auto pParentNode = pGraph->GetParentNode();
        if ( pParentNode != nullptr && pParentNode->GetSecondaryGraph() == pGraph )
        {
            NavigateTo( pParentNode->GetParentGraph() );
            m_primaryGraphView.SelectNode( pParentNode );
            UpdateSecondaryViewState();
        }
        else // Directly update the primary view
        {
            if ( m_primaryGraphView.GetViewedGraph() != pGraph )
            {
                m_primaryGraphView.SetGraphToView( &m_userContext, pGraph );
                m_primaryViewGraphID = pGraph->GetID();
            }
        }
    }

    void AnimationGraphWorkspace::DrawGraphViewNavigationBar()
    {
        auto pRootGraph = m_toolsGraph.GetRootGraph();

        ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold );

        // Sizes
        //-------------------------------------------------------------------------

        constexpr char const* const pBreadcrumbPopupName = "Breadcrumb";
        ImVec2 const navBarDimensions = ImGui::GetContentRegionAvail();
        constexpr static float const homeButtonWidth = 22;
        constexpr static float const stateMachineNavButtonWidth = 64;

        float const buttonHeight = navBarDimensions.y;
        float const statemachineNavRequiredSpace = m_primaryGraphView.IsViewingStateMachineGraph() ? ( stateMachineNavButtonWidth * 2 ) : 0;
        float const breadcrumbAvailableSpace = navBarDimensions.x - homeButtonWidth - statemachineNavRequiredSpace;

        // Get all entries for breadcrumb
        //-------------------------------------------------------------------------

        TInlineVector<VisualGraph::BaseNode*, 10> pathToRoot;

        auto pGraph = m_primaryGraphView.GetViewedGraph();
        auto pParentNode = pGraph->GetParentNode();
        if ( pParentNode != nullptr )
        {
            while ( pParentNode != nullptr )
            {
                pathToRoot.emplace_back( pParentNode );
                pParentNode = pParentNode->GetParentGraph()->GetParentNode();
            }
        }

        // Draw breadcrumbs
        //-------------------------------------------------------------------------

        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );

        if ( ImGuiX::ColoredButton( Colors::Green, Colors::White, EE_ICON_HOME"##GoHome", ImVec2( homeButtonWidth, buttonHeight ) ) )
        {
            NavigateTo( pRootGraph );
        }
        ImGuiX::ItemTooltip( "Go to root graph" );

        ImGui::SameLine( 0, 0 );
        if ( ImGui::Button( EE_ICON_CHEVRON_RIGHT"##RootBrowser", ImVec2( 0, buttonHeight ) ) )
        {
            m_pBreadcrumbPopupContext = nullptr;
            ImGui::OpenPopup( pBreadcrumbPopupName );
        }

        for ( auto i = (int32_t) pathToRoot.size() - 1; i >= 0; i-- )
        {
            bool drawChevron = true;
            bool drawItem = true;

            auto const pParentState = TryCast<GraphNodes::StateToolsNode>( pathToRoot[i]->GetParentGraph()->GetParentNode() );
            auto const pState = TryCast<GraphNodes::StateToolsNode>( pathToRoot[i] );

            // Hide the item is it is a state machine node whose parent is "state machine state"
            if ( pParentState != nullptr && !pParentState->IsBlendTreeState() )
            {
                drawItem = false;
            }

            // Hide the chevron for state machine states (as we dont want to navigate to the child state machine)
            bool const isStateMachineState = pState != nullptr && !pState->IsBlendTreeState();
            if ( isStateMachineState )
            {
                drawChevron = false;
            }

            // Check if the last graph we are in has child state machines
            if ( drawChevron && i == 0 && !IsOfType<GraphNodes::StateMachineToolsNode>( pathToRoot[i] ) )
            {
                auto const childStateMachines = pathToRoot[i]->GetChildGraph()->FindAllNodesOfType<GraphNodes::StateMachineToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Exact );
                if ( childStateMachines.empty() )
                {
                    drawChevron = false;
                }
            }

            // Draw the item
            if ( drawItem )
            {
                ImGui::SameLine( 0, 0 );
                InlineString const str( InlineString::CtorSprintf(), "%s##%s", pathToRoot[i]->GetName(), pathToRoot[i]->GetID().ToString().c_str() );
                if ( ImGui::Button( str.c_str(), ImVec2( 0, buttonHeight ) ) )
                {
                    if ( isStateMachineState )
                    {
                        auto const childStateMachines = pathToRoot[i]->GetChildGraph()->FindAllNodesOfType<GraphNodes::StateMachineToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Exact );
                        EE_ASSERT( childStateMachines.size() );
                        NavigateTo( childStateMachines[0]->GetChildGraph() );
                    }
                    else
                    {
                        NavigateTo( pathToRoot[i]->GetChildGraph() );
                    }
                }
            }

            // Draw the chevron
            if ( drawChevron )
            {
                ImGui::SameLine( 0, 0 );
                InlineString const separatorStr( InlineString::CtorSprintf(), EE_ICON_CHEVRON_RIGHT"##%s", pathToRoot[i]->GetName(), pathToRoot[i]->GetID().ToString().c_str() );
                if ( ImGui::Button( separatorStr.c_str(), ImVec2( 0, buttonHeight ) ) )
                {
                    m_pBreadcrumbPopupContext = pathToRoot[i];
                    ImGui::OpenPopup( pBreadcrumbPopupName );
                }
            }
        }
        ImGui::PopStyleColor();

        // Draw breadcrumb navigation menu
        //-------------------------------------------------------------------------

        if ( ImGui::BeginPopup( pBreadcrumbPopupName ) )
        {
            // If we navigating in a state machine node, we need to list all states
            auto pSM = TryCast<GraphNodes::StateMachineToolsNode>( m_pBreadcrumbPopupContext );
            if ( pSM )
            {
                auto const childStates = pSM->GetChildGraph()->FindAllNodesOfType<GraphNodes::StateToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
                for ( auto pChildState : childStates )
                {
                    // Ignore Off States
                    if ( pChildState->IsOffState() )
                    {
                        continue;
                    }

                    // Regular States
                    if ( pChildState->IsBlendTreeState() )
                    {
                        InlineString const label( InlineString::CtorSprintf(), EE_ICON_FILE_TREE" %s", pChildState->GetName() );
                        if ( ImGui::MenuItem( label.c_str() ) )
                        {
                            NavigateTo( pChildState->GetChildGraph() );
                            ImGui::CloseCurrentPopup();
                        }
                    }
                    else
                    {
                        InlineString const label( InlineString::CtorSprintf(), EE_ICON_STATE_MACHINE" %s", pChildState->GetName() );
                        if ( ImGui::MenuItem( label.c_str() ) )
                        {
                            auto const childStateMachines = pChildState->GetChildGraph()->FindAllNodesOfType<GraphNodes::StateMachineToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Exact );
                            EE_ASSERT( childStateMachines.size() );
                            NavigateTo( childStateMachines[0]->GetChildGraph() );
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
            }
            else // Just display all state machine nodes in this graph
            {
                auto const pGraphToSearch = ( m_pBreadcrumbPopupContext == nullptr ) ? pRootGraph : m_pBreadcrumbPopupContext->GetChildGraph();
                auto childSMs = pGraphToSearch->FindAllNodesOfType<GraphNodes::StateMachineToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
                for ( auto pChildSM : childSMs )
                {
                    if ( ImGui::MenuItem( pChildSM->GetName() ) )
                    {
                        NavigateTo( pChildSM->GetChildGraph() );
                        ImGui::CloseCurrentPopup();
                    }
                }
            }

            ImGui::EndPopup();
        }

        if ( !ImGui::IsPopupOpen( pBreadcrumbPopupName ) )
        {
            m_pBreadcrumbPopupContext = nullptr;
        }

        // Draw state machine navigation options
        //-------------------------------------------------------------------------

        if ( m_primaryGraphView.IsViewingStateMachineGraph() )
        {
            ImGui::SameLine( navBarDimensions.x - statemachineNavRequiredSpace, 0 );
            ImGui::AlignTextToFramePadding();
            if ( ImGuiX::ColoredButton( Colors::Green, Colors::White, EE_ICON_DOOR_OPEN" Entry", ImVec2( stateMachineNavButtonWidth, buttonHeight ) ) )
            {
                auto pSM = Cast<StateMachineGraph>( m_primaryGraphView.GetViewedGraph() );
                NavigateTo( pSM->GetEntryStateOverrideConduit() );
            }
            ImGuiX::ItemTooltip( "Entry State Overrides" );

            ImGui::SameLine( 0, -1 );
            if ( ImGuiX::ColoredButton( Colors::OrangeRed, Colors::White, EE_ICON_LIGHTNING_BOLT"Global", ImVec2( stateMachineNavButtonWidth, buttonHeight ) ) )
            {
                auto pSM = Cast<StateMachineGraph>( m_primaryGraphView.GetViewedGraph() );
                NavigateTo( pSM->GetGlobalTransitionConduit() );
            }
            ImGuiX::ItemTooltip( "Global Transitions" );
        }
    }

    void AnimationGraphWorkspace::DrawGraphView( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        EE_ASSERT( &m_userContext != nullptr );

        auto pRootGraph = m_toolsGraph.GetRootGraph();

        //-------------------------------------------------------------------------

        bool isGraphViewFocused = false;

        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_graphViewWindowName.c_str() ) )
        {
            auto pTypeRegistry = context.GetSystem<TypeSystem::TypeRegistry>();

            isGraphViewFocused = ImGui::IsWindowFocused( ImGuiFocusedFlags_ChildWindows );

            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 0, 0 ) );

            // Navigation Bar
            //-------------------------------------------------------------------------

            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 2 ) );
            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 1, 1 ) );
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 4, 1 ) );
            if ( ImGui::BeginChild( "NavBar", ImVec2( ImGui::GetContentRegionAvail().x, 24 ), false, ImGuiWindowFlags_AlwaysUseWindowPadding ) )
            {
                DrawGraphViewNavigationBar();
            }
            ImGui::EndChild();
            ImGui::PopStyleVar( 3 );

            // Primary View
            //-------------------------------------------------------------------------

            m_primaryGraphView.UpdateAndDraw( *pTypeRegistry, &m_userContext, m_primaryGraphViewHeight );

            if ( m_primaryGraphView.HasSelectionChangedThisFrame() )
            {
                SetSelectedNodes( m_primaryGraphView.GetSelectedNodes() );
            }

            // Splitter
            //-------------------------------------------------------------------------

            ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 0.0f );
            ImGui::Button( "##GraphViewSplitter", ImVec2( -1, 3 ) );
            ImGui::PopStyleVar();

            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetMouseCursor( ImGuiMouseCursor_ResizeNS );
            }

            if ( ImGui::IsItemActive() )
            {
                m_primaryGraphViewHeight += ImGui::GetIO().MouseDelta.y;
                m_primaryGraphViewHeight = Math::Max( 25.0f, m_primaryGraphViewHeight );
            }

            // SecondaryView
            //-------------------------------------------------------------------------

            UpdateSecondaryViewState();
            m_secondaryGraphView.UpdateAndDraw( *pTypeRegistry, &m_userContext );

            if ( m_secondaryGraphView.HasSelectionChangedThisFrame() )
            {
                SetSelectedNodes( m_secondaryGraphView.GetSelectedNodes() );
            }

            //-------------------------------------------------------------------------

            ImGui::PopStyleVar();
        }
        ImGui::End();
        ImGui::PopStyleVar();

        // Handle Focus and selection 
        //-------------------------------------------------------------------------

        VisualGraph::GraphView* pCurrentlyFocusedGraphView = nullptr;

        // Get the currently focused view
        //-------------------------------------------------------------------------

        if ( m_primaryGraphView.HasFocus() )
        {
            pCurrentlyFocusedGraphView = &m_primaryGraphView;
        }
        else if ( m_secondaryGraphView.HasFocus() )
        {
            pCurrentlyFocusedGraphView = &m_secondaryGraphView;
        }

        // Has the focus within the graph editor tool changed?
        //-------------------------------------------------------------------------

        if ( pCurrentlyFocusedGraphView != nullptr && pCurrentlyFocusedGraphView != m_pFocusedGraphView )
        {
            m_pFocusedGraphView = pCurrentlyFocusedGraphView;
        }
    }

    void AnimationGraphWorkspace::UpdateSecondaryViewState()
    {
        VisualGraph::BaseGraph* pSecondaryGraphToView = nullptr;

        if ( m_primaryGraphView.HasSelectedNodes() )
        {
            auto pSelectedNode = m_primaryGraphView.GetSelectedNodes().back().m_pNode;
            if ( pSelectedNode->HasSecondaryGraph() )
            {
                if ( auto pSecondaryGraph = TryCast<VisualGraph::FlowGraph>( pSelectedNode->GetSecondaryGraph() ) )
                {
                    pSecondaryGraphToView = pSecondaryGraph;
                }
            }
            else if ( auto pParameterReference = TryCast<GraphNodes::ParameterReferenceToolsNode>( pSelectedNode ) )
            {
                if ( auto pVP = pParameterReference->GetReferencedVirtualParameter() )
                {
                    pSecondaryGraphToView = pVP->GetChildGraph();
                }
            }
        }

        if ( m_secondaryGraphView.GetViewedGraph() != pSecondaryGraphToView )
        {
            m_secondaryGraphView.SetGraphToView( &m_userContext, pSecondaryGraphToView );
        }
    }

    //-------------------------------------------------------------------------
    // Property Grid
    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::DrawPropertyGrid( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        if ( m_selectedNodes.empty() )
        {
            m_propertyGrid.SetTypeToEdit( nullptr );
        }
        else
        {
            auto pSelectedNode = m_selectedNodes.back().m_pNode;
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
            if ( !m_selectedNodes.empty() )
            {
                ImGui::Text( "Node: %s", m_selectedNodes.back().m_pNode->GetName() );
            }

            m_propertyGrid.DrawGrid();
        }
        ImGui::End();
    }

    void AnimationGraphWorkspace::InitializePropertyGrid()
    {
        m_preEditEventBindingID = m_propertyGrid.OnPreEdit().Bind( [this] ( PropertyEditInfo const& info ) { m_toolsGraph.GetRootGraph()->BeginModification(); } );
        m_postEditEventBindingID = m_propertyGrid.OnPostEdit().Bind( [this] ( PropertyEditInfo const& info ) { m_toolsGraph.GetRootGraph()->EndModification(); } );

    }

    void AnimationGraphWorkspace::ShutdownPropertyGrid()
    {
        m_propertyGrid.OnPreEdit().Unbind( m_preEditEventBindingID );
        m_propertyGrid.OnPostEdit().Unbind( m_postEditEventBindingID );
    }

    //-------------------------------------------------------------------------
    // Compilation Log
    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::DrawCompilationLog( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        int32_t windowFlags = 0;
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 4 ) );
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_compilationLogWindowName.c_str(), nullptr, windowFlags ) )
        {
            if ( m_compilationLog.empty() )
            {
                ImGui::TextColored( ImGuiX::ConvertColor( Colors::LimeGreen ), EE_ICON_CHECK );
                ImGui::SameLine();
                ImGui::Text( "Graph Compiled Successfully" );
            }
            else
            {
                ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 6 ) );
                if ( ImGui::BeginTable( "Compilation Log Table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2( 0, 0 ) ) )
                {
                    ImGui::TableSetupColumn( "##Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 20 );
                    ImGui::TableSetupColumn( "Message", ImGuiTableColumnFlags_WidthStretch );
                    ImGui::TableSetupScrollFreeze( 0, 1 );

                    //-------------------------------------------------------------------------

                    ImGui::TableHeadersRow();

                    //-------------------------------------------------------------------------

                    ImGuiListClipper clipper;
                    clipper.Begin( (int32_t) m_compilationLog.size() );
                    while ( clipper.Step() )
                    {
                        for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                        {
                            auto const& entry = m_compilationLog[i];

                            ImGui::TableNextRow();

                            //-------------------------------------------------------------------------

                            ImGui::TableSetColumnIndex( 0 );
                            switch ( entry.m_severity )
                            {
                                case Log::Severity::Warning:
                                ImGui::TextColored( Colors::Yellow.ToFloat4(), EE_ICON_ALERT_OCTAGON );
                                break;

                                case Log::Severity::Error:
                                ImGui::TextColored( Colors::Red.ToFloat4(), EE_ICON_ALERT );
                                break;

                                case Log::Severity::Message:
                                ImGui::Text( "Message" );
                                break;
                            }

                            //-------------------------------------------------------------------------

                            ImGui::TableSetColumnIndex( 1 );
                            if ( ImGui::Selectable( entry.m_message.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick ) )
                            {
                                if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                                {
                                    auto pFoundNode = m_toolsGraph.GetRootGraph()->FindNode( entry.m_nodeID );
                                    if ( pFoundNode != nullptr )
                                    {
                                        NavigateTo( pFoundNode );
                                    }
                                }
                            }
                        }
                    }

                    // Auto scroll the table
                    if ( ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
                    {
                        ImGui::SetScrollHereY( 1.0f );
                    }

                    ImGui::EndTable();
                }
                ImGui::PopStyleVar();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    //-------------------------------------------------------------------------
    // Control Parameter Editor
    //-------------------------------------------------------------------------

    bool AnimationGraphWorkspace::DrawControlParameterEditor( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        m_pVirtualParamaterToEdit = nullptr;

        int32_t windowFlags = 0;
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 4 ) );
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_controlParametersWindowName.c_str(), nullptr, windowFlags ) )
        {
            bool const isPreviewing = m_userContext.HasDebugData();
            if ( isPreviewing )
            {
                if ( m_parameterPreviewStates.empty() )
                {
                    CreateControlParameterPreviewStates();
                }

                DrawParameterPreviewControls();
            }
            else
            {
                if ( !m_parameterPreviewStates.empty() )
                {
                    DestroyControlParameterPreviewStates();
                }

                DrawParameterList();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();

        //-------------------------------------------------------------------------

        return m_pVirtualParamaterToEdit != nullptr;
    }

    void AnimationGraphWorkspace::InitializeControlParameterEditor()
    {
        RefreshControlParameterCache();
        m_updateCacheTimer.Start( 0.0f );
    }

    void AnimationGraphWorkspace::ShutdownControlParameterEditor()
    {
        DestroyControlParameterPreviewStates();;
    }

    void AnimationGraphWorkspace::RefreshControlParameterCache()
    {
        m_controlParameters.clear();
        m_virtualParameters.clear();

        auto pRootGraph = m_toolsGraph.GetRootGraph();
        m_controlParameters = pRootGraph->FindAllNodesOfType<GraphNodes::ControlParameterToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
        m_virtualParameters = pRootGraph->FindAllNodesOfType<GraphNodes::VirtualParameterToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Exact );
    }

    void AnimationGraphWorkspace::DrawAddParameterCombo()
    {
        if ( ImGui::Button( "Add New Parameter", ImVec2( -1, 0 ) ) )
        {
            ImGui::OpenPopup( "AddParameterPopup" );
        }

        if ( ImGui::BeginPopup( "AddParameterPopup" ) )
        {
            ImGuiX::TextSeparator( "Control Parameters" );

            if ( ImGui::MenuItem( "Control Parameter - Bool" ) )
            {
                CreateControlParameter( GraphValueType::Bool );
            }

            if ( ImGui::MenuItem( "Control Parameter - ID" ) )
            {
                CreateControlParameter( GraphValueType::ID );
            }

            if ( ImGui::MenuItem( "Control Parameter - Int" ) )
            {
                CreateControlParameter( GraphValueType::Int );
            }

            if ( ImGui::MenuItem( "Control Parameter - Float" ) )
            {
                CreateControlParameter( GraphValueType::Float );
            }

            if ( ImGui::MenuItem( "Control Parameter - Vector" ) )
            {
                CreateControlParameter( GraphValueType::Vector );
            }

            if ( ImGui::MenuItem( "Control Parameter - Target" ) )
            {
                CreateControlParameter( GraphValueType::Target );
            }

            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "Virtual Parameters" );

            if ( ImGui::MenuItem( "Virtual Parameter - Bool" ) )
            {
                CreateVirtualParameter( GraphValueType::Bool );
            }

            if ( ImGui::MenuItem( "Virtual Parameter - ID" ) )
            {
                CreateVirtualParameter( GraphValueType::ID );
            }

            if ( ImGui::MenuItem( "Virtual Parameter - Int" ) )
            {
                CreateVirtualParameter( GraphValueType::Int );
            }

            if ( ImGui::MenuItem( "Virtual Parameter - Float" ) )
            {
                CreateVirtualParameter( GraphValueType::Float );
            }

            if ( ImGui::MenuItem( "Virtual Parameter - Vector" ) )
            {
                CreateVirtualParameter( GraphValueType::Vector );
            }

            if ( ImGui::MenuItem( "Virtual Parameter - Target" ) )
            {
                CreateVirtualParameter( GraphValueType::Target );
            }

            if ( ImGui::MenuItem( "Virtual Parameter - Bone Mask" ) )
            {
                CreateVirtualParameter( GraphValueType::BoneMask );
            }

            ImGui::EndPopup();
        }
    }

    void AnimationGraphWorkspace::DrawRenameParameterDialogWindow()
    {
        EE_ASSERT( m_activeOperation == GraphOperationType::RenameParameter );
        bool updateConfirmed = false;

        ImGui::AlignTextToFramePadding();
        ImGui::Text( "Name: " );
        ImGui::SameLine( 80 );

        if ( ImGui::IsWindowAppearing() )
        {
            ImGui::SetKeyboardFocusHere();
        }

        ImGui::SetNextItemWidth( -1 );
        if ( ImGui::InputText( "##ParameterName", m_parameterNameBuffer, 255, ImGuiInputTextFlags_EnterReturnsTrue ) )
        {
            updateConfirmed = true;
        }

        ImGui::AlignTextToFramePadding();
        ImGui::Text( "Category: " );
        ImGui::SameLine( 80 );

        ImGui::SetNextItemWidth( -1 );
        if ( ImGui::InputText( "##ParameterCategory", m_parameterCategoryBuffer, 255, ImGuiInputTextFlags_EnterReturnsTrue ) )
        {
            updateConfirmed = true;
        }

        ImGui::NewLine();

        float const dialogWidth = ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ).x;
        ImGui::SameLine( 0, dialogWidth - 104 );

        if ( ImGui::Button( "Ok", ImVec2( 50, 0 ) ) || updateConfirmed )
        {
            if ( auto pControlParameter = FindControlParameter( m_currentOperationParameterID ) )
            {
                RenameControlParameter( m_currentOperationParameterID, m_parameterNameBuffer, m_parameterCategoryBuffer );
            }
            else if ( auto pVirtualParameter = FindVirtualParameter( m_currentOperationParameterID ) )
            {
                RenameVirtualParameter( m_currentOperationParameterID, m_parameterNameBuffer, m_parameterCategoryBuffer );
            }
            else
            {
                EE_UNREACHABLE_CODE();
            }

            m_activeOperation = GraphOperationType::None;
        }

        ImGui::SameLine( 0, 4 );

        if ( ImGui::Button( "Cancel", ImVec2( 50, 0 ) ) )
        {
            m_activeOperation = GraphOperationType::None;
        }
    }

    void AnimationGraphWorkspace::DrawDeleteParameterDialogWindow()
    {
        EE_ASSERT( m_activeOperation == GraphOperationType::DeleteParameter );

        auto iter = m_cachedNumUses.find( m_currentOperationParameterID );
        ImGui::Text( "This parameter is used in %d places.", iter != m_cachedNumUses.end() ? iter->second : 0 );
        ImGui::Text( "Are you sure you want to delete this parameter?" );
        ImGui::NewLine();

        float const dialogWidth = ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ).x;
        ImGui::SameLine( 0, dialogWidth - 64 );

        if ( ImGui::Button( "Yes", ImVec2( 30, 0 ) ) )
        {
            if ( auto pControlParameter = FindControlParameter( m_currentOperationParameterID ) )
            {
                DestroyControlParameter( m_currentOperationParameterID );
            }
            else if ( auto pVirtualParameter = FindVirtualParameter( m_currentOperationParameterID ) )
            {
                DestroyVirtualParameter( m_currentOperationParameterID );
            }
            else
            {
                EE_UNREACHABLE_CODE();
            }

            m_activeOperation = GraphOperationType::None;
        }

        ImGui::SameLine( 0, 4 );

        if ( ImGui::Button( "No", ImVec2( 30, 0 ) ) )
        {
            m_activeOperation = GraphOperationType::None;
        }
    }

    void AnimationGraphWorkspace::DrawParameterList()
    {
        auto pRootGraph = m_toolsGraph.GetRootGraph();

        // Create parameter tree
        //-------------------------------------------------------------------------

        CategoryTree<GraphNodes::FlowToolsNode*> parameterTree;

        for ( auto pControlParameter : m_controlParameters)
        {
            parameterTree.AddItem( pControlParameter->GetParameterCategory(), pControlParameter->GetName(), pControlParameter );
        }

        for ( auto pVirtualParameter : m_virtualParameters )
        {
            parameterTree.AddItem( pVirtualParameter->GetParameterCategory(), pVirtualParameter->GetName(), pVirtualParameter );
        }

        // Update references cache
        //-------------------------------------------------------------------------

        if ( m_updateCacheTimer.Update() )
        {
            m_cachedNumUses.clear();

            auto referenceNodes = pRootGraph->FindAllNodesOfType<GraphNodes::ParameterReferenceToolsNode>( VisualGraph::SearchMode::Recursive );
            for ( auto pReferenceNode : referenceNodes )
            {
                auto const& ID = pReferenceNode->GetReferencedParameter()->GetID();
                auto iter = m_cachedNumUses.find( ID );
                if ( iter == m_cachedNumUses.end() )
                {
                    m_cachedNumUses.insert( TPair<UUID, int32_t>( ID, 0 ) );
                    iter = m_cachedNumUses.find( ID );
                }

                iter->second++;
            }

            m_updateCacheTimer.Start( 0.25f );
        }

        // Draw UI
        //-------------------------------------------------------------------------

        DrawAddParameterCombo();

        //-------------------------------------------------------------------------

        constexpr float const iconWidth = 20;

        auto DrawCategoryRow = [] ( String const& categoryName )
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text( categoryName.c_str() );
        };

        auto DrawControlParameterRow = [this, iconWidth] ( GraphNodes::ControlParameterToolsNode* pControlParameter )
        {
            ImGui::PushID( pControlParameter );
            ImGuiX::ScopedFont sf( ImGuiX::Font::Small );

            //-------------------------------------------------------------------------

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::Indent();

            ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) pControlParameter->GetTitleBarColor() );
            ImGui::Text( EE_ICON_ALPHA_C_BOX );
            ImGui::SameLine();
            if ( ImGui::Selectable( pControlParameter->GetName() ) )
            {
                SetSelectedNodes( { VisualGraph::SelectedNode( pControlParameter ) } );
            }
            ImGui::PopStyleColor();

            // Context menu
            if ( ImGui::BeginPopupContextItem( "ParamOptions" ) )
            {
                if ( ImGui::MenuItem( EE_ICON_RENAME_BOX" Rename" ) )
                {
                    StartParameterRename( pControlParameter->GetID() );
                }

                if ( ImGui::MenuItem( EE_ICON_DELETE" Delete" ) )
                {
                    StartParameterDelete( pControlParameter->GetID() );
                }

                ImGui::EndPopup();
            }

            // Drag and drop
            if ( ImGui::BeginDragDropSource() )
            {
                ImGui::SetDragDropPayload( "AnimGraphParameter", pControlParameter->GetName(), strlen( pControlParameter->GetName() ) + 1 );
                ImGui::Text( pControlParameter->GetName() );
                ImGui::EndDragDropSource();
            }
            ImGui::Unindent();

            //-------------------------------------------------------------------------

            ImGui::TableNextColumn();
            ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) pControlParameter->GetTitleBarColor() );
            ImGui::Text( GetNameForValueType( pControlParameter->GetValueType() ) );
            ImGui::PopStyleColor();

            auto iter = m_cachedNumUses.find( pControlParameter->GetID() );
            ImGui::TableNextColumn();
            ImGui::Text( "%d", iter != m_cachedNumUses.end() ? iter->second : 0 );

            ImGui::PopID();
        };

        auto DrawVirtualParameterRow = [this, iconWidth] ( GraphNodes::VirtualParameterToolsNode* pVirtualParameter )
        {
            ImGui::PushID( pVirtualParameter );
            ImGuiX::ScopedFont sf( ImGuiX::Font::Small );

            ImGui::TableNextRow();
            ImGui::TableNextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::Indent();

            ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) pVirtualParameter->GetTitleBarColor() );
            ImGui::Text( EE_ICON_ALPHA_V_BOX );
            ImGui::SameLine();
            ImGui::Selectable( pVirtualParameter->GetName() );
            ImGui::PopStyleColor();

            // Context menu
            if ( ImGui::BeginPopupContextItem( "ParamOptions" ) )
            {
                if ( ImGui::MenuItem( EE_ICON_PLAYLIST_EDIT" Edit" ) )
                {
                    m_pVirtualParamaterToEdit = pVirtualParameter;
                }

                ImGui::Separator();

                if ( ImGui::MenuItem( EE_ICON_RENAME_BOX" Rename" ) )
                {
                    StartParameterRename( pVirtualParameter->GetID() );
                }

                if ( ImGui::MenuItem( EE_ICON_DELETE" Delete" ) )
                {
                    StartParameterDelete( pVirtualParameter->GetID() );
                }

                ImGui::EndPopup();
            }

            // Drag and drop
            if ( ImGui::BeginDragDropSource() )
            {
                ImGui::SetDragDropPayload( "AnimGraphParameter", pVirtualParameter->GetName(), strlen( pVirtualParameter->GetName() ) + 1 );
                ImGui::Text( pVirtualParameter->GetName() );
                ImGui::EndDragDropSource();
            }
            ImGui::Unindent();

            //-------------------------------------------------------------------------

            ImGui::TableNextColumn();
            ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) pVirtualParameter->GetTitleBarColor() );
            ImGui::Text( GetNameForValueType( pVirtualParameter->GetValueType() ) );
            ImGui::PopStyleColor();

            auto iter = m_cachedNumUses.find( pVirtualParameter->GetID() );
            ImGui::TableNextColumn();
            ImGui::Text( "%d", iter != m_cachedNumUses.end() ? iter->second : 0 );

            ImGui::PopID();
        };

        //-------------------------------------------------------------------------

        if ( ImGui::BeginTable( "CPT", 3, 0 ) )
        {
            ImGui::TableSetupColumn( "##Name", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "##Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40 );
            ImGui::TableSetupColumn( "##Uses", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 20 );

            //-------------------------------------------------------------------------

            DrawCategoryRow( "General" );

            for ( auto const& item : parameterTree.GetRootCategory().m_items )
            {
                if ( auto pCP = TryCast<GraphNodes::ControlParameterToolsNode>( item.m_data ) )
                {
                    DrawControlParameterRow( pCP );
                }
                else
                {
                    DrawVirtualParameterRow( TryCast<GraphNodes::VirtualParameterToolsNode>( item.m_data ) );
                }
            }

            //-------------------------------------------------------------------------

            for ( auto const& category : parameterTree.GetRootCategory().m_childCategories )
            {
                DrawCategoryRow( category.m_name );

                for ( auto const& item : category.m_items )
                {
                    if ( auto pCP = TryCast<GraphNodes::ControlParameterToolsNode>( item.m_data ) )
                    {
                        DrawControlParameterRow( pCP );
                    }
                    else
                    {
                        DrawVirtualParameterRow( TryCast<GraphNodes::VirtualParameterToolsNode>( item.m_data ) );
                    }
                }
            }

            ImGui::EndTable();
        }
    }

    void AnimationGraphWorkspace::DrawParameterPreviewControls()
    {
        ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );

        //-------------------------------------------------------------------------

        CategoryTree<ControlParameterPreviewState*> parameterTree;
        for ( auto pPreviewState : m_parameterPreviewStates )
        {
            auto pControlParameter = pPreviewState->m_pParameter;
            parameterTree.AddItem( pControlParameter->GetParameterCategory(), pControlParameter->GetName(), pPreviewState );
        }

        //-------------------------------------------------------------------------

        auto DrawCategoryRow = [] ( String const& categoryName )
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text( categoryName.c_str() );
        };

        auto DrawControlParameterEditorRow = [this] ( ControlParameterPreviewState* pPreviewState )
        {
            auto pControlParameter = pPreviewState->m_pParameter;
            int16_t const parameterIdx = m_userContext.GetRuntimeGraphNodeIndex( pControlParameter->GetID() );

            ImGui::PushID( pControlParameter );
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex( 0 );
            ImGui::AlignTextToFramePadding();
            ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) pControlParameter->GetTitleBarColor() );
            ImGui::Indent();
            ImGui::Text( pControlParameter->GetName() );
            ImGui::Unindent();
            ImGui::PopStyleColor();

            ImGui::TableSetColumnIndex( 1 );
            pPreviewState->DrawPreviewEditor( &m_userContext );
            ImGui::PopID();
        };

        //-------------------------------------------------------------------------

        if ( ImGui::BeginTable( "Parameter Preview", 2, ImGuiTableFlags_Resizable, ImVec2( -1, -1 ) ) )
        {
            ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Editor", ImGuiTableColumnFlags_WidthStretch );

            //-------------------------------------------------------------------------

            DrawCategoryRow( "General" );

            for ( auto const& item : parameterTree.GetRootCategory().m_items )
            {
                DrawControlParameterEditorRow( item.m_data );
            }

            //-------------------------------------------------------------------------

            for ( auto const& category : parameterTree.GetRootCategory().m_childCategories )
            {
                DrawCategoryRow( category.m_name );

                for ( auto const& item : category.m_items )
                {
                    DrawControlParameterEditorRow( item.m_data );
                }
            }

            ImGui::EndTable();
        }
    }

    void AnimationGraphWorkspace::CreateControlParameter( GraphValueType type )
    {
        auto pRootGraph = m_toolsGraph.GetRootGraph();

        //-------------------------------------------------------------------------

        String parameterName = "Parameter";
        EnsureUniqueParameterName( parameterName );

        //-------------------------------------------------------------------------

        GraphNodes::ControlParameterToolsNode* pParameter = nullptr;

        VisualGraph::ScopedGraphModification gm( pRootGraph );

        switch ( type )
        {
            case GraphValueType::Bool:
            pParameter = pRootGraph->CreateNode<GraphNodes::BoolControlParameterToolsNode>( parameterName );
            break;

            case GraphValueType::ID:
            pParameter = pRootGraph->CreateNode<GraphNodes::IDControlParameterToolsNode>( parameterName );
            break;

            case GraphValueType::Int:
            pParameter = pRootGraph->CreateNode<GraphNodes::IntControlParameterToolsNode>( parameterName );
            break;

            case GraphValueType::Float:
            pParameter = pRootGraph->CreateNode<GraphNodes::FloatControlParameterToolsNode>( parameterName );
            break;

            case GraphValueType::Vector:
            pParameter = pRootGraph->CreateNode<GraphNodes::VectorControlParameterToolsNode>( parameterName );
            break;

            case GraphValueType::Target:
            pParameter = pRootGraph->CreateNode<GraphNodes::TargetControlParameterToolsNode>( parameterName );
            break;

            default:
            break;
        }

        EE_ASSERT( pParameter != nullptr );
        m_controlParameters.emplace_back( pParameter );
    }

    void AnimationGraphWorkspace::CreateVirtualParameter( GraphValueType type )
    {
        String parameterName = "Parameter";
        EnsureUniqueParameterName( parameterName );

        auto pRootGraph = m_toolsGraph.GetRootGraph();
        VisualGraph::ScopedGraphModification gm( pRootGraph );
        auto pParameter = pRootGraph->CreateNode<GraphNodes::VirtualParameterToolsNode>( parameterName, type );
        m_virtualParameters.emplace_back( pParameter );
    }

    void AnimationGraphWorkspace::RenameControlParameter( UUID parameterID, String const& newName, String const& category )
    {
        auto pParameter = FindControlParameter( parameterID );
        EE_ASSERT( pParameter != nullptr );

        String uniqueName = newName;
        EnsureUniqueParameterName( uniqueName );

        auto pRootGraph = m_toolsGraph.GetRootGraph();
        VisualGraph::ScopedGraphModification gm( pRootGraph );
        pParameter->Rename( uniqueName, category );
    }

    void AnimationGraphWorkspace::RenameVirtualParameter( UUID parameterID, String const& newName, String const& category )
    {
        auto pParameter = FindVirtualParameter( parameterID );
        EE_ASSERT( pParameter != nullptr );

        String uniqueName = newName;
        EnsureUniqueParameterName( uniqueName );

        auto pRootGraph = m_toolsGraph.GetRootGraph();
        VisualGraph::ScopedGraphModification gm( pRootGraph );
        pParameter->Rename( uniqueName, category );
    }

    void AnimationGraphWorkspace::DestroyControlParameter( UUID parameterID )
    {
        EE_ASSERT( FindControlParameter( parameterID ) != nullptr );

        auto pRootGraph = m_toolsGraph.GetRootGraph();
        VisualGraph::ScopedGraphModification gm( pRootGraph );

        ClearSelection();

        // Find and remove all reference nodes
        auto const parameterReferences = pRootGraph->FindAllNodesOfType<GraphNodes::ParameterReferenceToolsNode>( VisualGraph::SearchMode::Recursive, VisualGraph::SearchTypeMatch::Exact );
        for ( auto const& pFoundParameterNode : parameterReferences )
        {
            if ( pFoundParameterNode->GetReferencedParameterID() == parameterID )
            {
                pFoundParameterNode->Destroy();
            }
        }

        // Delete parameter
        for ( auto iter = m_controlParameters.begin(); iter != m_controlParameters.end(); ++iter )
        {
            auto pParameter = ( *iter );
            if ( pParameter->GetID() == parameterID )
            {
                pParameter->Destroy();
                m_controlParameters.erase( iter );
                break;
            }
        }
    }

    void AnimationGraphWorkspace::DestroyVirtualParameter( UUID parameterID )
    {
        EE_ASSERT( FindVirtualParameter( parameterID ) != nullptr );

        auto pRootGraph = m_toolsGraph.GetRootGraph();
        VisualGraph::ScopedGraphModification gm( pRootGraph );

        ClearSelection();

        // Find and remove all reference nodes
        auto const parameterReferences = pRootGraph->FindAllNodesOfType<GraphNodes::ParameterReferenceToolsNode>( VisualGraph::SearchMode::Recursive, VisualGraph::SearchTypeMatch::Exact );
        for ( auto const& pFoundParameterNode : parameterReferences )
        {
            if ( pFoundParameterNode->GetReferencedParameterID() == parameterID )
            {
                pFoundParameterNode->Destroy();
            }
        }

        // Delete parameter
        for ( auto iter = m_virtualParameters.begin(); iter != m_virtualParameters.end(); ++iter )
        {
            auto pParameter = ( *iter );
            if ( pParameter->GetID() == parameterID )
            {
                pParameter->Destroy();
                m_virtualParameters.erase( iter );
                break;
            }
        }
    }

    void AnimationGraphWorkspace::EnsureUniqueParameterName( String& parameterName ) const
    {
        String tempString = parameterName;
        bool isNameUnique = false;
        int32_t suffix = 0;

        while ( !isNameUnique )
        {
            isNameUnique = true;

            // Check control parameters
            for ( auto pParameter : m_controlParameters )
            {
                if ( pParameter->GetParameterName() == tempString )
                {
                    isNameUnique = false;
                    break;
                }
            }

            // Check virtual parameters
            if ( isNameUnique )
            {
                for ( auto pParameter : m_virtualParameters )
                {
                    if ( pParameter->GetParameterName() == tempString )
                    {
                        isNameUnique = false;
                        break;
                    }
                }
            }

            if ( !isNameUnique )
            {
                tempString.sprintf( "%s_%d", parameterName.c_str(), suffix );
                suffix++;
            }
        }

        //-------------------------------------------------------------------------

        parameterName = tempString;
    }

    GraphNodes::ControlParameterToolsNode* AnimationGraphWorkspace::FindControlParameter( UUID parameterID ) const
    {
        for ( auto pParameter : m_controlParameters )
        {
            if ( pParameter->GetID() == parameterID )
            {
                return pParameter;
            }
        }
        return nullptr;
    }

    GraphNodes::VirtualParameterToolsNode* AnimationGraphWorkspace::FindVirtualParameter( UUID parameterID ) const
    {
        for ( auto pParameter : m_virtualParameters )
        {
            if ( pParameter->GetID() == parameterID )
            {
                return pParameter;
            }
        }
        return nullptr;
    }

    void AnimationGraphWorkspace::CreateControlParameterPreviewStates()
    {
        int32_t const numParameters = (int32_t) m_controlParameters.size();
        for ( int32_t i = 0; i < numParameters; i++ )
        {
            auto pControlParameter = m_controlParameters[i];
            switch ( pControlParameter->GetValueType() )
            {
                case GraphValueType::Bool:
                {
                    m_parameterPreviewStates.emplace_back( EE::New<BoolParameterState>( pControlParameter ) );
                }
                break;

                case GraphValueType::Int:
                {
                    m_parameterPreviewStates.emplace_back( EE::New<IntParameterState>( pControlParameter ) );
                }
                break;

                case GraphValueType::Float:
                {
                    m_parameterPreviewStates.emplace_back( EE::New<FloatParameterState>( pControlParameter ) );
                }
                break;

                case GraphValueType::Vector:
                {
                    m_parameterPreviewStates.emplace_back( EE::New<VectorParameterState>( pControlParameter ) );
                }
                break;

                case GraphValueType::ID:
                {
                    m_parameterPreviewStates.emplace_back( EE::New<IDParameterState>( pControlParameter ) );
                }
                break;

                case GraphValueType::Target:
                {
                    m_parameterPreviewStates.emplace_back( EE::New<TargetParameterState>( pControlParameter ) );
                }
                break;
            }
        }
    }

    void AnimationGraphWorkspace::DestroyControlParameterPreviewStates()
    {
        for ( auto& pPreviewState : m_parameterPreviewStates )
        {
            EE::Delete( pPreviewState );
        }

        m_parameterPreviewStates.clear();
    }

    void AnimationGraphWorkspace::StartParameterRename( UUID const& parameterID )
    {
        EE_ASSERT( parameterID.IsValid() );
        m_currentOperationParameterID = parameterID;
        m_activeOperation = GraphOperationType::RenameParameter;

        if ( auto pControlParameter = FindControlParameter( parameterID ) )
        {
            strncpy_s( m_parameterNameBuffer, pControlParameter->GetName(), 255 );
            strncpy_s( m_parameterCategoryBuffer, pControlParameter->GetParameterCategory().c_str(), 255 );
        }
        else if ( auto pVirtualParameter = FindVirtualParameter( parameterID ) )
        {
            strncpy_s( m_parameterNameBuffer, pVirtualParameter->GetName(), 255 );
            strncpy_s( m_parameterCategoryBuffer, pVirtualParameter->GetParameterCategory().c_str(), 255 );
        }
        else
        {
            EE_UNREACHABLE_CODE();
        }
    }

    void AnimationGraphWorkspace::StartParameterDelete( UUID const& parameterID )
    {
        m_currentOperationParameterID = parameterID;
        m_activeOperation = GraphOperationType::DeleteParameter;
    }

    //-------------------------------------------------------------------------
    // Variation Editor
    //-------------------------------------------------------------------------

    static bool IsValidVariationNameChar( ImWchar c )
    {
        return isalnum( c ) || c == '_';
    }

    static int FilterVariationNameChars( ImGuiInputTextCallbackData* data )
    {
        if ( IsValidVariationNameChar( data->EventChar ) )
        {
            return 0;
        }
        return 1;
    }

    void AnimationGraphWorkspace::DrawVariationEditor( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 4 ) );
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_variationEditorWindowName.c_str(), nullptr, 0 ) )
        {
            DrawVariationSelector();
            DrawOverridesTable();
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void AnimationGraphWorkspace::StartCreate( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() );
        m_activeOperationVariationID = variationID;
        m_activeOperation = GraphOperationType::CreateVariation;
        strncpy_s( m_buffer, "NewChildVariation", 255 );
    }

    void AnimationGraphWorkspace::StartRename( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() );
        m_activeOperationVariationID = variationID;
        m_activeOperation = GraphOperationType::RenameVariation;
        strncpy_s( m_buffer, variationID.c_str(), 255 );
    }

    void AnimationGraphWorkspace::StartDelete( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() );
        m_activeOperationVariationID = variationID;
        m_activeOperation = GraphOperationType::DeleteVariation;
    }

    void AnimationGraphWorkspace::CreateVariation( StringID newVariationID, StringID parentVariationID )
    {
        auto pRootGraph = m_toolsGraph.GetRootGraph();
        auto& variationHierarchy = m_toolsGraph.GetVariationHierarchy();

        EE_ASSERT( !variationHierarchy.IsValidVariation( newVariationID ) );

        // Create new variation
        VisualGraph::ScopedGraphModification gm( pRootGraph );
        variationHierarchy.CreateVariation( newVariationID, parentVariationID );

        // Switch to the new variation
        SetSelectedVariation( newVariationID );
    }

    void AnimationGraphWorkspace::RenameVariation( StringID oldVariationID, StringID newVariationID )
    {
        auto pRootGraph = m_toolsGraph.GetRootGraph();
        auto& variationHierarchy = m_toolsGraph.GetVariationHierarchy();
        bool const isRenamingCurrentlySelectedVariation = ( m_activeOperationVariationID == m_selectedVariationID );

        EE_ASSERT( !variationHierarchy.IsValidVariation( newVariationID ) );

        // Perform rename
        VisualGraph::ScopedGraphModification gm( pRootGraph );
        variationHierarchy.RenameVariation( m_activeOperationVariationID, newVariationID );

        // Update all data slot nodes
        auto dataSlotNodes = pRootGraph->FindAllNodesOfType<GraphNodes::DataSlotToolsNode>( VisualGraph::SearchMode::Recursive, VisualGraph::SearchTypeMatch::Derived );
        for ( auto pDataSlotNode : dataSlotNodes )
        {
            pDataSlotNode->RenameOverride( m_activeOperationVariationID, newVariationID );
        }

        // Set the selected variation to the renamed variation
        if ( isRenamingCurrentlySelectedVariation )
        {
            SetSelectedVariation( newVariationID );
        }
    }

    void AnimationGraphWorkspace::DeleteVariation( StringID variationID )
    {
        auto pRootGraph = m_toolsGraph.GetRootGraph();
        VisualGraph::ScopedGraphModification gm( pRootGraph );

        auto& variationHierarchy = m_toolsGraph.GetVariationHierarchy();
        variationHierarchy.DestroyVariation( m_activeOperationVariationID );
    }

    void AnimationGraphWorkspace::DrawVariationTreeNode( VariationHierarchy const& variationHierarchy, StringID variationID )
    {
        ImGui::PushID( variationID.GetID() );

        auto const childVariations = variationHierarchy.GetChildVariations( variationID );
        bool const isSelected = m_selectedVariationID == variationID;

        // Draw Tree Node
        //-------------------------------------------------------------------------

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if ( childVariations.empty() )
        {
            flags |= ( ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet );
        }

        bool isTreeNodeOpen = false;
        ImGui::SetNextItemOpen( true );
        {
            ImGuiX::ScopedFont const sf( isSelected ? ImGuiX::Font::MediumBold : ImGuiX::Font::Medium, isSelected ? ImGuiX::ConvertColor( Colors::LimeGreen ) : ImGui::GetStyle().Colors[ImGuiCol_Text] );
            isTreeNodeOpen = ImGui::TreeNodeEx( variationID.c_str(), flags );
        }
        ImGuiX::TextTooltip( "Right click for options" );
        if ( ImGui::IsItemClicked() )
        {
            SetSelectedVariation( variationID );
        }

        // Context Menu
        //-------------------------------------------------------------------------

        if ( ImGui::BeginPopupContextItem( variationID.c_str() ) )
        {
            if ( ImGui::MenuItem( "Set Active" ) )
            {
                SetSelectedVariation( variationID );
            }

            //-------------------------------------------------------------------------

            if ( ImGui::MenuItem( "Create Child Variation" ) )
            {
                StartCreate( variationID );
            }

            if ( variationID != Variation::s_defaultVariationID )
            {
                ImGui::Separator();

                if ( ImGui::MenuItem( "Rename" ) )
                {
                    StartRename( variationID );
                }

                if ( ImGui::MenuItem( "Delete" ) )
                {
                    StartDelete( variationID );
                }
            }

            ImGui::EndPopup();
        }

        // Draw node contents
        //-------------------------------------------------------------------------

        if ( isTreeNodeOpen )
        {
            for ( StringID const& childVariationID : childVariations )
            {
                DrawVariationTreeNode( variationHierarchy, childVariationID );
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    void AnimationGraphWorkspace::DrawVariationSelector()
    {
        ImGui::SetNextItemWidth( -1 );
        if ( ImGui::BeginCombo( "##VariationSelector", m_selectedVariationID.c_str() ) )
        {
            auto& variationHierarchy = m_toolsGraph.GetVariationHierarchy();
            DrawVariationTreeNode( variationHierarchy, Variation::s_defaultVariationID );

            ImGui::EndCombo();
        }
        ImGuiX::ItemTooltip( "Variation Selector - Right click on variations for more options!" );
    }

    void AnimationGraphWorkspace::DrawOverridesTable()
    {
        auto pRootGraph = m_toolsGraph.GetRootGraph();
        auto dataSlotNodes = pRootGraph->FindAllNodesOfType<GraphNodes::DataSlotToolsNode>( VisualGraph::SearchMode::Recursive, VisualGraph::SearchTypeMatch::Derived );
        bool isDefaultVariationSelected = IsDefaultVariationSelected();

        //-------------------------------------------------------------------------

        ImGui::AlignTextToFramePadding();
        ImGui::Text( "Skeleton:" );
        ImGui::SameLine( 0, 0 );

        auto pVariation = m_toolsGraph.GetVariation( m_selectedVariationID );
        ResourceID resourceID = pVariation->m_pSkeleton.GetResourceID();
        if ( m_resourcePicker.DrawResourcePicker( Skeleton::GetStaticResourceTypeID(), &resourceID, true ) )
        {
            VisualGraph::ScopedGraphModification sgm( pRootGraph );

            if ( m_resourcePicker.GetSelectedResourceID().IsValid() )
            {
                pVariation->m_pSkeleton = m_resourcePicker.GetSelectedResourceID();
            }
            else
            {
                pVariation->m_pSkeleton.Clear();
            }
        }

        //-------------------------------------------------------------------------

        if ( ImGui::BeginTable( "SourceTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX ) )
        {
            ImGui::TableSetupColumn( "##Override", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 26 );
            ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Path", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Source", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            StringID const currentVariationID = m_selectedVariationID;

            for ( auto pDataSlotNode : dataSlotNodes )
            {
                ImGui::PushID( pDataSlotNode );
                ImGui::TableNextRow();

                // Override Controls
                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();

                if ( !isDefaultVariationSelected )
                {
                    ImVec2 const buttonSize( 26, 24 );

                    if ( pDataSlotNode->HasOverride( currentVariationID ) )
                    {
                        if ( ImGuiX::FlatButtonColored( ImGuiX::ConvertColor( Colors::MediumRed ), EE_ICON_CANCEL, buttonSize ) )
                        {
                            pDataSlotNode->RemoveOverride( currentVariationID );
                        }
                        ImGuiX::ItemTooltip( "Remove Override" );
                    }
                    else // Create an override
                    {
                        if ( ImGuiX::FlatButtonColored( ImGuiX::ConvertColor( Colors::LimeGreen ), EE_ICON_PLUS, buttonSize ) )
                        {
                            pDataSlotNode->CreateOverride( currentVariationID );
                        }
                        ImGuiX::ItemTooltip( "Add Override" );
                    }
                }

                // Get variation override value
                //-------------------------------------------------------------------------
                // This is done here since the controls above might add/remove an override

                bool const hasOverrideForVariation = isDefaultVariationSelected ? false : pDataSlotNode->HasOverride( currentVariationID );
                ResourceID const* pResourceID = hasOverrideForVariation ? pDataSlotNode->GetOverrideValue( currentVariationID ) : nullptr;

                // Node Name
                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();

                ImColor labelColor = ImGuiX::Style::s_colorText;
                if ( !isDefaultVariationSelected && hasOverrideForVariation )
                {
                    labelColor = ( pResourceID->IsValid() ) ? ImGuiX::ConvertColor( Colors::Lime ) : ImGuiX::ConvertColor( Colors::MediumRed );
                }

                ImGui::TextColored( labelColor, pDataSlotNode->GetName() );

                // Node Path
                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                {
                    ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( pDataSlotNode->GetPathFromRoot().c_str() );
                    ImGuiX::TextTooltip( pDataSlotNode->GetPathFromRoot().c_str() );
                }

                // Resource Picker
                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();

                // Default variations always have values created
                if ( isDefaultVariationSelected )
                {
                    if ( m_resourcePicker.DrawResourcePicker( pDataSlotNode->GetSlotResourceTypeID(), &pDataSlotNode->GetDefaultValue(), true ) )
                    {
                        VisualGraph::ScopedGraphModification sgm( pRootGraph );
                        pDataSlotNode->SetDefaultValue( m_resourcePicker.GetSelectedResourceID() );
                    }
                }
                else // Child Variation
                {
                    // If we have an override for this variation
                    if ( hasOverrideForVariation )
                    {
                        EE_ASSERT( pResourceID != nullptr );
                        if ( m_resourcePicker.DrawResourcePicker( pDataSlotNode->GetSlotResourceTypeID(), pResourceID, true ) )
                        {
                            VisualGraph::ScopedGraphModification sgm( pRootGraph );
                            pDataSlotNode->SetOverrideValue( currentVariationID, m_resourcePicker.GetSelectedResourceID() );
                        }
                    }
                    else // Show inherited value
                    {
                        ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );
                        auto& variationHierarchy = m_toolsGraph.GetVariationHierarchy();
                        ResourceID const resolvedResourceID = pDataSlotNode->GetResourceID( variationHierarchy, currentVariationID );
                        ImGui::Text( resolvedResourceID.c_str() );
                        ImGuiX::TextTooltip( resolvedResourceID.c_str() );
                    }
                }

                //-------------------------------------------------------------------------

                ImGui::PopID();
            }

            //-------------------------------------------------------------------------

            ImGui::EndTable();
        }
    }

    bool AnimationGraphWorkspace::DrawVariationNameEditor()
    {
        bool const isRenameOp = ( m_activeOperation == GraphOperationType::RenameVariation );
        bool nameChangeConfirmed = false;

        // Validate current buffer
        //-------------------------------------------------------------------------

        auto ValidateVariationName = [this, isRenameOp] ()
        {
            size_t const bufferLen = strlen( m_buffer );

            if ( bufferLen == 0 )
            {
                return false;
            }

            // Check for invalid chars
            for ( auto i = 0; i < bufferLen; i++ )
            {
                if ( !IsValidVariationNameChar( m_buffer[i] ) )
                {
                    return false;
                }
            }

            // Check for existing variations with the same name but different casing
            String newVariationName( m_buffer );
            auto const& variationHierarchy = m_toolsGraph.GetVariationHierarchy();
            for ( auto const& variation : variationHierarchy.GetAllVariations() )
            {
                int32_t const result = newVariationName.comparei( variation.m_ID.c_str() );
                if ( result == 0 )
                {
                    return false;
                }
            }

            return true;
        };

        // Input Field
        //-------------------------------------------------------------------------

        bool isValidVariationName = ValidateVariationName();
        ImGui::PushStyleColor( ImGuiCol_Text, isValidVariationName ? ImGui::GetStyle().Colors[ImGuiCol_Text] : ImGuiX::ConvertColor( Colors::Red ).Value );
        if ( ImGui::InputText( "##VariationName", m_buffer, 255, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CallbackCharFilter, FilterVariationNameChars ) )
        {
            nameChangeConfirmed = true;
        }
        ImGui::PopStyleColor();
        ImGui::NewLine();

        float const dialogWidth = ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ).x;
        ImGui::SameLine( 0, dialogWidth - 104 );

        // Buttons
        //-------------------------------------------------------------------------

        ImGui::BeginDisabled( !isValidVariationName );
        if ( ImGui::Button( "Ok", ImVec2( 50, 0 ) ) )
        {
            nameChangeConfirmed = true;
        }
        ImGui::EndDisabled();

        ImGui::SameLine( 0, 4 );

        if ( ImGui::Button( "Cancel", ImVec2( 50, 0 ) ) )
        {
            m_activeOperation = GraphOperationType::None;
            nameChangeConfirmed = false;
        }

        // Final validation
        //-------------------------------------------------------------------------

        if ( nameChangeConfirmed )
        {
            if ( !ValidateVariationName() )
            {
                nameChangeConfirmed = false;
            }
        }

        return nameChangeConfirmed;
    }

    void AnimationGraphWorkspace::DrawCreateVariationDialogWindow()
    {
        if ( DrawVariationNameEditor() )
        {
            StringID newVariationID( m_buffer );
            CreateVariation( newVariationID, m_activeOperationVariationID );
            m_activeOperationVariationID = StringID();
            m_activeOperation = GraphOperationType::None;
        }
    }

    void AnimationGraphWorkspace::DrawRenameVariationDialogWindow()
    {
        if ( DrawVariationNameEditor() )
        {
            StringID newVariationID( m_buffer );
            RenameVariation( m_activeOperationVariationID, newVariationID );
            m_activeOperationVariationID = StringID();
            m_activeOperation = GraphOperationType::None;
        }
    }

    void AnimationGraphWorkspace::DrawDeleteVariationDialogWindow()
    {
        ImGui::Text( "Are you sure you want to delete this variation?" );
        ImGui::NewLine();

        float const dialogWidth = ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ).x;
        ImGui::SameLine( 0, dialogWidth - 64 );

        if ( ImGui::Button( "Yes", ImVec2( 30, 0 ) ) )
        {
            EE_ASSERT( m_activeOperationVariationID != Variation::s_defaultVariationID );

            // Update selection
            auto& variationHierarchy = m_toolsGraph.GetVariationHierarchy();
            auto const pVariation = variationHierarchy.GetVariation( m_activeOperationVariationID );
            SetSelectedVariation( pVariation->m_parentID );

            // Destroy variation
            DeleteVariation( m_activeOperationVariationID );
            m_activeOperationVariationID = StringID();
            m_activeOperation = GraphOperationType::None;
        }

        ImGui::SameLine( 0, 4 );

        if ( ImGui::Button( "No", ImVec2( 30, 0 ) ) )
        {
            m_activeOperationVariationID = StringID();
            m_activeOperation = GraphOperationType::None;
        }
    }
}