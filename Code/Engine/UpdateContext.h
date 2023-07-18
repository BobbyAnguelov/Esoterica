#pragma once
#include "Engine/_Module/API.h"
#include "UpdateStage.h"
#include "Base/Time/Time.h"
#include "Base/Systems.h"

//-------------------------------------------------------------------------
// The base update context for anything in the engine that needs to be updated
//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINE_API UpdateContext
    {

    public:

        // Update info
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE Seconds GetDeltaTime() const { return m_deltaTime; }
        EE_FORCE_INLINE uint64_t GetFrameID() const { return m_frameID; }
        EE_FORCE_INLINE UpdateStage GetUpdateStage() const { return m_stage; }

        // Systems
        //-------------------------------------------------------------------------

        EE_FORCE_INLINE SystemRegistry const* GetSystemRegistry() const { return m_pSystemRegistry; }

        template<typename T> 
        EE_FORCE_INLINE T* GetSystem() const { return m_pSystemRegistry->GetSystem<T>(); }

        // Frame rate limiter
        //-------------------------------------------------------------------------

        inline bool HasFrameRateLimit() const { return m_frameRateLimitFPS > 0; }
        inline void SetFrameRateLimit( float FPS ) { m_frameRateLimitFPS = Math::Max( 0.0f, FPS ); }
        inline float GetFrameRateLimit() const { EE_ASSERT( HasFrameRateLimit() ); return m_frameRateLimitFPS; }
        inline Milliseconds GetLimitedFrameTime() const { EE_ASSERT( HasFrameRateLimit() ); return Milliseconds( 1000 ) / m_frameRateLimitFPS; }

    protected:

        // Set the time delta for this update
        inline void UpdateDeltaTime( Milliseconds deltaTime )
        {
            EE_ASSERT( deltaTime >= 0 );

            // Update internal time
            m_deltaTime = deltaTime.ToSeconds();
            m_frameID++;
        }

    protected:

        Seconds                                     m_deltaTime = 1.0f / 60.0f;
        uint64_t                                    m_frameID = 0;
        float                                       m_frameRateLimitFPS = 144.0f;
        UpdateStage                                 m_stage = UpdateStage::FrameStart;
        SystemRegistry*                             m_pSystemRegistry = nullptr;
    };
}
