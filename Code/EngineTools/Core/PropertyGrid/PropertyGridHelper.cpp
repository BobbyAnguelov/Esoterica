#include "PropertyGridHelper.h"

//-------------------------------------------------------------------------

namespace EE::PG
{
    EE_GLOBAL_REGISTRY( PropertyGridHelperFactory );

    //-------------------------------------------------------------------------

    PropertyHelper* PropertyGridHelperFactory::TryCreateHelper( ToolsContext const* pToolsContext, IReflectedType* pTypeInstance )
    {
        auto pCurrentFactory = s_pHead;
        while ( pCurrentFactory != nullptr )
        {
            if ( pTypeInstance->GetTypeID() == pCurrentFactory->GetSupportedTypeID() )
            {
                return pCurrentFactory->TryCreateHelperInternal( pToolsContext, pTypeInstance );
            }

            pCurrentFactory = pCurrentFactory->GetNextItem();
        }

        return nullptr;
    }
}