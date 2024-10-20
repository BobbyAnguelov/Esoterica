#include "Animation_ToolsGraphNode_Parameters.h"
#include "Animation_ToolsGraphNode_Result.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_FlowGraph.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Parameters.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    ParameterBaseToolsNode::ParameterBaseToolsNode( String const& name, String const& groupName )
        : m_group( groupName )
    {
        EE_ASSERT( !name.empty() );
        m_name = name;
    }

    void ParameterBaseToolsNode::SetParameterGroup( String const& newGroupName )
    {
        NodeGraph::ScopedNodeModification snm( this );
        m_group = newGroupName;
    }

    //-------------------------------------------------------------------------

    int16_t ControlParameterToolsNode::Compile( GraphCompilationContext& context ) const
    {
        switch ( GetOutputValueType() )
        {
            case GraphValueType::Bool:
            {
                GraphNodes::ControlParameterBoolNode::Definition* pDefinition = nullptr;
                context.GetDefinition<GraphNodes::ControlParameterBoolNode>( this, pDefinition );
                return pDefinition->m_nodeIdx;
            }
            break;

            case GraphValueType::ID:
            {
                GraphNodes::ControlParameterIDNode::Definition* pDefinition = nullptr;
                context.GetDefinition<GraphNodes::ControlParameterIDNode>( this, pDefinition );
                return pDefinition->m_nodeIdx;
            }
            break;

            case GraphValueType::Float:
            {
                GraphNodes::ControlParameterFloatNode::Definition* pDefinition = nullptr;
                context.GetDefinition<GraphNodes::ControlParameterFloatNode>( this, pDefinition );
                return pDefinition->m_nodeIdx;
            }
            break;

            case GraphValueType::Vector:
            {
                GraphNodes::ControlParameterVectorNode::Definition* pDefinition = nullptr;
                context.GetDefinition<GraphNodes::ControlParameterVectorNode>( this, pDefinition );
                return pDefinition->m_nodeIdx;
            }
            break;

            case GraphValueType::Target:
            {
                GraphNodes::ControlParameterTargetNode::Definition* pDefinition = nullptr;
                context.GetDefinition<GraphNodes::ControlParameterTargetNode>( this, pDefinition );
                return pDefinition->m_nodeIdx;
            }
            break;

            default:
            EE_UNREACHABLE_CODE();
            return InvalidIndex;
            break;
        }
    }

    ControlParameterToolsNode* ControlParameterToolsNode::Create( FlowGraph* pRootGraph, GraphValueType type, String const& name, String const& groupName )
    {
        EE_ASSERT( pRootGraph != nullptr && pRootGraph->IsRootGraph() );
        EE_ASSERT( type != GraphValueType::Pose && type != GraphValueType::BoneMask && type != GraphValueType::Unknown );

        ControlParameterToolsNode* pParameter = nullptr;

        switch ( type )
        {
            case GraphValueType::Bool:
            pParameter = pRootGraph->CreateNode<GraphNodes::BoolControlParameterToolsNode>( name, groupName );
            break;

            case GraphValueType::ID:
            pParameter = pRootGraph->CreateNode<GraphNodes::IDControlParameterToolsNode>( name, groupName );
            break;

            case GraphValueType::Float:
            pParameter = pRootGraph->CreateNode<GraphNodes::FloatControlParameterToolsNode>( name, groupName );
            break;

            case GraphValueType::Vector:
            pParameter = pRootGraph->CreateNode<GraphNodes::VectorControlParameterToolsNode>( name, groupName );
            break;

            case GraphValueType::Target:
            pParameter = pRootGraph->CreateNode<GraphNodes::TargetControlParameterToolsNode>( name, groupName );
            break;

            default:
            {
                EE_UNREACHABLE_CODE();
            }
            break;
        }

        return pParameter;
    }

    //-------------------------------------------------------------------------

    BoolControlParameterToolsNode::BoolControlParameterToolsNode()
        : ControlParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::Bool, true );
    }

    BoolControlParameterToolsNode::BoolControlParameterToolsNode( String const& name, String const& groupName )
        : ControlParameterToolsNode( name, groupName )
    {
        CreateOutputPin( "Value", GraphValueType::Bool, true );
    }

    //-------------------------------------------------------------------------

    FloatControlParameterToolsNode::FloatControlParameterToolsNode()
        : ControlParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::Float, true );
    }

    FloatControlParameterToolsNode::FloatControlParameterToolsNode( String const& name, String const& groupName )
        : ControlParameterToolsNode( name, groupName )
    {
        CreateOutputPin( "Value", GraphValueType::Float, true );
    }

    //-------------------------------------------------------------------------

    IDControlParameterToolsNode::IDControlParameterToolsNode()
        : ControlParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::ID, true );
    }

    IDControlParameterToolsNode::IDControlParameterToolsNode( String const& name, String const& groupName )
        : ControlParameterToolsNode( name, groupName )
    {
        CreateOutputPin( "Value", GraphValueType::ID, true );
    }

    void IDControlParameterToolsNode::GetLogicAndEventIDs( TVector<StringID>& outIDs ) const
    {
        outIDs.emplace_back( m_previewStartValue );
        for ( auto ID : m_expectedValues )
        {
            if ( ID.IsValid() )
            {
                outIDs.emplace_back( ID );
            }
        }
    }

    void IDControlParameterToolsNode::RenameLogicAndEventIDs( StringID oldID, StringID newID )
    {
        bool foundMatch = ( m_previewStartValue == oldID );
        if ( !foundMatch )
        {
            for ( auto const& ID : m_expectedValues )
            {
                if ( ID == oldID )
                {
                    foundMatch = true;
                    break;
                }
            }
        }

        if ( foundMatch )
        {
            NodeGraph::ScopedNodeModification snm( this );

            if ( m_previewStartValue == oldID )
            {
                m_previewStartValue = newID;
            }

            for ( auto& ID : m_expectedValues )
            {
                if ( ID == oldID )
                {
                    ID = newID;
                }
            }
        }
    }

    void IDControlParameterToolsNode::ReflectPreviewValues( ControlParameterToolsNode const* pOtherParameterNode )
    {
        auto pRHS = Cast<IDControlParameterToolsNode>( pOtherParameterNode );

        for ( auto otherID : pRHS->m_expectedValues )
        {
            if ( !otherID.IsValid() )
            {
                continue;
            }

            if ( !VectorContains( m_expectedValues, otherID ) )
            {
                m_expectedValues.emplace_back( otherID );
            }
        }
    }

    //-------------------------------------------------------------------------

    VectorControlParameterToolsNode::VectorControlParameterToolsNode()
        : ControlParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::Vector, true );
    }

    VectorControlParameterToolsNode::VectorControlParameterToolsNode( String const& name, String const& groupName )
        : ControlParameterToolsNode( name, groupName )
    {
        CreateOutputPin( "Value", GraphValueType::Vector, true );
    }

    //-------------------------------------------------------------------------

    TargetControlParameterToolsNode::TargetControlParameterToolsNode()
        : ControlParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::Target, true );
    }

    TargetControlParameterToolsNode::TargetControlParameterToolsNode( String const& name, String const& groupName )
        : ControlParameterToolsNode( name, groupName )
    {
        CreateOutputPin( "Value", GraphValueType::Target, true );
    }

    void TargetControlParameterToolsNode::SetPreviewTargetTransform( Transform const& transform )
    {
        NodeGraph::ScopedNodeModification snm( this );
        m_previewStartTransform = transform;
    }

    Target TargetControlParameterToolsNode::GetPreviewStartValue() const
    {
        Target target;

        if ( m_isSet )
        {
            if ( m_isBoneID )
            {
                target = Target( m_previewStartBoneID );
            }
            else
            {
                target = Target( m_previewStartTransform );
            }
        }

        return target;
    }

    //-------------------------------------------------------------------------
    // Virtual Parameter
    //-------------------------------------------------------------------------

    VirtualParameterToolsNode* VirtualParameterToolsNode::Create( FlowGraph* pRootGraph, GraphValueType type, String const& name, String const& groupName )
    {
        EE_ASSERT( pRootGraph != nullptr && pRootGraph->IsRootGraph() );
        EE_ASSERT( type != GraphValueType::Pose && type != GraphValueType::Unknown );

        VirtualParameterToolsNode* pParameter = nullptr;

        switch ( type )
        {
            case GraphValueType::Bool:
            pParameter = pRootGraph->CreateNode<GraphNodes::BoolVirtualParameterToolsNode>( name, groupName );
            break;

            case GraphValueType::ID:
            pParameter = pRootGraph->CreateNode<GraphNodes::IDVirtualParameterToolsNode>( name, groupName );
            break;

            case GraphValueType::Float:
            pParameter = pRootGraph->CreateNode<GraphNodes::FloatVirtualParameterToolsNode>( name, groupName );
            break;

            case GraphValueType::Vector:
            pParameter = pRootGraph->CreateNode<GraphNodes::VectorVirtualParameterToolsNode>( name, groupName );
            break;

            case GraphValueType::Target:
            pParameter = pRootGraph->CreateNode<GraphNodes::TargetVirtualParameterToolsNode>( name, groupName );
            break;

            case GraphValueType::BoneMask:
            pParameter = pRootGraph->CreateNode<GraphNodes::BoneMaskVirtualParameterToolsNode>( name, groupName );
            break;

            default:
            {
                EE_UNREACHABLE_CODE();
            }
            break;
        }

        return pParameter;
    }

    int16_t VirtualParameterToolsNode::Compile( GraphCompilationContext& context ) const
    {
        auto const resultNodes = GetChildGraph()->FindAllNodesOfType<ResultToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Derived );
        EE_ASSERT( resultNodes.size() == 1 );

        auto pConnectedNode = resultNodes[0]->GetConnectedInputNode<FlowToolsNode>( 0 );
        if ( pConnectedNode == nullptr )
        {
            context.LogError( this, "Empty virtual parameter detected!" );
            return InvalidIndex;
        }

        return resultNodes[0]->GetConnectedInputNode<FlowToolsNode>( 0 )->Compile( context );
    }

    VirtualParameterToolsNode::VirtualParameterToolsNode()
        : ParameterBaseToolsNode()
    {
        CreateChildGraph<FlowGraph>( GraphType::ValueTree );
    }

    VirtualParameterToolsNode::VirtualParameterToolsNode( String const& name, String const& groupName )
        : ParameterBaseToolsNode( name, groupName )
    {
        CreateChildGraph<FlowGraph>( GraphType::ValueTree );
    }

    //-------------------------------------------------------------------------

    BoolVirtualParameterToolsNode::BoolVirtualParameterToolsNode() 
        : VirtualParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::Bool, true );
        GetChildGraph()->CreateNode<BoolResultToolsNode>();
    }

    BoolVirtualParameterToolsNode::BoolVirtualParameterToolsNode( String const& name, String const& groupName) 
        : VirtualParameterToolsNode( name, groupName )
    {
        CreateOutputPin( "Value", GraphValueType::Bool, true );
        GetChildGraph()->CreateNode<BoolResultToolsNode>();
    }

    //-------------------------------------------------------------------------

    IDVirtualParameterToolsNode::IDVirtualParameterToolsNode() 
        : VirtualParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::ID, true );
        GetChildGraph()->CreateNode<IDResultToolsNode>();
    }

    IDVirtualParameterToolsNode::IDVirtualParameterToolsNode( String const& name, String const& groupName ) 
        : VirtualParameterToolsNode( name, groupName )
    {
        CreateOutputPin( "Value", GraphValueType::ID, true );
        GetChildGraph()->CreateNode<IDResultToolsNode>();
    }

    //-------------------------------------------------------------------------

    FloatVirtualParameterToolsNode::FloatVirtualParameterToolsNode() : VirtualParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::Float, true );
        GetChildGraph()->CreateNode<FloatResultToolsNode>();
    }

    FloatVirtualParameterToolsNode::FloatVirtualParameterToolsNode( String const& name, String const& groupName ) 
        : VirtualParameterToolsNode( name, groupName )
    {
        CreateOutputPin( "Value", GraphValueType::Float, true );
        GetChildGraph()->CreateNode<FloatResultToolsNode>();
    }

    //-------------------------------------------------------------------------

    VectorVirtualParameterToolsNode::VectorVirtualParameterToolsNode() : VirtualParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::Vector, true );
        GetChildGraph()->CreateNode<VectorResultToolsNode>();
    }

    VectorVirtualParameterToolsNode::VectorVirtualParameterToolsNode( String const& name, String const& groupName ) 
        : VirtualParameterToolsNode( name, groupName )
    {
        CreateOutputPin( "Value", GraphValueType::Vector, true );
        GetChildGraph()->CreateNode<VectorResultToolsNode>();
    }

    //-------------------------------------------------------------------------

    TargetVirtualParameterToolsNode::TargetVirtualParameterToolsNode() : VirtualParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::Target, true );
        GetChildGraph()->CreateNode<TargetResultToolsNode>();
    }

    TargetVirtualParameterToolsNode::TargetVirtualParameterToolsNode( String const& name, String const& groupName ) 
        : VirtualParameterToolsNode( name, groupName )
    {
        CreateOutputPin( "Value", GraphValueType::Target, true );
        GetChildGraph()->CreateNode<TargetResultToolsNode>();
    }

    //-------------------------------------------------------------------------

    BoneMaskVirtualParameterToolsNode::BoneMaskVirtualParameterToolsNode( String const& name, String const& groupName ) 
        : VirtualParameterToolsNode( name, groupName )
    {
        CreateOutputPin( "Value", GraphValueType::BoneMask, true );
        GetChildGraph()->CreateNode<BoneMaskResultToolsNode>();
    }

    BoneMaskVirtualParameterToolsNode::BoneMaskVirtualParameterToolsNode() 
        : VirtualParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::BoneMask, true );
        GetChildGraph()->CreateNode<BoneMaskResultToolsNode>();
    }

    //-------------------------------------------------------------------------
    // Parameter Reference
    //-------------------------------------------------------------------------

    ParameterReferenceToolsNode* ParameterReferenceToolsNode::Create( FlowGraph* pGraph, ParameterBaseToolsNode* pParameter )
    {
        EE_ASSERT( pGraph != nullptr );
        EE_ASSERT( pParameter != nullptr );

        ParameterReferenceToolsNode* pReferenceNode = nullptr;

        switch ( pParameter->GetOutputValueType() )
        {
            case GraphValueType::Bool:
            pReferenceNode = pGraph->CreateNode<GraphNodes::BoolParameterReferenceToolsNode>( pParameter );
            break;

            case GraphValueType::ID:
            pReferenceNode = pGraph->CreateNode<GraphNodes::IDParameterReferenceToolsNode>( pParameter );
            break;

            case GraphValueType::Float:
            pReferenceNode = pGraph->CreateNode<GraphNodes::FloatParameterReferenceToolsNode>( pParameter );
            break;

            case GraphValueType::Vector:
            pReferenceNode = pGraph->CreateNode<GraphNodes::VectorParameterReferenceToolsNode>( pParameter );
            break;

            case GraphValueType::Target:
            pReferenceNode = pGraph->CreateNode<GraphNodes::TargetParameterReferenceToolsNode>( pParameter );
            break;

            case GraphValueType::BoneMask:
            pReferenceNode = pGraph->CreateNode<GraphNodes::BoneMaskParameterReferenceToolsNode>( pParameter );
            break;

            default:
            {
                EE_UNREACHABLE_CODE();
            }
            break;
        }

        return pReferenceNode;
    }

    ParameterReferenceToolsNode::ParameterReferenceToolsNode( ParameterBaseToolsNode* pParameter )
        : m_pParameter( pParameter )
    {
        EE_ASSERT( pParameter != nullptr );
        UpdateCachedParameterData();
    }

    int16_t ParameterReferenceToolsNode::Compile( GraphCompilationContext& context ) const
    {
        return m_pParameter->Compile( context );
    }

    NodeGraph::BaseGraph* ParameterReferenceToolsNode::GetNavigationTarget()
    {
        if ( auto pVP = GetReferencedVirtualParameter() )
        {
            return pVP->GetChildGraph();
        }

        return nullptr;
    }

    FlowToolsNode* ParameterReferenceToolsNode::GetDisplayValueNode()
    {
        if ( IsReferencingControlParameter() )
        {
            return m_pParameter;
        }
        else // Find the input into the virtual parameter result node and display that
        {
            auto const resultNodes = m_pParameter->GetChildGraph()->FindAllNodesOfType<ResultToolsNode>( NodeGraph::SearchMode::Localized, NodeGraph::SearchTypeMatch::Derived );
            EE_ASSERT( resultNodes.size() == 1 );

            auto pConnectedNode = resultNodes[0]->GetConnectedInputNode<FlowToolsNode>( 0 );
            if ( pConnectedNode != nullptr )
            {
                if ( auto pConnectedParameterReference = TryCast<ParameterReferenceToolsNode>( pConnectedNode ) )
                {
                    return pConnectedParameterReference->GetDisplayValueNode();
                }
                else
                {
                    return pConnectedNode;
                }
            }
        }

        return nullptr;
    }

    void ParameterReferenceToolsNode::DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );

        //-------------------------------------------------------------------------

        int16_t runtimeNodeIdx = InvalidIndex;
        bool const isPreviewing = pGraphNodeContext->HasDebugData();
        if ( isPreviewing )
        {
            auto pValueNodeSource = GetDisplayValueNode();
            if ( pValueNodeSource != nullptr )
            {
                runtimeNodeIdx = pGraphNodeContext->GetRuntimeGraphNodeIndex( pValueNodeSource->GetID() );
            }
        }

        //-------------------------------------------------------------------------

        BeginDrawInternalRegion( ctx );

        if ( isPreviewing && ( runtimeNodeIdx != InvalidIndex ) && pGraphNodeContext->IsNodeActive( runtimeNodeIdx ) )
        {
            GraphValueType const valueType = m_pParameter->GetOutputValueType();
            DrawValueDisplayText( ctx, pGraphNodeContext, runtimeNodeIdx, valueType );
        }
        else
        {
            ImGui::NewLine();
        }

        EndDrawInternalRegion( ctx );
    }

    void ParameterReferenceToolsNode::UpdateCachedParameterData()
    {
        EE_ASSERT( m_pParameter != nullptr );
        m_parameterUUID = m_pParameter->GetID();
        m_parameterValueType = m_pParameter->GetOutputValueType();
        m_parameterName = m_pParameter->GetParameterName();
        m_parameterGroup = m_pParameter->GetParameterGroup();
    }
}