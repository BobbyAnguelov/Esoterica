#pragma once

#include "ReflectionProjectTypes.h"
#include "System/TypeSystem/TypeInfo.h"
#include "System/TypeSystem/CoreTypeIDs.h"
#include "System/Resource/ResourceTypeID.h"

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
        {}

        ReflectedProperty( String const& name, String const& typeName, int32_t lineNumber )
            : m_propertyID( name )
            , m_name( name )
            , m_typeName( typeName )
            , m_lineNumber( lineNumber )
        {}

        inline bool IsStructureProperty() const { return m_flags.IsFlagSet( PropertyInfo::Flags::IsStructure ); }
        inline bool IsEnumProperty() const { return m_flags.IsFlagSet( PropertyInfo::Flags::IsEnum ); }
        inline bool IsBitFlagsProperty() const { return m_flags.IsFlagSet( PropertyInfo::Flags::IsBitFlags ); }
        inline bool IsArrayProperty() const { return m_flags.IsFlagSet( PropertyInfo::Flags::IsArray ) || m_flags.IsFlagSet( PropertyInfo::Flags::IsDynamicArray ); }
        inline bool IsStaticArrayProperty() const { return m_flags.IsFlagSet( PropertyInfo::Flags::IsArray ) && !m_flags.IsFlagSet( PropertyInfo::Flags::IsDynamicArray ); }
        inline bool IsDynamicArrayProperty() const { return m_flags.IsFlagSet( PropertyInfo::Flags::IsDynamicArray ); }
        inline uint32_t GetArraySize() const { EE_ASSERT( m_arraySize > 0 ); return (uint32_t) m_arraySize; }

        inline bool operator==( ReflectedProperty const& RHS ) const { return m_propertyID == RHS.m_propertyID; }
        inline bool operator!=( ReflectedProperty const& RHS ) const { return m_propertyID != RHS.m_propertyID; }

        // Dev Info
        //-------------------------------------------------------------------------

        String GetFriendlyName() const;
        String GetCategory() const { return m_category; }

        // MetaData
        //-------------------------------------------------------------------------

        bool HasMetaData() const { return !m_metaData.empty(); }
        void ParseMetaData();

    public:

        StringID                                        m_propertyID;
        int32_t                                         m_lineNumber = -1;
        TypeID                                          m_typeID;
        String                                          m_name;
        String                                          m_metaData;
        String                                          m_description;
        String                                          m_typeName;
        String                                          m_templateArgTypeName;
        int32_t                                         m_arraySize = -1;
        TBitFlags<PropertyInfo::Flags>                  m_flags;
        bool                                            m_isDevOnly = true;

        // From MetaData
        String                                          m_category;
        bool                                            m_isToolsReadOnly = false;
        StringID                                        m_customEditorID;
    };

    //-------------------------------------------------------------------------

    struct ReflectedEnumConstant
    {
        StringID                                        m_ID;
        String                                          m_label;
        int32_t                                         m_value;
        String                                          m_description;
    };

    struct ReflectedType
    {
        enum class Flags
        {
            IsAbstract = 0,
            IsEnum,
            IsEntity,
            IsEntityComponent,
            IsEntitySystem,
            IsEntityWorldSystem
        };

    public:

        ReflectedType() = default;

        ReflectedType( TypeID ID, String const& name )
            : m_ID( ID )
            , m_name( name )
        {}

        inline bool IsAbstract() const { return m_flags.IsFlagSet( Flags::IsAbstract ); }
        inline bool IsEnum() const { return m_flags.IsFlagSet( Flags::IsEnum ); }
        inline bool IsEntity() const { return m_flags.IsFlagSet( Flags::IsEntity ); }
        inline bool IsEntityComponent() const { return m_flags.IsFlagSet( Flags::IsEntityComponent ); }
        inline bool IsEntitySystem() const { return m_flags.IsFlagSet( Flags::IsEntitySystem ); }
        inline bool IsEntityWorldSystem() const { return m_flags.IsFlagSet( Flags::IsEntityWorldSystem ); }

        // Structure functions
        ReflectedProperty const* GetPropertyDescriptor( StringID propertyID ) const;

        // Enum functions
        void AddEnumConstant( ReflectedEnumConstant const& constant );
        bool IsValidEnumLabelID( StringID labelID ) const;
        bool GetValueFromEnumLabel( StringID labelID, uint32_t& value ) const;

        // Dev tools helpers
        String GetFriendlyName() const;
        String GetCategory() const;

        // Generate additional type info
        inline bool HasProperties() const { return !m_properties.empty(); }
        bool HasArrayProperties() const;
        bool HasDynamicArrayProperties() const;
        bool HasResourcePtrProperties() const;
        bool HasResourcePtrOrStructProperties() const;

    public:

        TypeID                                          m_ID;
        HeaderID                                        m_headerID;
        String                                          m_name = "Invalid";
        String                                          m_namespace;
        TBitFlags<Flags>                                m_flags;

        // Structures
        TypeID                                          m_parentID;
        TVector<ReflectedProperty>                      m_properties;

        // Enums
        CoreTypeID                                      m_underlyingType = CoreTypeID::Uint8;
        TVector<ReflectedEnumConstant>                  m_enumConstants;

        bool                                            m_isDevOnly = true;
    };

    //-------------------------------------------------------------------------

    struct ReflectedResourceType
    {
        // Fill the resource type ID and the friendly name from the macro registration string
        bool TryParseResourceRegistrationMacroString( String const& registrationStr );

    public:

        TypeID                                          m_typeID;
        ResourceTypeID                                  m_resourceTypeID;
        String                                          m_friendlyName;
        HeaderID                                        m_headerID;
        String                                          m_className;
        String                                          m_namespace;
        TVector<TypeID>                                 m_parents;
    };
}