#pragma once
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Systems.h"
#include "Settings.h"

//-------------------------------------------------------------------------

namespace EE { class Console; }
namespace EE::TypeSystem { class TypeInfo; }

//-------------------------------------------------------------------------

namespace EE::Settings
{
    class EE_BASE_API SettingsRegistry : public ISystem
    {
        friend EE::Console;

    public:

        EE_SYSTEM( SettingsRegistry );

    private:

        struct Group
        {
            Group() = default;
            Group( uint64_t ID, String const& name = "" ) : m_groupID(ID), m_name(name) {}

            template<typename T>
            T* GetSettings()
            {
                static_assert( std::is_base_of<EE::Settings::ISettings, T>::value, "T must be derived from ISettings" );
                static_assert( !std::is_base_of<EE::Settings::GlobalSettings, T>::value, "T is not allowed to be derived from GlobalSettings" );

                for ( auto pSettings : m_settings )
                {
                    T* pT = TryCast<T>( pSettings );
                    if ( pT != nullptr )
                    {
                        return pT;
                    }
                }

                return nullptr;
            }

            template<typename T>
            T const* GetSettings() const
            {
                static_assert( std::is_base_of<EE::Settings::ISettings, T>::value, "T must be derived from ISettings" );
                static_assert( !std::is_base_of<EE::Settings::GlobalSettings, T>::value, "T is not allowed to be derived from GlobalSettings" );
                return const_cast<Group*>( this )->GetSettings<T>();
            }

            ISettings* GetSettings( TypeSystem::TypeInfo const* pTypeInfo );

            ISettings const* GetSettings( TypeSystem::TypeInfo const* pTypeInfo ) const { return const_cast<Group*>( this )->GetSettings( pTypeInfo ); }

        public:

            uint64_t                m_groupID = 0xFFFFFFFF;
            String                  m_name;
            TVector<ISettings*>     m_settings;
        };

    public:

        SettingsRegistry( TypeSystem::TypeRegistry const& typeRegistry ) : m_typeRegistry( typeRegistry ) {}

        bool Initialize( FileSystem::Path const& iniFilePath );
        void Shutdown();

        #if EE_DEVELOPMENT_TOOLS
        bool SaveGlobalSettingsToIniFile();
        #endif

        // Global Settings
        //-------------------------------------------------------------------------

        // Get the settings object of the specified type
        template<typename T>
        T* GetGlobalSettings()
        {
            static_assert( std::is_base_of<EE::Settings::GlobalSettings, T>::value, "T must be derived from GlobalSettings" );

            for ( auto pSettings : m_globalSettings )
            {
                T* pT = TryCast<T>( pSettings );
                if ( pT != nullptr )
                {
                    return pT;
                }
            }

            return nullptr;
        }

        // Get the settings object of the specified type
        template<typename T>
        T const* GetGlobalSettings() const
        {
            static_assert( std::is_base_of<EE::Settings::GlobalSettings, T>::value, "T must be derived from GlobalSettings" );
            return const_cast<SettingsRegistry*>( this )->GetGlobalSettings<T>();
        }

        // Dynamic Settings
        //-------------------------------------------------------------------------
        // You can create settings object at runtime and group them via ID
        // You are only allowed a single settings object of a given type per group

        template<typename T>
        T* CreateSettings( uint64_t groupID )
        {
            static_assert( std::is_base_of<EE::Settings::ISettings, T>::value, "T must be derived from ISettings" );
            static_assert( !std::is_base_of<EE::Settings::GlobalSettings, T>::value, "T is not allowed to be derived from GlobalSettings" );

            auto pGroup = FindGroup( groupID );
            EE_ASSERT( pGroup != nullptr );
            EE_ASSERT( pGroup->GetSettings<T>() == nullptr );

            T* pSettings = pGroup->m_settings.emplace_back( EE::New<T>() );
            return pSettings;
        }

        ISettings* CreateSettings( uint64_t groupID, TypeSystem::TypeInfo const* pTypeInfo );

        // Get the settings object of the specified type
        template<typename T>
        T* GetSettings( uint64_t groupID )
        {
            static_assert( std::is_base_of<EE::Settings::ISettings, T>::value, "T must be derived from ISettings" );
            static_assert( !std::is_base_of<EE::Settings::GlobalSettings, T>::value, "T is not allowed to be derived from GlobalSettings" );

            Group* pGroup = FindGroup( groupID );
            return ( pGroup != nullptr ) ? pGroup->GetSettings<T>() : nullptr;
        }

        // Get the settings object of the specified type
        template<typename T>
        T const* GetSettings( uint64_t groupID ) const
        {
            static_assert( std::is_base_of<EE::Settings::ISettings, T>::value, "T must be derived from ISettings" );
            static_assert( !std::is_base_of<EE::Settings::GlobalSettings, T>::value, "T is not allowed to be derived from GlobalSettings" );
            return const_cast<SettingsRegistry*>( this )->GetSettings<T>( groupID );
        }

        // Get the settings object of the specified type
        Settings::ISettings* GetSettings( uint64_t groupID, TypeSystem::TypeInfo const* pTypeInfo )
        {
            Group* pGroup = FindGroup( groupID );
            return ( pGroup != nullptr ) ? pGroup->GetSettings( pTypeInfo ) : nullptr;
        }

        // Get the settings object of the specified type
        Settings::ISettings const* GetSettings( uint64_t groupID,  TypeSystem::TypeInfo const* pTypeInfo ) const { return const_cast<SettingsRegistry*>( this )->GetSettings( groupID, pTypeInfo ); }

        // Create a group with a specific ID
        void CreateGroup( uint64_t groupID, String const& friendlyName = "" );

        // Destroy a group and all settings for it
        void DestroyGroup( uint64_t groupID );

    private:

        Group* FindGroup( uint64_t groupID );

    private:

        TypeSystem::TypeRegistry const&         m_typeRegistry;
        FileSystem::Path                        m_iniFilePath;
        TVector<GlobalSettings*>                m_globalSettings;
        TVector<Group>                          m_groups;
    };
}