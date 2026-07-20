#include "Animation_ToolsGraphNode_Selector.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_FlowGraph.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Selector.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    SelectorConditionToolsNode::SelectorConditionToolsNode()
        : ResultToolsNode()
    {}

    void SelectorConditionToolsNode::RefreshDynamicPins()
    {
        auto pParentSelectorNode = TryCast<SelectorBaseToolsNode>( GetParentNode() );
        if ( pParentSelectorNode == nullptr )
        {
            return;
        }

        int32_t const numInputPins = GetNumInputPins();
        for ( int32_t i = 0; i < numInputPins; i++ )
        {
            String const& label = pParentSelectorNode->GetOptionLabel( i );
            GetInputPin( i )->m_name.sprintf("%s", label.empty() ? "Option" : label.c_str() );
        }
    }

    //-------------------------------------------------------------------------

    SelectorBaseToolsNode::SelectorBaseToolsNode()
        : FlowToolsNode()
    {
        auto pConditionGraph = CreateSecondaryGraph<FlowGraph>( GraphType::ValueTree );
        pConditionGraph->CreateNode<SelectorConditionToolsNode>();

        //-------------------------------------------------------------------------

        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateDynamicInputPin( "Option", GetPinTypeForValueType( GraphValueType::Pose ) );
        CreateDynamicInputPin( "Option", GetPinTypeForValueType( GraphValueType::Pose ) );
    }

    void SelectorBaseToolsNode::OnDynamicPinCreation( UUID const& pinID )
    {
        auto pCreatedPin = GetInputPin( pinID );

        m_optionLabels.emplace_back( pCreatedPin->m_name );

        auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<SelectorConditionToolsNode>();
        EE_ASSERT( conditionNodes.size() == 1 );
        SelectorConditionToolsNode* pConditionNode = conditionNodes[0];
        pConditionNode->CreateDynamicInputPin( pCreatedPin->m_name.c_str(), GetPinTypeForValueType( GraphValueType::Bool ) );

        RefreshDynamicPins();
    }

    void SelectorBaseToolsNode::PreDynamicPinDestruction( UUID const& pinID )
    {
        int32_t const pinToBeRemovedIdx = GetInputPinIndex( pinID );
        EE_ASSERT( pinToBeRemovedIdx != InvalidIndex );

        m_optionLabels.erase( m_optionLabels.begin() + pinToBeRemovedIdx );

        auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<SelectorConditionToolsNode>();
        EE_ASSERT( conditionNodes.size() == 1 );
        SelectorConditionToolsNode* pConditionNode = conditionNodes[0];
        pConditionNode->DestroyDynamicInputPin( pConditionNode->GetInputPin( pinToBeRemovedIdx )->m_ID );
    }

    void SelectorBaseToolsNode::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        FlowToolsNode::PostPropertyEdit( pPropertyEdited );

        if ( pPropertyEdited->m_ID == StringID( "m_optionLabels" ) )
        {
            RefreshDynamicPins();
        }
    }

    void SelectorBaseToolsNode::RefreshDynamicPins()
    {
        int32_t firstDynamicPinIdx = InvalidIndex;
        int32_t const numInputPins = GetNumInputPins();
        for ( int32_t i = 0; i < numInputPins; i++ )
        {
            if ( GetInputPins()[i].IsDynamicPin() )
            {
                firstDynamicPinIdx = i;
                break;
            }
        }

        if ( firstDynamicPinIdx != InvalidIndex )
        {
            for ( int32_t i = firstDynamicPinIdx; i < numInputPins; i++ )
            {
                NodeGraph::Pin* pInputPin = GetInputPin( i );
                pInputPin->m_name.sprintf( "%s", m_optionLabels[i - firstDynamicPinIdx].empty() ? "Option" : m_optionLabels[i - firstDynamicPinIdx].c_str() );
            }
        }

        //-------------------------------------------------------------------------

        auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<SelectorConditionToolsNode>();
        EE_ASSERT( conditionNodes.size() == 1 );
        auto pConditionsNode = conditionNodes[0];
        pConditionsNode->RefreshDynamicPins();
    }

    //-------------------------------------------------------------------------

    int16_t SelectorToolsNode::Compile( GraphCompilationContext& context ) const
    {
        SelectorNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<SelectorNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            int32_t const numOptions = GetNumInputPins();

            auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<SelectorConditionToolsNode>();
            EE_ASSERT( conditionNodes.size() == 1 );
            auto pConditionsNode = conditionNodes[0];
            EE_ASSERT( pConditionsNode->GetNumInputPins() == numOptions );

            for ( auto i = 0; i < numOptions; i++ )
            {
                auto pOptionNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pOptionNode != nullptr )
                {
                    auto compiledNodeIdx = pOptionNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pDefinition->m_optionNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
                else
                {
                    context.LogError( this, "Disconnected option pin on selector node!" );
                    return InvalidIndex;
                }

                auto pConditionNode = pConditionsNode->GetConnectedInputNode<FlowToolsNode>( i );
                if ( pConditionNode != nullptr )
                {
                    auto compiledNodeIdx = pConditionNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pDefinition->m_conditionNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
                else
                {
                    context.LogError( pConditionsNode, "Disconnected condition pin on selector conditions node!" );
                    return InvalidIndex;
                }
            }

            EE_ASSERT( pDefinition->m_optionNodeIndices.size() == pDefinition->m_conditionNodeIndices.size() );

            //-------------------------------------------------------------------------

            if ( pDefinition->m_optionNodeIndices.empty() )
            {
                context.LogError( this, "No inputs on selector" );
                return InvalidIndex;
            }
        }
        return pDefinition->m_nodeIdx;
    }

    //-------------------------------------------------------------------------

    int16_t AnimationClipSelectorToolsNode::Compile( GraphCompilationContext& context ) const
    {
        AnimationClipSelectorNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<AnimationClipSelectorNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            int32_t const numOptions = GetNumInputPins();

            auto conditionNodes = GetSecondaryGraph()->FindAllNodesOfType<SelectorConditionToolsNode>();
            EE_ASSERT( conditionNodes.size() == 1 );
            auto pConditionsNode = conditionNodes[0];
            EE_ASSERT( pConditionsNode->GetNumInputPins() == numOptions );

            for ( auto i = 0; i < numOptions; i++ )
            {
                auto pOptionNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pOptionNode != nullptr )
                {
                    auto compiledNodeIdx = pOptionNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pDefinition->m_optionNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
                else
                {
                    context.LogError( this, "Disconnected option pin on selector node!" );
                    return InvalidIndex;
                }

                auto pConditionNode = pConditionsNode->GetConnectedInputNode<FlowToolsNode>( i );
                if ( pConditionNode != nullptr )
                {
                    auto compiledNodeIdx = pConditionNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pDefinition->m_conditionNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
                else
                {
                    context.LogError( pConditionsNode, "Disconnected condition pin on selector condition node!" );
                    return InvalidIndex;
                }
            }

            EE_ASSERT( pDefinition->m_optionNodeIndices.size() == pDefinition->m_conditionNodeIndices.size() );

            //-------------------------------------------------------------------------

            if ( pDefinition->m_optionNodeIndices.empty() )
            {
                context.LogError( this, "No inputs on selector" );
                return InvalidIndex;
            }
        }
        return pDefinition->m_nodeIdx;
    }

    bool AnimationClipSelectorToolsNode::IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const
    {
        int32_t const pinIdx = GetInputPinIndex( inputPinID );
        if ( pinIdx >= 0 )
        {
            return Cast<FlowToolsNode>( pOutputPinNode )->IsAnimationClipReferenceNode();
        }

        return FlowToolsNode::IsValidConnection( inputPinID, pOutputPinNode, outputPinID );
    }

    //-------------------------------------------------------------------------

    ParameterizedSelectorToolsNode::ParameterizedSelectorToolsNode()
        : VariationDataToolsNode()
    {
        m_defaultVariationData.CreateInstance( GetVariationDataTypeInfo() );

        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Parameter", GraphValueType::Float );
        CreateDynamicInputPin( "Option", GetPinTypeForValueType( GraphValueType::Pose ) );
        CreateDynamicInputPin( "Option", GetPinTypeForValueType( GraphValueType::Pose ) );
    }

    int16_t ParameterizedSelectorToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ParameterizedSelectorNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<ParameterizedSelectorNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            int32_t const numInputPins = GetNumInputPins();
            if ( numInputPins == 1 )
            {
                context.LogError( this, "No options on selector node!" );
                return InvalidIndex;
            }

            pDefinition->m_ignoreInvalidOptions = m_ignoreInvalidOptions;

            // Compile Parameter
            //-------------------------------------------------------------------------

            auto pParameterNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pParameterNode != nullptr )
            {
                auto compiledNodeIdx = pParameterNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_parameterNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected parameter pin on selector node!" );
                return InvalidIndex;
            }

            // Compile Options
            //-------------------------------------------------------------------------

            for ( auto i = 1; i < numInputPins; i++ )
            {
                auto pOptionNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pOptionNode != nullptr )
                {
                    auto compiledNodeIdx = pOptionNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pDefinition->m_optionNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
                else
                {
                    context.LogError( this, "Disconnected option pin on selector node!" );
                    return InvalidIndex;
                }
            }

            // Compile weights
            //-------------------------------------------------------------------------

            auto pData = GetResolvedVariationDataAs<Data>( context.GetVariationHierarchy(), context.GetVariationID() );
            EE_ASSERT( pData->m_optionWeights.size() == m_optionLabels.size() );

            bool hasWeightsSet = false;
            for ( int32_t i = 1; i < pData->m_optionWeights.size(); i++ )
            {
                hasWeightsSet |= ( pData->m_optionWeights[i] != pData->m_optionWeights[0] );
            }

            if ( hasWeightsSet )
            {
                for ( uint8_t v : pData->m_optionWeights )
                {
                    pDefinition->m_optionWeights.emplace_back( v );
                }
            }

            pDefinition->m_hasWeightsSet = hasWeightsSet;

            //-------------------------------------------------------------------------

            if ( pDefinition->m_optionNodeIndices.empty() )
            {
                context.LogError( this, "No inputs on selector" );
                return InvalidIndex;
            }
        }
        return pDefinition->m_nodeIdx;
    }

    void ParameterizedSelectorToolsNode::OnDynamicPinCreation( UUID const& pinID )
    {
        m_optionLabels.emplace_back( GetInputPin( pinID )->m_name );

        TFunction<void( Data* )> Fn = [] ( Data *pData )
        {
            pData->m_optionWeights.emplace_back( uint8_t( 0 ) );
        };
        ForEachVariationData( Fn );

        RefreshDynamicPins();
    }

    void ParameterizedSelectorToolsNode::PreDynamicPinDestruction( UUID const& pinID )
    {
        int32_t const pinToBeRemovedIdx = GetInputPinIndex( pinID );
        EE_ASSERT( pinToBeRemovedIdx != InvalidIndex );
        m_optionLabels.erase( m_optionLabels.begin() + pinToBeRemovedIdx - 1 );

        TFunction<void( Data* )> Fn = [pinToBeRemovedIdx] ( Data *pData )
        {
            pData->m_optionWeights.erase( pData->m_optionWeights.begin() + pinToBeRemovedIdx - 1 );
        };

        ForEachVariationData( Fn );
    }

    void ParameterizedSelectorToolsNode::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        FlowToolsNode::PostPropertyEdit( pPropertyEdited );

        if ( pPropertyEdited->m_ID == StringID( "m_optionLabels" ) )
        {
            RefreshDynamicPins();
        }
    }

    void ParameterizedSelectorToolsNode::RefreshDynamicPins()
    {
        for ( int32_t i = 1; i < GetNumInputPins(); i++ )
        {
            NodeGraph::Pin* pInputPin = GetInputPin( i );
            pInputPin->m_name.sprintf( "%s", m_optionLabels[i - 1].empty() ? "Option" : m_optionLabels[i - 1].c_str() );
        }
    }

    void ParameterizedSelectorToolsNode::OnVariationOverrideCreated( VariationDataToolsNode::Data* pCreatedData )
    {
        auto pData = Cast<ParameterizedSelectorToolsNode::Data>( pCreatedData );
        pData->m_optionWeights.resize( m_optionLabels.size(), 0 );
    }

    //-------------------------------------------------------------------------

    ParameterizedAnimationClipSelectorToolsNode::ParameterizedAnimationClipSelectorToolsNode()
        : VariationDataToolsNode()
    {
        m_defaultVariationData.CreateInstance( GetVariationDataTypeInfo() );

        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Parameter", GraphValueType::Float );
        CreateDynamicInputPin( "Option", GetPinTypeForValueType( GraphValueType::Pose ) );
        CreateDynamicInputPin( "Option", GetPinTypeForValueType( GraphValueType::Pose ) );
    }

    int16_t ParameterizedAnimationClipSelectorToolsNode::Compile( GraphCompilationContext& context ) const
    {
        ParameterizedAnimationClipSelectorNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<ParameterizedAnimationClipSelectorNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            int32_t const numInputPins = GetNumInputPins();
            if ( numInputPins == 1 )
            {
                context.LogError( this, "No options on selector node!" );
                return InvalidIndex;
            }

            pDefinition->m_ignoreInvalidOptions = m_ignoreInvalidOptions;

            // Compile Parameter
            //-------------------------------------------------------------------------

            auto pParameterNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pParameterNode != nullptr )
            {
                auto compiledNodeIdx = pParameterNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_parameterNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected parameter pin on selector node!" );
                return InvalidIndex;
            }

            // Compile Options
            //-------------------------------------------------------------------------

            for ( auto i = 1; i < numInputPins; i++ )
            {
                auto pOptionNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pOptionNode != nullptr )
                {
                    auto compiledNodeIdx = pOptionNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pDefinition->m_optionNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
                else
                {
                    context.LogError( this, "Disconnected option pin on selector node!" );
                    return InvalidIndex;
                }
            }

            // Compile weights
            //-------------------------------------------------------------------------

            auto pData = GetResolvedVariationDataAs<Data>( context.GetVariationHierarchy(), context.GetVariationID() );
            EE_ASSERT( pData->m_optionWeights.size() == m_optionLabels.size() );

            bool hasWeightsSet = false;
            for ( int32_t i = 1; i < pData->m_optionWeights.size(); i++ )
            {
                hasWeightsSet |= ( pData->m_optionWeights[i] != pData->m_optionWeights[0] );
            }

            if ( hasWeightsSet )
            {
                for ( uint8_t v : pData->m_optionWeights )
                {
                    pDefinition->m_optionWeights.emplace_back( v );
                }
            }

            pDefinition->m_hasWeightsSet = hasWeightsSet;

            //-------------------------------------------------------------------------

            if ( pDefinition->m_optionNodeIndices.empty() )
            {
                context.LogError( this, "No inputs on selector" );
                return InvalidIndex;
            }
        }

        return pDefinition->m_nodeIdx;
    }

    void ParameterizedAnimationClipSelectorToolsNode::OnDynamicPinCreation( UUID const& pinID )
    {
        m_optionLabels.emplace_back( GetInputPin( pinID )->m_name );

        eastl::function<void( Data* )> Fn = [] ( Data *pData )
        {
            pData->m_optionWeights.emplace_back( uint8_t( 0 ) );
        };
        ForEachVariationData( Fn );

        RefreshDynamicPins();
    }

    void ParameterizedAnimationClipSelectorToolsNode::PreDynamicPinDestruction( UUID const& pinID )
    {
        int32_t const pinToBeRemovedIdx = GetInputPinIndex( pinID );
        EE_ASSERT( pinToBeRemovedIdx != InvalidIndex );
        m_optionLabels.erase( m_optionLabels.begin() + pinToBeRemovedIdx - 1 );

        eastl::function<void( Data* )> Fn = [pinToBeRemovedIdx] ( Data *pData )
        {
            pData->m_optionWeights.erase( pData->m_optionWeights.begin() + pinToBeRemovedIdx - 1 );
        };

        ForEachVariationData( Fn );
    }

    bool ParameterizedAnimationClipSelectorToolsNode::IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const
    {
        int32_t const pinIdx = GetInputPinIndex( inputPinID );
        if ( pinIdx >= 1 )
        {
            return Cast<FlowToolsNode>( pOutputPinNode )->IsAnimationClipReferenceNode();
        }

        return FlowToolsNode::IsValidConnection( inputPinID, pOutputPinNode, outputPinID );
    }

    void ParameterizedAnimationClipSelectorToolsNode::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        FlowToolsNode::PostPropertyEdit( pPropertyEdited );

        if ( pPropertyEdited->m_ID == StringID( "m_optionLabels" ) )
        {
            RefreshDynamicPins();
        }
    }

    void ParameterizedAnimationClipSelectorToolsNode::RefreshDynamicPins()
    {
        for ( int32_t i = 1; i < GetNumInputPins(); i++ )
        {
            NodeGraph::Pin* pInputPin = GetInputPin( i );
            pInputPin->m_name.sprintf( "%s", m_optionLabels[i - 1].empty() ? "Option" : m_optionLabels[i - 1].c_str() );
        }
    }

    void ParameterizedAnimationClipSelectorToolsNode::OnVariationOverrideCreated( VariationDataToolsNode::Data* pCreatedData )
    {
        auto pData = Cast<ParameterizedAnimationClipSelectorToolsNode::Data>( pCreatedData );
        pData->m_optionWeights.resize( m_optionLabels.size(), 0 );
    }

    //-------------------------------------------------------------------------

    IDBasedSelectorToolsNode::IDBasedSelectorToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Parameter", GraphValueType::Float );
        CreateInputPin( "Optional Fallback", GraphValueType::Pose );
        CreateDynamicInputPin( "Option", GetPinTypeForValueType( GraphValueType::Pose ) );
        CreateDynamicInputPin( "Option", GetPinTypeForValueType( GraphValueType::Pose ) );
    }

    int16_t IDBasedSelectorToolsNode::Compile( GraphCompilationContext& context ) const
    {
        IDBasedSelectorNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<IDBasedSelectorNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            int32_t const numInputPins = GetNumInputPins();
            if ( numInputPins == 1 )
            {
                context.LogError( this, "No options on selector node!" );
                return InvalidIndex;
            }

            // Check uniqueness and validity of IDs
            //-------------------------------------------------------------------------

            for ( int32_t i = 0; i < m_optionLabels.size(); i++ )
            {
                if ( m_optionLabels[i].empty() )
                {
                    context.LogError( this, "Invalid option (%d) for ID based selector!", i );
                    return InvalidIndex;
                }

                for ( int32_t j = i + 1; j < m_optionLabels.size(); j++ )
                {
                    if ( m_optionLabels[i].comparei( m_optionLabels[j] ) == 0 )
                    {
                        context.LogError( this, "duplicate options detected for ID based selector!" );
                        return InvalidIndex;
                    }
                }
            }

            for ( String const &optStr : m_optionLabels )
            {
                pDefinition->m_optionIDs.emplace_back( StringID( optStr ) );
            }

            pDefinition->m_ignoreInvalidOptions = m_ignoreInvalidOptions;

            // Compile Parameter
            //-------------------------------------------------------------------------

            auto pParameterNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pParameterNode != nullptr )
            {
                auto compiledNodeIdx = pParameterNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_parameterNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected parameter pin on selector node!" );
                return InvalidIndex;
            }

            // Compile Optional fallback
            //-------------------------------------------------------------------------

            auto pFallbackNode = GetConnectedInputNode<FlowToolsNode>( 1 );
            if ( pFallbackNode != nullptr )
            {
                auto compiledNodeIdx = pFallbackNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_fallbackNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            // Compile Options
            //-------------------------------------------------------------------------

            for ( auto i = 2; i < numInputPins; i++ )
            {
                auto pOptionNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pOptionNode != nullptr )
                {
                    auto compiledNodeIdx = pOptionNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pDefinition->m_optionNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
                else
                {
                    context.LogError( this, "Disconnected option pin on selector node!" );
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            if ( pDefinition->m_optionNodeIndices.empty() )
            {
                context.LogError( this, "No inputs on selector" );
                return InvalidIndex;
            }
        }

        if ( pDefinition->m_optionNodeIndices.size() != pDefinition->m_optionIDs.size() )
        {
            context.LogError( this, "Corrupt Node" );
            return InvalidIndex;
        }

        return pDefinition->m_nodeIdx;
    }

    void IDBasedSelectorToolsNode::OnDynamicPinCreation( UUID const& pinID )
    {
        m_optionLabels.emplace_back( GetInputPin( pinID )->m_name );
        RefreshDynamicPins();
    }

    void IDBasedSelectorToolsNode::PreDynamicPinDestruction( UUID const& pinID )
    {
        int32_t const pinToBeRemovedIdx = GetInputPinIndex( pinID );
        EE_ASSERT( pinToBeRemovedIdx != InvalidIndex );
        m_optionLabels.erase( m_optionLabels.begin() + pinToBeRemovedIdx - 2 );
    }

    void IDBasedSelectorToolsNode::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        FlowToolsNode::PostPropertyEdit( pPropertyEdited );

        if ( pPropertyEdited->m_ID == StringID( "m_optionLabels" ) )
        {
            RefreshDynamicPins();
        }
    }

    void IDBasedSelectorToolsNode::RefreshDynamicPins()
    {
        for ( int32_t i = 2; i < GetNumInputPins(); i++ )
        {
            NodeGraph::Pin* pInputPin = GetInputPin( i );
            pInputPin->m_name.sprintf( "%s", m_optionLabels[i - 2].empty() ? "Option" : m_optionLabels[i - 2].c_str() );
        }
    }

    //-------------------------------------------------------------------------

    IDBasedAnimationClipSelectorToolsNode::IDBasedAnimationClipSelectorToolsNode()
        : FlowToolsNode()
    {
        CreateOutputPin( "Pose", GraphValueType::Pose );
        CreateInputPin( "Parameter", GraphValueType::Float );
        CreateInputPin( "Optional Fallback", GraphValueType::Pose );
        CreateDynamicInputPin( "Option", GetPinTypeForValueType( GraphValueType::Pose ) );
        CreateDynamicInputPin( "Option", GetPinTypeForValueType( GraphValueType::Pose ) );
    }

    int16_t IDBasedAnimationClipSelectorToolsNode::Compile( GraphCompilationContext& context ) const
    {
        IDBasedAnimationClipSelectorNode::Definition* pDefinition = nullptr;
        NodeCompilationState const state = context.GetDefinition<IDBasedAnimationClipSelectorNode>( this, pDefinition );
        if ( state == NodeCompilationState::NeedCompilation )
        {
            int32_t const numInputPins = GetNumInputPins();
            if ( numInputPins == 1 )
            {
                context.LogError( this, "No options on selector node!" );
                return InvalidIndex;
            }

            // Check uniqueness and validity of IDs
            //-------------------------------------------------------------------------

            for ( int32_t i = 0; i < m_optionLabels.size(); i++ )
            {
                if ( m_optionLabels[i].empty() )
                {
                    context.LogError( this, "Invalid option (%d) for ID based selector!", i );
                    return InvalidIndex;
                }

                for ( int32_t j = i + 1; j < m_optionLabels.size(); j++ )
                {
                    if ( m_optionLabels[i].comparei( m_optionLabels[j] ) == 0 )
                    {
                        context.LogError( this, "duplicate options detected for ID based selector!" );
                        return InvalidIndex;
                    }
                }
            }

            for ( String const &optStr : m_optionLabels )
            {
                pDefinition->m_optionIDs.emplace_back( StringID( optStr ) );
            }

            pDefinition->m_ignoreInvalidOptions = m_ignoreInvalidOptions;

            // Compile Parameter
            //-------------------------------------------------------------------------

            auto pParameterNode = GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pParameterNode != nullptr )
            {
                auto compiledNodeIdx = pParameterNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_parameterNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }
            else
            {
                context.LogError( this, "Disconnected parameter pin on selector node!" );
                return InvalidIndex;
            }

            // Compile Optional fallback
            //-------------------------------------------------------------------------

            auto pFallbackNode = GetConnectedInputNode<FlowToolsNode>( 1 );
            if ( pFallbackNode != nullptr )
            {
                auto compiledNodeIdx = pFallbackNode->Compile( context );
                if ( compiledNodeIdx != InvalidIndex )
                {
                    pDefinition->m_fallbackNodeIdx = compiledNodeIdx;
                }
                else
                {
                    return InvalidIndex;
                }
            }

            // Compile Options
            //-------------------------------------------------------------------------

            for ( auto i = 2; i < numInputPins; i++ )
            {
                auto pOptionNode = GetConnectedInputNode<FlowToolsNode>( i );
                if ( pOptionNode != nullptr )
                {
                    auto compiledNodeIdx = pOptionNode->Compile( context );
                    if ( compiledNodeIdx != InvalidIndex )
                    {
                        pDefinition->m_optionNodeIndices.emplace_back( compiledNodeIdx );
                    }
                    else
                    {
                        return InvalidIndex;
                    }
                }
                else
                {
                    context.LogError( this, "Disconnected option pin on selector node!" );
                    return InvalidIndex;
                }
            }

            //-------------------------------------------------------------------------

            if ( pDefinition->m_optionNodeIndices.empty() )
            {
                context.LogError( this, "No inputs on selector" );
                return InvalidIndex;
            }
        }

        if ( pDefinition->m_optionNodeIndices.size() != pDefinition->m_optionIDs.size() )
        {
            context.LogError( this, "Corrupt Node" );
            return InvalidIndex;
        }

        return pDefinition->m_nodeIdx;
    }

    void IDBasedAnimationClipSelectorToolsNode::OnDynamicPinCreation( UUID const& pinID )
    {
        m_optionLabels.emplace_back( GetInputPin( pinID )->m_name );
        RefreshDynamicPins();
    }

    void IDBasedAnimationClipSelectorToolsNode::PreDynamicPinDestruction( UUID const& pinID )
    {
        int32_t const pinToBeRemovedIdx = GetInputPinIndex( pinID );
        EE_ASSERT( pinToBeRemovedIdx != InvalidIndex );
        m_optionLabels.erase( m_optionLabels.begin() + pinToBeRemovedIdx - 2 );
    }

    bool IDBasedAnimationClipSelectorToolsNode::IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const
    {
        int32_t const pinIdx = GetInputPinIndex( inputPinID );
        if ( pinIdx >= 2 )
        {
            return Cast<FlowToolsNode>( pOutputPinNode )->IsAnimationClipReferenceNode();
        }

        return FlowToolsNode::IsValidConnection( inputPinID, pOutputPinNode, outputPinID );
    }

    void IDBasedAnimationClipSelectorToolsNode::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        FlowToolsNode::PostPropertyEdit( pPropertyEdited );

        if ( pPropertyEdited->m_ID == StringID( "m_optionLabels" ) )
        {
            RefreshDynamicPins();
        }
    }

    void IDBasedAnimationClipSelectorToolsNode::RefreshDynamicPins()
    {
        for ( int32_t i = 2; i < GetNumInputPins(); i++ )
        {
            NodeGraph::Pin* pInputPin = GetInputPin( i );
            pInputPin->m_name.sprintf( "%s", m_optionLabels[i - 2].empty() ? "Option" : m_optionLabels[i - 2].c_str() );
        }
    }
}