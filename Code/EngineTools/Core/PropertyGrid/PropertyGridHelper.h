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

namespace EE::PG
{
    class EE_ENGINETOOLS_API PropertyHelper
    {
    public:

        virtual ~PropertyHelper() = default;

    public:

        virtual bool IsReadOnly( StringID const& propertyID ) = 0;
        virtual bool IsHidden( StringID const& propertyID ) = 0;
    };

    //-------------------------------------------------------------------------

    template<typename T>
    class EE_ENGINETOOLS_API TPropertyHelper : public PropertyHelper
    {
        static_assert( std::is_base_of<EE::IReflectedType, T>::value, "T is not derived from IReflectedType" );

    public:

        TPropertyHelper( ToolsContext const* pToolsContext, T* pTypeInstance )
            : m_pTypeInstance( pTypeInstance )
        {}

    protected:

        T* m_pTypeInstance = nullptr;
    };

    //-------------------------------------------------------------------------
    // Property Grid Helper Factory
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API PropertyGridHelperFactory : public TGlobalRegistryBase<PropertyGridHelperFactory>
    {
        EE_DECLARE_GLOBAL_REGISTRY( PropertyGridHelperFactory );

    public:

        virtual ~PropertyGridHelperFactory() = default;

        static PropertyHelper* TryCreateHelper( ToolsContext const* pToolsContext, IReflectedType* pTypeInstance );

    protected:

        // Get the type that that this factory can create an helper for
        virtual TypeSystem::TypeID GetSupportedTypeID() const = 0;

        // Virtual method that will create a helper if the type ID matches the appropriate type
        virtual PropertyHelper* TryCreateHelperInternal( ToolsContext const* pToolsContext, IReflectedType* pTypeInstance ) const = 0;
    };

    //-------------------------------------------------------------------------
    //  Macro to create a property grid helper factory
    //-------------------------------------------------------------------------
    // Use in a CPP to define a factory e.g., EE_PROPERTY_GRID_HELPER( ObjectSettingsHelperFactory, ObjectSettings );

    #define EE_PROPERTY_GRID_HELPER( factoryName, editedType, helperClass )\
    class factoryName final : public PG::PropertyGridHelperFactory\
    {\
        virtual TypeSystem::TypeID GetSupportedTypeID() const override { return editedType::GetStaticTypeID(); }\
        virtual PG::PropertyHelper* TryCreateHelperInternal( ToolsContext const* pToolsContext, IReflectedType* pTypeInstance ) const override\
        {\
            EE_ASSERT( pTypeInstance != nullptr );\
            EE_ASSERT( IsOfType<editedType>( pTypeInstance ) );\
            return EE::New<helperClass>( pToolsContext, static_cast<editedType*>( pTypeInstance ) );\
        }\
    };\
    static factoryName g_##factoryName;

}