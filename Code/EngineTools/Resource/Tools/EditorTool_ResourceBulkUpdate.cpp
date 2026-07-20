#include "EditorTool_ResourceBulkUpdate.h"
#include "EngineTools/Resource/ResourceBulkUpdateOperation.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    ResourceBulkUpdateEditorTool::ResourceBulkUpdateEditorTool( ToolsContext const* pToolsContext )
        : EditorTool( pToolsContext, "Resource Bulk Edit" )
    {
        m_operationTypes = m_pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( BulkUpdateOperation::GetStaticTypeID(), false, false, true );
    }

    ResourceBulkUpdateEditorTool::~ResourceBulkUpdateEditorTool()
    {
        StopOperation();
    }

    void ResourceBulkUpdateEditorTool::Initialize( UpdateContext const& context )
    {
        EditorTool::Initialize( context );
        CreateToolWindow( "Bulk Edit", [this] ( UpdateContext const& context, bool isFocused ) { DrawWindow( context, isFocused ); } );
    }

    void ResourceBulkUpdateEditorTool::DrawWindow( UpdateContext const& context, bool isFocused )
    {
        // Update Active Operation
        //-------------------------------------------------------------------------

        if ( m_pOperation != nullptr )
        {
            float const buttonWidth = ImGuiX::CalculateButtonWidth( "Stop" );
            float const progressBarWidth = ImGui::GetContentRegionAvail().x - buttonWidth - ImGui::GetStyle().ItemSpacing.x;

            size_t const numResourcesToProcess = m_resourcesToProcess.size();
            int32_t const numResourcesProcessed = Math::Clamp( m_resourceToProcessIdx + 1, 0, INT32_MAX );
            float const progress = float( numResourcesProcessed ) / numResourcesToProcess;
            InlineString const progressStr( InlineString::CtorSprintf(), "%d / %d", numResourcesProcessed, numResourcesToProcess );
            ImGui::ProgressBar( progress, ImVec2( progressBarWidth, 0 ), progressStr.c_str() );

            ImGui::SameLine();

            if ( ImGuiX::ButtonColored( "Stop", Colors::Red, Colors::White, ImVec2( buttonWidth, 0 ) ) )
            {
                StopOperation();
            }
            else
            {
                UpdateOperation( context );
            }
        }

        // Pick New Operation
        //-------------------------------------------------------------------------

        else
        {
            float const buttonWidth = ImGuiX::CalculateButtonWidth( "Go" );
            float const comboWidth = ImGui::GetContentRegionAvail().x - buttonWidth - ImGui::GetStyle().ItemSpacing.x;

            ImGui::SetNextItemWidth( comboWidth );
            if ( ImGui::BeginCombo( "##OpType", ( m_pSelectedOperationType == nullptr ) ? "" : m_pSelectedOperationType->m_friendlyName.c_str() ) )
            {
                for ( auto const pTypeInfo : m_operationTypes )
                {
                    bool const isSelected = m_pSelectedOperationType == pTypeInfo;
                    if ( ImGui::Selectable( pTypeInfo->m_friendlyName.c_str(), isSelected ) )
                    {
                        m_pSelectedOperationType = pTypeInfo;
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine();

            ImGui::BeginDisabled( m_pSelectedOperationType == nullptr );
            if ( ImGuiX::ButtonColored( "Go", Colors::Green, Colors::White, ImVec2( buttonWidth, 0 ) ) )
            {
                TryStartOperation();
            }
            ImGui::EndDisabled();
        }

        // Log
        //-------------------------------------------------------------------------

        ImGui::InputTextMultiline( "##Log", m_logBuffer.Data(), m_logBuffer.Size(), ImGui::GetContentRegionAvail(), ImGuiInputTextFlags_ReadOnly );
    }

    void ResourceBulkUpdateEditorTool::TryStartOperation()
    {
        EE_ASSERT( m_pOperation == nullptr );
        EE_ASSERT( m_pSelectedOperationType != nullptr );

        // Create operation
        //-------------------------------------------------------------------------

        m_pOperation = Cast<BulkUpdateOperation>( m_pSelectedOperationType->CreateType() );
        if ( m_pOperation == nullptr )
        {
            return;
        }

        // Clear state
        //-------------------------------------------------------------------------

        m_log.ClearLog();
        m_logBuffer.Clear();
        m_resourcesToProcess.clear();
        m_loadedResource.Clear();
        m_resourceToProcessIdx = -1;
        m_resourceStage = Stage::None;

        // Get all applicable resources to process
        //-------------------------------------------------------------------------

        TInlineVector<ResourceTypeID, 4> const applicableResourceTypes = m_pOperation->GetApplicableTypeIDs();
        for ( ResourceTypeID const& typeID : applicableResourceTypes )
        {
            TVector<ResourceID> const foundResources = m_pToolsContext->m_pFileRegistry->GetAllResourcesOfType( typeID );
            m_resourcesToProcess.insert( m_resourcesToProcess.end(), foundResources.begin(), foundResources.end() );
        }

        if ( m_resourcesToProcess.empty() )
        {
            StopOperation();
        }
        else
        {
            m_resourceToProcessIdx = -1;
            m_resourceStage = Stage::ProcessNextResource;
        }
    }

    void ResourceBulkUpdateEditorTool::UpdateOperation( UpdateContext const& context )
    {
        EE_ASSERT( m_pOperation != nullptr );

        InlineString str;

        switch ( m_resourceStage )
        {
            case Stage::None:
            {
                EE_UNREACHABLE_CODE();
            }
            break;

            case Stage::ProcessNextResource:
            {
                m_resourceToProcessIdx++;

                if ( m_resourceToProcessIdx >= m_resourcesToProcess.size() )
                {
                    StopOperation();
                }
                else
                {
                    ResourceID const& resourceID = m_resourcesToProcess[m_resourceToProcessIdx];
                    auto pDescriptor = m_pToolsContext->m_pFileRegistry->GetLoadedDescriptor<ResourceDescriptor>( resourceID );
                    if ( pDescriptor != nullptr )
                    {
                        if ( m_pOperation->RequireResourceLoad() )
                        {
                            m_loadedResource = resourceID;
                            LoadResource( &m_loadedResource );
                            m_resourceStage = Stage::WaitForResourceLoad;
                        }
                        else
                        {
                            m_resourceStage = Stage::PerformOperationNoLoad;
                        }
                    }
                }
            }
            break;

            case Stage::WaitForResourceLoad:
            {
                // Do Nothing
            }
            break;

            case Stage::PerformOperationNoLoad:
            {
                ResourceID const& resourceID = m_resourcesToProcess[m_resourceToProcessIdx];
                FileSystem::Path const filePath = GetFileSystemPath( resourceID.GetDataPath() );

                ResourceDescriptor* pDescriptor = ResourceDescriptor::TryReadFromFile( *m_pToolsContext->m_pTypeRegistry, m_log, filePath );
                if ( pDescriptor != nullptr )
                {
                    if ( m_pOperation->PerformOperation( m_pToolsContext, nullptr, *pDescriptor ) )
                    {
                        if ( !ResourceDescriptor::TryWriteToFile( *m_pToolsContext->m_pTypeRegistry, m_log, filePath, pDescriptor ) )
                        {
                            str.sprintf( "Failed to Save: %s\n", filePath.c_str() );
                        }
                    }

                    str.sprintf( "Processed: %s\n", resourceID.c_str() );

                    EE::Delete( pDescriptor );
                }
                else
                {
                    str.sprintf( "Failed to load: %s\n", filePath.c_str() );
                }

                m_logBuffer.Append( str );

                m_resourceStage = Stage::ProcessNextResource;
            }
            break;

            case Stage::PerformOperation:
            {
                ResourceID const& resourceID = m_resourcesToProcess[m_resourceToProcessIdx];
                FileSystem::Path const filePath = GetFileSystemPath( resourceID.GetDataPath() );

                ResourceDescriptor* pDescriptor = ResourceDescriptor::TryReadFromFile( *m_pToolsContext->m_pTypeRegistry, m_log, filePath );
                if ( pDescriptor != nullptr )
                {
                    EE_ASSERT( m_loadedResource.IsLoaded() );

                    if ( m_pOperation->PerformOperation( m_pToolsContext, m_loadedResource.GetPtr<IResource>(), *pDescriptor ) )
                    {
                        if ( !ResourceDescriptor::TryWriteToFile( *m_pToolsContext->m_pTypeRegistry, m_log, filePath, pDescriptor ) )
                        {
                            str.sprintf( "Failed to Save: %s\n", filePath.c_str() );
                        }
                    }

                    str.sprintf( "Processed: %s\n", resourceID.c_str() );

                    EE::Delete( pDescriptor );
                }
                else
                {
                    str.sprintf( "Failed to load: %s\n", filePath.c_str() );
                }

                m_logBuffer.Append( str );

                UnloadResource( &m_loadedResource );
                m_loadedResource.Clear();
                m_resourceStage = Stage::ProcessNextResource;
            }
            break;

            case Stage::LoadFailed:
            {
                str.sprintf( "Load Failed: %s\n", m_loadedResource.GetResourceID().c_str() );
                m_logBuffer.Append( str );

                UnloadResource( &m_loadedResource );
                m_loadedResource.Clear();
                m_resourceStage = Stage::ProcessNextResource;
            }
            break;
        }
    }

    void ResourceBulkUpdateEditorTool::OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr )
    {
        EE_ASSERT( m_resourceStage == Stage::WaitForResourceLoad );

        if ( pResourcePtr->IsLoaded() )
        {
            m_resourceStage = Stage::PerformOperation;
        }
        else
        {
            m_resourceStage = Stage::LoadFailed;
        }
    }

    void ResourceBulkUpdateEditorTool::StopOperation()
    {
        if ( m_resourceStage == Stage::WaitForResourceLoad )
        {
            UnloadResource( &m_loadedResource );
            m_loadedResource.Clear();
        }

        EE::Delete( m_pOperation );
        m_resourceToProcessIdx = -1;
        m_resourceStage = Stage::None;
        m_resourcesToProcess.clear();
    }
}