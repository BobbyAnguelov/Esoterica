#pragma once
#include "ResourceCompilerContext.h"
#include "Base/Resource/ResourceID.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/Types/UUID.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    enum class NetworkResourceWorkerMessageID
    {
        CompileResource = 200,
        CompileRequestHeartbeat,
        CompileRequestComplete,
        ID
    };

    //-------------------------------------------------------------------------

    struct NetworkResourceCompilerRequest
    {
        EE_SERIALIZE( m_requests );

        struct Request
        {
            EE_SERIALIZE( m_resourceID, m_taskID, m_isForPackagedBuild, m_forceCompilation );

            Request() = default;

            Request( ResourceID const& resourceID, UUID taskID )
                : m_resourceID( resourceID )
                , m_taskID( taskID )
            {}

            ResourceID              m_resourceID;
            UUID                    m_taskID;
            bool                    m_isForPackagedBuild = false;
            bool                    m_forceCompilation = false;
        };

        inline void Clear() { m_requests.clear(); }

    public:

        TVector<Request>            m_requests;
    };

    //-------------------------------------------------------------------------

    struct NetworkResourceCompilerID
    {
        EE_SERIALIZE( m_workerID );

        int64_t                     m_workerID;
    };

    //-------------------------------------------------------------------------

    struct NetworkResourceCompilerResponse
    {
        EE_SERIALIZE( m_results );

        struct Result
        {
            EE_SERIALIZE( m_taskID, m_resourceID, m_log, m_upToDateCheckTimeMS, m_compilationTimeMS, m_compilationResult );

            Result() = default;

            UUID                    m_taskID;
            ResourceID              m_resourceID;
            String                  m_log;
            float                   m_upToDateCheckTimeMS = 0;
            float                   m_compilationTimeMS = 0;
            CompilationResult       m_compilationResult = CompilationResult::Failure;
        };

    public:

        inline void Clear() { m_results.clear(); }

    public:

        TVector<Result>             m_results;
    };
}