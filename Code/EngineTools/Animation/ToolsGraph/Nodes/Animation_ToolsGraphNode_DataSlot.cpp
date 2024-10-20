#include "Animation_ToolsGraphNode_DataSlot.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Variations.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    DataSlotToolsNode::DataSlotToolsNode()
        : FlowToolsNode()
    {
        Rename( GetDefaultSlotName() );
    }

    ResourceID DataSlotToolsNode::GetResolvedResourceID( VariationHierarchy const& variationHierarchy, StringID variationID ) const
    {
        EE_ASSERT( variationHierarchy.IsValidVariation( variationID ) );

        if ( variationID == Variation::s_defaultVariationID )
        {
            return m_defaultResourceID;
        }

        //-------------------------------------------------------------------------

        ResourceID resourceID;

        auto TryGetResourceID = [this, &resourceID] ( StringID variationID )
        {
            if ( variationID == Variation::s_defaultVariationID )
            {
                resourceID = m_defaultResourceID;
                return true;
            }

            for ( auto const& variation : m_overrides )
            {
                if ( variation.m_variationID == variationID )
                {
                    resourceID = variation.m_resourceID;
                    return true;
                }
            }

            return false;
        };

        //-------------------------------------------------------------------------

        // Try get the resource ID for this variation
        if ( TryGetResourceID( variationID ) )
        {
            return resourceID;
        }

        // Go up the hierarchy and return the first if a override exists
        StringID parentVariationID = variationHierarchy.GetParentVariationID( variationID );
        while ( parentVariationID.IsValid() )
        {
            if ( TryGetResourceID( parentVariationID ) )
            {
                break;
            }

            parentVariationID = variationHierarchy.GetParentVariationID( parentVariationID );
        }

        return resourceID;
    }

    void DataSlotToolsNode::SetVariationResourceID( ResourceID const& resourceID, StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() );
        EE_ASSERT( !resourceID.IsValid() || resourceID.GetResourceTypeID() == GetSlotResourceTypeID() );

        if ( variationID == Variation::s_defaultVariationID )
        {
            NodeGraph::ScopedNodeModification const snm( this );
            m_defaultResourceID = resourceID;
        }
        else
        {
            EE_ASSERT( HasVariationOverride( variationID ) );

            for ( auto& variation : m_overrides )
            {
                if ( variation.m_variationID == variationID )
                {
                    NodeGraph::ScopedNodeModification const snm( this );
                    variation.m_resourceID = resourceID;
                    return;
                }
            }
        }
    }

    ResourceID const* DataSlotToolsNode::GetVariationResourceID( StringID variationID ) const
    {
        EE_ASSERT( variationID.IsValid() );

        if ( variationID == Variation::s_defaultVariationID )
        {
            return &m_defaultResourceID;
        }
        else // Check all actual overrides
        {
            for ( auto& variation : m_overrides )
            {
                if ( variation.m_variationID == variationID )
                {
                    return &variation.m_resourceID;
                }
            }
        }

        return nullptr;
    }

    bool DataSlotToolsNode::HasVariationOverride( StringID variationID ) const
    {
        // This query makes no sense on the default variation
        EE_ASSERT( variationID != Variation::s_defaultVariationID );

        for ( auto& variation : m_overrides )
        {
            if ( variation.m_variationID == variationID )
            {
                return true;
            }
        }

        return false;
    }

    void DataSlotToolsNode::CreateVariationOverride( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() && variationID != Variation::s_defaultVariationID );
        EE_ASSERT( !HasVariationOverride( variationID ) );

        NodeGraph::ScopedNodeModification const snm( this );

        auto& createdOverride = m_overrides.emplace_back();
        createdOverride.m_variationID = variationID;
    }

    void DataSlotToolsNode::RenameVariationOverride( StringID oldVariationID, StringID newVariationID )
    {
        EE_ASSERT( oldVariationID.IsValid() && newVariationID.IsValid() );
        EE_ASSERT( oldVariationID != Variation::s_defaultVariationID && newVariationID != Variation::s_defaultVariationID );

        NodeGraph::ScopedNodeModification const snm( this );

        for ( auto& overrideValue : m_overrides )
        {
            if ( overrideValue.m_variationID == oldVariationID )
            {
                overrideValue.m_variationID = newVariationID;
            }
        }
    }

    void DataSlotToolsNode::RemoveVariationOverride( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() && variationID != Variation::s_defaultVariationID );

        NodeGraph::ScopedNodeModification const snm( this );

        for ( auto iter = m_overrides.begin(); iter != m_overrides.end(); ++iter )
        {
            if ( iter->m_variationID == variationID )
            {
                m_overrides.erase_unsorted( iter );
                return;
            }
        }

        EE_UNREACHABLE_CODE();
    }

    void DataSlotToolsNode::DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );

        DrawInternalSeparator( ctx );

        //-------------------------------------------------------------------------

        ResourceID const resourceID = GetResolvedResourceID( *pGraphNodeContext->m_pVariationHierarchy, pGraphNodeContext->m_selectedVariationID );
        if ( resourceID.IsValid() )
        {
            ImGui::Text( EE_ICON_CUBE" %s", resourceID.c_str() + 7 );
        }
        else
        {
            ImGui::Text( EE_ICON_CUBE_OUTLINE" Empty Slot!" );
        }

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y );

        //-------------------------------------------------------------------------

        FlowToolsNode::DrawExtraControls( ctx, pUserContext );
    }
}