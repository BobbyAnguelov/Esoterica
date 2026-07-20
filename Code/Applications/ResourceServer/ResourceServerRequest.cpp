#include "ResourceServerRequest.h"
#include "ResourceServerContext.h"
#include "Base/Time/TimeStamp.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    Request::Request( ResourceServerContext& context, uint64_t clientID, RequestOrigin origin, ResourceID const & resourceID, String const & extraInfo )
        : m_clientID( clientID )
        , m_resourceID( resourceID )
        , m_timestamp( TimeStamp().GetTime() )
        , m_extraInfo( extraInfo )
        , m_origin( origin )
    {
        if ( origin == RequestOrigin::Network )
        {
            EE_ASSERT( m_clientID != 0 );
        }
        else
        {
            EE_ASSERT( m_clientID == 0 );
        }

        // Validate resource ID
        if ( !m_resourceID.IsValid() )
        {
            m_log.sprintf( "Error: Invalid resource ID ( %s )", m_resourceID.c_str() );
            m_status = RequestStatus::Failed;
            return;
        }

        // Validate that this is a known resource
        m_pResourceInfo = context.m_typeRegistry.GetResourceInfo( m_resourceID.GetResourceTypeID() );
        if ( m_pResourceInfo == nullptr )
        {
            m_log.sprintf( "Error: Invalid resource ID ( %s )", m_resourceID.c_str() );
            m_status = RequestStatus::Failed;
        }

        // Update filepaths
        m_sourceFile = m_resourceID.GetFileSystemPath( context.GetSourceDataDirectory() );

        // Set the destination path based on request type
        if ( m_origin == RequestOrigin::Package )
        {
            m_destinationFile = m_resourceID.GetCompiledFileSystemPath( context.GetPackagedDataDirectory() );
        }
        else
        {
            m_destinationFile = m_resourceID.GetCompiledFileSystemPath( context.GetCompiledDataDirectory() );
        }
    }

    Milliseconds Request::GetCompletionTime() const
    {
        if ( !IsComplete() )
        {
            return PlatformClock::GetTimeInMilliseconds() - m_timeRequested;
        }

        return Milliseconds( m_timeCompleted - m_timeRequested );
    }

    bool Request::ShouldNotifyClientsWhenComplete() const
    {
        switch ( m_origin )
        {
            case RequestOrigin::Network:
            case RequestOrigin::FileWatcher:
            case RequestOrigin::ManualCompile:
            case RequestOrigin::ManualCompileForced:
            {
                return true;
            }
            break;

            default:
            {
                return false;
            }
            break;
        }
    }
}