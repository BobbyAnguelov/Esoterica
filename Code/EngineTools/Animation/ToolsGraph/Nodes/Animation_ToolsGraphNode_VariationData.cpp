#include "Animation_ToolsGraphNode_VariationData.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Variations.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    VariationDataToolsNode::VariationDataToolsNode()
        : FlowToolsNode()
    {}

    VariationDataToolsNode::Data* VariationDataToolsNode::GetResolvedVariationData( VariationHierarchy const& variationHierarchy, StringID variationID )
    {
        EE_ASSERT( variationHierarchy.IsValidVariation( variationID ) );

        VariationDataToolsNode::Data* pVariationData = m_defaultVariationData.Get();

        if ( variationID == Variation::s_defaultVariationID )
        {
            return pVariationData;
        }

        //-------------------------------------------------------------------------

        auto TryGetVariationData = [this, &pVariationData] ( StringID variationID )
        {
            if ( variationID == Variation::s_defaultVariationID )
            {
                pVariationData = m_defaultVariationData.Get();
                return true;
            }

            for ( auto& variation : m_overrides )
            {
                if ( variation.m_variationID == variationID )
                {
                    pVariationData = variation.m_variationData.Get();
                    return true;
                }
            }

            return false;
        };

        //-------------------------------------------------------------------------

        // Try get the resource ID for this variation
        if ( TryGetVariationData( variationID ) )
        {
            return pVariationData;
        }

        // Go up the hierarchy and return the first if a override exists
        StringID parentVariationID = variationHierarchy.GetParentVariationID( variationID );
        while ( parentVariationID.IsValid() )
        {
            if ( TryGetVariationData( parentVariationID ) )
            {
                break;
            }

            parentVariationID = variationHierarchy.GetParentVariationID( parentVariationID );
        }

        //-------------------------------------------------------------------------

        return pVariationData;
    }

    bool VariationDataToolsNode::HasVariationOverride( StringID variationID ) const
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

    void VariationDataToolsNode::CreateVariationOverride( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() && variationID != Variation::s_defaultVariationID );
        EE_ASSERT( !HasVariationOverride( variationID ) );

        NodeGraph::ScopedNodeModification const snm( this );

        auto& createdOverride = m_overrides.emplace_back();
        createdOverride.m_variationID = variationID;
        createdOverride.m_variationData.CreateInstance( GetVariationDataTypeInfo() );
    }

    void VariationDataToolsNode::RenameVariationOverride( StringID oldVariationID, StringID newVariationID )
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

    void VariationDataToolsNode::RemoveVariationOverride( StringID variationID )
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

    void VariationDataToolsNode::DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext )
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );

        DrawInternalSeparator( ctx );

        //-------------------------------------------------------------------------

        VariationDataToolsNode::Data* pVariationData = GetResolvedVariationData( *pGraphNodeContext->m_pVariationHierarchy, pGraphNodeContext->m_selectedVariationID );

        TInlineVector<ResourceID, 2> referencedResources;
        pVariationData->GetReferencedResources( referencedResources );
        if ( !referencedResources.empty() )
        {
            for ( ResourceID const& ID : referencedResources )
            {
                ImGui::Text( EE_ICON_CUBE" %s", ID.c_str() + 7 );
            }
        }

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y );

        //-------------------------------------------------------------------------

        FlowToolsNode::DrawExtraControls( ctx, pUserContext );
    }
}