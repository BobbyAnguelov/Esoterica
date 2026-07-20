#pragma once
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/TypeSystem/CoreTypeIDs.h"
#include "Base/Utils/CategoryTree.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem { class TypeRegistry; }

//-------------------------------------------------------------------------

namespace EE
{
    class IniFile;

    //-------------------------------------------------------------------------
    // Global System Settings
    //-------------------------------------------------------------------------
    // Allows for you to expose specific settings to the console via reflection
    // Those settings will then be saved/loaded from the engine ini file at startup
    //
    // Note: only certain core types are allows to be reflected and used for settings
    // 
    // These settings objects are also intended for transient settings that might be 
    // set via command-line at startup or any other scenario. These settings should not
    // be reflected...
    // 
    // Any derived settings objects will be automatically created and registered!

    class EE_BASE_API Settings : public IReflectedType
    {
        EE_REFLECT_TYPE( Settings );

        friend class SystemSettingsEditorTool;

    public:

        static bool IsSupportedSettingType( TypeSystem::PropertyInfo const& propertyInfo );

    public:

        // Get the name of the section in the INI file that the settings will go under
        virtual char const* GetSectionName() const = 0;

        // Load settings from an INI file
        bool LoadSettings( TypeSystem::TypeRegistry const& typeRegistry, IniFile const& ini );

        // Save settings from an INI file
        bool SaveSettings( TypeSystem::TypeRegistry const& typeRegistry, IniFile& ini ) const;

        // Override to load custom data from the ini file
        virtual bool LoadCustomSettings( TypeSystem::TypeRegistry const& typeRegistry, IniFile const& ini ) { return true; }

        // Override to save custom data to the ini file
        virtual bool SaveCustomSettings( TypeSystem::TypeRegistry const& typeRegistry, IniFile& ini ) const { return true; }

    protected:

        // Called after we finish loading the settings from an INI file, or a setting has been modified via an editor
        // Returns true if all settings are in a well-formed state
        virtual void PostLoadOrModify() {}

    private:

        #if EE_DEVELOPMENT_TOOLS
        CategoryTree<TypeSystem::PropertyInfo const*> m_categorizedConsoleEditableProperties;
        #endif
    };
}