#include "Animation_ToolsGraphNode_IDs.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    IDComparisonToolsNode::IDComparisonToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Bool, true );
        CreateInputPin( "ID", GraphValueType::ID );

        m_IDs.emplace_back();
    }

    int16_t IDComparisonToolsNode::Compile( GraphCompilationContext& context ) const
    {
        IDComparisonNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<IDComparisonNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inputValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pDefinition->m_comparison = m_comparison;
            pDefinition->m_comparisionIDs.clear();
            pDefinition->m_comparisionIDs.insert( pDefinition->m_comparisionIDs.begin(), m_IDs.begin(), m_IDs.end() );
        }
        return pDefinition->m_nodeIdx;
    }

    void IDComparisonToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        if ( m_comparison == IDComparisonNode::Comparison::Matches )
        {
            ImGui::Text( "One of these:" );
        }
        else
        {
            ImGui::Text( "NOT any of these:" );
        }

        for ( auto i = 0; i < m_IDs.size(); i++ )
        {
            if ( m_IDs[i].IsValid() )
            {
                ImGui::BulletText( m_IDs[i].c_str() );
            }
            else
            {
                ImGui::BulletText( "INVALID ID" );
            }
        }
    }

    void IDComparisonToolsNode::GetLogicAndEventIDs( TVector<StringID>& outIDs ) const
    {
        for ( auto ID : m_IDs )
        {
            outIDs.emplace_back( ID );
        }
    }

    void IDComparisonToolsNode::RenameLogicAndEventIDs( StringID oldID, StringID newID )
    {
        bool foundMatch = false;
        for ( auto const ID : m_IDs )
        {
            if ( ID == oldID )
            {
                foundMatch = true;
                break;
            }
        }

        if ( foundMatch )
        {
            NodeGraph::ScopedNodeModification snm( this );
            for ( auto& ID : m_IDs )
            {
                if ( ID == oldID )
                {
                    ID = newID;
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    VariationIDComparisonToolsNode::VariationIDComparisonToolsNode()
        : VariationDataToolsNode()
    {
        m_defaultVariationData.CreateInstance( GetVariationDataTypeInfo() );
        m_defaultVariationData.GetAs<Data>()->m_IDs.emplace_back();

        CreateOutputPin( "Result", GraphValueType::Bool, true );
        CreateInputPin( "ID", GraphValueType::ID );
    }

    int16_t VariationIDComparisonToolsNode::Compile( GraphCompilationContext& context ) const
    {
        IDComparisonNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<IDComparisonNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inputValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            auto pData = GetResolvedVariationDataAs<Data>( context.GetVariationHierarchy(), context.GetVariationID() );

            pDefinition->m_comparison = m_comparison;
            pDefinition->m_comparisionIDs.clear();
            pDefinition->m_comparisionIDs.insert( pDefinition->m_comparisionIDs.begin(), pData->m_IDs.begin(), pData->m_IDs.end() );
        }
        return pDefinition->m_nodeIdx;
    }

    void VariationIDComparisonToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        if ( m_comparison == IDComparisonNode::Comparison::Matches )
        {
            ImGui::Text( "One of these:" );
        }
        else
        {
            ImGui::Text( "NOT any of these:" );
        }

        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        auto pData = GetResolvedVariationDataAs<Data>( *pGraphNodeContext->m_pVariationHierarchy, pGraphNodeContext->m_selectedVariationID );

        for ( auto i = 0; i < pData->m_IDs.size(); i++ )
        {
            if ( pData->m_IDs[i].IsValid() )
            {
                ImGui::BulletText( pData->m_IDs[i].c_str() );
            }
            else
            {
                ImGui::BulletText( "INVALID ID" );
            }
        }
    }

    void VariationIDComparisonToolsNode::GetLogicAndEventIDs( TVector<StringID>& outIDs ) const
    {
        TFunction<void( Data const* )> AddValidID = [&]( Data const *pVariationData )
        {
            for ( StringID const &ID : pVariationData->m_IDs )
            {
                if ( ID.IsValid() )
                {
                    outIDs.emplace_back( ID );
                }
            }
        };

        ForEachVariationData( AddValidID );
    }

    void VariationIDComparisonToolsNode::RenameLogicAndEventIDs( StringID oldID, StringID newID )
    {
        NodeGraph::ScopedNodeModification snm( this );

        TFunction<void( Data* )> RenameValidID = [&] ( Data *pVariationData )
        {
            for ( StringID &ID : pVariationData->m_IDs )
            {
                if ( ID == oldID )
                {
                    ID = newID;
                }
            }
        };

        ForEachVariationData( RenameValidID );
    }

    //-------------------------------------------------------------------------

    IDToFloatToolsNode::IDToFloatToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::Float, true );
        CreateInputPin( "ID", GraphValueType::ID );
    }

    int16_t IDToFloatToolsNode::Compile( GraphCompilationContext& context ) const
    {
        IDToFloatNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<IDToFloatNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            if ( m_mappings.empty() )
            {
                context.LogError( this, "No Mappings Specified For ID To Float Node" );
                return InvalidIndex;
            }

            if ( !ValidateMappings() )
            {
                context.LogError( this, "Invalid Mappings Specified For ID To Float Node" );
                return InvalidIndex;
            }

            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_inputValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            //-------------------------------------------------------------------------

            pDefinition->m_defaultValue = m_defaultValue;

            for ( auto const& mapping : m_mappings )
            {
                pDefinition->m_IDs.emplace_back( mapping.m_ID );
                pDefinition->m_values.emplace_back( mapping.m_value );
            }

            EE_ASSERT( pDefinition->m_IDs.size() == pDefinition->m_values.size() );
        }
        return pDefinition->m_nodeIdx;
    }

    void IDToFloatToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        if ( !m_mappings.empty() && ValidateMappings() )
        {
            for ( auto const& mapping : m_mappings )
            {
                ImGui::Text( "%s = %.3f", mapping.m_ID.c_str(), mapping.m_value );
            }
        }
        else
        {
            ImGui::TextColored( Colors::Red.ToFloat4(), "Invalid Mappings" );
        }
    }

    bool IDToFloatToolsNode::ValidateMappings() const
    {
        for ( auto const& mapping : m_mappings )
        {
            if ( !mapping.m_ID.IsValid() )
            {
                return false;
            }
        }

        return true;
    }

    void IDToFloatToolsNode::GetLogicAndEventIDs( TVector<StringID>& outIDs ) const
    {
        for ( auto const& mapping : m_mappings )
        {
            outIDs.emplace_back( mapping.m_ID );
        }
    }

    void IDToFloatToolsNode::RenameLogicAndEventIDs( StringID oldID, StringID newID )
    {
        bool foundMatch = false;
        for ( auto const& mapping : m_mappings )
        {
            if ( mapping.m_ID == oldID )
            {
                foundMatch = true;
                break;
            }
        }

        if ( foundMatch )
        {
            NodeGraph::ScopedNodeModification snm( this );
            for ( auto& mapping : m_mappings )
            {
                if ( mapping.m_ID == oldID )
                {
                    mapping.m_ID = newID;
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    IDSwitchToolsNode::IDSwitchToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::ID, true );
        CreateInputPin( "Bool", GraphValueType::Bool );
        CreateInputPin( "If True", GraphValueType::ID );
        CreateInputPin( "If False", GraphValueType::ID );
    }

    int16_t IDSwitchToolsNode::Compile( GraphCompilationContext& context ) const
    {
        IDSwitchNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<IDSwitchNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            auto pInputNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_switchValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            pInputNode = GetConnectedInputNode<FlowToolsNode>( 1 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_trueValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            pInputNode = GetConnectedInputNode<FlowToolsNode>( 2 );
            if ( pInputNode != nullptr )
            {
                int16_t const compiledNodeIdx = pInputNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_falseValueNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected input pin!" );
                return InvalidIndex;
            }

            pDefinition->m_trueValue = m_true;
            pDefinition->m_falseValue = m_false;
        }
        return pDefinition->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    IDSelectorToolsNode::IDSelectorToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Result", GraphValueType::ID, true );
        CreateDynamicInputPin( "ID", GetPinTypeForValueType( GraphValueType::Bool ) );
        CreateDynamicInputPin( "ID", GetPinTypeForValueType( GraphValueType::Bool ) );
    }

    int16_t IDSelectorToolsNode::Compile( GraphCompilationContext &context ) const
    {
        IDSelectorNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<IDSelectorNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            int32_t const numOptions = GetNumInputPins();

            if ( numOptions == 0 )
            {
                context.LogError( this, "No options on dynamic float selector!" );
                return InvalidIndex;
            }

            for ( auto i = 0; i < numOptions; i++ )
            {
                auto pInputNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pInputNode != nullptr )
                {
                    int16_t const nCompiledNodeIdx = pInputNode->Compile( context );
                    if ( nCompiledNodeIdx != InvalidIndex )
                    {
                        pDefinition->m_conditionNodeIndices.emplace_back( nCompiledNodeIdx );
                        pDefinition->m_values.emplace_back( m_options[i] );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
                else
                {
                    context.LogError( this, "Disconnected input pin!" );
                    return InvalidIndex;
                }
            }
        }

        //-------------------------------------------------------------------------

        pDefinition->m_defaultValue = m_defaultID;

        return pDefinition->m_nodeIdx;
    }

    void IDSelectorToolsNode::OnDynamicPinCreation( UUID const& pinID )
    {
        m_options.emplace_back( GetNewDynamicInputPinName().c_str() );
        RefreshDynamicPins();
    }

    void IDSelectorToolsNode::PreDynamicPinDestruction( UUID const& pinID )
    {
        int32_t const pinToBeRemovedIdx = GetInputPinIndex( pinID );
        EE_ASSERT( pinToBeRemovedIdx != InvalidIndex );
        m_options.erase( m_options.begin() + pinToBeRemovedIdx );
    }

    void IDSelectorToolsNode::DrawInfoText( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        DrawInternalSeparator( ctx );

        ImGui::Text( "Default: %s", m_defaultID.IsValid() ? m_defaultID.c_str() : "" );
    }

    void IDSelectorToolsNode::RefreshDynamicPins()
    {
        for ( int32_t i = 0; i < GetNumInputPins(); i++ )
        {
            NodeGraph::Pin* pInputPin = GetInputPin( i );
            pInputPin->m_name = m_options[i].IsValid() ? m_options[i].c_str() : "";
        }
    }

    void IDSelectorToolsNode::GetLogicAndEventIDs( TVector<StringID>& outIDs ) const
    {
        outIDs.emplace_back( m_defaultID );

        for ( StringID option : m_options )
        {
            outIDs.emplace_back( option );
        }
    }

    void IDSelectorToolsNode::RenameLogicAndEventIDs( StringID oldID, StringID newID )
    {
        bool bFoundMatch = ( m_defaultID == oldID );
        if ( !bFoundMatch )
        {
            for ( StringID option : m_options )
            {
                if ( option == oldID )
                {
                    bFoundMatch = true;
                    break;
                }
            }
        }

        if ( bFoundMatch )
        {
            NodeGraph::ScopedNodeModification snm( this );

            if ( m_defaultID == oldID )
            {
                m_defaultID = newID;
            }

            for ( StringID option : m_options )
            {
                if ( option == oldID )
                {
                    option = newID;
                }
            }
        }
    }
}