#pragma once
#include "Animation_ToolsGraphNode.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Blend1D.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class Blend1DToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( Blend1DToolsNode );

    public:

        struct BlendSpacePoint : public IReflectedType
        {
            EE_REFLECT_TYPE( BlendSpacePoint );

        public:

            BlendSpacePoint() = default;
            BlendSpacePoint( String const& name, float value, UUID const& pinID ) : m_name( name ), m_value( value ), m_pinID( pinID ) {}

            inline bool operator<( BlendSpacePoint const& rhs ) const { return m_value < rhs.m_value; }
            inline bool operator<=( BlendSpacePoint const& rhs ) const { return m_value <= rhs.m_value; }

        public:

            EE_REFLECT();
            String                  m_name;

            EE_REFLECT();
            float                   m_value = 0.0f;

            EE_REFLECT( ReadOnly );
            UUID                    m_pinID;
        };

    public:

        Blend1DToolsNode();

        virtual char const* GetCategory() const override { return "Animation/Blends"; }
        virtual char const* GetTypeName() const override { return "Blend 1D"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override final { return TBitFlags<GraphType>( GraphType::BlendTree ); }

        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override { return "Option"; }
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::Pose ); }

    private:

        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) override;

        virtual void DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;
        virtual void OnDynamicPinCreation( UUID const& pinID ) override;
        virtual void PreDynamicPinDestruction( UUID const& pinID ) override;

        virtual void RefreshDynamicPins() override;

    private:

        EE_REFLECT( ShowAsStaticArray );
        TVector<BlendSpacePoint>     m_blendSpace;

        EE_REFLECT();
        bool                         m_allowLooping = true;
    };

    //-------------------------------------------------------------------------

    class VelocityBlendToolsNode final : public FlowToolsNode
    {
        EE_REFLECT_TYPE( VelocityBlendToolsNode );

    public:

        VelocityBlendToolsNode();

        virtual char const* GetCategory() const override { return "Animation/Blends"; }
        virtual char const* GetTypeName() const override { return "Blend 1D (Velocity)"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override final { return TBitFlags<GraphType>( GraphType::BlendTree ); }

        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override;
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::Pose ); }
        virtual bool IsValidConnection( UUID const& inputPinID, FlowNode const* pOutputPinNode, UUID const& outputPinID ) const override;

        virtual int16_t Compile( GraphCompilationContext& context ) const override;

    private:

        EE_REFLECT();
        bool                         m_allowLooping = true;
    };
}