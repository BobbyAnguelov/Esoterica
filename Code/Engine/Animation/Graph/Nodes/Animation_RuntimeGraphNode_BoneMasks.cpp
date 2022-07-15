#include "Animation_RuntimeGraphNode_BoneMasks.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Contexts.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void BoneMaskNode::Settings::InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<BoneMaskNode>( nodePtrs, options );

        auto const pBoneWeights = pDataSet->GetResource<BoneMaskDefinition>( m_dataSlotIdx );
        if ( pBoneWeights != nullptr )
        {
            pNode->m_boneMask = BoneMask( pDataSet->GetSkeleton(), *pBoneWeights, m_rootMotionWeight );
        }
        else // No bone mask set
        {
            pNode->m_boneMask = BoneMask( pDataSet->GetSkeleton(), 1.0f, m_rootMotionWeight );
        }
    }

    void BoneMaskNode::InitializeInternal( GraphContext& context )
    {
        BoneMaskValueNode::InitializeInternal( context );

    }

    void BoneMaskNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        *reinterpret_cast<BoneMask const**>( pOutValue ) = &m_boneMask;
    }

    //-------------------------------------------------------------------------

    void BoneMaskBlendNode::Settings::InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<BoneMaskBlendNode>( nodePtrs, options );
        SetNodePtrFromIndex( nodePtrs, m_sourceMaskNodeIdx, pNode->m_pSourceBoneMask );
        SetNodePtrFromIndex( nodePtrs, m_targetMaskNodeIdx, pNode->m_pTargetBoneMask );
        SetNodePtrFromIndex( nodePtrs, m_blendWeightValueNodeIdx, pNode->m_pBlendWeightValueNode );
    }

    void BoneMaskBlendNode::InitializeInternal( GraphContext& context )
    {
        BoneMaskValueNode::InitializeInternal( context );

        //-------------------------------------------------------------------------

        m_pSourceBoneMask->Initialize( context );
        m_pTargetBoneMask->Initialize( context );
        m_pBlendWeightValueNode->Initialize( context );

        //-------------------------------------------------------------------------

        if ( m_blendedBoneMask.GetSkeleton() != context.m_pSkeleton )
        {
            m_blendedBoneMask = BoneMask( context.m_pSkeleton );
        }
        else
        {
            m_blendedBoneMask.ResetWeights();
        }
    }

    void BoneMaskBlendNode::ShutdownInternal( GraphContext& context )
    {
        m_pBlendWeightValueNode->Shutdown( context );
        m_pTargetBoneMask->Shutdown( context );
        m_pSourceBoneMask->Shutdown( context );

        BoneMaskValueNode::ShutdownInternal( context );
    }

    void BoneMaskBlendNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() && m_pBlendWeightValueNode != nullptr && m_pSourceBoneMask != nullptr && m_pTargetBoneMask != nullptr );

        float const blendWeight = m_pBlendWeightValueNode->GetValue<float>( context );

        // If we dont need to perform the blend, set the ptr to the required source
        if ( blendWeight <= 0.0f )
        {
            *reinterpret_cast<BoneMask const**>( pOutValue ) = m_pSourceBoneMask->GetValue<BoneMask const*>( context );
        }
        else if ( blendWeight >= 1.0f )
        {
            *reinterpret_cast<BoneMask const**>( pOutValue ) = m_pTargetBoneMask->GetValue<BoneMask const*>( context );
        }
        else // Actually perform the blend
        {
            m_blendedBoneMask.SetFromBlend( *m_pSourceBoneMask->GetValue<BoneMask const*>( context ), *m_pTargetBoneMask->GetValue<BoneMask const*>( context ), blendWeight );
            *reinterpret_cast<BoneMask const**>( pOutValue ) = &m_blendedBoneMask;
        }
    }

    //-------------------------------------------------------------------------

    void BoneMaskSelectorNode::Settings::InstantiateNode( TVector<GraphNode*> const& nodePtrs, GraphDataSet const* pDataSet, InstantiationOptions options ) const
    {
        auto pNode = CreateNode<BoneMaskSelectorNode>( nodePtrs, options );

        for ( int16_t const maskNodeIdx : m_maskNodeIndices )
        {
            SetNodePtrFromIndex( nodePtrs, maskNodeIdx, pNode->m_boneMaskOptionNodes.emplace_back() );
        }

        SetNodePtrFromIndex( nodePtrs, m_defaultMaskNodeIdx, pNode->m_pDefaultMaskValueNode );
        SetNodePtrFromIndex( nodePtrs, m_parameterValueNodeIdx, pNode->m_pParameterValueNode );
    }

    void BoneMaskSelectorNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( context.IsValid() );
        BoneMaskValueNode::InitializeInternal( context );
        m_pParameterValueNode->Initialize( context );

        size_t const numMasks = m_boneMaskOptionNodes.size();
        for ( auto i = 0u; i < numMasks; i++ )
        {
            m_boneMaskOptionNodes[i]->Initialize( context );
        }

        //-------------------------------------------------------------------------

        m_selectedMaskIndex = TrySelectMask( context );
        m_newMaskIndex = InvalidIndex;
        m_isBlending = false;
    }

    void BoneMaskSelectorNode::ShutdownInternal( GraphContext& context )
    {
        m_selectedMaskIndex = InvalidIndex;
        m_newMaskIndex = InvalidIndex;
        m_isBlending = false;

        //-------------------------------------------------------------------------

        if ( m_pDefaultMaskValueNode != nullptr ) // TEMP UPGRADE HACK, REMOVE ONCE GRAPH RESAVED
        {
            m_pDefaultMaskValueNode->Shutdown( context );
        }

        size_t const numOptions =  m_boneMaskOptionNodes.size();
        for ( auto i = 0; i < numOptions; i++ )
        {
            m_boneMaskOptionNodes[i]->Shutdown( context );
        }

        m_pParameterValueNode->Shutdown( context );
        BoneMaskValueNode::ShutdownInternal( context );
    }

    BoneMask const* BoneMaskSelectorNode::GetBoneMaskForIndex( GraphContext& context, int32_t optionIndex ) const
    {
        EE_ASSERT( optionIndex >= -1 && optionIndex < m_boneMaskOptionNodes.size() );

        if ( optionIndex != InvalidIndex )
        {
            return m_boneMaskOptionNodes[optionIndex]->GetValue<BoneMask const*>( context );
        }
        else
        {
            return m_pDefaultMaskValueNode->GetValue<BoneMask const*>( context );
        }
    }

    void BoneMaskSelectorNode::GetValueInternal( GraphContext& context, void* pOutValue )
    {
        EE_ASSERT( context.IsValid() );

        if ( WasUpdated( context ) )
        {
            if ( m_isBlending )
            {
                *reinterpret_cast<BoneMask const**>( pOutValue ) = &m_blendedBoneMask;
            }
            else
            {
                *reinterpret_cast<BoneMask const**>( pOutValue ) = GetBoneMaskForIndex( context, m_selectedMaskIndex );
            }
            return;
        }

        //-------------------------------------------------------------------------

        MarkNodeActive( context );

        auto pSettings = GetSettings<BoneMaskSelectorNode>();
        if ( pSettings->m_switchDynamically )
        {
            // Only try to select a new mask if we are not blending
            if ( !m_isBlending )
            {
                m_newMaskIndex = TrySelectMask( context );

                // If the new mask is the same as the current one, do nothing
                if ( m_newMaskIndex == m_selectedMaskIndex )
                {
                    m_newMaskIndex = InvalidIndex;
                }
                else // Start a blend to the new mask
                {
                    m_currentTimeInBlend = 0.0f;
                    m_isBlending = true;
                }
            }
        }

        //-------------------------------------------------------------------------

        if ( m_isBlending )
        {
            m_currentTimeInBlend += context.m_deltaTime;
            float const blendWeight = m_currentTimeInBlend / pSettings->m_blendTime;

            // If the blend is complete, then update the selected mask index
            if ( blendWeight >= 1.0f )
            {
                m_selectedMaskIndex = m_newMaskIndex;
                m_newMaskIndex = InvalidIndex;
                m_isBlending = false;
            }
            else // Perform blend and return the result
            {
                m_blendedBoneMask = *GetBoneMaskForIndex( context, m_selectedMaskIndex );
                m_blendedBoneMask.BlendTo( *GetBoneMaskForIndex( context, m_newMaskIndex ), blendWeight );
                *reinterpret_cast<BoneMask const**>( pOutValue ) = &m_blendedBoneMask;
                return;
            }
        }

        // No blend or blend completed this frame
        if ( m_selectedMaskIndex != InvalidIndex )
        {
            *reinterpret_cast<BoneMask const**>( pOutValue ) = m_boneMaskOptionNodes[m_selectedMaskIndex]->GetValue<BoneMask const*>( context );
        }
        else
        {
            *reinterpret_cast<BoneMask const**>( pOutValue ) = nullptr;
        }
    }
}