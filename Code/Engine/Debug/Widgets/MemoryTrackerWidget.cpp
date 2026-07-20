#include "MemoryTrackerWidget.h"
#include "Engine/UpdateContext.h"
#include "Engine/Render/RenderSystem.h"
#include "Base/Memory/Memory.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE
{
    constexpr uint64_t KB = 1024;
    constexpr uint64_t MB = KB * 1024;
    constexpr uint64_t GB = MB * 1024;

    //-------------------------------------------------------------------------

    static void AppendFormattedBytes( InlineString& str, uint64_t bytes )
    {
        if ( bytes >= GB )
            str.append_sprintf( "%.2f GB", double( bytes ) / double( GB ) );
        else if ( bytes >= MB )
            str.append_sprintf( "%.2f MB", double( bytes ) / double( MB ) );
        else if ( bytes >= KB )
            str.append_sprintf( "%.2f KB", double( bytes ) / double( KB ) );
        else
            str.append_sprintf( "%llu B", static_cast<unsigned long long>( bytes ) );
    }

    static void DrawFormattedBytes( uint64_t bytes )
    {
        if ( bytes >= GB )
            ImGui::Text( "%.2f GB", double( bytes ) / double( GB ) );
        else if ( bytes >= MB )
            ImGui::Text( "%.2f MB", double( bytes ) / double( MB ) );
        else if ( bytes >= KB )
            ImGui::Text( "%.2f KB", double( bytes ) / double( KB ) );
        else
            ImGui::Text( "%llu B", static_cast<unsigned long long>( bytes ) );
    }

    static void DrawFormattedDeltaBytes( int64_t delta )
    {
        if ( delta == 0 )
        {
            ImGui::TextUnformatted( "-" );
            return;
        }

        uint64_t const absDelta = static_cast<uint64_t>( delta > 0 ? delta : -delta );

        char const sign = delta > 0 ? '+' : '-';
        ImVec4 const color = delta > 0 ? ImVec4( 1.0f, 0.4f, 0.4f, 1.0f ) : ImVec4( 0.4f, 1.0f, 0.4f, 1.0f );

        ImGui::PushStyleColor( ImGuiCol_Text, color );
        if ( absDelta >= GB )
            ImGui::Text( "%c%.2f GB", sign, double( absDelta ) / double( GB ) );
        else if ( absDelta >= MB )
            ImGui::Text( "%c%.2f MB", sign, double( absDelta ) / double( MB ) );
        else if ( absDelta >= KB )
            ImGui::Text( "%c%.2f KB", sign, double( absDelta ) / double( KB ) );
        else
            ImGui::Text( "%c%llu B", sign, static_cast<unsigned long long>( absDelta ) );
        ImGui::PopStyleColor();
    }

    static void AppendDescriptorTypeFlags( InlineString& str, TBitFlags<EE::Render::RHI::DescriptorTypeFlags> flags )
    {
        bool first = true;
        auto Append = [&str, &first] ( char const* pName )
        {
            if ( !first ) str.append( " | " );
            str.append( pName );
            first = false;
        };

        if ( flags.IsFlagSet( EE::Render::RHI::DescriptorTypeFlags::Sampler ) )                 Append( "Sampler" );
        if ( flags.IsFlagSet( EE::Render::RHI::DescriptorTypeFlags::ConstantBuffer ) )          Append( "CBV" );
        if ( flags.IsFlagSet( EE::Render::RHI::DescriptorTypeFlags::Buffer ) )                  Append( "SRV" );
        if ( flags.IsFlagSet( EE::Render::RHI::DescriptorTypeFlags::RWBuffer ) )                Append( "UAV" );
        if ( flags.IsFlagSet( EE::Render::RHI::DescriptorTypeFlags::Texture ) )                 Append( "Tex" );
        if ( flags.IsFlagSet( EE::Render::RHI::DescriptorTypeFlags::TextureCube ) )             Append( "Cube" );
        if ( flags.IsFlagSet( EE::Render::RHI::DescriptorTypeFlags::RWTexture ) )               Append( "RWTex" );
        if ( flags.IsFlagSet( EE::Render::RHI::DescriptorTypeFlags::RenderTarget ) )            Append( "RT" );
        if ( flags.IsFlagSet( EE::Render::RHI::DescriptorTypeFlags::Raw ) )                     Append( "Raw" );
        if ( flags.IsFlagSet( EE::Render::RHI::DescriptorTypeFlags::IndexBuffer ) )             Append( "IB" );
        if ( flags.IsFlagSet( EE::Render::RHI::DescriptorTypeFlags::IndirectArgumentBuffer ) )  Append( "Indirect" );
        if ( flags.IsFlagSet( EE::Render::RHI::DescriptorTypeFlags::RootConstant ) )            Append( "RootConst" );
        if ( flags.IsFlagSet( EE::Render::RHI::DescriptorTypeFlags::AccelerationStructure ) )   Append( "RTAS" );

        if ( first )
            str.append( "None" );
    }

    // Shared delta-display helpers
    //-------------------------------------------------------------------------

    static void AppendDeltaToString( InlineString& str, int64_t delta )
    {
        str.append_sprintf( " (%c", delta > 0 ? '+' : '-' );
        AppendFormattedBytes( str, static_cast<uint64_t>( delta > 0 ? delta : -delta ) );
        str.append( ")" );
    }

    static void DrawDeltaBytesBadge( bool hasSnapshot, int64_t delta )
    {
        if ( !hasSnapshot || delta == 0 )
            return;
        ImGui::SameLine( 0, 0 );
        ImGui::TextUnformatted( " (" );
        ImGui::SameLine( 0, 0 );
        DrawFormattedDeltaBytes( delta );
        ImGui::SameLine( 0, 0 );
        ImGui::TextUnformatted( ")" );
    }

    static void DrawDeltaCountBadge( bool hasSnapshot, int64_t delta )
    {
        if ( !hasSnapshot || delta == 0 )
            return;
        ImVec4 const color = delta > 0 ? ImVec4( 1.0f, 0.3f, 0.3f, 1.0f ) : ImVec4( 0.3f, 0.8f, 0.4f, 1.0f );
        char const sign = delta > 0 ? '+' : '-';
        uint64_t const absDelta = static_cast<uint64_t>( delta > 0 ? delta : -delta );
        ImGui::SameLine( 0, 0 );
        ImGui::TextUnformatted( " (" );
        ImGui::SameLine( 0, 0 );
        ImGui::PushStyleColor( ImGuiCol_Text, color );
        ImGui::Text( "%c%llu", sign, static_cast<unsigned long long>( absDelta ) );
        ImGui::PopStyleColor();
        ImGui::SameLine( 0, 0 );
        ImGui::TextUnformatted( ")" );
    }

    //-------------------------------------------------------------------------

    MemoryTrackerWidget::MemoryTrackerWidget()
    {
        for ( Memory::MemoryAllocator* pMemoryAllocator = Memory::MemoryAllocator::GetHead(); pMemoryAllocator; pMemoryAllocator = pMemoryAllocator->GetNextItem() )
        {
            m_entries.push_back( { pMemoryAllocator } );
        }

        constexpr uint64_t MB10 = 10 * MB;

        m_groups =
        { {
            { nullptr,         MB10,       0xFFFFFFFFFFFFFFFFull,  false },
            { "Under 10 MB",   MB,         MB10,                   true  },
            { "Under 1 MB",    10 * KB,    MB,                     true  },
            { "Under 10 KB",   1,          10 * KB,                true  },
            { "Unused",        0,          1,                      true  },
        } };

        for ( Memory::MemoryAllocator* pAlloc = Memory::MemoryAllocator::GetHead(); pAlloc; pAlloc = pAlloc->GetNextItem() )
        {
            pAlloc->ClaimAccumulatedBytes();
            pAlloc->ClaimAccumulatedAllocations();
        }
    }

    void MemoryTrackerWidget::UpdateAndDraw( UpdateContext const& context )
    {
        // Sum claims across all allocators
        m_numBytesAccumulated = 0;
        m_numAllocationsAccumulated = 0;

        Memory::MemoryAllocator* pAlloc = Memory::MemoryAllocator::GetHead();
        while( pAlloc != nullptr )
        {
            m_numBytesAccumulated += pAlloc->ClaimAccumulatedBytes();
            m_numAllocationsAccumulated += pAlloc->ClaimAccumulatedAllocations();
            pAlloc = pAlloc->GetNextItem();
        }

        DrawWindow( context );
    }

    void MemoryTrackerWidget::DrawWindow( UpdateContext const& context )
    {
        uint64_t totalTracked = 0;

        for ( Entry const& entry : m_entries )
        {
            totalTracked += entry.m_pMemoryAllocator->GetNumBytes();
        }

        uint64_t const totalRequested = Memory::GetTotalRequestedMemory();
        uint64_t const totalAllocated = Memory::GetTotalAllocatedMemory();
        uint64_t const totalVirtual = Memory::GetVirtualMemoryCommitted();
        uint64_t const untracked = ( totalRequested > totalTracked ) ? ( totalRequested - totalTracked ) : 0;
        uint64_t const unused = ( totalAllocated > totalRequested ) ? ( totalAllocated - totalRequested ) : 0;
        uint64_t const totalMax = totalAllocated > 0 ? totalAllocated : 1;

        // Gather GPU memory stats
        //-------------------------------------------------------------------------

        uint64_t gpuLocalUsage = 0;
        uint64_t gpuLocalBudget = 0;
        uint64_t gpuNonLocalUsage = 0;
        uint64_t gpuNonLocalBudget = 0;

        EE::Render::RenderSystem* pRenderSystem = context.GetSystem<EE::Render::RenderSystem>();
        EE::Render::RHI::Context* pContextRHI = pRenderSystem->GetContextRHI();

        EE::Render::RHI::GetDetailedMemoryStatistics( pContextRHI, gpuLocalUsage, gpuLocalBudget, gpuNonLocalUsage, gpuNonLocalBudget );
        EE::Render::RHI::GetResourceAllocationStatistics( pContextRHI, m_gpuBufferStats, m_gpuTextureStats );

        // Toolbar
        //-------------------------------------------------------------------------

        if ( ImGui::Button( "Take Snapshot" ) )
        {
            for ( Entry& entry : m_entries )
            {
                entry.m_numBytesSnapshot = entry.m_pMemoryAllocator->GetNumBytes();
                entry.m_numAllocationsSnapshot = entry.m_pMemoryAllocator->GetNumAllocations();
            }

            m_snapshotTotalTracked = totalTracked;
            m_snapshotUntracked = untracked;
            m_snapshotUnused = unused;
            m_snapshotVirtualMemory = totalVirtual;

            m_snapshotGpuLocalUsage = gpuLocalUsage;
            m_snapshotGpuLocalBudget = gpuLocalBudget;
            m_snapshotGpuNonLocalUsage = gpuNonLocalUsage;
            m_snapshotGpuNonLocalBudget = gpuNonLocalBudget;

            m_snapshotGpuBufferStats = m_gpuBufferStats;
            m_snapshotGpuTextureStats = m_gpuTextureStats;

            m_hasSnapshot = true;
        }

        ImGui::SameLine();

        bool const clearButtonDisabled = !m_hasSnapshot;
        if ( clearButtonDisabled )
            ImGui::BeginDisabled();

        if ( ImGui::Button( "Clear Snapshot" ) )
        {
            m_hasSnapshot = false;
        }

        if ( clearButtonDisabled )
            ImGui::EndDisabled();

        // Side-by-side: CPU (left) | GPU (right)
        //-------------------------------------------------------------------------

        float const halfWidth = ImGui::GetContentRegionAvail().x * 0.5f;

        // GPU sort + totals
        eastl::stable_sort( m_gpuBufferStats.begin(), m_gpuBufferStats.end(), [] ( auto const& a, auto const& b )
        {
            return a.m_numBytes > b.m_numBytes;
        } );
        eastl::stable_sort( m_gpuTextureStats.begin(), m_gpuTextureStats.end(), [] ( auto const& a, auto const& b )
        {
            return a.m_numBytes > b.m_numBytes;
        } );

        uint64_t totalBufferBytes = 0;
        uint64_t totalBufferCount = 0;
        int64_t  totalBufferBytesDelta = 0;
        int64_t  totalBufferCountDelta = 0;

        uint64_t totalTextureBytes = 0;
        uint64_t totalTextureCount = 0;
        int64_t  totalTextureBytesDelta = 0;
        int64_t  totalTextureCountDelta = 0;

        {
            auto AccumulateTotals = [&]( TVector<EE::Render::RHI::ResourceAllocationStatistic> const& stats,
                                          TVector<EE::Render::RHI::ResourceAllocationStatistic> const& snapshotStats,
                                          uint64_t& outBytes, uint64_t& outCount,
                                          int64_t& outBytesDelta, int64_t& outCountDelta )
            {
                for ( auto const& stat : stats )
                {
                    outBytes += stat.m_numBytes;
                    outCount += stat.m_numAllocations;
                    if ( m_hasSnapshot )
                    {
                        bool found = false;
                        for ( auto const& snapStat : snapshotStats )
                        {
                            if ( snapStat.m_descriptorTypes == stat.m_descriptorTypes )
                            {
                                outBytesDelta += int64_t( stat.m_numBytes ) - int64_t( snapStat.m_numBytes );
                                outCountDelta += int64_t( stat.m_numAllocations ) - int64_t( snapStat.m_numAllocations );
                                found = true;
                                break;
                            }
                        }
                        if ( !found )
                        {
                            outBytesDelta += int64_t( stat.m_numBytes );
                            outCountDelta += int64_t( stat.m_numAllocations );
                        }
                    }
                }
            };

            AccumulateTotals( m_gpuBufferStats, m_snapshotGpuBufferStats, totalBufferBytes, totalBufferCount, totalBufferBytesDelta, totalBufferCountDelta );
            AccumulateTotals( m_gpuTextureStats, m_snapshotGpuTextureStats, totalTextureBytes, totalTextureCount, totalTextureBytesDelta, totalTextureCountDelta );
        }

        uint64_t const totalTrackedGpuBytes = totalBufferBytes + totalTextureBytes;
        uint64_t const otherBytes = ( gpuLocalUsage > totalTrackedGpuBytes ) ? ( gpuLocalUsage - totalTrackedGpuBytes ) : 0;

        // Shared header row
        //-------------------------------------------------------------------------

        ImGui::BeginChild( "HeaderRow", ImVec2( 0, 0 ), ImGuiChildFlags_AutoResizeY );

        // --- Left: CPU header ---
        ImGui::BeginGroup();

        m_summaryHeader.sprintf( "CPU Memory - Tracked: " );
        AppendFormattedBytes( m_summaryHeader, totalTracked );
        m_summaryHeader.append( " / " );
        AppendFormattedBytes( m_summaryHeader, totalAllocated );
        m_summaryHeader.append( " allocated, this frame: " );
        AppendFormattedBytes( m_summaryHeader, m_numBytesAccumulated );
        m_summaryHeader.append_sprintf( " (%llu allocs)", static_cast<unsigned long long>( m_numAllocationsAccumulated ) );
        ImGui::TextUnformatted( m_summaryHeader.c_str() );

        {
            float const w = halfWidth - ImGui::GetStyle().WindowPadding.x * 2;
            auto DrawBar = [this, w] ( char const* pLabel, uint64_t value, uint64_t snapshotValue, uint64_t total, ImVec4 const& color )
            {
                m_barText.sprintf( "%s ", pLabel );
                AppendFormattedBytes( m_barText, value );
                if ( m_hasSnapshot )
                {
                    int64_t const delta = int64_t( value ) - int64_t( snapshotValue );
                    if ( delta != 0 )
                        AppendDeltaToString( m_barText, delta );
                }
                float const fraction = total > 0 ? float( double( value ) / double( total ) ) : 0.0f;
                ImGui::ProgressBar( fraction, ImVec2( w, 0 ), m_barText.c_str() );
            };

            DrawBar( "Tracked", totalTracked, m_snapshotTotalTracked, totalMax, ImVec4( 0.3f, 0.8f, 0.4f, 1.0f ) );
            DrawBar( "Untracked", untracked, m_snapshotUntracked, totalMax, ImVec4( 1.0f, 0.6f, 0.2f, 1.0f ) );
            DrawBar( "Unused", unused, m_snapshotUnused, totalMax, ImVec4( 0.5f, 0.5f, 0.5f, 1.0f ) );
            DrawBar( "Virtual", totalVirtual, m_snapshotVirtualMemory, totalMax, ImVec4( 0.4f, 0.5f, 0.9f, 1.0f ) );
        }

        m_filterWidget.UpdateAndDraw( halfWidth - ImGui::GetStyle().WindowPadding.x * 2 );

        ImGui::EndGroup();
        ImGui::SameLine();

        // --- Right: GPU header ---
        ImGui::BeginGroup();

        ImGui::TextUnformatted( "GPU Memory" );

        {
            float const w = ImGui::GetContentRegionAvail().x;
            auto DrawGpuBar = [this, w] ( char const* pLabel, uint64_t usage, uint64_t budget, uint64_t snapshotUsage, uint64_t snapshotBudget )
            {
                float const fraction = budget > 0 ? float( double( usage ) / double( budget ) ) : 0.0f;
                ImVec4 color;
                if ( fraction > 0.9f )      color = ImVec4( 1.0f, 0.3f, 0.3f, 1.0f );
                else if ( fraction > 0.7f ) color = ImVec4( 1.0f, 0.6f, 0.2f, 1.0f );
                else                        color = ImVec4( 0.3f, 0.8f, 0.4f, 1.0f );

                m_gpuBarText.sprintf( "%s ", pLabel );
                AppendFormattedBytes( m_gpuBarText, usage );
                m_gpuBarText.append( " / " );
                AppendFormattedBytes( m_gpuBarText, budget );
                m_gpuBarText.append_sprintf( " (%.1f%%)", 100.0 * fraction );
                if ( m_hasSnapshot )
                {
                    int64_t const delta = int64_t( usage ) - int64_t( snapshotUsage );
                    if ( delta != 0 )
                        AppendDeltaToString( m_gpuBarText, delta );
                }
                ImGui::ProgressBar( fraction, ImVec2( w, 0 ), m_gpuBarText.c_str() );
            };

            DrawGpuBar( "VRAM", gpuLocalUsage, gpuLocalBudget, m_snapshotGpuLocalUsage, m_snapshotGpuLocalBudget );
            DrawGpuBar( "Shared", gpuNonLocalUsage, gpuNonLocalBudget, m_snapshotGpuNonLocalUsage, m_snapshotGpuNonLocalBudget );

            uint64_t const gpuTotalUsage = gpuLocalUsage + gpuNonLocalUsage;
            uint64_t const gpuTotalBudget = gpuLocalBudget + gpuNonLocalBudget;
            uint64_t const gpuTotalUsageSnapshot = m_snapshotGpuLocalUsage + m_snapshotGpuNonLocalUsage;
            uint64_t const gpuTotalBudgetSnapshot = m_snapshotGpuLocalBudget + m_snapshotGpuNonLocalBudget;
            DrawGpuBar( "GPU Total", gpuTotalUsage, gpuTotalBudget, gpuTotalUsageSnapshot, gpuTotalBudgetSnapshot );
        }

        {
            InlineString breakdownText;
            auto AppendBreakdownItem = [&breakdownText, gpuLocalUsage, this] ( char const* pLabel, uint64_t bytes, int64_t deltaBytes )
            {
                breakdownText.append( pLabel );
                breakdownText.append( "  " );
                AppendFormattedBytes( breakdownText, bytes );
                if ( gpuLocalUsage > 0 )
                    breakdownText.append_sprintf( " (%.1f%%)", 100.0 * double( bytes ) / double( gpuLocalUsage ) );
                if ( m_hasSnapshot && deltaBytes != 0 )
                    AppendDeltaToString( breakdownText, deltaBytes );
            };

            breakdownText.clear();
            AppendBreakdownItem( "Buffers", totalBufferBytes, totalBufferBytesDelta );
            breakdownText.append( "  |  " );
            AppendBreakdownItem( "Textures", totalTextureBytes, totalTextureBytesDelta );
            breakdownText.append( "  |  " );
            AppendBreakdownItem( "Other", otherBytes, 0 );
            ImGui::TextUnformatted( breakdownText.c_str() );
        }

        ImGui::EndGroup();

        ImGui::EndChild();

        // Tables
        //-------------------------------------------------------------------------

        ImGui::BeginChild( "CpuMemoryPanel", ImVec2( halfWidth, 0 ), ImGuiChildFlags_None );

        // Sort by bytes descending
        //-------------------------------------------------------------------------

        eastl::stable_sort( m_entries.begin(), m_entries.end(), [] ( Entry const& a, Entry const& b )
        {
            return a.m_pMemoryAllocator->GetNumBytes() > b.m_pMemoryAllocator->GetNumBytes();
        } );

        for ( Group& g : m_groups )
        {
            g.m_numAllocations = 0;
            g.m_numTotalBytes = 0;
        }

        // Pass 1: bucket entries into groups
        //-------------------------------------------------------------------------

        size_t entryIdx = 0;
        for ( size_t g = 0; g < m_groups.size(); g++ )
        {
            Group& group = m_groups[g];
            for ( ; entryIdx < m_entries.size(); entryIdx++ )
            {
                uint64_t const bytes = m_entries[entryIdx].m_pMemoryAllocator->GetNumBytes();
                if ( bytes < group.m_thresholdLow || bytes >= group.m_thresholdHigh )
                    break;
                group.m_numAllocations++;
                group.m_numTotalBytes += bytes;
            }
        }

        // CPU table
        //-------------------------------------------------------------------------

        if ( ImGui::BeginTable( "CpuMemoryAllocators", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame, ImVec2( 0, 0 ) ) )
        {
            ImGui::TableSetupColumn( "Allocator" );
            ImGui::TableSetupColumn( "Bytes" );
            ImGui::TableSetupColumn( "Allocs" );

            ImGui::TableHeadersRow();

            // Total row
            //-----------------------------------------------------------------

            uint64_t numVisibleBytesTotal = 0;
            uint64_t numVisibleAllocsTotal = 0;

            size_t idx = 0;
            for ( size_t groupIndex = 0; groupIndex < m_groups.size(); groupIndex++ )
            {
                Group& group = m_groups[groupIndex];
                size_t const groupEnd = idx + group.m_numAllocations;
                for ( ; idx < groupEnd; idx++ )
                {
                    Entry const& entry = m_entries[idx];
                    if ( m_filterWidget.HasFilterSet() && !m_filterWidget.MatchesFilter( entry.m_pMemoryAllocator->GetName() ) )
                        continue;
                    numVisibleBytesTotal += entry.m_pMemoryAllocator->GetNumBytes();
                    numVisibleAllocsTotal += entry.m_pMemoryAllocator->GetNumAllocations();
                }
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted( "Total" );
            ImGui::TableNextColumn();
            DrawFormattedBytes( numVisibleBytesTotal );
            if ( totalTracked > 0 )
            {
                ImGui::SameLine( 0, 0 );
                ImGui::TextDisabled( " (%.1f%%)", 100.0 * double( numVisibleBytesTotal ) / double( totalTracked ) );
            }
            ImGui::TableNextColumn();
            ImGui::Text( "%llu", numVisibleAllocsTotal );

            // Pass 2: render
            //-----------------------------------------------------------------

            entryIdx = 0;
            for ( size_t groupIndex = 0; groupIndex < m_groups.size(); groupIndex++ )
            {
                Group& group = m_groups[groupIndex];
                if ( group.m_numAllocations == 0 )
                    continue;

                bool inTreeNode = false;

                if ( group.m_collapsible )
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();

                    group.m_headerText.sprintf( "%s : %zu items, ", group.m_pLabel, group.m_numAllocations );
                    AppendFormattedBytes( group.m_headerText, group.m_numTotalBytes );
                    group.m_headerText.append_sprintf( " (%.1f%%)", 100.0 * double( group.m_numTotalBytes ) / double( totalTracked ) );

                    ImGui::SetNextItemOpen( group.m_open );
                    bool const groupWasOpen = ImGui::TreeNodeEx( group.m_headerText.c_str(), ImGuiTreeNodeFlags_SpanAllColumns );
                    if ( groupWasOpen != group.m_open )
                        group.m_open = groupWasOpen;

                    if ( !groupWasOpen )
                    {
                        entryIdx += group.m_numAllocations;
                        continue;
                    }
                    inTreeNode = true;
                }

                size_t const groupEnd = entryIdx + group.m_numAllocations;
                for ( ; entryIdx < groupEnd; entryIdx++ )
                {
                    Entry const& entry = m_entries[entryIdx];
                    if ( m_filterWidget.HasFilterSet() && !m_filterWidget.MatchesFilter( entry.m_pMemoryAllocator->GetName() ) )
                        continue;

                    uint64_t const numBytes = entry.m_pMemoryAllocator->GetNumBytes();
                    uint64_t const numAllocs = entry.m_pMemoryAllocator->GetNumAllocations();

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TreeNodeEx( entry.m_pMemoryAllocator->GetName(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen );

                    // Bytes
                    //---------------------------------------------------------

                    ImGui::TableNextColumn();
                    DrawFormattedBytes( numBytes );

                    if ( totalTracked > 0 )
                    {
                        ImGui::SameLine( 0, 0 );
                        ImGui::TextDisabled( " (%.1f%%)", 100.0 * double( numBytes ) / double( totalTracked ) );
                    }

                    if ( m_hasSnapshot )
                    {
                        int64_t const delta = int64_t( numBytes ) - int64_t( entry.m_numBytesSnapshot );
                        DrawDeltaBytesBadge( true, delta );
                    }

                    // Allocs
                    //---------------------------------------------------------

                    ImGui::TableNextColumn();
                    ImGui::Text( "%llu", numAllocs );

                    if ( m_hasSnapshot )
                    {
                        int64_t const delta = int64_t( numAllocs ) - int64_t( entry.m_numAllocationsSnapshot );
                        DrawDeltaCountBadge( true, delta );
                    }
                }

                if ( inTreeNode )
                    ImGui::TreePop();
            }

            ImGui::EndTable();
        }

        ImGui::EndChild();

        ImGui::SameLine();

        // GPU resource table
        //-------------------------------------------------------------------------

        ImGui::BeginChild( "GpuMemoryPanel", ImVec2( 0, 0 ), ImGuiChildFlags_None );

        bool const hasGpuResources = !m_gpuBufferStats.empty() || !m_gpuTextureStats.empty();

        if ( hasGpuResources && ImGui::BeginTable( "GpuResourceTable", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame, ImVec2( 0, 0 ) ) )
        {
            ImGui::TableSetupColumn( "Resource Type" );
            ImGui::TableSetupColumn( "Bytes" );
            ImGui::TableSetupColumn( "Count" );

            ImGui::TableHeadersRow();

            // Merge-display buffers + textures sorted by size
            //-----------------------------------------------------------------

            auto GetDeltas = []( EE::Render::RHI::ResourceAllocationStatistic const& stat,
                                  TVector<EE::Render::RHI::ResourceAllocationStatistic> const& snapshotStats,
                                  int64_t& outDeltaBytes, int64_t& outDeltaCount )
            {
                for ( auto const& snapStat : snapshotStats )
                {
                    if ( snapStat.m_descriptorTypes == stat.m_descriptorTypes )
                    {
                        outDeltaBytes = int64_t( stat.m_numBytes ) - int64_t( snapStat.m_numBytes );
                        outDeltaCount = int64_t( stat.m_numAllocations ) - int64_t( snapStat.m_numAllocations );
                        return;
                    }
                }
                // Not found in snapshot - treat as entirely new
                outDeltaBytes = int64_t( stat.m_numBytes );
                outDeltaCount = int64_t( stat.m_numAllocations );
            };

            size_t bufferIndex = 0;
            size_t textureIndex = 0;

            while ( bufferIndex < m_gpuBufferStats.size() || textureIndex < m_gpuTextureStats.size() )
            {
                bool const takeBuffer = textureIndex >= m_gpuTextureStats.size() ||
                    ( bufferIndex < m_gpuBufferStats.size() && m_gpuBufferStats[bufferIndex].m_numBytes >= m_gpuTextureStats[textureIndex].m_numBytes );

                char const* pTypeLabel;
                EE::Render::RHI::ResourceAllocationStatistic const* pStat;
                TVector<EE::Render::RHI::ResourceAllocationStatistic> const* pSnapshotStats;

                if ( takeBuffer )
                {
                    pTypeLabel = "Buffer";
                    pStat = &m_gpuBufferStats[bufferIndex];
                    pSnapshotStats = &m_snapshotGpuBufferStats;
                    bufferIndex++;
                }
                else
                {
                    pTypeLabel = "Texture";
                    pStat = &m_gpuTextureStats[textureIndex];
                    pSnapshotStats = &m_snapshotGpuTextureStats;
                    textureIndex++;
                }

                m_gpuResourceLabel.clear();
                m_gpuResourceLabel.append( pTypeLabel );
                m_gpuResourceLabel.append( " [" );
                AppendDescriptorTypeFlags( m_gpuResourceLabel, pStat->m_descriptorTypes );
                m_gpuResourceLabel.append( "]" );

                int64_t deltaBytes = 0;
                int64_t deltaCount = 0;
                if ( m_hasSnapshot )
                    GetDeltas( *pStat, *pSnapshotStats, deltaBytes, deltaCount );

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TreeNodeEx( m_gpuResourceLabel.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_NoTreePushOnOpen );

                // Bytes
                //-------------------------------------------------------------

                ImGui::TableNextColumn();
                DrawFormattedBytes( pStat->m_numBytes );
                if ( totalTrackedGpuBytes > 0 )
                {
                    ImGui::SameLine( 0, 0 );
                    ImGui::TextDisabled( " (%.1f%%)", 100.0 * double( pStat->m_numBytes ) / double( totalTrackedGpuBytes ) );
                }
                if ( m_hasSnapshot && deltaBytes != 0 )
                    DrawDeltaBytesBadge( true, deltaBytes );

                // Count
                //-------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::Text( "%llu", pStat->m_numAllocations );
                if ( m_hasSnapshot && deltaCount != 0 )
                    DrawDeltaCountBadge( true, deltaCount );
            }

            ImGui::EndTable();
        }

        ImGui::EndChild();
    }
}
#endif
