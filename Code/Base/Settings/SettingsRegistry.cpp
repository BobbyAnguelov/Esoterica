#include "SettingsRegistry.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "IniFile.h"

//-------------------------------------------------------------------------

namespace EE::Settings
{
    ISettings* SettingsRegistry::Group::GetSettings( TypeSystem::TypeInfo const* pTypeInfo )
    {
        EE_ASSERT( pTypeInfo != nullptr );
        EE_ASSERT( pTypeInfo->IsDerivedFrom( ISettings::GetStaticTypeID() ) );
        EE_ASSERT( !pTypeInfo->IsDerivedFrom( GlobalSettings::GetStaticTypeID() ) );

        for ( auto pSettings : m_settings )
        {
            if ( pSettings->GetTypeID() == pTypeInfo->m_ID )
            {
                return pSettings;
            }
        }

        return nullptr;
    }

    //-------------------------------------------------------------------------

    bool SettingsRegistry::Initialize( FileSystem::Path const& iniFilePath )
    {
        EE_ASSERT( m_globalSettings.empty() );

        TVector<TypeSystem::TypeInfo const*> settingsTypes = m_typeRegistry.GetAllDerivedTypes( GlobalSettings::GetStaticTypeID(), false, false, false );
        for ( auto pTypeInfo : settingsTypes )
        {
            m_globalSettings.emplace_back( Cast<GlobalSettings>( pTypeInfo->CreateType() ) );
        }

        //-------------------------------------------------------------------------

        EE_ASSERT( iniFilePath );
        m_iniFilePath = iniFilePath;

        // If the ini file doesnt exist, create one with the default settings!
        Settings::IniFile iniFile( iniFilePath );
        if ( !iniFile.IsValid() )
        {
            #if EE_DEVELOPMENT_TOOLS
            if ( !SaveGlobalSettingsToIniFile() )
            {
                EE_LOG_FATAL_ERROR( "Settings", "Registry", "Failed to generate default INI file: %s", iniFilePath.c_str() );
                return false;
            }
            #endif

            // Try to load the newly created ini file
            iniFile = Settings::IniFile( iniFilePath );
            if ( !iniFile.IsValid() )
            {
                EE_LOG_FATAL_ERROR( "Settings", "Registry", "Failed to load settings from INI file: %s", iniFilePath.c_str() );
                return false;
            }
        }

        //-------------------------------------------------------------------------

        for ( auto pGlobalSettings : m_globalSettings )
        {
            if ( !pGlobalSettings->LoadSettings( iniFile ) )
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

        for ( auto pSettings : m_globalSettings )
        {
            EE::Delete( pSettings );
        }

        m_globalSettings.clear();
    }

    #if EE_DEVELOPMENT_TOOLS
    bool SettingsRegistry::SaveGlobalSettingsToIniFile()
    {
        Settings::IniFile iniFile;

        for ( auto pGlobalSettings : m_globalSettings )
        {
            if ( !pGlobalSettings->SaveSettings( iniFile ) )
            {
                return false;
            }
        }

        return iniFile.SaveToFile( m_iniFilePath );
    }
    #endif

    //-------------------------------------------------------------------------

    ISettings* SettingsRegistry::CreateSettings( uint64_t groupID, TypeSystem::TypeInfo const* pTypeInfo )
    {
        EE_ASSERT( pTypeInfo != nullptr );
        EE_ASSERT( pTypeInfo->IsDerivedFrom( ISettings::GetStaticTypeID() ) );
        EE_ASSERT( !pTypeInfo->IsDerivedFrom( GlobalSettings::GetStaticTypeID() ) );

        auto pGroup = FindGroup( groupID );
        EE_ASSERT( pGroup != nullptr );
        EE_ASSERT( pGroup->GetSettings( pTypeInfo ) == nullptr );

        ISettings* pSettings = pGroup->m_settings.emplace_back( Cast<ISettings>( pTypeInfo->CreateType() ) );
        return pSettings;
    }

    void SettingsRegistry::CreateGroup( uint64_t groupID, String const& friendlyName )
    {
        Group* pGroup = FindGroup( groupID );
        EE_ASSERT( pGroup == nullptr );
        m_groups.emplace_back( Group( groupID, friendlyName ) );
    }

    void SettingsRegistry::DestroyGroup( uint64_t groupID )
    {
        for ( auto iter = m_groups.begin(); iter != m_groups.end(); ++iter )
        {
            if ( iter->m_groupID == groupID )
            {
                for ( auto pSettings : iter->m_settings )
                {
                    EE::Delete( pSettings );
                }

                m_groups.erase_unsorted( iter );
                return;
            }
        }

        EE_UNREACHABLE_CODE();
    }

    SettingsRegistry::Group* SettingsRegistry::FindGroup( uint64_t groupID )
    {
        for ( auto& group : m_groups )
        {
            if ( group.m_groupID == groupID )
            {
                return &group;
            }
        }

        return nullptr;
    }
}