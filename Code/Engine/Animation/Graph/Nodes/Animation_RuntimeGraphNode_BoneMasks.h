#pragma once
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Animation/AnimationBoneMask.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class EE_ENGINE_API BoneMaskNode final : public BoneMaskValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public BoneMaskValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoneMaskValueNode::Settings, m_boneMaskID );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            StringID                                        m_boneMaskID;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        BoneMaskTaskList                                    m_taskList;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API FixedWeightBoneMaskNode final : public BoneMaskValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public BoneMaskValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoneMaskValueNode::Settings, m_boneWeight );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            float                                           m_boneWeight = 0.0f;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        BoneMaskTaskList                                    m_taskList;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API BoneMaskBlendNode final : public BoneMaskValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public BoneMaskValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoneMaskValueNode::Settings, m_sourceMaskNodeIdx, m_targetMaskNodeIdx, m_blendWeightValueNodeIdx );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                                         m_sourceMaskNodeIdx = InvalidIndex;
            int16_t                                         m_targetMaskNodeIdx = InvalidIndex;
            int16_t                                         m_blendWeightValueNodeIdx = InvalidIndex;
        };

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

    private:

        BoneMaskValueNode*                                  m_pSourceBoneMask = nullptr;
        BoneMaskValueNode*                                  m_pTargetBoneMask = nullptr;
        FloatValueNode*                                     m_pBlendWeightValueNode = nullptr;
        BoneMaskTaskList                                    m_taskList;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API BoneMaskSelectorNode final : public BoneMaskValueNode
    {
    public:

        struct EE_ENGINE_API Settings final : public BoneMaskValueNode::Settings
        {
            EE_REFLECT_TYPE( Settings );
            EE_SERIALIZE_GRAPHNODESETTINGS( BoneMaskValueNode::Settings, m_defaultMaskNodeIdx, m_parameterValueNodeIdx, m_switchDynamically, m_maskNodeIndices, m_parameterValues, m_blendTime );

            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const override;

            int16_t                                         m_defaultMaskNodeIdx = InvalidIndex;
            int16_t                                         m_parameterValueNodeIdx = InvalidIndex;
            bool                                            m_switchDynamically = false;
            TInlineVector<int16_t, 7>                       m_maskNodeIndices;
            TInlineVector<StringID, 7>                      m_parameterValues;
            Seconds                                         m_blendTime = 0.1f;
        };

        #if EE_DEVELOPMENT_TOOLS
        inline int32_t GetSelectedOptionIndex() const { return m_selectedMaskIndex; }
        #endif

    private:

        virtual void InitializeInternal( GraphContext& context ) override;
        virtual void ShutdownInternal( GraphContext& context ) override;
        virtual void GetValueInternal( GraphContext& context, void* pOutValue ) override;

        BoneMaskTaskList const* GetBoneMaskForIndex( GraphContext& context, int32_t optionIndex ) const;

        inline int32_t TrySelectMask( GraphContext& context ) const
        {
            return VectorFindIndex( GetSettings<BoneMaskSelectorNode>()->m_parameterValues, m_pParameterValueNode->GetValue<StringID>( context ) );
        }

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        IDValueNode*                                        m_pParameterValueNode = nullptr;
        BoneMaskValueNode*                                  m_pDefaultMaskValueNode = nullptr;
        TInlineVector<BoneMaskValueNode*, 7>                m_boneMaskOptionNodes;
        int32_t                                             m_selectedMaskIndex = InvalidIndex;
        int32_t                                             m_newMaskIndex = InvalidIndex;
        BoneMaskTaskList                                    m_taskList;
        Seconds                                             m_currentTimeInBlend = 0;
        bool                                                m_isBlending = false;
    };
}