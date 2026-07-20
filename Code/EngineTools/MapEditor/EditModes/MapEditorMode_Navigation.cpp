#include "MapEditorMode_Navigation.h"
#include "EngineTools/Entity/EntitySerializationTools.h"
#include "Engine/Navmesh/Components/Component_Navmesh.h"
#include "Engine/Entity/EntityMap.h"
#include "Engine/Navmesh/Settings/ViewportSettings_Navmesh.h"
#include "Base/Resource/ResourceSystem.h"
#include "Base/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    NavigationMapEditorMode::~NavigationMapEditorMode()
    {
        EE_ASSERT( m_pPropertyGrid == nullptr );
    }

    void NavigationMapEditorMode::Initialize( EntityModel::EditorContext* pEntityEditorContext )
    {
        EntityModel::MapEditorMode::Initialize( pEntityEditorContext );

        auto pWorld = m_pEntityEditorContext->GetWorld();
        auto pViewport = pWorld->GetMainViewport();
        if ( pViewport != nullptr )
        {
            NavmeshViewportSettings* pViewportSettings = pViewport->GetViewportSettings<NavmeshViewportSettings>();
            pViewportSettings->m_drawDebug = true;
            pViewportSettings->ShowAreas( pWorld );
        }
    }

    void NavigationMapEditorMode::Shutdown()
    {
        if ( m_pAsyncTask != nullptr && !m_pAsyncTask->GetIsComplete() )
        {
            TaskSystem* pTaskSystem = m_pToolsContext->m_pSystemRegistry->GetSystem<TaskSystem>();
            pTaskSystem->WaitForTask( m_pAsyncTask );
            EE::Delete( m_pAsyncTask );
        }

        auto pWorld = m_pEntityEditorContext->GetWorld();
        auto pViewport = pWorld->GetMainViewport();
        if ( pViewport != nullptr )
        {
            NavmeshViewportSettings* pViewportSettings = pViewport->GetViewportSettings<NavmeshViewportSettings>();
            pViewportSettings->m_drawDebug = false;
        }

        EntityModel::MapEditorMode::Shutdown();
    }

    void NavigationMapEditorMode::PrePropertyGridChange( PropertyEditInfo const& info )
    {
        if ( m_pNavmeshComponent != nullptr )
        {
            m_pEntityEditorContext->BeginSingleComponentEdit( m_pNavmeshEntity, m_pNavmeshComponent );
        }
    }

    void NavigationMapEditorMode::PostPropertyGridChange( PropertyEditInfo const& info )
    {
        if ( m_pNavmeshComponent != nullptr )
        {
            m_pEntityEditorContext->EndSingleComponentEdit( m_pNavmeshComponent );
        }
    }

    //-------------------------------------------------------------------------

    ResourceID NavigationMapEditorMode::GetNavmeshResourceIDForCurrentlyEditedMap() const
    {
        EntityModel::EntityMap* pEditedMap = m_pEntityEditorContext->GetEditedMap();
        return NavmeshData::GetNavmeshResourceIDForMap( pEditedMap->GetMapResourceID() );
    }

    FileSystem::Path NavigationMapEditorMode::GetUserGeneratedNavmeshFilePathForCurrentlyEditedMap() const
    {
        return NavmeshData::GetUserGeneratedNavmeshFilePathForMap( GetNavmeshResourceIDForCurrentlyEditedMap(), m_pToolsContext->GetSourceDataDirectory() );
    }

    void NavigationMapEditorMode::ReloadNavmeshResourceForCurrentlyEditedMap()
    {
        auto pResourceSystem = m_pToolsContext->m_pSystemRegistry->GetSystem<Resource::ResourceSystem>();
        ResourceID const navmeshResourceID = GetNavmeshResourceIDForCurrentlyEditedMap();
        pResourceSystem->RequestResourceHotReload( navmeshResourceID );
    }

    //-------------------------------------------------------------------------

    void NavigationMapEditorMode::UpdateAndDraw( UpdateContext const& context, bool isFocused )
    {
        auto pWorld = m_pEntityEditorContext->GetWorld();
        auto const& navmeshComponents = pWorld->GetAllComponentsOfType<NavmeshComponent>();
        if ( navmeshComponents.empty() )
        {
            m_pNavmeshEntity = nullptr;
            m_pNavmeshComponent = nullptr;
            m_hasMultipleNavmeshComponents = false;
        }
        else
        {
            m_pNavmeshEntity = pWorld->FindEntity( navmeshComponents.front()->GetEntityID() );
            m_pNavmeshComponent = Cast<NavmeshComponent>( m_pNavmeshEntity->FindComponent( navmeshComponents.front()->GetID() ) );
            m_hasMultipleNavmeshComponents = navmeshComponents.size() > 1;
        }

        //-------------------------------------------------------------------------

        if ( ImGui::BeginTabBar( "tb" ) )
        {
            if ( ImGui::BeginTabItem( "Navmesh" ) )
            {
                DrawNavmeshTab( context, isFocused );

                ImGui::EndTabItem();
            }

            if ( ImGui::BeginTabItem( "Tester" ) )
            {
                DrawTesterTab( context, isFocused );

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }

    void NavigationMapEditorMode::DrawNavmeshTab( UpdateContext const& context, bool isFocused )
    {
        EntityModel::EntityMap* pEditedMap = m_pEntityEditorContext->GetEditedMap();
        ResourceID const navmeshResourceID = NavmeshData::GetNavmeshResourceIDForMap( pEditedMap->GetMapResourceID() );

        // No Navmesh
        //-------------------------------------------------------------------------

        if ( m_pNavmeshComponent == nullptr )
        {
            if ( ImGui::Button( "Create Navmesh Component" ) )
            {
                Entity* pEntity = EE::New<Entity>( StringID( "Navmesh Entity" ) );
                pEntity->AddComponent( EE::New<NavmeshComponent>( navmeshResourceID ) );
                m_pEntityEditorContext->AddEntityToMap( pEditedMap, pEntity );
            }
        }

        // Invalid State
        //-------------------------------------------------------------------------

        else if( m_hasMultipleNavmeshComponents )
        {
            ImGui::Text( "More than one navmesh component found in this map, this is not supported!" );

            if ( ImGui::Button( "Delete extra components" ) )
            {
                EE_UNIMPLEMENTED_FUNCTION();
            }
        }

        // Valid State
        //-------------------------------------------------------------------------

        else
        {
            ImGui::SeparatorText( "User Generated Navmesh" );

            FileSystem::Path const userGeneratedNavmeshPath = GetUserGeneratedNavmeshFilePathForCurrentlyEditedMap();
            if ( userGeneratedNavmeshPath.Exists() )
            {
                constexpr static char const * const rebuildLabel = EE_ICON_WRENCH" Rebuild";
                constexpr static char const * const deleteLabel = EE_ICON_TRASH_CAN;
                float const rebuildButtonWidth = ImGuiX::CalculateButtonWidth( rebuildLabel );
                float const deleteButtonWidth = ImGuiX::CalculateButtonWidth( deleteLabel );
                float const buttonOffset = ImGui::GetContentRegionAvail().x - rebuildButtonWidth - deleteButtonWidth + ( ImGui::GetStyle().ItemSpacing.x );

                ImGui::AlignTextToFramePadding();
                ImGui::Text( "User Generated Navmesh Exists" );
                
                ImGui::BeginDisabled( IsGeneratingNavmesh() );
                ImGui::SameLine( buttonOffset );
                if ( ImGuiX::ButtonColored( rebuildLabel, Colors::Green, Colors::White, ImVec2( rebuildButtonWidth, 0 ), true ) )
                {
                    EE_ASSERT( m_generationStage == GenerationStage::None );
                    m_generationStage = GenerationStage::ExtractBuildData;
                }
                ImGuiX::ItemTooltip( "Rebuild the user generated navmesh!" );

                ImGui::SameLine( buttonOffset + rebuildButtonWidth + ImGui::GetStyle().ItemSpacing.x );
                if ( ImGuiX::ButtonColored( deleteLabel, Colors::FireBrick, Colors::White, ImVec2( deleteButtonWidth, 0 ), true ) )
                {
                    EE_ASSERT( m_generationStage == GenerationStage::None );
                    FileSystem::EraseFile( userGeneratedNavmeshPath );
                    ReloadNavmeshResourceForCurrentlyEditedMap();
                }
                ImGuiX::ItemTooltip( "Delete user generated navmesh and dynamically regenerate the navmesh!" );
                ImGui::EndDisabled();
            }
            else
            {
                constexpr static char const * const generateLabel = EE_ICON_WRENCH" Generate";
                float const buttonWidth = ImGuiX::CalculateButtonWidth( generateLabel );
                float const buttonOffset = ImGui::GetContentRegionAvail().x - ImGuiX::CalculateButtonWidth( generateLabel ) + ImGui::GetStyle().ItemSpacing.x;

                ImGui::AlignTextToFramePadding();
                ImGui::Text( "No User Generated Navmesh Exists" );
                ImGui::SameLine( buttonOffset );
                ImGui::BeginDisabled( IsGeneratingNavmesh() );
                if ( ImGuiX::ButtonColored( generateLabel, Colors::Green, Colors::White, ImVec2( buttonWidth, 0 ), true ) )
                {
                    EE_ASSERT( m_generationStage == GenerationStage::None );
                    m_generationStage = GenerationStage::ExtractBuildData;
                }
                ImGuiX::ItemTooltip( "Create a user generated navmesh! Navmesh will need to be manually rebuilt each time the level is changed." );
                ImGui::EndDisabled();
            }

            //-------------------------------------------------------------------------

            if ( m_generationStage == GenerationStage::None )
            {
                ImGui::SeparatorText( "Build Settings" );

                if ( m_pPropertyGrid->GetEditedType() != &m_pNavmeshComponent->GetBuildSettings() )
                {
                    m_pPropertyGrid->SetTypeToEdit( &m_pNavmeshComponent->GetBuildSettings() );
                }

                m_pPropertyGrid->UpdateAndDraw();
            }
            else // Update and Draw generation progress
            {
                ImGui::SeparatorText( "Navmesh Generation" );

                EE_ASSERT( m_generationStage != GenerationStage::None );
                switch ( m_generationStage )
                {
                    case GenerationStage::ExtractBuildData:
                    {
                        UpdateAndDrawExtractBuildDataStage( context );
                    }
                    break;

                    case GenerationStage::Generate:
                    {
                        UpdateAndDrawGenerateNavmeshStage( context );
                    }
                    break;

                    case GenerationStage::Report:
                    {
                        DrawGenerationReportStage( context );
                    }
                    break;

                    default:
                    EE_UNREACHABLE_CODE();
                    break;
                }
            }
        }
    }

    void NavigationMapEditorMode::DrawTesterTab( UpdateContext const& context, bool isFocused )
    {

    }

    void NavigationMapEditorMode::UpdateAndDrawExtractBuildDataStage( UpdateContext const& context )
    {
        EE_ASSERT( m_generationStage == GenerationStage::ExtractBuildData );

        if ( m_pAsyncTask == nullptr )
        {
            auto RunExtract = [this] ( TaskSetPartition range, uint32_t threadnum )
            {
                EntityModel::EntityMapDescriptor mapDesc;
                Log log( LogCategory::Navmesh, "Extract Map Data" );
                EntityModel::CreateEntityMapDescriptor( *m_pToolsContext->m_pTypeRegistry, log, m_pEntityEditorContext->GetEditedMap(), m_mapDesc );

                m_buildData.ExtractBuildData( *m_pToolsContext->m_pTypeRegistry, m_mapDesc );
                m_buildData.LoadCollisionMeshes( *m_pToolsContext->m_pTypeRegistry, m_pToolsContext->GetSourceDataDirectory() );
            };

            EE_ASSERT( m_pNavmeshComponent != nullptr );
            m_buildData.m_buildSettings = m_pNavmeshComponent->GetBuildSettings();

            TaskSystem* pTaskSystem = m_pToolsContext->m_pSystemRegistry->GetSystem<TaskSystem>();
            m_pAsyncTask = EE::New<AsyncTask>( RunExtract );
            pTaskSystem->ScheduleTask( m_pAsyncTask );
        }
        else
        {
            if ( m_pAsyncTask->GetIsComplete() )
            {
                EE::Delete( m_pAsyncTask );

                if ( !m_buildData.IsReadyToBuild() )
                {
                    m_generationStage = GenerationStage::Report;
                }
                else
                {
                    #if EE_ENABLE_NAVPOWER
                    FileSystem::Path navmeshOutputPath = GetUserGeneratedNavmeshFilePathForCurrentlyEditedMap();

                    auto RunGenerateNavmesh = [this, navmeshOutputPath] ( TaskSetPartition range, uint32_t threadnum )
                    {
                        if ( m_builder.Build( m_buildData, m_navmeshData ) )
                        {
                            EE_ASSERT( m_navmeshData.IsValid() );

                            Serialization::BinaryOutputArchive archive;
                            archive << m_navmeshData;

                            if ( !archive.WriteToFile( navmeshOutputPath ) )
                            {
                                m_builder.LogError( "Failed to save navmesh data to output path: %s", navmeshOutputPath.c_str() );
                            }
                        }
                    };

                    TaskSystem* pTaskSystem = m_pToolsContext->m_pSystemRegistry->GetSystem<TaskSystem>();
                    m_pAsyncTask = EE::New<AsyncTask>( 1, RunGenerateNavmesh );
                    pTaskSystem->ScheduleTask( m_pAsyncTask );
                    #endif

                    m_generationStage = GenerationStage::Generate;
                }
            }
        }

        //-------------------------------------------------------------------------

        ImGui::AlignTextToFramePadding();
        ImGui::Text( "Extracting Navmesh Build Data: " );
        ImGui::SameLine();
        ImGuiX::DrawSpinner( "Extracting Build Data" );
    }

    void NavigationMapEditorMode::UpdateAndDrawGenerateNavmeshStage( UpdateContext const& context )
    {
        EE_ASSERT( m_generationStage == GenerationStage::Generate );

        #if EE_ENABLE_NAVPOWER
        ImGui::AlignTextToFramePadding();
        ImGui::Text( "Generating Navmesh: " );
        ImGui::SameLine();
        ImGui::ProgressBar( m_builder.GetProgress(), ImVec2( -1.0f, 0.0f ) );

        if ( m_pAsyncTask->GetIsComplete() )
        {
            EE::Delete( m_pAsyncTask );
            m_generationStage = GenerationStage::Report;
            ReloadNavmeshResourceForCurrentlyEditedMap();
        }
        #endif
    }

    void NavigationMapEditorMode::DrawGenerationReportStage( UpdateContext const& context )
    {
        enum Result
        {
            Success = 0,
            SuccessWithWarnings,
            Failed,
            BuildDataFailed,
        };

        static Float4 colors[4] =
        {
            Colors::Lime.ToFloat4(),
            Colors::Gold.ToFloat4(),
            Colors::Red.ToFloat4(),
            Colors::Red.ToFloat4()
        };

        constexpr static char const * const labels[4] =
        {
            EE_ICON_CHECK" Navmesh Generation Succeeded!",
            EE_ICON_EXCLAMATION" Navmesh Generation Succeeded With Warnings!",
            EE_ICON_CLOSE" Navmesh Generation Failed!",
            EE_ICON_CLOSE" Navmesh Build Data Extraction Failed!"
        };

        Result result = Result::Success;

        #if EE_ENABLE_NAVPOWER
        if ( !m_builder.HasErrors() && !m_buildData.HasErrors() )
        {
            result = Result::Success;
        }
        else if ( ( !m_builder.HasErrors() && !m_buildData.HasErrors() ) && ( m_builder.HasWarnings() || m_buildData.HasWarnings() ) )
        {
            result = Result::SuccessWithWarnings;
        }
        else if ( m_builder.HasErrors() && !m_buildData.HasErrors() )
        {
            result = Result::Failed;
        }
        else
        {
            result = Result::BuildDataFailed;
        }
        #endif

        float const textOffset = ( ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize( labels[result] ).x ) / 2 ;
        ImGui::NewLine();
        ImGui::SameLine( textOffset );
        ImGui::TextColored( colors[result], labels[result] );

        //-------------------------------------------------------------------------

        if ( result != Result::Success )
        {
            if ( ImGui::BeginTable( "NGL", 3, ImGuiTableFlags_Borders, ImVec2( -1, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() ) ) )
            {
                ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 32 );
                ImGui::TableSetupColumn( "Source", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 100 );
                ImGui::TableSetupColumn( "Message", ImGuiTableColumnFlags_WidthStretch );

                ImGui::TableHeadersRow();

                for ( auto& log : m_buildData.GetLogEntries() )
                {
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding();
                    ImGuiX::DrawSeverityIcon( log.m_severity );

                    ImGui::TableNextColumn();
                    ImGui::Text( "Build Data" );

                    ImGui::TableNextColumn();
                    ImGui::Text( log.m_message.c_str() );
                }

                #if EE_ENABLE_NAVPOWER
                for ( auto& log : m_builder.GetLogEntries() )
                {
                    ImGui::TableNextRow();
                    ImGuiX::DrawSeverityIcon( log.m_severity );
                 
                    ImGui::TableNextColumn();
                    ImGui::Text( "Builder" );

                    ImGui::TableNextColumn();
                    ImGui::Text( "Builder" );

                    ImGui::TableNextColumn();
                    ImGui::Text( log.m_message.c_str() );
                }
                #endif

                ImGui::EndTable();
            }
        }

        //-------------------------------------------------------------------------

        if ( ImGui::Button( EE_ICON_CLOSE" Close Report", ImVec2( -1, 0 ) ) )
        {
            m_generationStage = GenerationStage::None;
        }
    }
}