#pragma once
#include "Base/Resource/ResourceID.h"
#include "Base/Time/Time.h"
#include "Base/Types/UUID.h"
#include "Base/Time/Timestamp.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class CompilationRequest final
    {
        friend class ResourceServer;
        friend class ResourceServerTask;

    public:

        enum class Status
        {
            Pending,
            Compiling,
            Succeeded,
            SucceededWithWarnings,
            SucceededUpToDate,
            Failed
        };

        enum class Origin
        {
            External,
            ManualCompile,
            ManualCompileForced,
            FileWatcher,
            Package
        };

    public:

        // Get the client that requested this resource
        inline uint32_t GetClientID() const { EE_ASSERT( !IsInternalRequest() ); return m_clientID; }

        // Get the resource ID for this request
        inline ResourceID const& GetResourceID() const { return m_resourceID; }

        // Returns whether the request was externally requested (i.e. by a client) or internally requested (i.e. due to a file changing and being detected)
        inline bool IsInternalRequest() const { return m_origin != Origin::External; }

        // Do we require a force recompile of this resource even if it's up to date
        inline bool RequiresForcedRecompiliation() const { return m_origin == Origin::ManualCompileForced || m_origin == Origin::Package; }

        // Do we have any extra information for this request?
        inline bool HasExtraInfo() const { return !m_extraInfo.empty(); }

        // Status
        inline Status GetStatus() const { return m_status; }
        inline bool IsPending() const { return m_status == Status::Pending; }
        inline bool IsCompiling() const { return m_status == Status::Compiling; }
        inline bool HasSucceeded() const { return m_status == Status::Succeeded || m_status == Status::SucceededWithWarnings || m_status == Status::SucceededUpToDate; }
        inline bool HasFailed() const { return m_status == Status::Failed; }
        inline bool IsComplete() const { return HasSucceeded() || HasFailed(); }

        // Request Info
        inline char const* GetLog() const { return m_log.c_str(); }
        inline char const* GetCompilerArgs() const { return m_compilerArgs.c_str(); }
        inline FileSystem::Path const& GetSourceFilePath() const { return m_sourceFile; }
        inline FileSystem::Path const& GetDestinationFilePath() const { return m_destinationFile; }

        inline TimeStamp const& GetTimeRequested() const { return m_timeRequested; }

        inline Milliseconds GetCompilationElapsedTime() const
        {
            if ( m_status == Status::Pending )
            {
                return 0;
            }

            if ( !IsComplete() )
            {
                return Milliseconds( PlatformClock::GetTime() - m_compilationTimeStarted );
            }

            return Milliseconds( m_compilationTimeFinished - m_compilationTimeStarted );
        }

    public:

        uint32_t                            m_clientID = 0;
        ResourceID                          m_resourceID;
        int32_t                             m_compilerVersion = -1;
        uint64_t                            m_fileTimestamp = 0;
        uint64_t                            m_sourceTimestampHash = 0;
        FileSystem::Path                    m_sourceFile;
        FileSystem::Path                    m_destinationFile;
        String                              m_compilerArgs;

        String                              m_extraInfo;

        TimeStamp                           m_timeRequested;
        Nanoseconds                         m_compilationTimeStarted = 0;
        Nanoseconds                         m_compilationTimeFinished = 0;

        String                              m_log;
        Status                              m_status = Status::Pending;
        Origin                              m_origin = Origin::External;
    };
}
