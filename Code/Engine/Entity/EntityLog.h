#pragma once

#include "Engine/_Module/API.h"

#include "EntityIDs.h"
#include "System/Types/Containers_ForwardDecl.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Entity;
    class EntityComponent;
}

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::EntityModel
{
    struct EntityLogRequest
    {
        String          m_timestamp;
        String          m_category;
        EntityID        m_entityID;
        ComponentID     m_componentID;
        String          m_message;
        String          m_filename;
        uint32_t        m_lineNumber;
        Log::Severity   m_severity;
    };

    //-------------------------------------------------------------------------

    EE_ENGINE_API void InitializeLogQueue();
    EE_ENGINE_API void ShutdownLogQueue();
    EE_ENGINE_API void EnqueueLogEntry( Entity const* pEntity, EE::Log::Severity severity, char const* pCategory, char const* pFilename, int pLineNumber, char const* pMessageFormat, ... );
    EE_ENGINE_API void EnqueueLogEntry( EntityComponent const* pComponent, EE::Log::Severity severity, char const* pCategory, char const* pFilename, int pLineNumber, char const* pMessageFormat, ... );

    // Retrieves all queued log entries and flushes the queue
    EE_ENGINE_API TVector<EntityLogRequest> RetrieveQueuedLogRequests();
}
#endif

// Entity Logging
//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
#define EE_LOG_ENTITY_MESSAGE( pEntityOrComponent, category, ... ) EE::EntityModel::EnqueueLogEntry( pEntityOrComponent, EE::Log::Severity::Message, category, __FILE__, __LINE__, __VA_ARGS__ )
#define EE_LOG_ENTITY_WARNING( pEntityOrComponent, category, ... ) EE::EntityModel::EnqueueLogEntry( pEntityOrComponent, EE::Log::Severity::Warning, category, __FILE__, __LINE__, __VA_ARGS__ )
#define EE_LOG_ENTITY_ERROR( pEntityOrComponent, category, ... ) EE::EntityModel::EnqueueLogEntry( pEntityOrComponent, EE::Log::Severity::Error, category, __FILE__, __LINE__, __VA_ARGS__ )
#define EE_LOG_ENTITY_FATAL_ERROR( pEntityOrComponent, category, ... ) EE::EntityModel::EnqueueLogEntry( pEntityOrComponent, EE::Log::Severity::FatalError, category, __FILE__, __LINE__, __VA_ARGS__ ); EE_HALT()
#else
#define EE_LOG_ENTITY_MESSAGE( pEntityOrComponent, category, ... ) 
#define EE_LOG_ENTITY_WARNING( pEntityOrComponent, category, ... ) 
#define EE_LOG_ENTITY_ERROR( pEntityOrComponent, category, ... ) 
#define EE_LOG_ENTITY_FATAL_ERROR( pEntityOrComponent, category, ... ) 
#endif