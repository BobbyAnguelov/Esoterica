#pragma once
#include "Animation_ToolsGraphNode.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Variations.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    class DataSlotToolsNode : public FlowToolsNode
    {
        EE_REFLECT_TYPE( DataSlotToolsNode );

    public:

        struct OverrideValue : public IReflectedType
        {
            EE_REFLECT_TYPE( OverrideValue );

            EE_REFLECT( ReadOnly );
            StringID               m_variationID;

            EE_REFLECT( ReadOnly );
            ResourceID             m_resourceID;
        };

    public:

        DataSlotToolsNode();

        virtual bool IsRenameable() const override { return true; }
        virtual bool RequiresUniqueName() const override final { return true; }
        virtual void DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

        // Slot
        //-------------------------------------------------------------------------

        virtual char const* GetDefaultSlotName() const { return "Slot"; }

        // What resource is expected in this slot
        virtual ResourceTypeID GetSlotResourceTypeID() const { return ResourceTypeID(); }

        // Should this node be created when we drop a resource of this type into the graph
        virtual bool IsDragAndDropTargetForResourceType( ResourceTypeID typeID ) const { return GetSlotResourceTypeID() == typeID; }

        // This will return the final resolved resource value for this slot
        ResourceID GetResolvedResourceID( VariationHierarchy const& variationHierarchy, StringID variationID ) const;

        // Variation override management
        //-------------------------------------------------------------------------

        void SetVariationResourceID( ResourceID const& resourceID, StringID variationID = Variation::s_defaultVariationID );
        ResourceID const* GetVariationResourceID( StringID variationID ) const;

        bool HasVariationOverride( StringID variationID ) const;
        void CreateVariationOverride( StringID variationID );
        void RenameVariationOverride( StringID oldVariationID, StringID newVariationID );
        void RemoveVariationOverride( StringID variationID );

    protected:

        EE_REFLECT( ReadOnly );
        ResourceID                  m_defaultResourceID;

        EE_REFLECT( ReadOnly );
        TVector<OverrideValue>      m_overrides;
    };
}