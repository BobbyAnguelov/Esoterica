#include "Animation_ToolsGraphNode_Blend2D.h"
#include "Animation_ToolsGraphNode_AnimationClip.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Blend2D.h"
#include "EngineTools/Core/PropertyGrid/PropertyGridEditor.h"
#include "EngineTools/ThirdParty/delabella/delabella.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void DrawBlendSpaceVisualization( Blend2DToolsNode::BlendSpace const& blendSpace, ImVec2 const& size, Float2* pDebugPoint = nullptr )
    {
        ImGui::PushStyleColor( ImGuiCol_ChildBg, ImGuiX::Style::s_colorGray8.Value );
        if ( ImGui::BeginChild( "Triangulation", size ) )
        {
            auto pDrawList = ImGui::GetWindowDrawList();
            auto windowPos = ImGui::GetWindowPos();
            auto windowSize = ImGui::GetWindowSize();

            float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;

            for ( auto& point : blendSpace.m_inputValues )
            {
                minX = Math::Min( point.m_x, minX );
                maxX = Math::Max( point.m_x, maxX );
                minY = Math::Min( point.m_y, minY );
                maxY = Math::Max( point.m_y, maxY );
            }

            FloatRange horizontalViewRange = FloatRange( minX, maxX );
            float const dilationAmountX = horizontalViewRange.GetLength() / 5;
            horizontalViewRange.m_begin -= dilationAmountX;
            horizontalViewRange.m_end += dilationAmountX;

            FloatRange verticalViewRange = FloatRange( minY, maxY );
            float const dilationAmountY = verticalViewRange.GetLength() / 5;
            verticalViewRange.m_begin -= dilationAmountY;
            verticalViewRange.m_end += dilationAmountY;

            //-------------------------------------------------------------------------

            bool const validTriangulation = !blendSpace.m_indices.empty();
            ImColor const color = validTriangulation ? ImGuiX::ImColors::Green : ImGuiX::ImColors::Red;

            InlineString str;
            TInlineVector<ImVec2, 20> screenPoints;
            for ( auto i = 0u; i < blendSpace.m_inputValues.size(); i++ )
            {
                auto& point = blendSpace.m_inputValues[i];

                auto pFont = ImGuiX::GetFont( ImGuiX::Font::Tiny );
                str.clear();
                str.append_sprintf( "%d : %.1f, %.1f", i, blendSpace.m_inputValues[i].m_x, blendSpace.m_inputValues[i].m_y );
                ImVec2 const labelSize = pFont->CalcTextSizeA( pFont->FontSize, FLT_MAX, -1.0f, str.c_str() );

                ImVec2 screenPos;
                screenPos.x = windowPos.x + ( horizontalViewRange.GetPercentageThrough( point.m_x ) * windowSize.x );
                screenPos.y = windowPos.y + ( ( 1.0f - verticalViewRange.GetPercentageThrough( point.m_y ) ) * windowSize.y );

                pDrawList->AddCircleFilled( screenPos, 5.0f, color );
                pDrawList->AddText( pFont, pFont->FontSize, screenPos + ImVec2( labelSize.x / -2.0f, 2.5f ), ImGuiX::ImColors::White, str.c_str() );
                screenPoints.emplace_back( screenPos );
            }

            //-------------------------------------------------------------------------

            if ( validTriangulation )
            {
                for ( auto i = 0; i < blendSpace.m_indices.size(); i += 3 )
                {
                    uint8_t idx0 = blendSpace.m_indices[i + 0];
                    uint8_t idx1 = blendSpace.m_indices[i + 1];
                    uint8_t idx2 = blendSpace.m_indices[i + 2];

                    pDrawList->AddLine( screenPoints[idx0], screenPoints[idx1], color );
                    pDrawList->AddLine( screenPoints[idx1], screenPoints[idx2], color );
                    pDrawList->AddLine( screenPoints[idx2], screenPoints[idx0], color );
                }

                // Hull has the first index duplicated at the end
                for ( auto i = 1; i < blendSpace.m_hullIndices.size(); i++ )
                {
                    uint8_t const i0 = blendSpace.m_hullIndices[i - 1];
                    uint8_t const i1 = blendSpace.m_hullIndices[i];
                    pDrawList->AddLine( screenPoints[i0], screenPoints[i1], ImGuiX::ImColors::GreenYellow );
                }
            }

            //-------------------------------------------------------------------------

            if ( pDebugPoint != nullptr )
            {
                ImVec2 screenPos;
                screenPos.x = windowPos.x + ( horizontalViewRange.GetPercentageThrough( pDebugPoint->m_x ) * windowSize.x );
                screenPos.y = windowPos.y + ( ( 1.0f - verticalViewRange.GetPercentageThrough( pDebugPoint->m_y ) ) * windowSize.y );
                pDrawList->AddCircleFilled( screenPos, 3.0f, ImGuiX::ImColors::Gold );
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
}

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void Blend2DToolsNode::BlendSpace::AddInput()
    {
        m_inputValues.emplace_back( Float2::Zero );
        GenerateTriangulation();
    }

    void Blend2DToolsNode::BlendSpace::RemoveInput( int32_t idx )
    {
        EE_ASSERT( idx >= 0 && idx < m_inputValues.size() );
        m_inputValues.erase( m_inputValues.begin() + idx );
        GenerateTriangulation();
    }

    bool Blend2DToolsNode::BlendSpace::GenerateTriangulation()
    {
        bool triangulationSucceeded = false;

        m_indices.clear();
        m_hullIndices.clear();

        if ( m_inputValues.size() < 3 )
        {
            return triangulationSucceeded;
        }

        //-------------------------------------------------------------------------

        IDelaBella2<float>* pIdb = IDelaBella2<float>::Create();
        int verts = pIdb->Triangulate( (int32_t) m_inputValues.size(), &m_inputValues[0].m_x, &m_inputValues[0].m_y, sizeof(Float2));
        triangulationSucceeded = verts > 0;

        // if positive, all ok 
        if ( triangulationSucceeded )
        {
            int tris = pIdb->GetNumPolygons();
            const IDelaBella2<float>::Simplex* dela = pIdb->GetFirstDelaunaySimplex();
            for ( int i = 0; i < tris; i++ )
            {
                EE_ASSERT( dela->v[0]->i < 255 );
                m_indices.emplace_back( (uint8_t) dela->v[0]->i );

                EE_ASSERT( dela->v[1]->i < 255 );
                m_indices.emplace_back( (uint8_t) dela->v[1]->i );

                EE_ASSERT( dela->v[2]->i < 255 );
                m_indices.emplace_back( (uint8_t) dela->v[2]->i );

                dela = dela->next;
            }

            auto* pVert = pIdb->GetFirstBoundaryVertex();
            for ( auto i = 0; i < pIdb->GetNumBoundaryVerts(); i++ )
            {
                m_hullIndices.emplace_back( (uint8_t) pVert->i );
                pVert = pVert->next;
            }

            // Add an extra index to avoid additional logic in loops
            m_hullIndices.emplace_back( m_hullIndices.front() );
        }

        pIdb->Destroy();

        //-------------------------------------------------------------------------

        return triangulationSucceeded;
    }

    bool Blend2DToolsNode::BlendSpace::operator!=( BlendSpace const& rhs )
    {
        if ( rhs.m_inputValues.size() != m_inputValues.size() )
        {
            return true;
        }

        for ( size_t i = 0; i < m_inputValues.size(); i++ )
        {
            if ( m_inputValues[i] != rhs.m_inputValues[i] )
            {
                return true;
            }
        }

        return false;
    }
}

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    Blend2DToolsNode::Blend2DToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "X", GraphValueType::Float );
        CreateInputPin( "Y", GraphValueType::Float );
        CreateInputPin( "Input 0", GraphValueType::Pose );
        CreateInputPin( "Input 1", GraphValueType::Pose );
        CreateInputPin( "Input 2", GraphValueType::Pose );

        //-------------------------------------------------------------------------

        m_blendSpace.AddInput();
        m_blendSpace.AddInput();
        m_blendSpace.AddInput();
    }

    void Blend2DToolsNode::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& nodeObjectValue )
    {
        FlowToolsNode::SerializeCustom( typeRegistry, nodeObjectValue );
        m_blendSpace.GenerateTriangulation();
    }

    int16_t Blend2DToolsNode::Compile( GraphCompilationContext & context ) const
    {
        Blend2DNode::Settings* pSettings = nullptr;
        NodeCompilationState const state = context.GetSettings<Blend2DNode>( this, pSettings );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            // Validate blendspace
            //-------------------------------------------------------------------------

            // We only allow 32 input nodes
            if ( GetNumInputPins() > 34 )
            {
                context.LogError( this, "You have too many input options (max is 32) - please remove some options to continue!" );
                return InvalidIndex;
            }

            BlendSpace blendspace = m_blendSpace;
            if ( !blendspace.GenerateTriangulation() )
            {
                context.LogError( this, "Triangulation Failed - You have invalid parameter values!" );
                return InvalidIndex;
            }

            // Parameters
            //-------------------------------------------------------------------------

            auto pParameterNode0 = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pParameterNode0 != nullptr )
            {
                int16_t const compiledNodeIdx = pParameterNode0->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputParameterNodeIdx0 = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected parameter pin on blend node!" );
                return InvalidIndex;
            }

            auto pParameterNode1 = GetConnectedInputNode<FlowToolsNode>( 1 );
            if ( pParameterNode1 != nullptr )
            {
                int16_t const compiledNodeIdx = pParameterNode1->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pSettings->m_inputParameterNodeIdx1 = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected parameter pin on blend node!" );
                return InvalidIndex;
            }

            // Sources
            //-------------------------------------------------------------------------

            for ( auto i = 2; i < GetNumInputPins(); i++ )
            {
                auto pSourceNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pSourceNode != nullptr )
                {
                    int16_t const compiledNodeIdx = pSourceNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pSettings->m_sourceNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
                else
                {
                    context.LogError( this, "Disconnected source pin on blend node!" );
                    return InvalidIndex;
                }
            }

            // Common Settings
            //-------------------------------------------------------------------------

            for ( auto const& point : m_blendSpace.m_inputValues )
            {
                pSettings->m_values.emplace_back( point );
            }

            for ( auto const& index : m_blendSpace.m_indices )
            {
                pSettings->m_indices.emplace_back( index );
            }

            for ( auto const& index : m_blendSpace.m_hullIndices )
            {
                pSettings->m_hullIndices.emplace_back( index );
            }
        }

        return pSettings->m_nodeIdx;
    }

    void Blend2DToolsNode::OnDynamicPinCreation( UUID pinID )
    {
        m_blendSpace.AddInput();
    }

    void Blend2DToolsNode::OnDynamicPinDestruction( UUID pinID )
    {
        int32_t const pinToBeRemovedIdx = GetInputPinIndex( pinID );
        EE_ASSERT( pinToBeRemovedIdx != InvalidIndex );
        m_blendSpace.RemoveInput( pinToBeRemovedIdx - 3 );

        // Rename Pins
        //-------------------------------------------------------------------------

        int32_t newPinIdx = 0;
        int32_t const numOptions = GetNumInputPins();
        for ( auto i = 2; i < numOptions; i++ )
        {
            if ( i == pinToBeRemovedIdx )
            {
                continue;
            }

            TInlineString<100> newPinName;
            newPinName.sprintf( "Input %d", newPinIdx );
            GetInputPin( i )->m_name = newPinName;
            newPinIdx++;
        }
    }

    TInlineString<100> Blend2DToolsNode::GetNewDynamicInputPinName() const
    {
        return TInlineString<100>( TInlineString<100>::CtorSprintf(), "Input %d", GetNumInputPins() - 3 );
    }

    void Blend2DToolsNode::DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext )
    {
        Float2 debugPoint;
        Float2* pDebugPoint = nullptr;

        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        int16_t const runtimeNodeIdx = pGraphNodeContext->HasDebugData() ? pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() ) : InvalidIndex;
        if ( runtimeNodeIdx != InvalidIndex )
        {
            debugPoint = pGraphNodeContext->GetBlend2DParameter( runtimeNodeIdx );
            pDebugPoint = &debugPoint;
        }

        DrawBlendSpaceVisualization( m_blendSpace, ImVec2( 200, 200 ), pDebugPoint );
    }
}

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class BlendSpaceEditor final : public PG::PropertyEditor
    {
    public:

        using PropertyEditor::PropertyEditor;

        BlendSpaceEditor( PG::PropertyEditorContext const& context, TypeSystem::PropertyInfo const& propertyInfo, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, m_pPropertyInstance )
        {}

    private:

        virtual void UpdatePropertyValue() override
        {
            auto pSettings = reinterpret_cast<Blend2DToolsNode::BlendSpace*>( m_pPropertyInstance );
            *pSettings = m_value_imgui;
            m_value_cached = m_value_imgui;
        }

        virtual void ResetWorkingCopy() override
        {
            auto pSettings = reinterpret_cast<Blend2DToolsNode::BlendSpace*>( m_pPropertyInstance );
            m_value_imgui = *pSettings;
            m_value_cached = *pSettings;
        }

        virtual void HandleExternalUpdate() override
        {
            auto pSettings = reinterpret_cast<Blend2DToolsNode::BlendSpace*>( m_pPropertyInstance );
            if ( *pSettings != m_value_cached )
            {
                ResetWorkingCopy();
            }
        }

        virtual bool InternalUpdateAndDraw() override
        {
            bool valueChanged = false;

            // Point editors
            //-------------------------------------------------------------------------

            for ( auto i = 0; i < m_value_imgui.m_inputValues.size(); i++ )
            {
                ImGui::PushID( i );
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "Input %d: ", i );
                ImGui::SameLine();
                if ( ImGuiX::InputFloat2( "i", m_value_imgui.m_inputValues[i] ) )
                {
                    m_value_imgui.GenerateTriangulation();
                    valueChanged = true;
                }
                ImGui::PopID();
            }

            // Draw visual representation of blendspace
            //-------------------------------------------------------------------------

            DrawBlendSpaceVisualization( m_value_imgui, ImVec2( ImGui::GetContentRegionAvail().x, 300 ) );

            //-------------------------------------------------------------------------

            return valueChanged;
        }

    private:

        Blend2DToolsNode::BlendSpace        m_value_imgui;
        Blend2DToolsNode::BlendSpace        m_value_cached;
    };

    //-------------------------------------------------------------------------

    EE_PROPERTY_GRID_TYPE_EDITOR( BlendSpaceEditorFactory, Blend2DToolsNode::BlendSpace, BlendSpaceEditor );
}