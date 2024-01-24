#include "GlobalSettings_Render.h"
#include "Base/Settings/IniFile.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    bool RenderGlobalSettings::LoadSettings( Settings::IniFile const& ini )
    {
        m_resolution.m_x = ini.GetIntOrDefault( "Render:ResolutionX", 1280 );
        m_resolution.m_y = ini.GetIntOrDefault( "Render:ResolutionY", 720 );
        m_refreshRate = ini.GetFloatOrDefault( "Render:RefreshRate", 60 );
        m_isFullscreen = ini.GetBoolOrDefault( "Render:Fullscreen", false );

        return true;
    }

    bool RenderGlobalSettings::SaveSettings( Settings::IniFile& ini ) const
    {
        ini.CreateSection( "Render" );
        ini.SetInt( "Render:ResolutionX", m_resolution.m_x );
        ini.SetInt( "Render:ResolutionY", m_resolution.m_y );
        ini.SetFloat( "Render:RefreshRate", m_refreshRate );
        ini.SetBool( "Render:Fullscreen", m_isFullscreen );

        return true;
    }
}