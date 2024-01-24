#include "Animation_ToolsGraph_Variations.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationGraph.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"
#include "Base/Serialization/TypeSerialization.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    EE::StringID const Variation::s_defaultVariationID( "Default" );

    //-------------------------------------------------------------------------

    String Variation::GenerateResourceFilePath( FileSystem::Path const& graphPath, StringID variationID )
    {
        return String( String::CtorSprintf(), "%s%c%s.agv", graphPath.c_str(), ResourcePath::s_pathDelimiter, variationID.c_str() );
    }

    String Variation::GetVariationNameFromResourceID( ResourceID const& resourceID )
    {
        String variationName;
        if ( resourceID.IsSubResourceID() )
        {
            variationName = resourceID.GetFileNameWithoutExtension();
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
        if ( resourceID.GetResourceTypeID() == GraphVariation::GetStaticResourceTypeID() && resourceID.IsSubResourceID() )
        {
            graphResourceID = resourceID.GetParentResourceID();

            if ( pOutOptionalVariation != nullptr )
            {
                *pOutOptionalVariation = StringID( resourceID.GetFileNameWithoutExtension() );
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
        EE_ASSERT( IsValidVariation( parentVariationID ) );

        auto& createdVariation = m_variations.emplace_back();
        createdVariation.m_ID = variationID;
        createdVariation.m_parentID = parentVariationID;
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

    bool VariationHierarchy::Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& objectValue )
    {
        m_variations.clear();

        for ( auto& variationObjectValue : objectValue.GetArray() )
        {
            auto& newVariation = m_variations.emplace_back();
            Serialization::ReadNativeType( typeRegistry, variationObjectValue, &newVariation );
        }

        return true;
    }

    void VariationHierarchy::Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const
    {
        writer.StartArray();

        int32_t const numVariations = (int32_t) m_variations.size();
        for ( int32_t i = 0; i < numVariations; i++ )
        {
            Serialization::WriteNativeType( typeRegistry, &m_variations[i], writer );
        }

        writer.EndArray();
    }
}