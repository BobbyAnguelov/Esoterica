#include "AnimationGraphEditor_ControlParameterEditor.h"
#include "AnimationGraphEditor_Context.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Definition.h"
#include "EngineTools/Animation/ToolsGraph/Nodes/Animation_ToolsGraphNode_Parameters.h"
#include "EngineTools/Core/Helpers/CategoryTree.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    struct BoolParameterState : public GraphControlParameterEditor::ParameterPreviewState
    {
        using ParameterPreviewState::ParameterPreviewState;

        virtual void DrawPreviewEditor( ToolsNodeContext* pGraphNodeContext ) override
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

    struct IntParameterState : public GraphControlParameterEditor::ParameterPreviewState
    {
        using ParameterPreviewState::ParameterPreviewState;

        virtual void DrawPreviewEditor( ToolsNodeContext* pGraphNodeContext ) override
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

    struct FloatParameterState : public GraphControlParameterEditor::ParameterPreviewState
    {
        FloatParameterState( GraphNodes::ControlParameterToolsNode* pParameter )
            : ParameterPreviewState( pParameter )
        {
            auto pFloatParameter = Cast<GraphNodes::FloatControlParameterToolsNode>( pParameter );
            m_min = pFloatParameter->GetPreviewRangeMin();
            m_max = pFloatParameter->GetPreviewRangeMax();
        }

        virtual void DrawPreviewEditor( ToolsNodeContext* pGraphNodeContext ) override
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

    struct VectorParameterState : public GraphControlParameterEditor::ParameterPreviewState
    {
        using ParameterPreviewState::ParameterPreviewState;

        virtual void DrawPreviewEditor( ToolsNodeContext* pGraphNodeContext ) override
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

    struct IDParameterState : public GraphControlParameterEditor::ParameterPreviewState
    {
        using ParameterPreviewState::ParameterPreviewState;

        virtual void DrawPreviewEditor( ToolsNodeContext* pGraphNodeContext ) override
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

    struct TargetParameterState : public GraphControlParameterEditor::ParameterPreviewState
    {
        using ParameterPreviewState::ParameterPreviewState;

        virtual void DrawPreviewEditor( ToolsNodeContext* pGraphNodeContext ) override
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
        char            m_buffer[256] = {0};
        EulerAngles     m_rotationOffset;
        Float3          m_translationOffset = Float3::Zero;
        Transform       m_transform = Transform::Identity;
    };

    //-------------------------------------------------------------------------

    GraphControlParameterEditor::GraphControlParameterEditor( GraphEditorContext& editorContext )
        : m_editorContext( editorContext )
    {
        m_updateCacheTimer.Start( 0.0f );
    }

    GraphControlParameterEditor::~GraphControlParameterEditor()
    {
        DestroyPreviewStates();
    }

    bool GraphControlParameterEditor::UpdateAndDraw( UpdateContext const& context, ToolsNodeContext* pGraphNodeContext, ImGuiWindowClass* pWindowClass, char const* pWindowName )
    {
        m_pVirtualParamaterToEdit = nullptr;

        int32_t windowFlags = 0;
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 4, 4 ) );
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( pWindowName, nullptr, windowFlags ) )
        {

            bool const isPreviewing = pGraphNodeContext->HasDebugData();
            if ( isPreviewing )
            {
                if ( m_parameterPreviewStates.empty() )
                {
                    CreatePreviewStates();
                }

                DrawParameterPreviewControls( pGraphNodeContext );
            }
            else
            {
                if ( !m_parameterPreviewStates.empty() )
                {
                    DestroyPreviewStates();
                }

                DrawParameterList();
            }

            //-------------------------------------------------------------------------

            if ( m_activeOperation != OperationType::None )
            {
                DrawDialogs();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar( 1 );

        //-------------------------------------------------------------------------

        return m_pVirtualParamaterToEdit != nullptr;
    }

    void GraphControlParameterEditor::DrawAddParameterCombo()
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
                m_editorContext.CreateControlParameter( GraphValueType::Bool );
            }

            if ( ImGui::MenuItem( "Control Parameter - ID" ) )
            {
                m_editorContext.CreateControlParameter( GraphValueType::ID );
            }

            if ( ImGui::MenuItem( "Control Parameter - Int" ) )
            {
                m_editorContext.CreateControlParameter( GraphValueType::Int );
            }

            if ( ImGui::MenuItem( "Control Parameter - Float" ) )
            {
                m_editorContext.CreateControlParameter( GraphValueType::Float );
            }

            if ( ImGui::MenuItem( "Control Parameter - Vector" ) )
            {
                m_editorContext.CreateControlParameter( GraphValueType::Vector );
            }

            if ( ImGui::MenuItem( "Control Parameter - Target" ) )
            {
                m_editorContext.CreateControlParameter( GraphValueType::Target );
            }

            //-------------------------------------------------------------------------

            ImGuiX::TextSeparator( "Virtual Parameters" );

            if ( ImGui::MenuItem( "Virtual Parameter - Bool" ) )
            {
                m_editorContext.CreateVirtualParameter( GraphValueType::Bool );
            }

            if ( ImGui::MenuItem( "Virtual Parameter - ID" ) )
            {
                m_editorContext.CreateVirtualParameter( GraphValueType::ID );
            }

            if ( ImGui::MenuItem( "Virtual Parameter - Int" ) )
            {
                m_editorContext.CreateVirtualParameter( GraphValueType::Int );
            }

            if ( ImGui::MenuItem( "Virtual Parameter - Float" ) )
            {
                m_editorContext.CreateVirtualParameter( GraphValueType::Float );
            }

            if ( ImGui::MenuItem( "Virtual Parameter - Vector" ) )
            {
                m_editorContext.CreateVirtualParameter( GraphValueType::Vector );
            }

            if ( ImGui::MenuItem( "Virtual Parameter - Target" ) )
            {
                m_editorContext.CreateVirtualParameter( GraphValueType::Target );
            }

            if ( ImGui::MenuItem( "Virtual Parameter - Bone Mask" ) )
            {
                m_editorContext.CreateVirtualParameter( GraphValueType::BoneMask );
            }

            ImGui::EndPopup();
        }
    }

    void GraphControlParameterEditor::DrawDialogs()
    {
        bool isDialogOpen = m_activeOperation != OperationType::None;

        if ( m_activeOperation == OperationType::Rename )
        {
            ImGui::OpenPopup( "Rename Dialog" );
            ImGui::SetNextWindowSize( ImVec2( 300, -1 ) );
            if ( ImGui::BeginPopupModal( "Rename Dialog", &isDialogOpen, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize ) )
            {
                EE_ASSERT( m_activeOperation == OperationType::Rename );
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
                    if ( auto pControlParameter = m_editorContext.FindControlParameter( m_currentOperationParameterID ) )
                    {
                        m_editorContext.RenameControlParameter( m_currentOperationParameterID, m_parameterNameBuffer, m_parameterCategoryBuffer );
                    }
                    else if ( auto pVirtualParameter = m_editorContext.FindVirtualParameter( m_currentOperationParameterID ) )
                    {
                        m_editorContext.RenameVirtualParameter( m_currentOperationParameterID, m_parameterNameBuffer, m_parameterCategoryBuffer );
                    }
                    else
                    {
                        EE_UNREACHABLE_CODE();
                    }

                    m_activeOperation = OperationType::None;
                }

                ImGui::SameLine( 0, 4 );

                if ( ImGui::Button( "Cancel", ImVec2( 50, 0 ) ) )
                {
                    m_activeOperation = OperationType::None;
                }

                ImGui::EndPopup();
            }
        }

        //-------------------------------------------------------------------------

        if ( m_activeOperation == OperationType::Delete )
        {
            ImGui::OpenPopup( "Delete Dialog" );
            if ( ImGui::BeginPopupModal( "Delete Dialog", &isDialogOpen, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize ) )
            {
                EE_ASSERT( m_activeOperation == OperationType::Delete );

                int32_t numUses = 0;
                auto referenceNodes = m_editorContext.FindAllNodesOfType<GraphNodes::ParameterReferenceToolsNode>( VisualGraph::SearchMode::Recursive );

                for ( auto pReferenceNode : referenceNodes )
                {
                    if ( pReferenceNode->IsReferencingControlParameter() && pReferenceNode->GetReferencedControlParameter()->GetID() == m_currentOperationParameterID )
                    {
                        numUses++;
                    }
                }

                ImGui::Text( "This parameter is used in %d places.", numUses );
                ImGui::Text( "Are you sure you want to delete this parameter?" );
                ImGui::NewLine();

                float const dialogWidth = ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ).x;
                ImGui::SameLine( 0, dialogWidth - 64 );

                if ( ImGui::Button( "Yes", ImVec2( 30, 0 ) ) )
                {
                    if ( auto pControlParameter = m_editorContext.FindControlParameter( m_currentOperationParameterID ) )
                    {
                        m_editorContext.DestroyControlParameter( m_currentOperationParameterID );
                    }
                    else if ( auto pVirtualParameter = m_editorContext.FindVirtualParameter( m_currentOperationParameterID ) )
                    {
                        m_editorContext.DestroyVirtualParameter( m_currentOperationParameterID );
                    }
                    else
                    {
                        EE_UNREACHABLE_CODE();
                    }

                    m_activeOperation = OperationType::None;
                }

                ImGui::SameLine( 0, 4 );

                if ( ImGui::Button( "No", ImVec2( 30, 0 ) ) )
                {
                    m_activeOperation = OperationType::None;
                }

                ImGui::EndPopup();
            }
        }

        //-------------------------------------------------------------------------

        if ( !isDialogOpen )
        {
            m_activeOperation = OperationType::None;
        }
    }

    void GraphControlParameterEditor::DrawParameterList()
    {
        // Create parameter tree
        //-------------------------------------------------------------------------

        CategoryTree<GraphNodes::FlowToolsNode*> parameterTree;

        for ( auto pControlParameter : m_editorContext.GetControlParameters() )
        {
            parameterTree.AddItem( pControlParameter->GetParameterCategory(), pControlParameter->GetName(), pControlParameter );
        }

        for ( auto pVirtualParameter : m_editorContext.GetVirtualParameters() )
        {
            parameterTree.AddItem( pVirtualParameter->GetParameterCategory(), pVirtualParameter->GetName(), pVirtualParameter );
        }

        // Update cache
        //-------------------------------------------------------------------------

        if ( m_updateCacheTimer.Update() )
        {
            m_cachedNumUses.clear();

            auto referenceNodes = m_editorContext.FindAllNodesOfType<GraphNodes::ParameterReferenceToolsNode>( VisualGraph::SearchMode::Recursive );
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
                m_editorContext.SetSelectedNodes( { VisualGraph::SelectedNode( pControlParameter ) } );
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

    void GraphControlParameterEditor::DrawParameterPreviewControls( ToolsNodeContext* pGraphNodeContext )
    {
        EE_ASSERT( pGraphNodeContext != nullptr );

        ImGuiX::ScopedFont const sf( ImGuiX::Font::Small );

        //-------------------------------------------------------------------------

        CategoryTree<ParameterPreviewState*> parameterTree;
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

        auto DrawControlParameterEditorRow = [this, pGraphNodeContext] ( ParameterPreviewState* pPreviewState )
        {
            auto pControlParameter = pPreviewState->m_pParameter;
            int16_t const parameterIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( pControlParameter->GetID() );

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
            pPreviewState->DrawPreviewEditor( pGraphNodeContext );
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

    //-------------------------------------------------------------------------

    void GraphControlParameterEditor::CreatePreviewStates()
    {
        int32_t const numParameters = m_editorContext.GetNumControlParameters();
        for ( int32_t i = 0; i < numParameters; i++ )
        {
            auto pControlParameter = m_editorContext.GetControlParameters()[i];
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

    void GraphControlParameterEditor::DestroyPreviewStates()
    {
        for ( auto& pPreviewState : m_parameterPreviewStates )
        {
            EE::Delete( pPreviewState );
        }

        m_parameterPreviewStates.clear();
    }

    //-------------------------------------------------------------------------

    void GraphControlParameterEditor::StartParameterRename( UUID const& parameterID )
    {
        EE_ASSERT( parameterID.IsValid() );
        m_currentOperationParameterID = parameterID;
        m_activeOperation = OperationType::Rename;

        if ( auto pControlParameter = m_editorContext.FindControlParameter( parameterID ) )
        {
            strncpy_s( m_parameterNameBuffer, pControlParameter->GetName(), 255 );
            strncpy_s( m_parameterCategoryBuffer, pControlParameter->GetParameterCategory().c_str(), 255);
        }
        else if ( auto pVirtualParameter = m_editorContext.FindVirtualParameter( parameterID ) )
        {
            strncpy_s( m_parameterNameBuffer, pVirtualParameter->GetName(), 255 );
            strncpy_s( m_parameterCategoryBuffer, pVirtualParameter->GetParameterCategory().c_str(), 255 );
        }
        else
        {
            EE_UNREACHABLE_CODE();
        }
    }

    void GraphControlParameterEditor::StartParameterDelete( UUID const& parameterID )
    {
        m_currentOperationParameterID = parameterID;
        m_activeOperation = OperationType::Delete;
    }
}