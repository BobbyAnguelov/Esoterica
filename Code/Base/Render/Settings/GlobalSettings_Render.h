#pragma once

#include "Base/Settings/Settings.h"
#include "Base/Math/Math.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class RenderGlobalSettings : public Settings::GlobalSettings
    {
        EE_REFLECT_TYPE( RenderGlobalSettings );

    public:

        virtual bool LoadSettings( Settings::IniFile const& ini ) override;
        virtual bool SaveSettings( Settings::IniFile& ini ) const override;

    public:

        Int2    m_resolution = Int2( 1280, 720 );
        float   m_refreshRate = 60.0f;
        bool    m_isFullscreen = false;
    };
}