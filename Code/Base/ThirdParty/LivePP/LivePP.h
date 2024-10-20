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
            lpp::LppLocalPreferences localPreferences = lpp::LppCreateDefaultLocalPreferences();

            lpp::LppProjectPreferences projectPreferences = lpp::LppCreateDefaultProjectPreferences();
            projectPreferences.unitySplitting.isEnabled = false;
            projectPreferences.exceptionHandler.isEnabled = false;

            m_agent = lpp::LppCreateDefaultAgentWithPreferences( &localPreferences, L"../../External/LivePP", &projectPreferences );
            m_agent.EnableModule( lpp::LppGetCurrentModulePath(), lpp::LPP_MODULES_OPTION_ALL_IMPORT_MODULES, nullptr, nullptr );
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