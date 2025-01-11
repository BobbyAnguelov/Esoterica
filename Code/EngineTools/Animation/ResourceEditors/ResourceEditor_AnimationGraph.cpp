#include "ResourceEditor_AnimationGraph.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_Parameters.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_EntryStates.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_GlobalTransitions.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_VariationData.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_StateMachine.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_State.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_ReferencedGraph.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_BoneMasks.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_ExternalGraph.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_StateMachineGraph.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationGraph.h"
#include "EngineTools/PropertyGrid/PropertyGridEditor.h"
#include "EngineTools/PropertyGrid/PropertyGridTypeEditingRules.h"
#include "EngineTools/Core/Dialogs.h"
#include "Engine/Camera/Components/Component_DebugCamera.h"
#include "Engine/Animation/DebugViews/DebugView_Animation.h"
#include "Engine/Animation/Components/Component_AnimationGraph.h"
#include "Engine/Animation/Systems/EntitySystem_Animation.h"
#include "Engine/Animation/Systems/WorldSystem_Animation.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/Entity/EntityWorldUpdateContext.h"
#include "Base/FileSystem/FileSystemUtils.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Input/InputSystem.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------
// Widgets
//-------------------------------------------------------------------------

namespace EE::Animation
{
    AnimationGraphEditor::IDComboWidget::IDComboWidget( AnimationGraphEditor* pGraphEditor )
        : ImGuiX::ComboWithFilterWidget<StringID>( Flags::HidePreview )
        , m_pGraphEditor( pGraphEditor )
    {
        EE_ASSERT( m_pGraphEditor != nullptr );
    }

    AnimationGraphEditor::IDComboWidget::IDComboWidget( AnimationGraphEditor* pGraphEditor, IDControlParameterToolsNode* pControlParameter )
        : ImGuiX::ComboWithFilterWidget<StringID>( Flags::HidePreview )
        , m_pGraphEditor( pGraphEditor )
        , m_pControlParameter( pControlParameter )
    {
        EE_ASSERT( m_pGraphEditor != nullptr && pControlParameter != nullptr );
    }

    void AnimationGraphEditor::IDComboWidget::PopulateOptionsList()
    {
        TVector<StringID> foundIDs;

        // Get all IDs in the graph
        auto pRootGraph = m_pGraphEditor->GetEditedRootGraph();
        auto const foundNodes = pRootGraph->FindAllNodesOfType<NodeGraph::BaseNode>( NodeGraph::SearchMode::Recursive, NodeGraph::SearchTypeMatch::Derived );
        for ( auto pNode : foundNodes )
        {
            if ( auto pStateNode = TryCast<StateToolsNode>( pNode ) )
            {
                pStateNode->GetLogicAndEventIDs( foundIDs );
            }
            else if ( auto pFlowNode = TryCast<FlowToolsNode>( pNode ) )
            {
                pFlowNode->GetLogicAndEventIDs( foundIDs );
            }
        }

        // De-duplicate and remove invalid IDs
        TVector<StringID> sortedIDlist;
        sortedIDlist.clear();

        for ( auto ID : foundIDs )
        {
            if ( !ID.IsValid() )
            {
                continue;
            }

            if ( VectorContains( sortedIDlist, ID ) )
            {
                continue;
            }

            sortedIDlist.emplace_back( ID );
        }

        // Sort
        auto Comparator = [] ( StringID const& a, StringID const& b ) { return strcmp( a.c_str(), b.c_str() ) < 0; };
        eastl::sort( sortedIDlist.begin(), sortedIDlist.end(), Comparator );

        // Add empty option
        auto& emptyOpt = m_options.emplace_back();
        emptyOpt.m_label = "* Clear ID *##IDCWClear";
        emptyOpt.m_filterComparator = "";
        emptyOpt.m_value = StringID();

        // Add parameter options
        if ( m_pControlParameter != nullptr && !m_pControlParameter->GetExpectedPreviewValues().empty() )
        {
            auto& expectedValueSeparator = m_options.emplace_back();
            expectedValueSeparator.m_label = "Expected Values";
            expectedValueSeparator.m_isSeparator = true;

            for ( auto ID : m_pControlParameter->GetExpectedPreviewValues() )
            {
                if ( !ID.IsValid() )
                {
                    continue;
                }

                auto& opt = m_options.emplace_back();
                opt.m_label = ID.c_str();
                opt.m_filterComparator = ID.c_str();
                opt.m_filterComparator.make_lower();
                opt.m_value = ID;
            }

            auto& globalValueSeparator = m_options.emplace_back();
            globalValueSeparator.m_label = "Global Values";
            globalValueSeparator.m_isSeparator = true;
        }

        // Create global options
        for ( auto const& ID : sortedIDlist )
        {
            if ( !ID.IsValid() )
            {
                continue;
            }

            auto& opt = m_options.emplace_back();
            opt.m_label = ID.c_str();
            opt.m_filterComparator = ID.c_str();
            opt.m_filterComparator.make_lower();
            opt.m_value = ID;
        }
    };
}

//-------------------------------------------------------------------------
// Control Parameter Preview States
//-------------------------------------------------------------------------

namespace EE::Animation
{
    AnimationGraphEditor::ControlParameterPreviewState::ControlParameterPreviewState( AnimationGraphEditor* pGraphEditor, ControlParameterToolsNode* pParameter )
        : m_pGraphEditor( pGraphEditor)
        , m_pParameter( pParameter ) 
    {
        EE_ASSERT( m_pGraphEditor != nullptr );
        EE_ASSERT( m_pParameter != nullptr ); 
    }

    inline int16_t AnimationGraphEditor::ControlParameterPreviewState::GetRuntimeGraphNodeIndex( UUID const& nodeID ) const
    {
        EE_ASSERT( m_pGraphEditor != nullptr );
        auto const foundIter = m_pGraphEditor->GetEditedGraphData()->m_nodeIDtoIndexMap.find( nodeID );
        if ( foundIter != m_pGraphEditor->GetEditedGraphData()->m_nodeIDtoIndexMap.end() )
        {
            return foundIter->second;
        }

        return InvalidIndex;
    }

    //-------------------------------------------------------------------------

    class AnimationGraphEditor::BoolParameterState : public AnimationGraphEditor::ControlParameterPreviewState
    {
        using ControlParameterPreviewState::ControlParameterPreviewState;

        virtual void DrawPreviewEditor( UpdateContext const& context, Transform const& characterWorldTransform, bool isLiveDebug ) override
        {
            auto pGraphInstance = GetGraphInstance();
            int16_t const parameterIdx = GetRuntimeGraphNodeIndex( m_pParameter->GetID() );
            EE_ASSERT( parameterIdx != InvalidIndex );

            //-------------------------------------------------------------------------

            if ( m_autoResetOnNextFrame && m_graphUpdateID != pGraphInstance->GetUpdateID() )
            {
                pGraphInstance->SetControlParameterValue( parameterIdx, false );
                m_autoResetOnNextFrame = false;
            }

            //-------------------------------------------------------------------------

            bool value = pGraphInstance->GetControlParameterValue<bool>( parameterIdx );
            if ( ImGui::Checkbox( "##bp", &value ) )
            {
                pGraphInstance->SetControlParameterValue( parameterIdx, value );
            }

            ImGui::SameLine();

            ImGui::BeginDisabled( value );
            if ( ImGui::Button( "Signal" ) )
            {
                pGraphInstance->SetControlParameterValue( parameterIdx, true );
                m_autoResetOnNextFrame = true;
                m_graphUpdateID = pGraphInstance->GetUpdateID();
            }
            ImGui::EndDisabled();
        }

    private:

        bool        m_autoResetOnNextFrame = false;
        uint32_t    m_graphUpdateID = 0;
    };

    //-------------------------------------------------------------------------

    class AnimationGraphEditor::FloatParameterState : public ControlParameterPreviewState
    {
    public:

        FloatParameterState( AnimationGraphEditor* pGraphEditor, ControlParameterToolsNode* pParameter )
            : ControlParameterPreviewState( pGraphEditor, pParameter )
        {
            auto pFloatParameter = Cast<FloatControlParameterToolsNode>( pParameter );
            m_min = pFloatParameter->GetPreviewRangeMin();
            m_max = pFloatParameter->GetPreviewRangeMax();
        }

    private:

