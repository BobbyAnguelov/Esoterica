#pragma once

#include "Base/_Module/API.h"
#include "Base/Types/Arrays.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Threading/Threading.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class EE_BASE_API DebugDrawSystem
    {
        struct CommandBufferRecord
        {
            Threading::ThreadID                         m_threadID;
            DebugDrawInternal::ThreadCommandBuffer*     m_pBuffer = nullptr;
        };

    public:

        DebugDrawSystem() = default;
        ~DebugDrawSystem();

        // Empty all per thread buffers
        void Reset();

        // Returns a per-thread drawing context, this removes the need for constantly calling get thread command buffer
        inline DebugDrawContext GetDebugDrawContext() { return DebugDrawContext( GetThreadCommandBuffer() ); }

        // Reflects all the individual per-thread buffers into a single supplied frame command buffer. Clears all thread buffers.
        void ReflectFrameCommandBuffer( DebugDrawInternal::FrameCommandBuffer& reflectedFrameCommands );

    private:

        DebugDrawInternal::ThreadCommandBuffer& GetThreadCommandBuffer();

    private:

        TInlineVector<CommandBufferRecord, 32>          m_threadCommandBuffers;
        Threading::ReadWriteMutex                       m_threadCommandBuffersMutex;
    };
}
#endif