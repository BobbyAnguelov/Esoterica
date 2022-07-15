#include "Animation_EditorGraph_FlowGraph.h"
#include "Animation_EditorGraph_Variations.h"
#include "Animation_EditorGraph_Compilation.h"
#include "Engine/Animation/Components/Component_AnimationGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Parameters.h"
#include "System/Imgui/ImguiStyle.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    static void TraverseHierarchy( VisualGraph::BaseNode const* pNode, TVector<VisualGraph::BaseNode const*>& nodePath )
    {
        EE_ASSERT( pNode != nullptr );
        nodePath.emplace_back( pNode );

        if ( pNode->HasParentGraph() && !pNode->GetParentGraph()->IsRootGraph() )
        {
            TraverseHierarchy( pNode->GetParentGraph()->GetParentNode(), nodePath );
        }
    }

    //-------------------------------------------------------------------------

    void EditorGraphNode::DrawExtraControls( VisualGraph::DrawContext const& ctx )
    {
        auto pGraphNodeContext = reinterpret_cast<GraphNodeContext*>( ctx.m_pUserContext );
        bool const isPreviewing = pGraphNodeContext->HasDebugData();
        int16_t const runtimeNodeIdx = isPreviewing ? pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() ) : InvalidIndex;
        bool const isPreviewingAndValidRuntimeNodeIdx = isPreviewing && ( runtimeNodeIdx != InvalidIndex );

        //-------------------------------------------------------------------------
        // Draw Pose Node
        //-------------------------------------------------------------------------

        if ( GetValueType() == GraphValueType::Pose )
        {
            if ( isPreviewingAndValidRuntimeNodeIdx && pGraphNodeContext->IsNodeActive( runtimeNodeIdx ) )
            {
                PoseNodeDebugInfo const debugInfo = pGraphNodeContext->GetPoseNodeDebugInfo( runtimeNodeIdx );
                DrawPoseNodeDebugInfo( ctx, GetSize().x, debugInfo );
            }
            else
            {
                DrawEmptyPoseNodeDebugInfo( ctx, GetSize().x );
            }

            ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 4 );
        }

        //-------------------------------------------------------------------------
        // Draw Value Node
        //-------------------------------------------------------------------------

        else
        {
            if ( isPreviewingAndValidRuntimeNodeIdx && pGraphNodeContext->IsNodeActive( runtimeNodeIdx ) )
            {
                if ( HasOutputPin() )
                {
                    GraphValueType const valueType = GetValueType();
                    switch ( valueType )
                    {
                        case GraphValueType::Bool:
                        {
                            auto const value = pGraphNodeContext->GetRuntimeNodeValue<bool>( runtimeNodeIdx );
                            ImGui::Text( value ? "True" : "False" );
                        }
                        break;

                        case GraphValueType::ID:
                        {
                            auto const value = pGraphNodeContext->GetRuntimeNodeValue<StringID>( runtimeNodeIdx );
                            if ( value.IsValid() )
                            {
                                ImGui::Text( value.c_str() );
                            }
                            else
                            {
                                ImGui::Text( "Invalid" );
                            }
                        }
                        break;

                        case GraphValueType::Int:
                        {
                            auto const value = pGraphNodeContext->GetRuntimeNodeValue<int32_t>( runtimeNodeIdx );
                            ImGui::Text( "%d", value );
                        }
                        break;

                        case GraphValueType::Float:
                        {
                            auto const value = pGraphNodeContext->GetRuntimeNodeValue<float>( runtimeNodeIdx );
                            ImGui::Text( "%.3f", value );
                        }
                        break;

                        case GraphValueType::Vector:
                        {
                            auto const value = pGraphNodeContext->GetRuntimeNodeValue<Vector>( runtimeNodeIdx );
                            ImGuiX::DisplayVector4( value );
                        }
                        break;

                        case GraphValueType::Target:
                        {
                            auto const value = pGraphNodeContext->GetRuntimeNodeValue<Target>( runtimeNodeIdx );
                            if ( value.IsBoneTarget() )
                            {
                                if ( value.GetBoneID().IsValid() )
                                {
                                    ImGui::Text( value.GetBoneID().c_str() );
                                }
                                else
                                {
                                    ImGui::Text( "Invalid" );
                                }
                            }
                            else
                            {
                                ImGui::Text( "Target TODO" );
                            }
                        }
                        break;

                        case GraphValueType::BoneMask:
                        case GraphValueType::Pose:
                        default:
                        break;
                    }
                }
            }
        }

        //-------------------------------------------------------------------------

        DrawInfoText( ctx );
    }

    bool EditorGraphNode::IsActive( VisualGraph::DrawContext const& ctx ) const
    {
        auto pGraphNodeContext = reinterpret_cast<GraphNodeContext*>( ctx.m_pUserContext );
        if ( pGraphNodeContext->HasDebugData() )
        {
            // Some nodes dont have runtime representations
            auto const runtimeNodeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( GetID() );
            if ( runtimeNodeIdx != InvalidIndex )
            {
                return pGraphNodeContext->IsNodeActive( runtimeNodeIdx );
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    void DataSlotEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        m_name = GetUniqueSlotName( GetDefaultSlotName() );
    }

    bool DataSlotEditorNode::AreSlotValuesValid() const
    {
        if ( m_defaultResourceID.GetResourceTypeID() != GetSlotResourceTypeID() )
        {
            return false;
        }

        for ( auto const& variation : m_overrides )
        {
            if ( !variation.m_variationID.IsValid() )
            {
                return false;
            }

            if ( variation.m_resourceID.GetResourceTypeID() != GetSlotResourceTypeID() )
            {
                return false;
            }
        }

        return true;
    }

    ResourceID DataSlotEditorNode::GetResourceID( VariationHierarchy const& variationHierarchy, StringID variationID ) const
    {
        EE_ASSERT( variationHierarchy.IsValidVariation( variationID ) );

        if ( variationID == GraphVariation::DefaultVariationID )
        {
            return m_defaultResourceID;
        }

        //-------------------------------------------------------------------------

        ResourceID resourceID;

        auto TryGetResourceID = [this, &resourceID] ( StringID variationID )
        {
            if ( variationID == GraphVariation::DefaultVariationID )
            {
                resourceID = m_defaultResourceID;
                return true;
            }

            for ( auto const& variation : m_overrides )
            {
                if ( variation.m_variationID == variationID )
                {
                    resourceID = variation.m_resourceID;
                    return true;
                }
            }

            return false;
        };

        //-------------------------------------------------------------------------

        // Try get the resource ID for this variation
        if ( TryGetResourceID( variationID ) )
        {
            return resourceID;
        }

        // Go up the hierarchy and return the first if a override exists
        StringID parentVariationID = variationHierarchy.GetParentVariationID( variationID );
        while ( parentVariationID.IsValid() )
        {
            if ( TryGetResourceID( parentVariationID ) )
            {
                break;
            }

            parentVariationID = variationHierarchy.GetParentVariationID( parentVariationID );
        }

        return resourceID;
    }

    ResourceID* DataSlotEditorNode::GetOverrideValueForVariation( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() );

        if ( variationID == GraphVariation::DefaultVariationID )
        {
            return &m_defaultResourceID;
        }

        for ( auto& variation : m_overrides )
        {
            if ( variation.m_variationID == variationID )
            {
                return &variation.m_resourceID;
            }
        }

        return nullptr;
    }

    void DataSlotEditorNode::SetOverrideValueForVariation( StringID variationID, ResourceID const& resourceID )
    {
        EE_ASSERT( variationID.IsValid() );
        EE_ASSERT( !resourceID.IsValid() || resourceID.GetResourceTypeID() == GetSlotResourceTypeID() );

        if ( variationID == GraphVariation::DefaultVariationID )
        {
            m_defaultResourceID = resourceID;
            return;
        }

        //-------------------------------------------------------------------------

        EE_ASSERT( HasOverrideForVariation( variationID ) );

        for ( auto& variation : m_overrides )
        {
            if ( variation.m_variationID == variationID )
            {
                variation.m_resourceID = resourceID;
                return;
            }
        }

        EE_UNREACHABLE_CODE();
    }

    void DataSlotEditorNode::CreateOverride( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() && variationID != GraphVariation::DefaultVariationID );
        EE_ASSERT( !HasOverrideForVariation( variationID ) );

        auto& createdOverride = m_overrides.emplace_back();
        createdOverride.m_variationID = variationID;
    }

    void DataSlotEditorNode::RenameOverride( StringID oldVariationID, StringID newVariationID )
    {
        EE_ASSERT( oldVariationID.IsValid() && newVariationID.IsValid() );
        EE_ASSERT( oldVariationID != GraphVariation::DefaultVariationID && newVariationID != GraphVariation::DefaultVariationID );

        for ( auto& overrideValue : m_overrides )
        {
            if ( overrideValue.m_variationID == oldVariationID )
            {
                overrideValue.m_variationID = newVariationID;
            }
        }
    }

    void DataSlotEditorNode::RemoveOverride( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() && variationID != GraphVariation::DefaultVariationID );

        for ( auto iter = m_overrides.begin(); iter != m_overrides.end(); ++iter )
        {
            if ( iter->m_variationID == variationID )
            {
                m_overrides.erase_unsorted( iter );
                return;
            }
        }

        EE_UNREACHABLE_CODE();
    }

    void DataSlotEditorNode::DrawExtraControls( VisualGraph::DrawContext const& ctx )
    {
        // Draw separator
        //-------------------------------------------------------------------------

        float const spacerWidth = Math::Max( GetSize().x, 40.0f );
        ImVec2 originalCursorPos = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton( "S1", ImVec2( spacerWidth, 10 ) );
        ctx.m_pDrawList->AddLine( originalCursorPos + ImVec2( 0, 4 ), originalCursorPos + ImVec2( GetSize().x, 4 ), ImColor( ImGuiX::Style::s_colorTextDisabled ) );

        //-------------------------------------------------------------------------

        auto pDebugContext = reinterpret_cast<GraphNodeContext*>( ctx.m_pUserContext );
        auto pResource = GetOverrideValueForVariation( pDebugContext->m_currentVariationID );
        if ( pResource == nullptr || !pResource->IsValid() )
        {
            ImGui::Text( EE_ICON_CUBE_OUTLINE "None Set!" );
        }
        else
        {
            ImGui::Text( EE_ICON_CUBE" %s", pResource->c_str() + 7);
        }

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 4 );

        //-------------------------------------------------------------------------

        originalCursorPos = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton( "S2", ImVec2( spacerWidth, 10 ) );
        ctx.m_pDrawList->AddLine( originalCursorPos + ImVec2( 0, 4 ), originalCursorPos + ImVec2( GetSize().x, 4 ), ImColor( ImGuiX::Style::s_colorTextDisabled ) );

        //-------------------------------------------------------------------------

        EditorGraphNode::DrawExtraControls( ctx );
    }

    void DataSlotEditorNode::SetName( String const& newName )
    {
        EE_ASSERT( IsRenamable() );
        m_name = GetUniqueSlotName( newName );
    }

    String DataSlotEditorNode::GetUniqueSlotName( String const& desiredName )
    {
        // Ensure that the slot name is unique within the same graph
        int32_t cnt = 0;
        String uniqueName = desiredName;
        auto const dataSlotNodes = GetParentGraph()->FindAllNodesOfType<DataSlotEditorNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
        for ( int32_t i = 0; i < (int32_t) dataSlotNodes.size(); i++ )
        {
            if ( dataSlotNodes[i] == this )
            {
                continue;
            }

            if ( dataSlotNodes[i]->GetName() == uniqueName )
            {
                uniqueName = String( String::CtorSprintf(), "%s_%d", desiredName.c_str(), cnt );
                cnt++;
                i = 0;
            }
        }

        return uniqueName;
    }

    void DataSlotEditorNode::PostPaste()
    {
        EditorGraphNode::PostPaste();
        m_name = GetUniqueSlotName( m_name );
    }

    #if EE_DEVELOPMENT_TOOLS
    void DataSlotEditorNode::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        EditorGraphNode::PostPropertyEdit( pPropertyEdited );
        m_name = GetUniqueSlotName( m_name );
    }
    #endif

    //-------------------------------------------------------------------------

    ResultEditorNode::ResultEditorNode( GraphValueType valueType )
        : m_valueType( valueType )
    {}

    void ResultEditorNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        EditorGraphNode::Initialize( pParent );
        CreateInputPin( "Out", m_valueType );
    }

    int16_t ResultEditorNode::Compile( GraphCompilationContext& context ) const
    {
        // Get connected node and compile it
        auto pConnectedNode = GetConnectedInputNode<EditorGraphNode>( 0 );
        if ( pConnectedNode != nullptr )
        {
            return pConnectedNode->Compile( context );
        }

        context.LogError( this, "Result node not connected" );
        return InvalidIndex;
    }

    //-------------------------------------------------------------------------

    void ParameterReferenceEditorNode::RefreshParameterReferences( VisualGraph::BaseGraph* pRootGraph )
    {
        EE_ASSERT( pRootGraph != nullptr && pRootGraph->IsRootGraph() );
        auto const controlParameters = pRootGraph->FindAllNodesOfType<ControlParameterEditorNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
        auto const virtualParameters = pRootGraph->FindAllNodesOfType<VirtualParameterEditorNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Exact );

        //-------------------------------------------------------------------------

        TInlineVector<ParameterReferenceEditorNode*, 10> invalidReferenceNodes;
        auto parameterReferenceNodes = pRootGraph->FindAllNodesOfType<ParameterReferenceEditorNode>( VisualGraph::SearchMode::Recursive, VisualGraph::SearchTypeMatch::Exact );
        for ( auto pReferenceNode : parameterReferenceNodes )
        {
            EditorGraphNode* pFoundParameterNode = nullptr;

            for ( auto pParameter : controlParameters )
            {
                if ( pParameter->GetID() == pReferenceNode->GetReferencedParameterID() )
                {
                    pFoundParameterNode = pParameter;
                    break;
                }
            }

            if ( pFoundParameterNode == nullptr )
            {
                for ( auto pParameter : virtualParameters )
                {
                    if ( pParameter->GetID() == pReferenceNode->GetReferencedParameterID() )
                    {
                        pFoundParameterNode = pParameter;
                        break;
                    }
                }
            }

            if ( pFoundParameterNode != nullptr && ( pReferenceNode->GetParameterValueType() == pFoundParameterNode->GetValueType() ) )
            {
                pReferenceNode->m_pParameter = pFoundParameterNode;
            }
            else // flag invalid references nodes for destruction
            {
                invalidReferenceNodes.emplace_back( pReferenceNode );
            }
        }

        // Destroy any invalid reference nodes
        //-------------------------------------------------------------------------

        if ( !invalidReferenceNodes.empty() )
        {
            VisualGraph::ScopedGraphModification const sgm( pRootGraph );
            for ( auto pInvalidNode : invalidReferenceNodes )
            {
                pInvalidNode->Destroy();
            }
        }
    }

    //-------------------------------------------------------------------------

    ControlParameterEditorNode::ControlParameterEditorNode( String const& name )
        : m_name( name )
    {
        EE_ASSERT( !m_name.empty() );
    }

    int16_t ControlParameterEditorNode::Compile( GraphCompilationContext& context ) const
    {
        switch ( GetValueType() )
        {
            case GraphValueType::Bool:
            {
                GraphNodes::ControlParameterBoolNode::Settings* pSettings = nullptr;
                context.GetSettings<GraphNodes::ControlParameterBoolNode>( this, pSettings );
                return pSettings->m_nodeIdx;
            }
            break;

            case GraphValueType::ID:
            {
                GraphNodes::ControlParameterIDNode::Settings* pSettings = nullptr;
                context.GetSettings<GraphNodes::ControlParameterIDNode>( this, pSettings );
                return pSettings->m_nodeIdx;
            }
            break;

            case GraphValueType::Int:
            {
                GraphNodes::ControlParameterIntNode::Settings* pSettings = nullptr;
                context.GetSettings<GraphNodes::ControlParameterIntNode>( this, pSettings );
                return pSettings->m_nodeIdx;
            }
            break;

            case GraphValueType::Float:
            {
                GraphNodes::ControlParameterFloatNode::Settings* pSettings = nullptr;
                context.GetSettings<GraphNodes::ControlParameterFloatNode>( this, pSettings );
                return pSettings->m_nodeIdx;
            }
            break;

            case GraphValueType::Vector:
            {
                GraphNodes::ControlParameterVectorNode::Settings* pSettings = nullptr;
                context.GetSettings<GraphNodes::ControlParameterVectorNode>( this, pSettings );
                return pSettings->m_nodeIdx;
            }
            break;

            case GraphValueType::Target:
            {
                GraphNodes::ControlParameterTargetNode::Settings* pSettings = nullptr;
                context.GetSettings<GraphNodes::ControlParameterTargetNode>( this, pSettings );
                return pSettings->m_nodeIdx;
            }
            break;
        }

        EE_UNREACHABLE_CODE();
        return InvalidIndex;
    }

    void ControlParameterEditorNode::Rename( String const& name, String const& category )
    {
        VisualGraph::ScopedNodeModification snm( this );
        m_name = name;
        m_parameterCategory = category;
    }

    //-------------------------------------------------------------------------

    VirtualParameterEditorNode::VirtualParameterEditorNode( String const& name, GraphValueType type )
        : m_name( name )
        , m_type( type )
    {
        EE_ASSERT( !m_name.empty() );
    }

    void VirtualParameterEditorNode::Initialize( VisualGraph::BaseGraph* pParentGraph )
    {
        EditorGraphNode::Initialize( pParentGraph );

        CreateOutputPin( "Value", m_type, true );

        auto pParameterGraph = EE::New<FlowGraph>( GraphType::ValueTree );
        pParameterGraph->CreateNode<ResultEditorNode>( m_type );
        SetChildGraph( pParameterGraph );
    }

    int16_t VirtualParameterEditorNode::Compile( GraphCompilationContext& context ) const
    {
        auto const resultNodes = GetChildGraph()->FindAllNodesOfType<ResultEditorNode>();
        EE_ASSERT( resultNodes.size() == 1 );

        auto pConnectedNode = resultNodes[0]->GetConnectedInputNode<EditorGraphNode>( 0 );
        if ( pConnectedNode == nullptr )
        {
            context.LogError( this, "Empty virtual parameter detected!" );
            return InvalidIndex;
        }

        return resultNodes[0]->GetConnectedInputNode<EditorGraphNode>( 0 )->Compile( context );
    }

    void VirtualParameterEditorNode::Rename( String const& name, String const& category )
    {
        VisualGraph::ScopedNodeModification snm( this );
        m_name = name;
        m_parameterCategory = category;
    }

    //-------------------------------------------------------------------------

    ParameterReferenceEditorNode::ParameterReferenceEditorNode( VirtualParameterEditorNode* pParameter )
        : m_pParameter( pParameter )
    {
        EE_ASSERT( pParameter != nullptr );
        m_parameterUUID = pParameter->GetID();
        m_parameterValueType = pParameter->GetValueType();
    }

    ParameterReferenceEditorNode::ParameterReferenceEditorNode( ControlParameterEditorNode* pParameter )
        : m_pParameter( pParameter )
    {
        EE_ASSERT( pParameter != nullptr );
        m_parameterUUID = pParameter->GetID();
        m_parameterValueType = pParameter->GetValueType();
    }

    void ParameterReferenceEditorNode::Initialize( VisualGraph::BaseGraph* pParentGraph )
    {
        EditorGraphNode::Initialize( pParentGraph );
        CreateOutputPin( "Value", m_pParameter->GetValueType(), true );
    }

    int16_t ParameterReferenceEditorNode::Compile( GraphCompilationContext& context ) const
    {
        return m_pParameter->Compile( context );
    }
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    FlowGraph::FlowGraph( GraphType type )
        : m_type( type )
    {}

    void FlowGraph::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue )
    {
        VisualGraph::FlowGraph::SerializeCustom( typeRegistry, graphObjectValue );
        m_type = (GraphType) graphObjectValue["GraphType"].GetUint();
    }

    void FlowGraph::SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const
    {
        VisualGraph::FlowGraph::SerializeCustom( typeRegistry, writer );

        writer.Key( "GraphType" );
        writer.Uint( (uint8_t) m_type );
    }

    void FlowGraph::PostPasteNodes( TInlineVector<VisualGraph::BaseNode*, 20> const& pastedNodes )
    {
        VisualGraph::FlowGraph::PostPasteNodes( pastedNodes );
        GraphNodes::ParameterReferenceEditorNode::RefreshParameterReferences( GetRootGraph() );
    }
}