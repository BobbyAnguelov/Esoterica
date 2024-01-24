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

            bool operator!=( BlendSpace const& rhs );

        public:

            EE_REFLECT();
            TVector<StringID>       m_pointIDs;

            EE_REFLECT();
            TVector<Float2>         m_points;

            EE_REFLECT( "IsToolsReadOnly" : true );
            TVector<uint8_t>        m_indices;

            EE_REFLECT( "IsToolsReadOnly" : true );
            TVector<uint8_t>        m_hullIndices;
        };

    public:

        Blend2DToolsNode();

        virtual char const* GetTypeName() const override { return "Blend 2D"; }
        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetCategory() const override { return "Animation/Blends"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override final { return TBitFlags<GraphType>( GraphType::BlendTree ); }

        virtual bool SupportsUserEditableDynamicInputPins() const override { return true; }
        virtual TInlineString<100> GetNewDynamicInputPinName() const override { return "Input"; }
        virtual StringID GetDynamicInputPinValueType() const override { return GetPinTypeForValueType( GraphValueType::Pose ); }

    private:

        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void SerializeCustom( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& nodeObjectValue ) override;
        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) override;

        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;
        virtual void OnDynamicPinCreation( UUID pinID ) override;
        virtual void OnDynamicPinDestruction( UUID pinID ) override;

        void UpdateDynamicPins();

    private:

        EE_REFLECT();
        BlendSpace                  m_blendSpace;
    };
}