#pragma once

#include "ClangParserContext.h"
#include "Base/Time/Time.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    class ReflectedHeader;

    //-------------------------------------------------------------------------

    class ClangParser
    {
    public:

        enum Pass
        {
            DevToolsPass,
            NoDevToolsPass
        };

    public:

        ClangParser( FileSystem::Path const& solutionPath, ReflectionDatabase* pDatabase );

        inline Milliseconds GetParsingTime() const { return m_totalParsingTime; }
        inline Milliseconds GetVisitingTime() const { return m_totalVisitingTime; }

        bool Parse( TVector<ReflectedHeader*> const& headers, Pass pass );
        String GetErrorMessage() const { return m_context.GetErrorMessage(); }

    private:

        ClangParserContext                  m_context;
        Milliseconds                        m_totalParsingTime;
        Milliseconds                        m_totalVisitingTime;
        FileSystem::Path                    m_reflectionDataPath;
    };
}
