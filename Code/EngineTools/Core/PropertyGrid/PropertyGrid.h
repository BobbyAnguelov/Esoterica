#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceFilePicker.h"
#include "System/Imgui/ImguiX.h"
#include "System/TypeSystem/TypeInfo.h"
#include "System/TypeSystem/RegisteredType.h"
#include "System/Types/Event.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem 
{
    class TypeRegistry; 
    class PropertyInfo;
}

namespace EE::Resource { class ResourceDatabase; }

//-------------------------------------------------------------------------
// Property grid
//-------------------------------------------------------------------------
// Allows you to edit registered types

namespace EE
{
    class ToolsContext;
    namespace TypeSystem{ class PropertyEditor; }

    //-------------------------------------------------------------------------

    struct PropertyEditInfo
    {
        enum class Action
        {
            Invalid = -1,
            Edit,
            AddArrayElement,
            RemoveArrayElement,
        };

    public:

        IRegisteredType*                        m_pEditedTypeInstance = nullptr;
        TypeSystem::PropertyInfo const*         m_pPropertyInfo = nullptr;
        Action                                  m_action = Action::Invalid;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API PropertyGrid
    {
        friend struct ScopedChangeNotifier;

    public:

        PropertyGrid( ToolsContext const* pToolsContext );
        ~PropertyGrid();

        // Get the current edited type
        IRegisteredType const* GetEditedType() const { return m_pTypeInstance; }

        // Get the current edited type
        IRegisteredType* GetEditedType() { return m_pTypeInstance; }

        // Set the type instance to edit, will reset dirty status
        void SetTypeToEdit( IRegisteredType* pTypeInstance );

        // Set the type instance to edit, will reset dirty status
        void SetTypeToEdit( nullptr_t );

        // Display the grid
        void DrawGrid();

        // Has the type been modified
        inline bool IsDirty() const { return m_isDirty; }

        // Clear the modified flag i.e. when changes have been committed/saved
        void ClearDirty() { m_isDirty = false; }

        // Manually flag the grid as dirty
        void MarkDirty() { m_isDirty = true; }

        // Expand all properties
        void ExpandAllPropertyViews();

        // Expand all properties
        void CollapseAllPropertyViews();

        //-------------------------------------------------------------------------

        // Event fired just before a property is modified
        inline TEventHandle<PropertyEditInfo const&> OnPreEdit() { return m_preEditEvent; }

        // Event fired just after a property was modified
        inline TEventHandle<PropertyEditInfo const&> OnPostEdit() { return m_postEditEvent; }

    private:

        void DrawPropertyRow( TypeSystem::TypeInfo const* pTypeInfo, IRegisteredType* pTypeInstance, TypeSystem::PropertyInfo const& propertyInfo, uint8_t* propertyInstance );
        void DrawValuePropertyRow( TypeSystem::TypeInfo const* pTypeInfo, IRegisteredType* pTypeInstance, TypeSystem::PropertyInfo const& propertyInfo, uint8_t* propertyInstance, int32_t arrayIdx = InvalidIndex );
        void DrawArrayPropertyRow( TypeSystem::TypeInfo const* pTypeInfo, IRegisteredType* pTypeInstance, TypeSystem::PropertyInfo const& propertyInfo, uint8_t* propertyInstance );

        TypeSystem::PropertyEditor* GetPropertyEditor( TypeSystem::PropertyInfo const& propertyInfo, uint8_t* pActualPropertyInstance );
        void DestroyPropertyEditors();

    private:

        ToolsContext const*                                         m_pToolsContext;
        TypeSystem::TypeInfo const*                                 m_pTypeInfo = nullptr;
        Resource::ResourceFilePicker                                m_resourcePicker;
        IRegisteredType*                                            m_pTypeInstance = nullptr;
        bool                                                        m_isDirty = false;

        TEvent<PropertyEditInfo const&>                             m_preEditEvent; // Fired just before we change a property value
        TEvent<PropertyEditInfo const&>                             m_postEditEvent; // Fired just after we change a property value
        THashMap<void*, TypeSystem::PropertyEditor*>                m_propertyEditors;
    };
}