#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/TypeSystem/TypeID.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ToolsContext;

    namespace TypeSystem
    {
        class TypeInfo;
    }

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API TypeInfoPicker final
    {
        struct Option
        {
            TypeSystem::TypeInfo const*     m_pTypeInfo = nullptr;
            String                          m_friendlyName;
            String                          m_filterableData;
        };

    public:

        TypeInfoPicker( ToolsContext const& toolsContext, TypeSystem::TypeID const& baseClassTypeID = TypeSystem::TypeID(), TypeSystem::TypeInfo const*  pSelectedTypeInfo = nullptr );

        // Update the widget and draws it, returns true if the type was updated
        bool UpdateAndDraw();

        // Disable the picker
        void SetDisabled( bool isDisabled ) { m_isPickerDisabled = isDisabled; }

        // Set the type of resource we wish to select
        void SetRequiredBaseClass( TypeSystem::TypeID baseClassTypeID );

        // Set the selected type
        void SetSelectedType( TypeSystem::TypeInfo const* pSelectedType );

        // Set the selected type
        void SetSelectedType( TypeSystem::TypeID typeID );

        // Get the currently selected type
        inline TypeSystem::TypeInfo const* GetSelectedType() const { return m_pSelectedTypeInfo; }

    private:

        // Generate the set of valid resource options
        void GenerateOptionsList();

        // Generate the set of filtered options
        void GenerateFilteredOptionList();

        // Create a human readable typeinfo name
        String ConstructFriendlyTypeInfoName( TypeSystem::TypeInfo const* pTypeInfo );

    private:

        ToolsContext const&                                         m_toolsContext;
        TypeSystem::TypeID                                          m_baseClassTypeID;
        TypeSystem::TypeInfo const*                                 m_pSelectedTypeInfo = nullptr;
        ImGuiX::FilterWidget                                        m_filterWidget;
        TVector<Option>                                             m_generatedOptions;
        TVector<Option>                                             m_filteredOptions;
        bool                                                        m_isComboOpen = false;
        bool                                                        m_isPickerDisabled = false;
    };
}