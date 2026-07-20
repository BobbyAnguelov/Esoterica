#pragma once
#include "Base/Resource/ResourceID.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/Time/Time.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    enum class NetworkMessageID
    {
        RequestResource = 100,
        ResourceRequestHeartbeat,
        ResourceRequestComplete,
        ResourceUpdated,
        AddRecompilationBlocker,
        RemoveRecompilationBlocker,
    };

    //-------------------------------------------------------------------------

    struct NetworkMessageSettings
    {
        constexpr static size_t const   s_maxNumberOfRequestsPerMessage = 32;
        constexpr static float const    s_heartbeatSendIntervalSeconds = 1.5f;
        constexpr static float const    s_heartbeatTimeoutSeconds = 4.0f;
        constexpr static int8_t const   s_maxRequestTimeoutRetries = 5;
    };

    //-------------------------------------------------------------------------

    struct NetworkResourceRequest
    {
        EE_SERIALIZE( m_resourceIDs );

    public:

        TVector<ResourceID>             m_resourceIDs;
    };

    //-------------------------------------------------------------------------

    struct NetworkResourceRecompilationBlockerRequest
    {
        EE_SERIALIZE( m_ID, m_path, m_timeToBlock );

        UUID                            m_ID;
        DataPath                        m_path;
        Seconds                         m_timeToBlock = 30;
    };

    //-------------------------------------------------------------------------

    struct NetworkResourceResponse
    {
        EE_SERIALIZE( m_results );

        struct Result
        {
            EE_SERIALIZE( m_resourceID, m_filePath, m_log );

            Result() = default;

            Result( ResourceID const& ID, String const& path )
                : m_resourceID( ID )
                , m_filePath( path )
            {}

            Result( ResourceID const& ID, String const& path, String const& log )
                : m_resourceID( ID )
                , m_filePath( path )
                , m_log( log )
            {}

            ResourceID                  m_resourceID;
            String                      m_filePath;
            String                      m_log;
        };

    public:

        inline bool Empty() const { return m_results.empty(); }
        inline void Clear() { m_results.clear(); }

    public:

        TVector<Result>                 m_results;
    };
}