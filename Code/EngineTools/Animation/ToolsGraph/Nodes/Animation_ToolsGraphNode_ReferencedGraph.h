#pragma once
#include "Animation_ToolsGraphNode_VariationData.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class ReferencedGraphToolsNode final : public VariationDataToolsNode
    {
        EE_REFLECT_TYPE( ReferencedGraphToolsNode );

        struct Data final : public VariationDataToolsNode::Data
        {
            EE_REFLECT_TYPE( Data );

            virtual void GetReferencedResources( TInlineVector<ResourceID, 2>& outReferencedResources ) const override { outReferencedResources.emplace_back( m_graphDefinition.GetResourceID() ); }

        public:

            EE_REFLECT();
            TResourcePtr<GraphDefinition>     m_graphDefinition;
        };

    public:

        ReferencedGraphToolsNode();

        virtual char const* GetTypeName() const override { return "Child Graph"; }
        virtual char const* GetCategory() const override { return "Animation/Graphs"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual Color GetTitleBarColor() const override { return Colors::Gold; }
        virtual void DrawContextMenuOptions( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, NodeGraph::Pin* pPin ) override;

        ResourceID GetReferencedGraphResourceID( VariationHierarchy const& variationHierarchy, StringID variationID ) const;

    private:

        virtual TypeSystem::TypeInfo const* GetVariationDataTypeInfo() const override { return ReferencedGraphToolsNode::Data::s_pTypeInfo; }
    };

    //-------------------------------------------------------------------------

    struct OpenReferencedGraphCommand : public NodeGraph::CustomCommand
    {
        EE_REFLECT_TYPE( OpenReferencedGraphCommand );

        enum Option { OpenInPlace, OpenInNewEditor };

    public:

        OpenReferencedGraphCommand() = default;

        OpenReferencedGraphCommand( ReferencedGraphToolsNode* pSourceNode, Option option )
            : m_option( option )
        {
            EE_ASSERT( pSourceNode != nullptr );
            m_pCommandSourceNode = pSourceNode;
        }

    public:

        Option m_option = OpenInPlace;
    };

    //-------------------------------------------------------------------------

    struct ReflectParametersCommand : public NodeGraph::CustomCommand
    {
        EE_REFLECT_TYPE( ReflectParametersCommand );

        enum Option { FromParent, FromChild };

    public:

        ReflectParametersCommand() = default;

        ReflectParametersCommand( ReferencedGraphToolsNode* pSourceNode, Option option )
            : m_option( option )
        {
            EE_ASSERT( pSourceNode != nullptr );
            m_pCommandSourceNode = pSourceNode;
        }

    public:

        Option m_option = FromParent;
    };
}