        virtual void DrawPreviewEditor( UpdateContext const& context, Transform const& characterWorldTransform, bool isLiveDebug ) override
        {
            int16_t const parameterIdx = GetRuntimeGraphNodeIndex( m_pParameter->GetID() );
            EE_ASSERT( parameterIdx != InvalidIndex );

            float value = GetGraphInstance()->GetControlParameterValue<float>( parameterIdx );

            //-------------------------------------------------------------------------

            auto UpdateLimits = [this, &value, parameterIdx] ()
            {
                if ( m_min > m_max )
                {
                    m_max = m_min;
                }

                // Clamp the value to the range
                value = Math::Clamp( value, m_min, m_max );
                GetGraphInstance()->SetControlParameterValue( parameterIdx, value );
            };

            //-------------------------------------------------------------------------

            constexpr float const limitWidth = 40;

            ImGui::SetNextItemWidth( limitWidth );
            if ( ImGui::InputFloat( "##min", &m_min, 0.0f, 0.0f, "%.1f" ) )
            {
                UpdateLimits();
            }

            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                UpdateLimits();
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x - limitWidth - ImGui::GetStyle().ItemSpacing.x );
            if ( ImGui::SliderFloat( "##fp", &value, m_min, m_max ) )
            {
                GetGraphInstance()->SetControlParameterValue( parameterIdx, value );
            }

            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                GetGraphInstance()->SetControlParameterValue( parameterIdx, value );
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth( limitWidth );
            if ( ImGui::InputFloat( "##max", &m_max, 0.0f, 0.0f, "%.1f" ) )
            {
                UpdateLimits();
            }

            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                UpdateLimits();
            }
        }

    private:

        float m_min = 0.0f;
        float m_max = 1.0f;
    };

    //-------------------------------------------------------------------------

    class AnimationGraphEditor::VectorParameterState : public ControlParameterPreviewState
    {
        enum class ControlMode : uint8_t
        {
            Mouse,
            LeftStick,
            RightStick
        };

        constexpr static float const s_circleRadius = 45.0f;

    private:

        using ControlParameterPreviewState::ControlParameterPreviewState;

        virtual void DrawPreviewEditor( UpdateContext const& context, Transform const& characterWorldTransform, bool isLiveDebug ) override
        {
            DrawBasicEditor( context, characterWorldTransform, isLiveDebug );

            if ( isLiveDebug )
            {
                m_showAdvancedEditor = false;
            }

            // Advanced Editor
            //-------------------------------------------------------------------------

            if ( m_showAdvancedEditor )
            {
                DrawAdvancedEditor( context, characterWorldTransform );
            }
        }

        void DrawBasicEditor( UpdateContext const& context, Transform const& characterWorldTransform, bool isLiveDebug )
        {
            int16_t const parameterIdx = GetRuntimeGraphNodeIndex( m_pParameter->GetID() );
            EE_ASSERT( parameterIdx != InvalidIndex );
            Float3 parameterValue = GetGraphInstance()->GetControlParameterValue<Float3>( parameterIdx );

            // Basic Editor
            //-------------------------------------------------------------------------

            constexpr static float const buttonWidth = 28;

            ImGui::BeginDisabled( m_showAdvancedEditor );
            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x - buttonWidth - ImGui::GetStyle().ItemSpacing.x );
            if ( ImGui::InputFloat3( "##vp", &parameterValue.m_x ) )
            {
                GetGraphInstance()->SetControlParameterValue( parameterIdx, Vector( parameterValue ) );
            }
            ImGui::EndDisabled();

            ImGui::SameLine();

            ImGui::BeginDisabled( isLiveDebug );
            Color const buttonColor = m_showAdvancedEditor ? Colors::Green : ImGuiX::Style::s_colorGray1;
            if ( ImGuiX::ButtonColored( EE_ICON_COG, buttonColor, Colors::White, ImVec2( buttonWidth, 0 ) ) )
            {
                m_showAdvancedEditor = !m_showAdvancedEditor;
                m_maxLength = 1.0f;
                m_controlMode = ControlMode::Mouse;
                m_isEditingValueWithMouse = false;
                m_invertAxisX = m_invertAxisY = true;
            }
            ImGuiX::ItemTooltip( "Show Advanced Editor" );
            ImGui::EndDisabled();
        }

        void DrawAdvancedEditor( UpdateContext const& context, Transform const& characterWorldTransform )
        {
            int16_t const parameterIdx = GetRuntimeGraphNodeIndex( m_pParameter->GetID() );
            EE_ASSERT( parameterIdx != InvalidIndex );
            Float3 parameterValue = GetGraphInstance()->GetControlParameterValue<Float3>( parameterIdx );
            bool shouldUpdateParameterValue = false;

            // Get visual value
            //-------------------------------------------------------------------------

            ImVec2 visualValue = ConvertParameterValueToVisualValue( characterWorldTransform, parameterValue );

            //-------------------------------------------------------------------------

            ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );
            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 2, 2 ) );
            if ( ImGui::BeginChild( "#CW", ImVec2( 0, 140 ), ImGuiChildFlags_AlwaysUseWindowPadding ) )
            {
                constexpr float const labelOffset = 110.0f;
                constexpr float const widgetOffset = 175;

                // Length and Z-Value
                //-------------------------------------------------------------------------

                ImGui::SameLine( labelOffset );
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Max Length" );
                ImGui::SameLine( widgetOffset );
                ImGui::SetNextItemWidth( -1 );
                if ( ImGui::InputFloat( "##L", &m_maxLength ) )
                {
                    if ( m_maxLength <= 0.0 )
                    {
                        m_maxLength = 0.01f;
                    }

                    shouldUpdateParameterValue = true;
                }
                ImGuiX::ItemTooltip( "Max Length" );

                ImGui::NewLine();

                ImGui::SameLine( labelOffset );
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Z" );
                ImGui::SameLine( widgetOffset );
                ImGui::SetNextItemWidth( -1 );
                if ( ImGui::DragFloat( "##Z", &m_valueZ, 0.01f ) )
                {
                    shouldUpdateParameterValue = true;
                }
                ImGuiX::ItemTooltip( "Z-Value" );

                // Control Mode
                //-------------------------------------------------------------------------

                char const* const controlModeComboOptions[] = { "Mouse", "Left Stick", "Right Stick" };
                int32_t controlModeComboValue = (int32_t) m_controlMode;

                ImGui::NewLine();

                ImGui::SameLine( labelOffset );
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Control" );
                ImGui::SameLine( widgetOffset );
                ImGui::SetNextItemWidth( -1 );
                if ( ImGui::Combo( "##SC", &controlModeComboValue, controlModeComboOptions, 3 ) )
                {
                    m_controlMode = (ControlMode) controlModeComboValue;
                }
                ImGuiX::ItemTooltip( "Control" );

                // Options
                //-------------------------------------------------------------------------

                ImGui::NewLine();

                ImGui::SameLine( widgetOffset );
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Invert X" );
                ImGui::SameLine();
                ImGui::Checkbox( "##ISX", &m_invertAxisX );
                ImGuiX::ItemTooltip( "Invert the X axis" );

                ImGui::SameLine( 0, 10 );
                ImGui::Text( "Invert Y" );
                ImGui::SameLine();
                ImGui::Checkbox( "##ISY", &m_invertAxisY );
                ImGuiX::ItemTooltip( "Invert the Y axis" );

                if ( m_controlMode == ControlMode::Mouse )
                {
                    ImGui::SameLine( 0, 10 );
                    if ( ImGui::Button( "Reset" ) )
                    {
                        visualValue = ImVec2( 0, 0 );
                        shouldUpdateParameterValue = true;
                    }
                }

                // Calculate circle origin
                //-------------------------------------------------------------------------

                ImVec2 const circleOrigin = ImGui::GetWindowPos() + ImVec2( s_circleRadius + 5, s_circleRadius + 5 );

                // Controls
                //-------------------------------------------------------------------------

                if ( m_controlMode == ControlMode::Mouse )
                {
                    Vector const mousePos = ImGui::GetMousePos();

                    constexpr float const editRegionRadius = 10.0f;
                    float const distanceFromCircleOrigin = mousePos.GetDistance2( circleOrigin );

                    // Manage edit state
                    if ( !m_isEditingValueWithMouse )
                    {
                        // Should we start editing
                        if ( ImGui::IsWindowFocused() && distanceFromCircleOrigin < ( s_circleRadius + editRegionRadius ) && ImGui::IsMouseDragging( ImGuiMouseButton_Left, 2.0f ) )
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
                        if ( distanceFromCircleOrigin > s_circleRadius )
                        {
                            visualValue = ( mousePos - Vector( circleOrigin ) ).GetNormalized2() * s_circleRadius;
                        }
                        else
                        {
                            visualValue = ( mousePos - Vector( circleOrigin ) );
                        }

                        shouldUpdateParameterValue = true;
                    }
                }
                else // Stick Control
                {
                    auto pInputSystem = context.GetSystem<Input::InputSystem>();
                    if ( pInputSystem->IsControllerConnected( 0 ) )
                    {
                        auto pInputState = pInputSystem->GetController( 0 );
                        Vector const stickValue = ( m_controlMode == ControlMode::LeftStick ) ? pInputState->GetLeftStickValue() : pInputState->GetRightStickValue();

                        if ( !stickValue.IsNearZero2() )
                        {

                            Vector direction;
                            float length;
                            stickValue.ToDirectionAndLength2( direction, length );
                        
                            // Invert y to match screen coords (+Y is down)
                            direction.SetY( -direction.GetY() );

                            // Clamp to control area if we are outside it
                            if ( length > 1 )
                            {
                                visualValue = direction * s_circleRadius;
                            }
                            else // If we are within the control area
                            {
                                visualValue = direction * length * s_circleRadius;
                            }
                        }
                        else
                        {
                            visualValue = ImVec2( 0, 0 );
                        }

                        shouldUpdateParameterValue = true;
                    }
                }

                // Draw Circle and Editing Dot
                //-------------------------------------------------------------------------

                auto pDrawList = ImGui::GetWindowDrawList();
                pDrawList->AddCircle( circleOrigin, s_circleRadius, 0xFFFFFFFF, 0, 2.0f );
                pDrawList->AddCircleFilled( circleOrigin, 1, 0xFFFFFFFF, 0 );

                ImVec2 const fwdArrowEnd = circleOrigin - ImVec2( 0, s_circleRadius / 2 );
                ImGuiX::DrawArrow( pDrawList, circleOrigin, fwdArrowEnd, Colors::LightGray, 2, 4 );

                ImVec2 const textSize = ImGui::CalcTextSize( "fwd" );
                pDrawList->AddText( fwdArrowEnd - ImVec2( textSize.x / 2, textSize.y ), Colors::LightGray, "fwd" );

                constexpr float const dotRadius = 5.0f;
                Color const dotColor = m_isEditingValueWithMouse ? Colors::Yellow : Colors::LimeGreen;
                pDrawList->AddCircleFilled( circleOrigin + visualValue, dotRadius, dotColor );

                // Update parameter value
                //-------------------------------------------------------------------------

                if ( shouldUpdateParameterValue )
                {
                    parameterValue = ConvertVisualValueToParameterValue( characterWorldTransform, visualValue );
                    GetGraphInstance()->SetControlParameterValue( parameterIdx, Vector( parameterValue ) );
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleVar();
        }

        ImVec2 ConvertParameterValueToVisualValue( Transform const& characterWorldTransform, Float3 const& parameterValue )
        {
            float const scaleFactor = s_circleRadius / m_maxLength;

            ImVec2 visualValue( parameterValue.m_x * scaleFactor, -parameterValue.m_y * scaleFactor ); // +Y is downwards in screenspace

            if ( m_invertAxisX )
            {
                visualValue.x = -visualValue.x;
            }

            if ( !m_invertAxisY )
            {
                visualValue.y = -visualValue.y;
            }

            return visualValue;
        }

        Float4 ConvertVisualValueToParameterValue( Transform const& characterWorldTransform, ImVec2 const& visualValue )
        {
            float const scaleFactor = m_maxLength / s_circleRadius;
            Float4 parameterValue = Vector( visualValue ) * scaleFactor;
            parameterValue.m_z = m_valueZ;

            if ( m_invertAxisX )
            {
                parameterValue.m_x = -parameterValue.m_x;
            }

            if ( m_invertAxisY )
            {
                parameterValue.m_y = -parameterValue.m_y;
            }

            return parameterValue;
        }

    public:

        bool            m_showAdvancedEditor = false;
        float           m_maxLength = 1.0f;
        float           m_valueZ = 0.0f;
        ControlMode     m_controlMode = ControlMode::Mouse;
        bool            m_isEditingValueWithMouse = false;
        bool            m_invertAxisX = true;
        bool            m_invertAxisY = true;
    };

    //-------------------------------------------------------------------------

    class AnimationGraphEditor::IDParameterState : public ControlParameterPreviewState
    {
    public:

        IDParameterState( AnimationGraphEditor* pGraphEditor, ControlParameterToolsNode* pParameter )
            : ControlParameterPreviewState( pGraphEditor, pParameter )
            , m_comboWidget( pGraphEditor, Cast<IDControlParameterToolsNode>( pParameter ) )
        {}

    private:

        virtual void DrawPreviewEditor( UpdateContext const& context, Transform const& characterWorldTransform, bool isLiveDebug ) override
        {
            int16_t const parameterIdx = GetRuntimeGraphNodeIndex( m_pParameter->GetID() );
            EE_ASSERT( parameterIdx != InvalidIndex );

            auto value = GetGraphInstance()->GetControlParameterValue<StringID>( parameterIdx );
            if ( value.IsValid() )
            {
                strncpy_s( m_buffer, 255, value.c_str(), strlen( value.c_str() ) );
            }
            else
            {
                memset( m_buffer, 0, 255 );
            }

            //-------------------------------------------------------------------------

            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x - 28 );
            if ( ImGui::InputText( "##tp", m_buffer, 255, ImGuiInputTextFlags_EnterReturnsTrue ) )
            {
                GetGraphInstance()->SetControlParameterValue( parameterIdx, StringID( m_buffer ) );
            }

            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                GetGraphInstance()->SetControlParameterValue( parameterIdx, StringID( m_buffer ) );
            }

            //-------------------------------------------------------------------------

            ImGui::SameLine(0, 0);
            if ( m_comboWidget.DrawAndUpdate() )
            {
                StringID ID;
                if ( m_comboWidget.HasValidSelection() )
                {
                    ID = m_comboWidget.GetSelectedOption()->m_value;
                }

                GetGraphInstance()->SetControlParameterValue( parameterIdx, ID );
            }
        }

    private:

        char                m_buffer[256] = { 0 };
        IDComboWidget       m_comboWidget;
    };

    //-------------------------------------------------------------------------

    class AnimationGraphEditor::TargetParameterState : public ControlParameterPreviewState
    {
        using ControlParameterPreviewState::ControlParameterPreviewState;

        virtual void DrawPreviewEditor( UpdateContext const& context, Transform const& characterWorldTransform, bool isLiveDebug ) override
        {
            int16_t const parameterIdx = GetRuntimeGraphNodeIndex( m_pParameter->GetID() );
            EE_ASSERT( parameterIdx != InvalidIndex );

            Target value = GetGraphInstance()->GetControlParameterValue<Target>( parameterIdx );

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

                GetGraphInstance()->SetControlParameterValue( parameterIdx, value );
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
// Editor
//-------------------------------------------------------------------------

namespace EE::Animation
{
    EE_RESOURCE_EDITOR_FACTORY( AnimationGraphEditorFactory, GraphDefinition, AnimationGraphEditor );

    //-------------------------------------------------------------------------

    TEvent<ResourceID const&> AnimationGraphEditor::s_graphModifiedEvent;

    //-------------------------------------------------------------------------

    bool AnimationGraphEditor::DebugTarget::IsValid() const
    {
        switch ( m_type )
        {
            case DebugTargetType::MainGraph:
            {
                return m_pComponentToDebug != nullptr;
            }
            break;

            case DebugTargetType::ReferencedGraph:
            {
                return m_pComponentToDebug != nullptr && m_referencedGraphID.IsValid();
            }
            break;

            case DebugTargetType::ExternalGraph:
            {
                return m_pComponentToDebug != nullptr && m_externalSlotID.IsValid();
            }
            break;

            case DebugTargetType::DirectPreview:
            case DebugTargetType::Recording:
            {
                return true;
            }
            break;

            default:
            {
                return false;
            }
            break;
        }
    }

    //-------------------------------------------------------------------------

    AnimationGraphEditor::AnimationGraphEditor( ToolsContext const* pToolsContext, ResourceID const& resourceID, EntityWorld* pWorld )
        : DataFileEditor( pToolsContext, Variation::GetGraphResourceID( resourceID ).GetDataPath(), pWorld )
        , m_openedResourceID( resourceID )
        , m_clipBrowser( m_pToolsContext )
        , m_nodeEditorPropertyGrid( m_pToolsContext )
        , m_nodeVariationDataPropertyGrid( m_pToolsContext )
        , m_primaryGraphView( &m_userContext )
        , m_secondaryGraphView( &m_userContext )
        , m_oldIDWidget( this )
        , m_newIDWidget( this )
        , m_variationSkeletonPicker( *m_pToolsContext, Skeleton::GetStaticResourceTypeID() )
        , m_variationDataPropertyGrid( m_pToolsContext )
    {
        // Set up property grid
        //-------------------------------------------------------------------------

        m_nodeEditorPropertyGrid.SetUserContext( this );
        m_nodeEditorPropertyGridPreEditEventBindingID = m_nodeEditorPropertyGrid.OnPreEdit().Bind( [this] ( PropertyEditInfo const& info ) { PropertyGridPreEdit( info ); } );
        m_nodeEditorPropertyGridPostEditEventBindingID = m_nodeEditorPropertyGrid.OnPostEdit().Bind( [this] ( PropertyEditInfo const& info ) { PropertyGridPostEdit( info ); } );

        m_nodeVariationDataPropertyGrid.SetUserContext( this );
        m_nodeVariationDataPropertyGridPreEditEventBindingID = m_nodeVariationDataPropertyGrid.OnPreEdit().Bind( [this] ( PropertyEditInfo const& info ) { PropertyGridPreEdit( info ); } );
        m_nodeVariationDataPropertyGridPostEditEventBindingID = m_nodeVariationDataPropertyGrid.OnPostEdit().Bind( [this] ( PropertyEditInfo const& info ) { PropertyGridPostEdit( info ); } );

        // Setup graph outliner
        //-------------------------------------------------------------------------

        m_outlinerTreeContext.m_rebuildTreeFunction = [this] ( TreeListViewItem* pRootItem ) { RebuildOutlinerTree( pRootItem ); };

        m_outlinerTreeView.SetFlag( TreeListView::Flags::ShowBranchesFirst, false );
        m_outlinerTreeView.SetFlag( TreeListView::Flags::ExpandItemsOnlyViaArrow );
        m_outlinerTreeView.SetFlag( TreeListView::Flags::SortTree );

        // Variation editor
        //-------------------------------------------------------------------------

        m_variationTreeContext.m_rebuildTreeFunction = [this] ( TreeListViewItem* pRootItem ) { RebuildVariationDataTree( pRootItem ); };
        m_variationTreeContext.m_setupExtraColumnHeadersFunction = [this] () { SetupVariationEditorItemExtraColumnHeaders(); };
        m_variationTreeContext.m_drawItemExtraColumnsFunction = [this] ( TreeListViewItem* pItem, int32_t columnIdx ) { DrawVariationEditorItemExtraColumns( pItem, columnIdx ); };
        m_variationTreeContext.m_numExtraColumns = 1;

        m_variationDataPropertyGrid.SetUserContext( this );
        m_variationDataPropertyGridPreEditEventBindingID = m_variationDataPropertyGrid.OnPreEdit().Bind( [this] ( PropertyEditInfo const& info ) { PropertyGridPreEdit( info ); } );
        m_variationDataPropertyGridPostEditEventBindingID = m_variationDataPropertyGrid.OnPostEdit().Bind( [this] ( PropertyEditInfo const& info ) { PropertyGridPostEdit( info ); } );

        m_variationTreeView.SetFlag( TreeListView::Flags::ShowBranchesFirst, false );
        m_variationTreeView.SetFlag( TreeListView::Flags::ExpandItemsOnlyViaArrow );
        m_variationTreeView.SetFlag( TreeListView::Flags::SortTree );

        // Gizmo
        //-------------------------------------------------------------------------

        m_gizmo.SetCoordinateSystemSpace( CoordinateSpace::Local );
        m_gizmo.SetOption( ImGuiX::Gizmo::Options::AllowScale, false );
        m_gizmo.SetOption( ImGuiX::Gizmo::Options::AllowCoordinateSpaceSwitching, false );
        m_gizmo.SetMode( ImGuiX::Gizmo::GizmoMode::Translation );

        // Create main graph data storage
        //-------------------------------------------------------------------------

        m_loadedGraphStack.emplace_back( EE::New<LoadedGraphData>() );

        // Get Node Type Info
        //-------------------------------------------------------------------------

        m_registeredNodeTypes = m_pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( FlowToolsNode::GetStaticTypeID(), false, false, true );

        for ( auto pNodeType : m_registeredNodeTypes )
        {
            auto pDefaultNode = Cast<FlowToolsNode const>( pNodeType->m_pDefaultInstance );
            if ( pDefaultNode->IsUserCreatable() )
            {
                m_categorizedNodeTypes.AddItem( pDefaultNode->GetCategory(), pDefaultNode->GetTypeName(), pNodeType );
            }
        }

        m_variationDataNodeTypes = m_pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( VariationDataToolsNode::GetStaticTypeID(), false, false, true );

        // User context
        //-------------------------------------------------------------------------

        m_userContext.m_pTypeRegistry = m_pToolsContext->m_pTypeRegistry;
        m_userContext.m_pFileRegistry = m_pToolsContext->m_pFileRegistry;
        m_userContext.m_pCategorizedNodeTypes = &m_categorizedNodeTypes.GetRootCategory();
        m_userContext.m_pVariationHierarchy = nullptr;
        m_userContext.m_pParameters = &m_parameters;

        m_nodeDoubleClickedEventBindingID = m_userContext.OnNodeDoubleClicked().Bind( [this] ( NodeGraph::BaseNode* pNode ) { NodeDoubleClicked( pNode ); } );
        m_graphDoubleClickedEventBindingID = m_userContext.OnGraphDoubleClicked().Bind( [this] ( NodeGraph::BaseGraph* pGraph ) { GraphDoubleClicked( pGraph ); } );
        m_postPasteNodesEventBindingID = m_userContext.OnPostPasteNodes().Bind( [this] ( TInlineVector<NodeGraph::BaseNode*, 20> const& pastedNodes ) { PostPasteNodes( pastedNodes ); } );
        m_resourceOpenRequestEventBindingID = m_userContext.OnRequestOpenResource().Bind( [this] ( ResourceID const& resourceID ) { m_pToolsContext->TryOpenResource( resourceID ); } );
        m_navigateToNodeEventBindingID = m_userContext.OnNavigateToNode().Bind( [this] ( NodeGraph::BaseNode* pNode ) { NavigateTo( pNode ); } );
        m_navigateToGraphEventBindingID = m_userContext.OnNavigateToGraph().Bind( [this] ( NodeGraph::BaseGraph* pGraph ) { NavigateTo( pGraph ); } );
        m_customCommandEventBindingID = m_userContext.OnCustomCommandRequested().Bind( [this] ( NodeGraph::CustomCommand const* pCommand ) { ProcessCustomCommandRequest( pCommand ); } );

        // Bind events
        //-------------------------------------------------------------------------

        m_rootGraphBeginModificationBindingID = NodeGraph::BaseGraph::OnBeginRootGraphModification().Bind( [this] ( NodeGraph::BaseGraph* pRootGraph ) { OnBeginGraphModification( pRootGraph ); } );
        m_rootGraphEndModificationBindingID = NodeGraph::BaseGraph::OnEndRootGraphModification().Bind( [this] ( NodeGraph::BaseGraph* pRootGraph ) { OnEndGraphModification( pRootGraph ); } );
    }

    AnimationGraphEditor::~AnimationGraphEditor()
    {
        m_nodeEditorPropertyGrid.OnPreEdit().Unbind( m_nodeEditorPropertyGridPreEditEventBindingID );
        m_nodeEditorPropertyGrid.OnPostEdit().Unbind( m_nodeEditorPropertyGridPostEditEventBindingID );

        m_nodeVariationDataPropertyGrid.OnPreEdit().Unbind( m_nodeVariationDataPropertyGridPreEditEventBindingID );
        m_nodeVariationDataPropertyGrid.OnPostEdit().Unbind( m_nodeVariationDataPropertyGridPostEditEventBindingID );

        //-------------------------------------------------------------------------

        m_variationDataPropertyGrid.OnPreEdit().Unbind( m_variationDataPropertyGridPreEditEventBindingID );
        m_variationDataPropertyGrid.OnPostEdit().Unbind( m_variationDataPropertyGridPostEditEventBindingID );

        //-------------------------------------------------------------------------

        m_userContext.OnNavigateToNode().Unbind( m_navigateToNodeEventBindingID );
        m_userContext.OnNavigateToGraph().Unbind( m_navigateToGraphEventBindingID );
        m_userContext.OnRequestOpenResource().Unbind( m_resourceOpenRequestEventBindingID );
        m_userContext.OnPostPasteNodes().Unbind( m_postPasteNodesEventBindingID );
        m_userContext.OnGraphDoubleClicked().Unbind( m_graphDoubleClickedEventBindingID );
        m_userContext.OnNodeDoubleClicked().Unbind( m_nodeDoubleClickedEventBindingID );
        m_userContext.OnCustomCommandRequested().Unbind( m_customCommandEventBindingID );

        m_userContext.m_pTypeRegistry = nullptr;
        m_userContext.m_pCategorizedNodeTypes = nullptr;
        m_userContext.m_pVariationHierarchy = nullptr;
        m_userContext.m_pParameters = nullptr;

        //-------------------------------------------------------------------------

        NodeGraph::BaseGraph::OnEndRootGraphModification().Unbind( m_rootGraphEndModificationBindingID );
        NodeGraph::BaseGraph::OnBeginRootGraphModification().Unbind( m_rootGraphBeginModificationBindingID );

        EE_ASSERT( m_loadedGraphStack.size() == 1 );
        EE::Delete( m_loadedGraphStack[0] );

        EE_ASSERT( m_pDebugGraphInstance == nullptr );
        EE_ASSERT( m_pHostGraphInstance == nullptr );
    }

    bool AnimationGraphEditor::IsEditingFile( DataPath const& dataPath ) const
    {
        ResourceID const actualResourceID = Variation::GetGraphResourceID( dataPath );
        return actualResourceID == GetDataFilePath();
    }

    void AnimationGraphEditor::Initialize( UpdateContext const& context )
    {
        DataFileEditor::Initialize( context );

        if ( IsDataFileLoaded() )
        {
            UpdateUserContext();
        }

        //-------------------------------------------------------------------------

        auto OnGlobalGraphEdited = [this]( ResourceID const& editedGraph )
        {
            if ( m_graphRecorder.HasRecordedDataForGraph( editedGraph ) )
            {
                if ( m_debugMode == DebugMode::ReviewRecording )
                {
                    StopDebugging();
                }

                m_graphRecorder.Reset();
            }
        };

        m_globalGraphEditEventBindingID = s_graphModifiedEvent.Bind( OnGlobalGraphEdited );

        //-------------------------------------------------------------------------

        CreateToolWindow( "Variation Editor", [this] ( UpdateContext const& context, bool isFocused ) { DrawVariationEditor( context, isFocused ); } );
        CreateToolWindow( "Clip Browser", [this] ( UpdateContext const& context, bool isFocused ) { DrawClipBrowser( context, isFocused ); } );
        CreateToolWindow( "Log", [this] ( UpdateContext const& context, bool isFocused ) { DrawGraphLog( context, isFocused ); } );
        CreateToolWindow( "Debugger", [this] ( UpdateContext const& context, bool isFocused ) { DrawDebuggerWindow( context, isFocused ); } );
        CreateToolWindow( "Control Parameters", [this] ( UpdateContext const& context, bool isFocused ) { DrawControlParameterEditor( context, isFocused ); } );
        CreateToolWindow( "Graph View", [this] ( UpdateContext const& context, bool isFocused ) { DrawGraphView( context, isFocused ); } );
        CreateToolWindow( "Node Editor", [this] ( UpdateContext const& context, bool isFocused ) { DrawNodeEditor( context, isFocused ); } );
        CreateToolWindow( "Outliner", [this] ( UpdateContext const& context, bool isFocused ) { DrawOutliner( context, isFocused ); } );

        HideDataFileWindow();
    }

    void AnimationGraphEditor::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID topLeftDockID = 0, bottomLeftDockID = 0, centerDockID = 0, rightDockID = 0, bottomRightDockID;

        ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.2f, &topLeftDockID, &centerDockID );
        ImGui::DockBuilderSplitNode( topLeftDockID, ImGuiDir_Down, 0.33f, &bottomLeftDockID, &topLeftDockID );
        ImGui::DockBuilderSplitNode( centerDockID, ImGuiDir_Left, 0.66f, &centerDockID, &rightDockID );
        ImGui::DockBuilderSplitNode( rightDockID, ImGuiDir_Down, 0.66f, &bottomRightDockID, &rightDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetToolWindowName( s_viewportWindowName ).c_str(), rightDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Debugger" ).c_str(), bottomRightDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Outliner" ).c_str(), topLeftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Control Parameters" ).c_str(), topLeftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Graph View" ).c_str(), centerDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Variation Editor" ).c_str(), centerDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Log" ).c_str(), bottomLeftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Clip Browser" ).c_str(), bottomLeftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Node Editor" ).c_str(), bottomLeftDockID );
    }

    void AnimationGraphEditor::Shutdown( UpdateContext const& context )
    {
        if ( IsDebugging() )
        {
            StopDebugging();
        }

        if ( !m_previewParameterStates.empty() )
        {
            DestroyControlParameterPreviewStates();
        }

        s_graphModifiedEvent.Unbind( m_globalGraphEditEventBindingID );

        //-------------------------------------------------------------------------

        EE_ASSERT( !m_previewGraphDefinitionPtr.IsSet() );

        ClearGraphStack();

        //-------------------------------------------------------------------------

        DataFileEditor::Shutdown( context );
    }

    void AnimationGraphEditor::WorldUpdate( EntityWorldUpdateContext const& updateContext )
    {
        // Frame End
        //-------------------------------------------------------------------------

        if ( updateContext.GetUpdateStage() == UpdateStage::FrameEnd && IsDebugging() && m_pDebugGraphComponent->IsInitialized() )
        {
            // Update character transform
            //-------------------------------------------------------------------------

            m_characterTransform = m_pDebugGraphComponent->GetDebugWorldTransform();

            // Transfer pose from game entity to preview entity
            //-------------------------------------------------------------------------

            if ( IsLiveDebugging() )
            {
                if ( m_pDebugMeshComponent != nullptr && m_pDebugMeshComponent->IsInitialized() )
                {
                    m_pDebugMeshComponent->SetPose( m_pDebugGraphComponent->GetPrimaryPose() );
                    m_pDebugMeshComponent->FinalizePose();
                    m_pDebugMeshComponent->SetWorldTransform( m_characterTransform );
                }
            }

            // Debug drawing
            //-------------------------------------------------------------------------

            // Update camera tracking
            // TODO: this is pretty janky so need to try to improve it
            if ( m_isCameraTrackingEnabled )
            {
                Transform const currentCameraTransform = GetCameraTransform();
                Transform const offsetDelta = Transform::Delta( m_previousCameraTransform, currentCameraTransform );
                m_cameraOffsetTransform = offsetDelta * m_cameraOffsetTransform;
                m_previousCameraTransform = m_cameraOffsetTransform * m_characterTransform;
                SetCameraTransform( m_previousCameraTransform );
            }

            // Reflect debug options
            if ( m_pDebugGraphComponent->IsInitialized() )
            {
                auto drawingContext = updateContext.GetDrawingContext();
                m_pDebugGraphComponent->SetGraphDebugMode( m_graphDebugMode );
                m_pDebugGraphComponent->SetRootMotionDebugMode( m_rootMotionDebugMode );
                m_pDebugGraphComponent->SetTaskSystemDebugMode( m_taskSystemDebugMode );
                m_pDebugGraphComponent->DrawDebug( drawingContext );

                // Only mess with skeleton LOD on previews and not when live-attaching
                if ( IsPreviewing() )
                {
                    m_pDebugGraphComponent->SetSkeletonLOD( m_skeletonLOD );
                }
            }

            // Draw preview capsule
            if ( m_showPreviewCapsule )
            {
                if ( m_pDebugGraphComponent->HasGraphInstance() )
                {
                    auto drawContext = GetDrawingContext();
                    Transform capsuleTransform = m_pDebugGraphComponent->GetDebugWorldTransform();
                    capsuleTransform.AddTranslation( capsuleTransform.GetAxisZ() * ( m_previewCapsuleHalfHeight + m_previewCapsuleRadius ) );
                    drawContext.DrawCapsule( capsuleTransform, m_previewCapsuleRadius, m_previewCapsuleHalfHeight, Colors::LimeGreen, 3.0f );
                }
            }
        }
    }

    void AnimationGraphEditor::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        // Process variation requests
        //-------------------------------------------------------------------------

        if ( !m_variationRequests.empty() )
        {
            StringID const activeVariation = GetActiveVariation();
            EE_ASSERT( activeVariation != Variation::s_defaultVariationID );

            for ( auto& varRequest : m_variationRequests )
            {
                if ( varRequest.second )
                {
                    varRequest.first->CreateVariationOverride( activeVariation );
                }
                else
                {
                    varRequest.first->RemoveVariationOverride( activeVariation );
                }
            }
        }
        m_variationRequests.clear();

        // Set read-only state
        //-------------------------------------------------------------------------

        bool const isViewReadOnly = IsInReadOnlyState();
        m_nodeEditorPropertyGrid.SetReadOnly( isViewReadOnly );
        m_nodeVariationDataPropertyGrid.SetReadOnly( isViewReadOnly );
        m_primaryGraphView.SetReadOnly( isViewReadOnly );
        m_secondaryGraphView.SetReadOnly( isViewReadOnly );

        // Global Go-To
        //-------------------------------------------------------------------------

        if ( isFocused )
        {
            if ( !m_dialogManager.HasActiveModalDialog() )
            {
                if ( ( ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_G ) ) || ( ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed( ImGuiKey_F ) ) )
                {
                    StartNavigationOperation();
                }
            }
        }

        // Control Parameter Manipulation
        //-------------------------------------------------------------------------

        // Check if we have a target parameter selected
        m_pSelectedTargetControlParameter = nullptr;
        if ( !m_selectedNodes.empty() )
        {
            auto pSelectedNode = m_selectedNodes.back().m_pNode;
            m_pSelectedTargetControlParameter = TryCast<TargetControlParameterToolsNode>( pSelectedNode );

            // Handle reference nodes
            if ( m_pSelectedTargetControlParameter == nullptr )
            {
                auto pReferenceNode = TryCast<ParameterReferenceToolsNode>( pSelectedNode );
                if ( pReferenceNode != nullptr && pReferenceNode->IsReferencingControlParameter() )
                {
                    m_pSelectedTargetControlParameter = TryCast<TargetControlParameterToolsNode>( pReferenceNode->GetReferencedControlParameter() );
                }
            }
        }

        // Debug Request
        //-------------------------------------------------------------------------

        if ( m_requestedDebugTarget.IsValid() )
        {
            if ( !IsHotReloading() && !IsLoadingResources() )
            {
                if ( m_requestedDebugTarget.m_pComponentToDebug != nullptr )
                {
                    if ( m_requestedDebugTarget.m_pComponentToDebug->HasGraphInstance() )
                    {
                        StartDebugging( context );
                    }
                }
                else
                {
                    StartDebugging( context );
                }
            }
            else
            {
                EE_TRACE_MSG( "Waiting to start debug!" );
            }
        }

        // Debugging
        //-------------------------------------------------------------------------

        if ( IsDebugging() )
        {
            if ( m_previewGraphDefinitionPtr.HasLoadingFailed() )
            {
                StopDebugging();
                return;
            }

            if ( !m_pDebugGraphComponent->IsInitialized() )
            {
                return;
            }

            //-------------------------------------------------------------------------

            if ( IsPreviewOrReview() )
            {
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
                        ReflectInitialPreviewParameterValues( context );

                        if ( m_initializeGraphToSpecifiedSyncTime )
                        {
                            m_pDebugGraphInstance->ResetGraphState( m_previewStartSyncTime );
                        }
                        else
                        {
                            m_pDebugGraphInstance->ResetGraphState();
                        }
                    }

                    m_isFirstPreviewFrame = false;
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

                // Try to generate the relevant graph data for the first frame
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
            }
        }
    }

    void AnimationGraphEditor::DrawMenu( UpdateContext const& context )
    {
        DataFileEditor::DrawMenu( context );

        ImGui::Separator();

        // Variation Selector
        //-------------------------------------------------------------------------

        ImGui::Text( "Variation: " );
        ImGui::SameLine();
        DrawVariationSelector( 150 );

        // Editing Tools
        //-------------------------------------------------------------------------

        if ( ImGui::BeginMenu( EE_ICON_WRENCH" Tools" ) )
        {
            if ( ImGui::MenuItem( EE_ICON_TAB_SEARCH" Find/Navigate") )
            {
                StartNavigationOperation();
            }

            if ( ImGui::MenuItem( EE_ICON_RENAME_BOX" Rename IDs and EventIDs" ) )
            {
                StartRenameIDs();
            }

            if ( ImGui::MenuItem( EE_ICON_ALPHA_C_BOX" Copy all parameter names to Clipboard" ) )
            {
                CopyAllParameterNamesToClipboard();
            }

            if ( ImGui::MenuItem( EE_ICON_IDENTIFIER" Copy all IDs to Clipboard" ) )
            {
                CopyAllGraphIDsToClipboard();
            }

            ImGui::EndMenu();
        }

        // Options
        //-------------------------------------------------------------------------

        if ( ImGui::BeginMenu( EE_ICON_TUNE" Options" ) )
        {
            bool isUsingLowLOD = m_skeletonLOD == Skeleton::LOD::Low;
            if ( ImGui::Checkbox( "Use Low Skeleton LOD (for preview)", &isUsingLowLOD ) )
            {
                m_skeletonLOD = isUsingLowLOD ? Skeleton::LOD::Low : Skeleton::LOD::High;
            }

            if ( ImGui::Checkbox( "Show Runtime Node Indices", &m_userContext.m_showRuntimeIndices ) )
            {
                m_skeletonLOD = isUsingLowLOD ? Skeleton::LOD::Low : Skeleton::LOD::High;
            }

            ImGui::EndMenu();
        }

        // Live Debug
        //-------------------------------------------------------------------------

        if ( IsLiveDebugging() )
        {
            if ( ImGuiX::FlatIconButton( EE_ICON_POWER_PLUG_OFF_OUTLINE, "Stop Live Debug", Colors::Red, ImVec2( 0, 0 ) ) )
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
    }

    void AnimationGraphEditor::DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        DataFileEditor::DrawViewportToolbar( context, pViewport );

        // Debug Options
        //-------------------------------------------------------------------------

        auto DrawDebugOptions = [this] ()
        {
            ImGui::SeparatorText( "Graph Debug" );

            bool isGraphDebugEnabled = ( m_graphDebugMode == GraphDebugMode::On );
            if ( ImGui::Checkbox( "Enable Graph Debug", &isGraphDebugEnabled ) )
            {
                m_graphDebugMode = isGraphDebugEnabled ? GraphDebugMode::On : GraphDebugMode::Off;
            }

            ImGui::SeparatorText( "Root Motion Debug" );

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

            ImGui::SeparatorText( "Pose Debug" );

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

            ImGui::SeparatorText( "Capsule Debug" );

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

        auto DrawPreviewOptions = [this]()
        {
            ImGui::SeparatorText( "Preview Settings" );
            ImGui::Checkbox( "Start Paused", &m_startPaused );

            if ( ImGui::Checkbox( "Camera Follows Character", &m_isCameraTrackingEnabled ) )
            {
                if ( m_isCameraTrackingEnabled && IsDebugging() )
                {
                    CalculateCameraOffset();
                }
            }

            ImGui::SeparatorText( "Start Transform" );
            ImGuiX::InputTransform( "StartTransform", m_previewStartTransform, 250.0f );

            if ( ImGui::Button( "Reset Start Transform", ImVec2( -1, 0 ) ) )
            {
                m_previewStartTransform = Transform::Identity;
            }

            ImGui::SeparatorText( "Start Sync Time" );
            
            ImGui::Checkbox( "Init Graph", &m_initializeGraphToSpecifiedSyncTime );

            if ( m_initializeGraphToSpecifiedSyncTime )
            {
                ImGui::InputInt( "Event Idx", &m_previewStartSyncTime.m_eventIdx );

                float percentageThrough = m_previewStartSyncTime.m_percentageThrough.ToFloat();
                if ( ImGui::InputFloat( "Normalized %", &percentageThrough ) )
                {
                    m_previewStartSyncTime.m_percentageThrough = Percentage( percentageThrough ).GetClamped();
                }

                if ( ImGui::Button( "Reset Sync Time", ImVec2( -1, 0 ) ) )
                {
                    m_previewStartSyncTime = SyncTrackTime();
                }
            }
        };

        //-------------------------------------------------------------------------

        constexpr float const buttonWidth = 180;

        if ( IsDebugging() )
        {
            if ( ImGuiX::ComboIconButton( EE_ICON_STOP, "Stop Debugging", DrawPreviewOptions, Colors::Red, ImVec2( buttonWidth, 0 ) ) )
            {
                StopDebugging();
            }
        }
        else
        {
            if ( ImGuiX::ComboIconButton( EE_ICON_PLAY, "Preview Graph", DrawPreviewOptions, Colors::Lime, ImVec2( buttonWidth, 0 ) ) )
            {
                DebugTarget target;
                target.m_type = DebugTargetType::DirectPreview;
                RequestDebugSession( context, target );
            }
        }
    }

    void AnimationGraphEditor::DrawViewportOverlayElements( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        DataFileEditor::DrawViewportOverlayElements( context, pViewport );

        // Allow for in-viewport manipulation of the parameter preview value
        if ( m_pSelectedTargetControlParameter != nullptr )
        {
            Transform gizmoTransform;
            bool drawGizmo = false;

            if ( IsDebugging() )
            {
                if ( m_pDebugGraphComponent->HasGraphInstance() )
                {
                    int16_t const parameterIdx = m_pDebugGraphComponent->GetControlParameterIndex( StringID( m_pSelectedTargetControlParameter->GetParameterName() ) );
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
                if ( !m_pSelectedTargetControlParameter->IsBoneTarget() )
                {
                    gizmoTransform = m_pSelectedTargetControlParameter->GetPreviewTargetTransform();
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

                auto SetParameterValue = [this] ( Transform const& newTransform )
                {
                    if ( IsDebugging() )
                    {
                        EE_ASSERT( m_pDebugGraphComponent->HasGraphInstance() );
                        int16_t const parameterIdx = m_pDebugGraphComponent->GetControlParameterIndex( StringID( m_pSelectedTargetControlParameter->GetParameterName() ) );
                        EE_ASSERT( parameterIdx != InvalidIndex );
                        m_pDebugGraphComponent->SetControlParameterValue<Target>( parameterIdx, Target( newTransform ) );
                    }
                    else
                    {
                        m_pSelectedTargetControlParameter->SetPreviewTargetTransform( newTransform );
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
                            GetEditedRootGraph()->BeginModification();
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
                            GetEditedRootGraph()->EndModification();
                        }
                    }
                    break;

                    default:
                    break;
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    void AnimationGraphEditor::ShowNotifyDialog( String const& title, String const& message )
    {
        EE_ASSERT( !title.empty() && !message.empty() );

        auto DrawDialog = [message] ( UpdateContext const& context )
        {
            ImGui::Text( message.c_str() );

            ImGui::NewLine();

            ImGui::SameLine( ( ImGui::GetContentRegionAvail().x - 100 ) / 2.0f );
            if ( ImGui::Button( "OK", ImVec2( 100, 0 ) ) )
            {
                return false;
            }

            return true;
        };

        m_dialogManager.CreateModalDialog( title, DrawDialog, ImVec2( -1, -1 ), false );
    }

    void AnimationGraphEditor::ShowNotifyDialog( String const& title, char const* pMessageFormat, ... )
    {
        String message;
        va_list args;
        va_start( args, pMessageFormat );
        message.sprintf( pMessageFormat, args );
        va_end( args );

        ShowNotifyDialog( title, message );
    }

    //-------------------------------------------------------------------------

    void AnimationGraphEditor::OnResourceUnload( Resource::ResourcePtr* pResourcePtr )
    {
        if ( IsPreviewOrReview() )
        {
            StopDebugging();
        }
    }

    void AnimationGraphEditor::OnDataFileLoadCompleted()
    {
        if ( IsDataFileLoaded() )
        {
            GraphResourceDescriptor* pGraphDesc = GetDataFile<GraphResourceDescriptor>();
            if ( !pGraphDesc->m_graphDefinition.IsValid() )
            {
                pGraphDesc->m_graphDefinition.CreateDefaultRootGraph();
            }

            m_loadedGraphStack[0]->m_pGraphDefinition = &pGraphDesc->m_graphDefinition;
        }
        else
        {
            m_loadedGraphStack[0]->m_pGraphDefinition = nullptr;
            return;
        }

        // Update Variation
        //-------------------------------------------------------------------------

        TrySetActiveVariation( Variation::GetVariationNameFromResourceID( m_openedResourceID ) );
        UpdateUserContext();

        // Reset graph stack
        //-------------------------------------------------------------------------

        ClearGraphStack();
        m_undoStack.Reset();

        // Refresh UI
        //-------------------------------------------------------------------------

        RefreshVariationEditor();

        RefreshOutliner();

        RefreshControlParameterCache();

        //-------------------------------------------------------------------------

        RestoreViewAndSelectionState();
    }

    void AnimationGraphEditor::OnDataFileUnload()
    {
        if ( IsPreviewOrReview() )
        {
            StopDebugging();
            m_graphRecorder.Reset();
        }

        RecordViewAndSelectionState();
    }

    //-------------------------------------------------------------------------
    // Graph Operations
    //-------------------------------------------------------------------------

    void AnimationGraphEditor::NodeDoubleClicked( NodeGraph::BaseNode* pNode )
    {
        if ( auto pReferencedGraphNode = TryCast<ReferencedGraphToolsNode>( pNode ) )
        {
            ResourceID const resourceID = pReferencedGraphNode->GetReferencedGraphResourceID( *m_userContext.m_pVariationHierarchy, m_userContext.m_selectedVariationID );
            if ( resourceID.IsValid() )
            {
                EE_ASSERT( resourceID.GetResourceTypeID() == GraphDefinition::GetStaticResourceTypeID() );
                if ( m_userContext.m_isCtrlDown )
                {
                    m_userContext.RequestOpenResource( resourceID );
                }
                else
                {
                    PushOnGraphStack( pReferencedGraphNode, resourceID );
                }
            }
        }
        else if ( auto pVariationDataNode = TryCast<VariationDataToolsNode>( pNode ) )
        {
            auto pData = pVariationDataNode->GetResolvedVariationData( *m_userContext.m_pVariationHierarchy, m_userContext.m_selectedVariationID );
            TInlineVector<ResourceID, 2> referencedResources;
            pData->GetReferencedResources( referencedResources );
            if ( referencedResources.size() == 1 && referencedResources[0].IsValid() )
            {
                m_userContext.RequestOpenResource( referencedResources[0] );
            }
        }
        else if ( auto pBoneMaskNode = TryCast<BoneMaskToolsNode>( pNode ) )
        {
            Variation const* pSelectedVariation = m_userContext.m_pVariationHierarchy->GetVariation( m_userContext.m_selectedVariationID );

            if ( pSelectedVariation->m_skeleton.GetResourceID().IsValid() )
            {
                m_userContext.RequestOpenResource( pSelectedVariation->m_skeleton.GetResourceID() );
            }
        }
        else if ( auto pExternalGraphNode = TryCast<ExternalGraphToolsNode>( pNode ) )
        {
            if ( m_userContext.HasDebugData() )
            {
                int16_t runtimeNodeIdx = m_userContext.GetRuntimeGraphNodeIndex( pNode->GetID() );
                if ( runtimeNodeIdx != InvalidIndex )
                {
                    StringID const SlotID( pNode->GetName() );
                    GraphInstance const* pConnectedGraphInstance = m_userContext.m_pGraphInstance->GetExternalGraphDebugInstance( SlotID );
                    if ( pConnectedGraphInstance != nullptr )
                    {
                        m_userContext.RequestOpenResource( pConnectedGraphInstance->GetDefinitionResourceID() );
                    }
                }
            }
        }
    }

    void AnimationGraphEditor::GraphDoubleClicked( NodeGraph::BaseGraph* pGraph )
    {
        EE_ASSERT( pGraph != nullptr );
        int32_t const stackIdx = GetStackIndexForGraph( pGraph );
        if ( pGraph->GetParentNode() == nullptr && stackIdx > 0 )
        {
            NavigateTo( m_loadedGraphStack[stackIdx]->m_pParentNode );
        }
    }

    void AnimationGraphEditor::PostPasteNodes( TInlineVector<NodeGraph::BaseNode*, 20> const& pastedNodes )
    {
        NodeGraph::ScopedGraphModification gm( GetEditedRootGraph() );
        GetEditedGraphData()->m_pGraphDefinition->RefreshParameterReferences();
        OnGraphStateModified();
    }

    void AnimationGraphEditor::OnGraphStateModified()
    {
        m_graphRecorder.Reset();
        RefreshControlParameterCache();
        RefreshVariationEditor();
        RefreshOutliner();
        s_graphModifiedEvent.Execute( GetDataFilePath() );
    }

    //-------------------------------------------------------------------------
    // Debugging
    //-------------------------------------------------------------------------

    void AnimationGraphEditor::DrawDebuggerWindow( UpdateContext const& context, bool isFocused )
    {
        DrawRecorderUI( context );

        //-------------------------------------------------------------------------

        auto NavigateToSourceNode = [this] ( DebugPath const& path )
        {
            ClearGraphStack();

            // If this is a referenced/external graph then we need to remove the parent path
            //-------------------------------------------------------------------------

            DebugPath finalPath = path;

            if ( !m_pDebugGraphInstance->IsStandaloneInstance() )
            {
                EE_ASSERT( m_pHostGraphInstance != nullptr );
                DebugPath const pathToInstance = m_pHostGraphInstance->GetDebugPathForReferencedOrExternalGraphDebugInstance( PointerID( m_pDebugGraphInstance ) );
                finalPath.PopFront( pathToInstance.size() );
            }

            // Try to find node at path
            //-------------------------------------------------------------------------

            int32_t const numElements = (int32_t) finalPath.GetNumElements();
            for ( int32_t i = 0; i < numElements; i++ )
            {
                UUID const nodeID = m_userContext.GetGraphNodeUUID( (int16_t) finalPath[i].m_itemID );
                auto pVisualNode = m_loadedGraphStack.back()->GetRootGraph()->FindNode( nodeID, true );
                EE_ASSERT( pVisualNode != nullptr );

                if ( i == numElements - 1 )
                {
                    NavigateTo( pVisualNode );
                }
                else // Referenced Graph
                {
                    auto pReferencedGraphNode = Cast<ReferencedGraphToolsNode>( pVisualNode );
                    ResourceID const referencedGraphResourceID = pReferencedGraphNode->GetReferencedGraphResourceID( *m_userContext.m_pVariationHierarchy, m_userContext.m_selectedVariationID );
                    EE_ASSERT( referencedGraphResourceID.IsValid() && referencedGraphResourceID.GetResourceTypeID() == GraphDefinition::GetStaticResourceTypeID() );
                    PushOnGraphStack( pReferencedGraphNode, referencedGraphResourceID );
                }
            }
        };

        //-------------------------------------------------------------------------

        GraphInstance* pDebuggerInstance = m_pDebugGraphInstance;
        DebugPath pathToInstance;

        if ( m_pHostGraphInstance != nullptr )
        {
            pDebuggerInstance = m_pHostGraphInstance;
            pathToInstance = m_pHostGraphInstance->GetDebugPathForReferencedOrExternalGraphDebugInstance( PointerID( m_pDebugGraphInstance ) );
        }

        bool const showDebugData = IsDebugging() && pDebuggerInstance != nullptr && pDebuggerInstance->WasInitialized();
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Pose Tasks" ) )
        {
            if ( showDebugData )
            {
                AnimationDebugView::DrawGraphActiveTasksDebugView( pDebuggerInstance, pathToInstance, NavigateToSourceNode );
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
                AnimationDebugView::DrawRootMotionDebugView( pDebuggerInstance, pathToInstance, NavigateToSourceNode );
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
                AnimationDebugView::DrawSampledAnimationEventsView( pDebuggerInstance, pathToInstance, NavigateToSourceNode );
            }
            else
            {
                ImGui::Text( "-" );
            }
        }

        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Graph Events" ) )
        {
            if ( showDebugData )
            {
                AnimationDebugView::DrawSampledGraphEventsView( pDebuggerInstance, pathToInstance, NavigateToSourceNode );
            }
            else
            {
                ImGui::Text( "-" );
            }
        }
    }

    void AnimationGraphEditor::RequestDebugSession( UpdateContext const& context, DebugTarget target )
    {
        EE_ASSERT( target.IsValid() );

        // Try to compile the graph
        //-------------------------------------------------------------------------

        ToolsGraphDefinition* pToolsGraphDefinition = GetEditedGraphData()->m_pGraphDefinition;

        GraphDefinitionCompiler definitionCompiler;
        bool const graphCompiledSuccessfully = definitionCompiler.CompileGraph( *pToolsGraphDefinition, GetActiveVariation() );
        m_compilationLog = definitionCompiler.GetLog();

        // Compilation failed, stop preview attempt
        if ( !graphCompiledSuccessfully )
        {
            MessageDialog::Error( "Compile Error!", "The graph failed to compile! Please check the compilation log for details!" );
            ImGui::SetWindowFocus( GetToolWindowName( "Log" ).c_str() );
            return;
        }

        // Save the graph and compile it
        //-------------------------------------------------------------------------

        // Save the graph if needed
        if ( IsDirty() )
        {
            // Ensure that we save the graph and re-generate the dataset on preview
            Save();

            // Only force recompile, if we are not playing back a recording
            bool const requestImmediateCompilation = target.m_type != DebugTargetType::Recording;
            if ( requestImmediateCompilation )
            {
                ResourceID const graphVariationResourceID = Variation::GenerateResourceDataPath( m_pToolsContext->GetSourceDataDirectory(), GetDataFileSystemPath() , GetEditedGraphData()->m_activeVariationID );
                if ( !RequestImmediateResourceCompilation( graphVariationResourceID ) )
                {
                    return;
                }
            }
        }

        // Create the request
        //-------------------------------------------------------------------------

        m_requestedDebugTarget = target;
    }

    void AnimationGraphEditor::StartDebugging( UpdateContext const& context )
    {
        EE_ASSERT( !IsDirty() );
        EE_ASSERT( !IsDebugging() );
        EE_ASSERT( m_pPreviewEntity == nullptr );
        EE_ASSERT( m_pDebugGraphComponent == nullptr && m_pDebugGraphInstance == nullptr );
        EE_ASSERT( !IsHotReloading() );
        EE_ASSERT( m_requestedDebugTarget.IsValid() );

        // Create preview entity
        //-------------------------------------------------------------------------

        m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );

        // Set debug component data
        //-------------------------------------------------------------------------

        // If previewing, create the component
        if ( m_requestedDebugTarget.m_type == DebugTargetType::DirectPreview || m_requestedDebugTarget.m_type == DebugTargetType::Recording )
        {
            // Create resource ID for the graph variation
            StringID const selectedVariationID = GetEditedGraphData()->m_activeVariationID;
            ResourceID const graphDefinitionResourceID = Variation::GenerateResourceDataPath( m_pToolsContext->GetSourceDataDirectory(), GetDataFileSystemPath() , selectedVariationID );

            // Create Preview Graph Component
            EE_ASSERT( !m_previewGraphDefinitionPtr.IsSet() );
            m_previewGraphDefinitionPtr = TResourcePtr<GraphDefinition>( graphDefinitionResourceID );
            LoadResource( &m_previewGraphDefinitionPtr );

            m_pDebugGraphComponent = EE::New<GraphComponent>( StringID( "Animation Component" ) );
            m_pDebugGraphComponent->ShouldApplyRootMotionToEntity( true );
            m_pDebugGraphComponent->SetGraphDefinition( graphDefinitionResourceID );
            m_pDebugGraphComponent->SetSkeletonLOD( m_skeletonLOD );

            if ( m_requestedDebugTarget.m_type == DebugTargetType::Recording )
            {
                m_pDebugGraphComponent->SwitchToRecordingPlaybackMode();
            }

            m_pDebugGraphInstance = nullptr; // This will be set later when the component initializes
            m_pHostGraphInstance = nullptr;

            m_pPreviewEntity->AddComponent( m_pDebugGraphComponent );
            m_pPreviewEntity->CreateSystem<AnimationSystem>();

            m_debuggedEntityID = m_pPreviewEntity->GetID();
            m_debuggedComponentID = m_pDebugGraphComponent->GetID();
        }
        else // Use the supplied target
        {
            m_pDebugGraphComponent = m_requestedDebugTarget.m_pComponentToDebug;
            m_debuggedEntityID = m_pDebugGraphComponent->GetEntityID();
            m_debuggedComponentID = m_pDebugGraphComponent->GetID();

            switch ( m_requestedDebugTarget.m_type )
            {
                case DebugTargetType::MainGraph:
                {
                    m_pDebugGraphInstance = m_pDebugGraphComponent->GetDebugGraphInstance();
                }
                break;

                case DebugTargetType::ReferencedGraph:
                {
                    m_pHostGraphInstance = m_pDebugGraphComponent->GetDebugGraphInstance();
                    m_pDebugGraphInstance = const_cast<GraphInstance*>( m_pHostGraphInstance->GetReferencedGraphDebugInstance( m_requestedDebugTarget.m_referencedGraphID ) );
                    EE_ASSERT( m_pDebugGraphInstance != nullptr );
                }
                break;

                case DebugTargetType::ExternalGraph:
                {
                    m_debugExternalGraphSlotID = m_requestedDebugTarget.m_externalSlotID;
                    m_pHostGraphInstance = m_pDebugGraphComponent->GetDebugGraphInstance();
                    m_pDebugGraphInstance = const_cast<GraphInstance*>( m_pHostGraphInstance->GetExternalGraphDebugInstance( m_debugExternalGraphSlotID ) );
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
            if ( GetEditedGraphData()->m_activeVariationID != debuggedInstanceVariationID )
            {
                SetActiveVariation( debuggedInstanceVariationID );
            }
        }

        // Try Create Preview Mesh Component
        //-------------------------------------------------------------------------

        auto pVariation = GetEditedGraphData()->m_pGraphDefinition->GetVariation( GetEditedGraphData()->m_activeVariationID );
        EE_ASSERT( pVariation != nullptr );
        if ( pVariation->m_skeleton.IsSet() )
        {
            // Load resource descriptor for skeleton to get the preview mesh
            FileSystem::Path const resourceDescPath = GetFileSystemPath( pVariation->m_skeleton.GetDataPath() );
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
        if ( m_requestedDebugTarget.m_type == DebugTargetType::DirectPreview )
        {
            SetWorldPaused( m_startPaused );
            m_characterTransform = m_previewStartTransform;
            m_debugMode = DebugMode::Preview;
        }
        // Review
        else if ( m_requestedDebugTarget.m_type == DebugTargetType::Recording )
        {
            SetWorldPaused( false );
            m_characterTransform = m_graphRecorder.m_recordedData.front().m_characterWorldTransform;
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

        // Clear debug request
        //-------------------------------------------------------------------------

        m_requestedDebugTarget.Clear();
    }

    void AnimationGraphEditor::StopDebugging()
    {
        EE_ASSERT( m_debugMode != DebugMode::None );

        // Stop active recordings
        //-------------------------------------------------------------------------

        if ( m_isRecording )
        {
            StopRecording();
        }

        // Clear debug mode
        //-------------------------------------------------------------------------

        bool const isPreviewOrReview = IsPreviewOrReview();
        bool const isReviewingARecording = IsReviewingRecording();
        bool const isLiveDebug = IsLiveDebugging();

        // We need to clear the debug mode before we do anything else since the unload of a resource below might trigger a stop debug!
        m_debugMode = DebugMode::None;

        // Recording Review
        //-------------------------------------------------------------------------

        if ( isReviewingARecording )
        {
            m_currentReviewFrameIdx = InvalidIndex;
            m_reviewStarted = false;
        }

        // Destroy entity and clear debug state
        //-------------------------------------------------------------------------

        EE_ASSERT( m_pPreviewEntity != nullptr );
        DestroyEntityInWorld( m_pPreviewEntity );
        m_pPreviewEntity = nullptr;
        m_pDebugGraphComponent = nullptr;
        m_pDebugMeshComponent = nullptr;
        m_pDebugGraphInstance = nullptr;
        m_pHostGraphInstance = nullptr;
        m_debugExternalGraphSlotID.Clear();

        // Release variation reference
        //-------------------------------------------------------------------------

        if ( isPreviewOrReview )
        {
            EE_ASSERT( m_previewGraphDefinitionPtr.IsSet() );
            // Need to check this explicitly since it may have already been unloaded via a hot-reload request
            if( m_previewGraphDefinitionPtr.WasRequested() )
            {
                UnloadResource( &m_previewGraphDefinitionPtr );
            }
            m_previewGraphDefinitionPtr.Clear();
        }

        // Reset camera
        //-------------------------------------------------------------------------

        if ( isLiveDebug || m_isCameraTrackingEnabled )
        {
            m_previousCameraTransform = m_cameraOffsetTransform;
            SetCameraTransform( m_cameraOffsetTransform );
        }

        // Update graph views
        //-------------------------------------------------------------------------

        m_primaryGraphView.RefreshNodeSizes();
        m_secondaryGraphView.RefreshNodeSizes();

        // Clear debug mode
        //-------------------------------------------------------------------------

        ClearGraphStackDebugData();
        UpdateUserContext();
    }

    void AnimationGraphEditor::ReflectInitialPreviewParameterValues( UpdateContext const& context )
    {
        EE_ASSERT( m_isFirstPreviewFrame && m_pDebugGraphComponent != nullptr && m_pDebugGraphComponent->IsInitialized() );

        auto pRootGraph = GetEditedRootGraph();
        auto const controlParameters = pRootGraph->FindAllNodesOfType<ControlParameterToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Derived );
        for ( auto pControlParameter : controlParameters )
        {
            int16_t const parameterIdx = m_pDebugGraphComponent->GetControlParameterIndex( StringID( pControlParameter->GetParameterName() ) );

            // Can occur if the preview starts before the compile completes since we currently dont have a mechanism to force compile and wait in the editor
            if( parameterIdx == InvalidIndex )
            {
                continue;
            }

            switch ( pControlParameter->GetOutputValueType() )
            {
                case GraphValueType::Bool:
                {
                    auto pNode = TryCast<BoolControlParameterToolsNode>( pControlParameter );
                    m_pDebugGraphComponent->SetControlParameterValue( parameterIdx, pNode->GetPreviewStartValue() );
                }
                break;

                case GraphValueType::ID:
                {
                    auto pNode = TryCast<IDControlParameterToolsNode>( pControlParameter );
                    m_pDebugGraphComponent->SetControlParameterValue( parameterIdx, pNode->GetPreviewStartValue() );
                }
                break;

                case GraphValueType::Float:
                {
                    auto pNode = TryCast<FloatControlParameterToolsNode>( pControlParameter );
                    m_pDebugGraphComponent->SetControlParameterValue( parameterIdx, pNode->GetPreviewStartValue() );
                }
                break;

                case GraphValueType::Vector:
                {
                    auto pNode = TryCast<VectorControlParameterToolsNode>( pControlParameter );
                    m_pDebugGraphComponent->SetControlParameterValue( parameterIdx, pNode->GetPreviewStartValue() );
                }
                break;

                case GraphValueType::Target:
                {
                    auto pNode = TryCast<TargetControlParameterToolsNode>( pControlParameter );
                    m_pDebugGraphComponent->SetControlParameterValue( parameterIdx, pNode->GetPreviewStartValue() );
                }
                break;

                default:
                break;
            }
        }
    }

    void AnimationGraphEditor::DrawLiveDebugTargetsMenu( UpdateContext const& context )
    {
        ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );

        //-------------------------------------------------------------------------

        if ( IsDirty() )
        {
            ImGui::Text( "Graph is dirty! Please save before trying to attach!" );
            return;
        }

        //-------------------------------------------------------------------------

        bool hasTargets = false;
        auto const debugWorlds = m_pToolsContext->GetAllWorlds();
        for ( auto pWorld : m_pToolsContext->GetAllWorlds() )
        {
            if ( pWorld->GetWorldType() == EntityWorldType::Tools )
            {
                continue;
            }

            auto pWorldSystem = pWorld->GetWorldSystem<AnimationWorldSystem>();
            auto const& graphComponents = pWorldSystem->GetRegisteredGraphComponents();

            for ( GraphComponent* pGraphComponent : graphComponents )
            {
                if ( !pGraphComponent->HasGraphInstance() )
                {
                    continue;
                }

                Entity const* pEntity = pWorld->FindEntity( pGraphComponent->GetEntityID() );

                // Check main instance
                GraphInstance const* pGraphInstance = pGraphComponent->GetDebugGraphInstance();
                if ( pGraphInstance->GetDefinitionResourceID() == GetDataFilePath() )
                {
                    InlineString const targetName( InlineString::CtorSprintf(), "%s (%s)", pEntity->GetNameID().c_str(), pGraphComponent->GetNameID().c_str() );
                    if ( ImGui::MenuItem( targetName.c_str() ) )
                    {
                        DebugTarget target;
                        target.m_type = DebugTargetType::MainGraph;
                        target.m_pComponentToDebug = pGraphComponent;
                        RequestDebugSession( context, target );
                    }

                    hasTargets = true;
                }

                // Check referenced graph instances
                TVector<GraphInstance::DebuggableReferencedGraph> referencedGraphs;
                pGraphInstance->GetReferencedGraphsForDebug( referencedGraphs );
                for ( auto const& referencedGraph : referencedGraphs )
                {
                    if ( referencedGraph.m_pInstance->GetDefinitionResourceID() == GetDataFilePath() )
                    {
                        InlineString const targetName( InlineString::CtorSprintf(), "%s (%s) - %s", pEntity->GetNameID().c_str(), pGraphComponent->GetNameID().c_str(), referencedGraph.m_path.GetFlattenedPath().c_str() );
                        if ( ImGui::MenuItem( targetName.c_str() ) )
                        {
                            DebugTarget target;
                            target.m_type = DebugTargetType::ReferencedGraph;
                            target.m_pComponentToDebug = pGraphComponent;
                            target.m_referencedGraphID = referencedGraph.GetID();
                            RequestDebugSession( context, target );
                        }

                        hasTargets = true;
                    }
                }

                // Check external graph instances
                auto const& externalGraphs = pGraphInstance->GetExternalGraphsForDebug();
                for ( auto const& externalGraph : externalGraphs )
                {
                    if ( externalGraph.m_pInstance->GetDefinitionResourceID() == GetDataFilePath() )
                    {
                        InlineString const targetName( InlineString::CtorSprintf(), "%s (%s) - %s", pEntity->GetNameID().c_str(), pGraphComponent->GetNameID().c_str(), externalGraph.m_slotID.c_str() );
                        if ( ImGui::MenuItem( targetName.c_str() ) )
                        {
                            DebugTarget target;
                            target.m_type = DebugTargetType::ExternalGraph;
                            target.m_pComponentToDebug = pGraphComponent;
                            target.m_externalSlotID = externalGraph.m_slotID;
                            RequestDebugSession( context, target );
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

    void AnimationGraphEditor::CalculateCameraOffset()
    {
        m_previousCameraTransform = GetCameraTransform();
        m_cameraOffsetTransform = Transform::Delta( m_characterTransform, m_previousCameraTransform );
    }

    //-------------------------------------------------------------------------
    // Selection
    //-------------------------------------------------------------------------

    void AnimationGraphEditor::SetActiveVariation( StringID variationID )
    {
        EE_ASSERT( GetEditedGraphData()->m_pGraphDefinition->IsValidVariation( variationID ) );
        if ( GetEditedGraphData()->m_activeVariationID != variationID )
        {
            if ( IsDebugging() )
            {
                StopDebugging();
            }

            GetEditedGraphData()->m_activeVariationID = variationID;
            UpdateUserContext();
            RefreshVariationEditor();
        }
    }

    bool AnimationGraphEditor::TrySetActiveVariation( String const& variationName )
    {
        auto const& varHierarchy = GetEditedGraphData()->m_pGraphDefinition->GetVariationHierarchy();
        StringID const variationID = varHierarchy.TryGetCaseCorrectVariationID( variationName );
        if ( variationID.IsValid() )
        {
            SetActiveVariation( variationID );
            return true;
        }

        return false;
    }

    //-------------------------------------------------------------------------
    // User Context
    //-------------------------------------------------------------------------

    void AnimationGraphEditor::UpdateUserContext()
    {
        m_userContext.m_selectedVariationID = m_loadedGraphStack.back()->m_activeVariationID;
        m_userContext.m_pVariationHierarchy = &m_loadedGraphStack.back()->m_pGraphDefinition->GetVariationHierarchy();

        //-------------------------------------------------------------------------

        if ( IsDebugging() && m_pDebugGraphInstance != nullptr )
        {
            m_userContext.m_pGraphInstance = m_loadedGraphStack.back()->m_pGraphInstance;
            m_userContext.m_nodeIDtoIndexMap = m_loadedGraphStack.back()->m_nodeIDtoIndexMap;
            m_userContext.m_nodeIndexToIDMap = m_loadedGraphStack.back()->m_nodeIndexToIDMap;
        }
        else
        {
            m_userContext.m_pGraphInstance = nullptr;
            m_userContext.m_nodeIDtoIndexMap.clear();
            m_userContext.m_nodeIndexToIDMap.clear();
        }

        //-------------------------------------------------------------------------

        if ( m_loadedGraphStack.size() > 1 )
        {
            m_userContext.SetExtraGraphTitleInfoText( "External Referenced Graph" );
            m_userContext.SetExtraTitleInfoTextColor( Colors::HotPink );
        }
        else // In main graph
        {
            m_userContext.ResetExtraTitleInfo();
        }
    }

    //-------------------------------------------------------------------------
    // Graph View
    //-------------------------------------------------------------------------

    bool AnimationGraphEditor::IsInReadOnlyState() const
    {
        if ( IsDebugging() )
        {
            return true;
        }

        return m_loadedGraphStack.size() > 1;
    }

    void AnimationGraphEditor::DrawGraphViewNavigationBar()
    {
        auto pRootGraph = GetEditedRootGraph();

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
            NodeGraph::BaseNode*  m_pNode = nullptr;
            int32_t                 m_stackIdx = InvalidIndex;
            bool                    m_isStackBoundary = false;
        };

        TInlineVector<NavBarItem, 20> pathFromChildToRoot;

        int32_t pathStackIdx = int32_t( m_loadedGraphStack.size() ) - 1;
        auto pGraph = m_primaryGraphView.GetViewedGraph();
        NodeGraph::BaseNode* pParentNode = nullptr;

        do
        {
            // Switch stack if possible
            bool isStackBoundary = false;
            pParentNode = pGraph->GetParentNode();
            if ( pParentNode == nullptr && pathStackIdx > 0 )
            {
                EE_ASSERT( pGraph == m_loadedGraphStack[pathStackIdx]->m_pGraphDefinition->GetRootGraph() );
                pParentNode = m_loadedGraphStack[pathStackIdx]->m_pParentNode;
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

        if ( ImGuiX::ButtonColored( EE_ICON_HOME"##GoHome", Colors::Green, Colors::White, ImVec2( homeButtonWidth, buttonHeight ) ) )
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
        NodeGraph::BaseGraph* pGraphToNavigateTo = nullptr;

        // Draw from root to child
        for ( int32_t i = int32_t( pathFromChildToRoot.size() ) - 1; i >= 0; i-- )
        {
            bool const isLastItem = ( i == 0 );
            bool drawChevron = true;
            bool drawItem = true;

            auto const pParentState = TryCast<StateToolsNode>( pathFromChildToRoot[i].m_pNode->GetParentGraph()->GetParentNode() );
            auto const pState = TryCast<StateToolsNode>( pathFromChildToRoot[i].m_pNode );

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
                NodeGraph::BaseGraph* pChildGraphToCheck = nullptr;

                // Check external referenced graph for state machines
                if ( IsOfType<ReferencedGraphToolsNode>( pathFromChildToRoot[i].m_pNode ) )
                {
                    EE_ASSERT( ( pathFromChildToRoot[i].m_stackIdx + 1 ) < m_loadedGraphStack.size() );
                    pChildGraphToCheck = m_loadedGraphStack[pathFromChildToRoot[i].m_stackIdx + 1]->m_pGraphDefinition->GetRootGraph();
                }
                // We should search state machine graph
                else if ( !IsOfType<StateMachineToolsNode>( pathFromChildToRoot[i].m_pNode ) )
                {
                    pChildGraphToCheck = pathFromChildToRoot[i].m_pNode->GetChildGraph();
                }

                // If we have a graph to check then check for child state machines
                if ( pChildGraphToCheck != nullptr )
                {
                    auto const childStateMachines = pChildGraphToCheck->FindAllNodesOfType<StateMachineToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Exact );
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
                bool const isExternalReferencedGraphItem = pathFromChildToRoot[i].m_stackIdx > 0;
                if ( isExternalReferencedGraphItem )
                {
                    ImVec4 const color( Vector( 0.65f, 0.65f, 0.65f, 1.0f ) * Vector( ImGuiX::Style::s_colorText.ToFloat4() ) );
                    ImGui::PushStyleColor( ImGuiCol_Text, color );
                }

                ImGui::SameLine( 0, 0 );
                InlineString const str( InlineString::CtorSprintf(), "%s##%s", pathFromChildToRoot[i].m_pNode->GetName(), pathFromChildToRoot[i].m_pNode->GetID().ToString().c_str() );
                if ( ImGui::Button( str.c_str(), ImVec2( 0, buttonHeight ) ) )
                {
                    if ( isStateMachineState )
                    {
                        auto const childStateMachines = pathFromChildToRoot[i].m_pNode->GetChildGraph()->FindAllNodesOfType<StateMachineToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Exact );
                        EE_ASSERT( childStateMachines.size() );
                        pGraphToNavigateTo = childStateMachines[0]->GetChildGraph();
                    }
                    else
                    {
                        // Go to the external child graph's root graph
                        if ( auto pCG = TryCast<ReferencedGraphToolsNode>( pathFromChildToRoot[i].m_pNode ) )
                        {
                            EE_ASSERT( m_loadedGraphStack[pathFromChildToRoot[i].m_stackIdx + 1]->m_pParentNode == pCG );
                            pGraphToNavigateTo = m_loadedGraphStack[pathFromChildToRoot[i].m_stackIdx + 1]->m_pGraphDefinition->GetRootGraph();
                        }
                        else // Go to the node's child graph
                        {
                            pGraphToNavigateTo = pathFromChildToRoot[i].m_pNode->GetChildGraph();
                        }
                    }
                }

                if ( isExternalReferencedGraphItem )
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
            if ( auto pSM = TryCast<StateMachineToolsNode>( m_pBreadcrumbPopupContext ) )
            {
                auto const childStates = pSM->GetChildGraph()->FindAllNodesOfType<StateToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Derived );
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
                            auto const childStateMachines = pChildState->GetChildGraph()->FindAllNodesOfType<StateMachineToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Exact );
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
                NodeGraph::BaseGraph* pGraphToSearch = nullptr;

                if ( m_pBreadcrumbPopupContext == nullptr )
                {
                    pGraphToSearch = pRootGraph;
                }
                else if ( auto pCG = TryCast<ReferencedGraphToolsNode>( m_pBreadcrumbPopupContext ) )
                {
                    int32_t const nodeStackIdx = GetStackIndexForNode( pCG );
                    int32_t const referencedGraphStackIdx = nodeStackIdx + 1;
                    EE_ASSERT( referencedGraphStackIdx < m_loadedGraphStack.size() );
                    pGraphToSearch = m_loadedGraphStack[referencedGraphStackIdx]->m_pGraphDefinition->GetRootGraph();
                }
                else
                {
                    pGraphToSearch = m_pBreadcrumbPopupContext->GetChildGraph();
                }

                EE_ASSERT( pGraphToSearch != nullptr );

                auto childSMs = pGraphToSearch->FindAllNodesOfType<StateMachineToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Derived );
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
            if ( ImGuiX::ButtonColored( EE_ICON_DOOR_OPEN" Entry", Colors::Green, Colors::White, ImVec2( stateMachineNavButtonWidth, buttonHeight ) ) )
            {
                auto pSM = Cast<StateMachineGraph>( m_primaryGraphView.GetViewedGraph() );
                NavigateTo( pSM->GetEntryStateOverrideConduit(), false );
            }
            ImGuiX::ItemTooltip( "Entry State Overrides" );

            ImGui::SameLine( 0, -1 );
            if ( ImGuiX::ButtonColored( EE_ICON_LIGHTNING_BOLT"Global", Colors::OrangeRed, Colors::White, ImVec2( stateMachineNavButtonWidth, buttonHeight ) ) )
            {
                auto pSM = Cast<StateMachineGraph>( m_primaryGraphView.GetViewedGraph() );
                NavigateTo( pSM->GetGlobalTransitionConduit(), false );
            }
            ImGuiX::ItemTooltip( "Global Transitions" );
        }
    }

    void AnimationGraphEditor::DrawGraphView( UpdateContext const& context, bool isFocused )
    {
        EE_ASSERT( &m_userContext != nullptr );

        //-------------------------------------------------------------------------

        {
            auto pTypeRegistry = context.GetSystem<TypeSystem::TypeRegistry>();

            // Navigation Bar
            //-------------------------------------------------------------------------

            ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 2 ) );
            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 1, 1 ) );
            ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 4, 1 ) );
            if ( ImGui::BeginChild( "NavBar", ImVec2( ImGui::GetContentRegionAvail().x, 24 ), ImGuiChildFlags_AlwaysUseWindowPadding ) )
            {
                DrawGraphViewNavigationBar();
            }
            ImGui::EndChild();
            ImGui::PopStyleVar( 3 );

            // Primary View
            //-------------------------------------------------------------------------

            ImVec2 const availableRegion = ImGui::GetContentRegionAvail();
            m_primaryGraphView.UpdateAndDraw( *pTypeRegistry, availableRegion.y * m_primaryGraphViewProportionalHeight );

            if ( m_primaryGraphView.HasSelectionChangedThisFrame() )
            {
                SetSelectedNodes( m_primaryGraphView.GetSelectedNodes() );
            }

            // Splitter
            //-------------------------------------------------------------------------

            ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 0.0f );
            ImGui::Button( "##GraphViewSplitter", ImVec2( -1, 5 ) );
            ImGui::PopStyleVar();

            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetMouseCursor( ImGuiMouseCursor_ResizeNS );
            }

            if ( ImGui::IsItemActive() )
            {
                m_primaryGraphViewProportionalHeight += ( ImGui::GetIO().MouseDelta.y / availableRegion.y );
                m_primaryGraphViewProportionalHeight = Math::Max( 0.1f, m_primaryGraphViewProportionalHeight );
            }

            // SecondaryView
            //-------------------------------------------------------------------------

            UpdateSecondaryViewState();
            m_secondaryGraphView.UpdateAndDraw( *pTypeRegistry );

            if ( m_secondaryGraphView.HasSelectionChangedThisFrame() )
            {
                SetSelectedNodes( m_secondaryGraphView.GetSelectedNodes() );

                if ( m_selectedNodes.empty() )
                {
                    m_selectedNodes = m_primaryGraphView.GetSelectedNodes();
                }
            }
        }

        // Handle Focus and selection
        //-------------------------------------------------------------------------

        NodeGraph::GraphView* pCurrentlyFocusedGraphView = nullptr;

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

    void AnimationGraphEditor::UpdateSecondaryViewState()
    {
        NodeGraph::BaseGraph* pSecondaryGraphToView = nullptr;

        if ( m_primaryGraphView.HasSelectedNodes() )
        {
            if ( m_primaryGraphView.GetSelectedNodes().size() == 1 )
            {
                auto pSelectedNode = m_primaryGraphView.GetSelectedNodes().back().m_pNode;
                if ( pSelectedNode->HasSecondaryGraph() )
                {
                    if ( auto pSecondaryGraph = TryCast<NodeGraph::FlowGraph>( pSelectedNode->GetSecondaryGraph() ) )
                    {
                        pSecondaryGraphToView = pSecondaryGraph;
                    }
                }
                else if ( auto pParameterReference = TryCast<ParameterReferenceToolsNode>( pSelectedNode ) )
                {
                    if ( auto pVP = pParameterReference->GetReferencedVirtualParameter() )
                    {
                        pSecondaryGraphToView = pVP->GetChildGraph();
                    }
                }
            }
        }

        if ( m_secondaryGraphView.GetViewedGraph() != pSecondaryGraphToView )
        {
            m_secondaryGraphView.SetGraphToView( pSecondaryGraphToView );
        }
    }

    int32_t AnimationGraphEditor::GetStackIndexForNode( NodeGraph::BaseNode* pNode ) const
    {
        EE_ASSERT( pNode != nullptr );

        for ( auto i = 0; i < m_loadedGraphStack.size(); i++ )
        {
            auto const pFoundNode = m_loadedGraphStack[i]->m_pGraphDefinition->GetRootGraph()->FindNode( pNode->GetID(), true );
            if ( pFoundNode != nullptr )
            {
                EE_ASSERT( pFoundNode == pNode );
                return i;
            }
        }

        return InvalidIndex;
    }

    int32_t AnimationGraphEditor::GetStackIndexForGraph( NodeGraph::BaseGraph* pGraph ) const
    {
        EE_ASSERT( pGraph != nullptr );

        for ( auto i = 0; i < m_loadedGraphStack.size(); i++ )
        {
            auto const pFoundGraph = m_loadedGraphStack[i]->m_pGraphDefinition->GetRootGraph()->FindPrimaryGraph( pGraph->GetID() );
            if ( pFoundGraph != nullptr )
            {
                EE_ASSERT( pFoundGraph == pGraph ); // Multiple graphs with the same ID!
                return i;
            }
        }

        return InvalidIndex;
    }

    void AnimationGraphEditor::PushOnGraphStack( NodeGraph::BaseNode* pSourceNode, ResourceID const& graphID )
    {
        EE_ASSERT( pSourceNode != nullptr );
        EE_ASSERT( graphID.IsValid() && graphID.GetResourceTypeID() == GraphDefinition::GetStaticResourceTypeID() );

        ResourceID const graphDefinitionResourceID = Variation::GetGraphResourceID( graphID );
        FileSystem::Path const referencedGraphFilePath = GetFileSystemPath( graphDefinitionResourceID.GetDataPath() );
        if ( referencedGraphFilePath.IsValid() )
        {
            // Try to load the graph from the file
            auto pReferencedGraphStackData = m_loadedGraphStack.emplace_back( EE::New<LoadedGraphData>() );

            // Load child graph
            if ( GraphResourceDescriptor::TryReadFromFile( *m_pToolsContext->m_pTypeRegistry, referencedGraphFilePath, pReferencedGraphStackData->m_descriptor ) )
            {
                // Set ptr
                pReferencedGraphStackData->m_pGraphDefinition = &pReferencedGraphStackData->m_descriptor.m_graphDefinition;

                // Set up variation and other aspects
                StringID const variationID = pReferencedGraphStackData->m_pGraphDefinition->GetVariationHierarchy().TryGetCaseCorrectVariationID( Variation::GetVariationNameFromResourceID( graphID ) );
                pReferencedGraphStackData->m_activeVariationID = variationID;
                pReferencedGraphStackData->m_pParentNode = pSourceNode;
                NavigateTo( pReferencedGraphStackData->m_pGraphDefinition->GetRootGraph() );

                if ( IsDebugging() )
                {
                    if ( !GenerateGraphStackDebugData() )
                    {
                        StopDebugging();
                    }
                }
                UpdateUserContext();
            }
            else
            {
                MessageDialog::Error( "Error!", "Failed to load referenced graph!" );
                EE::Delete( pReferencedGraphStackData );
                m_loadedGraphStack.pop_back();
            }
        }
        else // Show error dialog
        {
            MessageDialog::Error( "Error!", "Invalid referenced graph resource ID" );
        }
    }

    void AnimationGraphEditor::PopGraphStack()
    {
        EE_ASSERT( m_loadedGraphStack.size() > 1 );
        EE::Delete( m_loadedGraphStack.back() );
        m_loadedGraphStack.pop_back();

        UpdateUserContext();
    }

    void AnimationGraphEditor::ClearGraphStack()
    {
        if ( m_loadedGraphStack.empty() )
        {
            return;
        }

        if ( m_loadedGraphStack[0]->m_pGraphDefinition != nullptr )
        {
            NavigateTo( GetEditedRootGraph() );
        }
        else
        {
            ClearSelection();
            m_primaryGraphView.SetGraphToView( nullptr );
            UpdateSecondaryViewState();
        }

        for ( auto i = int32_t( m_loadedGraphStack.size() ) - 1; i > 0; i-- )
        {
            EE::Delete( m_loadedGraphStack[i] );
            m_loadedGraphStack.pop_back();
        }

        UpdateUserContext();

        EE_ASSERT( m_loadedGraphStack.size() == 1 );
    }

    bool AnimationGraphEditor::GenerateGraphStackDebugData()
    {
        ClearGraphStackDebugData();

        //-------------------------------------------------------------------------

        if ( IsDebugging() )
        {
            GraphDefinitionCompiler definitionCompiler;

            TVector<int16_t> referencedGraphPathNodeIndices;
            for ( auto i = 0; i < m_loadedGraphStack.size(); i++ )
            {
                // Compile the external referenced graph
                if ( !definitionCompiler.CompileGraph( *m_loadedGraphStack[i]->m_pGraphDefinition, m_loadedGraphStack[i]->m_activeVariationID ) )
                {
                    MessageDialog::Error( "Compilation Error", "Failed to compile graph - Stopping Debug!" );
                    return false;
                }

                // Record node mappings for this graph
                m_loadedGraphStack[i]->m_nodeIDtoIndexMap = definitionCompiler.GetUUIDToRuntimeIndexMap();
                m_loadedGraphStack[i]->m_nodeIndexToIDMap = definitionCompiler.GetRuntimeIndexToUUIDMap();

                // Set the debug graph instance
                if ( i == 0 )
                {
                    m_loadedGraphStack[i]->m_pGraphInstance = m_pDebugGraphInstance;
                }
                else // Get the instance for the specified node idx
                {
                    LoadedGraphData* pParentStack = m_loadedGraphStack[i - 1];
                    auto const foundIter = pParentStack->m_nodeIDtoIndexMap.find( m_loadedGraphStack[i]->m_pParentNode->GetID() );
                    EE_ASSERT( foundIter != pParentStack->m_nodeIDtoIndexMap.end() );
                    m_loadedGraphStack[i]->m_pGraphInstance = const_cast<GraphInstance*>( pParentStack->m_pGraphInstance->GetReferencedGraphDebugInstance( foundIter->second ) );
                }
            }
        }

        return true;
    }

    void AnimationGraphEditor::ClearGraphStackDebugData()
    {
        for ( auto i = 0; i < m_loadedGraphStack.size(); i++ )
        {
            m_loadedGraphStack[i]->m_pGraphInstance = nullptr;
            m_loadedGraphStack[i]->m_nodeIDtoIndexMap.clear();
            m_loadedGraphStack[i]->m_nodeIndexToIDMap.clear();
        }
    }

    //-------------------------------------------------------------------------
    // Navigation
    //-------------------------------------------------------------------------

    void AnimationGraphEditor::NavigationTarget::CreateNavigationTargets( NodeGraph::BaseNode const* pNode, TVector<NavigationTarget>& outTargets )
    {
        // Skip control and virtual parameters
        //-------------------------------------------------------------------------

        if ( IsOfType<ControlParameterToolsNode>( pNode ) || IsOfType<VirtualParameterToolsNode>( pNode ) )
        {
            return;
        }

        // Get type of target
        //-------------------------------------------------------------------------

        TVector<StringID> foundIDs;
        NavigationTarget::Type type = NavigationTarget::Type::Unknown;

        if ( IsOfType<ParameterReferenceToolsNode>( pNode ) )
        {
            type = NavigationTarget::Type::Parameter;
        }
        else if ( auto pStateNode = TryCast<StateToolsNode>( pNode ) )
        {
            type = NavigationTarget::Type::Pose;
            pStateNode->GetLogicAndEventIDs( foundIDs );
        }
        else if ( auto pFlowNode = TryCast<FlowToolsNode>( pNode ) )
        {
            GraphValueType const valueType = pFlowNode->GetOutputValueType();
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
            addedNodeTarget.m_path = pNode->GetStringPathFromRoot();
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
                if ( !ID.IsValid() )
                {
                    continue;
                }

                auto& addedIDTarget = outTargets.emplace_back( NavigationTarget() );
                addedIDTarget.m_pNode = pNode;
                addedIDTarget.m_label = ID.c_str();
                addedIDTarget.m_path = pNode->GetStringPathFromRoot();
                addedIDTarget.m_sortKey = addedIDTarget.m_path + addedIDTarget.m_label;
                addedIDTarget.m_compKey = StringID( addedIDTarget.m_sortKey );
                addedIDTarget.m_type = Type::ID;
            }
        }
    }

    void AnimationGraphEditor::NavigateTo( NodeGraph::BaseNode* pNode, bool focusViewOnNode )
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
            SetSelectedNodes( { NodeGraph::SelectedNode( pNode ) } );
            if ( focusViewOnNode )
            {
                m_primaryGraphView.CenterView( pNode );
            }
        }
        else if ( m_secondaryGraphView.GetViewedGraph() != nullptr && m_secondaryGraphView.GetViewedGraph()->FindNode( pNode->GetID() ) )
        {
            m_secondaryGraphView.SelectNode( pNode );
            SetSelectedNodes( { NodeGraph::SelectedNode( pNode ) } );
            if ( focusViewOnNode )
            {
                m_secondaryGraphView.CenterView( pNode );
            }
        }
    }

    void AnimationGraphEditor::NavigateTo( NodeGraph::BaseGraph* pGraph )
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
                while ( stackIdx < ( m_loadedGraphStack.size() - 1 ) )
                {
                    PopGraphStack();
                }

                m_primaryGraphView.SetGraphToView( pGraph );
                UpdateSecondaryViewState();
            }
        }
    }

    void AnimationGraphEditor::StartNavigationOperation()
    {
        m_navigationTargetNodes.clear();
        m_navigationActiveTargetNodes.clear();
        GenerateNavigationTargetList();
        m_dialogManager.CreateModalDialog( "Navigate", [this] ( UpdateContext const& context ) { return DrawNavigationDialogWindow( context ); }, ImVec2( 800, 600 ) );
        
    }

    void AnimationGraphEditor::GenerateNavigationTargetList()
    {
        FlowGraph const* pRootGraph = GetEditedRootGraph();

        m_navigationTargetNodes.clear();

        // Find all graph nodes
        //-------------------------------------------------------------------------

        auto const foundNodes = pRootGraph->FindAllNodesOfType<NodeGraph::BaseNode>( NodeGraph::SearchMode::Recursive, NodeGraph::SearchTypeMatch::Derived );
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

    void AnimationGraphEditor::GenerateActiveTargetList()
    {
        EE_ASSERT( IsDebugging() && m_userContext.HasDebugData() );

        m_navigationActiveTargetNodes.clear();

        for ( auto activeNodeIdx : m_userContext.GetActiveNodes() )
        {
            UUID const nodeID = m_userContext.GetGraphNodeUUID( activeNodeIdx );
            if ( nodeID.IsValid() )
            {
                auto pFoundNode = GetEditedRootGraph()->FindNode( nodeID, true );
                if ( pFoundNode != nullptr )
                {
                    NavigationTarget::CreateNavigationTargets( pFoundNode, m_navigationActiveTargetNodes );
                }
            }
        }
    }

    bool AnimationGraphEditor::DrawNavigationDialogWindow( UpdateContext const& context )
    {
        // Filter
        //-------------------------------------------------------------------------

        m_navigationFilter.UpdateAndDraw( ImGui::GetContentRegionAvail().x - 130, ImGuiX::FilterWidget::TakeInitialFocus );

        ImGui::SameLine();

        ImGuiX::Checkbox( "Include Paths", &m_navigationDialogSearchesPaths );

        auto ApplyFilter = [this] ( NavigationTarget const& target )
        {
            if ( m_navigationFilter.HasFilterSet() )
            {
                // Check node name
                if ( !m_navigationFilter.MatchesFilter( target.m_label ) )
                {
                    // Check path
                    if ( m_navigationDialogSearchesPaths )
                    {
                        if ( !m_navigationFilter.MatchesFilter( target.m_path ) )
                        {
                            return false;
                        }
                    }
                    else
                    {
                        return false;
                    }
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

                Color const colors[] = { Colors::Plum, ImGuiX::Style::s_colorText, Colors::LightSteelBlue, Colors::GoldenRod };
                static char const* const icons[] = { EE_ICON_ARROW_RIGHT_BOLD, EE_ICON_WALK, EE_ICON_NUMERIC, EE_ICON_TAG_TEXT };
                static char const* const tooltips[] = { "Parameter", "Pose", "Value", "ID" };
                int32_t const typeIndex = ( int32_t ) pTarget->m_type;

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::PushStyleColor( ImGuiCol_Text, isActiveTarget ? Colors::Green : ImGuiX::Style::s_colorText );
                ImGui::Text( icons[typeIndex] );
                ImGuiX::TextTooltip( tooltips[typeIndex] );
                ImGui::PopStyleColor();

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::PushStyleColor( ImGuiCol_Text, colors[typeIndex] );

                if ( ImGui::Selectable( pTarget->m_label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns ) )
                {
                    if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                    {
                        NavigateTo( const_cast<NodeGraph::BaseNode*>( pTarget->m_pNode ) );
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

        return true;
    }

    //-------------------------------------------------------------------------
    // Outliner
    //-------------------------------------------------------------------------

    class GraphOutlinerItem final : public TreeListViewItem
    {
    public:

        GraphOutlinerItem( TreeListViewItem* pParent, AnimationGraphEditor* pGraphEditor, NodeGraph::BaseNode* pNode )
            : TreeListViewItem( pParent )
            , m_pGraphEditor( pGraphEditor )
            , m_pNode( pNode )
            , m_nameID( pNode->GetName() )
        {
            EE_ASSERT( m_pGraphEditor != nullptr );
            m_displayColor = ImGuiX::Style::s_colorText;
        }

        GraphOutlinerItem( TreeListViewItem* pParent, AnimationGraphEditor* pGraphEditor, VariationDataToolsNode* pVariationDataNode )
            : TreeListViewItem( pParent )
            , m_pGraphEditor( pGraphEditor )
            , m_pNode( pVariationDataNode )
            , m_nameID( pVariationDataNode->GetName() )
        {
            EE_ASSERT( m_pGraphEditor != nullptr );
            m_displayColor = pVariationDataNode->GetTitleBarColor();
        }

        virtual uint64_t GetUniqueID() const override { return (uint64_t) m_pNode; }
        virtual StringID GetNameID() const override { return m_nameID; }
        virtual InlineString GetDisplayName() const
        {
            InlineString displayName;

            if ( IsOfType<StateMachineToolsNode>( m_pNode ) )
            {
                displayName.sprintf( EE_ICON_STATE_MACHINE" %s", m_pNode->GetName() );
            }
            else if ( IsOfType<ReferencedGraphToolsNode>( m_pNode ) )
            {
                displayName.sprintf( EE_ICON_GRAPH" %s", m_pNode->GetName() );
            }
            else if ( IsOfType<VariationDataToolsNode>( m_pNode ) )
            {
                displayName.sprintf( EE_ICON_ARROW_BOTTOM_RIGHT" %s", m_pNode->GetName() );
            }
            else
            {
                displayName.sprintf( m_pNode->GetName() );
            }

            return displayName;
        }

        virtual Color GetDisplayColor( ItemState state ) const override { return m_displayColor; }

        virtual bool OnDoubleClick() override { m_pGraphEditor->NavigateTo( m_pNode ); return false; }

    private:

        AnimationGraphEditor*       m_pGraphEditor = nullptr;
        NodeGraph::BaseNode*        m_pNode = nullptr;
        StringID                    m_nameID;
        Color                       m_displayColor;
    };

    void AnimationGraphEditor::DrawOutliner( UpdateContext const& context, bool isFocused )
    {
        constexpr static float const buttonWidth = 30;
        float const availableWidth = ImGui::GetContentRegionAvail().x;
        float const filterWidth = availableWidth - ( 2 * ( buttonWidth + ImGui::GetStyle().ItemSpacing.x ) );

        if ( m_outlinerFilterWidget.UpdateAndDraw( filterWidth ) )
        {
            m_outlinerTreeView.ForEachItem( [] ( TreeListViewItem* pItem ) { pItem->SetExpanded( true ); } );
            m_outlinerTreeView.UpdateItemVisibility( [this] ( TreeListViewItem const* pItem ) { return m_outlinerFilterWidget.MatchesFilter( pItem->GetDisplayName() ); } );
        }

        //-------------------------------------------------------------------------

        ImGui::SameLine();
        if ( ImGui::Button( EE_ICON_EXPAND_ALL "##Expand All", ImVec2( buttonWidth, 0 ) ) )
        {
            m_outlinerTreeView.ForEachItem( [] ( TreeListViewItem* pItem ) { pItem->SetExpanded( true ); } );
        }
        ImGuiX::ItemTooltip( "Expand All" );

        ImGui::SameLine();
        if ( ImGui::Button( EE_ICON_COLLAPSE_ALL "##Collapse ALL", ImVec2( buttonWidth, 0 ) ) )
        {
            m_outlinerTreeView.ForEachItem( [] ( TreeListViewItem* pItem ) { pItem->SetExpanded( false ); } );
        }
        ImGuiX::ItemTooltip( "Collapse All" );

        //-------------------------------------------------------------------------

        m_outlinerTreeView.UpdateAndDraw( m_outlinerTreeContext );
    }

    void AnimationGraphEditor::RefreshOutliner()
    {
        m_outlinerTreeView.RebuildTree( m_outlinerTreeContext );
    }

    void AnimationGraphEditor::RebuildOutlinerTree( TreeListViewItem* pRootItem )
    {
        auto pRootGraph = GetEditedRootGraph();

        // Get all states
        auto stateNodes = pRootGraph->FindAllNodesOfType<StateToolsNode>( NodeGraph::SearchMode::Recursive, NodeGraph::SearchTypeMatch::Derived );

        // Get all variation data nodes
        auto variationDataNodes = pRootGraph->FindAllNodesOfType<VariationDataToolsNode>( NodeGraph::SearchMode::Recursive, NodeGraph::SearchTypeMatch::Derived );

        // Construction Helper
        //-------------------------------------------------------------------------

        THashMap<UUID, TreeListViewItem*> constructionMap;

        TVector<NodeGraph::BaseNode*> nodePath;

        auto CreateParentItem = [&] ( TreeListViewItem* pRootItem, NodeGraph::BaseNode* pNode ) -> TreeListViewItem*
        {
            nodePath.clear();
            nodePath = pNode->GetNodePathFromRoot();
            nodePath.pop_back();

            TreeListViewItem* pLastFoundParentItem = pRootItem;
            for ( auto i = 0; i < nodePath.size(); i++ )
            {
                // Try to see if this item exists
                auto foundParentIter = constructionMap.find( nodePath[i]->GetID());
                if ( foundParentIter != constructionMap.end() )
                {
                    pLastFoundParentItem = foundParentIter->second;
                    continue;
                }
                else // This item doesnt exist so we need to create all subsequent node path items
                {
                    for ( auto j = i; j < nodePath.size(); j++ )
                    {
                        pLastFoundParentItem = pLastFoundParentItem->CreateChild<GraphOutlinerItem>( this, nodePath[j] );
                        constructionMap.insert( TPair<UUID, TreeListViewItem*>( nodePath[j]->GetID(), pLastFoundParentItem ) );
                    }

                    break;
                }
            }

            return pLastFoundParentItem;
        };

        auto FindParentItem = [&] ( TreeListViewItem* pRootItem, NodeGraph::BaseNode* pNode ) -> TreeListViewItem*
        {
            auto pParentNode = pNode->GetParentNode();
            if ( pParentNode == nullptr )
            {
                return pRootItem;
            }

            auto foundParentIter = constructionMap.find( pParentNode->GetID() );
            if ( foundParentIter != constructionMap.end() )
            {
                return (TreeListViewItem*) foundParentIter->second;
            }
            else
            {
                return CreateParentItem( pRootItem, pNode );
            }
        };

        // Create tree
        //-------------------------------------------------------------------------

        for ( auto pStateNode : stateNodes )
        {
            auto pParentItem = FindParentItem( pRootItem, pStateNode );
            auto pCreatedItem = pParentItem->CreateChild<GraphOutlinerItem>( this, pStateNode );
            constructionMap.insert( TPair<UUID, TreeListViewItem*>( pStateNode->GetID(), pCreatedItem ) );
        }

        for ( auto pVariationDataNode : variationDataNodes )
        {
            auto pParentItem = FindParentItem( pRootItem, pVariationDataNode );
            auto pCreatedItem = pParentItem->CreateChild<GraphOutlinerItem>( this, pVariationDataNode );
            constructionMap.insert( TPair<UUID, TreeListViewItem*>( pVariationDataNode->GetID(), pCreatedItem ) );
        }
    }

    //-------------------------------------------------------------------------
    // Property Grid
    //-------------------------------------------------------------------------

    void AnimationGraphEditor::DrawNodeEditor( UpdateContext const& context, bool isFocused )
    {
        if ( m_selectedNodes.empty() || m_selectedNodes.size() > 1 )
        {
            m_nodeEditorPropertyGrid.SetTypeToEdit( nullptr );
            m_nodeVariationDataPropertyGrid.SetTypeToEdit( nullptr );
            return;
        }

        // Update node editor property grid
        //-------------------------------------------------------------------------

        auto pSelectedNode = m_selectedNodes.back().m_pNode;
        if ( m_nodeEditorPropertyGrid.GetEditedType() != pSelectedNode )
        {
            // Handle control parameters as a special case
            auto pReferenceNode = TryCast<ParameterReferenceToolsNode>( pSelectedNode );
            if ( pReferenceNode != nullptr && pReferenceNode->IsReferencingControlParameter() )
            {
                m_nodeEditorPropertyGrid.SetTypeToEdit( pReferenceNode->GetReferencedControlParameter() );
            }
            else
            {
                m_nodeEditorPropertyGrid.SetTypeToEdit( pSelectedNode );
            }
        }

        // Update node variation data property grid
        //-------------------------------------------------------------------------

        StringID const activeVariationID = GetActiveVariation();
        bool const isDefaultVariation = activeVariationID == Variation::s_defaultVariationID;
        VariationDataToolsNode* pVariationDataNode = TryCast<VariationDataToolsNode>( pSelectedNode );

        bool const shouldDrawVariationPropertyGrid = pVariationDataNode != nullptr;
        bool const hasVariationData = isDefaultVariation || ( pVariationDataNode ? pVariationDataNode->HasVariationOverride( activeVariationID ) : false );

        if ( pVariationDataNode != nullptr )
        {
            // Get the resolved data object, this is only editable if it is the default variation or an explicit override for the current variation
            auto pRootGraph = GetEditedRootGraph();
            auto& variationHierarchy = GetEditedGraphData()->m_pGraphDefinition->GetVariationHierarchy();
            auto pData = pVariationDataNode->GetResolvedVariationData( variationHierarchy, activeVariationID );
            if ( m_nodeVariationDataPropertyGrid.GetEditedType() != pData )
            {
                m_nodeVariationDataPropertyGrid.SetTypeToEdit( pData );
            }

            m_nodeVariationDataPropertyGrid.SetReadOnly( IsInReadOnlyState() || !hasVariationData );
        }
        else
        {
            m_nodeVariationDataPropertyGrid.SetTypeToEdit( nullptr );
        }

        // Draw UI
        //-------------------------------------------------------------------------

        // Header
        InlineString separatorLabel( InlineString::CtorSprintf(), "Node: %s", m_selectedNodes.back().m_pNode->GetName() );
        ImGui::SeparatorText( separatorLabel.c_str() );

        // Draw property grid
        if ( shouldDrawVariationPropertyGrid )
        {
            if ( ImGui::BeginChild( "ResizableChild", ImVec2( -FLT_MIN, m_nodeEditorPropertyGrid.GetMinimumHeight() ), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY ) )
            {
                m_nodeEditorPropertyGrid.UpdateAndDraw();
            }
            ImGui::EndChild();
        }
        else
        {
            m_nodeEditorPropertyGrid.UpdateAndDraw();
        }

        if ( shouldDrawVariationPropertyGrid )
        {
            // Header
            separatorLabel = InlineString( InlineString::CtorSprintf(), "Variation Data: %s", activeVariationID.c_str() );
            ImGui::SeparatorText( separatorLabel.c_str() );

            // Draw override controls
            if ( !isDefaultVariation )
            {
                if ( hasVariationData )
                {
                    if ( ImGuiX::ButtonColored( EE_ICON_CLOSE" Remove Override", ImGuiX::Style::s_colorGray3, Colors::MediumRed, ImVec2( -1, 0 ) ) )
                    {
                        m_variationRequests.emplace_back( pVariationDataNode, false );
                    }
                    ImGuiX::ItemTooltip( "Remove Override" );
                }
                else
                {
                    if ( ImGuiX::ButtonColored( EE_ICON_PLUS" Add Override", ImGuiX::Style::s_colorGray3, Colors::LimeGreen, ImVec2( -1, 0 ) ) )
                    {
                        m_variationRequests.emplace_back( pVariationDataNode, true );
                    }
                    ImGuiX::ItemTooltip( "Add Override" );
                }
            }

            // Draw property grid
            m_nodeVariationDataPropertyGrid.UpdateAndDraw();
        }
    }

    //-------------------------------------------------------------------------
    // Graph Log
    //-------------------------------------------------------------------------

    void AnimationGraphEditor::DrawGraphLog( UpdateContext const& context, bool isFocused )
    {
        if ( m_compilationLog.empty() )
        {
            ImGui::TextColored( Colors::LimeGreen.ToFloat4(), EE_ICON_CHECK );
            ImGui::SameLine();
            ImGui::Text( "Graph Compiled Successfully" );
        }
        else // Draw compilation log
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
                            case Severity::Warning:
                            ImGui::TextColored( Colors::Yellow.ToFloat4(), EE_ICON_ALERT_OCTAGON );
                            break;

                            case Severity::Error:
                            ImGui::TextColored( Colors::Red.ToFloat4(), EE_ICON_ALERT );
                            break;

                            case Severity::Info:
                            ImGui::Text( EE_ICON_INFORMATION );
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
                                auto pFoundNode = GetEditedRootGraph()->FindNode( entry.m_nodeID, true );
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

        //-------------------------------------------------------------------------

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
                            case Severity::Warning:
                            ImGui::TextColored( Colors::Yellow.ToFloat4(), EE_ICON_ALERT_OCTAGON );
                            break;

                            case Severity::Error:
                            ImGui::TextColored( Colors::Red.ToFloat4(), EE_ICON_ALERT );
                            break;

                            case Severity::Info:
                            ImGui::Text( EE_ICON_INFORMATION );
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
                                    auto pFoundNode = GetEditedRootGraph()->FindNode( nodeID, true );
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
    }

    //-------------------------------------------------------------------------
    // Control Parameter Editor
    //-------------------------------------------------------------------------

    void AnimationGraphEditor::DrawControlParameterEditor( UpdateContext const& context, bool isFocused )
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

    void AnimationGraphEditor::RefreshControlParameterCache()
    {
        auto pRootGraph = GetEditedRootGraph();
        m_parameters = pRootGraph->FindAllNodesOfType<ParameterBaseToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Derived );
        RefreshParameterGroupTree();
    }

    void AnimationGraphEditor::RefreshParameterGroupTree()
    {
        m_parameterGroupTree.RemoveAllItems();

        for ( auto pParameter : m_parameters )
        {
            m_parameterGroupTree.AddItem( pParameter->GetParameterGroup(), pParameter->GetName(), pParameter );
        }

        m_parameterGroupTree.RemoveEmptyCategories();

        //-------------------------------------------------------------------------

        m_cachedUses.clear();

        auto pRootGraph = GetEditedRootGraph();
        auto referenceNodes = pRootGraph->FindAllNodesOfType<ParameterReferenceToolsNode>( NodeGraph::SearchMode::Recursive, NodeGraph::SearchTypeMatch::Derived );
        for ( auto pReferenceNode : referenceNodes )
        {
            auto const& ID = pReferenceNode->GetReferencedParameter()->GetID();
            auto iter = m_cachedUses.find( ID );
            if ( iter == m_cachedUses.end() )
            {
                m_cachedUses.insert( TPair<UUID, TVector<UUID>>( ID, {} ) );
                iter = m_cachedUses.find( ID );
            }

            iter->second.emplace_back( pReferenceNode->GetID() );
        }
    }

    bool AnimationGraphEditor::DrawCreateOrRenameParameterDialogWindow( UpdateContext const& context, bool isRename )
    {
        bool isDialogOpen = true;

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
        ImGui::Text( "Group: " );
        ImGui::SameLine( fieldOffset );

        auto OptionsCombo = [this] ()
        {
            for ( auto const& category : m_parameterGroupTree.GetRootCategory().m_childCategories )
            {
                if ( ImGui::MenuItem( category.m_name.c_str() ) )
                {
                    strncpy_s( m_parameterGroupBuffer, category.m_name.c_str(), 255 );
                }
            }
        };

        ImGui::SetNextItemWidth( -1 );
        if ( ImGuiX::InputTextCombo( "##ParameterGroup", m_parameterGroupBuffer, 255, OptionsCombo, ImGuiInputTextFlags_EnterReturnsTrue ) )
        {
            updateConfirmed = true;
        }

        //-------------------------------------------------------------------------

        ImGui::NewLine();

        float const dialogWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SameLine( 0, dialogWidth - 104 );

        //-------------------------------------------------------------------------

        // Rename existing parameter
        if ( isRename )
        {
            if ( auto pParameter = FindParameter( m_currentOperationParameterID ) )
            {
                bool const isTheSame = ( pParameter->GetParameterName().comparei( m_parameterNameBuffer ) == 0 && pParameter->GetParameterGroup().comparei( m_parameterGroupBuffer ) == 0 );

                ImGui::BeginDisabled( isTheSame );
                if ( ImGui::Button( "Ok", ImVec2( 50, 0 ) ) || updateConfirmed )
                {
                    RenameParameter( m_currentOperationParameterID, m_parameterNameBuffer, m_parameterGroupBuffer );
                    isDialogOpen = false;
                }
                ImGui::EndDisabled();
            }
        }
        else // Create new parameter
        {
            if ( ImGui::Button( "Ok", ImVec2( 50, 0 ) ) || updateConfirmed )
            {
                CreateParameter( m_currentOperationParameterType, m_currentOperationParameterValueType, m_parameterNameBuffer, m_parameterGroupBuffer );
                isDialogOpen = false;
                m_currentOperationParameterValueType = GraphValueType::Unknown;
            }
        }

        ImGui::SameLine( 0, 4 );

        if ( ImGui::Button( "Cancel", ImVec2( 50, 0 ) ) )
        {
            isDialogOpen = false;
        }

        return isDialogOpen;
    }

    bool AnimationGraphEditor::DrawDeleteParameterDialogWindow( UpdateContext const& context )
    {
        bool isDialogOpen = true;

        auto iter = m_cachedUses.find( m_currentOperationParameterID );
        ImGui::Text( "This parameter is used in %d places.", iter != m_cachedUses.end() ? (int32_t) iter->second.size() : 0 );
        ImGui::Text( "Are you sure you want to delete this parameter?" );
        ImGui::NewLine();

        float const dialogWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SameLine( 0, dialogWidth - 64 );

        if ( ImGui::Button( "Yes", ImVec2( 30, 0 ) ) )
        {
            DestroyParameter( m_currentOperationParameterID );
            isDialogOpen = false;
        }

        ImGui::SameLine( 0, 4 );

        if ( ImGui::Button( "No", ImVec2( 30, 0 ) ) )
        {
            isDialogOpen = false;
        }

        return isDialogOpen;
    }

    bool AnimationGraphEditor::DrawDeleteUnusedParameterDialogWindow( UpdateContext const& context )
    {
        bool isDialogOpen = true;

        ImGui::Text( "Are you sure you want to delete all unused parameters?" );
        ImGui::NewLine();

        if ( ImGui::Button( "Yes", ImVec2( 30, 0 ) ) )
        {
            TInlineVector<UUID, 100> unusedParameters;
            for ( auto CP : m_parameters )
            {
                auto iter = m_cachedUses.find( CP->GetID() );
                if ( iter == m_cachedUses.end() || iter->second.empty() )
                {
                    unusedParameters.emplace_back( CP->GetID() );
                }
            }

            if ( !unusedParameters.empty() )
            {
                auto pRootGraph = GetEditedRootGraph();
                NodeGraph::ScopedGraphModification gm( pRootGraph );

                for ( auto const& CPID : unusedParameters )
                {
                    DestroyParameter( CPID );
                }
            }

            isDialogOpen = false;
        }

        ImGui::SameLine( 0, 4 );

        if ( ImGui::Button( "No", ImVec2( 30, 0 ) ) )
        {
            isDialogOpen = false;
        }

        return isDialogOpen;
    }

    void AnimationGraphEditor::ControlParameterGroupDragAndDropHandler( Category<ParameterBaseToolsNode*>& group )
    {
        InlineString payloadString;

        //-------------------------------------------------------------------------

        if ( ImGui::BeginDragDropTarget() )
        {
            if ( ImGuiPayload const* payload = ImGui::AcceptDragDropPayload( FlowGraph::s_graphParameterPayloadID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
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
            for ( auto pParameter : m_parameters )
            {
                if ( pParameter->GetParameterName().comparei( payloadString.c_str() ) == 0 )
                {
                    if ( pParameter->GetParameterGroup() != group.m_name )
                    {
                        RenameParameter( pParameter->GetID(), pParameter->GetParameterName(), group.m_name );
                        return;
                    }
                }
            }
        }
    }

    void AnimationGraphEditor::DrawParameterListRow( FlowToolsNode* pParameterNode )
    {
        constexpr float const iconWidth = 20;

        auto pCP = TryCast< ControlParameterToolsNode>( pParameterNode );
        auto pVP = TryCast< VirtualParameterToolsNode>( pParameterNode );
        InlineString const parameterName = ( pCP != nullptr ) ? pCP->GetParameterName().c_str() : pVP->GetParameterName().c_str();

        //-------------------------------------------------------------------------

        ImGui::PushID( pParameterNode );

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::Indent();

        ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) pParameterNode->GetTitleBarColor() );
        ImGui::Text( ( pVP != nullptr ) ? EE_ICON_ALPHA_V_CIRCLE : EE_ICON_ALPHA_C_BOX);
        ImGui::SameLine();
        if ( ImGui::Selectable( parameterName.c_str() ) )
        {
            if ( pCP != nullptr )
            {
                SetSelectedNodes( { NodeGraph::SelectedNode( pCP ) } );
            }
        }
        ImGui::PopStyleColor();

        // Context menu
        if ( ImGui::BeginPopupContextItem( "ParamOptions" ) )
        {
            if ( pVP != nullptr )
            {
                if ( ImGui::MenuItem( EE_ICON_PLAYLIST_EDIT" Edit Virtual Parameter" ) )
                {
                    NavigateTo( pParameterNode->GetChildGraph() );
                }

                ImGui::Separator();
            }

            if ( pCP )
            {
                if ( ImGui::MenuItem( EE_ICON_IDENTIFIER" Copy Parameter Name" ) )
                {
                    ImGui::SetClipboardText( parameterName.c_str() );
                }
            }

            if ( ImGui::MenuItem( EE_ICON_RENAME_BOX" Rename" ) )
            {
                StartParameterRename( pParameterNode->GetID() );
            }

            if ( ImGui::MenuItem( EE_ICON_DELETE" Delete" ) )
            {
                StartParameterDelete( pParameterNode->GetID() );
            }

            ImGui::EndPopup();
        }

        // Drag and drop
        if ( ImGui::BeginDragDropSource() )
        {
            ImGui::SetDragDropPayload( FlowGraph::s_graphParameterPayloadID, pParameterNode->GetName(), strlen( pParameterNode->GetName() ) + 1 );
            ImGui::Text( pParameterNode->GetName() );
            ImGui::EndDragDropSource();
        }
        ImGui::Unindent();

        // Uses
        //-------------------------------------------------------------------------

        ImGui::TableNextColumn();
        ImGui::PushStyleColor( ImGuiCol_Text, (ImVec4) pParameterNode->GetTitleBarColor() );
        ImGui::Text( GetNameForValueType( pParameterNode->GetOutputValueType() ) );
        ImGui::PopStyleColor();

        auto iter = m_cachedUses.find( pParameterNode->GetID() );
        ImGui::TableNextColumn();

        auto ParameterUsesContextMenu = [&] ()
        {
            EE_ASSERT( iter != m_cachedUses.end() );
            auto pRootGraph = GetEditedRootGraph();

            for ( auto const& nodeID : iter->second )
            {
                auto pNode = pRootGraph->FindNode( nodeID, true );
                if ( ImGui::MenuItem( pNode->GetStringPathFromRoot().c_str() ) )
                {
                    NavigateTo( pNode );
                }
            }
        };

        InlineString const buttonLabel( InlineString::CtorSprintf(), "%d##%s", iter != m_cachedUses.end() ? (int32_t) iter->second.size() : 0, parameterName.c_str() );
        ImGui::BeginDisabled( iter == m_cachedUses.end() );
        ImGuiX::DropDownButton( buttonLabel.c_str(), ParameterUsesContextMenu, ImVec2( 30, 0 ) );
        ImGui::EndDisabled();
        ImGuiX::ItemTooltip( "Number of uses" );

        ImGui::PopID();
    }

    void AnimationGraphEditor::DrawParameterList()
    {
        // Draw Control Bar
        //-------------------------------------------------------------------------

        constexpr static float const eraseButtonWidth = 30.0f;
        constexpr static float const virtualParameterButtonWidth = 80.0f;

        auto DrawNewParameterOptions = [this] ()
        {
            {
                ImGuiX::ScopedFont sf( GetColorForValueType( GraphValueType::Bool ) );
                if ( ImGui::MenuItem( "Bool" ) )
                {
                    StartParameterCreate( GraphValueType::Bool, ParameterType::Control );
                }
            }

            {
                ImGuiX::ScopedFont sf( GetColorForValueType( GraphValueType::ID ) );
                if ( ImGui::MenuItem( "ID" ) )
                {
                    StartParameterCreate( GraphValueType::ID, ParameterType::Control );
                }
            }

            {
                ImGuiX::ScopedFont sf( GetColorForValueType( GraphValueType::Float ) );
                if ( ImGui::MenuItem( "Float" ) )
                {
                    StartParameterCreate( GraphValueType::Float, ParameterType::Control );
                }
            }

            {
                ImGuiX::ScopedFont sf( GetColorForValueType( GraphValueType::Vector ) );
                if ( ImGui::MenuItem( "Vector" ) )
                {
                    StartParameterCreate( GraphValueType::Vector, ParameterType::Control );
                }
            }

            {
                ImGuiX::ScopedFont sf( GetColorForValueType( GraphValueType::Target ) );
                if ( ImGui::MenuItem( "Target" ) )
                {
                    StartParameterCreate( GraphValueType::Target, ParameterType::Control );
                }
            }
        };

        ImGuiX::DropDownIconButtonColored( EE_ICON_PLUS_THICK, "Add New Parameter", DrawNewParameterOptions, Colors::Green, Colors::White, Colors::White, ImVec2( ImGui::GetContentRegionAvail().x - eraseButtonWidth - virtualParameterButtonWidth - ( ImGui::GetStyle().ItemSpacing.x * 2 ), 0 ) );

        ImGui::SameLine();

        auto DrawNewVirtualParameterOptions = [this] ()
        {
            {
                ImGuiX::ScopedFont sf( GetColorForValueType( GraphValueType::Bool ) );
                if ( ImGui::MenuItem( "Bool" ) )
                {
                    StartParameterCreate( GraphValueType::Bool, ParameterType::Virtual );
                }
            }

            {
                ImGuiX::ScopedFont sf( GetColorForValueType( GraphValueType::ID ) );
                if ( ImGui::MenuItem( "ID" ) )
                {
                    StartParameterCreate( GraphValueType::ID, ParameterType::Virtual );
                }
            }

            {
                ImGuiX::ScopedFont sf( GetColorForValueType( GraphValueType::Float ) );
                if ( ImGui::MenuItem( "Float" ) )
                {
                    StartParameterCreate( GraphValueType::Float, ParameterType::Virtual );
                }
            }

            {
                ImGuiX::ScopedFont sf( GetColorForValueType( GraphValueType::Vector ) );
                if ( ImGui::MenuItem( "Vector" ) )
                {
                    StartParameterCreate( GraphValueType::Vector, ParameterType::Virtual );
                }
            }

            {
                ImGuiX::ScopedFont sf( GetColorForValueType( GraphValueType::Target ) );
                if ( ImGui::MenuItem( "Target" ) )
                {
                    StartParameterCreate( GraphValueType::Target, ParameterType::Virtual );
                }
            }

            {
                ImGuiX::ScopedFont sf( GetColorForValueType( GraphValueType::BoneMask ) );
                if ( ImGui::MenuItem( "Bone Mask" ) )
                {
                    StartParameterCreate( GraphValueType::BoneMask, ParameterType::Virtual );
                }
            }
        };

        ImGuiX::DropDownIconButtonColored( EE_ICON_PLUS_CIRCLE, "Virtual", DrawNewVirtualParameterOptions, Colors::DarkOrange, Colors::White, Colors::White, ImVec2( virtualParameterButtonWidth, 0 ) );

        ImGui::SameLine();

        ImGui::BeginDisabled( m_dialogManager.HasActiveModalDialog() );
        if ( ImGui::Button( EE_ICON_TRASH_CAN"##EraseUnused", ImVec2( eraseButtonWidth, 0 ) ) )
        {
            m_currentOperationParameterID = UUID();
            m_dialogManager.CreateModalDialog( "Rename IDs", [this] ( UpdateContext const& context ) { return DrawDeleteUnusedParameterDialogWindow( context ); }, ImVec2( 500, -1 ) );
        }
        ImGui::EndDisabled();
        ImGuiX::ItemTooltip( "Delete all unused parameters" );

        // Draw Parameters
        //-------------------------------------------------------------------------

        if ( ImGui::BeginChild( "ParameterList", ImVec2( 0, 0 ) ) )
        {
            constexpr static float const usesColumnWidth = 30.0f;

            ImGui::SetNextItemOpen( !m_parameterGroupTree.GetRootCategory().m_isCollapsed );
            if ( ImGui::CollapsingHeader( "General" ) )
            {
                if ( !m_parameterGroupTree.GetRootCategory().m_items.empty() )
                {
                    if ( ImGui::BeginTable( "CPGT", 3, 0 ) )
                    {
                        ImGui::TableSetupColumn( "##Name", ImGuiTableColumnFlags_WidthStretch );
                        ImGui::TableSetupColumn( "##Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40 );
                        ImGui::TableSetupColumn( "##Uses", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, usesColumnWidth );

                        for ( auto const& item : m_parameterGroupTree.GetRootCategory().m_items )
                        {
                            if ( auto pCP = TryCast<ControlParameterToolsNode>( item.m_data ) )
                            {
                                DrawParameterListRow( pCP );
                            }
                            else
                            {
                                DrawParameterListRow( item.m_data );
                            }
                        }

                        ImGui::EndTable();
                    }

                    ControlParameterGroupDragAndDropHandler( m_parameterGroupTree.GetRootCategory() );
                }

                m_parameterGroupTree.GetRootCategory().m_isCollapsed = false;
            }
            else
            {
                m_parameterGroupTree.GetRootCategory().m_isCollapsed = true;
            }

            // Child Groups
            //-------------------------------------------------------------------------

            for ( auto& category : m_parameterGroupTree.GetRootCategory().m_childCategories )
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
                            ImGui::TableSetupColumn( "##Uses", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, usesColumnWidth );

                            //-------------------------------------------------------------------------

                            for ( auto const& item : category.m_items )
                            {
                                if ( auto pCP = TryCast<ControlParameterToolsNode>( item.m_data ) )
                                {
                                    DrawParameterListRow( pCP );
                                }
                                else
                                {
                                    DrawParameterListRow( item.m_data );
                                }
                            }

                            ImGui::EndTable();
                        }

                        ControlParameterGroupDragAndDropHandler( category );
                    }

                    category.m_isCollapsed = false;
                }
                else
                {
                    category.m_isCollapsed = true;
                }
            }
        }
        ImGui::EndChild();
    }

    void AnimationGraphEditor::DrawPreviewParameterList( UpdateContext const& context )
    {
        ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );

        //-------------------------------------------------------------------------

        auto DrawControlParameterEditorRow = [this, &context] ( ControlParameterPreviewState* pPreviewState )
        {
            auto pControlParameter = pPreviewState->GetParameter();

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
            pPreviewState->DrawPreviewEditor( context, m_characterTransform, IsLiveDebugging() );
            ImGui::PopID();
        };

        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 4 ) );

        // Uncategorized
        //-------------------------------------------------------------------------

        ImGui::SetNextItemOpen( !m_previewParameterGroupTree.GetRootCategory().m_isCollapsed );
        if ( ImGui::CollapsingHeader( "General" ) )
        {
            if ( ImGui::BeginTable( "Parameter Preview", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerH ) )
            {
                ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupColumn( "Editor", ImGuiTableColumnFlags_WidthStretch );

                for ( auto const& item : m_previewParameterGroupTree.GetRootCategory().m_items )
                {
                    DrawControlParameterEditorRow( item.m_data );
                }

                ImGui::EndTable();
            }

            m_previewParameterGroupTree.GetRootCategory().m_isCollapsed = false;
        }
        else
        {
            m_previewParameterGroupTree.GetRootCategory().m_isCollapsed = true;
        }

        // Child Categories
        //-------------------------------------------------------------------------

        for ( auto& category : m_previewParameterGroupTree.GetRootCategory().m_childCategories )
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

    void AnimationGraphEditor::CreateParameter( ParameterType parameterType, GraphValueType valueType, String const& desiredParameterName, String const& desiredGroupName )
    {
        String finalParameterName = desiredParameterName;
        EnsureUniqueParameterName( finalParameterName );

        String finalGroupName = desiredGroupName;
        ResolveParameterGroupName( finalGroupName );

        //-------------------------------------------------------------------------

        auto pRootGraph = GetEditedRootGraph();
        NodeGraph::ScopedGraphModification gm( pRootGraph );

        ParameterBaseToolsNode* pParameter = nullptr;

        if ( parameterType == ParameterType::Control )
        {
            pParameter = ControlParameterToolsNode::Create( pRootGraph, valueType, finalParameterName, finalGroupName );
        }
        else
        {
            pParameter = VirtualParameterToolsNode::Create( pRootGraph, valueType, finalParameterName, finalGroupName );
        }

        EE_ASSERT( pParameter != nullptr );
        m_parameters.emplace_back( pParameter );

        RefreshParameterGroupTree();
    }

    void AnimationGraphEditor::RenameParameter( UUID const& parameterID, String const& desiredParameterName, String const& desiredGroupName )
    {
        auto pParameter = FindParameter( parameterID );
        EE_ASSERT( pParameter != nullptr );

        String const originalParameterName = pParameter->GetParameterName();
        String const originalParameterGroup = pParameter->GetParameterGroup();

        // Check if we actually need to do anything
        bool const requiresNameChange = originalParameterName.comparei( desiredParameterName ) != 0;
        if ( !requiresNameChange && originalParameterGroup.comparei( desiredGroupName ) == 0 )
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

        String finalGroupName = desiredGroupName;
        ResolveParameterGroupName( finalGroupName );

        //-------------------------------------------------------------------------

        {
            auto pRootGraph = GetEditedRootGraph();
            NodeGraph::ScopedGraphModification gm( pRootGraph );
            pParameter->Rename( finalParameterName );
            pParameter->SetParameterGroup( finalGroupName );
        }
    }

    void AnimationGraphEditor::DestroyParameter( UUID const& parameterID )
    {
        EE_ASSERT( FindParameter( parameterID ) != nullptr );

        ClearSelection();

        // Navigate away from the virtual parameter graph if we are currently viewing it
        auto pParentNode = m_primaryGraphView.GetViewedGraph()->GetParentNode();
        if ( pParentNode != nullptr && pParentNode->GetID() == parameterID )
        {
            NavigateTo( GetEditedRootGraph() );
        }

        //-------------------------------------------------------------------------

        auto pRootGraph = GetEditedRootGraph();
        NodeGraph::ScopedGraphModification gm( pRootGraph );

        // Find and remove all reference nodes
        auto const parameterReferences = pRootGraph->FindAllNodesOfType<ParameterReferenceToolsNode>( NodeGraph::SearchMode::Recursive, NodeGraph::SearchTypeMatch::Derived );
        for ( auto const& pFoundParameterNode : parameterReferences )
        {
            if ( pFoundParameterNode->GetReferencedParameterID() == parameterID )
            {
                pFoundParameterNode->Destroy();
            }
        }

        // Delete parameter
        for ( auto iter = m_parameters.begin(); iter != m_parameters.end(); ++iter )
        {
            auto pParameter = ( *iter );
            if ( pParameter->GetID() == parameterID )
            {
                pParameter->Destroy();
                m_parameters.erase( iter );
                break;
            }
        }

        //-------------------------------------------------------------------------

        RefreshParameterGroupTree();
    }

    void AnimationGraphEditor::EnsureUniqueParameterName( String& parameterName ) const
    {
        String tempString = parameterName;
        bool isNameUnique = false;
        int32_t suffix = 0;

        while ( !isNameUnique )
        {
            isNameUnique = true;

            for ( auto pParameter : m_parameters )
            {
                if ( pParameter->GetParameterName().comparei( tempString ) == 0 )
                {
                    isNameUnique = false;
                    break;
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

    void AnimationGraphEditor::ResolveParameterGroupName( String& desiredGroupName ) const
    {
        for ( auto const& category : m_parameterGroupTree.GetRootCategory().m_childCategories )
        {
            if ( category.m_name.comparei( desiredGroupName ) == 0 )
            {
                desiredGroupName = category.m_name;
                return;
            }
        }
    }

    ParameterBaseToolsNode* AnimationGraphEditor::FindParameter( UUID const& parameterID ) const
    {
        for ( auto pParameter : m_parameters )
        {
            if ( pParameter->GetID() == parameterID )
            {
                return pParameter;
            }
        }

        return nullptr;
    }

    void AnimationGraphEditor::CreateControlParameterPreviewStates()
    {
        int32_t const numParameters = (int32_t) m_parameters.size();
        for ( int32_t i = 0; i < numParameters; i++ )
        {
            auto pControlParameter = TryCast<ControlParameterToolsNode>( m_parameters[i] );
            if ( pControlParameter == nullptr )
            {
                continue;
            }

            switch ( pControlParameter->GetOutputValueType() )
            {
                case GraphValueType::Bool:
                {
                    m_previewParameterStates.emplace_back( EE::New<BoolParameterState>( this, pControlParameter ) );
                }
                break;

                case GraphValueType::Float:
                {
                    m_previewParameterStates.emplace_back( EE::New<FloatParameterState>( this, pControlParameter ) );
                }
                break;

                case GraphValueType::Vector:
                {
                    m_previewParameterStates.emplace_back( EE::New<VectorParameterState>( this, pControlParameter ) );
                }
                break;

                case GraphValueType::ID:
                {
                    m_previewParameterStates.emplace_back( EE::New<IDParameterState>( this, pControlParameter ) );
                }
                break;

                case GraphValueType::Target:
                {
                    m_previewParameterStates.emplace_back( EE::New<TargetParameterState>( this, pControlParameter ) );
                }
                break;

                default:
                EE_UNREACHABLE_CODE();
                break;
            }
        }

        //-------------------------------------------------------------------------

        EE_ASSERT( m_previewParameterGroupTree.GetRootCategory().IsEmpty() );

        for ( auto pPreviewState : m_previewParameterStates )
        {
            auto pControlParameter = pPreviewState->GetParameter();
            m_previewParameterGroupTree.AddItem( pControlParameter->GetParameterGroup(), pControlParameter->GetName(), pPreviewState );
        }
    }

    void AnimationGraphEditor::DestroyControlParameterPreviewStates()
    {
        m_previewParameterGroupTree.Clear();

        for ( auto& pPreviewState : m_previewParameterStates )
        {
            EE::Delete( pPreviewState );
        }

        m_previewParameterStates.clear();
    }

    void AnimationGraphEditor::StartParameterCreate( GraphValueType valueType, ParameterType parameterType )
    {
        m_currentOperationParameterID = UUID();
        m_currentOperationParameterValueType = valueType;
        m_currentOperationParameterType = parameterType;

        Memory::MemsetZero( m_parameterNameBuffer, 255 );
        Memory::MemsetZero( m_parameterGroupBuffer, 255 );
        strncpy_s( m_parameterNameBuffer, "Parameter", 9 );

        m_dialogManager.CreateModalDialog( "Create Parameter", [this] ( UpdateContext const& context ) { return DrawCreateOrRenameParameterDialogWindow( context, false ); }, ImVec2( 300, -1 ) );
    }

    void AnimationGraphEditor::StartParameterRename( UUID const& parameterID )
    {
        EE_ASSERT( parameterID.IsValid() );
        m_currentOperationParameterID = parameterID;

        if ( auto pParameter = FindParameter( parameterID ) )
        {
            strncpy_s( m_parameterNameBuffer, pParameter->GetName(), 255 );
            strncpy_s( m_parameterGroupBuffer, pParameter->GetParameterGroup().c_str(), 255 );
        }
        else
        {
            EE_UNREACHABLE_CODE();
        }

        m_dialogManager.CreateModalDialog( "Rename Parameter", [this] ( UpdateContext const& context ) { return DrawCreateOrRenameParameterDialogWindow( context, true ); }, ImVec2( 300, -1 ) );
    }

    void AnimationGraphEditor::StartParameterDelete( UUID const& parameterID )
    {
        m_currentOperationParameterID = parameterID;
        m_dialogManager.CreateModalDialog( "Delete Parameter", [this] ( UpdateContext const& context ) { return DrawDeleteParameterDialogWindow( context ); }, ImVec2( 300, -1 ) );
    }

    //-------------------------------------------------------------------------
    // Variation Editor
    //-------------------------------------------------------------------------

    class VariationEditorItem final : public TreeListViewItem
    {
    public:

        VariationEditorItem( TreeListViewItem* pParent, AnimationGraphEditor* pGraphEditor, String const& path )
            : TreeListViewItem( pParent )
            , m_pGraphEditor( pGraphEditor )
            , m_path( path )
            , m_nameID( path )
        {
            EE_ASSERT( m_pGraphEditor != nullptr );
            m_displayColor = ImGuiX::Style::s_colorText;
        }

        VariationEditorItem( TreeListViewItem* pParent, AnimationGraphEditor* pGraphEditor, VariationDataToolsNode* pVariationDataNode )
            : TreeListViewItem( pParent )
            , m_pGraphEditor( pGraphEditor )
            , m_pNode( pVariationDataNode )
            , m_nameID( pVariationDataNode->GetName() )
        {
            EE_ASSERT( m_pGraphEditor != nullptr );
            m_displayColor = pVariationDataNode->GetTitleBarColor();
        }

        virtual uint64_t GetUniqueID() const override { return m_ID.GetValueU64( 0 ); }

        virtual StringID GetNameID() const override { return m_nameID; }

        virtual InlineString GetDisplayName() const
        {
            InlineString displayName;

            if ( m_pNode != nullptr )
            {
                displayName = m_pNode->GetName();
            }
            else
            {
                displayName = m_path.c_str();
            }

            return displayName;
        }

        virtual Color GetDisplayColor( ItemState state ) const override 
        {
            if ( m_pNode != nullptr )
            {
                StringID const currentVariationID = m_pGraphEditor->GetActiveVariation();
                if ( currentVariationID != Variation::s_defaultVariationID )
                {
                    if ( !m_pNode->HasVariationOverride( currentVariationID ) )
                    {
                        return ImGuiX::Style::s_colorTextDisabled;
                    }
                }
            }

            return m_displayColor;
        }

        virtual bool OnDoubleClick() override
        {
            if ( m_pNode != nullptr )
            {
                m_pGraphEditor->NavigateTo( m_pNode );
            }

            return false;
        }

        VariationDataToolsNode* GetNode() const { return m_pNode; }

    private:

        AnimationGraphEditor*               m_pGraphEditor = nullptr;
        String                              m_path;
        VariationDataToolsNode*             m_pNode = nullptr;
        StringID                            m_nameID;
        UUID                                m_ID = UUID::GenerateID();
        Color                               m_displayColor;
    };

    void AnimationGraphEditor::SetupVariationEditorItemExtraColumnHeaders()
    {
        ImGui::TableSetupColumn( "##Override", ImGuiTableColumnFlags_WidthFixed, 30 );
    }

    void AnimationGraphEditor::DrawVariationEditorItemExtraColumns( TreeListViewItem* pRootItem, int32_t columnIdx )
    {
        auto pVarItem = static_cast<VariationEditorItem*>( pRootItem );
        VariationDataToolsNode* pVariationDataNode = pVarItem->GetNode();
        if ( pVariationDataNode == nullptr )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( columnIdx == 0 )
        {
            constexpr static float const overrideButtonSize = 30;
            ImVec2 const buttonSize( overrideButtonSize, overrideButtonSize );

            StringID const currentVariationID = GetEditedGraphData()->m_activeVariationID;
            if ( currentVariationID == Variation::s_defaultVariationID )
            {
                // Do Nothing
            }
            else
            {
                if ( pVariationDataNode->HasVariationOverride( currentVariationID ) )
                {
                    if ( ImGuiX::ButtonColored( EE_ICON_CLOSE, ImGuiX::Style::s_colorGray3, Colors::MediumRed, buttonSize ) )
                    {
                        m_variationRequests.emplace_back( pVariationDataNode, false );
                    }
                    ImGuiX::ItemTooltip( "Remove Override" );
                }
                else // Create an override
                {
                    if ( ImGuiX::ButtonColored( EE_ICON_PLUS, ImGuiX::Style::s_colorGray3, Colors::LimeGreen, buttonSize ) )
                    {
                        m_variationRequests.emplace_back( pVariationDataNode, true );
                    }
                    ImGuiX::ItemTooltip( "Add Override" );
                }
            }
        }
    }

    void AnimationGraphEditor::DrawVariationEditor( UpdateContext const& context, bool isFocused )
    {
        auto pRootGraph = GetEditedRootGraph();
        auto& variationHierarchy = GetEditedGraphData()->m_pGraphDefinition->GetVariationHierarchy();

        //-------------------------------------------------------------------------
        // Variation Selector and Skeleton Picker
        //-------------------------------------------------------------------------

        constexpr static char const* const variationLabel = "Variation: ";
        constexpr static char const* const skeletonLabel = "Skeleton: ";

        ImVec2 const variationLabelSize = ImGui::CalcTextSize( variationLabel );
        ImVec2 const skeletonLabelSize = ImGui::CalcTextSize( skeletonLabel );
        float const offset = Math::Max( skeletonLabelSize.x, variationLabelSize.x ) + ( ImGui::GetStyle().ItemSpacing.x * 2 );

        ImGui::AlignTextToFramePadding();
        ImGui::Text( variationLabel );
        ImGui::SameLine( offset );
        DrawVariationSelector();

        //-------------------------------------------------------------------------

        ImGui::AlignTextToFramePadding();
        ImGui::Text( skeletonLabel );
        ImGui::SameLine( offset );
        auto pVariation = GetEditedGraphData()->m_pGraphDefinition->GetVariation( GetEditedGraphData()->m_activeVariationID );
        if ( m_variationSkeletonPicker.UpdateAndDraw() )
        {
            NodeGraph::ScopedGraphModification sgm( pRootGraph );
            pVariation->m_skeleton = m_variationSkeletonPicker.GetResourceID();
        }

        //-------------------------------------------------------------------------
        // Variation Data Nodes
        //-------------------------------------------------------------------------

        StringID const currentVariationID = GetEditedGraphData()->m_activeVariationID;

        ImGui::BeginDisabled( IsInReadOnlyState() );
        {
            auto variationDataNodes = pRootGraph->FindAllNodesOfType<VariationDataToolsNode>( NodeGraph::SearchMode::Recursive, NodeGraph::SearchTypeMatch::Derived );
            bool isDefaultVariationSelected = IsDefaultVariationSelected();

            // Filter
            //-------------------------------------------------------------------------

            // Name/Path filter
            constexpr float const buttonWidth = 30.0f;
            float const textFilterWidth = ImGui::GetContentRegionAvail().x - buttonWidth - ImGui::GetStyle().ItemSpacing.x;
            bool filterUpdated = m_variationEditorFilter.UpdateAndDraw( textFilterWidth );

            ImGui::SameLine();

            // Type filter
            auto DrawFiltersMenu = [this, &filterUpdated] ()
            {
                InlineString label;
                int32_t const numTypeFilters = (int32_t) m_variationDataNodeTypes.size();
                for ( int32_t i = 0; i < numTypeFilters; i++ )
                {
                    bool isChecked = VectorContains( m_selectedVariationDataNodeTypeFilters, m_variationDataNodeTypes[i] );
                    label.sprintf( "%s##VDNT%d", m_variationDataNodeTypes[i]->m_friendlyName.c_str(), i );

                    if ( ImGui::Checkbox( label.c_str(), &isChecked ) )
                    {
                        if ( isChecked )
                        {
                            m_selectedVariationDataNodeTypeFilters.emplace_back( m_variationDataNodeTypes[i] );
                        }
                        else
                        {
                            m_selectedVariationDataNodeTypeFilters.erase_first_unsorted( m_variationDataNodeTypes[i] );
                        }

                        filterUpdated = true;
                    }
                }
            };

            ImGuiX::DropDownIconButton( EE_ICON_FILTER, "##Resource Filters", DrawFiltersMenu, ImGuiX::Style::s_colorText, ImVec2( buttonWidth, 0 ) );

            // Apply filter
            if ( filterUpdated )
            {
                auto VisibilityTest = [this] ( TreeListViewItem const* pItem )
                {
                    auto pVarItem = static_cast<VariationEditorItem const*>( pItem );
                    if ( pVarItem->GetNode() != nullptr && !m_selectedVariationDataNodeTypeFilters.empty() )
                    {
                        if ( !VectorContains( m_selectedVariationDataNodeTypeFilters, pVarItem->GetNode()->GetTypeInfo() ) )
                        {
                            return false;
                        }
                    }

                    return m_variationEditorFilter.MatchesFilter( pItem->GetDisplayName() );
                };

                m_variationTreeView.ExpandAll();
                m_variationTreeView.UpdateItemVisibility( VisibilityTest );
            }

            // Main Layout
            //-------------------------------------------------------------------------

            if ( ImGui::BeginTable( "VarEditorLayout", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY ) )
            {
                ImGui::TableSetupColumn( "##VD", ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupColumn( "##PG", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize );

                ImGui::TableNextRow();

                // Nodes
                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();

                if ( m_variationTreeView.UpdateAndDraw( m_variationTreeContext ) )
                {
                    m_selectedVariationDataNode.Clear();

                    TVector<TreeListViewItem*> const& selection = m_variationTreeView.GetSelection();
                    EE_ASSERT( selection.size() <= 1 );
                    if ( selection.size() == 1 )
                    {
                        auto pVarItem = static_cast<VariationEditorItem const*>( selection[0] );
                        if ( pVarItem->GetNode() )
                        {
                            m_selectedVariationDataNode = pVarItem->GetNode()->GetID();
                        }
                    }
                }

                // Property Grid
                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();

                VariationDataToolsNode* pSelectedDataNode =  ( m_selectedVariationDataNode.IsValid() ) ? Cast<VariationDataToolsNode>( pRootGraph->FindNode( m_selectedVariationDataNode, true ) ) : nullptr;
                if ( pSelectedDataNode != nullptr )
                {
                    bool const isReadOnly = currentVariationID != Variation::s_defaultVariationID && !pSelectedDataNode->HasVariationOverride( currentVariationID );
                    m_variationDataPropertyGrid.SetReadOnly( isReadOnly );

                    auto pVariationData = pSelectedDataNode->GetResolvedVariationData( variationHierarchy, currentVariationID );
                    EE_ASSERT( pVariationData != nullptr );
                    if ( m_variationDataPropertyGrid.GetEditedType() != pVariationData )
                    {
                        m_variationDataPropertyGrid.SetTypeToEdit( pVariationData );
                    }
                }
                else
                {
                    m_variationDataPropertyGrid.SetTypeToEdit( nullptr );
                }

                m_variationDataPropertyGrid.UpdateAndDraw();

                ImGui::EndTable();
            }
        }
        ImGui::EndDisabled();
    }

    void AnimationGraphEditor::StartCreateVariation( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() );
        m_activeOperationVariationID = variationID;
        strncpy_s( m_nameBuffer, "NewChildVariation", 255 );
        m_dialogManager.CreateModalDialog( "Create Variation", [this] ( UpdateContext const& context ) { return DrawCreateVariationDialogWindow( context ); }, ImVec2( 300, -1 ) );
    }

    void AnimationGraphEditor::StartRenameVariation( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() );
        m_activeOperationVariationID = variationID;
        strncpy_s( m_nameBuffer, variationID.c_str(), 255 );
        m_dialogManager.CreateModalDialog( "Rename Variation", [this] ( UpdateContext const& context ) { return DrawRenameVariationDialogWindow( context ); }, ImVec2( 300, -1 ) );
    }

    void AnimationGraphEditor::StartDeleteVariation( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() );
        m_activeOperationVariationID = variationID;
        m_dialogManager.CreateModalDialog( "Delete Variation", [this] ( UpdateContext const& context ) { return DrawDeleteVariationDialogWindow( context ); }, ImVec2( 300, -1 ) );
    }

    void AnimationGraphEditor::CreateVariation( StringID newVariationID, StringID parentVariationID )
    {
        auto pRootGraph = GetEditedRootGraph();
        auto& variationHierarchy = GetEditedGraphData()->m_pGraphDefinition->GetVariationHierarchy();

        EE_ASSERT( !variationHierarchy.IsValidVariation( newVariationID ) );

        // Create new variation
        NodeGraph::ScopedGraphModification gm( pRootGraph );
        variationHierarchy.CreateVariation( newVariationID, parentVariationID );

        // Switch to the new variation
        SetActiveVariation( newVariationID );
    }

    void AnimationGraphEditor::RenameVariation( StringID oldVariationID, StringID newVariationID )
    {
        auto pRootGraph = GetEditedRootGraph();
        auto& variationHierarchy = GetEditedGraphData()->m_pGraphDefinition->GetVariationHierarchy();
        bool const isRenamingCurrentlySelectedVariation = ( m_activeOperationVariationID == GetEditedGraphData()->m_activeVariationID );

        EE_ASSERT( !variationHierarchy.IsValidVariation( newVariationID ) );

        // Perform rename
        NodeGraph::ScopedGraphModification gm( pRootGraph );
        variationHierarchy.RenameVariation( m_activeOperationVariationID, newVariationID );

        // Update all variation data nodes
        auto variationDataNodes = pRootGraph->FindAllNodesOfType<VariationDataToolsNode>( NodeGraph::SearchMode::Recursive, NodeGraph::SearchTypeMatch::Derived );
        for ( auto pVariationDataNode : variationDataNodes )
        {
            pVariationDataNode->RenameVariationOverride( m_activeOperationVariationID, newVariationID );
        }

        // Set the active variation to the renamed variation
        if ( isRenamingCurrentlySelectedVariation )
        {
            SetActiveVariation( newVariationID );
        }
    }

    void AnimationGraphEditor::DeleteVariation( StringID variationID )
    {
        auto pRootGraph = GetEditedRootGraph();
        NodeGraph::ScopedGraphModification gm( pRootGraph );

        auto& variationHierarchy = GetEditedGraphData()->m_pGraphDefinition->GetVariationHierarchy();
        variationHierarchy.DestroyVariation( m_activeOperationVariationID );
    }

    void AnimationGraphEditor::DrawVariationTreeNode( VariationHierarchy const& variationHierarchy, StringID variationID )
    {
        ImGui::PushID( variationID.c_str() );

        auto const childVariations = variationHierarchy.GetChildVariations( variationID );
        bool const isSelected = GetEditedGraphData()->m_activeVariationID == variationID;

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
            ImGuiX::ScopedFont const sf( isSelected ? ImGuiX::Font::MediumBold : ImGuiX::Font::Medium, isSelected ? Colors::LimeGreen.m_color : ImGui::ColorConvertFloat4ToU32( ImGui::GetStyle().Colors[ImGuiCol_Text] ) );
            isTreeNodeOpen = ImGui::TreeNodeEx( variationID.c_str(), flags );
        }
        ImGuiX::TextTooltip( "Right click for options" );
        if ( ImGui::IsItemClicked() )
        {
            SetActiveVariation( variationID );
        }

        // Context Menu
        //-------------------------------------------------------------------------

        if ( ImGui::BeginPopupContextItem( variationID.c_str() ) )
        {
            if ( ImGui::MenuItem( "Set Active" ) )
            {
                SetActiveVariation( variationID );
            }

            //-------------------------------------------------------------------------

            if ( ImGui::MenuItem( "Create Child Variation" ) )
            {
                StartCreateVariation( variationID );
            }

            if ( variationID != Variation::s_defaultVariationID )
            {
                ImGui::Separator();

                if ( ImGui::MenuItem( "Rename" ) )
                {
                    StartRenameVariation( variationID );
                }

                if ( ImGui::MenuItem( "Delete" ) )
                {
                    StartDeleteVariation( variationID );
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

    void AnimationGraphEditor::DrawVariationSelector( float width )
    {
        ImGui::BeginDisabled( IsInReadOnlyState() );
        ImGui::SetNextItemWidth( width );
        if ( ImGui::BeginCombo( "##VariationSelector", GetEditedGraphData()->m_activeVariationID.c_str() ) )
        {
            auto& variationHierarchy = GetEditedGraphData()->m_pGraphDefinition->GetVariationHierarchy();
            DrawVariationTreeNode( variationHierarchy, Variation::s_defaultVariationID );
            ImGui::EndCombo();
        }
        ImGuiX::ItemTooltip( "Variation Selector - Right click on variations for more options!" );
        ImGui::EndDisabled();
    }

    bool AnimationGraphEditor::DrawVariationNameEditor( bool& isDialogOpen, bool isRenameOp )
    {
        bool nameChangeConfirmed = false;

        // Validate current buffer
        //-------------------------------------------------------------------------

        auto ValidateVariationName = [this] ()
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
            auto const& variationHierarchy = GetEditedGraphData()->m_pGraphDefinition->GetVariationHierarchy();
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
        ImVec4 const textColor = isValidVariationName ? ImGui::GetStyle().Colors[ImGuiCol_Text] : ImVec4( Colors::Red.ToFloat4() );
        ImGui::PushStyleColor( ImGuiCol_Text, textColor );
        ImGui::SetNextItemWidth( -1 );
        if ( ImGui::InputText( "##VariationName", m_nameBuffer, 255, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CallbackCharFilter, ImGuiX::FilterNameIDChars ) )
        {
            nameChangeConfirmed = true;
        }
        ImGui::PopStyleColor();
        ImGui::NewLine();

        float const dialogWidth = ImGui::GetContentRegionAvail().x;

        // Buttons
        //-------------------------------------------------------------------------

        constexpr static float const buttonWidth = 65.0f;

        ImGui::SameLine( 0, dialogWidth - ( buttonWidth * 2 ) - ImGui::GetStyle().ItemSpacing.x );

        ImGui::BeginDisabled( !isValidVariationName );
        if ( ImGui::Button( "Ok", ImVec2( buttonWidth, 0 ) ) )
        {
            nameChangeConfirmed = true;
        }
        ImGui::EndDisabled();

        ImGui::SameLine( 0, 4 );

        if ( ImGui::Button( "Cancel", ImVec2( buttonWidth, 0 ) ) )
        {
            nameChangeConfirmed = false;
            isDialogOpen = false;
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

    bool AnimationGraphEditor::DrawCreateVariationDialogWindow( UpdateContext const& context )
    {
        bool isDialogOpen = true;
        if ( DrawVariationNameEditor( isDialogOpen, false ) )
        {
            StringID newVariationID( m_nameBuffer );
            CreateVariation( newVariationID, m_activeOperationVariationID );
            m_activeOperationVariationID = StringID();
            isDialogOpen = false;
        }

        return isDialogOpen;
    }

    bool AnimationGraphEditor::DrawRenameVariationDialogWindow( UpdateContext const& context )
    {
        bool isDialogOpen = true;
        if ( DrawVariationNameEditor( isDialogOpen, true ) )
        {
            StringID newVariationID( m_nameBuffer );
            RenameVariation( m_activeOperationVariationID, newVariationID );
            m_activeOperationVariationID = StringID();
        }

        return isDialogOpen;
    }

    bool AnimationGraphEditor::DrawDeleteVariationDialogWindow( UpdateContext const& context )
    {
        bool isDialogOpen = true;

        ImGui::Text( "Are you sure you want to delete this variation?" );
        ImGui::NewLine();

        float const dialogWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SameLine( 0, dialogWidth - 64 );

        if ( ImGui::Button( "Yes", ImVec2( 30, 0 ) ) )
        {
            EE_ASSERT( m_activeOperationVariationID != Variation::s_defaultVariationID );

            // Update selection
            auto& variationHierarchy = GetEditedGraphData()->m_pGraphDefinition->GetVariationHierarchy();
            auto const pVariation = variationHierarchy.GetVariation( m_activeOperationVariationID );
            SetActiveVariation( pVariation->m_parentID );

            // Destroy variation
            DeleteVariation( m_activeOperationVariationID );
            m_activeOperationVariationID = StringID();
            isDialogOpen = false;
        }

        ImGui::SameLine( 0, 4 );

        if ( ImGui::Button( "No", ImVec2( 30, 0 ) ) )
        {
            m_activeOperationVariationID = StringID();
            isDialogOpen = false;
        }

        return isDialogOpen;
    }

    void AnimationGraphEditor::RefreshVariationEditor()
    {
        auto pVariation = GetEditedGraphData()->m_pGraphDefinition->GetVariation( GetEditedGraphData()->m_activeVariationID );
        m_variationSkeletonPicker.SetResourceID( pVariation->m_skeleton.GetResourceID() );
        m_variationTreeView.RebuildTree( m_variationTreeContext );
        m_variationTreeView.ExpandAll();
    }

    static void AddCategoryToTree( AnimationGraphEditor* pGraphEditor, TreeListViewItem* pParentItem, Category<VariationDataToolsNode*> const& category, bool isRootCategory = false )
    {
        TreeListViewItem* pCategoryItem = isRootCategory ? pParentItem : pParentItem->CreateChild<VariationEditorItem>( pGraphEditor, category.m_name );

        for ( auto const& childCategory : category.m_childCategories )
        {
            AddCategoryToTree( pGraphEditor, pCategoryItem, childCategory );
        }

        for ( auto const &item : category.m_items )
        {
            pCategoryItem->CreateChild<VariationEditorItem>( pGraphEditor, item.m_data );
        }
    }

    void AnimationGraphEditor::RebuildVariationDataTree( TreeListViewItem* pRootItem )
    {
        auto pRootGraph = GetEditedRootGraph();
        auto variationDataNodes = pRootGraph->FindAllNodesOfType<VariationDataToolsNode>( NodeGraph::SearchMode::Recursive, NodeGraph::SearchTypeMatch::Derived );

        CategoryTree<VariationDataToolsNode*> variationDataTree;
        for ( auto pVariationDataNode : variationDataNodes )
        {
            String path = pVariationDataNode->GetParentGraph()->GetStringPathFromRoot();
            variationDataTree.AddItem( path, pVariationDataNode->GetName(), pVariationDataNode );
        }

        variationDataTree.CollapseAllIntermediateCategories();

        AddCategoryToTree( this, pRootItem, variationDataTree.GetRootCategory(), true );
    }

    //-------------------------------------------------------------------------
    // Recording
    //-------------------------------------------------------------------------

    void AnimationGraphEditor::DrawRecorderUI( UpdateContext const& context )
    {
        ImGui::SetNextItemOpen( true, ImGuiCond_FirstUseEver );
        if ( ImGui::CollapsingHeader( "Recorder" ) )
        {
            ImVec2 const buttonSize( 30, 0 );

            // Review / Stop
            //-------------------------------------------------------------------------

            // Stop the review for every editor
            if ( IsReviewingRecording() )
            {
                if ( ImGuiX::IconButton( EE_ICON_STOP, "##StopReview", Colors::Red, buttonSize ) )
                {
                    StopDebugging();
                }
                ImGuiX::ItemTooltip( "Stop Review" );
            }
            else // Show the start/join button
            {
                ImGui::BeginDisabled( !m_graphRecorder.HasRecordedData() );
                if ( ImGuiX::IconButton( EE_ICON_PLAY, "##StartReview", Colors::Lime, buttonSize ) )
                {
                    if ( IsDebugging() )
                    {
                        StopDebugging();
                    }

                    DebugTarget debugTarget;
                    debugTarget.m_type = DebugTargetType::Recording;
                    RequestDebugSession( context, debugTarget );
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
                if ( ImGuiX::IconButton( EE_ICON_STOP, "##StopRecord", Colors::White, buttonSize ) )
                {
                    StopRecording();
                }
                ImGuiX::ItemTooltip( "Stop Recording" );
            }
            else // Show start recording button
            {
                if ( ImGuiX::IconButton( EE_ICON_RECORD, "##Record", Colors::Red, buttonSize ) )
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
            int32_t const numFramesRecorded = IsReviewingRecording() ? m_graphRecorder.GetNumRecordedFrames() : 1;
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
                    ImGui::Text( "Delta Time: %.2fms", m_graphRecorder.m_recordedData[m_currentReviewFrameIdx].m_deltaTime.ToMilliseconds().ToFloat() );

                    Transform const& frameWorldTransform = m_graphRecorder.m_recordedData[m_currentReviewFrameIdx].m_characterWorldTransform;
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

    void AnimationGraphEditor::StartRecording()
    {
        EE_ASSERT( IsLiveDebugging() && !m_isRecording );
        EE_ASSERT( m_pDebugGraphInstance != nullptr );

        ClearRecordedData();

        m_pDebugGraphInstance->StartRecording( &m_graphRecorder );

        m_isRecording = true;
    }

    void AnimationGraphEditor::StopRecording()
    {
        EE_ASSERT( IsLiveDebugging() && m_isRecording );
        EE_ASSERT( m_pDebugGraphInstance != nullptr );

        m_pDebugGraphInstance->StopRecording();

        m_isRecording = false;
    }

    void AnimationGraphEditor::ClearRecordedData()
    {
        EE_ASSERT( !m_isRecording && !IsReviewingRecording() );
        m_graphRecorder.Reset();
    }

    void AnimationGraphEditor::SetFrameToReview( int32_t newFrameIdx )
    {
        EE_ASSERT( IsReviewingRecording() && m_reviewStarted );
        EE_ASSERT( m_graphRecorder.IsValidRecordedFrameIndex( newFrameIdx ) );
        EE_ASSERT( m_pDebugGraphComponent != nullptr && m_pDebugGraphComponent->IsInitialized() );
        EE_ASSERT( m_pDebugMeshComponent != nullptr );

        if ( m_currentReviewFrameIdx == newFrameIdx )
        {
            return;
        }

        //-------------------------------------------------------------------------

        if ( newFrameIdx == ( m_currentReviewFrameIdx + 1 ) )
        {
            auto const& frameData = m_graphRecorder.m_recordedData[newFrameIdx];

            // Set parameters
            m_pDebugGraphInstance->SetRecordedFrameUpdateData( frameData );

            // Evaluate graph
            m_pDebugGraphComponent->EvaluateGraph( frameData.m_deltaTime, frameData.m_characterWorldTransform, nullptr );
        }
        else // Re-evaluate the entire graph to the new index point
        {
            // Set initial state
            m_graphRecorder.m_initialState.PrepareForReading();
            m_pDebugGraphInstance->SetToRecordedState( m_graphRecorder.m_initialState );

            // Update graph instance till we get to the specified frame
            for ( auto i = 0; i <= newFrameIdx; i++ )
            {
                auto const& frameData = m_graphRecorder.m_recordedData[i];

                // Set parameters
                m_pDebugGraphInstance->SetRecordedFrameUpdateData( frameData );

                // Evaluate graph
                m_pDebugGraphComponent->EvaluateGraph( frameData.m_deltaTime, frameData.m_characterWorldTransform, nullptr );

                // Explicitly end root motion debug update for intermediate steps
                if ( i < ( newFrameIdx - 1 ) )
                {
                    auto const& nextFrameData = m_graphRecorder.m_recordedData[i + 1];
                    m_pDebugGraphInstance->EndRootMotionDebuggerUpdate( nextFrameData.m_characterWorldTransform );
                }
            }
        }

        //-------------------------------------------------------------------------

        // Use the transform from the next frame as the end transform of the character used to evaluate the pose tasks
        int32_t const nextFrameIdx = ( newFrameIdx < m_graphRecorder.GetNumRecordedFrames() - 1 ) ? newFrameIdx + 1 : newFrameIdx;
        auto const& nextRecordedFrameData = m_graphRecorder.m_recordedData[nextFrameIdx];

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

    void AnimationGraphEditor::StepReviewForward()
    {
        EE_ASSERT( IsReviewingRecording() );
        int32_t const newFrameIdx = Math::Min( m_graphRecorder.GetNumRecordedFrames() - 1, m_currentReviewFrameIdx + 1 );
        SetFrameToReview( newFrameIdx );
    }

    void AnimationGraphEditor::StepReviewBackward()
    {
        EE_ASSERT( IsReviewingRecording() );
        int32_t const newFrameIdx = Math::Max( 0, m_currentReviewFrameIdx - 1 );
        SetFrameToReview( newFrameIdx );
    }

    //-------------------------------------------------------------------------
    // Commands
    //-------------------------------------------------------------------------

    void AnimationGraphEditor::ProcessCustomCommandRequest( NodeGraph::CustomCommand const* pCommand )
    {
        if ( auto pOpenReferencedGraphCommand = TryCast<OpenReferencedGraphCommand>( pCommand ) )
        {
            EE_ASSERT( pOpenReferencedGraphCommand->m_pCommandSourceNode != nullptr );
            auto pReferencedGraphNode = Cast<ReferencedGraphToolsNode>( pOpenReferencedGraphCommand->m_pCommandSourceNode );

            ResourceID const referencedGraphResourceID = pReferencedGraphNode->GetReferencedGraphResourceID( *m_userContext.m_pVariationHierarchy, m_userContext.m_selectedVariationID );
            EE_ASSERT( referencedGraphResourceID.IsValid() && referencedGraphResourceID.GetResourceTypeID() == GraphDefinition::GetStaticResourceTypeID() );

            if ( pOpenReferencedGraphCommand->m_option == OpenReferencedGraphCommand::OpenInPlace )
            {
                PushOnGraphStack( pReferencedGraphNode, referencedGraphResourceID );
            }
            else
            {
                m_userContext.RequestOpenResource( referencedGraphResourceID );
            }
        }

        //-------------------------------------------------------------------------

        if ( auto pReflectParametersCommand = TryCast<ReflectParametersCommand>( pCommand ) )
        {
            EE_ASSERT( pReflectParametersCommand->m_pCommandSourceNode != nullptr );
            auto pReferencedGraphNode = Cast<ReferencedGraphToolsNode>( pReflectParametersCommand->m_pCommandSourceNode );

            ResourceID const referencedGraphResourceID = pReferencedGraphNode->GetReferencedGraphResourceID( *m_userContext.m_pVariationHierarchy, m_userContext.m_selectedVariationID );
            EE_ASSERT( referencedGraphResourceID.IsValid() && referencedGraphResourceID.GetResourceTypeID() == GraphDefinition::GetStaticResourceTypeID() );

            //-------------------------------------------------------------------------

            ResourceID const referencedGraphDefinitionResourceID = Variation::GetGraphResourceID( referencedGraphResourceID );
            FileSystem::Path const referencedGraphFilePath = referencedGraphDefinitionResourceID.GetFileSystemPath( m_pToolsContext->m_pFileRegistry->GetSourceDataDirectoryPath() );
            if ( !referencedGraphFilePath.IsValid() )
            {
                ShowNotifyDialog( EE_ICON_EXCLAMATION" Error", "Reflect Parameters Failed: Invalid !" );
                return;
            }

            // Load referenced graph
            GraphResourceDescriptor referencedGraphDescriptor;
            if ( !GraphResourceDescriptor::TryReadFromFile( *m_pToolsContext->m_pTypeRegistry, referencedGraphFilePath, referencedGraphDescriptor ) )
            {
                ShowNotifyDialog( EE_ICON_EXCLAMATION" Error", "Reflect Parameters Failed: Invalid Referenced Graph ID!" );
            }

            //-------------------------------------------------------------------------

            TVector<String> resultLog;

            // Push all parameters to referenced graph
            if ( pReflectParametersCommand->m_option == ReflectParametersCommand::FromParent )
            {
                referencedGraphDescriptor.m_graphDefinition.ReflectParameters( *GetEditedGraphData()->m_pGraphDefinition, true, &resultLog );

                // Save referenced graph
                if ( !GraphResourceDescriptor::TryWriteToFile( *m_pToolsContext->m_pTypeRegistry, referencedGraphFilePath, &referencedGraphDescriptor ) )
                {
                    ShowNotifyDialog( EE_ICON_EXCLAMATION" Error", "Reflect Parameters Failed: Invalid Referenced Graph ID!" );
                    return;
                }

                // Open referenced graph
                m_userContext.RequestOpenResource( referencedGraphDefinitionResourceID );
            }
            else // Transfer from the referenced graph any required parameters
            {
                NodeGraph::ScopedGraphModification gm( GetEditedRootGraph() );
                GetEditedGraphData()->m_pGraphDefinition->ReflectParameters( referencedGraphDescriptor.m_graphDefinition, false, &resultLog );

                OnGraphStateModified();
            }

            //-------------------------------------------------------------------------

            String message;
            for ( auto const& logEntry : resultLog )
            {
                message.append_sprintf( "%s\n", logEntry.c_str() );
            }

            ShowNotifyDialog( EE_ICON_CHECK" Completed", message.c_str() );
        }
    }

    void AnimationGraphEditor::StartRenameIDs()
    {
        memset( m_oldIDBuffer, 0, 255 );
        memset( m_newIDBuffer, 0, 255 );
        m_dialogManager.CreateModalDialog( "Rename IDs", [this] ( UpdateContext const& context ) { return DrawRenameIDsDialog( context ); }, ImVec2( 400, 0 ), true );
    }

    bool AnimationGraphEditor::DrawRenameIDsDialog( UpdateContext const& context )
    {
        bool isDialogOpen = true;

        constexpr float const comboWidth = 30;
        float const itemSpacingX = ImGui::GetStyle().ItemSpacing.x;
        float const inputWidth = ImGui::GetContentRegionAvail().x - comboWidth - itemSpacingX;

        // Old Value
        //-------------------------------------------------------------------------

        ImGui::AlignTextToFramePadding();
        ImGui::Text( "Old ID: " );
        ImGui::SameLine();

        ImGui::SetNextItemWidth( inputWidth - itemSpacingX - ImGui::CalcTextSize("Old ID: " ).x );
        ImGui::InputText( "##oldid", m_oldIDBuffer, 255 );

        ImGui::SameLine();

        if ( m_oldIDWidget.DrawAndUpdate( comboWidth ) )
        {
            auto pSelectedOption = m_oldIDWidget.GetSelectedOption();
            if ( pSelectedOption != nullptr && pSelectedOption->m_value.IsValid() )
            {
                strncpy_s( m_oldIDBuffer, 255, pSelectedOption->m_value.c_str(), strlen( pSelectedOption->m_value.c_str() ) );
            }
            else // Clear value
            {
                memset( m_oldIDBuffer, 0, 255 );
            }
        }

        // New Value
        //-------------------------------------------------------------------------

        ImGui::AlignTextToFramePadding();
        ImGui::Text( "New ID: " );
        ImGui::SameLine();

        ImGui::SetNextItemWidth( inputWidth - itemSpacingX - ImGui::CalcTextSize( "New ID: " ).x );
        ImGui::InputText( "##newid", m_newIDBuffer, 255 );

        ImGui::SameLine();

        if ( m_newIDWidget.DrawAndUpdate( comboWidth ) )
        {
            auto pSelectedOption = m_newIDWidget.GetSelectedOption();
            if ( pSelectedOption != nullptr && pSelectedOption->m_value.IsValid() )
            {
                strncpy_s( m_newIDBuffer, 255, pSelectedOption->m_value.c_str(), strlen( pSelectedOption->m_value.c_str() ) );
            }
            else // Clear value
            {
                memset( m_newIDBuffer, 0, 255 );
            }
        }

        // Get old and new IDs
        //-------------------------------------------------------------------------

        StringUtils::StripTrailingWhitespace( m_oldIDBuffer );
        StringID oldID;
        if ( strlen( m_oldIDBuffer ) > 0 )
        {
            oldID = StringID( m_oldIDBuffer );
        }

        StringUtils::StripTrailingWhitespace( m_newIDBuffer );
        StringID newID;
        if ( strlen( m_newIDBuffer ) > 0 )
        {
            newID = StringID( m_newIDBuffer );
        }

        //-------------------------------------------------------------------------

        float const buttonWidth = ( ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x ) / 2;

        ImGui::BeginDisabled( !oldID.IsValid() || !newID.IsValid() );
        if ( ImGui::Button( "Rename", ImVec2( buttonWidth, 0 ) ) )
        {
            NodeGraph::ScopedGraphModification gm( GetEditedRootGraph() );

            // Get all IDs in the graph
            auto pRootGraph = GetEditedRootGraph();
            auto const foundNodes = pRootGraph->FindAllNodesOfType<NodeGraph::BaseNode>( NodeGraph::SearchMode::Recursive, NodeGraph::SearchTypeMatch::Derived );
            for ( auto pNode : foundNodes )
            {
                if ( auto pStateNode = TryCast<StateToolsNode>( pNode ) )
                {
                    pStateNode->RenameLogicAndEventIDs( oldID, newID );
                }
                else if ( auto pFlowNode = TryCast<FlowToolsNode>( pNode ) )
                {
                    pFlowNode->RenameLogicAndEventIDs( oldID, newID );
                }
            }
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        if ( ImGui::Button( "Cancel", ImVec2( buttonWidth, 0 ) ) )
        {
            isDialogOpen = false;
        }

        return isDialogOpen;
    }

    void AnimationGraphEditor::CopyAllParameterNamesToClipboard()
    {
        TVector<String> foundParameterNames;

        auto pRootGraph = GetEditedRootGraph();
        auto const controlParameters = pRootGraph->FindAllNodesOfType<ControlParameterToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Derived );
        for ( auto pControlParameter : controlParameters )
        {
            foundParameterNames.emplace_back( pControlParameter->GetParameterName() );
        }

        // Sort
        auto Comparator = [] ( String const& a, String const& b ) { return strcmp( a.c_str(), b.c_str() ) < 0; };
        eastl::sort( foundParameterNames.begin(), foundParameterNames.end(), Comparator );

        String foundIDString;
        for ( auto ID : foundParameterNames )
        {
            foundIDString.append_sprintf( "%s\r\n", ID.c_str() );
        }

        ImGui::SetClipboardText( foundIDString.c_str() );
    }

    void AnimationGraphEditor::CopyAllGraphIDsToClipboard()
    {
        TVector<StringID> foundIDs;

        // Get all IDs in the graph
        auto pRootGraph = GetEditedRootGraph();
        auto const foundNodes = pRootGraph->FindAllNodesOfType<NodeGraph::BaseNode>( NodeGraph::SearchMode::Recursive, NodeGraph::SearchTypeMatch::Derived );
        for ( auto pNode : foundNodes )
        {
            if ( auto pStateNode = TryCast<StateToolsNode>( pNode ) )
            {
                pStateNode->GetLogicAndEventIDs( foundIDs );
            }
            else if ( auto pFlowNode = TryCast<FlowToolsNode>( pNode ) )
            {
                pFlowNode->GetLogicAndEventIDs( foundIDs );
            }
        }

        // De-duplicate and remove invalid IDs
        TVector<StringID> sortedIDlist;
        sortedIDlist.clear();

        for ( auto ID : foundIDs )
        {
            if ( !ID.IsValid() )
            {
                continue;
            }

            if ( VectorContains( sortedIDlist, ID ) )
            {
                continue;
            }

            sortedIDlist.emplace_back( ID );
        }

        // Sort
        auto Comparator = [] ( StringID const& a, StringID const& b ) { return strcmp( a.c_str(), b.c_str() ) < 0; };
        eastl::sort( sortedIDlist.begin(), sortedIDlist.end(), Comparator );

        String foundIDString;
        for ( auto ID : sortedIDlist )
        {
            foundIDString.append_sprintf( "%s\r\n", ID.c_str() );
        }

        ImGui::SetClipboardText( foundIDString.c_str() );
    }

    //-------------------------------------------------------------------------
    // Undo/Redo
    //-------------------------------------------------------------------------

    void AnimationGraphEditor::OnBeginGraphModification( NodeGraph::BaseGraph* pRootGraph )
    {
        if ( GetEditedRootGraph() == nullptr )
        {
            return;
        }

        if ( pRootGraph == GetEditedRootGraph() )
        {
            EE_ASSERT( m_pActiveUndoableAction == nullptr );

            auto pGraphUndoableAction = EE::New<DataFileUndoableAction>( m_pToolsContext->m_pTypeRegistry, this );
            pGraphUndoableAction->SerializeBeforeState();
            m_pActiveUndoableAction = pGraphUndoableAction;
        }
    }

    void AnimationGraphEditor::OnEndGraphModification( NodeGraph::BaseGraph* pRootGraph )
    {
        if ( GetEditedRootGraph() == nullptr )
        {
            return;
        }

        if ( pRootGraph == GetEditedRootGraph() )
        {
            EE_ASSERT( m_pActiveUndoableAction != nullptr );

            Cast<DataFileUndoableAction>( m_pActiveUndoableAction )->SerializeAfterState();
            m_undoStack.RegisterAction( m_pActiveUndoableAction );
            m_pActiveUndoableAction = nullptr;
            MarkDirty();

            OnGraphStateModified();
        }
    }

    void AnimationGraphEditor::RecordViewAndSelectionState()
    {
        m_recordedViewAndSelectionState.m_viewedGraphPath = m_primaryGraphView.GetViewedGraph()->GetIDPathFromRoot();

        if ( !m_selectedNodes.empty() )
        {
            // Secondary view selection
            if ( m_secondaryGraphView.HasSelectedNodes() )
            {
                m_recordedViewAndSelectionState.m_selectedNodes = m_selectedNodes;
                m_recordedViewAndSelectionState.m_isSecondaryViewSelection = true;
                EE_ASSERT( m_primaryGraphView.GetSelectedNodes().size() == 1 );
                m_recordedViewAndSelectionState.m_primaryViewSelectedNode = m_primaryGraphView.GetSelectedNodes().front();
            }
            else // Primary View Selection
            {
                m_recordedViewAndSelectionState.m_selectedNodes = m_selectedNodes;
                m_recordedViewAndSelectionState.m_isSecondaryViewSelection = false;
                m_recordedViewAndSelectionState.m_primaryViewSelectedNode.m_nodeID.Clear();
            }

            m_selectedNodes.clear();
        }
    }

    void AnimationGraphEditor::RestoreViewAndSelectionState()
    {
        enum class ViewRestoreResult { Failed, PartialMatch, Success };

        auto pRootGraph = GetEditedRootGraph();
        ViewRestoreResult viewRestoreResult = ViewRestoreResult::Failed;

        // Try to restore the view to the graph we were viewing before the undo/redo
        // This is necessary since undo/redo will change all graph ptrs
        if ( !m_recordedViewAndSelectionState.m_viewedGraphPath.empty() )
        {
            NodeGraph::BaseNode* pFoundParentNode = nullptr;
            for ( auto iter = m_recordedViewAndSelectionState.m_viewedGraphPath.rbegin(); iter != m_recordedViewAndSelectionState.m_viewedGraphPath.rend(); ++iter )
            {
                pFoundParentNode = pRootGraph->FindNode( *iter, true );
                if ( pFoundParentNode != nullptr )
                {
                    auto pReferencedGraph = pFoundParentNode->GetChildGraph();
                    if ( pReferencedGraph != nullptr )
                    {
                        m_primaryGraphView.SetGraphToView( pReferencedGraph );
                        viewRestoreResult = ( iter == m_recordedViewAndSelectionState.m_viewedGraphPath.rbegin() ) ? ViewRestoreResult::Success : ViewRestoreResult::PartialMatch;
                        break;
                    }
                }
            }
        }
        else // We had the root graph selected
        {
            NavigateTo( pRootGraph );
            viewRestoreResult = ViewRestoreResult::Success;
        }

        // Restore selection
        if ( viewRestoreResult == ViewRestoreResult::Success )
        {
            TVector<NodeGraph::BaseNode const*> validNodesToReselect;

            m_selectedNodes.clear();

            if ( m_recordedViewAndSelectionState.m_isSecondaryViewSelection )
            {
                auto pPrimaryViewedGraph = m_primaryGraphView.GetViewedGraph();
                auto pSelectedPrimaryNode = pPrimaryViewedGraph->FindNode( m_recordedViewAndSelectionState.m_primaryViewSelectedNode.m_nodeID );
                if ( pSelectedPrimaryNode != nullptr )
                {
                    m_primaryGraphView.SelectNode( pSelectedPrimaryNode );
                    UpdateSecondaryViewState();

                    auto pSecondaryViewedGraph = m_secondaryGraphView.GetViewedGraph();
                    for ( auto const& selectedNode : m_recordedViewAndSelectionState.m_selectedNodes )
                    {
                        auto pSelectedNode = pSecondaryViewedGraph->FindNode( selectedNode.m_nodeID );
                        if ( pSelectedNode != nullptr )
                        {
                            m_selectedNodes.emplace_back( NodeGraph::SelectedNode( pSelectedNode ) );
                            validNodesToReselect.emplace_back( pSelectedNode );
                        }
                    }

                    m_secondaryGraphView.SelectNodes( validNodesToReselect );
                }
            }
            else
            {
                auto pPrimaryViewedGraph = m_primaryGraphView.GetViewedGraph();
                for ( auto const& selectedNode : m_recordedViewAndSelectionState.m_selectedNodes )
                {
                    auto pSelectedNode = pPrimaryViewedGraph->FindNode( selectedNode.m_nodeID );
                    if ( pSelectedNode != nullptr )
                    {
                        m_selectedNodes.emplace_back( NodeGraph::SelectedNode( pSelectedNode ) );
                        validNodesToReselect.emplace_back( pSelectedNode );
                    }
                }

                m_primaryGraphView.SelectNodes( validNodesToReselect );
                UpdateSecondaryViewState();
            }
        }
        else if ( viewRestoreResult == ViewRestoreResult::Failed )
        {
            NavigateTo( pRootGraph );
        }

        // Clear saved state and refresh variation editor data
        m_recordedViewAndSelectionState.m_viewedGraphPath.clear();
        m_recordedViewAndSelectionState.Clear();
    }

    void AnimationGraphEditor::PreUndoRedo( UndoStack::Operation operation )
    {
        DataFileEditor::PreUndoRedo( operation );

        m_pBreadcrumbPopupContext = nullptr;

        // Stop Debugging
        //-------------------------------------------------------------------------

        if ( IsDebugging() )
        {
            StopDebugging();
        }

        RecordViewAndSelectionState();
        m_nodeEditorPropertyGrid.SetTypeToEdit( nullptr );
        m_nodeVariationDataPropertyGrid.SetTypeToEdit( nullptr );
    }

    void AnimationGraphEditor::PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        DataFileEditor::PostUndoRedo( operation, pAction );
        RestoreViewAndSelectionState();
        OnGraphStateModified();
    }

    void AnimationGraphEditor::PropertyGridPreEdit( PropertyEditInfo const& info )
    {
        EE_ASSERT( m_pActiveUndoableAction == nullptr );
        EE_ASSERT( IsDataFileLoaded() );

        if ( auto pNode = TryCast<NodeGraph::BaseNode>( info.m_pTypeInstanceEdited ) )
        {
            pNode->BeginModification();
        }
        else // Other modification
        {
            auto pGraphUndoableAction = EE::New<DataFileUndoableAction>( m_pToolsContext->m_pTypeRegistry, this );
            pGraphUndoableAction->SerializeBeforeState();
            m_pActiveUndoableAction = pGraphUndoableAction;
        }
    }

    void AnimationGraphEditor::PropertyGridPostEdit( PropertyEditInfo const& info )
    {
        EE_ASSERT( IsDataFileLoaded() );

        if ( auto pNode = TryCast<NodeGraph::BaseNode>( info.m_pTypeInstanceEdited ) )
        {
            pNode->EndModification();
        }
        else // Other modification
        {
            EE_ASSERT( m_pActiveUndoableAction != nullptr );

            Cast<DataFileUndoableAction>( m_pActiveUndoableAction )->SerializeAfterState();
            m_undoStack.RegisterAction( m_pActiveUndoableAction );
            m_pActiveUndoableAction = nullptr;
            MarkDirty();

            OnGraphStateModified();
        }
    }

    //-------------------------------------------------------------------------
    // Clip Browser
    //-------------------------------------------------------------------------

    void AnimationGraphEditor::DrawClipBrowser( UpdateContext const& context, bool isFocused )
    {
        // Update skeleton
        ResourceID skeletonID = GetEditedGraphData()->GetActiveVariationSkeleton();
        m_clipBrowser.SetSkeleton( skeletonID );

        // Draw browser window
        m_clipBrowser.Draw( context, isFocused );
    }
}

//-------------------------------------------------------------------------
// Property Grid Extensions
//-------------------------------------------------------------------------

namespace EE::Animation
{
    class IDEditor final : public PG::PropertyEditor
    {
        constexpr static uint32_t const s_bufferSize = 256;

    public:

        IDEditor( PG::PropertyEditorContext const& context, TypeSystem::PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, m_pPropertyInstance )
            , m_picker( reinterpret_cast<AnimationGraphEditor*>( context.m_pUserContext ) )
        {
            IDEditor::ResetWorkingCopy();
        }

    private:

        virtual Result InternalUpdateAndDraw() override
        {
            bool valueUpdated = false;

            //-------------------------------------------------------------------------

            ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x - 30 );
            if ( ImGui::InputText( "##agid", m_buffer, 255, ImGuiInputTextFlags_EnterReturnsTrue ) )
            {
                valueUpdated = true;
            }

            if ( ImGui::IsItemDeactivatedAfterEdit() )
            {
                valueUpdated = true;
            }

            if ( valueUpdated )
            {
                StringUtils::StripTrailingWhitespace( m_buffer );
            }

            //-------------------------------------------------------------------------

            ImGui::SameLine( 0, 0 );

            if ( m_picker.DrawAndUpdate( 30 ) )
            {
                auto pSelectedOption = m_picker.GetSelectedOption();
                if ( pSelectedOption != nullptr && pSelectedOption->m_value.IsValid() )
                {
                    m_value_cached = pSelectedOption->m_value;
                    strncpy_s( m_buffer, 255, pSelectedOption->m_value.c_str(), strlen( pSelectedOption->m_value.c_str() ) );
                }
                else // Clear value
                {
                    m_value_cached.Clear();
                    memset( m_buffer, 0, 255 );
                }

                valueUpdated = true;
            }

            //-------------------------------------------------------------------------

            return valueUpdated ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            if ( strlen( m_buffer ) > 0 )
            {
                m_value_cached = StringID( m_buffer );
            }
            else
            {
                m_value_cached.Clear();
            }

            *reinterpret_cast<StringID*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = *reinterpret_cast<StringID*>( m_pPropertyInstance );
            if ( m_value_cached.IsValid() )
            {
                strncpy_s( m_buffer, 255, m_value_cached.c_str(), strlen( m_value_cached.c_str() ) );
            }
            else
            {
                memset( m_buffer, 0, 255 );
            }
        }

        virtual void HandleExternalUpdate() override
        {
            StringID* pID = reinterpret_cast<StringID*>( m_pPropertyInstance );
            if ( *pID != m_value_cached )
            {
                m_value_cached = *pID;
                if ( m_value_cached.IsValid() )
                {
                    strncpy_s( m_buffer, 255, m_value_cached.c_str(), strlen( m_value_cached.c_str() ) );
                }
                else
                {
                    memset( m_buffer, 0, 255 );
                }
            }
        }

    private:

        AnimationGraphEditor::IDComboWidget         m_picker;
        char                                        m_buffer[256] = { 0 };
        StringID                                    m_value_cached;
    };

    //-------------------------------------------------------------------------

    class BoneMaskIDEditor final : public PG::PropertyEditor, public ImGuiX::ComboWithFilterWidget<StringID>
    {
        constexpr static uint32_t const s_bufferSize = 256;

    public:

        BoneMaskIDEditor( PG::PropertyEditorContext const& context, TypeSystem::PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, pTypeInstance, m_pPropertyInstance )
            , m_pGraphEditor( reinterpret_cast<AnimationGraphEditor*>( context.m_pUserContext ) )
        {
            BoneMaskIDEditor::ResetWorkingCopy();
        }

    private:

        virtual Result InternalUpdateAndDraw() override
        {
            return ImGuiX::ComboWithFilterWidget<StringID>::DrawAndUpdate() ? Result::ValueUpdated : Result::None;
        }

        virtual void UpdatePropertyValue() override
        {
            if ( HasValidSelection() )
            {
                m_value_cached = GetSelectedOption()->m_value;
            }
            else
            {
                m_value_cached.Clear();
            }

            *reinterpret_cast<StringID*>( m_pPropertyInstance ) = m_value_cached;
        }

        virtual void ResetWorkingCopy() override
        {
            m_value_cached = *reinterpret_cast<StringID*>( m_pPropertyInstance );
            SetSelectedOption( m_value_cached );
        }

        virtual void HandleExternalUpdate() override
        {
            StringID* pBoneMaskID = reinterpret_cast<StringID*>( m_pPropertyInstance );
            if ( *pBoneMaskID != m_value_cached )
            {
                m_value_cached = *pBoneMaskID;
                SetSelectedOption( m_value_cached );
            }
        }

        virtual void PopulateOptionsList() override
        {
            ResourceID const& skeletonResourceID = m_pGraphEditor->GetEditedGraphData()->GetActiveVariationSkeleton();
            auto pSkeletonFileEntry = m_pGraphEditor->m_pToolsContext->m_pFileRegistry->GetFileEntry( skeletonResourceID );
            if ( pSkeletonFileEntry != nullptr )
            {
                SkeletonResourceDescriptor const* pSkeletonDescriptor = Cast<SkeletonResourceDescriptor>( pSkeletonFileEntry->m_pDataFile );
                for ( auto const& boneMaskID : pSkeletonDescriptor->GetBoneMaskIDs() )
                {
                    if ( !boneMaskID.IsValid() )
                    {
                        continue;
                    }

                    auto& opt = m_options.emplace_back();
                    opt.m_label = boneMaskID.c_str();
                    opt.m_filterComparator = boneMaskID.c_str();
                    opt.m_filterComparator.make_lower();
                    opt.m_value = boneMaskID;
                }
            }
        };

    private:

        AnimationGraphEditor*                    m_pGraphEditor = nullptr;
        StringID                                    m_value_cached;
    };

    //-------------------------------------------------------------------------

    class TargetControlParameterEditingRules : public PG::TTypeEditingRules<TargetControlParameterToolsNode>
    {
        using PG::TTypeEditingRules<TargetControlParameterToolsNode>::TTypeEditingRules;

        virtual HiddenState IsHidden( StringID const& propertyID ) override
        {
            static StringID const PID_isBone( "m_isBoneID" );
            static StringID const PID_startTransform( "m_previewStartTransform" );
            static StringID const PID_boneID( "m_previewStartBoneID" );

            if ( propertyID == PID_isBone )
            {
                if ( !m_pTypeInstance->IsSet() )
                {
                    return HiddenState::Hidden;
                }
            }
            else if ( propertyID == PID_startTransform )
            {
                if ( !m_pTypeInstance->IsSet() || m_pTypeInstance->IsBoneTarget() )
                {
                    return HiddenState::Hidden;
                }
            }
            else if ( propertyID == PID_boneID )
            {
                if ( !m_pTypeInstance->IsSet() || !m_pTypeInstance->IsBoneTarget() )
                {
                    return HiddenState::Hidden;
                }
            }

            return HiddenState::Unhandled;
        }
    };

    //-------------------------------------------------------------------------

    EE_PROPERTY_GRID_CUSTOM_EDITOR( BoneMaskIDEditorFactory, "AnimGraph_BoneMaskID", BoneMaskIDEditor );
    EE_PROPERTY_GRID_CUSTOM_EDITOR( IDEditorFactory, "AnimGraph_ID", IDEditor );
    EE_PROPERTY_GRID_EDITING_RULES( TargetControlParameterEditingRulesFactory, TargetControlParameterToolsNode, TargetControlParameterEditingRules );
}