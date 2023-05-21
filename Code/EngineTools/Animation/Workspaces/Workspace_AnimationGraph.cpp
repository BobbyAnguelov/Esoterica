#include "Workspace_AnimationGraph.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_Parameters.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_EntryStates.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_GlobalTransitions.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_DataSlot.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_StateMachine.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_State.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_ChildGraph.h"
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
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "System/FileSystem/FileSystemUtils.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    EE_RESOURCE_WORKSPACE_FACTORY( AnimationGraphWorkspaceFactory, GraphDefinition, AnimationGraphWorkspace );
    EE_RESOURCE_WORKSPACE_FACTORY( AnimationGraphVariationWorkspaceFactory, GraphVariation, AnimationGraphWorkspace );

    //-------------------------------------------------------------------------

    GraphUndoableAction::GraphUndoableAction( AnimationGraphWorkspace* pWorkspace )
        : m_pWorkspace( pWorkspace )
    {
        EE_ASSERT( m_pWorkspace != nullptr );
    }

    void GraphUndoableAction::Undo()
    {
        Serialization::JsonArchiveReader archive;
        archive.ReadFromString( m_valueBefore.c_str() );
        m_pWorkspace->GetMainGraphData()->m_graphDefinition.LoadFromJson( *m_pWorkspace->m_pToolsContext->m_pTypeRegistry, archive.GetDocument() );
    }

    void GraphUndoableAction::Redo()
    {
        Serialization::JsonArchiveReader archive;
        archive.ReadFromString( m_valueAfter.c_str() );
        m_pWorkspace->GetMainGraphData()->m_graphDefinition.LoadFromJson( *m_pWorkspace->m_pToolsContext->m_pTypeRegistry, archive.GetDocument() );
    }

    void GraphUndoableAction::SerializeBeforeState()
    {
        if ( m_pWorkspace->IsDebugging() )
        {
            m_pWorkspace->StopDebugging();
        }

        Serialization::JsonArchiveWriter archive;
        m_pWorkspace->GetMainGraphData()->m_graphDefinition.SaveToJson( *m_pWorkspace->m_pToolsContext->m_pTypeRegistry, *archive.GetWriter() );
        m_valueBefore.resize( archive.GetStringBuffer().GetSize() );
        memcpy( m_valueBefore.data(), archive.GetStringBuffer().GetString(), archive.GetStringBuffer().GetSize() );
    }

    void GraphUndoableAction::SerializeAfterState()
    {
        Serialization::JsonArchiveWriter archive;
        m_pWorkspace->GetMainGraphData()->m_graphDefinition.SaveToJson( *m_pWorkspace->m_pToolsContext->m_pTypeRegistry, *archive.GetWriter() );
        m_valueAfter.resize( archive.GetStringBuffer().GetSize() );
        memcpy( m_valueAfter.data(), archive.GetStringBuffer().GetString(), archive.GetStringBuffer().GetSize() );
    }

    //-------------------------------------------------------------------------

    class ControlParameterPreviewState
    {
    public:

        ControlParameterPreviewState( GraphNodes::ControlParameterToolsNode* pParameter ) : m_pParameter( pParameter ) { EE_ASSERT( m_pParameter != nullptr ); }
        virtual ~ControlParameterPreviewState() = default;

        virtual void DrawPreviewEditor( UpdateContext const& context, AnimationGraphWorkspace::GraphData* pMainGraphData, Transform const& characterWorldTransform, bool isLiveDebug ) = 0;

    protected:

        inline int16_t GetRuntimeGraphNodeIndex( AnimationGraphWorkspace::GraphData* pMainGraphData, UUID const& nodeID ) const
        {
            auto const foundIter = pMainGraphData->m_nodeIDtoIndexMap.find( nodeID );
            if ( foundIter != pMainGraphData->m_nodeIDtoIndexMap.end() )
            {
                return foundIter->second;
            }

            return InvalidIndex;
        }

    public:

        GraphNodes::ControlParameterToolsNode*     m_pParameter = nullptr;
    };

    namespace
    {
        struct BoolParameterState : public ControlParameterPreviewState
        {
            using ControlParameterPreviewState::ControlParameterPreviewState;

            virtual void DrawPreviewEditor( UpdateContext const& context, AnimationGraphWorkspace::GraphData* pMainGraphData, Transform const& characterWorldTransform, bool isLiveDebug ) override
            {
                int16_t const parameterIdx = GetRuntimeGraphNodeIndex( pMainGraphData, m_pParameter->GetID() );
                EE_ASSERT( parameterIdx != InvalidIndex );

                auto value = pMainGraphData->m_pGraphInstance->GetControlParameterValue<bool>( parameterIdx );
                if ( ImGui::Checkbox( "##bp", &value ) )
                {
                    pMainGraphData->m_pGraphInstance->SetControlParameterValue( parameterIdx, value );
                }
            }
        };

        struct IntParameterState : public ControlParameterPreviewState
        {
            using ControlParameterPreviewState::ControlParameterPreviewState;

            virtual void DrawPreviewEditor( UpdateContext const& context, AnimationGraphWorkspace::GraphData* pMainGraphData, Transform const& characterWorldTransform, bool isLiveDebug ) override
            {
                int16_t const parameterIdx = GetRuntimeGraphNodeIndex( pMainGraphData, m_pParameter->GetID() );
                EE_ASSERT( parameterIdx != InvalidIndex );

                auto value = pMainGraphData->m_pGraphInstance->GetControlParameterValue<int32_t>( parameterIdx );
                ImGui::SetNextItemWidth( -1 );
                if ( ImGui::InputInt( "##ip", &value ) )
                {
                    pMainGraphData->m_pGraphInstance->SetControlParameterValue( parameterIdx, value );
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

            virtual void DrawPreviewEditor( UpdateContext const& context, AnimationGraphWorkspace::GraphData* pMainGraphData, Transform const& characterWorldTransform, bool isLiveDebug ) override
            {
                int16_t const parameterIdx = GetRuntimeGraphNodeIndex( pMainGraphData, m_pParameter->GetID() );
                EE_ASSERT( parameterIdx != InvalidIndex );

                float value = pMainGraphData->m_pGraphInstance->GetControlParameterValue<float>( parameterIdx );

                //-------------------------------------------------------------------------

                auto UpdateLimits = [this, pMainGraphData, &value, parameterIdx] ()
                {
                    if ( m_min > m_max )
                    {
                        m_max = m_min;
                    }

                    // Clamp the value to the range
                    value = Math::Clamp( value, m_min, m_max );
                    pMainGraphData->m_pGraphInstance->SetControlParameterValue( parameterIdx, value );
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
                    pMainGraphData->m_pGraphInstance->SetControlParameterValue( parameterIdx, value );
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

            virtual void DrawPreviewEditor( UpdateContext const& context, AnimationGraphWorkspace::GraphData* pMainGraphData, Transform const& characterWorldTransform, bool isLiveDebug ) override
            {
                int16_t const parameterIdx = GetRuntimeGraphNodeIndex( pMainGraphData, m_pParameter->GetID() );
                EE_ASSERT( parameterIdx != InvalidIndex );

                Float4 basicEditorValue = pMainGraphData->m_pGraphInstance->GetControlParameterValue<Vector>( parameterIdx );

                // Basic Editor
                //-------------------------------------------------------------------------

                ImGui::BeginDisabled( m_showAdvancedEditor );
                ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x - 30 );
                if ( ImGui::InputFloat4( "##vp", &basicEditorValue.m_x ) )
                {
                    pMainGraphData->m_pGraphInstance->SetControlParameterValue( parameterIdx, basicEditorValue );
                }
                ImGui::EndDisabled();

                ImGui::SameLine();

                ImColor const buttonColor = m_showAdvancedEditor ? ImGuiX::ImColors::Green : ImGuiX::Style::s_colorGray1;
                if ( ImGuiX::ColoredButton( buttonColor, ImGuiX::ImColors::White, EE_ICON_COG, ImVec2( 24, 0 ) ) )
                {
                    m_showAdvancedEditor = !m_showAdvancedEditor;
                    m_maxLength = 1.0f;
                    m_selectedStickComboItem = 0;
                    m_convertToCharacterSpace = false;
                    m_isEditingValueWithMouse = false;
                    m_invertStickX = m_invertStickY = false;
                }
                ImGuiX::ItemTooltip( "Advanced Editor" );

                // Advanced Editor
                //-------------------------------------------------------------------------

                if ( m_showAdvancedEditor )
                {
                    Float4 advancedEditorValue = basicEditorValue;

                    if ( m_convertToCharacterSpace )
                    {
                        advancedEditorValue = characterWorldTransform.RotateVector( advancedEditorValue );
                    }

                    //-------------------------------------------------------------------------

                    ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );
                    ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 2, 2 ) );
                    if ( ImGui::BeginChild( "#CW", ImVec2( 0, 100 ), false, ImGuiWindowFlags_AlwaysUseWindowPadding ) )
                    {
                        char const* const comboOptions[] = { "Mouse", "Left Stick", "Right Stick" };
                        constexpr float const labelOffset = 110.0f;
                        constexpr float const widgetOffset = 175;

                        Float2 const previousValueRatio( advancedEditorValue.m_x / m_maxLength, advancedEditorValue.m_y / m_maxLength );

                        ImGui::SameLine( labelOffset );
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text( "XY Radius" );
                        ImGui::SameLine( widgetOffset );
                        ImGui::SetNextItemWidth( -1 );
                        if ( ImGui::InputFloat( "##L", &m_maxLength ) )
                        {
                            if ( m_maxLength <= 0.0 )
                            {
                                m_maxLength = 0.01f;
                            }

                            advancedEditorValue.m_x = ( previousValueRatio.m_x * m_maxLength );
                            advancedEditorValue.m_y = ( previousValueRatio.m_y * m_maxLength );

                            if ( m_convertToCharacterSpace )
                            {
                                Vector const paramValue = characterWorldTransform.InverseRotateVector( advancedEditorValue );
                                pMainGraphData->m_pGraphInstance->SetControlParameterValue( parameterIdx, paramValue );
                            }
                            else
                            {
                                pMainGraphData->m_pGraphInstance->SetControlParameterValue( parameterIdx, advancedEditorValue );
                            }
                        }
                        ImGuiX::ItemTooltip( "Max Length" );

                        ImGui::NewLine();

                        ImGui::SameLine( labelOffset );
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text( "Z" );
                        ImGui::SameLine( widgetOffset );
                        ImGui::SetNextItemWidth( -1 );
                        if ( ImGui::DragFloat( "##Z", &advancedEditorValue.m_z, 0.01f ) )
                        {
                            pMainGraphData->m_pGraphInstance->SetControlParameterValue( parameterIdx, advancedEditorValue );
                        }
                        ImGuiX::ItemTooltip( "Z-Value" );

                        ImGui::NewLine();

                        ImGui::SameLine( labelOffset );
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text( "Control" );
                        ImGui::SameLine( widgetOffset );
                        ImGui::SetNextItemWidth( -1 );
                        ImGui::Combo( "##SC", &m_selectedStickComboItem, comboOptions, 3 );
                        ImGuiX::ItemTooltip( "Control" );

                        ImGui::NewLine();

                        ImGui::SameLine( labelOffset );
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text( "Convert CS" );
                        ImGui::SameLine( widgetOffset );
                        ImGui::Checkbox( "##WS", &m_convertToCharacterSpace );
                        ImGuiX::ItemTooltip( "Convert this (world space) vector to character space" );

                        ImGui::BeginDisabled( m_selectedStickComboItem == 0 );
                        ImGui::SameLine( 0, 10 );
                        ImGui::Text( "Inv. X" );
                        ImGui::SameLine();
                        ImGui::Checkbox( "##ISX", &m_invertStickX );
                        ImGuiX::ItemTooltip( "Invert the stick X axis" );

                        ImGui::SameLine( 0, 10 );
                        ImGui::Text( "Inv. Y" );
                        ImGui::SameLine();
                        ImGui::Checkbox( "##ISY", &m_invertStickY );
                        ImGuiX::ItemTooltip( "Invert the stick Y axis" );
                        ImGui::EndDisabled();

                        //-------------------------------------------------------------------------

                        constexpr float const circleRadius = 45.0f;
                        constexpr float const dotRadius = 5.0f;
                        ImVec2 const circleOrigin = ImGui::GetWindowPos() + ImVec2( circleRadius + 5, circleRadius + 5 );

                        auto pDrawList = ImGui::GetWindowDrawList();
                        pDrawList->AddCircle( circleOrigin, circleRadius, 0xFFFFFFFF, 0, 2.0f );
                        pDrawList->AddCircleFilled( circleOrigin, 1, 0xFFFFFFFF, 0 );

                        // Stick Control
                        //-------------------------------------------------------------------------

                        if ( m_selectedStickComboItem != 0 )
                        {
                            Float2 scaledStickValue( 0, 0 );

                            auto pInputState = context.GetSystem<Input::InputSystem>()->GetControllerState( 0 );
                            if ( m_selectedStickComboItem == 1 )
                            {
                                scaledStickValue = pInputState->GetLeftAnalogStickValue() * m_maxLength;
                            }
                            else // Right stick
                            {
                                scaledStickValue = pInputState->GetRightAnalogStickValue() * m_maxLength;
                            }

                            advancedEditorValue.m_x = scaledStickValue.m_x;
                            advancedEditorValue.m_y = scaledStickValue.m_y;

                            Float4 paramValue = advancedEditorValue;
                            if ( m_invertStickX )
                            {
                                paramValue.m_x = -paramValue.m_x;
                            }

                            if ( m_invertStickY )
                            {
                                paramValue.m_y = -paramValue.m_y;
                            }

                            if ( m_convertToCharacterSpace )
                            {
                                paramValue = characterWorldTransform.InverseRotateVector( paramValue );
                            }

                            pMainGraphData->m_pGraphInstance->SetControlParameterValue( parameterIdx, paramValue );

                            //-------------------------------------------------------------------------

                            float const scale = circleRadius / m_maxLength;
                            ImVec2 const offset( advancedEditorValue.m_x * scale, -advancedEditorValue.m_y * scale );
                            ImVec2 const dotOrigin = circleOrigin + offset;
                            pDrawList->AddCircleFilled( circleOrigin + offset, dotRadius, ImGuiX::ImColors::LimeGreen );
                        }
                        else // Mouse Control
                        {
                            Vector const mousePos = ImGui::GetMousePos();

                            constexpr float const editRegionRadius = 10.0f;
                            float const distanceFromCircleOrigin = mousePos.GetDistance2( circleOrigin );

                            // Manage edit state
                            if ( !m_isEditingValueWithMouse )
                            {
                                // Should we start editing
                                if ( ImGui::IsWindowFocused() && distanceFromCircleOrigin < ( circleRadius + editRegionRadius ) && ImGui::IsMouseDragging( ImGuiMouseButton_Left, 2.0f ) )
                                {
                                    m_isEditingValueWithMouse = true;
                                }
                            }
                            else // Check if we should stop editing
                            {
                                if ( !ImGui::IsMouseDown( ImGuiMouseButton_Left ) )
                                {
                                    m_isEditingValueWithMouse = false;
                                }
                            }

                            // Update the value
                            if ( m_isEditingValueWithMouse )
                            {
                                float const distanceFromCircleOriginPercentage = Math::Min( 1.0f, distanceFromCircleOrigin / circleRadius );
                                Float4 const newValue = ( mousePos - Vector( circleOrigin ) ).GetNormalized2() * ( distanceFromCircleOriginPercentage * m_maxLength );
                                advancedEditorValue.m_x = newValue.m_x;
                                advancedEditorValue.m_y = -newValue.m_y;

                                if ( m_convertToCharacterSpace )
                                {
                                    Vector const paramValue = characterWorldTransform.InverseRotateVector( advancedEditorValue );
                                    pMainGraphData->m_pGraphInstance->SetControlParameterValue( parameterIdx, paramValue );
                                }
                                else
                                {
                                    pMainGraphData->m_pGraphInstance->SetControlParameterValue( parameterIdx, advancedEditorValue );
                                }
                            }

                            // Calculate offset and dot position and draw dot
                            ImVec2 const offset = Vector( advancedEditorValue.m_x, -advancedEditorValue.m_y, 0, 0 ) * ( circleRadius / m_maxLength );
                            ImVec2 const dotOrigin = circleOrigin + offset;
                            pDrawList->AddCircleFilled( circleOrigin + offset, dotRadius, ImGuiX::ToIm( m_isEditingValueWithMouse ? Colors::Yellow : Colors::LimeGreen ) );
                        }
                    }
                    ImGui::EndChild();
                    ImGui::PopStyleVar();
                }
            }

        public:

            bool        m_showAdvancedEditor = false;
            float       m_maxLength = 1.0f;
            int32_t     m_selectedStickComboItem = 0;
            bool        m_convertToCharacterSpace = false;
            bool        m_isEditingValueWithMouse = false;
            bool        m_invertStickX = true;
            bool        m_invertStickY = true;
        };

        struct IDParameterState : public ControlParameterPreviewState
        {
            using ControlParameterPreviewState::ControlParameterPreviewState;

            virtual void DrawPreviewEditor( UpdateContext const& context, AnimationGraphWorkspace::GraphData* pMainGraphData, Transform const& characterWorldTransform, bool isLiveDebug ) override
            {
                int16_t const parameterIdx = GetRuntimeGraphNodeIndex( pMainGraphData, m_pParameter->GetID() );
                EE_ASSERT( parameterIdx != InvalidIndex );

                auto value = pMainGraphData->m_pGraphInstance->GetControlParameterValue<StringID>( parameterIdx );
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
                    pMainGraphData->m_pGraphInstance->SetControlParameterValue( parameterIdx, StringID( m_buffer ) );
                }
            }

        private:

            char m_buffer[256];
        };

        struct TargetParameterState : public ControlParameterPreviewState
        {
            using ControlParameterPreviewState::ControlParameterPreviewState;

            virtual void DrawPreviewEditor( UpdateContext const& context, AnimationGraphWorkspace::GraphData* pMainGraphData, Transform const& characterWorldTransform, bool isLiveDebug ) override
            {
                int16_t const parameterIdx = GetRuntimeGraphNodeIndex( pMainGraphData, m_pParameter->GetID() );
                EE_ASSERT( parameterIdx != InvalidIndex );

                Target value = pMainGraphData->m_pGraphInstance->GetControlParameterValue<Target>( parameterIdx );

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

                    pMainGraphData->m_pGraphInstance->SetControlParameterValue( parameterIdx, value );
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
        : TWorkspace<GraphDefinition>( pToolsContext, pWorld, Variation::GetGraphResourceID( resourceID ), false )
        , m_propertyGrid( m_pToolsContext )
        , m_primaryGraphView( &m_userContext )
        , m_secondaryGraphView( &m_userContext )
        , m_resourcePicker( *m_pToolsContext )
    {
        // Create main graph data storage
        //-------------------------------------------------------------------------

        m_graphStack.emplace_back( EE::New<GraphData>() );

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

        m_graphFilePath = GetFileSystemPath( m_workspaceResource.GetResourcePath() );
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
            if ( !GetMainGraphData()->m_graphDefinition.LoadFromJson( *m_pToolsContext->m_pTypeRegistry, archive.GetDocument() ) )
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

        m_gizmo.SetCoordinateSystemSpace( CoordinateSpace::Local );
        m_gizmo.SetOption( ImGuiX::Gizmo::Options::AllowScale, false );
        m_gizmo.SetOption( ImGuiX::Gizmo::Options::AllowCoordinateSpaceSwitching, false );
        m_gizmo.SwitchMode( ImGuiX::Gizmo::GizmoMode::Translation );

        // Bind events
        //-------------------------------------------------------------------------

        m_rootGraphBeginModificationBindingID = VisualGraph::BaseGraph::OnBeginModification().Bind( [this] ( VisualGraph::BaseGraph* pRootGraph ) { OnBeginGraphModification( pRootGraph ); } );
        m_rootGraphEndModificationBindingID = VisualGraph::BaseGraph::OnEndModification().Bind( [this] ( VisualGraph::BaseGraph* pRootGraph ) { OnEndGraphModification( pRootGraph ); } );

        // Set initial graph view
        //-------------------------------------------------------------------------

        NavigateTo( GetMainGraphData()->m_graphDefinition.GetRootGraph() );
    }

    AnimationGraphWorkspace::~AnimationGraphWorkspace()
    {
        VisualGraph::BaseGraph::OnEndModification().Unbind( m_rootGraphEndModificationBindingID );
        VisualGraph::BaseGraph::OnBeginModification().Unbind( m_rootGraphBeginModificationBindingID );

        EE_ASSERT( m_graphStack.size() == 1 );
        EE::Delete( m_graphStack[0] );
    }

    bool AnimationGraphWorkspace::IsWorkingOnResource( ResourceID const& resourceID ) const
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
        m_graphLogWindowName.sprintf( "Log##%u", GetID() );
        m_debuggerWindowName.sprintf( "Debugger##%u", GetID() );

        //-------------------------------------------------------------------------

        InitializePropertyGrid();
        InitializeUserContext();
        InitializeControlParameterEditor();

        //-------------------------------------------------------------------------

        UpdateUserContext();
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
        ImGui::DockBuilderDockWindow( m_graphLogWindowName.c_str(), bottomLeftDockID );
    }

    void AnimationGraphWorkspace::Shutdown( UpdateContext const& context )
    {
        if ( IsDebugging() )
        {
            StopDebugging();
        }

        EE_ASSERT( !m_previewGraphVariationPtr.IsSet() );

        ClearGraphStack();

        //-------------------------------------------------------------------------

        ShutdownControlParameterEditor();
        ShutdownUserContext();
        ShutdownPropertyGrid();

        //-------------------------------------------------------------------------

        TWorkspace<GraphDefinition>::Shutdown( context );
    }

    void AnimationGraphWorkspace::Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused )
    {
        // Set read-only state
        //-------------------------------------------------------------------------

        bool const isViewReadOnly = IsInReadOnlyState();
        m_propertyGrid.SetReadOnly( isViewReadOnly );
        m_primaryGraphView.SetReadOnly( isViewReadOnly );
        m_secondaryGraphView.SetReadOnly( isViewReadOnly );

        // Global Go-To
        //-------------------------------------------------------------------------

        if ( m_activeOperation == GraphOperationType::None )
        {
            if ( ( ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_G ) ) || ( ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_F ) ) )
            {
                StartNavigationOperation();
            }
        }

        // Draw UI
        //-------------------------------------------------------------------------

        DrawVariationEditor( context, pWindowClass );
        DrawControlParameterEditor( context, pWindowClass );
        DrawPropertyGrid( context, pWindowClass );
        DrawGraphLog( context, pWindowClass );
        DrawDebuggerWindow( context, pWindowClass );
        DrawGraphView( context, pWindowClass );
        DrawDialogs( context );
    }

    void AnimationGraphWorkspace::DrawWorkspaceToolbar( UpdateContext const& context )
    {
        TWorkspace<GraphDefinition>::DrawWorkspaceToolbar( context );

        ImGui::Separator();

        if ( ImGui::MenuItem( EE_ICON_TAB_SEARCH ) )
        {
            StartNavigationOperation();
        }
        ImGuiX::ItemTooltip( "Find/Navigate" );


        if ( IsLiveDebugging() )
        {
            if ( ImGuiX::FlatIconButton( EE_ICON_POWER_PLUG_OFF_OUTLINE, "Stop Live Debug", ImGuiX::ImColors::Red ) )
            {
                StopDebugging();
            }
        }
        else
        {
            ImGui::BeginDisabled( IsDebugging() );
            if ( ImGui::BeginMenu( EE_ICON_POWER_PLUG_OUTLINE" Live Debug" ) )
            {
                DrawLiveDebugTargetsMenu( context );
                ImGui::EndMenu();
            }
            ImGui::EndDisabled();
        }

        //-------------------------------------------------------------------------

        float const gapWidth = 150.0f + ImGui::GetStyle().ItemSpacing.x;
        float const availableX = ImGui::GetContentRegionAvail().x;
        if ( availableX > gapWidth )
        {
            ImGui::Dummy( ImVec2( availableX - gapWidth, 0 ) );
        }

        //-------------------------------------------------------------------------

        DrawVariationSelector( 150 );
    }

    void AnimationGraphWorkspace::DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        TWorkspace<GraphDefinition>::DrawViewportToolbar( context, pViewport );

        // Debug Options
        //-------------------------------------------------------------------------

        auto DrawDebugOptions = [this] ()
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

            ImGuiX::TextSeparator( "Capsule Debug" );

            ImGui::Checkbox( "Show Preview Capsule", &m_showPreviewCapsule );

            ImGui::Text( "Half-Height" );
            ImGui::SameLine( 90 );
            if ( ImGui::InputFloat( "##HH", &m_previewCapsuleHalfHeight, 0.05f ) )
            {
                m_previewCapsuleHalfHeight = Math::Clamp( m_previewCapsuleHalfHeight, 0.05f, 10.0f );
            }

            ImGui::Text( "Radius" );
            ImGui::SameLine( 90 );
            if ( ImGui::InputFloat( "##R", &m_previewCapsuleRadius, 0.01f ) )
            {
                m_previewCapsuleRadius = Math::Clamp( m_previewCapsuleRadius, 0.01f, 5.0f );
            }
        };

        ImGui::SameLine();
        DrawViewportToolbarComboIcon( "##DebugOptions", EE_ICON_BUG_CHECK, "Debug Options", DrawDebugOptions );

        // Preview Controls
        //-------------------------------------------------------------------------

        ImGuiX::ScopedFont const sf( ImGuiX::Font::MediumBold );
        ImGui::SameLine();

        auto DrawPreviewOptions = [this, context]()
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
        };

        //-------------------------------------------------------------------------

        constexpr float const buttonWidth = 160;

        if ( IsDebugging() )
        {
            if ( ImGuiX::IconButtonWithDropDown( "SD", EE_ICON_STOP, "Stop Debugging", ImGuiX::ImColors::Red,  buttonWidth, DrawPreviewOptions ) )
            {
                StopDebugging();
            }
        }
        else
        {
            if ( ImGuiX::IconButtonWithDropDown( "PG", EE_ICON_PLAY, "Preview Graph", ImGuiX::ImColors::Lime,  buttonWidth, DrawPreviewOptions ) )
            {
                StartDebugging( context, DebugTarget() );
            }
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
                if ( pReferenceNode != nullptr && pReferenceNode->GetReferencedParameterValueType() == GraphValueType::Target && pReferenceNode->IsReferencingControlParameter() )
                {
                    pSelectedTargetControlParameter = TryCast<GraphNodes::TargetControlParameterToolsNode>( pReferenceNode->GetReferencedControlParameter() );
                }
            }
        }

        // Allow for in-viewport manipulation of the parameter preview value
        if ( pSelectedTargetControlParameter != nullptr )
        {
            Transform gizmoTransform;
            bool drawGizmo = false;

            if ( IsDebugging() )
            {
                if ( m_pDebugGraphComponent->HasGraphInstance() )
                {
                    int16_t const parameterIdx = m_pDebugGraphComponent->GetControlParameterIndex( StringID( pSelectedTargetControlParameter->GetParameterName() ) );
                    EE_ASSERT( parameterIdx != InvalidIndex );
                    Target const targetValue = m_pDebugGraphComponent->GetControlParameterValue<Target>( parameterIdx );
                    if ( targetValue.IsTargetSet() && !targetValue.IsBoneTarget() )
                    {
                        gizmoTransform = targetValue.GetTransform();
                        drawGizmo = true;
                    }
                }
            }
            else
            {
                if ( !pSelectedTargetControlParameter->IsBoneTarget() )
                {
                    gizmoTransform = pSelectedTargetControlParameter->GetPreviewTargetTransform();
                    drawGizmo = true;
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

                //-------------------------------------------------------------------------

                auto SetParameterValue = [this, pSelectedTargetControlParameter] ( Transform const& newTransform )
                {
                    if ( IsDebugging() )
                    {
                        EE_ASSERT( m_pDebugGraphComponent->HasGraphInstance() );
                        int16_t const parameterIdx = m_pDebugGraphComponent->GetControlParameterIndex( StringID( pSelectedTargetControlParameter->GetParameterName() ) );
                        EE_ASSERT( parameterIdx != InvalidIndex );
                        m_pDebugGraphComponent->SetControlParameterValue<Target>( parameterIdx, Target( newTransform ) );
                    }
                    else
                    {
                        pSelectedTargetControlParameter->SetPreviewTargetTransform( newTransform );
                    }
                };

                auto const gizmoResult = m_gizmo.Draw( gizmoTransform.GetTranslation(), gizmoTransform.GetRotation(), *pViewport );
                switch ( gizmoResult.m_state )
                {
                    case ImGuiX::Gizmo::State::StartedManipulating:
                    {
                        if ( !IsDebugging() )
                        {
                            MarkDirty();
                            GetMainGraphData()->m_graphDefinition.GetRootGraph()->BeginModification();
                        }

                        gizmoResult.ApplyResult( gizmoTransform );
                        SetParameterValue( gizmoTransform );
                    }
                    break;

                    case ImGuiX::Gizmo::State::Manipulating:
                    {
                        gizmoResult.ApplyResult( gizmoTransform );
                        SetParameterValue( gizmoTransform );
                    }
                    break;

                    case ImGuiX::Gizmo::State::StoppedManipulating:
                    {
                        gizmoResult.ApplyResult( gizmoTransform );
                        SetParameterValue( gizmoTransform );
                        if ( !IsDebugging() )
                        {
                            GetMainGraphData()->m_graphDefinition.GetRootGraph()->EndModification();
                        }
                    }
                    break;

                    default:
                    break;
                }
            }
        }

        //-------------------------------------------------------------------------

        if ( IsDebugging() && m_showPreviewCapsule )
        {
            if ( m_pDebugGraphComponent->HasGraphInstance() )
            {
                auto drawContext = GetDrawingContext();
                Transform capsuleTransform = m_pDebugGraphComponent->GetDebugWorldTransform() * Transform( Quaternion( EulerAngles( 90, 0, 0 ) ) );
                capsuleTransform.AddTranslation( capsuleTransform.GetAxisY() * ( m_previewCapsuleHalfHeight + m_previewCapsuleRadius ) );
                drawContext.DrawCapsule( capsuleTransform, m_previewCapsuleRadius, m_previewCapsuleHalfHeight, Colors::LimeGreen, 3.0f );
            }
        }
    }

    void AnimationGraphWorkspace::DrawDialogs( UpdateContext const& context )
    {
        bool isDialogOpen = m_activeOperation != GraphOperationType::None;

        //-------------------------------------------------------------------------

        auto EscCancelCheck = [&] ()
        {
            if ( ImGui::IsKeyPressed( ImGuiKey_Escape ) )
            {
                isDialogOpen = false;
                ImGui::CloseCurrentPopup();
            }
        };

        //-------------------------------------------------------------------------

        switch ( m_activeOperation )
        {
            case GraphOperationType::None:
            break;

            case GraphOperationType::Navigate:
            {
                if ( ImGuiX::BeginViewportPopupModal( "Navigate", &isDialogOpen, ImVec2( 800, 600 ), ImGuiCond_FirstUseEver, ImGuiWindowFlags_NoSavedSettings ) )
                {
                    DrawNavigationDialogWindow( context );
                    EscCancelCheck();
                    ImGui::EndPopup();
                }
            }
            break;

            case GraphOperationType::CreateParameter:
            {
                if ( ImGuiX::BeginViewportPopupModal( "Create Parameter", &isDialogOpen, ImVec2( 300, -1 ) ) )
                {
                    DrawCreateOrRenameParameterDialogWindow();
                    EscCancelCheck();
                    ImGui::EndPopup();
                }
            }
            break;

            case GraphOperationType::RenameParameter:
            {
                if ( ImGuiX::BeginViewportPopupModal( "Rename Parameter", &isDialogOpen, ImVec2( 300, -1 ) ) )
                {
                    DrawCreateOrRenameParameterDialogWindow();
                    EscCancelCheck();
                    ImGui::EndPopup();
                }
            }
            break;

            case GraphOperationType::DeleteParameter:
            {
                if ( ImGuiX::BeginViewportPopupModal( "Delete Parameter", &isDialogOpen, ImVec2( 300, -1 ) ) )
                {
                    DrawDeleteParameterDialogWindow();
                    EscCancelCheck();
                    ImGui::EndPopup();
                }
            }
            break;

            case GraphOperationType::CreateVariation:
            {
                if ( ImGuiX::BeginViewportPopupModal( "Create Variation", &isDialogOpen, ImVec2( 300, -1 ) ) )
                {
                    DrawCreateVariationDialogWindow();
                    EscCancelCheck();
                    ImGui::EndPopup();
                }
            }
            break;

            case GraphOperationType::RenameVariation:
            {
                if ( ImGuiX::BeginViewportPopupModal( "Rename Variation", &isDialogOpen, ImVec2( 300, -1 ) ) )
                {
                    DrawRenameVariationDialogWindow();
                    EscCancelCheck();
                    ImGui::EndPopup();
                }
            }
            break;

            case GraphOperationType::DeleteVariation:
            {
                if ( ImGuiX::BeginViewportPopupModal( "Delete Variation", &isDialogOpen ) )
                {
                    DrawDeleteVariationDialogWindow();
                    EscCancelCheck();
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

        // If the dialog was closed (i.e. operation canceled)
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
        GetMainGraphData()->m_graphDefinition.SaveToJson( *m_pToolsContext->m_pTypeRegistry, *archive.GetWriter() );
        if ( archive.WriteToFile( m_graphFilePath ) )
        {
            auto const definitionExtension = GraphDefinition::GetStaticResourceTypeID().ToString();
            EE_ASSERT( m_graphFilePath.IsValid() && m_graphFilePath.MatchesExtension( definitionExtension.c_str() ) );

            // Create list of valid descriptor
            //-------------------------------------------------------------------------

            TVector<FileSystem::Path> validVariations;

            auto const& variationHierarchy = GetMainGraphData()->m_graphDefinition.GetVariationHierarchy();
            for ( auto const& variation : variationHierarchy.GetAllVariations() )
            {
                validVariations.emplace_back( Variation::GenerateResourceFilePath( m_graphFilePath, variation.m_ID ) );
            }

            // Delete all invalid descriptors for this graph
            //-------------------------------------------------------------------------

            auto const variationExtension = GraphVariation::GetStaticResourceTypeID().ToString();
            TVector<FileSystem::Path> allVariationFiles;
            if ( FileSystem::GetDirectoryContents( m_graphFilePath.GetParentDirectory(), allVariationFiles, FileSystem::DirectoryReaderOutput::OnlyFiles, FileSystem::DirectoryReaderMode::DontExpand, { variationExtension.c_str() } ) )
            {
                String const variationPathPrefix = Variation::GenerateResourceFilePathPrefix( m_graphFilePath );
                for ( auto const& variationFilePath : allVariationFiles )
                {
                    if ( variationFilePath.GetFullPath().find( variationPathPrefix.c_str() ) != String::npos )
                    {
                        if ( !VectorContains( validVariations, variationFilePath ) )
                        {
                            FileSystem::EraseFile( variationFilePath );
                        }
                    }
                }
            }

            // Generate all variation descriptors
            //-------------------------------------------------------------------------

            for ( auto const& variation : variationHierarchy.GetAllVariations() )
            {
                if ( !Variation::TryCreateVariationFile( *m_pToolsContext->m_pTypeRegistry, m_pToolsContext->GetRawResourceDirectory(), m_graphFilePath, variation.m_ID ) )
                {
                    InlineString const str( InlineString::CtorSprintf(), "Failed to create variation file - %s", Variation::GenerateResourceFilePath( m_graphFilePath, variation.m_ID ).c_str() );
                    pfd::message( "Error Saving!", str.c_str(), pfd::choice::ok, pfd::icon::error ).result();
                    return false;
                }
            }

            ClearDirty();
            return true;
        }

        return false;
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
        auto pRootGraph = GetMainGraphData()->m_graphDefinition.GetRootGraph();

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
                m_primaryGraphView.SetGraphToView( pFoundGraph );
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

    void AnimationGraphWorkspace::OnHotReloadStarted( bool descriptorNeedsReload, TInlineVector<Resource::ResourcePtr*, 10> const& resourcesToBeReloaded )
    {
        TWorkspace<GraphDefinition>::OnHotReloadStarted( descriptorNeedsReload, resourcesToBeReloaded );

        if ( IsPreviewOrReview() && !m_isFirstPreviewFrame )
        {
            StopDebugging();
        }
    }

    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::PreUpdateWorld( EntityWorldUpdateContext const& updateContext )
    {
        if ( IsPreviewOrReview() )
        {
            if ( !m_pDebugGraphComponent->IsInitialized() )
            {
                return;
            }

            //-------------------------------------------------------------------------

            if ( m_isFirstPreviewFrame )
            {
                // We need to set the graph instance here since it was not available when we started the debug session
                EE_ASSERT( m_pDebugGraphComponent->HasGraphInstance() );
                m_pDebugGraphInstance = m_pDebugGraphComponent->GetDebugGraphInstance();
                if ( !GenerateGraphStackDebugData() )
                {
                    StopDebugging();
                }

                UpdateUserContext();

                if ( IsReviewingRecording() )
                {
                    m_reviewStarted = true;
                    SetFrameToReview( 0 );
                }
                else if ( IsPreviewing() )
                {
                    ReflectInitialPreviewParameterValues( updateContext );
                }

                m_isFirstPreviewFrame = false;
            }

            //-------------------------------------------------------------------------

            m_characterTransform = m_pDebugGraphComponent->GetDebugWorldTransform();
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

            if ( m_isFirstPreviewFrame )
            {
                EE_ASSERT( m_pDebugGraphInstance != nullptr );
                if ( !GenerateGraphStackDebugData() )
                {
                    StopDebugging();
                }

                UpdateUserContext();
                m_isFirstPreviewFrame = false;
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
                    Transform const currentCameraTransform = GetCameraTransform();
                    Transform const offsetDelta = Transform::Delta( m_previousCameraTransform, currentCameraTransform );
                    m_cameraOffsetTransform = offsetDelta * m_cameraOffsetTransform;
                    m_previousCameraTransform = m_cameraOffsetTransform * m_characterTransform;
                    SetCameraTransform( m_previousCameraTransform );
                }
            }

            // Reflect debug options
            if ( updateContext.GetUpdateStage() == UpdateStage::FrameEnd )
            {
                if ( m_pDebugGraphComponent->IsInitialized() )
                {
                    auto drawingContext = updateContext.GetDrawingContext();
                    m_pDebugGraphComponent->SetGraphDebugMode( m_graphDebugMode );
                    m_pDebugGraphComponent->SetRootMotionDebugMode( m_rootMotionDebugMode );
                    m_pDebugGraphComponent->SetTaskSystemDebugMode( m_taskSystemDebugMode );
                    m_pDebugGraphComponent->DrawDebug( drawingContext );
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    // Graph Operations
    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::OnBeginGraphModification( VisualGraph::BaseGraph* pRootGraph )
    {
        if ( pRootGraph == GetMainGraphData()->m_graphDefinition.GetRootGraph() )
        {
            EE_ASSERT( m_pActiveUndoableAction == nullptr );

            auto pGraphUndoableAction = EE::New<GraphUndoableAction>( this );
            pGraphUndoableAction->SerializeBeforeState();
            m_pActiveUndoableAction = pGraphUndoableAction;
        }
    }

    void AnimationGraphWorkspace::OnEndGraphModification( VisualGraph::BaseGraph* pRootGraph )
    {
        if ( pRootGraph == GetMainGraphData()->m_graphDefinition.GetRootGraph() )
        {
            EE_ASSERT( m_pActiveUndoableAction != nullptr );

            Cast<GraphUndoableAction>( m_pActiveUndoableAction )->SerializeAfterState();
            m_undoStack.RegisterAction( m_pActiveUndoableAction );
            m_pActiveUndoableAction = nullptr;
            MarkDirty();

            RefreshControlParameterCache();
        }
    }

    void AnimationGraphWorkspace::GraphDoubleClicked( VisualGraph::BaseGraph* pGraph )
    {
        EE_ASSERT( pGraph != nullptr );
        int32_t const stackIdx = GetStackIndexForGraph( pGraph );
        if ( pGraph->GetParentNode() == nullptr && stackIdx > 0 )
        {
            NavigateTo( m_graphStack[stackIdx]->m_pParentNode );
        }
    }

    void AnimationGraphWorkspace::PostPasteNodes( TInlineVector<VisualGraph::BaseNode*, 20> const& pastedNodes )
    {
        VisualGraph::ScopedGraphModification gm( GetMainGraphData()->m_graphDefinition.GetRootGraph() );
        GetMainGraphData()->m_graphDefinition.RefreshParameterReferences();
        RefreshControlParameterCache();
    }

    //-------------------------------------------------------------------------
    // Debugging
    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::DrawDebuggerWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_debuggerWindowName.c_str(), nullptr, ImGuiWindowFlags_AlwaysVerticalScrollbar ) )
        {
            DrawRecorderUI( context );

            //-------------------------------------------------------------------------

            bool const showDebugData = IsDebugging() && m_pDebugGraphInstance != nullptr && m_pDebugGraphInstance->IsInitialized();
            ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
            if ( ImGui::CollapsingHeader( "Pose Tasks" ) )
            {
                if ( showDebugData )
                {
                    AnimationDebugView::DrawGraphActiveTasksDebugView( m_pDebugGraphInstance );
                }
                else
                {
                    ImGui::Text( "-" );
                }
            }

            ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
            if ( ImGui::CollapsingHeader( "Root Motion" ) )
            {
                if ( showDebugData )
                {
                    AnimationDebugView::DrawRootMotionDebugView( m_pDebugGraphInstance );
                }
                else
                {
                    ImGui::Text( "-" );
                }
            }

            ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
            if ( ImGui::CollapsingHeader( "Anim Events" ) )
            {
                if ( showDebugData )
                {
                    AnimationDebugView::DrawSampledAnimationEventsView( m_pDebugGraphInstance );
                }
                else
                {
                    ImGui::Text( "-" );
                }
            }

            ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
            if ( ImGui::CollapsingHeader( "State Events" ) )
            {
                if ( showDebugData )
                {
                    AnimationDebugView::DrawSampledStateEventsView( m_pDebugGraphInstance );
                }
                else
                {
                    ImGui::Text( "-" );
                }
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

        // Try to compile the graph
        //-------------------------------------------------------------------------

        GraphDefinitionCompiler definitionCompiler;
        bool const graphCompiledSuccessfully = definitionCompiler.CompileGraph( GetMainGraphData()->m_graphDefinition );
        m_visualLog = definitionCompiler.GetLog();

        // Compilation failed, stop preview attempt
        if ( !graphCompiledSuccessfully )
        {
            pfd::message( "Compile Error!", "The graph failed to compile! Please check the compilation log for details!", pfd::choice::ok, pfd::icon::error ).result();
            ImGui::SetWindowFocus( m_graphLogWindowName.c_str() );
            return;
        }

        // Create preview entity
        //-------------------------------------------------------------------------

        m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );

        // Set debug component data
        //-------------------------------------------------------------------------

        // If previewing, create the component
        if ( target.m_type == DebugTargetType::None || target.m_type == DebugTargetType::Recording )
        {
            // Create resource ID for the graph variation
            String const variationPathStr = Variation::GenerateResourceFilePath( m_graphFilePath, GetMainGraphData()->m_selectedVariationID );
            ResourceID const graphVariationResourceID( GetResourcePath( variationPathStr.c_str() ) );

            // Save the graph if needed
            if ( IsDirty() )
            {
                // Ensure that we save the graph and re-generate the dataset on preview
                Save();
            }

            if ( !RequestImmediateResourceCompilation( graphVariationResourceID ) )
            {
                return;
            }

            // Create Preview Graph Component
            EE_ASSERT( !m_previewGraphVariationPtr.IsSet() );
            m_previewGraphVariationPtr = TResourcePtr<GraphVariation>( graphVariationResourceID );
            LoadResource( &m_previewGraphVariationPtr );

            m_pDebugGraphComponent = EE::New<AnimationGraphComponent>( StringID( "Animation Component" ) );
            m_pDebugGraphComponent->ShouldApplyRootMotionToEntity( true );
            m_pDebugGraphComponent->SetGraphVariation( graphVariationResourceID );

            if ( target.m_type == DebugTargetType::Recording )
            {
                m_pDebugGraphComponent->SwitchToRecordingPlaybackMode();
            }

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
            if ( GetMainGraphData()->m_selectedVariationID != debuggedInstanceVariationID )
            {
                SetSelectedVariation( debuggedInstanceVariationID );
            }
        }

        // Try Create Preview Mesh Component
        //-------------------------------------------------------------------------

        auto pVariation = GetMainGraphData()->m_graphDefinition.GetVariation( GetMainGraphData()->m_selectedVariationID );
        EE_ASSERT( pVariation != nullptr );
        if ( pVariation->m_skeleton.IsSet() )
        {
            // Load resource descriptor for skeleton to get the preview mesh
            FileSystem::Path const resourceDescPath = GetFileSystemPath( pVariation->m_skeleton.GetResourcePath() );
            SkeletonResourceDescriptor resourceDesc;
            if ( Resource::ResourceDescriptor::TryReadFromFile( *m_pToolsContext->m_pTypeRegistry, resourceDescPath, resourceDesc ) )
            {
                // Create a preview mesh component
                m_pDebugMeshComponent = EE::New<Render::SkeletalMeshComponent>( StringID( "Mesh Component" ) );
                m_pDebugMeshComponent->SetSkeleton( pVariation->m_skeleton.GetResourceID() );
                m_pDebugMeshComponent->SetMesh( resourceDesc.m_previewMesh.GetResourceID() );
                m_pPreviewEntity->AddComponent( m_pDebugMeshComponent );
            }
        }

        // Add preview entity to the World
        //-------------------------------------------------------------------------

        AddEntityToWorld( m_pPreviewEntity );

        // Set up preview data
        //-------------------------------------------------------------------------

        m_isFirstPreviewFrame = true;

        // Preview
        if ( target.m_type == DebugTargetType::None )
        {
            SetWorldPaused( m_startPaused );
            m_characterTransform = m_previewStartTransform;
            m_debugMode = DebugMode::Preview;
        }
        // Review
        else if ( target.m_type == DebugTargetType::Recording )
        {
            SetWorldPaused( false );
            m_characterTransform = m_updateRecorder.m_recordedData.front().m_characterWorldTransform;
            m_debugMode = DebugMode::ReviewRecording;
            m_currentReviewFrameIdx = InvalidIndex;
            m_reviewStarted = false;
        }
        else // Live Debug
        {
            SetWorldPaused( false );
            m_debugMode = DebugMode::LiveDebug;
            m_characterTransform = m_pDebugGraphComponent->GetDebugWorldTransform();
        }

        // Mesh Transform
        //-------------------------------------------------------------------------

        if ( m_pDebugMeshComponent != nullptr )
        {
            m_pDebugMeshComponent->SetWorldTransform( m_characterTransform );
        }

        // Adjust Camera
        //-------------------------------------------------------------------------

        if ( m_debugMode == DebugMode::LiveDebug || m_debugMode == DebugMode::ReviewRecording )
        {
            AABB const bounds( m_characterTransform.GetTranslation(), Vector::One );
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

        // Recording
        //-------------------------------------------------------------------------

        if ( IsReviewingRecording() )
        {
            m_currentReviewFrameIdx = InvalidIndex;
            m_reviewStarted = false;
        }
        else
        {
            if ( m_isRecording )
            {
                StopRecording();
            }
        }

        // Destroy entity and clear debug state
        //-------------------------------------------------------------------------

        EE_ASSERT( m_pPreviewEntity != nullptr );
        DestroyEntityInWorld( m_pPreviewEntity );
        m_pPreviewEntity = nullptr;
        m_pDebugGraphComponent = nullptr;
        m_pDebugMeshComponent = nullptr;
        m_pDebugGraphInstance = nullptr;
        m_debugExternalGraphSlotID.Clear();

        // Release variation reference
        //-------------------------------------------------------------------------

        if ( IsPreviewOrReview() )
        {
            EE_ASSERT( m_previewGraphVariationPtr.IsSet() );
            UnloadResource( &m_previewGraphVariationPtr );
            m_previewGraphVariationPtr.Clear();
        }

        // Reset debug mode
        //-------------------------------------------------------------------------

        if ( IsLiveDebugging() )
        {
            SetCameraTransform( m_cameraOffsetTransform );
            m_previousCameraTransform = m_cameraOffsetTransform;
            ResetCameraView();
        }

        //-------------------------------------------------------------------------

        m_primaryGraphView.RefreshNodeSizes();
        m_secondaryGraphView.RefreshNodeSizes();

        //-------------------------------------------------------------------------

        m_debugMode = DebugMode::None;

        //-------------------------------------------------------------------------

        ClearGraphStackDebugData();
        UpdateUserContext();
    }

    void AnimationGraphWorkspace::ReflectInitialPreviewParameterValues( UpdateContext const& context )
    {
        EE_ASSERT( m_isFirstPreviewFrame && m_pDebugGraphComponent != nullptr && m_pDebugGraphComponent->IsInitialized() );

        auto pRootGraph = GetMainGraphData()->m_graphDefinition.GetRootGraph();
        auto const controlParameters = pRootGraph->FindAllNodesOfType<GraphNodes::ControlParameterToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
        for ( auto pControlParameter : controlParameters )
        {
            int16_t const parameterIdx = m_pDebugGraphComponent->GetControlParameterIndex( StringID( pControlParameter->GetParameterName() ) );

            // Can occur if the preview starts before the compile completes since we currently dont have a mechanism to force compile and wait in the editor
            if( parameterIdx == InvalidIndex )
            {
                continue;
            }

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
        ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );

        //-------------------------------------------------------------------------

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
                if ( pGraphInstance->GetDefinitionResourceID() == m_workspaceResource.GetResourceID() )
                {
                    Entity const* pEntity = pWorld->FindEntity( pGraphComponent->GetEntityID() );
                    InlineString const targetName( InlineString::CtorSprintf(), "%s (%s)", pEntity->GetNameID().c_str(), pGraphComponent->GetNameID().c_str() );
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
                    if ( childGraph.m_pInstance->GetDefinitionResourceID() == m_workspaceResource.GetResourceID() )
                    {
                        Entity const* pEntity = pWorld->FindEntity( pGraphComponent->GetEntityID() );
                        InlineString const targetName( InlineString::CtorSprintf(), "%s (%s) - %s", pEntity->GetNameID().c_str(), pGraphComponent->GetNameID().c_str(), childGraph.m_pathToInstance.c_str() );
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
                    if ( externalGraph.m_pInstance->GetDefinitionResourceID() == m_workspaceResource.GetResourceID() )
                    {
                        Entity const* pEntity = pWorld->FindEntity( pGraphComponent->GetEntityID() );
                        InlineString const targetName( InlineString::CtorSprintf(), "%s (%s) - %s", pEntity->GetNameID().c_str(), pGraphComponent->GetNameID().c_str(), externalGraph.m_slotID.c_str() );
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
        m_previousCameraTransform = GetCameraTransform();
        m_cameraOffsetTransform = Transform::Delta( m_characterTransform, m_previousCameraTransform );
    }

    //-------------------------------------------------------------------------
    // Selection
    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::SetSelectedVariation( StringID variationID )
    {
        EE_ASSERT( GetMainGraphData()->m_graphDefinition.IsValidVariation( variationID ) );
        if ( GetMainGraphData()->m_selectedVariationID != variationID )
        {
            if ( IsDebugging() )
            {
                StopDebugging();
            }

            GetMainGraphData()->m_selectedVariationID = variationID;
            UpdateUserContext();
        }
    }

    void AnimationGraphWorkspace::TrySetSelectedVariation( String const& variationName )
    {
        auto const& varHierarchy = GetMainGraphData()->m_graphDefinition.GetVariationHierarchy();
        StringID const variationID = varHierarchy.TryGetCaseCorrectVariationID( variationName );
        if ( variationID.IsValid() )
        {
            SetSelectedVariation( variationID );
        }
    }

    //-------------------------------------------------------------------------
    // User Context
    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::InitializeUserContext()
    {
        m_userContext.m_pTypeRegistry = m_pToolsContext->m_pTypeRegistry;
        m_userContext.m_pCategorizedNodeTypes = &m_categorizedNodeTypes.GetRootCategory();
        m_userContext.m_pVariationHierarchy = &GetMainGraphData()->m_graphDefinition.GetVariationHierarchy();
        m_userContext.m_pControlParameters = &m_controlParameters;
        m_userContext.m_pVirtualParameters = &m_virtualParameters;

        m_graphDoubleClickedEventBindingID = m_userContext.OnGraphDoubleClicked().Bind( [this] ( VisualGraph::BaseGraph* pGraph ) { GraphDoubleClicked( pGraph ); } );
        m_postPasteNodesEventBindingID = m_userContext.OnPostPasteNodes().Bind( [this] ( TInlineVector<VisualGraph::BaseNode*, 20> const& pastedNodes ) { PostPasteNodes( pastedNodes ); } );
        m_requestOpenChildGraphBindingID = m_userContext.OnRequestOpenChildGraph().Bind( [this] ( VisualGraph::BaseNode* pSourceNode, ResourceID const& graphID, bool openInNewWorkspace ) { OpenChildGraph( pSourceNode, graphID, openInNewWorkspace ); } );
        m_resourceOpenRequestEventBindingID = m_userContext.OnRequestOpenResource().Bind( [this] ( ResourceID const& resourceID ) { m_pToolsContext->TryOpenResource( resourceID ); } );
        m_navigateToNodeEventBindingID = m_userContext.OnNavigateToNode().Bind( [this] ( VisualGraph::BaseNode* pNode ) { NavigateTo( pNode ); } );
        m_navigateToGraphEventBindingID = m_userContext.OnNavigateToGraph().Bind( [this] ( VisualGraph::BaseGraph* pGraph ) { NavigateTo( pGraph ); } );
    }

    void AnimationGraphWorkspace::UpdateUserContext()
    {
        m_userContext.m_selectedVariationID = m_graphStack.back()->m_selectedVariationID;
        m_userContext.m_pVariationHierarchy = &m_graphStack.back()->m_graphDefinition.GetVariationHierarchy();

        //-------------------------------------------------------------------------

        if ( IsDebugging() && m_pDebugGraphInstance != nullptr )
        {
            m_userContext.m_pGraphInstance = m_graphStack.back()->m_pGraphInstance;
            m_userContext.m_nodeIDtoIndexMap = m_graphStack.back()->m_nodeIDtoIndexMap;
            m_userContext.m_nodeIndexToIDMap = m_graphStack.back()->m_nodeIndexToIDMap;
        }
        else
        {
            m_userContext.m_pGraphInstance = nullptr;
            m_userContext.m_nodeIDtoIndexMap.clear();
            m_userContext.m_nodeIndexToIDMap.clear();
        }

        //-------------------------------------------------------------------------

        if ( m_graphStack.size() > 1 )
        {
            m_userContext.SetExtraGraphTitleInfoText( "External Child Graph" );
            m_userContext.SetExtraTitleInfoTextColor( Colors::HotPink.ToUInt32_ABGR() );
        }
        else // In main graph
        {
            m_userContext.ResetExtraTitleInfo();
        }
    }

    void AnimationGraphWorkspace::ShutdownUserContext()
    {
        m_userContext.OnNavigateToNode().Unbind( m_navigateToNodeEventBindingID );
        m_userContext.OnNavigateToGraph().Unbind( m_navigateToGraphEventBindingID );
        m_userContext.OnRequestOpenResource().Unbind( m_resourceOpenRequestEventBindingID );
        m_userContext.OnRequestOpenChildGraph().Unbind( m_requestOpenChildGraphBindingID );
        m_userContext.OnPostPasteNodes().Unbind( m_postPasteNodesEventBindingID );
        m_userContext.OnGraphDoubleClicked().Unbind( m_graphDoubleClickedEventBindingID );

        m_userContext.m_pTypeRegistry = nullptr;
        m_userContext.m_pCategorizedNodeTypes = nullptr;
        m_userContext.m_pVariationHierarchy = nullptr;
        m_userContext.m_pControlParameters = nullptr;
        m_userContext.m_pVirtualParameters = nullptr;
    }

    //-------------------------------------------------------------------------
    // Graph View
    //-------------------------------------------------------------------------

    bool AnimationGraphWorkspace::IsInReadOnlyState() const
    {
        if ( IsDebugging() )
        {
            return true;
        }

        return m_graphStack.size() > 1;
    }

    void AnimationGraphWorkspace::DrawGraphViewNavigationBar()
    {
        auto pRootGraph = GetMainGraphData()->m_graphDefinition.GetRootGraph();

        ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold );

        // Sizes
        //-------------------------------------------------------------------------

        constexpr char const* const pBreadcrumbPopupName = "Breadcrumb";
        ImVec2 const navBarDimensions = ImGui::GetContentRegionAvail();
        constexpr static float const homeButtonWidth = 22;
        constexpr static float const stateMachineNavButtonWidth = 64;

        float const buttonHeight = navBarDimensions.y;
        float const statemachineNavRequiredSpace = m_primaryGraphView.IsViewingStateMachineGraph() ? ( stateMachineNavButtonWidth * 2 ) : 0;

        // Get all entries for breadcrumb
        //-------------------------------------------------------------------------

        struct NavBarItem
        {
            VisualGraph::BaseNode*  m_pNode = nullptr;
            int32_t                 m_stackIdx = InvalidIndex;
            bool                    m_isStackBoundary = false;
        };

        TInlineVector<NavBarItem, 20> pathFromChildToRoot;

        int32_t pathStackIdx = int32_t( m_graphStack.size() ) - 1;
        auto pGraph = m_primaryGraphView.GetViewedGraph();
        VisualGraph::BaseNode* pParentNode = nullptr;

        do
        {
            // Switch stack if possible
            bool isStackBoundary = false;
            pParentNode = pGraph->GetParentNode();
            if ( pParentNode == nullptr && pathStackIdx > 0 )
            {
                EE_ASSERT( pGraph == m_graphStack[pathStackIdx]->m_graphDefinition.GetRootGraph() );
                pParentNode = m_graphStack[pathStackIdx]->m_pParentNode;
                isStackBoundary = true;
                pathStackIdx--;
            }

            // Add item
            if ( pParentNode != nullptr )
            {
                NavBarItem item;
                item.m_pNode = pParentNode;
                item.m_stackIdx = pathStackIdx;
                item.m_isStackBoundary = isStackBoundary;

                pathFromChildToRoot.emplace_back( item );
                pGraph = pParentNode->GetParentGraph();
            }

        } while ( pParentNode != nullptr );

        // Draw Home Button
        //-------------------------------------------------------------------------

        ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );

        if ( ImGuiX::ColoredButton( ImGuiX::ImColors::Green, ImGuiX::ImColors::White, EE_ICON_HOME"##GoHome", ImVec2( homeButtonWidth, buttonHeight ) ) )
        {
            NavigateTo( pRootGraph );
        }
        ImGuiX::ItemTooltip( "Go to root" );

        ImGui::SameLine( 0, 0 );
        if ( ImGui::Button( EE_ICON_CHEVRON_RIGHT"##RootBrowser", ImVec2( 0, buttonHeight ) ) )
        {
            m_pBreadcrumbPopupContext = nullptr;
            ImGui::OpenPopup( pBreadcrumbPopupName );
        }

        // Draw breadcrumbs
        //-------------------------------------------------------------------------

        // Defer all navigation requests until after the UI is finished drawing
        VisualGraph::BaseGraph* pGraphToNavigateTo = nullptr;

        // Draw from root to child
        for ( int32_t i = int32_t( pathFromChildToRoot.size() ) - 1; i >= 0; i-- )
        {
            bool const isLastItem = ( i == 0 );
            bool drawChevron = true;
            bool drawItem = true;

            auto const pParentState = TryCast<GraphNodes::StateToolsNode>( pathFromChildToRoot[i].m_pNode->GetParentGraph()->GetParentNode() );
            auto const pState = TryCast<GraphNodes::StateToolsNode>( pathFromChildToRoot[i].m_pNode );

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
            if ( drawChevron && isLastItem )
            {
                VisualGraph::BaseGraph* pChildGraphToCheck = nullptr;

                // Check external child graph for state machines
                if ( IsOfType<GraphNodes::ChildGraphToolsNode>( pathFromChildToRoot[i].m_pNode ) )
                {
                    EE_ASSERT( ( pathFromChildToRoot[i].m_stackIdx + 1 ) < m_graphStack.size() );
                    pChildGraphToCheck = m_graphStack[pathFromChildToRoot[i].m_stackIdx + 1]->m_graphDefinition.GetRootGraph();
                }
                // We should search child graph
                else if ( !IsOfType<GraphNodes::StateMachineToolsNode>( pathFromChildToRoot[i].m_pNode ) )
                {
                    pChildGraphToCheck = pathFromChildToRoot[i].m_pNode->GetChildGraph();
                }

                // If we have a graph to check then check for child state machines
                if ( pChildGraphToCheck != nullptr )
                {
                    auto const childStateMachines = pChildGraphToCheck->FindAllNodesOfType<GraphNodes::StateMachineToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Exact );
                    if ( childStateMachines.empty() )
                    {
                        drawChevron = false;
                    }
                }
                else // Hide chevron
                {
                    drawChevron = false;
                }
            }

            // Draw the item
            if ( drawItem )
            {
                bool const isExternalChildGraphItem = pathFromChildToRoot[i].m_stackIdx > 0;
                if ( isExternalChildGraphItem )
                {
                    ImVec4 const color = ( Vector( 0.65f, 0.65f, 0.65f, 1.0f ) * Vector( ImGuiX::Style::s_colorText.Value ) ).ToFloat4();
                    ImGui::PushStyleColor( ImGuiCol_Text, color );
                }

                ImGui::SameLine( 0, 0 );
                InlineString const str( InlineString::CtorSprintf(), "%s##%s", pathFromChildToRoot[i].m_pNode->GetName(), pathFromChildToRoot[i].m_pNode->GetID().ToString().c_str() );
                if ( ImGui::Button( str.c_str(), ImVec2( 0, buttonHeight ) ) )
                {
                    if ( isStateMachineState )
                    {
                        auto const childStateMachines = pathFromChildToRoot[i].m_pNode->GetChildGraph()->FindAllNodesOfType<GraphNodes::StateMachineToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Exact );
                        EE_ASSERT( childStateMachines.size() );
                        pGraphToNavigateTo = childStateMachines[0]->GetChildGraph();
                    }
                    else
                    {
                        // Go to the external child graph's root graph
                        if ( auto pCG = TryCast<GraphNodes::ChildGraphToolsNode>( pathFromChildToRoot[i].m_pNode ) )
                        {
                            EE_ASSERT( m_graphStack[pathFromChildToRoot[i].m_stackIdx + 1]->m_pParentNode == pCG );
                            pGraphToNavigateTo = m_graphStack[pathFromChildToRoot[i].m_stackIdx + 1]->m_graphDefinition.GetRootGraph();
                        }
                        else // Go to the node's child graph
                        {
                            pGraphToNavigateTo = pathFromChildToRoot[i].m_pNode->GetChildGraph();
                        }
                    }
                }

                if ( isExternalChildGraphItem )
                {
                    ImGui::PopStyleColor();
                }
            }

            // Draw the chevron
            if ( drawChevron )
            {
                ImGui::SameLine( 0, 0 );
                InlineString const separatorStr( InlineString::CtorSprintf(), EE_ICON_CHEVRON_RIGHT"##%s", pathFromChildToRoot[i].m_pNode->GetName(), pathFromChildToRoot[i].m_pNode->GetID().ToString().c_str() );
                if ( ImGui::Button( separatorStr.c_str(), ImVec2( 0, buttonHeight ) ) )
                {
                    if ( pGraphToNavigateTo == nullptr ) // Dont open the popup if we have a nav request
                    {
                        m_pBreadcrumbPopupContext = pathFromChildToRoot[i].m_pNode;
                        ImGui::OpenPopup( pBreadcrumbPopupName );
                    }
                }
            }
        }
        ImGui::PopStyleColor();

        // Draw chevron navigation menu
        //-------------------------------------------------------------------------

        if ( ImGui::BeginPopup( pBreadcrumbPopupName ) )
        {
            bool hasItems = false;

            // If we navigating in a state machine node, we need to list all states
            if ( auto pSM = TryCast<GraphNodes::StateMachineToolsNode>( m_pBreadcrumbPopupContext ) )
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
                            pGraphToNavigateTo = pChildState->GetChildGraph();
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
                            pGraphToNavigateTo = childStateMachines[0]->GetChildGraph();
                            ImGui::CloseCurrentPopup();
                        }
                    }

                    hasItems = true;
                }
            }
            else // Just display all state machine nodes in this graph
            {
                VisualGraph::BaseGraph* pGraphToSearch = nullptr;

                if ( m_pBreadcrumbPopupContext == nullptr )
                {
                    pGraphToSearch = pRootGraph;
                }
                else if ( auto pCG = TryCast<GraphNodes::ChildGraphToolsNode>( m_pBreadcrumbPopupContext ) )
                {
                    int32_t const nodeStackIdx = GetStackIndexForNode( pCG );
                    int32_t const childGraphStackIdx = nodeStackIdx + 1;
                    EE_ASSERT( childGraphStackIdx < m_graphStack.size() );
                    pGraphToSearch = m_graphStack[childGraphStackIdx]->m_graphDefinition.GetRootGraph();
                }
                else
                {
                    pGraphToSearch = m_pBreadcrumbPopupContext->GetChildGraph();
                }

                EE_ASSERT( pGraphToSearch != nullptr );

                auto childSMs = pGraphToSearch->FindAllNodesOfType<GraphNodes::StateMachineToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
                for ( auto pChildSM : childSMs )
                {
                    if ( ImGui::MenuItem( pChildSM->GetName() ) )
                    {
                        pGraphToNavigateTo = pChildSM->GetChildGraph();
                        ImGui::CloseCurrentPopup();
                    }

                    hasItems = true;
                }
            }

            //-------------------------------------------------------------------------

            if ( !hasItems )
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if ( !ImGui::IsPopupOpen( pBreadcrumbPopupName ) )
        {
            m_pBreadcrumbPopupContext = nullptr;
        }

        // Handle navigation request
        //-------------------------------------------------------------------------

        if ( pGraphToNavigateTo != nullptr )
        {
            NavigateTo( pGraphToNavigateTo );
        }

        // Draw state machine navigation options
        //-------------------------------------------------------------------------

        if ( m_primaryGraphView.IsViewingStateMachineGraph() )
        {
            ImGui::SameLine( navBarDimensions.x - statemachineNavRequiredSpace, 0 );
            ImGui::AlignTextToFramePadding();
            if ( ImGuiX::ColoredButton( ImGuiX::ImColors::Green, ImGuiX::ImColors::White, EE_ICON_DOOR_OPEN" Entry", ImVec2( stateMachineNavButtonWidth, buttonHeight ) ) )
            {
                auto pSM = Cast<StateMachineGraph>( m_primaryGraphView.GetViewedGraph() );
                NavigateTo( pSM->GetEntryStateOverrideConduit(), false );
            }
            ImGuiX::ItemTooltip( "Entry State Overrides" );

            ImGui::SameLine( 0, -1 );
            if ( ImGuiX::ColoredButton( ImGuiX::ImColors::OrangeRed, ImGuiX::ImColors::White, EE_ICON_LIGHTNING_BOLT"Global", ImVec2( stateMachineNavButtonWidth, buttonHeight ) ) )
            {
                auto pSM = Cast<StateMachineGraph>( m_primaryGraphView.GetViewedGraph() );
                NavigateTo( pSM->GetGlobalTransitionConduit(), false );
            }
            ImGuiX::ItemTooltip( "Global Transitions" );
        }
    }

    void AnimationGraphWorkspace::DrawGraphView( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        EE_ASSERT( &m_userContext != nullptr );

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

            m_primaryGraphView.UpdateAndDraw( *pTypeRegistry, m_primaryGraphViewHeight );

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
            m_secondaryGraphView.UpdateAndDraw( *pTypeRegistry );

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
            m_secondaryGraphView.SetGraphToView( pSecondaryGraphToView );
        }
    }

    int32_t AnimationGraphWorkspace::GetStackIndexForNode( VisualGraph::BaseNode* pNode ) const
    {
        EE_ASSERT( pNode != nullptr );

        for ( auto i = 0; i < m_graphStack.size(); i++ )
        {
            auto const pFoundNode = m_graphStack[i]->m_graphDefinition.GetRootGraph()->FindNode( pNode->GetID(), true );
            if ( pFoundNode != nullptr )
            {
                EE_ASSERT( pFoundNode == pNode );
                return i;
            }
        }

        return InvalidIndex;
    }

    int32_t AnimationGraphWorkspace::GetStackIndexForGraph( VisualGraph::BaseGraph* pGraph ) const
    {
        EE_ASSERT( pGraph != nullptr );
        EE_ASSERT( pGraph->GetParentNode() == nullptr || pGraph->GetParentNode()->GetSecondaryGraph() != pGraph );

        for ( auto i = 0; i < m_graphStack.size(); i++ )
        {
            auto const pFoundGraph = m_graphStack[i]->m_graphDefinition.GetRootGraph()->FindPrimaryGraph( pGraph->GetID() );
            if ( pFoundGraph != nullptr )
            {
                EE_ASSERT( pFoundGraph == pGraph );
                return i;
            }
        }

        return InvalidIndex;
    }

    void AnimationGraphWorkspace::PushOnGraphStack( VisualGraph::BaseNode* pSourceNode, ResourceID const& graphID )
    {
        EE_ASSERT( pSourceNode != nullptr );
        EE_ASSERT( graphID.IsValid() && graphID.GetResourceTypeID() == GraphVariation::GetStaticResourceTypeID() );

        ResourceID const graphDefinitionResourceID = Variation::GetGraphResourceID( graphID );
        FileSystem::Path const childGraphFilePath = GetFileSystemPath( graphDefinitionResourceID.GetResourcePath() );
        if ( childGraphFilePath.IsValid() )
        {
            // Try read JSON data
            Serialization::JsonArchiveReader archive;
            if ( archive.ReadFromFile( childGraphFilePath ) )
            {
                // Try to load the graph from the file
                auto pChildGraphViewData = m_graphStack.emplace_back( EE::New<GraphData>() );
                if ( pChildGraphViewData->m_graphDefinition.LoadFromJson( *m_pToolsContext->m_pTypeRegistry, archive.GetDocument() ) )
                {
                    StringID const variationID = pChildGraphViewData->m_graphDefinition.GetVariationHierarchy().TryGetCaseCorrectVariationID( Variation::GetVariationNameFromResourceID( graphID ) );
                    pChildGraphViewData->m_selectedVariationID = variationID;
                    pChildGraphViewData->m_pParentNode = pSourceNode;
                    NavigateTo( pChildGraphViewData->m_graphDefinition.GetRootGraph() );

                    if ( IsDebugging() )
                    {
                        if ( !GenerateGraphStackDebugData() )
                        {
                            StopDebugging();
                        }
                    }
                    UpdateUserContext();
                }
                else // Remove created invalid view data
                {
                    pfd::message( "Error!", "Failed to load child graph!", pfd::choice::ok, pfd::icon::error ).result();
                    EE::Delete( pChildGraphViewData );
                    m_graphStack.pop_back();
                }
            }
            else // Show error dialog
            {
                pfd::message( "Error!", "Failed to load child graph!", pfd::choice::ok, pfd::icon::error ).result();
            }
        }
        else // Show error dialog
        {
            pfd::message( "Error!", "Invalid child graph resource ID", pfd::choice::ok, pfd::icon::error ).result();
        }
    }

    void AnimationGraphWorkspace::PopGraphStack()
    {
        EE_ASSERT( m_graphStack.size() > 1 );
        EE::Delete( m_graphStack.back() );
        m_graphStack.pop_back();

        UpdateUserContext();
    }

    void AnimationGraphWorkspace::ClearGraphStack()
    {
        if ( m_graphStack.empty() )
        {
            return;
        }

        NavigateTo( GetMainGraphData()->m_graphDefinition.GetRootGraph() );

        for ( auto i = int32_t( m_graphStack.size() ) - 1; i > 0; i-- )
        {
            EE::Delete( m_graphStack[i] );
            m_graphStack.pop_back();
        }

        UpdateUserContext();

        EE_ASSERT( m_graphStack.size() == 1 );
    }

    bool AnimationGraphWorkspace::GenerateGraphStackDebugData()
    {
        ClearGraphStackDebugData();

        //-------------------------------------------------------------------------

        if ( IsDebugging() )
        {
            GraphDefinitionCompiler definitionCompiler;

            TVector<int16_t> childGraphPathNodeIndices;
            for ( auto i = 0; i < m_graphStack.size(); i++ )
            {
                // Compile the external child graph
                if ( !definitionCompiler.CompileGraph( m_graphStack[i]->m_graphDefinition ) )
                {
                    pfd::message( "Error!", "Failed to compile graph - Stopping Debug!", pfd::choice::ok, pfd::icon::error ).result();
                    return false;
                }

                // Record node mappings for this graph
                m_graphStack[i]->m_nodeIDtoIndexMap = definitionCompiler.GetUUIDToRuntimeIndexMap();
                m_graphStack[i]->m_nodeIndexToIDMap = definitionCompiler.GetRuntimeIndexToUUIDMap();

                // Set the debug graph instance
                if ( i == 0 )
                {
                    m_graphStack[i]->m_pGraphInstance = m_pDebugGraphInstance;
                }
                else // Get the instance for the specified node idx
                {
                    GraphData* pParentStack = m_graphStack[i - 1];
                    auto const foundIter = pParentStack->m_nodeIDtoIndexMap.find( m_graphStack[i]->m_pParentNode->GetID() );
                    EE_ASSERT( foundIter != pParentStack->m_nodeIDtoIndexMap.end() );
                    m_graphStack[i]->m_pGraphInstance = const_cast<GraphInstance*>( pParentStack->m_pGraphInstance->GetChildGraphDebugInstance( foundIter->second ) );
                }
            }
        }

        return true;
    }

    void AnimationGraphWorkspace::ClearGraphStackDebugData()
    {
        for ( auto i = 0; i < m_graphStack.size(); i++ )
        {
            m_graphStack[i]->m_pGraphInstance = nullptr;
            m_graphStack[i]->m_nodeIDtoIndexMap.clear();
            m_graphStack[i]->m_nodeIndexToIDMap.clear();
        }
    }

    //-------------------------------------------------------------------------
    // Navigation
    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::NavigationTarget::CreateNavigationTargets( VisualGraph::BaseNode const* pNode, TVector<NavigationTarget>& outTargets )
    {
        // Skip control and virtual parameters
        //-------------------------------------------------------------------------

        if ( IsOfType<GraphNodes::ControlParameterToolsNode>( pNode ) || IsOfType<GraphNodes::VirtualParameterToolsNode>( pNode ) )
        {
            return;
        }

        // Get type of target
        //-------------------------------------------------------------------------

        TVector<StringID> foundIDs;
        NavigationTarget::Type type = NavigationTarget::Type::Unknown;

        if ( IsOfType<GraphNodes::ParameterReferenceToolsNode>( pNode ) )
        {
            type = NavigationTarget::Type::Parameter;
        }
        else if ( auto pStateNode = TryCast<GraphNodes::StateToolsNode>( pNode ) )
        {
            type = NavigationTarget::Type::Pose;
            pStateNode->GetLogicAndEventIDs( foundIDs );
        }
        else if ( auto pFlowNode = TryCast<GraphNodes::FlowToolsNode>( pNode ) )
        {
            GraphValueType const valueType = pFlowNode->GetValueType();
            if ( valueType == GraphValueType::Pose )
            {
                type = NavigationTarget::Type::Pose;
            }
            else
            {
                type = NavigationTarget::Type::Value;
            }

            pFlowNode->GetLogicAndEventIDs( foundIDs );
        }

        // Create nav target for this node
        //-------------------------------------------------------------------------

        if ( type != Type::Unknown )
        {
            auto& addedNodeTarget = outTargets.emplace_back( NavigationTarget() );
            addedNodeTarget.m_pNode = pNode;
            addedNodeTarget.m_label = pNode->GetName();
            addedNodeTarget.m_path = pNode->GetPathFromRoot();
            addedNodeTarget.m_sortKey = addedNodeTarget.m_path;
            addedNodeTarget.m_compKey = StringID( addedNodeTarget.m_path );
            addedNodeTarget.m_type = type;
        }

        // Generate new targets for all IDs in the node
        //-------------------------------------------------------------------------

        if ( type != Type::Unknown )
        {
            for ( auto ID : foundIDs )
            {
                auto& addedIDTarget = outTargets.emplace_back( NavigationTarget() );
                addedIDTarget.m_pNode = pNode;
                addedIDTarget.m_label = ID.c_str();
                addedIDTarget.m_path = pNode->GetPathFromRoot();
                addedIDTarget.m_sortKey = addedIDTarget.m_path + addedIDTarget.m_label;
                addedIDTarget.m_compKey = StringID( addedIDTarget.m_sortKey );
                addedIDTarget.m_type = Type::ID;
            }
        }
    }

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
        EE_ASSERT( pGraph != nullptr );

        ClearSelection();

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
                int32_t const stackIdx = GetStackIndexForGraph( pGraph );
                while ( stackIdx < ( m_graphStack.size() - 1 ) )
                {
                    PopGraphStack();
                }

                m_primaryGraphView.SetGraphToView( pGraph );
                m_primaryViewGraphID = pGraph->GetID();
                UpdateSecondaryViewState();
            }
        }
    }

    void AnimationGraphWorkspace::OpenChildGraph( VisualGraph::BaseNode* pSourceNode, ResourceID const& graphID, bool openInNewWorkspace )
    {
        EE_ASSERT( pSourceNode != nullptr );
        EE_ASSERT( graphID.IsValid() && graphID.GetResourceTypeID() == GraphVariation::GetStaticResourceTypeID() );

        if ( openInNewWorkspace )
        {
            m_userContext.RequestOpenResource( graphID );
        }
        else
        {
            PushOnGraphStack( pSourceNode, graphID );
        }
    }

    void AnimationGraphWorkspace::StartNavigationOperation()
    {
        m_navigationTargetNodes.clear();
        m_navigationActiveTargetNodes.clear();
        GenerateNavigationTargetList();
        m_activeOperation = GraphOperationType::Navigate;
    }

    void AnimationGraphWorkspace::GenerateNavigationTargetList()
    {
        FlowGraph const* pRootGraph = GetMainGraphData()->m_graphDefinition.GetRootGraph();

        m_navigationTargetNodes.clear();

        // Find all graph nodes
        //-------------------------------------------------------------------------

        auto MatchFunction = [] ( VisualGraph::BaseNode const* pNode )
        {
            if ( auto pGraphNode = TryCast<GraphNodes::FlowToolsNode>( pNode ) )
            {
                if ( pGraphNode->GetValueType() == GraphValueType::Pose )
                {
                    return true;
                }
            }

            return false;
        };

        auto const foundNodes = pRootGraph->FindAllNodesOfTypeAdvanced<VisualGraph::BaseNode>( MatchFunction, VisualGraph::SearchMode::Recursive, VisualGraph::SearchTypeMatch::Derived );
        for ( auto pNode : foundNodes )
        {
            NavigationTarget::CreateNavigationTargets( pNode, m_navigationTargetNodes );
        }

        // Sort
        //-------------------------------------------------------------------------

        auto Comparator = [] ( NavigationTarget const& a, NavigationTarget const& b )
        {
            return a.m_sortKey < b.m_sortKey;
        };

        eastl::sort( m_navigationTargetNodes.begin(), m_navigationTargetNodes.end(), Comparator );
    }

    void AnimationGraphWorkspace::GenerateActiveTargetList()
    {
        EE_ASSERT( IsDebugging() && m_userContext.HasDebugData() );

        m_navigationActiveTargetNodes.clear();

        for ( auto activeNodeIdx : m_userContext.GetActiveNodes() )
        {
            UUID const nodeID = m_userContext.GetGraphNodeUUID( activeNodeIdx );
            if ( nodeID.IsValid() )
            {
                auto pFoundNode = GetMainGraphData()->m_graphDefinition.GetRootGraph()->FindNode( nodeID, true );
                if ( pFoundNode != nullptr )
                {
                    NavigationTarget::CreateNavigationTargets( pFoundNode, m_navigationActiveTargetNodes );
                }
            }
        }
    }

    void AnimationGraphWorkspace::DrawNavigationDialogWindow( UpdateContext const& context )
    {
        // Filter
        //-------------------------------------------------------------------------

        ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x - 27 - ImGui::GetStyle().ItemSpacing.x );
        m_navigationFilter.DrawAndUpdate( ImGuiX::FilterWidget::TakeInitialFocus );

        auto ApplyFilter = [this] ( NavigationTarget const& target )
        {
            if ( m_navigationFilter.HasFilterSet() )
            {
                bool nameFailedFilter = false;
                if ( !m_navigationFilter.MatchesFilter( target.m_label ) )
                {
                    nameFailedFilter = true;
                }

                bool pathFailedFilter = false;
                if ( !m_navigationFilter.MatchesFilter( target.m_path ) )
                {
                    pathFailedFilter = true;
                }

                if ( pathFailedFilter && nameFailedFilter )
                {
                    return false;
                }
            }

            return true;
        };

        // Active targets
        //-------------------------------------------------------------------------

        if ( IsDebugging() && m_userContext.HasDebugData() )
        {
            GenerateActiveTargetList();
        }

        // Table
        //-------------------------------------------------------------------------

        int32_t const numActiveTargets = (int32_t) m_navigationActiveTargetNodes.size();
        int32_t const numTargets = (int32_t) m_navigationTargetNodes.size();
        int32_t const numTotalTargets = numActiveTargets + numTargets;

        if ( ImGui::BeginTable( "NavTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2( 0, 0 ) ) )
        {
            ImGui::TableSetupColumn( "##Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 20 );
            ImGui::TableSetupColumn( "Node", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Path", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupScrollFreeze( 0, 1 );
            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            for ( int32_t i = 0; i < numTotalTargets; i++ )
            {
                NavigationTarget* pTarget = nullptr;
                bool isActiveTarget = false;
                if ( i >= numActiveTargets )
                {
                    pTarget = &m_navigationTargetNodes[i - numActiveTargets];
                }
                else
                {
                    pTarget = &m_navigationActiveTargetNodes[i];
                    isActiveTarget = true;
                }

                EE_ASSERT( pTarget != nullptr );
                EE_ASSERT( pTarget->m_pNode != nullptr && pTarget->m_type != NavigationTarget::Type::Unknown );

                //-------------------------------------------------------------------------

                // Skip nodes that dont match the filter
                if ( !ApplyFilter( *pTarget ) )
                {
                    continue;
                }

                // Skip nodes already shown in the active list
                if ( !isActiveTarget && VectorContains( m_navigationActiveTargetNodes, *pTarget ) )
                {
                    continue;
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextRow();

                ImColor const colors[] = { ImGuiX::ImColors::Plum, ImGuiX::Style::s_colorText, ImGuiX::ImColors::LightSteelBlue, ImGuiX::ImColors::GoldenRod };
                static char const* const icons[] = { EE_ICON_ARROW_RIGHT_BOLD, EE_ICON_WALK, EE_ICON_NUMERIC, EE_ICON_TAG_TEXT };
                static char const* const tooltips[] = { "Parameter", "Pose", "Value", "ID" };
                int32_t const typeIndex = ( int32_t ) pTarget->m_type;

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::PushStyleColor( ImGuiCol_Text, isActiveTarget ? ImGuiX::ImColors::Lime.Value : ImGuiX::Style::s_colorText.Value );
                ImGui::Text( icons[typeIndex] );
                ImGuiX::TextTooltip( tooltips[typeIndex] );
                ImGui::PopStyleColor();

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::PushStyleColor( ImGuiCol_Text, colors[typeIndex].Value );

                if ( ImGui::Selectable( pTarget->m_label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns ) )
                {
                    if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                    {
                        NavigateTo( const_cast<VisualGraph::BaseNode*>( pTarget->m_pNode ) );
                        m_activeOperation = GraphOperationType::None;
                    }
                }

                ImGui::PopStyleColor();

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::Text( pTarget->m_path.c_str() );
            }

            //-------------------------------------------------------------------------

            ImGui::EndTable();
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
        m_preEditEventBindingID = m_propertyGrid.OnPreEdit().Bind( [this] ( PropertyEditInfo const& info ) { GetMainGraphData()->m_graphDefinition.GetRootGraph()->BeginModification(); } );
        m_postEditEventBindingID = m_propertyGrid.OnPostEdit().Bind( [this] ( PropertyEditInfo const& info ) { GetMainGraphData()->m_graphDefinition.GetRootGraph()->EndModification(); } );

    }

    void AnimationGraphWorkspace::ShutdownPropertyGrid()
    {
        m_propertyGrid.OnPreEdit().Unbind( m_preEditEventBindingID );
        m_propertyGrid.OnPostEdit().Unbind( m_postEditEventBindingID );
    }

    //-------------------------------------------------------------------------
    // Graph Log
    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::DrawGraphLog( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        int32_t windowFlags = 0;
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 4 ) );
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_graphLogWindowName.c_str(), nullptr, windowFlags ) )
        {
            if ( IsDebugging() && m_pDebugGraphInstance != nullptr )
            {
                if ( ImGui::BeginTable( "RTLog", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2( 0, 0 ) ) )
                {
                    ImGui::TableSetupColumn( "##Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 20 );
                    ImGui::TableSetupColumn( "Message", ImGuiTableColumnFlags_WidthStretch );
                    ImGui::TableSetupScrollFreeze( 0, 1 );

                    //-------------------------------------------------------------------------

                    ImGui::TableHeadersRow();

                    //-------------------------------------------------------------------------

                    auto const& runtimeLog = m_pDebugGraphInstance->GetLog();

                    //-------------------------------------------------------------------------

                    ImGuiListClipper clipper;
                    clipper.Begin( (int32_t) runtimeLog.size() );
                    while ( clipper.Step() )
                    {
                        for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                        {
                            auto const& entry = runtimeLog[i];

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

                                default:
                                break;
                            }

                            //-------------------------------------------------------------------------

                            ImGui::TableSetColumnIndex( 1 );
                            if ( ImGui::Selectable( entry.m_message.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick ) )
                            {
                                if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                                {
                                    UUID const nodeID = m_userContext.GetGraphNodeUUID( entry.m_nodeIdx );
                                    if ( nodeID.IsValid() )
                                    {
                                        auto pFoundNode = GetMainGraphData()->m_graphDefinition.GetRootGraph()->FindNode( nodeID, true );
                                        if ( pFoundNode != nullptr )
                                        {
                                            NavigateTo( pFoundNode );
                                        }

                                        break;
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
            }
            else
            {
                if ( m_visualLog.empty() )
                {
                    ImGui::TextColored( ImGuiX::ImColors::LimeGreen, EE_ICON_CHECK );
                    ImGui::SameLine();
                    ImGui::Text( "Graph Compiled Successfully" );
                }
                else
                {
                    ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 6 ) );
                    if ( ImGui::BeginTable( "CLog", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2( 0, 0 ) ) )
                    {
                        ImGui::TableSetupColumn( "##Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 20 );
                        ImGui::TableSetupColumn( "Message", ImGuiTableColumnFlags_WidthStretch );
                        ImGui::TableSetupScrollFreeze( 0, 1 );

                        //-------------------------------------------------------------------------

                        ImGui::TableHeadersRow();

                        //-------------------------------------------------------------------------

                        ImGuiListClipper clipper;
                        clipper.Begin( (int32_t) m_visualLog.size() );
                        while ( clipper.Step() )
                        {
                            for ( int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                            {
                                auto const& entry = m_visualLog[i];

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

                                    default:
                                    break;
                                }

                                //-------------------------------------------------------------------------

                                ImGui::TableSetColumnIndex( 1 );
                                if ( ImGui::Selectable( entry.m_message.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick ) )
                                {
                                    if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                                    {
                                        auto pFoundNode = GetMainGraphData()->m_graphDefinition.GetRootGraph()->FindNode( entry.m_nodeID, true );
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
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    //-------------------------------------------------------------------------
    // Control Parameter Editor
    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::DrawControlParameterEditor( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        int32_t windowFlags = 0;
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 4 ) );
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_controlParametersWindowName.c_str(), nullptr, windowFlags ) )
        {
            if ( IsDebugging() && m_userContext.HasDebugData() )
            {
                if ( m_previewParameterStates.empty() )
                {
                    CreateControlParameterPreviewStates();
                }

                ImGui::BeginDisabled( IsReviewingRecording() );
                DrawPreviewParameterList( context );
                ImGui::EndDisabled();
            }
            else
            {
                if ( !m_previewParameterStates.empty() )
                {
                    DestroyControlParameterPreviewStates();
                }

                DrawParameterList();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void AnimationGraphWorkspace::InitializeControlParameterEditor()
    {
        RefreshControlParameterCache();
    }

    void AnimationGraphWorkspace::ShutdownControlParameterEditor()
    {
        DestroyControlParameterPreviewStates();
    }

    void AnimationGraphWorkspace::RefreshControlParameterCache()
    {
        m_controlParameters.clear();
        m_virtualParameters.clear();

        auto pRootGraph = GetMainGraphData()->m_graphDefinition.GetRootGraph();
        m_controlParameters = pRootGraph->FindAllNodesOfType<GraphNodes::ControlParameterToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
        m_virtualParameters = pRootGraph->FindAllNodesOfType<GraphNodes::VirtualParameterToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Exact );

        RefreshParameterCategoryTree();
    }

    void AnimationGraphWorkspace::RefreshParameterCategoryTree()
    {
        m_parameterCategoryTree.RemoveAllItems();

        for ( auto pControlParameter : m_controlParameters )
        {
            m_parameterCategoryTree.AddItem( pControlParameter->GetParameterCategory(), pControlParameter->GetName(), pControlParameter );
        }

        for ( auto pVirtualParameter : m_virtualParameters )
        {
            m_parameterCategoryTree.AddItem( pVirtualParameter->GetParameterCategory(), pVirtualParameter->GetName(), pVirtualParameter );
        }

        m_parameterCategoryTree.RemoveEmptyCategories();

        //-------------------------------------------------------------------------

        m_cachedNumUses.clear();

        auto pRootGraph = GetMainGraphData()->m_graphDefinition.GetRootGraph();
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
    }

    void AnimationGraphWorkspace::DrawCreateOrRenameParameterDialogWindow()
    {
        EE_ASSERT( m_activeOperation == GraphOperationType::CreateParameter || m_activeOperation == GraphOperationType::RenameParameter );

        constexpr static float const fieldOffset = 80;
        bool updateConfirmed = false;

        ImGui::AlignTextToFramePadding();
        ImGui::Text( "Name: " );
        ImGui::SameLine( fieldOffset );

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
        ImGui::SameLine( fieldOffset );

        float const categoryFieldWidth = ImGui::GetContentRegionAvail().x - 30;
        ImGui::SetNextItemWidth( categoryFieldWidth );
        if ( ImGui::InputText( "##ParameterCategory", m_parameterCategoryBuffer, 255, ImGuiInputTextFlags_EnterReturnsTrue ) )
        {
            updateConfirmed = true;
        }

        ImGui::SameLine( fieldOffset + categoryFieldWidth + ImGui::GetStyle().ItemSpacing.x );
        if ( ImGui::BeginCombo( "##CategorySelector", nullptr, ImGuiComboFlags_NoPreview ) )
        {
            for ( auto const& category : m_parameterCategoryTree.GetRootCategory().m_childCategories )
            {
                if ( ImGui::MenuItem( category.m_name.c_str() ) )
                {
                    strncpy_s( m_parameterCategoryBuffer, category.m_name.c_str(), 255 );
                }
            }

            ImGui::EndCombo();
        }

        ImGui::NewLine();

        float const dialogWidth = ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ).x;
        ImGui::SameLine( 0, dialogWidth - 104 );

        //-------------------------------------------------------------------------

        // Rename existing parameter
        if ( m_activeOperation == GraphOperationType::RenameParameter )
        {
            if ( auto pControlParameter = FindControlParameter( m_currentOperationParameterID ) )
            {
                bool const isTheSame = ( pControlParameter->GetParameterName().comparei( m_parameterNameBuffer ) == 0 && pControlParameter->GetParameterCategory().comparei( m_parameterCategoryBuffer ) == 0 );

                ImGui::BeginDisabled( isTheSame );
                if ( ImGui::Button( "Ok", ImVec2( 50, 0 ) ) || updateConfirmed )
                {
                    RenameParameter( m_currentOperationParameterID, m_parameterNameBuffer, m_parameterCategoryBuffer );
                    m_activeOperation = GraphOperationType::None;
                }
                ImGui::EndDisabled();
            }
            else
            {
                auto pVirtualParameter = FindVirtualParameter( m_currentOperationParameterID );
                EE_ASSERT( pVirtualParameter != nullptr );

                bool const isTheSame = ( pVirtualParameter->GetParameterName().comparei( m_parameterNameBuffer ) == 0 && pVirtualParameter->GetParameterCategory().comparei( m_parameterCategoryBuffer ) == 0 );

                ImGui::BeginDisabled( isTheSame );
                if ( ImGui::Button( "Ok", ImVec2( 50, 0 ) ) || updateConfirmed )
                {
                    RenameParameter( m_currentOperationParameterID, m_parameterNameBuffer, m_parameterCategoryBuffer );
                    m_activeOperation = GraphOperationType::None;
                }
                ImGui::EndDisabled();
            }
        }
        else // Create new parameter
        {
            if ( ImGui::Button( "Ok", ImVec2( 50, 0 ) ) || updateConfirmed )
            {
                CreateParameter( m_currentOperationParameterType, m_currentOperationParameterValueType, m_parameterNameBuffer, m_parameterCategoryBuffer );
                m_activeOperation = GraphOperationType::None;
                m_currentOperationParameterValueType = GraphValueType::Unknown;
            }
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
            DestroyParameter( m_currentOperationParameterID );
            m_activeOperation = GraphOperationType::None;
        }

        ImGui::SameLine( 0, 4 );

        if ( ImGui::Button( "No", ImVec2( 30, 0 ) ) )
        {
            m_activeOperation = GraphOperationType::None;
        }
    }

    void AnimationGraphWorkspace::ControlParameterCategoryDragAndDropHandler( Category<GraphNodes::FlowToolsNode*>& category )
    {
        InlineString payloadString;

        //-------------------------------------------------------------------------

        if ( ImGui::BeginDragDropTarget() )
        {
            if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( "AnimGraphParameter", ImGuiDragDropFlags_AcceptBeforeDelivery ) )
            {
                if ( payload->IsDelivery() )
                {
                    payloadString = (char*) payload->Data;
                }
            }
            ImGui::EndDragDropTarget();
        }

        //-------------------------------------------------------------------------

        if ( !payloadString.empty() )
        {
            for ( auto pControlParameter : m_controlParameters )
            {
                if ( pControlParameter->GetParameterName().comparei( payloadString.c_str() ) == 0 )
                {
                    if ( pControlParameter->GetCategory() != category.m_name )
                    {
                        RenameParameter( pControlParameter->GetID(), pControlParameter->GetParameterName(), category.m_name );
                        return;
                    }
                }
            }

            for ( auto pVirtualParameter : m_virtualParameters )
            {
                if ( pVirtualParameter->GetParameterName().comparei( payloadString.c_str() ) == 0 )
                {
                    if ( pVirtualParameter->GetCategory() != category.m_name )
                    {
                        RenameParameter( pVirtualParameter->GetID(), pVirtualParameter->GetParameterName(), category.m_name );
                        return;
                    }
                }
            }
        }
    }

    void AnimationGraphWorkspace::DrawParameterList()
    {
        // Draw Control Bar
        //-------------------------------------------------------------------------

        if ( ImGui::Button( "Add New Parameter", ImVec2( -1, 0 ) ) )
        {
            ImGui::OpenPopup( "AddParameterPopup" );
        }

        if ( ImGui::BeginPopup( "AddParameterPopup" ) )
        {
            ImGuiX::TextSeparator( "Control Parameters" );

            if ( ImGui::MenuItem( "Control Parameter - Bool" ) )
            {
                StartParameterCreate( GraphValueType::Bool, ParameterType::Default );
            }

            if ( ImGui::MenuItem( "Control Parameter - ID" ) )
            {
                StartParameterCreate( GraphValueType::ID, ParameterType::Default );
            }

            if ( ImGui::MenuItem( "Control Parameter - Int" ) )
            {
                StartParameterCreate( GraphValueType::Int, ParameterType::Default );
            }

            if ( ImGui::MenuItem( "Control Parameter - Float" ) )
            {
                StartParameterCreate( GraphValueType::Float, ParameterType::Default );
            }

            if ( ImGui::MenuItem( "Control Parameter - Vector" ) )
            {
                StartParameterCreate( GraphValueType::Vector, ParameterType::Default );
            }

            if ( ImGui::MenuItem( "Control Parameter - Target" ) )
            {
                StartParameterCreate( GraphValueType::Target, ParameterType::Default );
            }

            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "Virtual Parameters" );

            if ( ImGui::MenuItem( "Virtual Parameter - Bool" ) )
            {
                StartParameterCreate( GraphValueType::Bool, ParameterType::Virtual );
            }

            if ( ImGui::MenuItem( "Virtual Parameter - ID" ) )
            {
                StartParameterCreate( GraphValueType::ID, ParameterType::Virtual );
            }

            if ( ImGui::MenuItem( "Virtual Parameter - Int" ) )
            {
                StartParameterCreate( GraphValueType::Int, ParameterType::Virtual );
            }

            if ( ImGui::MenuItem( "Virtual Parameter - Float" ) )
            {
                StartParameterCreate( GraphValueType::Float, ParameterType::Virtual );
            }

            if ( ImGui::MenuItem( "Virtual Parameter - Vector" ) )
            {
                StartParameterCreate( GraphValueType::Vector, ParameterType::Virtual );
            }

            if ( ImGui::MenuItem( "Virtual Parameter - Target" ) )
            {
                StartParameterCreate( GraphValueType::Target, ParameterType::Virtual );
            }

            if ( ImGui::MenuItem( "Virtual Parameter - Bone Mask" ) )
            {
                StartParameterCreate( GraphValueType::BoneMask, ParameterType::Virtual );
            }

            ImGui::EndPopup();
        }

        // Draw Parameters
        //-------------------------------------------------------------------------

        constexpr float const iconWidth = 20;

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
                if ( ImGui::MenuItem( EE_ICON_IDENTIFIER" Copy ID" ) )
                {
                    ImGui::SetClipboardText( pControlParameter->GetParameterID().c_str() );
                }

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
                    NavigateTo( pVirtualParameter->GetChildGraph() );
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

        // Uncategorized
        //-------------------------------------------------------------------------

        ImGui::SetNextItemOpen( !m_parameterCategoryTree.GetRootCategory().m_isCollapsed );
        if ( ImGui::CollapsingHeader( "General" ) )
        {
            if ( !m_parameterCategoryTree.GetRootCategory().m_items.empty() )
            {
                if ( ImGui::BeginTable( "CPGT", 3, 0 ) )
                {
                    ImGui::TableSetupColumn( "##Name", ImGuiTableColumnFlags_WidthStretch );
                    ImGui::TableSetupColumn( "##Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40 );
                    ImGui::TableSetupColumn( "##Uses", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 20 );

                    for ( auto const& item : m_parameterCategoryTree.GetRootCategory().m_items )
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

                    ImGui::EndTable();
                }

                ControlParameterCategoryDragAndDropHandler( m_parameterCategoryTree.GetRootCategory() );
            }

            m_parameterCategoryTree.GetRootCategory().m_isCollapsed = false;
        }
        else
        {
            m_parameterCategoryTree.GetRootCategory().m_isCollapsed = true;
        }

        // Child Categories
        //-------------------------------------------------------------------------

        for ( auto& category : m_parameterCategoryTree.GetRootCategory().m_childCategories )
        {
            ImGui::SetNextItemOpen( !category.m_isCollapsed );
            if ( ImGui::CollapsingHeader( category.m_name.c_str() ) )
            {
                if ( !category.m_items.empty() )
                {
                    if ( ImGui::BeginTable( "CPT", 3, 0 ) )
                    {
                        ImGui::TableSetupColumn( "##Name", ImGuiTableColumnFlags_WidthStretch );
                        ImGui::TableSetupColumn( "##Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40 );
                        ImGui::TableSetupColumn( "##Uses", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 20 );

                        //-------------------------------------------------------------------------

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

                        ImGui::EndTable();
                    }

                    ControlParameterCategoryDragAndDropHandler( category );
                }

                category.m_isCollapsed = false;
            }
            else
            {
                category.m_isCollapsed = true;
            }
        }
    }

    void AnimationGraphWorkspace::DrawPreviewParameterList( UpdateContext const& context )
    {
        ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );

        //-------------------------------------------------------------------------

        auto DrawControlParameterEditorRow = [this, &context] ( ControlParameterPreviewState* pPreviewState )
        {
            auto pControlParameter = pPreviewState->m_pParameter;

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
            pPreviewState->DrawPreviewEditor( context, m_graphStack.front(), m_characterTransform, IsLiveDebugging() );
            ImGui::PopID();
        };

        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 4 ) );

        // Uncategorized
        //-------------------------------------------------------------------------

        ImGui::SetNextItemOpen( !m_previewParameterCategoryTree.GetRootCategory().m_isCollapsed );
        if ( ImGui::CollapsingHeader( "General" ) )
        {
            if ( ImGui::BeginTable( "Parameter Preview", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerH ) )
            {
                ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupColumn( "Editor", ImGuiTableColumnFlags_WidthStretch );

                for ( auto const& item : m_previewParameterCategoryTree.GetRootCategory().m_items )
                {
                    DrawControlParameterEditorRow( item.m_data );
                }

                ImGui::EndTable();
            }

            m_previewParameterCategoryTree.GetRootCategory().m_isCollapsed = false;
        }
        else
        {
            m_previewParameterCategoryTree.GetRootCategory().m_isCollapsed = true;
        }

        // Child Categories
        //-------------------------------------------------------------------------

        for ( auto& category : m_previewParameterCategoryTree.GetRootCategory().m_childCategories )
        {
            ImGui::SetNextItemOpen( !category.m_isCollapsed );
            if ( ImGui::CollapsingHeader( category.m_name.c_str() ) )
            {
                if ( ImGui::BeginTable( "Parameter Preview", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerH ) )
                {
                    ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_WidthStretch );
                    ImGui::TableSetupColumn( "Editor", ImGuiTableColumnFlags_WidthStretch );

                    //-------------------------------------------------------------------------

                    for ( auto const& item : category.m_items )
                    {
                        DrawControlParameterEditorRow( item.m_data );
                    }

                    ImGui::EndTable();
                }

                category.m_isCollapsed = false;
            }
            else
            {
                category.m_isCollapsed = true;
            }
        }

        //-------------------------------------------------------------------------

        ImGui::PopStyleVar();
    }

    void AnimationGraphWorkspace::CreateParameter( ParameterType parameterType, GraphValueType valueType, String const& desiredParameterName, String const& desiredCategoryName )
    {
        String finalParameterName = desiredParameterName;
        EnsureUniqueParameterName( finalParameterName );

        String finalCategoryName = desiredCategoryName;
        ResolveCategoryName( finalCategoryName );

        //-------------------------------------------------------------------------

        auto pRootGraph = GetMainGraphData()->m_graphDefinition.GetRootGraph();
        VisualGraph::ScopedGraphModification gm( pRootGraph );

        if ( parameterType == ParameterType::Default )
        {
            GraphNodes::ControlParameterToolsNode* pParameter = nullptr;
            switch ( valueType )
            {
                case GraphValueType::Bool:
                pParameter = pRootGraph->CreateNode<GraphNodes::BoolControlParameterToolsNode>( finalParameterName, finalCategoryName );
                break;

                case GraphValueType::ID:
                pParameter = pRootGraph->CreateNode<GraphNodes::IDControlParameterToolsNode>( finalParameterName, finalCategoryName );
                break;

                case GraphValueType::Int:
                pParameter = pRootGraph->CreateNode<GraphNodes::IntControlParameterToolsNode>( finalParameterName, finalCategoryName );
                break;

                case GraphValueType::Float:
                pParameter = pRootGraph->CreateNode<GraphNodes::FloatControlParameterToolsNode>( finalParameterName, finalCategoryName );
                break;

                case GraphValueType::Vector:
                pParameter = pRootGraph->CreateNode<GraphNodes::VectorControlParameterToolsNode>( finalParameterName, finalCategoryName );
                break;

                case GraphValueType::Target:
                pParameter = pRootGraph->CreateNode<GraphNodes::TargetControlParameterToolsNode>( finalParameterName, finalCategoryName );
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                }
                break;
            }

            EE_ASSERT( pParameter != nullptr );
            m_controlParameters.emplace_back( pParameter );
        }
        else
        {
            auto pParameter = pRootGraph->CreateNode<GraphNodes::VirtualParameterToolsNode>( valueType, finalParameterName, finalCategoryName );
            m_virtualParameters.emplace_back( pParameter );
        }

        RefreshParameterCategoryTree();
    }

    void AnimationGraphWorkspace::RenameParameter( UUID parameterID, String const& desiredParameterName, String const& desiredCategoryName )
    {
        String originalParameterName;
        String originalParameterCategory;

        auto pControlParameter = FindControlParameter( parameterID );
        auto pVirtualParameter = FindVirtualParameter( parameterID );
        EE_ASSERT( pControlParameter != nullptr || pVirtualParameter != nullptr );
        EE_ASSERT( pControlParameter == nullptr || pVirtualParameter == nullptr );

        if ( pControlParameter != nullptr )
        {
            originalParameterName = pControlParameter->GetParameterName();
            originalParameterCategory = pControlParameter->GetParameterCategory();
        }
        else
        {
            originalParameterName = pVirtualParameter->GetParameterName();
            originalParameterCategory = pVirtualParameter->GetParameterCategory();
        }

        // Check if we actually need to do anything
        bool const requiresNameChange = originalParameterName.comparei( desiredParameterName ) != 0;
        if ( !requiresNameChange && originalParameterCategory.comparei( desiredCategoryName ) == 0 )
        {
            return;
        }

        //-------------------------------------------------------------------------

        String finalParameterName = desiredParameterName;
        if ( requiresNameChange )
        {
            EnsureUniqueParameterName( finalParameterName );
        }

        //-------------------------------------------------------------------------

        String finalCategoryName = desiredCategoryName;
        ResolveCategoryName( finalCategoryName );

        //-------------------------------------------------------------------------

        auto pRootGraph = GetMainGraphData()->m_graphDefinition.GetRootGraph();
        VisualGraph::ScopedGraphModification gm( pRootGraph );
        if ( pControlParameter != nullptr )
        {
            pControlParameter->Rename( finalParameterName, finalCategoryName );
        }
        else
        {
            pVirtualParameter->Rename( finalParameterName, finalCategoryName );
        }
    }

    void AnimationGraphWorkspace::DestroyParameter( UUID parameterID )
    {
        auto pControlParameter = FindControlParameter( parameterID );
        auto pVirtualParameter = FindVirtualParameter( parameterID );
        EE_ASSERT( pControlParameter != nullptr || pVirtualParameter != nullptr );
        EE_ASSERT( pControlParameter == nullptr || pVirtualParameter == nullptr );

        ClearSelection();

        // Navigate away from the virtual parameter graph if we are currently viewing it
        if ( pVirtualParameter != nullptr )
        {
            auto pParentNode = m_primaryGraphView.GetViewedGraph()->GetParentNode();
            if ( pParentNode != nullptr && pParentNode->GetID() == parameterID )
            {
                NavigateTo( GetMainGraphData()->m_graphDefinition.GetRootGraph() );
            }
        }

        //-------------------------------------------------------------------------

        auto pRootGraph = GetMainGraphData()->m_graphDefinition.GetRootGraph();
        VisualGraph::ScopedGraphModification gm( pRootGraph );

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
        if ( pControlParameter != nullptr )
        {
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
        else
        {
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

        //-------------------------------------------------------------------------

        RefreshParameterCategoryTree();
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
                if ( pParameter->GetParameterName().comparei( tempString ) == 0 )
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
                    if ( pParameter->GetParameterName().comparei( tempString ) == 0 )
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

    void AnimationGraphWorkspace::ResolveCategoryName( String& desiredCategoryName ) const
    {
        for ( auto const& category : m_parameterCategoryTree.GetRootCategory().m_childCategories )
        {
            if ( category.m_name.comparei( desiredCategoryName ) == 0 )
            {
                desiredCategoryName = category.m_name;
                return;
            }
        }
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
                    m_previewParameterStates.emplace_back( EE::New<BoolParameterState>( pControlParameter ) );
                }
                break;

                case GraphValueType::Int:
                {
                    m_previewParameterStates.emplace_back( EE::New<IntParameterState>( pControlParameter ) );
                }
                break;

                case GraphValueType::Float:
                {
                    m_previewParameterStates.emplace_back( EE::New<FloatParameterState>( pControlParameter ) );
                }
                break;

                case GraphValueType::Vector:
                {
                    m_previewParameterStates.emplace_back( EE::New<VectorParameterState>( pControlParameter ) );
                }
                break;

                case GraphValueType::ID:
                {
                    m_previewParameterStates.emplace_back( EE::New<IDParameterState>( pControlParameter ) );
                }
                break;

                case GraphValueType::Target:
                {
                    m_previewParameterStates.emplace_back( EE::New<TargetParameterState>( pControlParameter ) );
                }
                break;

                default:
                EE_UNREACHABLE_CODE();
                break;
            }
        }

        //-------------------------------------------------------------------------

        EE_ASSERT( m_previewParameterCategoryTree.GetRootCategory().IsEmpty() );

        for ( auto pPreviewState : m_previewParameterStates )
        {
            auto pControlParameter = pPreviewState->m_pParameter;
            m_previewParameterCategoryTree.AddItem( pControlParameter->GetParameterCategory(), pControlParameter->GetName(), pPreviewState );
        }
    }

    void AnimationGraphWorkspace::DestroyControlParameterPreviewStates()
    {
        m_previewParameterCategoryTree.Clear();

        for ( auto& pPreviewState : m_previewParameterStates )
        {
            EE::Delete( pPreviewState );
        }

        m_previewParameterStates.clear();
    }

    void AnimationGraphWorkspace::StartParameterCreate( GraphValueType valueType, ParameterType parameterType )
    {
        m_currentOperationParameterID = UUID();
        m_activeOperation = GraphOperationType::CreateParameter;
        m_currentOperationParameterValueType = valueType;
        m_currentOperationParameterType = parameterType;

        Memory::MemsetZero( m_parameterNameBuffer, 255 );
        Memory::MemsetZero( m_parameterCategoryBuffer, 255 );
        strncpy_s( m_parameterNameBuffer, "Parameter", 9 );
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
        strncpy_s( m_nameBuffer, "NewChildVariation", 255 );
    }

    void AnimationGraphWorkspace::StartRename( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() );
        m_activeOperationVariationID = variationID;
        m_activeOperation = GraphOperationType::RenameVariation;
        strncpy_s( m_nameBuffer, variationID.c_str(), 255 );
    }

    void AnimationGraphWorkspace::StartDelete( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() );
        m_activeOperationVariationID = variationID;
        m_activeOperation = GraphOperationType::DeleteVariation;
    }

    void AnimationGraphWorkspace::CreateVariation( StringID newVariationID, StringID parentVariationID )
    {
        auto pRootGraph = GetMainGraphData()->m_graphDefinition.GetRootGraph();
        auto& variationHierarchy = GetMainGraphData()->m_graphDefinition.GetVariationHierarchy();

        EE_ASSERT( !variationHierarchy.IsValidVariation( newVariationID ) );

        // Create new variation
        VisualGraph::ScopedGraphModification gm( pRootGraph );
        variationHierarchy.CreateVariation( newVariationID, parentVariationID );

        // Switch to the new variation
        SetSelectedVariation( newVariationID );
    }

    void AnimationGraphWorkspace::RenameVariation( StringID oldVariationID, StringID newVariationID )
    {
        auto pRootGraph = GetMainGraphData()->m_graphDefinition.GetRootGraph();
        auto& variationHierarchy = GetMainGraphData()->m_graphDefinition.GetVariationHierarchy();
        bool const isRenamingCurrentlySelectedVariation = ( m_activeOperationVariationID == GetMainGraphData()->m_selectedVariationID );

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
        auto pRootGraph = GetMainGraphData()->m_graphDefinition.GetRootGraph();
        VisualGraph::ScopedGraphModification gm( pRootGraph );

        auto& variationHierarchy = GetMainGraphData()->m_graphDefinition.GetVariationHierarchy();
        variationHierarchy.DestroyVariation( m_activeOperationVariationID );
    }

    void AnimationGraphWorkspace::DrawVariationTreeNode( VariationHierarchy const& variationHierarchy, StringID variationID )
    {
        ImGui::PushID( variationID.GetID() );

        auto const childVariations = variationHierarchy.GetChildVariations( variationID );
        bool const isSelected = GetMainGraphData()->m_selectedVariationID == variationID;

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
            ImGuiX::ScopedFont const sf( isSelected ? ImGuiX::Font::MediumBold : ImGuiX::Font::Medium, isSelected ? ImGuiX::ImColors::LimeGreen : ImGui::GetStyle().Colors[ImGuiCol_Text] );
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

    void AnimationGraphWorkspace::DrawVariationSelector( float width )
    {
        ImGui::BeginDisabled( IsInReadOnlyState() );
        ImGui::SetNextItemWidth( width );
        if ( ImGui::BeginCombo( "##VariationSelector", GetMainGraphData()->m_selectedVariationID.c_str() ) )
        {
            auto& variationHierarchy = GetMainGraphData()->m_graphDefinition.GetVariationHierarchy();
            DrawVariationTreeNode( variationHierarchy, Variation::s_defaultVariationID );

            ImGui::EndCombo();
        }
        ImGuiX::ItemTooltip( "Variation Selector - Right click on variations for more options!" );
        ImGui::EndDisabled();
    }

    void AnimationGraphWorkspace::DrawOverridesTable()
    {
        ImGui::BeginDisabled( IsInReadOnlyState() );

        //-------------------------------------------------------------------------

        auto pRootGraph = GetMainGraphData()->m_graphDefinition.GetRootGraph();
        auto dataSlotNodes = pRootGraph->FindAllNodesOfType<GraphNodes::DataSlotToolsNode>( VisualGraph::SearchMode::Recursive, VisualGraph::SearchTypeMatch::Derived );
        bool isDefaultVariationSelected = IsDefaultVariationSelected();

        // Skeleton Picker
        //-------------------------------------------------------------------------

        ImGui::AlignTextToFramePadding();
        ResourcePath newPath;
        auto pVariation = GetMainGraphData()->m_graphDefinition.GetVariation( GetMainGraphData()->m_selectedVariationID );
        if ( m_resourcePicker.DrawPicker( pVariation->m_skeleton, newPath ) )
        {
            VisualGraph::ScopedGraphModification sgm( pRootGraph );

            if ( newPath.IsValid() )
            {
                pVariation->m_skeleton = ResourceID( newPath );
            }
            else
            {
                pVariation->m_skeleton.Clear();
            }
        }

        // Filter
        //-------------------------------------------------------------------------

        ImGui::Separator();
        m_variationFilter.DrawAndUpdate();

        // Overrides
        //-------------------------------------------------------------------------

        ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );
        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 2 ) );
        if ( ImGui::BeginTable( "SourceTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX ) )
        {
            ImGui::TableSetupColumn( "##Override", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 26 );
            ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Path", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Source", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            StringID const currentVariationID = GetMainGraphData()->m_selectedVariationID;

            for ( auto pDataSlotNode : dataSlotNodes )
            {
                String const nodePath = pDataSlotNode->GetPathFromRoot();

                // Apply filter
                //-------------------------------------------------------------------------

                if ( m_variationFilter.HasFilterSet() )
                {
                    bool nameFailedFilter = false;
                    if ( !m_variationFilter.MatchesFilter( pDataSlotNode->GetName() ) )
                    {
                        nameFailedFilter = true;
                    }

                    bool pathFailedFilter = false;
                    if ( !m_variationFilter.MatchesFilter( nodePath ) )
                    {
                        pathFailedFilter = true;
                    }

                    if ( pathFailedFilter && nameFailedFilter )
                    {
                        continue;
                    }
                }

                //-------------------------------------------------------------------------

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
                        if ( ImGuiX::FlatButtonColored( ImGuiX::ImColors::MediumRed, EE_ICON_CANCEL, buttonSize ) )
                        {
                            pDataSlotNode->RemoveOverride( currentVariationID );
                        }
                        ImGuiX::ItemTooltip( "Remove Override" );
                    }
                    else // Create an override
                    {
                        if ( ImGuiX::FlatButtonColored( ImGuiX::ImColors::LimeGreen, EE_ICON_PLUS, buttonSize ) )
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
                    labelColor = ( pResourceID->IsValid() ) ? ImGuiX::ImColors::Lime : ImGuiX::ImColors::MediumRed;
                }

                ImGui::TextColored( labelColor, pDataSlotNode->GetName() );

                // Node Path
                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                if ( ImGuiX::FlatButton( EE_ICON_MAGNIFY_SCAN"##FN", ImVec2( 24, 24 ) ) )
                {
                    NavigateTo( pDataSlotNode );
                }
                ImGui::SameLine();
                ImGui::Text( nodePath.c_str() );
                ImGuiX::TextTooltip( nodePath.c_str() );

                // Resource Picker
                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();

                // Validate the chosen resource to prevent self-references
                auto ValidateResourceSelected = [this] ( ResourcePath const& path )
                {
                    if ( path.IsValid() )
                    {
                        ResourceID const ID( path );
                        if ( ID.GetResourceTypeID() == GraphDefinition::GetStaticResourceTypeID() || ID.GetResourceTypeID() == GraphVariation::GetStaticResourceTypeID() )
                        {
                            ResourceID const graphResourceID = Variation::GetGraphResourceID( ID );
                            if ( graphResourceID == m_workspaceResource.GetResourceID() )
                            {
                                return false;
                            }
                        }
                    }

                    return true;
                };

                // Default variations always have values created
                if ( isDefaultVariationSelected )
                {
                    if ( m_resourcePicker.DrawPicker( pDataSlotNode->GetDefaultValue(), newPath, pDataSlotNode->GetSlotResourceTypeID() ) )
                    {
                        if ( ValidateResourceSelected( newPath ) )
                        {
                            VisualGraph::ScopedGraphModification sgm( pRootGraph );
                            pDataSlotNode->SetDefaultValue( newPath.IsValid() ? ResourceID( newPath ) : ResourceID() );
                        }
                    }
                }
                else // Child Variation
                {
                    // If we have an override for this variation
                    if ( hasOverrideForVariation )
                    {
                        EE_ASSERT( pResourceID != nullptr );
                        if ( m_resourcePicker.DrawPicker( *pResourceID, newPath, pDataSlotNode->GetSlotResourceTypeID() ) )
                        {
                            if ( ValidateResourceSelected( newPath ) )
                            {
                                VisualGraph::ScopedGraphModification sgm( pRootGraph );
                                pDataSlotNode->SetOverrideValue( currentVariationID, newPath );
                            }
                        }
                    }
                    else // Show inherited value
                    {
                        auto& variationHierarchy = GetMainGraphData()->m_graphDefinition.GetVariationHierarchy();
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
        ImGui::PopStyleVar();

        //-------------------------------------------------------------------------

        ImGui::EndDisabled();
    }

    bool AnimationGraphWorkspace::DrawVariationNameEditor()
    {
        bool const isRenameOp = ( m_activeOperation == GraphOperationType::RenameVariation );
        bool nameChangeConfirmed = false;

        // Validate current buffer
        //-------------------------------------------------------------------------

        auto ValidateVariationName = [this, isRenameOp] ()
        {
            size_t const bufferLen = strlen( m_nameBuffer );

            if ( bufferLen == 0 )
            {
                return false;
            }

            // Check for invalid chars
            for ( auto i = 0; i < bufferLen; i++ )
            {
                if ( !ImGuiX::IsValidNameIDChar( m_nameBuffer[i] ) )
                {
                    return false;
                }
            }

            // Check for existing variations with the same name but different casing
            String newVariationName( m_nameBuffer );
            auto const& variationHierarchy = GetMainGraphData()->m_graphDefinition.GetVariationHierarchy();
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
        ImGui::PushStyleColor( ImGuiCol_Text, isValidVariationName ? ImGui::GetStyle().Colors[ImGuiCol_Text] : ImGuiX::ImColors::Red.Value );
        if ( ImGui::InputText( "##VariationName", m_nameBuffer, 255, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CallbackCharFilter, ImGuiX::FilterNameIDChars ) )
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
            StringID newVariationID( m_nameBuffer );
            CreateVariation( newVariationID, m_activeOperationVariationID );
            m_activeOperationVariationID = StringID();
            m_activeOperation = GraphOperationType::None;
        }
    }

    void AnimationGraphWorkspace::DrawRenameVariationDialogWindow()
    {
        if ( DrawVariationNameEditor() )
        {
            StringID newVariationID( m_nameBuffer );
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
            auto& variationHierarchy = GetMainGraphData()->m_graphDefinition.GetVariationHierarchy();
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

    //-------------------------------------------------------------------------
    // Recording
    //-------------------------------------------------------------------------

    void AnimationGraphWorkspace::DrawRecorderUI( UpdateContext const& context )
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Recorder" ) )
        {
            ImVec2 const buttonSize( 26, 0 );

            // Review / Stop
            //-------------------------------------------------------------------------

            // Stop the review for every workspace
            if ( IsReviewingRecording() )
            {
                if ( ImGuiX::IconButton( EE_ICON_STOP, "##StopReview", ImGuiX::ImColors::Red, buttonSize ) )
                {
                    StopDebugging();
                }
                ImGuiX::ItemTooltip( "Stop Review" );
            }
            else // Show the start/join button
            {
                ImGui::BeginDisabled( !m_updateRecorder.HasRecordedData() );
                if ( ImGuiX::IconButton( EE_ICON_PLAY, "##StartReview", ImGuiX::ImColors::Lime, buttonSize ) )
                {
                    if ( IsDebugging() )
                    {
                        StopDebugging();
                    }

                    DebugTarget debugTarget;
                    debugTarget.m_type = DebugTargetType::Recording;
                    StartDebugging( context, debugTarget );
                }
                ImGuiX::ItemTooltip( "Start Review" );
                ImGui::EndDisabled();
            }

            // Record / Stop
            //-------------------------------------------------------------------------

            ImGui::SameLine();

            ImGui::BeginDisabled( !IsLiveDebugging() );
            if ( m_isRecording )
            {
                if ( ImGuiX::IconButton( EE_ICON_STOP, "##StopRecord", ImGuiX::ImColors::White, buttonSize ) )
                {
                    StopRecording();
                }
                ImGuiX::ItemTooltip( "Stop Recording" );
            }
            else // Show start recording button
            {
                if ( ImGuiX::IconButton( EE_ICON_RECORD, "##Record", ImGuiX::ImColors::Red, buttonSize ) )
                {
                    StartRecording();
                }
                ImGuiX::ItemTooltip( "Record" );
            }
            ImGui::EndDisabled();

            // Timeline
            //-------------------------------------------------------------------------

            ImGui::SameLine();

            ImGui::BeginDisabled( !IsReviewingRecording() || !m_reviewStarted );

            if ( ImGui::Button( EE_ICON_STEP_BACKWARD"##StepFrameBack", buttonSize ) )
            {
                StepReviewBackward();
            }

            ImGui::SameLine();

            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x - ( buttonSize.x + ImGui::GetStyle().ItemSpacing.x ) * 2 );
            int32_t const numFramesRecorded = IsReviewingRecording() ? m_updateRecorder.GetNumRecordedFrames() : 1;
            int32_t frameIdx = IsReviewingRecording() ? m_currentReviewFrameIdx : 0;
            if ( ImGui::SliderInt( "##timeline", &frameIdx, 0, numFramesRecorded - 1 ) )
            {
                SetFrameToReview( frameIdx );
            }

            ImGui::SameLine();

            if ( ImGui::Button( EE_ICON_STEP_FORWARD"##StepFrameForward", buttonSize ) )
            {
                StepReviewForward();
            }

            ImGui::EndDisabled();

            ImGui::SameLine();

            ImGuiX::ToggleButton( EE_ICON_INFORMATION_OUTLINE"##DrawExtraInfo", EE_ICON_INFORMATION_OFF_OUTLINE"##DrawExtraInfo", m_drawExtraRecordingInfo, buttonSize );
            ImGuiX::ItemTooltip( m_drawExtraRecordingInfo ? "Hide extra recording information" : "Show extra recording information" );

            // Frame Info
            //-------------------------------------------------------------------------

            if ( m_drawExtraRecordingInfo )
            {
                if ( IsReviewingRecording() && m_currentReviewFrameIdx >= 0 )
                {
                    ImGui::Indent();
                    ImGui::Text( "Delta Time: %.2fms", m_updateRecorder.m_recordedData[m_currentReviewFrameIdx].m_deltaTime.ToMilliseconds().ToFloat() );

                    Transform const& frameWorldTransform = m_updateRecorder.m_recordedData[m_currentReviewFrameIdx].m_characterWorldTransform;
                    Float3 const angles = frameWorldTransform.GetRotation().ToEulerAngles().GetAsDegrees();
                    Vector const translation = frameWorldTransform.GetTranslation();
                    ImGui::Text( EE_ICON_ROTATE_360" X: %.3f, Y: %.3f, Z: %.3f", angles.m_x, angles.m_y, angles.m_z );
                    ImGui::Text( EE_ICON_AXIS_ARROW" X: %.3f, Y: %.3f, Z: %.3f", translation.GetX(), translation.GetY(), translation.GetZ() );
                    ImGui::Text( EE_ICON_ARROW_EXPAND" %.3f", frameWorldTransform.GetScale() );
                    ImGui::Unindent();
                }
            }
        }
    }

    void AnimationGraphWorkspace::StartRecording()
    {
        EE_ASSERT( IsLiveDebugging() && !m_isRecording );
        EE_ASSERT( m_pDebugGraphInstance != nullptr );

        ClearRecordedData();

        m_pDebugGraphInstance->StartRecording( m_recordedGraphState, &m_updateRecorder );

        m_isRecording = true;
    }

    void AnimationGraphWorkspace::StopRecording()
    {
        EE_ASSERT( IsLiveDebugging() && m_isRecording );
        EE_ASSERT( m_pDebugGraphInstance != nullptr );

        m_pDebugGraphInstance->StopRecording();

        m_isRecording = false;
    }

    void AnimationGraphWorkspace::ClearRecordedData()
    {
        EE_ASSERT( !m_isRecording && !IsReviewingRecording() );
        m_recordedGraphState.Reset();
        m_updateRecorder.Reset();
    }

    void AnimationGraphWorkspace::SetFrameToReview( int32_t newFrameIdx )
    {
        EE_ASSERT( IsReviewingRecording() && m_reviewStarted );
        EE_ASSERT( m_updateRecorder.IsValidRecordedFrameIndex( newFrameIdx ) );
        EE_ASSERT( m_pDebugGraphComponent != nullptr && m_pDebugGraphComponent->IsInitialized() );
        EE_ASSERT( m_pDebugMeshComponent != nullptr );

        if ( m_currentReviewFrameIdx == newFrameIdx )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( newFrameIdx == ( m_currentReviewFrameIdx + 1 ) )
        {
            auto const& frameData = m_updateRecorder.m_recordedData[newFrameIdx];

            // Set parameters
            m_pDebugGraphInstance->SetRecordedUpdateData( frameData );

            // Evaluate graph
            m_pDebugGraphComponent->EvaluateGraph( frameData.m_deltaTime, frameData.m_characterWorldTransform, nullptr );
        }
        else // Re-evaluate the entire graph to the new index point
        {
            // Set initial state
            m_recordedGraphState.PrepareForReading();
            m_pDebugGraphInstance->SetToRecordedState( m_recordedGraphState );

            // Update graph instance till we get to the specified frame
            for ( auto i = 0; i <= newFrameIdx; i++ )
            {
                auto const& frameData = m_updateRecorder.m_recordedData[i];

                // Set parameters
                m_pDebugGraphInstance->SetRecordedUpdateData( frameData );

                // Evaluate graph
                m_pDebugGraphComponent->EvaluateGraph( frameData.m_deltaTime, frameData.m_characterWorldTransform, nullptr );

                // Explicitly end root motion debug update for intermediate steps
                if ( i < ( newFrameIdx - 1 ) )
                {
                    auto const& nextFrameData = m_updateRecorder.m_recordedData[i + 1];
                    m_pDebugGraphInstance->EndRootMotionDebuggerUpdate( nextFrameData.m_characterWorldTransform );
                }
            }
        }

        //-------------------------------------------------------------------------

        // Use the transform from the next frame as the end transform of the character used to evaluate the pose tasks
        int32_t const nextFrameIdx = ( newFrameIdx < m_updateRecorder.GetNumRecordedFrames() - 1 ) ? newFrameIdx + 1 : newFrameIdx;
        auto const& nextRecordedFrameData = m_updateRecorder.m_recordedData[nextFrameIdx];

        // Do we need to evaluate the the pose tasks for this client
        if ( m_pDebugGraphInstance->DoesTaskSystemNeedUpdate() )
        {
            m_pDebugGraphInstance->ExecutePrePhysicsPoseTasks( nextRecordedFrameData.m_characterWorldTransform );
            m_pDebugGraphInstance->ExecutePostPhysicsPoseTasks();
        }

        // Set mesh world Transform and update frame idx
        m_pDebugMeshComponent->SetWorldTransform( nextRecordedFrameData.m_characterWorldTransform );
        m_currentReviewFrameIdx = newFrameIdx;
    }

    void AnimationGraphWorkspace::StepReviewForward()
    {
        EE_ASSERT( IsReviewingRecording() );
        int32_t const newFrameIdx = Math::Min( m_updateRecorder.GetNumRecordedFrames() - 1, m_currentReviewFrameIdx + 1 );
        SetFrameToReview( newFrameIdx );
    }

    void AnimationGraphWorkspace::StepReviewBackward()
    {
        EE_ASSERT( IsReviewingRecording() );
        int32_t const newFrameIdx = Math::Max( 0, m_currentReviewFrameIdx - 1 );
        SetFrameToReview( newFrameIdx );
    }
}