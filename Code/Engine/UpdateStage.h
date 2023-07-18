#pragma once

#include "Base/Memory/Memory.h"

//-------------------------------------------------------------------------
// Update Stages
//-------------------------------------------------------------------------
// We only allow 254 levels of priority
// The lower the number the earlier it runs, i.e. priority 1 will run before priority 127
// A priority of 255 (0xFF) means that stage is disabled

namespace EE
{
    // Pre-defined engine stages
    //-------------------------------------------------------------------------

    enum class UpdateStage : uint8_t
    {
        FrameStart = 0,
        PrePhysics,
        Physics,
        PostPhysics,
        FrameEnd,

        Paused, // Special stage that runs only when the game is paused

        NumStages,
    };

    // Some pre-defined priority levels
    //-------------------------------------------------------------------------

    enum class UpdatePriority : uint8_t
    {
        Highest = 0x00,
        High = 0x40,
        Medium = 0x80,
        Low = 0xC0,
        Disabled = 0xFF,
        Default = Medium,
    };

    // Stage & Priority pair
    //-------------------------------------------------------------------------

    struct UpdateStagePriority
    {
        inline UpdateStagePriority( UpdateStage stage ) : m_stage( stage ) {}
        inline UpdateStagePriority( UpdateStage stage, uint8_t priority ) : m_stage( stage ), m_priority( priority ) {}
        inline UpdateStagePriority( UpdateStage stage, UpdatePriority priority ) : m_stage( stage ), m_priority( (uint8_t) priority ) {}

    public:

        UpdateStage     m_stage;
        uint8_t         m_priority = (uint8_t) UpdatePriority::Default;
    };

    // Syntactic sugar for use in macro declarations
    using RequiresUpdate = UpdateStagePriority;

    // List of priorities per update stage
    //-------------------------------------------------------------------------

    struct UpdatePriorityList
    {
        UpdatePriorityList()
        {
            Reset();
        }

        template<typename... Args>
        UpdatePriorityList( Args&&... args )
        {
            Reset();
            ( ( *this << eastl::forward<Args>( args ) ), ... );
        }

        inline void Reset()
        {
            memset( m_priorities, (uint8_t) UpdatePriority::Disabled, sizeof( m_priorities ) );
        }

        inline bool IsStageEnabled( UpdateStage stage ) const
        {
            EE_ASSERT( stage < UpdateStage::NumStages );
            return m_priorities[(uint8_t) stage] != (uint8_t) UpdatePriority::Disabled;
        }

        inline uint8_t GetPriorityForStage( UpdateStage stage ) const
        {
            EE_ASSERT( stage < UpdateStage::NumStages );
            return m_priorities[(uint8_t) stage];
        }

        inline UpdatePriorityList& SetStagePriority( UpdateStagePriority&& stagePriority )
        {
            m_priorities[(uint8_t) stagePriority.m_stage] = stagePriority.m_priority;
            return *this;
        }

        // Set a priority for a given stage
        inline UpdatePriorityList& operator<<( UpdateStagePriority&& stagePriority )
        {
            m_priorities[(uint8_t) stagePriority.m_stage] = stagePriority.m_priority;
            return *this;
        }

        inline bool AreAllStagesDisabled() const
        {
            static uint8_t const disabledStages[(int32_t) UpdateStage::NumStages] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
            static_assert( sizeof( disabledStages ) == sizeof( m_priorities ), "disabled stages must be the same size as the priorities list" );
            return memcmp( m_priorities, disabledStages, sizeof( m_priorities ) ) == 0;
        }

    private:

        uint8_t           m_priorities[(int32_t) UpdateStage::NumStages];
    };
}