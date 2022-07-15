#include "Animation_RuntimeGraphNode_Vectors.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Contexts.h"
#include "System/Log.h"
#include "System/Math/MathHelpers.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void VectorInfoNode::Settings::InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<VectorInfoNode>( nodePtrs, options );
        SetNodePtrFromIndex( nodePtrs, m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void VectorInfoNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        FloatValueNode::InitializeInternal( context );
        m_pInputValueNode->Initialize( context );
    }

    void VectorInfoNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        m_pInputValueNode->Shutdown( context );
        FloatValueNode::ShutdownInternal( context );
    }

    void VectorInfoNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        auto pSettings = GetSettings<VectorInfoNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            Vector const inputVector = m_pInputValueNode->GetValue<Vector>( context );
            switch ( pSettings->m_desiredInfo )
            {
            case Info::X: m_value = inputVector.m_x; break;
            case Info::Y: m_value = inputVector.m_y; break;
            case Info::Z: m_value = inputVector.m_z; break;
            case Info::W: m_value = inputVector.m_w; break;

            case Info::Length:
            {
                m_value = inputVector.GetLength3();
            }
            break;

            case Info::AngleHorizontal:
            {
                m_value = (float) Math::GetYawAngleBetweenVectors( Vector::WorldForward, inputVector );
            }
            break;

            case Info::AngleVertical:
            {
                m_value = (float) Quaternion::FromRotationBetweenVectors( Vector::WorldForward, inputVector ).ToEulerAngles().GetPitch();
            }
            break;

            case Info::SizeHorizontal:
            {
                m_value = inputVector.GetLength3();
            }
            break;

            case Info::SizeVertical:
            {
                m_value = Vector( inputVector.m_x, 0.0f, inputVector.m_z ).GetLength3();
            }
            break;
            }
        }

        *reinterpret_cast<float*>( pOutValue ) = m_value;
    }

    //-------------------------------------------------------------------------

    void VectorNegateNode::Settings::InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<VectorNegateNode>( nodePtrs, options );
        SetNodePtrFromIndex( nodePtrs, m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void VectorNegateNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( m_pInputValueNode != nullptr );
        VectorValueNode::InitializeInternal( context );
        m_pInputValueNode->Initialize( context );
    }

    void VectorNegateNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( m_pInputValueNode != nullptr );
        m_pInputValueNode->Shutdown( context );
        VectorValueNode::ShutdownInternal( context );
    }

    void VectorNegateNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( m_pInputValueNode != nullptr );

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            m_value = m_pInputValueNode->GetValue<Vector>( context ).GetNegated();
        }

        *reinterpret_cast<Vector*>( pOutValue ) = m_value;
    }
}