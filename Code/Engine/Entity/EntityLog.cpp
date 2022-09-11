#include "EntityLog.h"
#include "Entity.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::EntityModel
{
    Threading::LockFreeQueue<EntityLogRequest>* g_pEntryQueue = nullptr;

    //-------------------------------------------------------------------------

    static void EnqueueLogEntry( EntityID entityID, ComponentID componentID, EE::Log::Severity severity, char const* pCategory, char const* pFilename, int pLineNumber, char const* pMessageFormat, va_list args )
    {
        EE_ASSERT( entityID.IsValid() );
        EE_ASSERT( pCategory != nullptr && pFilename != nullptr && pMessageFormat != nullptr );

        EntityLogRequest entry;
        entry.m_category = pCategory;
        entry.m_entityID = entityID;
        entry.m_componentID = componentID;
        entry.m_filename = pFilename;
        entry.m_lineNumber = pLineNumber;
        entry.m_severity = severity;

        // Message
        entry.m_message.sprintf_va_list( pMessageFormat, args );

        // Timestamp
        entry.m_timestamp.resize( 9 );
        time_t const t = std::time( nullptr );
        strftime( entry.m_timestamp.data(), 9, "%H:%M:%S", std::localtime( &t ) );

        //-------------------------------------------------------------------------

        EE_ASSERT( g_pEntryQueue != nullptr );
        g_pEntryQueue->enqueue( entry );
    }

    //-------------------------------------------------------------------------

    void InitializeLogQueue()
    {
        EE_ASSERT( g_pEntryQueue == nullptr );
        g_pEntryQueue = EE::New< Threading::LockFreeQueue<EntityLogRequest> >();
    }

    void ShutdownLogQueue()
    {
        EE_ASSERT( g_pEntryQueue != nullptr );
        EE::Delete( g_pEntryQueue );
    }

    void EnqueueLogEntry( Entity const* pEntity, EE::Log::Severity severity, char const* pCategory, char const* pFilename, int pLineNumber, char const* pMessageFormat, ... )
    {
        EE_ASSERT( pEntity != nullptr );

        va_list args;
        va_start( args, pMessageFormat );
        EnqueueLogEntry( pEntity->GetID(), ComponentID(), severity, pCategory, pFilename, pLineNumber, pMessageFormat, args);
        va_end( args );
    }

    void EnqueueLogEntry( EntityComponent const* pComponent, EE::Log::Severity severity, char const* pCategory, char const* pFilename, int pLineNumber, char const* pMessageFormat, ... )
    {
        EE_ASSERT( pComponent != nullptr && pComponent->GetEntityID().IsValid() );

        va_list args;
        va_start( args, pMessageFormat );
        EnqueueLogEntry( pComponent->GetEntityID(), pComponent->GetID(), severity, pCategory, pFilename, pLineNumber, pMessageFormat, args);
        va_end( args );
    }

    //-------------------------------------------------------------------------

    TVector<EntityLogRequest> RetrieveQueuedLogRequests()
    {
        TVector<EntityLogRequest> entries;
        entries.reserve( g_pEntryQueue->size_approx() );
        entries.push_back( EntityLogRequest() );

        while ( g_pEntryQueue->try_dequeue( entries.back() ) )
        {
            entries.push_back();
        }

        entries.pop_back();
        return entries;
    }
}
#endif