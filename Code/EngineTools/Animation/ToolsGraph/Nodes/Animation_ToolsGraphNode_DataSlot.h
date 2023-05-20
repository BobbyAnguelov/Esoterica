#pragma once
#include "Animation_ToolsGraphNode.h"

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

            EE_REFLECT( "IsToolsReadOnly" : true );
            StringID               m_variationID;

            EE_REFLECT( "IsToolsReadOnly" : true );
            ResourceID             m_resourceID;
        };

    public:

        virtual char const* GetName() const override { return m_name.c_str(); }
        virtual bool IsRenameable() const override { return true; }
        virtual void SetName( String const& newName ) override;

        virtual void DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext ) override;
        virtual void Initialize( VisualGraph::BaseGraph* pParent ) override;
        virtual void OnDoubleClick( VisualGraph::UserContext* pUserContext ) override;
        virtual void PostPaste() override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited ) override;
        #endif

        // Slot
        //-------------------------------------------------------------------------

        virtual char const* GetDefaultSlotName() const { return "Slot"; }

        // What resource is expected in this slot
        virtual ResourceTypeID GetSlotResourceTypeID() const { return ResourceTypeID(); }

        // Should this node be created when we drop a resource of this type into the graph
        virtual bool IsDragAndDropTargetForResourceType( ResourceTypeID typeID ) const { return GetSlotResourceTypeID() == typeID; }

        // This will return the final resolved resource value for this slot
        ResourceID GetResourceID( VariationHierarchy const& variationHierarchy, StringID variationID ) const;

        // This sets the resource for the default variation
        void SetDefaultResourceID( ResourceID const& defaultResourceID )
        {
            EE_ASSERT( defaultResourceID.GetResourceTypeID() == GetSlotResourceTypeID() );
            m_defaultResourceID = defaultResourceID;
        }

        // Variation override management
        //-------------------------------------------------------------------------

        ResourceID const& GetDefaultValue() const { return m_defaultResourceID; }
        void SetDefaultValue( ResourceID const& resourceID );

        bool HasOverride( StringID variationID ) const;
        ResourceID const* GetOverrideValue( StringID variationID ) const;
        void SetOverrideValue( StringID variationID, ResourceID const& resourceID );

        void CreateOverride( StringID variationID );
        void RenameOverride( StringID oldVariationID, StringID newVariationID );
        void RemoveOverride( StringID variationID );

    private:

        String GetUniqueSlotName( String const& desiredName );

    protected:

        EE_REFLECT( "IsToolsReadOnly" : true );
        String                      m_name;

        EE_REFLECT( "IsToolsReadOnly" : true );
        ResourceID                  m_defaultResourceID;

        EE_REFLECT( "IsToolsReadOnly" : true );
        TVector<OverrideValue>      m_overrides;
    };
}