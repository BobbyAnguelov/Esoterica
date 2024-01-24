#pragma once
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE::Settings
{
    class IniFile;

    //-------------------------------------------------------------------------

    class EE_BASE_API ISettings : public IReflectedType
    {
        EE_REFLECT_TYPE( ISettings );
    };

    //-------------------------------------------------------------------------
    // System Settings
    //-------------------------------------------------------------------------
    // Derived from this type to create a set of global setting that will be
    // saved to, and loaded from, the engine ini file.
    //
    // Derived global settings objects will be automatically created and registered!

    class EE_BASE_API GlobalSettings : public ISettings
    {
        EE_REFLECT_TYPE( GlobalSettings );

        virtual bool LoadSettings( IniFile const& ini ) = 0;
        virtual bool SaveSettings( IniFile& ini ) const = 0;
    };
}