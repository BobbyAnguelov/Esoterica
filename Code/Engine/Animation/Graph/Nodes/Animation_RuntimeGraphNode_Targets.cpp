#include "Animation_RuntimeGraphNode_Targets.h"


//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void IsTargetSetNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<IsTargetSetNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void IsTargetSetNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        BoolValueNode::InitializeInternal( context );
        m_pInputValueNode->Initialize( context );
        m_result = false;
    }

    void IsTargetSetNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );
        m_pInputValueNode->Shutdown( context );
        BoolValueNode::ShutdownInternal( context );
    }

    void IsTargetSetNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pInputValueNode != nullptr );

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );
            m_result = m_pInputValueNode->GetValue<Target>( context ).IsTargetSet();
        }

        *( (bool*) pOutValue ) = m_result;
    }

    //-------------------------------------------------------------------------

    void TargetInfoNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<TargetInfoNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pTargetNode );
    }

    void TargetInfoNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pTargetNode != nullptr );
        FloatValueNode::InitializeInternal( context );
        m_pTargetNode->Initialize( context );
    }

    void TargetInfoNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() && m_pTargetNode != nullptr );
        m_pTargetNode->Shutdown( context );
        FloatValueNode::ShutdownInternal( context );
    }

    void TargetInfoNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pTargetNode != nullptr );
        auto pDefinition = GetDefinition<TargetInfoNode>();

        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            Target const inputTarget = m_pTargetNode->GetValue<Target>( context );
            if ( inputTarget.IsTargetSet() )
            {
                // Get the transform out of the source target
                Transform inputTargetTransform;
                bool bIsValidTransform = inputTarget.TryGetTransform( context.m_pPreviousPose, inputTargetTransform );

                // If we have a valid transform, update the internal value
                if ( bIsValidTransform )
                {
                    if ( pDefinition->m_isWorldSpaceTarget )
                    {
                        inputTargetTransform = inputTargetTransform * context.m_worldTransformInverse;
                    }

                    //-------------------------------------------------------------------------
                    // NB: Target transform is in character space

                    switch ( pDefinition->m_infoType )
                    {
                    case Info::AngleHorizontal:
                    {
                        Vector const direction = inputTargetTransform.GetTranslation().GetNormalized2();
                        if ( direction.IsNearZero3() )
                        {
                            m_value = 0.0f;
                        }
                        else
                        {
                            float const dotForward = Vector::WorldForward.GetDot3( direction );
                            m_value = Math::ACos( dotForward );
                            m_value = Math::RadiansToDegrees * m_value;

                            float const dotRight = Vector::WorldRight.GetDot3( direction );
                            if ( dotRight < 0.0f )
                            {
                                m_value = -m_value;
                            }
                        }
                    }
                    break;

                    //-------------------------------------------------------------------------

                    case Info::AngleVertical:
                    {
                        Vector const Direction = inputTargetTransform.GetTranslation().GetNormalized3();
                        if ( Direction.IsNearZero3() )
                        {
                            m_value = 0.0f;
                        }
                        else
                        {
                            m_value = Math::RadiansToDegrees * ( Math::PiDivTwo - Math::ACos( Vector::WorldUp.GetDot3( Direction ) ) );
                        }
                    }
                    break;

                    //-------------------------------------------------------------------------

                    case Info::Distance:
                    {
                        m_value = inputTargetTransform.GetTranslation().GetLength3();
                    }
                    break;

                    //-------------------------------------------------------------------------

                    case Info::DistanceHorizontalOnly:
                    {
                        m_value = inputTargetTransform.GetTranslation().GetLength2();
                    }
                    break;

                    //-------------------------------------------------------------------------

                    case Info::DistanceVerticalOnly:
                    {
                        m_value = Math::Abs( inputTargetTransform.GetTranslation().GetZ() );
                    }
                    break;

                    //-------------------------------------------------------------------------

                    case Info::DeltaOrientationX:
                    {
                        Transform const targetTransformCS = inputTargetTransform * context.m_worldTransformInverse;
                        m_value = (float) targetTransformCS.GetRotation().ToEulerAngles().m_x.ToDegrees();
                    }
                    break;

                    case Info::DeltaOrientationY:
                    {
                        Transform const targetTransformCS = inputTargetTransform * context.m_worldTransformInverse;
                        m_value = (float) targetTransformCS.GetRotation().ToEulerAngles().m_y.ToDegrees();
                    }
                    break;

                    case Info::DeltaOrientationZ:
                    {
                        Transform const targetTransformCS = inputTargetTransform * context.m_worldTransformInverse;
                        m_value = (float) targetTransformCS.GetRotation().ToEulerAngles().m_z.ToDegrees();
                    }
                    break;
                    }
                }
            }
            else
            {
                m_value = 0.0f;
            }
        }

        *reinterpret_cast<float*>( pOutValue ) = m_value;
    }

    //-------------------------------------------------------------------------

    void TargetOffsetNode::Definition::InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<TargetOffsetNode>( context, options );
        context.SetNodePtrFromIndex( m_inputValueNodeIdx, pNode->m_pInputValueNode );
    }

    void TargetOffsetNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( m_pInputValueNode != nullptr );
        TargetValueNode::InitializeInternal( context );
        m_pInputValueNode->Initialize( context );
    }

    void TargetOffsetNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( m_pInputValueNode != nullptr );
        m_pInputValueNode->Shutdown( context );
        TargetValueNode::ShutdownInternal( context );
    }

    void TargetOffsetNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        if ( !WasUpdated( context ) )
        {
            MarkNodeActive( context );

            m_value = m_pInputValueNode->GetValue<Target>( context );

            if ( m_value.IsTargetSet() )
            {
                auto pDefinition = GetDefinition<TargetOffsetNode>();
                m_value.SetOffsets( pDefinition->m_rotationOffset, pDefinition->m_translationOffset, pDefinition->m_isBoneSpaceOffset );
            }
            else
            {
                #if EE_DEVELOPMENT_TOOLS
                context.LogWarning( GetNodeIndex(), "Trying to set an offset on an unset node!" );
                #endif
            }
        }

        *reinterpret_cast<Target*>( pOutValue ) = m_value;
    }
}