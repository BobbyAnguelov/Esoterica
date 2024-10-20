#pragma once
#include "Base/Types/StringID.h"
#include "Base/Types/BitFlags.h"
#include "Base/Types/String.h"

//-------------------------------------------------------------------------
// Property Metadata
//-------------------------------------------------------------------------
// Extra information about properties that is relevant for the tools code
// Primarily the property grid

#if EE_DEVELOPMENT_TOOLS
namespace EE::TypeSystem
{
    struct PropertyMetadata
    {
        // Currently Supported Meta Data:
        enum Flag : int8_t
        {
            Unknown = -1, // Any unrecognized key/values will have this flag
            FriendlyName = 0, // Generated from the c++ property name or through an explicit definition in the reflect macro
            Description, // Generated from either the comments for the property or an explicit definition in the reflect macro
            Category, // Group properties together
            Hidden, // Hides this property in any property grids or property visualizers
            ReadOnly, // Is this visible but read-only in a property grid
            ShowAsStaticArray, // Disallows size manipulation for dynamic arrays
            Min, // Add minimum value clamp - relevant for multiple types
            Max, // Add maximum value clamp - relevant for multiple types
            DisableTypePicker, // Only applicable to type-instance properties, disallow type editing via the property grid
            CustomEditor, // Should we use a custom property grid editor?
        };

        inline constexpr static char const* const s_flagStrings[] =
        {
            "FriendlyName",
            "Description",
            "Category",
            "Hidden",
            "ReadOnly",
            "ShowAsStaticArray",
            "Min",
            "Max",
            "DisableTypePicker",
            "CustomEditor"
        };

        inline constexpr static int32_t const s_numFlags = sizeof( s_flagStrings ) / sizeof( s_flagStrings[0] );

        // A parsed metadata entry
        struct KV
        {
            KV() = default;
            explicit KV( Flag k, String const& v ) : m_key( k ), m_value( v ) {}
            explicit KV( String const& k, String const& v ) : m_key( Flag::Unknown ), m_keyValue( k ), m_value( v ) {}
            explicit KV( String const& k, String const& v, double f ) : m_key( Flag::Unknown ), m_keyValue( k ), m_value( v ) {}

            Flag                m_key;
            String              m_keyValue; // Only relevant for unknown metadata
            String              m_value; // Not all entries have a value so this can be empty
        };

    public:

        void Clear()
        {
            m_flags.ClearAllFlags();
            m_keyValues.clear();
        }

        // Get the friendly name - always exists
        inline String const& GetFriendlyName() const { return m_keyValues[FriendlyName].m_value; }

        // Get the description - always exists
        inline String const& GetDescription() const { return m_keyValues[Description].m_value; }

        //-------------------------------------------------------------------------

        inline bool HasFlag( Flag f ) const
        { 
            return m_flags.IsFlagSet( f );
        }

        KV const* TryGetEntryForFlag( Flag f ) const
        {
            for ( KV const& kv : m_keyValues )
            {
                if ( kv.m_key == f )
                {
                    return &kv;
                }
            }

            return nullptr;
        }

        inline String GetValue( Flag f, String const& defaultValue = String() ) const
        {
            KV const* pKV = TryGetEntryForFlag( f );
            return ( pKV == nullptr ) ? defaultValue : pKV->m_value;
        }

        // Default assumes an integer type
        template<typename T>
        inline bool TryGetNumericValue( Flag f, T const defaultValue, T& outValue ) const
        {
            KV const* pKV = TryGetEntryForFlag( f );
            if ( pKV != nullptr )
            {
                char *pEnd = nullptr;
                double dblValue = std::strtod( pKV->m_value.c_str(), &pEnd );
                if ( pEnd != pKV->m_value.c_str() )
                {
                    outValue = static_cast<T>( round( dblValue ) );
                    return true;
                }
            }

            outValue = defaultValue;
            return false;
        }

        template<>
        inline bool TryGetNumericValue( Flag f, float const defaultValue, float& outValue ) const
        {
            KV const* pKV = TryGetEntryForFlag( f );
            if ( pKV != nullptr )
            {
                char *pEnd = nullptr;
                outValue = std::strtof( pKV->m_value.c_str(), &pEnd );
                if ( pEnd != pKV->m_value.c_str() )
                {
                    return true;
                }
            }

            outValue = defaultValue;
            return false;
        }

        template<>
        inline bool TryGetNumericValue( Flag f, double const defaultValue, double& outValue ) const
        {
            KV const* pKV = TryGetEntryForFlag( f );
            if ( pKV != nullptr )
            {
                char *pEnd = nullptr;
                outValue = std::strtod( pKV->m_value.c_str(), &pEnd );
                if ( pEnd != pKV->m_value.c_str() )
                {
                    return true;
                }
            }

            outValue = defaultValue;
            return false;
        }

    public:

        TBitFlags<Flag>         m_flags;
        TVector<KV>             m_keyValues;
    };
}
#endif