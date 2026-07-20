#pragma once
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Systems.h"
#include "Settings.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem { class TypeInfo; }

//-------------------------------------------------------------------------

namespace EE
{
    class EE_BASE_API SettingsRegistry : public ISystem
    {
        friend class SystemSettingsEditorTool;

    public:

        EE_SYSTEM( SettingsRegistry );

    public:

        SettingsRegistry( TypeSystem::TypeRegistry const& typeRegistry ) : m_typeRegistry( typeRegistry ) {}

        bool Initialize( FileSystem::Path const& iniFilePath );
        void Shutdown();

        bool SaveSettingsToIniFile();

        // Global Settings
        //-------------------------------------------------------------------------

        // Get the settings object of the specified type
        template<typename T>
        T* GetSettings()
        {
            static_assert( std::is_base_of<Settings, T>::value, "T must be derived from GlobalSettings" );

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

        // Get the settings object of the specified type
        template<typename T>
        T const* GetSettings() const
        {
            static_assert( std::is_base_of<Settings, T>::value, "T must be derived from GlobalSettings" );
            return const_cast<SettingsRegistry*>( this )->GetSettings<T>();
        }

    private:

        TypeSystem::TypeRegistry const&         m_typeRegistry;
        FileSystem::Path                        m_iniFilePath;
        TVector<Settings*>                      m_settings;
    };
}