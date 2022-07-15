#pragma once
#include "System/Animation/AnimationSkeleton.h"
#include "System/Resource/ResourcePtr.h"
#include "System/TypeSystem/RegisteredType.h"
#include "System/Types/StringID.h"
#include "System/Serialization/JsonSerialization.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem { class TypeRegistry;}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    struct Variation : public IRegisteredType
    {
        EE_REGISTER_TYPE( Variation );

        inline bool IsValid() const { return m_ID.IsValid(); }
        inline bool operator==( StringID const& variationID ) const { return m_ID == variationID; }
        inline bool operator!=( StringID const& variationID ) const { return m_ID != variationID; }

        EE_EXPOSE      StringID                m_ID;
        EE_REGISTER    StringID                m_parentID;
        EE_EXPOSE      TResourcePtr<Skeleton>  m_pSkeleton;
    };

    //-------------------------------------------------------------------------

    class VariationHierarchy
    {

    public:

        VariationHierarchy();

        void Reset();

        // Does the specified variation exist?
        inline bool IsValidVariation( StringID variationID ) const { return eastl::find( m_variations.begin(), m_variations.end(), variationID ) != m_variations.end(); }

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