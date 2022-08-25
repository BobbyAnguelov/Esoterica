#pragma once

#include "EngineTools/_Module/API.h"
#include "System/Types/Event.h"

//-------------------------------------------------------------------------

namespace EE::VisualGraph
{
    class BaseNode;
    class BaseGraph;

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API UserContext
    {
        void NavigateTo( BaseNode* pNode );
        void NavigateTo( BaseGraph* pGraph );

        inline TEventHandle<BaseNode*> OnNavigateToNode() { return m_onNavigateToNode; }
        inline TEventHandle<BaseGraph*> OnNavigateToGraph() { return m_onNavigateToGraph; }

    protected:

        TEvent<BaseNode*>                                  m_onNavigateToNode;
        TEvent<BaseGraph*>                                 m_onNavigateToGraph;
    };
}