#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Core/ToolsContext.h"
#include "Base/TypeSystem/CoreTypeIDs.h"
#include "Base/Utils/GlobalRegistryBase.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace Resource { class ResourcePicker; }
    namespace TypeSystem { class PropertyInfo; }
}

//-------------------------------------------------------------------------

namespace EE::PG
{
    struct PropertyEditorContext
    {
        ToolsContext const*     m_pToolsContext = nullptr;
        void*                   m_pUserContext = nullptr; // Additional context provided to the property grid for specialized use cases
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API PropertyEditor
    {

    public:

        enum Result
        {
            None,
            ValueUpdated,
            ValueUpdatedAndGridNeedsRebuild // This is necessary if you have messed with any other property state than your own or a child of your own property
        };

    public:

        PropertyEditor( PropertyEditorContext const& context, TypeSystem::PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance );
        virtual ~PropertyEditor() = default;

        // Returns true if the working copy value was updated and the property instance value now needs to be updated
        // Warning: DO NOT actually update the instance value in this function as it is not safe to do so!!!
        Result UpdateAndDraw();

        // Actually set the real property value from the working copy
        virtual void UpdatePropertyValue() = 0;

        // Called to reset the cached working copy value to that of the actual instance
        virtual void ResetWorkingCopy() = 0;

        // Optional: Allows you to provide a name override for a given property
        // The array element idx is set if this property is an array element
        virtual InlineString GetPropertyNameOverride( int32_t arrayElementIdx ) const { return InlineString(); }

    protected:

        // Check if the external value is different from the cached working copy and if so updates the cached working copy
        virtual void HandleExternalUpdate() = 0;

        // Draw the editor widgets and handle the user input, returns true if the value has been updated
        virtual Result InternalUpdateAndDraw() = 0;

        // Get type registry
        EE_FORCE_INLINE TypeSystem::TypeRegistry const* GetTypeRegistry() const { return m_context.m_pToolsContext->m_pTypeRegistry; }

    protected:

        PropertyEditorContext               m_context;
        TypeSystem::PropertyInfo const&     m_propertyInfo;
        IReflectedType*                     m_pTypeInstance = nullptr;
        void*                               m_pPropertyInstance = nullptr;
        TypeSystem::CoreTypeID const        m_coreType;
    };

    //-------------------------------------------------------------------------
    // Property Grid Editor Factory
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API PropertyGridEditorFactory : public TGlobalRegistryBase<PropertyGridEditorFactory>
    {
        EE_GLOBAL_REGISTRY( PropertyGridEditorFactory );

    public:

        virtual ~PropertyGridEditorFactory() = default;

        static PropertyEditor* TryCreateEditor( PropertyEditorContext const& context, TypeSystem::PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance );

    protected:

        // Does this factory support this type
        virtual bool SupportsTypeID( TypeSystem::TypeID typeID ) const = 0;

        // Get the type that that this factory can create an editor for
        virtual bool SupportsCustomEditorID( StringID customEditorID ) const = 0;

        // Virtual method that will create a editor if the property type ID matches the appropriate types
        virtual PropertyEditor* TryCreateEditorInternal( PropertyEditorContext const& context, TypeSystem::PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance ) const = 0;
    };

    //-------------------------------------------------------------------------
    //  Macro to create a property grid editor factory
    //-------------------------------------------------------------------------
    // Use in a CPP to define a factory e.g., EE_PROPERTY_GRID_EDITOR( ObjectSettingsEditorFactory, ObjectSettings, ObjectSettingsEditor );

    #define EE_PROPERTY_GRID_TYPE_EDITOR( factoryName, editedType, propertyGridEditorClass )\
    class factoryName final : public PG::PropertyGridEditorFactory\
    {\
        virtual bool SupportsTypeID( TypeSystem::TypeID typeID ) const override { return editedType::GetStaticTypeID() == typeID; }\
        virtual bool SupportsCustomEditorID( StringID customEditorID ) const override { return false; }\
        virtual PG::PropertyEditor* TryCreateEditorInternal( PG::PropertyEditorContext const& context, TypeSystem::PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance ) const override\
        {\
            EE_ASSERT( propertyInfo.IsValid() );\
            EE_ASSERT( pPropertyInstance != nullptr );\
            return EE::New<propertyGridEditorClass>( context, propertyInfo, pTypeInstance, pPropertyInstance );\
        }\
    };\
    static factoryName g_##factoryName;

    #define EE_PROPERTY_GRID_CUSTOM_EDITOR( factoryName, customEditorID, propertyGridEditorClass )\
    class factoryName final : public PG::PropertyGridEditorFactory\
    {\
        virtual bool SupportsTypeID( TypeSystem::TypeID typeID ) const override { return false; }\
        virtual bool SupportsCustomEditorID( StringID editorID ) const override { static StringID const supportedEditorID( customEditorID ); return editorID == supportedEditorID; }\
        virtual PG::PropertyEditor* TryCreateEditorInternal( PG::PropertyEditorContext const& context, TypeSystem::PropertyInfo const& propertyInfo, IReflectedType* pTypeInstance, void* pPropertyInstance ) const override\
        {\
            EE_ASSERT( propertyInfo.IsValid() );\
            EE_ASSERT( pPropertyInstance != nullptr );\
            return EE::New<propertyGridEditorClass>( context, propertyInfo, pTypeInstance, pPropertyInstance );\
        }\
    };\
    static factoryName g_##factoryName;
}