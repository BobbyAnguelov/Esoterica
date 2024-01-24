#include "Animation_RuntimeGraphNode_Vectors.h"

#include "Base/Math/MathUtils.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void VectorInfoNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<VectorInfoNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
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
        auto pDefinition = GetDefinition<VectorInfoNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            Vector const inputVector = m_pInputValueNode->GetValue<Vector>( context );
            switch ( pDefinition->m_desiredInfo )
            {
                case Info::X:
                m_value = inputVector.GetX();
                break;

                case Info::Y:
                m_value = inputVector.GetY();
                break;

                case Info::Z:
                m_value = inputVector.GetZ();
                break;

                case Info::W:
                m_value = inputVector.GetW();
                break;

                case Info::Length:
                {
                    m_value = inputVector.GetLength3();
                }
                break;

                // Vectors are assumed to be in character space
                case Info::AngleHorizontal:
                {
                    if ( !inputVector.IsNearZero3() )
                    {
                        m_value = Math::GetYawAngleBetweenVectors( Vector::WorldForward, inputVector ).ToDegrees().ToFloat();
                    }
                    else
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        context.LogWarning( GetNodeIndex(), "Zero input vector for info node!" );
                        #endif
                    }
                }
                break;

                // Vectors are assumed to be in character space
                case Info::AngleVertical:
                {
                    if ( !inputVector.IsNearZero3() )
                    {
                        EulerAngles const angles = Quaternion::FromRotationBetweenVectors( Vector::WorldForward, inputVector ).ToEulerAngles();
                        m_value = angles.GetPitch().ToDegrees().ToFloat();
                    }
                    else
                    {
                        #if EE_DEVELOPMENT_TOOLS
                        context.LogWarning( GetNodeIndex(), "Zero input vector for info node!" );
                        #endif
                    }
                }
                break;
            }
        }

        *reinterpret_cast<float*>( pOutValue ) = m_value;
    }

    //-------------------------------------------------------------------------

    void VectorCreateNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<VectorCreateNode>( context, options );
        context.SetOptionalNodePtrFromIndex( m_inputVectorValueNodeIdx, pNode->m_pInputVectorValueNode );
        context.SetOptionalNodePtrFromIndex( m_inputValueXNodeIdx, pNode->m_pInputXValueNode );
        context.SetOptionalNodePtrFromIndex( m_inputValueYNodeIdx, pNode->m_pInputYValueNode );
        context.SetOptionalNodePtrFromIndex( m_inputValueZNodeIdx, pNode->m_pInputZValueNode );
    }

    void VectorCreateNode::InitializeInternal( GraphContext& context )
    {
        VectorValueNode::InitializeInternal( context );

        if ( m_pInputVectorValueNode != nullptr )
        {
            m_pInputVectorValueNode->Initialize( context );
        }

        if ( m_pInputXValueNode != nullptr )
        {
            m_pInputXValueNode->Initialize( context );
        }

        if ( m_pInputYValueNode != nullptr )
        {
            m_pInputYValueNode->Initialize( context );
        }

        if ( m_pInputZValueNode != nullptr )
        {
            m_pInputZValueNode->Initialize( context );
        }
    }

    void VectorCreateNode::ShutdownInternal( GraphContext& context )
    {
        if ( m_pInputVectorValueNode != nullptr )
        {
            m_pInputVectorValueNode->Shutdown( context );
        }

        if ( m_pInputXValueNode != nullptr )
        {
            m_pInputXValueNode->Shutdown( context );
        }

        if ( m_pInputYValueNode != nullptr )
        {
            m_pInputYValueNode->Shutdown( context );
        }

        if ( m_pInputZValueNode != nullptr )
        {
            m_pInputZValueNode->Shutdown( context );
        }

        VectorValueNode::ShutdownInternal( context );
    }

    void VectorCreateNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            //-------------------------------------------------------------------------

            if ( m_pInputVectorValueNode != nullptr )
            {
                m_value = m_pInputVectorValueNode->GetValue<Vector>( context );
            }
            else
            {
                m_value = Vector::Zero;
            }

            //-------------------------------------------------------------------------

            if ( m_pInputXValueNode != nullptr )
            {
                m_value.SetX( m_pInputXValueNode->GetValue<float>( context ) );
            }

            if ( m_pInputYValueNode != nullptr )
            {
                m_value.SetY( m_pInputYValueNode->GetValue<float>( context ) );
            }

            if ( m_pInputZValueNode != nullptr )
            {
                m_value.SetZ( m_pInputZValueNode->GetValue<float>( context ) );
            }
        }

        *reinterpret_cast<Vector*>( pOutValue ) = m_value;
    }

    //-------------------------------------------------------------------------

    void VectorNegateNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<VectorNegateNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
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