#include "AnimationClipBrowser.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationClip.h"
#include "EngineTools/FileSystem/FileRegistry.h"
#include "EngineTools/Core/CommonToolTypes.h"

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
        if ( skeletonID != m_skeleton )
        {
            m_skeleton = skeletonID;
            RebuildCache();
        }
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

        // Clear selection if not in filtered list
        if ( eastl::find( m_filteredClips.begin(), m_filteredClips.end(), m_selectedClipID ) == m_filteredClips.end() )
        {
            m_selectedClipID.Clear();
        }
    }

    void AnimationClipBrowser::RebuildCache()
    {
        m_clips.clear();
        m_filteredClips.clear();
        m_selectedClipID.Clear();

        if ( !m_skeleton.IsValid() || !m_pToolsContext->m_pFileRegistry->IsDataFileCacheBuilt() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        auto SkeletonFilter = [this] ( Resource::ResourceDescriptor const* pDescriptor )
        {
            auto pClipDescriptor = Cast<AnimationClipResourceDescriptor>( pDescriptor );
            return pClipDescriptor->m_skeleton.GetResourceID() == m_skeleton;
        };

        m_clips = m_pToolsContext->m_pFileRegistry->GetAllResourcesOfTypeFiltered( AnimationClip::GetStaticResourceTypeID(), SkeletonFilter );
        UpdateFilter();

        m_refreshTimer.Reset();
    }

    void AnimationClipBrowser::Draw( UpdateContext const& context, bool isFocused )
    {
        // Update Cache
        //-------------------------------------------------------------------------

        if ( m_skeleton.IsValid() )
        {
            if ( m_refreshTimer.GetElapsedTimeSeconds() > 1.0f && m_pToolsContext->m_pFileRegistry->IsDataFileCacheBuilt() )
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
                        bool isSelected = m_filteredClips[i] == m_selectedClipID;

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        if ( ImGui::Selectable( m_filteredClips[i].c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick ) )
                        {
                            m_selectedClipID = m_filteredClips[i];

                            // Open clip resource
                            if ( ImGui::IsMouseDoubleClicked( ImGuiMouseButton_Left ) )
                            {
                                m_pToolsContext->TryOpenResource( m_filteredClips[i] );
                            }
                        }

                        // Drag and drop support
                        if ( ImGui::BeginDragDropSource( ImGuiDragDropFlags_None ) )
                        {
                            ImGui::SetDragDropPayload( DragAndDrop::s_filePayloadID, (void*) m_filteredClips[i].GetDataPath().c_str(), m_filteredClips[i].GetDataPath().GetString().length() + 1 );
                            ImGui::Text( m_filteredClips[i].GetDataPath().GetFilename().c_str() );
                            ImGui::EndDragDropSource();
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