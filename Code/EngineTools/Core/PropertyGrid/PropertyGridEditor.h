#pragma once

#include "EngineTools/Core/Helpers/GlobalRegistryBase.h"
#include "EngineTools/_Module/API.h"
#include "System/TypeSystem/CoreTypeIDs.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ToolsContext;
    namespace Resource { class ResourcePicker; }
    namespace TypeSystem { class PropertyInfo; }
}

//-------------------------------------------------------------------------

namespace EE::PG
{
    class EE_ENGINETOOLS_API PropertyEditor
    {

    public:

        PropertyEditor( ToolsContext const* pToolsContext, Resource::ResourcePicker& resourcePicker, TypeSystem::PropertyInfo const& propertyInfo, void* pPropertyInstance );
        virtual ~PropertyEditor() = default;

        // Returns true if the value was updated
        bool UpdateAndDraw();

        // Actually set the real property value from the working copy
        virtual void UpdatePropertyValue() = 0;

        // Called to reset the cached working copy value to that of the actual instance
        virtual void ResetWorkingCopy() = 0;

    protected:

        // Check if the external value is different from the cached working copy and if so updates the cached working copy
        virtual void HandleExternalUpdate() = 0;

        // Draw the editor widgets and handle the user input, returns true if the value has been updated
        virtual bool InternalUpdateAndDraw() = 0;

    protected:

        ToolsContext const*                 m_pToolsContext;
        TypeSystem::PropertyInfo const&     m_propertyInfo;
        void*                               m_pPropertyInstance;
        TypeSystem::CoreTypeID const        m_coreType;
    };

    //-------------------------------------------------------------------------
    // Property Grid Editor Factory
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API PropertyGridEditorFactory : public TGlobalRegistryBase<PropertyGridEditorFactory>
    {
        EE_DECLARE_GLOBAL_REGISTRY( PropertyGridEditorFactory );

    public:

        virtual ~PropertyGridEditorFactory() = default;

        static PropertyEditor* TryCreateEditor( ToolsContext const* pToolsContext, Resource::ResourcePicker& resourcePicker, TypeSystem::PropertyInfo const& propertyInfo, void* pPropertyInstance );

    protected:

        // Get the type that that this factory can create an editor for
        virtual TypeSystem::TypeID GetSupportedTypeID() const = 0;

        // Virtual method that will create a editor if the property type ID matches the appropriate types
        virtual PropertyEditor* TryCreateEditorInternal( ToolsContext const* pToolsContext, Resource::ResourcePicker& resourcePicker, TypeSystem::PropertyInfo const& propertyInfo, void* pPropertyInstance ) const = 0;
    };

    //-------------------------------------------------------------------------
    //  Macro to create a property grid editor factory
    //-------------------------------------------------------------------------
    // Use in a CPP to define a factory e.g., EE_PROPERTY_GRID_EDITOR( ObjectSettingsEditorFactory, ObjectSettings, ObjectSettingsEditor );

    #define EE_PROPERTY_GRID_EDITOR( factoryName, editedType, propertyGridEditorClass )\
    class factoryName final : public PG::PropertyGridEditorFactory\
    {\
        virtual TypeSystem::TypeID GetSupportedTypeID() const override { return editedType::GetStaticTypeID(); }\
        virtual PG::PropertyEditor* TryCreateEditorInternal( ToolsContext const* pToolsContext, Resource::ResourcePicker& resourcePicker, TypeSystem::PropertyInfo const& propertyInfo, void* pPropertyInstance ) const override\
        {\
            EE_ASSERT( propertyInfo.IsValid() );\
            EE_ASSERT( pPropertyInstance != nullptr );\
            return EE::New<propertyGridEditorClass>( pToolsContext, resourcePicker, propertyInfo, pPropertyInstance );\
        }\
    };\
    static factoryName g_##factoryName;
}