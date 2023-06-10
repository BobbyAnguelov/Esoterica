#pragma once

#include "EngineTools/_Module/API.h"
#include "System/Utils/GlobalRegistryBase.h"
#include "System/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE 
{
    class StringID;
    class ToolsContext;
}

//-------------------------------------------------------------------------
// Type Editing Rules
//-------------------------------------------------------------------------
// These classes provide additional editing rules/guidelines for the property grid to use when editing a given type

namespace EE::PG
{
    class EE_ENGINETOOLS_API TypeEditingRules
    {
    public:

        virtual ~TypeEditingRules() = default;

    public:

        virtual bool IsReadOnly( StringID const& propertyID ) = 0;
        virtual bool IsHidden( StringID const& propertyID ) = 0;
    };

    //-------------------------------------------------------------------------

    template<typename T>
    class EE_ENGINETOOLS_API TTypeEditingRules : public TypeEditingRules
    {
        static_assert( std::is_base_of<EE::IReflectedType, T>::value, "T is not derived from IReflectedType" );

    public:

        TTypeEditingRules( ToolsContext const* pToolsContext, T* pTypeInstance )
            : m_pTypeInstance( pTypeInstance )
        {}

    protected:

        T* m_pTypeInstance = nullptr;
    };

    //-------------------------------------------------------------------------
    // Factory
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API TypeEditingRulesFactory : public TGlobalRegistryBase<TypeEditingRulesFactory>
    {
        EE_DECLARE_GLOBAL_REGISTRY( TypeEditingRulesFactory );

    public:

        virtual ~TypeEditingRulesFactory() = default;

        static TypeEditingRules* TryCreateRules( ToolsContext const* pToolsContext, IReflectedType* pTypeInstance );

    protected:

        // Get the type that that this factory can create an helper for
        virtual TypeSystem::TypeID GetSupportedTypeID() const = 0;

        // Virtual method that will create a helper if the type ID matches the appropriate type
        virtual TypeEditingRules* TryCreateRulesInternal( ToolsContext const* pToolsContext, IReflectedType* pTypeInstance ) const = 0;
    };

    //-------------------------------------------------------------------------
    //  Macro to create a factory
    //-------------------------------------------------------------------------
    // Use in a CPP to define a factory e.g., EE_PROPERTY_GRID_EDITING_RULES( ObjectSettingsHelperFactory, ObjectSettings );

    #define EE_PROPERTY_GRID_EDITING_RULES( factoryName, editedType, rulesClass )\
    class factoryName final : public PG::TypeEditingRulesFactory\
    {\
        virtual TypeSystem::TypeID GetSupportedTypeID() const override { return editedType::GetStaticTypeID(); }\
        virtual PG::TypeEditingRules* TryCreateRulesInternal( ToolsContext const* pToolsContext, IReflectedType* pTypeInstance ) const override\
        {\
            EE_ASSERT( pTypeInstance != nullptr );\
            EE_ASSERT( IsOfType<editedType>( pTypeInstance ) );\
            return EE::New<rulesClass>( pToolsContext, static_cast<editedType*>( pTypeInstance ) );\
        }\
    };\
    static factoryName g_##factoryName;

}