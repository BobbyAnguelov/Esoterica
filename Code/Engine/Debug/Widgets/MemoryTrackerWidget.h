#pragma once
#include "Engine/_Module/API.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/String.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/Render/RHI.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    class UpdateContext;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API MemoryTrackerWidget
    {
        struct Entry
        {
            EE::Memory::MemoryAllocator*                    m_pMemoryAllocator = nullptr;
            uint64_t                                        m_numBytesSnapshot = 0;
            uint64_t                                        m_numAllocationsSnapshot = 0;
        };

        struct Group
        {
            char const*                                     m_pLabel;
            uint64_t                                        m_thresholdLow;   // inclusive
            uint64_t                                        m_thresholdHigh;  // exclusive
            bool                                            m_collapsible;
            size_t                                          m_numAllocations;
            uint64_t                                        m_numTotalBytes;
            InlineString                                    m_headerText;
            bool                                            m_open = false;
        };

    public:

        MemoryTrackerWidget();

        void UpdateAndDraw( UpdateContext const& context );

    private:

        void DrawWindow( UpdateContext const& context );

    private:

        TInlineVector<Entry, 256>                                   m_entries;
        TInlineVector<Group, 5>                                     m_groups;
        bool                                                        m_hasSnapshot = false;
        InlineString                                                m_summaryHeader;
        InlineString                                                m_barText;
        InlineString                                                m_gpuBarText;
        InlineString                                                m_gpuResourceLabel;

        // Summary snapshot values
        uint64_t                                                    m_snapshotTotalTracked = 0;
        uint64_t                                                    m_snapshotUntracked = 0;
        uint64_t                                                    m_snapshotUnused = 0;
        uint64_t                                                    m_snapshotVirtualMemory = 0;

        // GPU snapshot values
        uint64_t                                                    m_snapshotGpuLocalUsage = 0;
        uint64_t                                                    m_snapshotGpuLocalBudget = 0;
        uint64_t                                                    m_snapshotGpuNonLocalUsage = 0;
        uint64_t                                                    m_snapshotGpuNonLocalBudget = 0;

        // Cached GPU stats
        TVector<EE::Render::RHI::ResourceAllocationStatistic>       m_gpuBufferStats;
        TVector<EE::Render::RHI::ResourceAllocationStatistic>       m_gpuTextureStats;

        // GPU resource snapshot
        TVector<EE::Render::RHI::ResourceAllocationStatistic>       m_snapshotGpuBufferStats;
        TVector<EE::Render::RHI::ResourceAllocationStatistic>       m_snapshotGpuTextureStats;

        // Filter
        ImGuiX::FilterWidget                                        m_filterWidget;
        uint64_t                                                    m_numBytesAccumulated = 0;
        uint64_t                                                    m_numAllocationsAccumulated = 0;
    };
}
#endif
