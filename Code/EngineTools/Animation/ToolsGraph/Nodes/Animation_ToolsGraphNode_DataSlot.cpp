#include "Animation_ToolsGraphNode_DataSlot.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Variations.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "System/Imgui/ImguiStyle.h"

//-------------------------------------------------------------------------

namespace EE::Animation::GraphNodes
{
    void DataSlotToolsNode::Initialize( VisualGraph::BaseGraph* pParent )
    {
        FlowToolsNode::Initialize( pParent );
        m_name = GetUniqueSlotName( GetDefaultSlotName() );
    }

    ResourceID DataSlotToolsNode::GetResourceID( VariationHierarchy const& variationHierarchy, StringID variationID ) const
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

    void DataSlotToolsNode::SetName( String const& newName )
    {
        EE_ASSERT( IsRenameable() );
        VisualGraph::ScopedNodeModification const snm( this );
        m_name = GetUniqueSlotName( newName );
    }

    void DataSlotToolsNode::SetDefaultValue( ResourceID const& resourceID )
    {
        EE_ASSERT( !resourceID.IsValid() || resourceID.GetResourceTypeID() == GetSlotResourceTypeID() );
        m_defaultResourceID = resourceID;
    }

    bool DataSlotToolsNode::HasOverride( StringID variationID ) const
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

    ResourceID const* DataSlotToolsNode::GetOverrideValue( StringID variationID ) const
    {
        EE_ASSERT( variationID.IsValid() );
        EE_ASSERT( HasOverride( variationID ) );

        // Check all actual overrides
        for ( auto& variation : m_overrides )
        {
            if ( variation.m_variationID == variationID )
            {
                return &variation.m_resourceID;
            }
        }

        EE_UNREACHABLE_CODE();
        return nullptr;
    }

    void DataSlotToolsNode::SetOverrideValue( StringID variationID, ResourceID const& resourceID )
    {
        VisualGraph::ScopedNodeModification const snm( this );

        EE_ASSERT( variationID.IsValid() );
        EE_ASSERT( !resourceID.IsValid() || resourceID.GetResourceTypeID() == GetSlotResourceTypeID() );
        EE_ASSERT( HasOverride( variationID ) );

        for ( auto& variation : m_overrides )
        {
            if ( variation.m_variationID == variationID )
            {
                variation.m_resourceID = resourceID;
                return;
            }
        }

        EE_UNREACHABLE_CODE();
    }

    void DataSlotToolsNode::CreateOverride( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() && variationID != Variation::s_defaultVariationID );
        EE_ASSERT( !HasOverride( variationID ) );

        VisualGraph::ScopedNodeModification const snm( this );

        auto& createdOverride = m_overrides.emplace_back();
        createdOverride.m_variationID = variationID;
    }

    void DataSlotToolsNode::RenameOverride( StringID oldVariationID, StringID newVariationID )
    {
        EE_ASSERT( oldVariationID.IsValid() && newVariationID.IsValid() );
        EE_ASSERT( oldVariationID != Variation::s_defaultVariationID && newVariationID != Variation::s_defaultVariationID );

        VisualGraph::ScopedNodeModification const snm( this );

        for ( auto& overrideValue : m_overrides )
        {
            if ( overrideValue.m_variationID == oldVariationID )
            {
                overrideValue.m_variationID = newVariationID;
            }
        }
    }

    void DataSlotToolsNode::RemoveOverride( StringID variationID )
    {
        EE_ASSERT( variationID.IsValid() && variationID != Variation::s_defaultVariationID );

        VisualGraph::ScopedNodeModification const snm( this );

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

    void DataSlotToolsNode::DrawExtraControls( VisualGraph::DrawContext const& ctx, VisualGraph::UserContext* pUserContext )
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );

        // Draw separator
        //-------------------------------------------------------------------------

        float const spacerWidth = Math::Max( GetWidth(), 40.0f );
        ImVec2 originalCursorPos = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton( "S1", ImVec2( spacerWidth, 10 ) );
        ctx.m_pDrawList->AddLine( originalCursorPos + ImVec2( 0, 4 ), originalCursorPos + ImVec2( GetWidth(), 4 ), ImColor( ImGuiX::Style::s_colorTextDisabled ) );

        //-------------------------------------------------------------------------

        ResourceID const resourceID = GetResourceID( *pGraphNodeContext->m_pVariationHierarchy, pGraphNodeContext->m_selectedVariationID );
        if ( resourceID.IsValid() )
        {
            ImGui::Text( EE_ICON_CUBE" %s", resourceID.c_str() + 7 );
        }
        else
        {
            ImGui::Text( EE_ICON_CUBE_OUTLINE" Empty Slot!" );
        }

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 4 );

        //-------------------------------------------------------------------------

        originalCursorPos = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton( "S2", ImVec2( spacerWidth, 10 ) );
        ctx.m_pDrawList->AddLine( originalCursorPos + ImVec2( 0, 4 ), originalCursorPos + ImVec2( GetWidth(), 4 ), ImColor( ImGuiX::Style::s_colorTextDisabled ) );

        //-------------------------------------------------------------------------

        FlowToolsNode::DrawExtraControls( ctx, pUserContext );
    }

    String DataSlotToolsNode::GetUniqueSlotName( String const& desiredName )
    {
        // Ensure that the slot name is unique within the same graph
        int32_t cnt = 0;
        String uniqueName = desiredName;
        auto const dataSlotNodes = GetParentGraph()->FindAllNodesOfType<DataSlotToolsNode>( VisualGraph::SearchMode::Localized, VisualGraph::SearchTypeMatch::Derived );
        for ( int32_t i = 0; i < (int32_t) dataSlotNodes.size(); i++ )
        {
            if ( dataSlotNodes[i] == this )
            {
                continue;
            }

            if ( dataSlotNodes[i]->GetName() == uniqueName )
            {
                uniqueName = String( String::CtorSprintf(), "%s_%d", desiredName.c_str(), cnt );
                cnt++;
                i = 0;
            }
        }

        return uniqueName;
    }

    void DataSlotToolsNode::PostPaste()
    {
        FlowToolsNode::PostPaste();
        m_name = GetUniqueSlotName( m_name );
    }

    void DataSlotToolsNode::OnDoubleClick( VisualGraph::UserContext* pUserContext )
    {
        auto pGraphNodeContext = static_cast<ToolsGraphUserContext*>( pUserContext );
        ResourceID const resourceID = GetResourceID( *pGraphNodeContext->m_pVariationHierarchy, pGraphNodeContext->m_selectedVariationID );
        if ( resourceID.IsValid() )
        {
            pGraphNodeContext->RequestOpenResource( resourceID );
        }
    }

    #if EE_DEVELOPMENT_TOOLS
    void DataSlotToolsNode::PostPropertyEdit( TypeSystem::PropertyInfo const* pPropertyEdited )
    {
        FlowToolsNode::PostPropertyEdit( pPropertyEdited );
        m_name = GetUniqueSlotName( m_name );
    }
    #endif
}