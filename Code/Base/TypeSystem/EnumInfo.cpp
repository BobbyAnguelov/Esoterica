#include "EnumInfo.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    #if EE_DEVELOPMENT_TOOLS
    TVector<EnumInfo::ConstantInfo> EnumInfo::GetConstantsInAlphabeticalOrder() const
    {
        TVector<ConstantInfo> sortedConstants = m_constants;
        eastl::sort( sortedConstants.begin(), sortedConstants.end(), [] ( ConstantInfo const& a, ConstantInfo const& b ) { return a.m_alphabeticalOrder < b.m_alphabeticalOrder; } );
        return sortedConstants;
    }
    #endif
}