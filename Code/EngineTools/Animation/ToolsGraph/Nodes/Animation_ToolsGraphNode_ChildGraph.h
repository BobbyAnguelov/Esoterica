#pragma once
#include "Animation_ToolsGraphNode_DataSlot.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class ChildGraphToolsNode final : public DataSlotToolsNode
    {
        EE_REFLECT_TYPE( ChildGraphToolsNode );

    public:

        ChildGraphToolsNode();

        virtual GraphValueType GetValueType() const override { return GraphValueType::Pose; }
        virtual char const* GetTypeName() const override { return "Child Graph"; }
        virtual char const* GetCategory() const override { return "Animation/Graphs"; }
        virtual TBitFlags<GraphType> GetAllowedParentGraphTypes() const override { return TBitFlags<GraphType>( GraphType::BlendTree ); }
        virtual int16_t Compile( GraphCompilationContext& context ) const override;
        virtual void OnDoubleClick( VisualGraph::UserContext* pUserContext ) override;
        virtual ImColor GetTitleBarColor() const override { return ImGuiX::ImColors::Gold; }
        virtual void DrawContextMenuOptions( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext, Float2 const& mouseCanvasPos, VisualGraph::Flow::Pin* pPin ) override;

        virtual char const* GetDefaultSlotName() const override { return "Graph"; }
        virtual ResourceTypeID GetSlotResourceTypeID() const override;
    };

    //-------------------------------------------------------------------------

    struct OpenChildGraphCommand : public VisualGraph::AdvancedCommand
    {
        EE_REFLECT_TYPE( OpenChildGraphCommand );

        enum Option { OpenInPlace, OpenInNewEditor };

    public:

        OpenChildGraphCommand() = default;

        OpenChildGraphCommand( ChildGraphToolsNode* pSourceNode, Option option )
            : m_option( option )
        {
            EE_ASSERT( pSourceNode != nullptr );
            m_pCommandSourceNode = pSourceNode;
        }

    public:

        Option m_option = OpenInPlace;
    };

    //-------------------------------------------------------------------------

    struct ReflectParametersCommand : public VisualGraph::AdvancedCommand
    {
        EE_REFLECT_TYPE( ReflectParametersCommand );

        enum Option { FromParent, FromChild };

    public:

        ReflectParametersCommand() = default;

        ReflectParametersCommand( ChildGraphToolsNode* pSourceNode, Option option )
            : m_option( option )
        {
            EE_ASSERT( pSourceNode != nullptr );
            m_pCommandSourceNode = pSourceNode;
        }

    public:

        Option m_option = FromParent;
    };
}