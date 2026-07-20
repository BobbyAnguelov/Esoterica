#pragma once
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    struct EntityGroup : public IReflectedType
    {
        EE_REFLECT_TYPE( EntityGroup );

    public:

        EntityGroup() = default;
        EntityGroup( StringID ID ) : m_ID( ID ) { EE_ASSERT( m_ID.IsValid() ); }

    public:

        EE_REFLECT();
        StringID            m_ID;

        EE_REFLECT();
        TVector<StringID>   m_names;
    };
}