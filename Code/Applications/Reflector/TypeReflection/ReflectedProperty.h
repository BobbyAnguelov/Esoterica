#pragma once
#include "Base/TypeSystem/PropertyInfo.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    struct ReflectedProperty
    {

    public:

        ReflectedProperty() = default;

        ReflectedProperty( String const& name, int32_t lineNumber )
            : m_propertyID( name )
            , m_name( name )
            , m_lineNumber( lineNumber )
        {
        }

        ReflectedProperty( String const& name, String const& typeName, int32_t lineNumber )
            : m_propertyID( name )
            , m_name( name )
            , m_typeName( typeName )
            , m_lineNumber( lineNumber )
        {
        }

        inline bool IsStructureProperty() const { return m_flags.IsFlagSet( PropertyInfo::Flags::IsStructure ); }
        inline bool IsEnumProperty() const { return m_flags.IsFlagSet( PropertyInfo::Flags::IsEnum ); }
        inline bool IsBitFlagsProperty() const { return m_flags.IsFlagSet( PropertyInfo::Flags::IsBitFlags ); }
        inline bool IsTypeInstanceProperty() const { return m_flags.IsFlagSet( PropertyInfo::Flags::IsTypeInstance ); }
        inline bool IsArrayProperty() const { return m_flags.IsFlagSet( PropertyInfo::Flags::IsArray ) || m_flags.IsFlagSet( PropertyInfo::Flags::IsDynamicArray ); }
        inline bool IsStaticArrayProperty() const { return m_flags.IsFlagSet( PropertyInfo::Flags::IsArray ) && !m_flags.IsFlagSet( PropertyInfo::Flags::IsDynamicArray ); }
        inline bool IsDynamicArrayProperty() const { return m_flags.IsFlagSet( PropertyInfo::Flags::IsDynamicArray ); }
        inline bool IsResourcePtrProperty() const { return m_flags.IsFlagSet( PropertyInfo::Flags::IsResourcePtr ); }
        inline uint32_t GetArraySize() const { EE_ASSERT( m_arraySize > 0 ); return (uint32_t) m_arraySize; }

        inline bool operator==( ReflectedProperty const& RHS ) const { return m_propertyID == RHS.m_propertyID; }
        inline bool operator!=( ReflectedProperty const& RHS ) const { return m_propertyID != RHS.m_propertyID; }

        // MetaData
        //-------------------------------------------------------------------------

        void GenerateMetaData( InlineString& outWarnings );

    public:

        StringID                                        m_propertyID;
        int32_t                                         m_lineNumber = -1;
        TypeID                                          m_typeID;
        String                                          m_name;
        String                                          m_rawMetaDataStr;
        PropertyMetadata                                m_metaData;
        String                                          m_reflectedFriendlyName; // the generated friendly name from the c++ name
        String                                          m_reflectedDescription;
        String                                          m_typeName;
        String                                          m_templateArgTypeName;
        int32_t                                         m_arraySize = -1;
        TBitFlags<PropertyInfo::Flags>                  m_flags;
        bool                                            m_isDevOnly = true;
    };
}