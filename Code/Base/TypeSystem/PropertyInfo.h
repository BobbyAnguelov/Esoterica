#pragma once

#include "TypeID.h"
#include "Base/Types/BitFlags.h"
#include "Base/Types/String.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    class EE_BASE_API PropertyInfo
    {
    public:

        enum class Flags
        {
            IsArray = 0,
            IsDynamicArray,
            IsEnum,
            IsBitFlags,
            IsStructure,
        };

    public:

        PropertyInfo() = default;

        inline bool IsValid() const { return m_ID.IsValid(); }

        // General queries
        //-------------------------------------------------------------------------

        inline bool IsStructureProperty() const { return m_flags.IsFlagSet( Flags::IsStructure ); }
        inline bool IsEnumProperty() const { return m_flags.IsFlagSet( Flags::IsEnum ); }
        inline bool IsBitFlagsProperty() const { return m_flags.IsFlagSet( Flags::IsBitFlags ); }
        inline bool IsArrayProperty() const { return m_flags.IsFlagSet( Flags::IsArray ) || m_flags.IsFlagSet( Flags::IsDynamicArray ); }
        inline bool IsStaticArrayProperty() const { return m_flags.IsFlagSet( Flags::IsArray ) && !m_flags.IsFlagSet( Flags::IsDynamicArray ); }
        inline bool IsDynamicArrayProperty() const { return m_flags.IsFlagSet( Flags::IsDynamicArray ); }

        // Addressing functions
        //-------------------------------------------------------------------------
        // Warning! These are only valid if the type address is the immediate parent type!

        template<typename T>
        inline T* GetPropertyAddress( void* pTypeAddress ) const
        {
            EE_ASSERT( IsValid() );
            return reinterpret_cast<T*>( ( reinterpret_cast<uint8_t*>( pTypeAddress ) + m_offset ) );
        }

        template<typename T>
        inline T const* GetPropertyAddress( void const* pTypeAddress ) const
        {
            EE_ASSERT( IsValid() );
            return reinterpret_cast<T const*>( ( reinterpret_cast<uint8_t const*>( pTypeAddress ) + m_offset ) );
        }

        inline void* GetPropertyAddress( void* pTypeAddress ) const
        {
            EE_ASSERT( IsValid() );
            return reinterpret_cast<uint8_t*>( pTypeAddress ) + m_offset;
        }

        inline void const* GetPropertyAddress( void const* pTypeAddress ) const
        {
            EE_ASSERT( IsValid() );
            return reinterpret_cast<uint8_t const*>( pTypeAddress ) + m_offset;
        }

        // Default value
        //-------------------------------------------------------------------------
        // NB! no type-safety here, these functions are not for general use!

        template<typename T>
        inline T const& GetDefaultValue() const
        {
            EE_ASSERT( !IsArrayProperty() );
            T const& defaultValue = *reinterpret_cast<T const*>( m_pDefaultValue );
            return defaultValue;
        }

        inline void const* GetArrayDefaultElementPtr( int32_t elementIdx ) const
        {
            EE_ASSERT( IsArrayProperty() && m_pDefaultArrayData != nullptr );
            EE_ASSERT( elementIdx >= 0 && elementIdx < m_arraySize );
            EE_ASSERT( m_arraySize > 0 && m_arrayElementSize > 0 );
            uint8_t const* arrayDataPtr = (uint8_t const*) m_pDefaultArrayData;
            return arrayDataPtr + ( m_arrayElementSize * elementIdx );
        }

    public:

        StringID                    m_ID;                                   // Property ID
        TypeID                      m_typeID;                               // Property Type ID
        TypeID                      m_parentTypeID;                         // The type ID for the parent type that this property belongs to
        TypeID                      m_templateArgumentTypeID;               // A property's contained TypeID for templatized types i.e. the specialization type for a TResourcePtr
        int32_t                     m_size = -1;                            // uint8_t size of property / total array byte size for static array properties
        int32_t                     m_offset = -1;                          // The byte offset from its owning type
        int32_t                     m_arraySize = -1;                       // Number of elements the array (for static arrays this is the static dimensions, for dynamic arrays this is the default size)
        int32_t                     m_arrayElementSize = -1;                // uint8_t size of an individual array element
        void const*                 m_pDefaultValue = nullptr;              // A ptr to the default value of the property
        void const*                 m_pDefaultArrayData = nullptr;          // A ptr to the contained data within the default value array
        TBitFlags<Flags>            m_flags;                                // Info about property type

        #if EE_DEVELOPMENT_TOOLS
        bool                        m_isForDevelopmentUseOnly = false;      // Whether this property only exists in development builds
        bool                        m_isToolsReadOnly = false;              // Whether this property can be modified by any tools or if it's just a serializable value
        String                      m_friendlyName;
        String                      m_category;
        String                      m_description;                          // Generated from the comments for the property
        StringID                    m_customEditorID;                       // Should we use a custom property grid editor?
        #endif
    };
}