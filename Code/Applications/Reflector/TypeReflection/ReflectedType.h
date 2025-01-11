#pragma once
#include "ReflectedProperty.h"
#include "ReflectedEnumConstant.h"
#include "Base/TypeSystem/CoreTypeIDs.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
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

        constexpr static char const* const s_reflectedTypeInterfaceClassName = "IReflectedType";
        constexpr static char const* const s_defaultInstanceCustomCtorArgTypename = "EE::DefaultInstanceCtor_t";
        constexpr static char const* const s_baseEntityClassName = "Entity";
        constexpr static char const* const s_baseEntityComponentClassName = "EntityComponent";
        constexpr static char const* const s_baseEntitySystemClassName = "IEntitySystem";

    public:

        ReflectedType() = default;
        ReflectedType( TypeID ID, String const& name );

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
        String GetInternalNamespace() const;
        String GetCategory() const;

        // Generate additional type info
        inline bool HasProperties() const { return !m_properties.empty(); }
        bool HasArrayProperties() const;
        bool HasDynamicArrayProperties() const;
        bool HasResourcePtrProperties() const;
        bool HasResourcePtrOrStructProperties() const;

    public:

        TypeID                                          m_ID;
        StringID                                        m_headerID;
        String                                          m_name = "Invalid";
        String                                          m_namespace;
        TBitFlags<Flags>                                m_flags;

        // Structures
        TypeID                                          m_parentID;
        TVector<ReflectedProperty>                      m_properties;

        // Enums
        CoreTypeID                                      m_underlyingType = CoreTypeID::Uint8;
        TVector<ReflectedEnumConstant>                  m_enumConstants;

        bool                                            m_hasCustomDefaultInstanceCtor = false;
        bool                                            m_isDevOnly = true;
    };
}