#pragma once
#include "Base/Resource/ResourceID.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/TypeSystem/ResourceInfo.h"
#include "Base/Time/Time.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    enum class RequestOrigin
    {
        Network,
        ManualCompile,
        ManualCompileForced,
        FileWatcher,
        UnblockedRecompilationRequest,
        Package
    };

    enum class RequestStatus
    {
        Pending,
        Compiling,
        Succeeded,
        SucceededWithWarnings,
        SucceededUpToDate,
        Failed
    };

    //-------------------------------------------------------------------------

    struct Request
    {
        explicit Request( struct ResourceServerContext& context, uint64_t clientID, RequestOrigin origin, ResourceID const& resourceID, String const& extraInfo = String() );

        Milliseconds GetCompletionTime() const;
        inline FileSystem::Path const& GetSourceFilePath() const { return m_sourceFile; }
        inline FileSystem::Path const& GetDestinationFilePath() const { return m_destinationFile; }

        // Returns whether the request was externally requested (i.e. by a client) or internally requested (i.e. due to a file changing and being detected)
        inline bool IsInternalRequest() const { return m_origin != RequestOrigin::Network; }

        // Is this a packaging request
        inline bool IsPackagingRequest() const { return m_origin == RequestOrigin::Package; }

        // Should we send an update message to clients when this request completes?
        bool ShouldNotifyClientsWhenComplete() const;

        // Status
        inline RequestStatus GetStatus() const { return m_status; }
        inline bool IsPending() const { return m_status == RequestStatus::Pending; }
        inline bool IsCompiling() const { return m_status == RequestStatus::Compiling; }
        inline bool HasSucceeded() const { return m_status == RequestStatus::Succeeded || m_status == RequestStatus::SucceededWithWarnings || m_status == RequestStatus::SucceededUpToDate; }
        inline bool HasFailed() const { return m_status == RequestStatus::Failed; }
        inline bool IsComplete() const { return HasSucceeded() || HasFailed(); }

    public:

        uint64_t                                m_clientID = 0;
        ResourceID                              m_resourceID;
        FileSystem::Path                        m_sourceFile;
        FileSystem::Path                        m_destinationFile;
        String                                  m_timestamp;
        String                                  m_extraInfo;
        String                                  m_log;
        String                                  m_standaloneCompilerArgs;
        RequestOrigin                           m_origin = RequestOrigin::Network;
        RequestStatus                           m_status = RequestStatus::Pending;
        Milliseconds const                      m_timeRequested = PlatformClock::GetTimeInMilliseconds();
        Milliseconds                            m_lastWorkerHeartbeatTime = 0;
        Milliseconds                            m_lastSentHeartbeatTime = 0;
        Milliseconds                            m_timeCompleted = 0;
        Milliseconds                            m_upToDateCheckTime = 0;
        Milliseconds                            m_compileTime = 0;
        TypeSystem::ResourceInfo const*         m_pResourceInfo = nullptr;
    };
}