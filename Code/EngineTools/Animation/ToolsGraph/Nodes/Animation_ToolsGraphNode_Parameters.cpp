#include "Animation_ToolsGraphNode_Parameters.h"
#include "Animation_ToolsGraphNode_Result.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Parameters.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    ControlParameterToolsNode::ControlParameterToolsNode( String const& name )
        : m_name( name )
    {
        EE_ASSERT( !m_name.empty() );
    }

    int16_t ControlParameterToolsNode::Compile( GraphCompilationContext& context ) const
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

    void ControlParameterToolsNode::Rename( String const& name, String const& category )
    {
        VisualGraph::ScopedNodeModification snm( this );
        m_name = name;
        m_parameterCategory = category;
    }

    //-------------------------------------------------------------------------

    VirtualParameterToolsNode::VirtualParameterToolsNode( String const& name, GraphValueType type )
        : m_name( name )
        , m_type( type )
    {
        EE_ASSERT( !m_name.empty() );
    }

    void VirtualParameterToolsNode::Initialize( VisualGraph::BaseGraph* pParentGraph )
    {
        FlowToolsNode::Initialize( pParentGraph );

        CreateOutputPin( "Value", m_type, true );

        auto pParameterGraph = EE::New<FlowGraph>( GraphType::ValueTree );
        pParameterGraph->CreateNode<ResultToolsNode>( m_type );
        SetChildGraph( pParameterGraph );
    }

    int16_t VirtualParameterToolsNode::Compile( GraphCompilationContext& context ) const
    {
        auto const resultNodes = GetChildGraph()->FindAllNodesOfType<ResultToolsNode>();
        EE_ASSERT( resultNodes.size() == 1 );

        auto pConnectedNode = resultNodes[0]->GetConnectedInputNode<FlowToolsNode>( 0 );
        if ( pConnectedNode == nullptr )
        {
            context.LogError( this, "Empty virtual parameter detected!" );
            return InvalidIndex;
        }

        return resultNodes[0]->GetConnectedInputNode<FlowToolsNode>( 0 )->Compile( context );
    }

    void VirtualParameterToolsNode::Rename( String const& name, String const& category )
    {
        VisualGraph::ScopedNodeModification snm( this );
        m_name = name;
        m_parameterCategory = category;
    }

    //-------------------------------------------------------------------------

    ParameterReferenceToolsNode::ParameterReferenceToolsNode( VirtualParameterToolsNode* pParameter )
        : m_pParameter( pParameter )
    {
        EE_ASSERT( pParameter != nullptr );
        m_parameterUUID = pParameter->GetID();
        m_parameterValueType = pParameter->GetValueType();
    }

    ParameterReferenceToolsNode::ParameterReferenceToolsNode( ControlParameterToolsNode* pParameter )
        : m_pParameter( pParameter )
    {
        EE_ASSERT( pParameter != nullptr );
        m_parameterUUID = pParameter->GetID();
        m_parameterValueType = pParameter->GetValueType();
    }

    void ParameterReferenceToolsNode::Initialize( VisualGraph::BaseGraph* pParentGraph )
    {
        FlowToolsNode::Initialize( pParentGraph );
        CreateOutputPin( "Value", m_pParameter->GetValueType(), true );
    }

    int16_t ParameterReferenceToolsNode::Compile( GraphCompilationContext& context ) const
    {
        return m_pParameter->Compile( context );
    }

    void ParameterReferenceToolsNode::GloballyRefreshParameterReferences( VisualGraph::BaseGraph* pRootGraph )
    {
        EE_ASSERT( pRootGraph != nullptr && pRootGraph->IsRootGraph() );
        auto const controlParameters = pRootGraph->FindAllNodesOfType<ControlParameterToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
        auto const virtualParameters = pRootGraph->FindAllNodesOfType<VirtualParameterToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Exact );

        //-------------------------------------------------------------------------

        TInlineVector<ParameterReferenceToolsNode*, 10> invalidReferenceNodes;
        auto parameterReferenceNodes = pRootGraph->FindAllNodesOfType<ParameterReferenceToolsNode>( VisualGraph::SearchMode::Recursive, VisualGraph::SearchTypeMatch::Exact );
        for ( auto pReferenceNode : parameterReferenceNodes )
        {
            FlowToolsNode* pFoundParameterNode = nullptr;

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

    void BoolControlParameterToolsNode::Initialize( VisualGraph::BaseGraph* pParentGraph )
    {
        FlowToolsNode::Initialize( pParentGraph );
        CreateOutputPin( "Value", GraphValueType::Bool, true );
    }

    void FloatControlParameterToolsNode::Initialize( VisualGraph::BaseGraph* pParentGraph )
    {
        FlowToolsNode::Initialize( pParentGraph );
        CreateOutputPin( "Value", GraphValueType::Float, true );
    }

    void IntControlParameterToolsNode::Initialize( VisualGraph::BaseGraph* pParentGraph )
    {
        FlowToolsNode::Initialize( pParentGraph );
        CreateOutputPin( "Value", GraphValueType::Int, true );
    }

    void IDControlParameterToolsNode::Initialize( VisualGraph::BaseGraph* pParentGraph )
    {
        FlowToolsNode::Initialize( pParentGraph );
        CreateOutputPin( "Value", GraphValueType::ID, true );
    }

    void VectorControlParameterToolsNode::Initialize( VisualGraph::BaseGraph* pParentGraph )
    {
        FlowToolsNode::Initialize( pParentGraph );
        CreateOutputPin( "Value", GraphValueType::Vector, true );
    }

    void TargetControlParameterToolsNode::Initialize( VisualGraph::BaseGraph* pParentGraph )
    {
        FlowToolsNode::Initialize( pParentGraph );
        CreateOutputPin( "Value", GraphValueType::Target, true );
    }

    void TargetControlParameterToolsNode::SetPreviewTargetTransform( Transform const& transform )
    {
        VisualGraph::ScopedNodeModification snm( this );
        m_previewStartTransform = transform;
    }
}