#include "AnimationClipBrowser.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationClip.h"
#include "EngineTools/Resource/ResourceDatabase.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    AnimationClipBrowser::AnimationClipBrowser( ToolsContext const* pToolsContext )
        : m_pToolsContext( pToolsContext )
    {
        EE_ASSERT( pToolsContext != nullptr );
    }

    void AnimationClipBrowser::ClearFilter()
    {
        m_filter.Clear();
        UpdateFilter();
    }

    void AnimationClipBrowser::SetSkeleton( ResourceID const& skeletonID )
    {
        m_skeleton = skeletonID;
        RebuildCache();
    }

    void AnimationClipBrowser::UpdateFilter()
    {
        m_filteredClips.clear();

        for ( auto const& clip : m_clips )
        {
            if ( m_filter.MatchesFilter( clip.c_str() ) )
            {
                m_filteredClips.emplace_back( clip );
            }
        }
    }

    void AnimationClipBrowser::RebuildCache()
    {
        m_clips.clear();
        m_filteredClips.clear();

        if ( !m_skeleton.IsValid() || !m_pToolsContext->m_pResourceDatabase->IsDescriptorCacheBuilt() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        auto SkeletonFilter = [this] ( Resource::ResourceDescriptor const* pDescriptor )
        {
            auto pClipDescriptor = Cast<AnimationClipResourceDescriptor>( pDescriptor );
            return pClipDescriptor->m_skeleton.GetResourceID() == m_skeleton;
        };

        m_clips = m_pToolsContext->m_pResourceDatabase->GetAllResourcesOfTypeFiltered( AnimationClip::GetStaticResourceTypeID(), SkeletonFilter );
        UpdateFilter();

        m_refreshTimer.Reset();
    }

    void AnimationClipBrowser::Draw( UpdateContext const& context, bool isFocused )
    {
        // Update Cache
        //-------------------------------------------------------------------------

        if ( m_skeleton.IsValid() )
        {
            if ( m_refreshTimer.GetElapsedTimeSeconds() > 1.0f && m_pToolsContext->m_pResourceDatabase->IsDescriptorCacheBuilt() )
            {
                RebuildCache();
            }
        }

        // Filter clips
        //-------------------------------------------------------------------------

        if ( m_filter.UpdateAndDraw() )
        {
            UpdateFilter();
        }

        // Draw Clips
        //-------------------------------------------------------------------------

        if ( ImGui::BeginChild( "Clips" ) )
        {
            ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 4 ) );
            if ( ImGui::BeginTable( "ClipBrowser", 1, ImGuiTableFlags_RowBg, ImVec2( -1, -1 ) ) )
            {
                ImGuiListClipper clipper;
                clipper.Begin( (int32_t) m_filteredClips.size() );
                while ( clipper.Step() )
                {
                    for ( int32_t i = clipper.DisplayStart; i < clipper.DisplayEnd; i++ )
                    {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        if ( ImGuiX::FlatButton( m_filteredClips[i].c_str() ) )
                        {
                            m_pToolsContext->TryOpenResource( m_filteredClips[i] );
                        }
                    }
                }

                ImGui::EndTable();
            }
            ImGui::PopStyleVar();
        }
        ImGui::EndChild();
    }
}