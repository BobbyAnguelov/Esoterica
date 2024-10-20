#pragma once
#include "Animation_ToolsGraphNode.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class Blend2DToolsNode : public FlowToolsNode
    {
        EE_REFLECT_TYPE( Blend2DToolsNode );

    public:

        struct BlendSpace : public IReflectedType
        {
            EE_REFLECT_TYPE( BlendSpace );

        public:

            void AddPoint();
            void RemovePoint( int32_t idx );
            inline int32_t GetNumPoints() const { return (int32_t) m_points.size(); }

            bool GenerateTriangulation();

            bool operator!=( BlendSpace const& rhs ) const;

        public:

            EE_REFLECT();
            TVector<String>         m_pointIDs;

            EE_REFLECT();
            TVector<Float2>         m_points;

            EE_REFLECT( ReadOnly );
            TVector<uint8_t>        m_indices;

            EE_REFLECT( ReadOnly );
            TVector<uint8_t>        m_hullIndices;
        };

    public:

        Blend2DToolsNode();

        virtual char const* GetTypeName() const override { return "Blend 2D"; }
        virtual char const* GetCategory() const override { return "Animation/Blends"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override final { return TBitFlags<GraphType>( GraphType::BlendTree ); }

        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override { return "Input"; }
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::Pose ); }

    private:

        virtual void PostDeserialize() override;

        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) override;

        virtual void DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;
        virtual void OnDynamicPinCreation( UUID const& pinID ) override;
        virtual void PreDynamicPinDestruction( UUID const& pinID ) override;

        virtual void RefreshDynamicPins() override;

    private:

        EE_REFLECT();
        BlendSpace                  m_blendSpace;

        EE_REFLECT();
        bool                         m_allowLooping = true;
    };
}