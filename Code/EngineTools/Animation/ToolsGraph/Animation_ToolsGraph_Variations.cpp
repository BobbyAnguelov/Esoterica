#include "Animation_ToolsGraph_Variations.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    StaticStringID const Variation::s_defaultVariationID( "Default" );

    //-------------------------------------------------------------------------

    DataPath Variation::GenerateResourceDataPath( FileSystem::Path const& sourceDataDirectoryPath, FileSystem::Path const& graphPath, StringID variationID )
    {
        String const pathStr( String::CtorSprintf(), "%s%c%s.%s", graphPath.c_str(), DataPath::s_pathDelimiter, variationID.c_str(), GraphDefinition::GetStaticResourceTypeID().ToString().c_str() );
        return DataPath::FromFileSystemPath( sourceDataDirectoryPath, pathStr );
    }

    String Variation::GetVariationNameFromResourceID( ResourceID const& resourceID )
    {
        String variationName;
        if ( resourceID.IsSubResourceID() )
        {
            variationName = resourceID.GetFilenameWithoutExtension();
        }
        else
        {
            variationName = Variation::s_defaultVariationID.c_str();
        }
        return variationName;
    }

    ResourceID Variation::GetGraphResourceID( ResourceID const& resourceID, StringID* pOutOptionalVariation )
    {
        ResourceID graphResourceID;
        
        // Try to extract the actual graph ID from the resource ID for variations
        if ( resourceID.GetResourceTypeID() == GraphDefinition::GetStaticResourceTypeID() && resourceID.IsSubResourceID() )
        {
            graphResourceID = resourceID.GetParentResourceID();

            if ( pOutOptionalVariation != nullptr )
            {
                *pOutOptionalVariation = StringID( resourceID.GetFilenameWithoutExtension() );
            }
        }
        else // Just pass through the resource ID
        {
            graphResourceID = resourceID;
        }

        return graphResourceID;
    }

    //-------------------------------------------------------------------------

    VariationHierarchy::VariationHierarchy()
    {
        Reset();
    }

    bool VariationHierarchy::IsValidVariation( StringID variationID ) const
    {
        if ( variationID == Variation::s_defaultVariationID )
        {
            return true;
        }

        for ( Variation const &variation : m_variations )
        {
            if ( variation.m_ID == variationID )
            {
                return true;
            }
        }

        return false;
    }

    void VariationHierarchy::Reset()
    {
        m_variations.clear();

        auto& defaultVariation = m_variations.emplace_back();
        defaultVariation.m_ID = Variation::s_defaultVariationID;
    }

    StringID VariationHierarchy::TryGetCaseCorrectVariationID( String const& variationName ) const
    {
        for ( auto const& variation : m_variations )
        {
            int32_t const result = variationName.comparei( variation.m_ID.c_str() );
            if ( result == 0 )
            {
                return variation.m_ID;
            }
        }

        return StringID();
    }

    Variation* VariationHierarchy::GetVariation( StringID variationID )
    {
        for ( auto& variation : m_variations )
        {
            if ( variation.m_ID == variationID )
            {
                return &variation;
            }
        }

        return nullptr;
    }

    StringID VariationHierarchy::GetParentVariationID( StringID variationID ) const
    {
        int32_t const variationIdx = GetVariationIndex( variationID );
        EE_ASSERT( variationIdx != InvalidIndex );

        StringID const parentID = m_variations[variationIdx].m_parentID;
        if ( !parentID.IsValid() )
        {
            EE_ASSERT( variationID == Variation::s_defaultVariationID );
        }

        return parentID;
    }

    TInlineVector<EE::StringID, 5> VariationHierarchy::GetChildVariations( StringID variationID ) const
    {
        EE_ASSERT( IsValidVariation( variationID ) );

        TInlineVector<EE::StringID, 5> childVariations;
        for ( auto const& variation : m_variations )
        {
            if ( variation.m_parentID == variationID )
            {
                childVariations.emplace_back( variation.m_ID );
            }
        }

        return childVariations;
    }

    void VariationHierarchy::CreateVariation( StringID variationID, StringID parentVariationID )
    {
        EE_ASSERT( variationID.IsValid() && parentVariationID.IsValid() );
        EE_ASSERT( !IsValidVariation( variationID ) );

        Variation const* pParentVariation = GetVariation( parentVariationID );
        EE_ASSERT( pParentVariation != nullptr );

        auto& createdVariation = m_variations.emplace_back();
        createdVariation.m_ID = variationID;
        createdVariation.m_parentID = parentVariationID;
        createdVariation.m_skeleton = pParentVariation->m_skeleton;
    }

    void VariationHierarchy::RenameVariation( StringID oldVariationID, StringID newVariationID )
    {
        EE_ASSERT( oldVariationID.IsValid() && newVariationID.IsValid() );
        EE_ASSERT( oldVariationID != Variation::s_defaultVariationID && newVariationID != Variation::s_defaultVariationID );
        EE_ASSERT( IsValidVariation( oldVariationID ) );
        EE_ASSERT( !IsValidVariation( newVariationID ) );

        for ( auto& variation : m_variations )
        {
            // Update ID
            if ( variation.m_ID == oldVariationID )
            {
                variation.m_ID = newVariationID;
            }

            // Update parents
            if ( variation.m_parentID == oldVariationID )
            {
                variation.m_parentID = newVariationID;
            }
        }
    }

    void VariationHierarchy::DestroyVariation( StringID variationID )
    {
        EE_ASSERT( variationID != Variation::s_defaultVariationID );

        // Delete the specified variation
        int32_t const variationIdx = GetVariationIndex( variationID );
        EE_ASSERT( variationIdx != InvalidIndex );
        m_variations.erase_unsorted( m_variations.begin() + variationIdx );

        // Delete all child variations
        while ( true )
        {
            bool variationRemoved = false;
            for ( int32_t i = (int32_t) m_variations.size() - 1; i >= 0; i-- )
            {
                if ( m_variations[i].m_ID == Variation::s_defaultVariationID )
                {
                    continue;
                }

                if ( !IsValidVariation( m_variations[i].m_parentID ) )
                {
                    m_variations.erase_unsorted( m_variations.begin() + i );
                    variationRemoved = true;
                }
            }

            if ( !variationRemoved )
            {
                break;
            }
        }
    }
}