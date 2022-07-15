#include "ResourceRecord.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    ResourceRecord::~ResourceRecord()
    {
        EE_ASSERT( m_pResource == nullptr && !HasReferences() );
    }
}