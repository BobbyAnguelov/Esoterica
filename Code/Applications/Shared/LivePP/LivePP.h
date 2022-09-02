#pragma once

#if EE_ENABLE_LPP
#include "LPP_API_x64_CPP.h"
#endif

//-------------------------------------------------------------------------

namespace EE
{
    #if EE_ENABLE_LPP
    class [[nodiscard]] ScopedLPPAgent
    {
    public:

        ScopedLPPAgent()
        {
            lpp::LppProjectPreferences preferences = lpp::LppCreateDefaultProjectPreferences();
            preferences.unitySplitting.isEnabled = false;

            m_agent = lpp::LppCreateDefaultAgentWithPreferences( L"../../External/LivePP", &preferences );
            m_agent.EnableModule( lpp::LppGetCurrentModulePath(), lpp::LPP_MODULES_OPTION_ALL_IMPORT_MODULES );
        }

        ~ScopedLPPAgent()
        {
            lpp::LppDestroyDefaultAgent( &m_agent );
        }

    private:

        lpp::LppDefaultAgent m_agent;
    };
    #endif
}