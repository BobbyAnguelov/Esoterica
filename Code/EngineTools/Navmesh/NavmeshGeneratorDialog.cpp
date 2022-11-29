#include "NavmeshGeneratorDialog.h"
#include "NavmeshGenerator.h"
#include "EngineTools/Resource/ResourceDatabase.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"
#include "Engine/UpdateContext.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    NavmeshGeneratorDialog::NavmeshGeneratorDialog( ToolsContext const* pToolsContext, NavmeshBuildSettings const& initialBuildSettings, EntityModel::SerializedEntityCollection const& entityCollection, FileSystem::Path const& navmeshOutputPath )
        : m_pToolsContext( pToolsContext )
        , m_buildSettings( initialBuildSettings )
        , m_entityCollection( entityCollection )
        , m_navmeshOutputPath( navmeshOutputPath )
        , m_propertyGrid( pToolsContext )
    {
        m_propertyGrid.SetTypeToEdit( &m_buildSettings );
        m_propertyGrid.ExpandAllPropertyViews();
    }

    NavmeshGeneratorDialog::~NavmeshGeneratorDialog()
    {
        EE_ASSERT( m_pGenerator == nullptr );
    }

    bool NavmeshGeneratorDialog::UpdateAndDrawDialog( UpdateContext const& ctx )
    {
        constexpr static char const* navmeshGeneratorTitle = "Navmesh Generator";

        // Update build progress
        //-------------------------------------------------------------------------

        #if EE_ENABLE_NAVPOWER
        if ( m_pGenerator != nullptr )
        {
            auto const state = m_pGenerator->GetState();
            if ( state != NavmeshGenerator::State::Generating )
            {
                if ( state == NavmeshGenerator::State::CompletedSuccess )
                {
                    pfd::message( "Generation Complete", "Navmesh generated successfully!", pfd::choice::ok, pfd::icon::info ).result();
                }
                else
                {
                    pfd::message( "Generation Complete", "Navmesh generation failed! Please check system log for details!", pfd::choice::ok, pfd::icon::error ).result();
                }

                EE::Delete( m_pGenerator );
            }
        }
        #endif

        // Dialog pop up
        //-------------------------------------------------------------------------

        if ( !ImGui::IsPopupOpen( navmeshGeneratorTitle ) )
        {
            ImGui::OpenPopup( navmeshGeneratorTitle );
        }

        bool isOpen = true;
        ImGui::SetNextWindowSize( ImVec2( 600, 800 ), ImGuiCond_FirstUseEver );
        if ( ImGui::BeginPopupModal( navmeshGeneratorTitle, &isOpen ) )
        {
            // Generator
            if ( m_pGenerator != nullptr )
            {
                #if EE_ENABLE_NAVPOWER
                ImGui::Text( m_pGenerator->GetProgressMessage() );
                ImGui::ProgressBar( m_pGenerator->GetProgressBarValue(), ImVec2( -1.0f, 0.0f ) );
                #endif
            }
            else // Build Settings
            {
                #if EE_ENABLE_NAVPOWER
                if ( ImGuiX::ColoredButton( ImGuiX::ImColors::Green, ImGuiX::ImColors::White, "Generate", ImVec2( -1, 0 ) ) )
                {
                    m_pGenerator = EE::New<NavmeshGenerator>( *m_pToolsContext->m_pTypeRegistry, m_pToolsContext->m_pResourceDatabase->GetRawResourceDirectoryPath(), m_navmeshOutputPath, m_entityCollection, m_buildSettings );
                    m_pGenerator->GenerateAsync( *ctx.GetSystem<TaskSystem>() );
                }
                #endif

                if ( ImGui::BeginChild( "PG", ImVec2( -1, 0 ) ) )
                {
                    m_propertyGrid.DrawGrid();
                }
                ImGui::EndChild();
            }

            ImGui::EndPopup();
        }

        // Always disable build logging
        bool const isPopupClosed = isOpen && ImGui::IsPopupOpen( navmeshGeneratorTitle );
        if ( !isPopupClosed )
        {
            m_buildSettings.m_enableBuildLogging = false;
        }

        return isPopupClosed;
    }
}