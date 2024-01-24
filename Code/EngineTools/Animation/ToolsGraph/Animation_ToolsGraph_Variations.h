#pragma once
#include "Engine/Animation/AnimationSkeleton.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Types/StringID.h"
#include "Base/Serialization/JsonSerialization.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem { class TypeRegistry; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    struct Variation : public IReflectedType
    {
        EE_REFLECT_TYPE( Variation );

    public:

        // The default name for a graph variation
        static StringID const s_defaultVariationID;

        // Create a file path for the graph variation resource
        static String GenerateResourceFilePath( FileSystem::Path const& graphPath, StringID variationID );

        // Note: Resource paths are case-insensitive so the name will be lowercase
        static String GetVariationNameFromResourceID( ResourceID const& resourceID );

        // Get the graph resource ID from either a graph or a graph variation resource ID
        static ResourceID GetGraphResourceID( ResourceID const& resourceID, StringID* pOutOptionalVariation = nullptr );

    public:

        inline bool operator==( StringID const& variationID ) const { return m_ID == variationID; }
        inline bool operator!=( StringID const& variationID ) const { return m_ID != variationID; }

    public:

        EE_REFLECT()
        StringID                m_ID;

        EE_REFLECT( "IsToolsReadOnly" : true )
        StringID                m_parentID;

        EE_REFLECT()
        TResourcePtr<Skeleton>  m_skeleton;
    };

    //-------------------------------------------------------------------------

    class VariationHierarchy
    {

    public:

        VariationHierarchy();

        void Reset();

        // Does the specified variation exist?
        inline bool IsValidVariation( StringID variationID ) const { return eastl::find( m_variations.begin(), m_variations.end(), variationID ) != m_variations.end(); }

        // Get the actual case-correct variation ID - this is useful to give the users some degree of flexibility
        StringID TryGetCaseCorrectVariationID( String const& variationName ) const;

        // Get the actual case-correct variation ID - this is useful to give the users some degree of flexibility
        StringID TryGetCaseCorrectVariationID( StringID caseInsensitiveVariationID ) const { return TryGetCaseCorrectVariationID( caseInsensitiveVariationID.c_str() ); }

        // Get a specified variation
        Variation* GetVariation( StringID variationID );

        // Get a specified variation
        Variation const* GetVariation( StringID variationID ) const { return const_cast<VariationHierarchy*>( this )->GetVariation( variationID ); }

        // Returns the ID of the parent variation (invalid ID if called for the default variation)
        StringID GetParentVariationID( StringID variationID ) const;

        // Get all children for a specified variation
        TInlineVector<StringID, 5> GetChildVariations( StringID variationID ) const;

        // Get a flat list of all variations
        TVector<Variation> const& GetAllVariations() const { return m_variations; }

        // This will create a new variation under the specified parent variation
        void CreateVariation( StringID variationID, StringID parentVariationID );

        // This will rename the specified variation
        void RenameVariation( StringID oldVariationID, StringID newVariationID );

        // This will destroy the specified variation and all children
        void DestroyVariation( StringID variationID );

        // Serialization
        bool Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& objectValue );
        void Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const;

    private:

        inline int32_t GetVariationIndex( StringID variationID ) const
        {
            auto iter = eastl::find( m_variations.begin(), m_variations.end(), variationID );
            if ( iter != m_variations.end() )
            {
                return int32_t( iter - m_variations.begin() );
            }

            return InvalidIndex;
        }

    private:

        TVector<Variation>                          m_variations;
    };
}