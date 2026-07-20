#include "DebugDrawingSystem.h"
#include "Base/Memory/Memory.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    DebugDrawInternal::ThreadCommandBuffer& DebugDrawSystem::GetThreadCommandBuffer()
    {
        thread_local static Threading::ThreadID const threadID = Threading::GetCurrentThreadID();

        // Check for an already created buffer for this thread
        {
            Threading::ScopeLockRead Lock( m_threadCommandBuffersMutex );
            for ( auto& record : m_threadCommandBuffers )
            {
                if ( record.m_threadID == threadID )
                {
                    return *record.m_pBuffer;
                }
            }
        }

        // Create a new buffer
        //-------------------------------------------------------------------------

        Threading::ScopeLockWrite Lock( m_threadCommandBuffersMutex );
        CommandBufferRecord& record = m_threadCommandBuffers.emplace_back();
        record.m_threadID = threadID;
        record.m_pBuffer = EE::New<DebugDrawInternal::ThreadCommandBuffer>( threadID );

        return *record.m_pBuffer;
    }

    DebugDrawSystem::~DebugDrawSystem()
    {
        for ( auto& record : m_threadCommandBuffers )
        {
            EE::Delete( record.m_pBuffer );
        }
        m_threadCommandBuffers.clear();
    }

    void DebugDrawSystem::ReflectFrameCommandBuffer( DebugDrawInternal::FrameCommandBuffer& reflectedFrameCommands )
    {
        // Reflect all the new commands into the frame buffer
        Threading::ScopeLockRead Lock( m_threadCommandBuffersMutex );
        for ( CommandBufferRecord& record : m_threadCommandBuffers )
        {
            reflectedFrameCommands.AddThreadCommands( *record.m_pBuffer );
            record.m_pBuffer->Clear();
        }
    }

    void DebugDrawSystem::Reset()
    {
        Threading::ScopeLockRead Lock( m_threadCommandBuffersMutex );
        for ( CommandBufferRecord& record : m_threadCommandBuffers )
        {
            record.m_pBuffer->Clear();
        }
    }
}
#endif
