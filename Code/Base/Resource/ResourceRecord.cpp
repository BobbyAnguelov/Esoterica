#include "ResourceRecord.h"
#include "ResourceRequest.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    ResourceRecord::~ResourceRecord()
    {
        EE_ASSERT( m_pResource == nullptr && !HasReferences() );
    }

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    ResourceLoadStage ResourceRecord::GetLoadStage() const
    {
        return m_loadStage;
    }

    Milliseconds ResourceRecord::GetLoadStageTime( ResourceLoadStage stage ) const
    {
        EE_ASSERT( stage != ResourceLoadStage::None );
        return m_stageDurations[(uint8_t) stage];
    }

    Milliseconds ResourceRecord::GetTotalLoadTime() const
    {
        Milliseconds totalTime = 0;
        for ( Milliseconds const& stageDuration : m_stageDurations )
        {
            totalTime += stageDuration;
        }
        return totalTime;
    }

    Milliseconds ResourceRecord::GetLoadTime() const
    {
        return m_stageDurations[(uint8_t) ResourceLoadStage::WaitForRawResourceRequest] + m_stageDurations[(uint8_t) ResourceLoadStage::LoadResource];
    }

    Milliseconds ResourceRecord::GetInstallTime() const
    {
        return m_stageDurations[(uint8_t) ResourceLoadStage::WaitForInstallDependencies] + m_stageDurations[(uint8_t) ResourceLoadStage::InstallResource];
    }
    #endif
}