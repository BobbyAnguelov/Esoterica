#include "Animation_ToolsGraph_Definition.h"
#include "Nodes/Animation_ToolsGraphNode_Result.h"
#include "Nodes/Animation_ToolsGraphNode_Parameters.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationGraph.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    using namespace EE::Animation::GraphNodes;

    //-------------------------------------------------------------------------

    ToolsGraphDefinition::ToolsGraphDefinition()
    {
        ResetToDefaultState();
    }

    ToolsGraphDefinition::~ToolsGraphDefinition()
    {
        ResetInternalState();
    }

    void ToolsGraphDefinition::ResetInternalState()
    {
        m_variationHierarchy.Reset();

        if ( m_pRootGraph != nullptr )
        {
            m_pRootGraph->Shutdown();
            EE::Delete( m_pRootGraph );
        }
    }

    void ToolsGraphDefinition::ResetToDefaultState()
    {
        ResetInternalState();
        m_pRootGraph = EE::New<FlowGraph>( GraphType::BlendTree );
        m_pRootGraph->CreateNode<ResultToolsNode>( GraphValueType::Pose );
    }

    bool ToolsGraphDefinition::LoadFromJson( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphDescriptorObjectValue )
    {
        EE_ASSERT( graphDescriptorObjectValue.IsObject() );

        ResetInternalState();

        // Find relevant json values
        //-------------------------------------------------------------------------

        Serialization::JsonValue const* pGraphObjectValue = nullptr;
        Serialization::JsonValue const* pVariationsObjectValue = nullptr;
        Serialization::JsonValue const* pRootGraphObjectValue = nullptr;

        auto graphDefinitionValueIter = graphDescriptorObjectValue.FindMember( "GraphDefinition" );
        if ( graphDefinitionValueIter != graphDescriptorObjectValue.MemberEnd() && graphDefinitionValueIter->value.IsObject() )
        {
            pGraphObjectValue = &graphDefinitionValueIter->value;
        }

        if ( pGraphObjectValue != nullptr )
        {
            auto rootGraphValueIter = pGraphObjectValue->FindMember( "RootGraph" );
            if ( rootGraphValueIter != pGraphObjectValue->MemberEnd() )
            {
                pRootGraphObjectValue = &rootGraphValueIter->value;
            }
        }

        if ( pGraphObjectValue != nullptr )
        {
            auto variationsValueIter = pGraphObjectValue->FindMember( "Variations" );
            if ( variationsValueIter != pGraphObjectValue->MemberEnd() && variationsValueIter->value.IsArray() )
            {
                pVariationsObjectValue = &variationsValueIter->value;
            }
        }

        // Deserialize graph
        //-------------------------------------------------------------------------

        bool serializationSuccessful = false;

        if ( pRootGraphObjectValue != nullptr && pVariationsObjectValue != nullptr )
        {
            m_pRootGraph = Cast<FlowGraph>( VisualGraph::BaseGraph::CreateGraphFromSerializedData( typeRegistry, *pRootGraphObjectValue, nullptr ) );
            serializationSuccessful = m_variationHierarchy.Serialize( typeRegistry, *pVariationsObjectValue );
        }

        // If serialization failed, reset the graph state to a valid one
        if ( !serializationSuccessful )
        {
            ResetToDefaultState();
        }

        //-------------------------------------------------------------------------

        if ( serializationSuccessful )
        {
            GraphNodes::ParameterReferenceToolsNode::GloballyRefreshParameterReferences( m_pRootGraph );
        }

        return serializationSuccessful;
    }

    void ToolsGraphDefinition::SaveToJson( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const
    {
        writer.StartObject();
        writer.Key( Serialization::s_typeIDKey );
        writer.String( GraphResourceDescriptor::GetStaticTypeID().c_str() );

        writer.Key( "GraphDefinition" );
        writer.StartObject();
        {
            writer.Key( "RootGraph" );
            m_pRootGraph->Serialize( typeRegistry, writer );

            writer.Key( "Variations" );
            m_variationHierarchy.Serialize( typeRegistry, writer );

            writer.EndObject();
        }
        writer.EndObject();
    }
}