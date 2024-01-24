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
        ImGui::PushStyleColor( ImGuiCol_ChildBg, ImGuiX::Style::s_colorGray8 );
        if ( ImGui::BeginChild( "Triangulation", size ) )
        {
            auto pDrawList = ImGui::GetWindowDrawList();
            auto windowPos = ImGui::GetWindowPos();
            auto windowSize = ImGui::GetWindowSize();

            bool const validTriangulation = !blendSpace.m_indices.empty();
            Color const color = validTriangulation ? Colors::Green : Colors::Red;

            // Calculate points
            //-------------------------------------------------------------------------

            TInlineVector<ImVec2, 20> screenPoints;
            FloatRange horizontalViewRange;
            FloatRange verticalViewRange;

            if ( blendSpace.GetNumPoints() > 0 )
            {
                float minX = FLT_MAX, minY = FLT_MAX, maxX = -FLT_MAX, maxY = -FLT_MAX;

                for ( auto& point : blendSpace.m_points )
                {
                    minX = Math::Min( point.m_x, minX );
                    maxX = Math::Max( point.m_x, maxX );
                    minY = Math::Min( point.m_y, minY );
                    maxY = Math::Max( point.m_y, maxY );
                }

                horizontalViewRange = FloatRange( minX, maxX );
                float const dilationAmountX = horizontalViewRange.GetLength() / 5;
                horizontalViewRange.m_begin -= dilationAmountX;
                horizontalViewRange.m_end += dilationAmountX;

                verticalViewRange = FloatRange( minY, maxY );
                float const dilationAmountY = verticalViewRange.GetLength() / 5;
                verticalViewRange.m_begin -= dilationAmountY;
                verticalViewRange.m_end += dilationAmountY;

                InlineString str;
                for ( auto i = 0u; i < blendSpace.m_points.size(); i++ )
                {
                    auto& point = blendSpace.m_points[i];
                    ImVec2 screenPos;
                    screenPos.x = windowPos.x + ( horizontalViewRange.GetPercentageThrough( point.m_x ) * windowSize.x );
                    screenPos.y = windowPos.y + ( ( 1.0f - verticalViewRange.GetPercentageThrough( point.m_y ) ) * windowSize.y );
                    screenPoints.emplace_back( screenPos );
                }
            }

            // Draw Triangulation
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
                    pDrawList->AddLine( screenPoints[i0], screenPoints[i1], Colors::GreenYellow );
                }
            }
            else
            {
                ImVec2 const textSize = ImGui::CalcTextSize( "Invalid" );
                ImVec2 const textPos = windowPos + ImVec2( ( windowSize.x - textSize.x ) / 2, ( windowSize.y - textSize.y ) / 2 );
                pDrawList->AddText( textPos, Colors::Red, "Invalid" );
            }

            // Draw Points
            //-------------------------------------------------------------------------

            InlineString pointLabel;
            for ( auto i = 0u; i < blendSpace.m_points.size(); i++ )
            {
                auto& point = blendSpace.m_points[i];

                auto pFont = ImGuiX::GetFont( ImGuiX::Font::Medium );
                pointLabel = blendSpace.m_pointIDs[i].IsValid() ? blendSpace.m_pointIDs[i].c_str() : "Input";
                ImVec2 const labelSize = pFont->CalcTextSizeA( pFont->FontSize, FLT_MAX, -1.0f, pointLabel.c_str() );

                pDrawList->AddCircleFilled( screenPoints[i], 5.0f, color );
                pDrawList->AddText( pFont, pFont->FontSize, screenPoints[i] + ImVec2( labelSize.x / -2.0f, 6.f ), Colors::White, pointLabel.c_str() );
            }

            // Draw debug point
            //-------------------------------------------------------------------------

            if ( pDebugPoint != nullptr )
            {
                ImVec2 screenPos;
                screenPos.x = windowPos.x + ( horizontalViewRange.GetPercentageThrough( pDebugPoint->m_x ) * windowSize.x );
                screenPos.y = windowPos.y + ( ( 1.0f - verticalViewRange.GetPercentageThrough( pDebugPoint->m_y ) ) * windowSize.y );
                pDrawList->AddCircleFilled( screenPos, 3.0f, Colors::Gold );
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
}

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void Blend2DToolsNode::BlendSpace::AddPoint()
    {
        m_points.emplace_back( Float2::Zero );
        m_pointIDs.emplace_back( StringID() );
        GenerateTriangulation();
    }

    void Blend2DToolsNode::BlendSpace::RemovePoint( int32_t idx )
    {
        EE_ASSERT( idx >= 0 && idx < m_points.size() );
        m_points.erase( m_points.begin() + idx );
        m_pointIDs.erase( m_pointIDs.begin() + idx );
        GenerateTriangulation();
    }

    bool Blend2DToolsNode::BlendSpace::GenerateTriangulation()
    {
        bool triangulationSucceeded = false;

        m_indices.clear();
        m_hullIndices.clear();

        if ( m_points.size() < 3 )
        {
            return triangulationSucceeded;
        }

        //-------------------------------------------------------------------------

        IDelaBella2<float>* pIdb = IDelaBella2<float>::Create();
        int verts = pIdb->Triangulate( (int32_t) m_points.size(), &m_points[0].m_x, &m_points[0].m_y, sizeof(Float2));
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
        if ( rhs.m_points.size() != m_points.size() )
        {
            return true;
        }

        for ( size_t i = 0; i < m_points.size(); i++ )
        {
            if ( m_points[i] != rhs.m_points[i] )
            {
                return true;
            }

            if ( m_pointIDs[i] != rhs.m_pointIDs[i] )
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
    }

    void Blend2DToolsNode::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& nodeObjectValue )
    {
        FlowToolsNode::SerializeCustom( typeRegistry, nodeObjectValue );
        m_blendSpace.GenerateTriangulation();
    }

    int16_t Blend2DToolsNode::Compile( GraphCompilationContext & context ) const
    {
        Blend2DNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<Blend2DNode>( this, pDefinition );
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
                    pDefinition->m_inputParameterNodeIdx0 = compiledNodeIdx;
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
                    pDefinition->m_inputParameterNodeIdx1 = compiledNodeIdx;
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
                        pDefinition->m_sourceNodeIndices.emplace_back( compiledNodeIdx );
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

            // Common Definition
            //-------------------------------------------------------------------------

            for ( auto const& point : m_blendSpace.m_points )
            {
                pDefinition->m_values.emplace_back( point );
            }

            for ( auto const& index : m_blendSpace.m_indices )
            {
                pDefinition->m_indices.emplace_back( index );
            }

            for ( auto const& index : m_blendSpace.m_hullIndices )
            {
                pDefinition->m_hullIndices.emplace_back( index );
            }
        }

        return pDefinition->m_nodeIdx;
    }

    void Blend2DToolsNode::OnDynamicPinCreation( UUID pinID )
    {
        m_blendSpace.AddPoint();
        UpdateDynamicPins();
    }

    void Blend2DToolsNode::OnDynamicPinDestruction( UUID pinID )
    {
        int32_t const pinToBeRemovedIdx = GetInputPinIndex( pinID );
        EE_ASSERT( pinToBeRemovedIdx != InvalidIndex );
        int32_t const pointIdx = pinToBeRemovedIdx - 2;
        m_blendSpace.RemovePoint( pointIdx );
    }

    void Blend2DToolsNode::DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext )
    {
        Float2* pActualDebugPoint = nullptr; // This is done to let the visualization know if we have a point or not
        Float2 debugPointStorage;

        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        int16_t const runtimeNodeIdx = pGraphNodeContext->HasDebugData() ? pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() ) : InvalidIndex;
        if ( runtimeNodeIdx != InvalidIndex )
        {
            auto pBlendNode = static_cast<GraphNodes::Blend2DNode const*>( pGraphNodeContext->GetNodeDebugInstance( runtimeNodeIdx ) );
            if ( pBlendNode->IsInitialized() )
            {
                debugPointStorage = pBlendNode->GetParameter();
                pActualDebugPoint = &debugPointStorage;
            }
        }

        DrawBlendSpaceVisualization( m_blendSpace, ImVec2( 200, 200 ), pActualDebugPoint );
    }

    void Blend2DToolsNode::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        FlowToolsNode::PostPropertyEdit( pPropertyEdited );

        if ( pPropertyEdited->m_ID == StringID( "m_blendSpace" ) )
        {
            UpdateDynamicPins();
        }
    }

    void Blend2DToolsNode::UpdateDynamicPins()
    {
        int32_t const numPins = GetNumInputPins();
        EE_ASSERT( numPins == ( m_blendSpace.GetNumPoints() + 2 ) );
        for ( int32_t i = 2; i < numPins; i++ )
        {
            VisualGraph::Flow::Pin* pInputPin = GetInputPin( i );
            int32_t const pointIdx = i - 2;
            StringID const& pointID = m_blendSpace.m_pointIDs[pointIdx];
            pInputPin->m_name.sprintf( "%s (%.2f, %.2f)", pointID.IsValid() ? pointID.c_str() : "Input", m_blendSpace.m_points[pointIdx].m_x, m_blendSpace.m_points[pointIdx].m_y );
        }
    }
}

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class BlendSpaceEditor final : public PG::PropertyEditor
    {
        constexpr static uint32_t const s_bufferSize = 64;

    public:

        using PropertyEditor::PropertyEditor;

        BlendSpaceEditor( PG::PropertyEditorContext const& context, TypeSystem::PropertyInfo const& propertyInfo, void* m_pPropertyInstance )
            : PropertyEditor( context, propertyInfo, m_pPropertyInstance )
        {}

    private:

        void UpdateBuffers()
        {
            int32_t const numPoints = m_value_cached.GetNumPoints();

            m_IDBuffers.resize( numPoints );
            for ( int32_t i = 0; i < numPoints; i++ )
            {
                m_IDBuffers[i].resize( s_bufferSize );
                if ( m_value_cached.m_pointIDs[i].IsValid() )
                {
                    char const* const pIDString = m_value_cached.m_pointIDs[i].c_str();
                    memcpy( m_IDBuffers[i].data(), pIDString, strlen( pIDString ) );
                }
                else
                {
                    Memory::MemsetZero( m_IDBuffers[i].data(), s_bufferSize );
                }
            }
        }

        virtual void UpdatePropertyValue() override
        {
            auto pDefinition = reinterpret_cast<Blend2DToolsNode::BlendSpace*>( m_pPropertyInstance );
            *pDefinition = m_value_imgui;
            m_value_cached = m_value_imgui;
            UpdateBuffers();
        }

        virtual void ResetWorkingCopy() override
        {
            auto pDefinition = reinterpret_cast<Blend2DToolsNode::BlendSpace*>( m_pPropertyInstance );
            m_value_imgui = *pDefinition;
            m_value_cached = *pDefinition;
            UpdateBuffers();
        }

        virtual void HandleExternalUpdate() override
        {
            auto pDefinition = reinterpret_cast<Blend2DToolsNode::BlendSpace*>( m_pPropertyInstance );
            if ( *pDefinition != m_value_cached )
            {
                ResetWorkingCopy();
            }
        }

        virtual bool InternalUpdateAndDraw() override
        {
            bool valueUpdated = false;

            // Point editors
            //-------------------------------------------------------------------------

            int32_t const numPoints = m_value_imgui.GetNumPoints();
            for ( auto i = 0; i < numPoints; i++ )
            {
                ImGui::PushID( i );
                ImGui::AlignTextToFramePadding();

                // ID
                //-------------------------------------------------------------------------

                ImGui::Text( "ID" );
                ImGui::SameLine();

                bool IDUpdated = false;
                ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail().x / 3 );
                if ( ImGui::InputText( "##StringInput", m_IDBuffers[i].data(), s_bufferSize, ImGuiInputTextFlags_EnterReturnsTrue) )
                {
                    IDUpdated = true;
                }

                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    IDUpdated = true;
                }

                if ( IDUpdated )
                {
                    m_value_imgui.m_pointIDs[i] = StringID( m_IDBuffers[i].data() );
                    valueUpdated = true;
                }

                // Value
                //-------------------------------------------------------------------------

                ImGui::SameLine();
                if ( ImGuiX::InputFloat2( "i", m_value_imgui.m_points[i] ) )
                {
                    m_value_imgui.GenerateTriangulation();
                    valueUpdated = true;
                }
                ImGui::PopID();
            }

            // Draw visual representation of blendspace
            //-------------------------------------------------------------------------

            DrawBlendSpaceVisualization( m_value_imgui, ImVec2( ImGui::GetContentRegionAvail().x, 300 ) );

            //-------------------------------------------------------------------------

            return valueUpdated;
        }

    private:

        Blend2DToolsNode::BlendSpace                        m_value_imgui;
        Blend2DToolsNode::BlendSpace                        m_value_cached;
        TVector<TInlineVector<char, s_bufferSize>>          m_IDBuffers;
    };

    //-------------------------------------------------------------------------

    EE_PROPERTY_GRID_TYPE_EDITOR( BlendSpaceEditorFactory, Blend2DToolsNode::BlendSpace, BlendSpaceEditor );
}