#include "SettingsRegistry.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "IniFile.h"

//-------------------------------------------------------------------------

namespace EE
{
    bool SettingsRegistry::Initialize( FileSystem::Path const& iniFilePath )
    {
        EE_ASSERT( m_settings.empty() );

        TVector<TypeSystem::TypeInfo const*> settingsTypes = m_typeRegistry.GetAllDerivedTypes( Settings::GetStaticTypeID(), false, false, false );
        for ( auto pTypeInfo : settingsTypes )
        {
            m_settings.emplace_back( Cast<Settings>( pTypeInfo->CreateType() ) );
        }

        //-------------------------------------------------------------------------

        EE_ASSERT( iniFilePath );
        m_iniFilePath = iniFilePath;

        // If the ini file doesnt exist, create one with the default settings!
        IniFile iniFile( iniFilePath );
        if ( !iniFile.IsValid() )
        {
            if ( !SaveSettingsToIniFile() )
            {
                EE_LOG_FATAL_ERROR( LogCategory::System, "Settings Registry", "Failed to generate default INI file: %s", iniFilePath.c_str() );
                return false;
            }

            // Try to load the newly created ini file
            iniFile = IniFile( iniFilePath );
            if ( !iniFile.IsValid() )
            {
                EE_LOG_FATAL_ERROR( LogCategory::System, "Settings Registry", "Failed to load settings from INI file: %s", iniFilePath.c_str() );
                return false;
            }
        }

        //-------------------------------------------------------------------------

        for ( auto pGlobalSettings : m_settings )
        {
            if ( !pGlobalSettings->LoadSettings( m_typeRegistry, iniFile ) )
            {
                return false;
            }
        }

        return true;
    }

    void SettingsRegistry::Shutdown()
    {
        m_iniFilePath.Clear();

        //-------------------------------------------------------------------------

        for ( auto pSettings : m_settings )
        {
            EE::Delete( pSettings );
        }

        m_settings.clear();
    }

    bool SettingsRegistry::SaveSettingsToIniFile()
    {
        IniFile iniFile;

        for ( auto pGlobalSettings : m_settings )
        {
            if ( !pGlobalSettings->SaveSettings( m_typeRegistry, iniFile ) )
            {
                return false;
            }
        }

        return iniFile.SaveToFile( m_iniFilePath );
    }
}