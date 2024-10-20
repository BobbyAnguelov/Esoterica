#include "PropertyGridTypeEditingRules.h"

//-------------------------------------------------------------------------

namespace EE::PG
{
    TypeEditingRules* TypeEditingRulesFactory::TryCreateRules( ToolsContext const* pToolsContext, IReflectedType* pTypeInstance )
    {
        auto pCurrentFactory = s_pHead;
        while ( pCurrentFactory != nullptr )
        {
            if ( pTypeInstance->GetTypeID() == pCurrentFactory->GetSupportedTypeID() )
            {
                return pCurrentFactory->TryCreateRulesInternal( pToolsContext, pTypeInstance );
            }

            pCurrentFactory = pCurrentFactory->GetNextItem();
        }

        return nullptr;
    }
}