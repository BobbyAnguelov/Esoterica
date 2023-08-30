#include "Animation_ToolsGraphNode_Parameters.h"
#include "Animation_ToolsGraphNode_Result.h"
#include "EngineTools/Animation/ToolsGraph/Graphs/Animation_ToolsGraph_FlowGraph.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Parameters.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    ControlParameterToolsNode::ControlParameterToolsNode( String const& name, String const& categoryName )
        : m_name( name )
        , m_parameterCategory( categoryName )
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

            default:
            EE_UNREACHABLE_CODE();
            return InvalidIndex;
            break;
        }
    }

    void ControlParameterToolsNode::Rename( String const& name, String const& category )
    {
        VisualGraph::ScopedNodeModification snm( this );
        m_name = name;
        m_parameterCategory = category;
    }

    ControlParameterToolsNode* ControlParameterToolsNode::Create( FlowGraph* pRootGraph, GraphValueType type, String const& name, String const& category )
    {
        EE_ASSERT( pRootGraph != nullptr && pRootGraph->IsRootGraph() );
        EE_ASSERT( type != GraphValueType::Pose && type != GraphValueType::BoneMask && type != GraphValueType::Unknown );

        ControlParameterToolsNode* pParameter = nullptr;

        switch ( type )
        {
            case GraphValueType::Bool:
            pParameter = pRootGraph->CreateNode<GraphNodes::BoolControlParameterToolsNode>( name, category );
            break;

            case GraphValueType::ID:
            pParameter = pRootGraph->CreateNode<GraphNodes::IDControlParameterToolsNode>( name, category );
            break;

            case GraphValueType::Float:
            pParameter = pRootGraph->CreateNode<GraphNodes::FloatControlParameterToolsNode>( name, category );
            break;

            case GraphValueType::Vector:
            pParameter = pRootGraph->CreateNode<GraphNodes::VectorControlParameterToolsNode>( name, category );
            break;

            case GraphValueType::Target:
            pParameter = pRootGraph->CreateNode<GraphNodes::TargetControlParameterToolsNode>( name, category );
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

    BoolControlParameterToolsNode::BoolControlParameterToolsNode( String const& name, String const& categoryName )
        : ControlParameterToolsNode( name, categoryName )
    {
        CreateOutputPin( "Value", GraphValueType::Bool, true );
    }

    //-------------------------------------------------------------------------

    FloatControlParameterToolsNode::FloatControlParameterToolsNode()
        : ControlParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::Float, true );
    }

    FloatControlParameterToolsNode::FloatControlParameterToolsNode( String const& name, String const& categoryName )
        : ControlParameterToolsNode( name, categoryName )
    {
        CreateOutputPin( "Value", GraphValueType::Float, true );
    }

    //-------------------------------------------------------------------------

    IDControlParameterToolsNode::IDControlParameterToolsNode()
        : ControlParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::ID, true );
    }

    IDControlParameterToolsNode::IDControlParameterToolsNode( String const& name, String const& categoryName )
        : ControlParameterToolsNode( name, categoryName )
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
            VisualGraph::ScopedNodeModification snm( this );

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

    VectorControlParameterToolsNode::VectorControlParameterToolsNode( String const& name, String const& categoryName )
        : ControlParameterToolsNode( name, categoryName )
    {
        CreateOutputPin( "Value", GraphValueType::Vector, true );
    }

    //-------------------------------------------------------------------------

    TargetControlParameterToolsNode::TargetControlParameterToolsNode()
        : ControlParameterToolsNode()
    {
        CreateOutputPin( "Value", GraphValueType::Target, true );
    }

    TargetControlParameterToolsNode::TargetControlParameterToolsNode( String const& name, String const& categoryName )
        : ControlParameterToolsNode( name, categoryName )
    {
        CreateOutputPin( "Value", GraphValueType::Target, true );
    }

    void TargetControlParameterToolsNode::SetPreviewTargetTransform( Transform const& transform )
    {
        VisualGraph::ScopedNodeModification snm( this );
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

    VirtualParameterToolsNode::VirtualParameterToolsNode( String const& name, String const& categoryName )
        : m_name( name )
        , m_parameterCategory( categoryName )
    {
        EE_ASSERT( !m_name.empty() );
    }

    void VirtualParameterToolsNode::Initialize( VisualGraph::BaseGraph* pParentGraph )
    {
        FlowToolsNode::Initialize( pParentGraph );

        auto pParameterGraph = EE::New<FlowGraph>( GraphType::ValueTree );

        switch ( GetValueType() )
        {
            case GraphValueType::Bool:
            pParameterGraph->CreateNode<BoolResultToolsNode>();
            break;

            case GraphValueType::ID:
            pParameterGraph->CreateNode<IDResultToolsNode>();
            break;

            case GraphValueType::Float:
            pParameterGraph->CreateNode<FloatResultToolsNode>();
            break;

            case GraphValueType::Vector:
            pParameterGraph->CreateNode<VectorResultToolsNode>();
            break;

            case GraphValueType::Target:
            pParameterGraph->CreateNode<TargetResultToolsNode>();
            break;

            case GraphValueType::BoneMask:
            pParameterGraph->CreateNode<BoneMaskResultToolsNode>();
            break;

            case GraphValueType::Pose:
            pParameterGraph->CreateNode<PoseResultToolsNode>();
            break;

            default:
            {
                EE_UNREACHABLE_CODE();
            }
            break;
        }

        SetChildGraph( pParameterGraph );
    }

    int16_t VirtualParameterToolsNode::Compile( GraphCompilationContext& context ) const
    {
        auto const resultNodes = GetChildGraph()->FindAllNodesOfType<ResultToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
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

    VirtualParameterToolsNode* VirtualParameterToolsNode::Create( FlowGraph* pRootGraph, GraphValueType type, String const& name, String const& category )
    {
        EE_ASSERT( pRootGraph != nullptr && pRootGraph->IsRootGraph() );
        EE_ASSERT( type != GraphValueType::Pose && type != GraphValueType::Unknown );

        VirtualParameterToolsNode* pParameter = nullptr;

        switch ( type )
        {
            case GraphValueType::Bool:
            pParameter = pRootGraph->CreateNode<GraphNodes::BoolVirtualParameterToolsNode>( name, category );
            break;

            case GraphValueType::ID:
            pParameter = pRootGraph->CreateNode<GraphNodes::IDVirtualParameterToolsNode>( name, category );
            break;

            case GraphValueType::Float:
            pParameter = pRootGraph->CreateNode<GraphNodes::FloatVirtualParameterToolsNode>( name, category );
            break;

            case GraphValueType::Vector:
            pParameter = pRootGraph->CreateNode<GraphNodes::VectorVirtualParameterToolsNode>( name, category );
            break;

            case GraphValueType::Target:
            pParameter = pRootGraph->CreateNode<GraphNodes::TargetVirtualParameterToolsNode>( name, category );
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
    // Parameter Reference
    //-------------------------------------------------------------------------

    ParameterReferenceToolsNode::ParameterReferenceToolsNode( VirtualParameterToolsNode* pParameter )
        : m_pParameter( pParameter )
    {
        EE_ASSERT( pParameter != nullptr );
        UpdateCachedParameterData();
    }

    ParameterReferenceToolsNode::ParameterReferenceToolsNode( ControlParameterToolsNode* pParameter )
        : m_pParameter( pParameter )
    {
        EE_ASSERT( pParameter != nullptr );
        UpdateCachedParameterData();
    }

    int16_t ParameterReferenceToolsNode::Compile( GraphCompilationContext& context ) const
    {
        return m_pParameter->Compile( context );
    }

    void ParameterReferenceToolsNode::OnDoubleClick( VisualGraph::UserContext* pUserContext )
    {
        if ( auto pVP = GetReferencedVirtualParameter() )
        {
            pUserContext->NavigateTo( pVP->GetChildGraph() );
        }
    }

    FlowToolsNode* ParameterReferenceToolsNode::GetDisplayValueNode()
    {
        if ( IsReferencingControlParameter() )
        {
            return m_pParameter;
        }
        else // Find the input into the virtual parameter result node and display that
        {
            auto const resultNodes = m_pParameter->GetChildGraph()->FindAllNodesOfType<ResultToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
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

    void ParameterReferenceToolsNode::DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext )
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
            GraphValueType const valueType = m_pParameter->GetValueType();
            DrawValueDisplayText( ctx, pGraphNodeContext, runtimeNodeIdx, valueType );
        }
        else
        {
            ImGui::NewLine();
        }

        EndDrawInternalRegion( ctx );
    }

    void ParameterReferenceToolsNode::SetReferencedParameter( FlowToolsNode* pParameter )
    {
        EE_ASSERT( pParameter != nullptr );
        EE_ASSERT( IsOfType<ControlParameterToolsNode>( pParameter ) || IsOfType<VirtualParameterToolsNode>( pParameter ) );
        EE_ASSERT( pParameter->GetValueType() == GetValueType() );
        m_pParameter = pParameter;
        UpdateCachedParameterData();
    }

    void ParameterReferenceToolsNode::UpdateCachedParameterData()
    {
        EE_ASSERT( m_pParameter != nullptr );

        if ( IsReferencingControlParameter() )
        {
            auto pParameter = GetReferencedControlParameter();
            m_parameterUUID = pParameter->GetID();
            m_parameterValueType = pParameter->GetValueType();
            m_parameterName = pParameter->GetParameterName();
            m_parameterCategory = pParameter->GetParameterCategory();
        }
        else
        {
            auto pParameter = GetReferencedVirtualParameter();
            m_parameterUUID = pParameter->GetID();
            m_parameterValueType = pParameter->GetValueType();
            m_parameterName = pParameter->GetParameterName();
            m_parameterCategory = pParameter->GetParameterCategory();
        }
    }

    String const& ParameterReferenceToolsNode::GetReferencedParameterCategory() const
    {
        if ( m_pParameter != nullptr )
        {
            if ( IsReferencingControlParameter() )
            {
                auto pParameter = GetReferencedControlParameter();
                return pParameter->GetParameterCategory();
            }
            else
            {
                auto pParameter = GetReferencedVirtualParameter();
                return pParameter->GetParameterCategory();
            }
        }

        return m_parameterCategory;
    }
}