#include "ResourceWorkspace.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "System/Serialization/TypeSerialization.h"

//-------------------------------------------------------------------------

namespace EE
{
    EE_DEFINE_GLOBAL_REGISTRY( ResourceWorkspaceFactory );

    //-------------------------------------------------------------------------

    class ResourceDescriptorUndoableAction final : public IUndoableAction
    {
    public:

        ResourceDescriptorUndoableAction( TypeSystem::TypeRegistry const& typeRegistry, GenericResourceWorkspace* pWorkspace )
            : m_typeRegistry( typeRegistry )
            , m_pWorkspace( pWorkspace )
        {
            EE_ASSERT( m_pWorkspace != nullptr );
            EE_ASSERT( m_pWorkspace->m_pDescriptor != nullptr );
        }

        virtual void Undo() override
        {
            Serialization::JsonArchiveReader typeReader;

            typeReader.ReadFromString( m_valueBefore.c_str() );
            auto const& document = typeReader.GetDocument();
            Serialization::ReadNativeType( m_typeRegistry, document, m_pWorkspace->m_pDescriptor );
            m_pWorkspace->SerializeCustomDescriptorData( m_typeRegistry, document );
            m_pWorkspace->m_isDirty = true;
        }

        virtual void Redo() override
        {
            Serialization::JsonArchiveReader typeReader;

            typeReader.ReadFromString( m_valueAfter.c_str() );
            auto const& document = typeReader.GetDocument();
            Serialization::ReadNativeType( m_typeRegistry, document, m_pWorkspace->m_pDescriptor );
            m_pWorkspace->SerializeCustomDescriptorData( m_typeRegistry, document );
            m_pWorkspace->m_isDirty = true;
        }

        void SerializeBeforeState()
        {
            Serialization::JsonArchiveWriter writer;

            auto pWriter = writer.GetWriter();
            pWriter->StartObject();
            Serialization::WriteNativeTypeContents( m_typeRegistry, m_pWorkspace->m_pDescriptor, *pWriter );
            m_pWorkspace->SerializeCustomDescriptorData( m_typeRegistry, *pWriter );
            pWriter->EndObject();

            m_valueBefore.resize( writer.GetStringBuffer().GetSize() );
            memcpy( m_valueBefore.data(), writer.GetStringBuffer().GetString(), writer.GetStringBuffer().GetSize() );
        }

        void SerializeAfterState()
        {
            Serialization::JsonArchiveWriter writer;

            auto pWriter = writer.GetWriter();
            pWriter->StartObject();
            Serialization::WriteNativeTypeContents( m_typeRegistry, m_pWorkspace->m_pDescriptor, *pWriter );
            m_pWorkspace->SerializeCustomDescriptorData( m_typeRegistry, *pWriter );
            pWriter->EndObject();

            m_valueAfter.resize( writer.GetStringBuffer().GetSize() );
            memcpy( m_valueAfter.data(), writer.GetStringBuffer().GetString(), writer.GetStringBuffer().GetSize() );

            m_pWorkspace->m_isDirty = true;
        }

    private:

        TypeSystem::TypeRegistry const&     m_typeRegistry;
        GenericResourceWorkspace*           m_pWorkspace = nullptr;
        String                              m_valueBefore;
        String                              m_valueAfter;
    };

    //-------------------------------------------------------------------------

    GenericResourceWorkspace::GenericResourceWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID )
        : EditorWorkspace( pToolsContext, pWorld, resourceID.GetFileNameWithoutExtension() )
        , m_descriptorID( resourceID )
        , m_descriptorPath( GetFileSystemPath( resourceID ) )
        , m_descriptorPropertyGrid( m_pToolsContext )
    {
        EE_ASSERT( resourceID.IsValid() );
        m_preEditEventBindingID = m_descriptorPropertyGrid.OnPreEdit().Bind( [this] ( PropertyEditInfo const& info ) { PreEdit( info ); } );
        m_postEditEventBindingID = m_descriptorPropertyGrid.OnPostEdit().Bind( [this] ( PropertyEditInfo const& info ) { PostEdit( info ); } );
    }

    GenericResourceWorkspace::~GenericResourceWorkspace()
    {
        EE_ASSERT( m_pDescriptor == nullptr );
        EE_ASSERT( m_pActiveUndoableAction == nullptr );

        m_descriptorPropertyGrid.OnPreEdit().Unbind( m_preEditEventBindingID );
        m_descriptorPropertyGrid.OnPostEdit().Unbind( m_postEditEventBindingID );
    }

    void GenericResourceWorkspace::Initialize( UpdateContext const& context )
    {
        EditorWorkspace::Initialize( context );
        m_descriptorWindowName.sprintf( "Descriptor##%u", GetID() );
        LoadDescriptor();
    }

    void GenericResourceWorkspace::Shutdown( UpdateContext const& context )
    {
        EE::Delete( m_pDescriptor );
        EditorWorkspace::Shutdown( context );
    }

    void GenericResourceWorkspace::BeginHotReload( TVector<Resource::ResourceRequesterID> const& usersToBeReloaded, TVector<ResourceID> const& resourcesToBeReloaded )
    {
        // Destroy descriptor if the resource we are operating on was modified
        if ( VectorContains( resourcesToBeReloaded, m_descriptorID ) )
        {
            EE::Delete( m_pDescriptor );
        }

        EditorWorkspace::BeginHotReload( usersToBeReloaded, resourcesToBeReloaded );
    }

    void GenericResourceWorkspace::EndHotReload()
    {
        EditorWorkspace::EndHotReload();

        // Reload the descriptor if needed
        if ( m_pDescriptor == nullptr )
        {
            LoadDescriptor();
        }
    }

    void GenericResourceWorkspace::LoadDescriptor()
    {
        EE_ASSERT( m_pDescriptor == nullptr );

        Serialization::JsonArchiveReader archive;
        if ( !archive.ReadFromFile( m_descriptorPath ) )
        {
            EE_LOG_ERROR( "Editor", "Failed to read resource descriptor file: %s", m_descriptorPath.c_str() );
            return;
        }

        auto const& document = archive.GetDocument();
        m_pDescriptor = Cast<Resource::ResourceDescriptor>( Serialization::CreateAndReadNativeType( *m_pToolsContext->m_pTypeRegistry, document ) );
        m_descriptorPropertyGrid.SetTypeToEdit( m_pDescriptor );

        SerializeCustomDescriptorData( *m_pToolsContext->m_pTypeRegistry, document );
    }

    void GenericResourceWorkspace::InitializeDockingLayout( ImGuiID dockspaceID ) const
    {
        ImGui::DockBuilderDockWindow( m_descriptorWindowName.c_str(), dockspaceID );
    }

    void GenericResourceWorkspace::UpdateWorkspace( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused )
    {
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_descriptorWindowName.c_str() ) )
        {
            if ( auto pDockNode = ImGui::GetWindowDockNode() )
            {
                pDockNode->LocalFlags |= ImGuiDockNodeFlags_HiddenTabBar;
            }

            //-------------------------------------------------------------------------

            if ( m_pDescriptor == nullptr )
            {
                ImGui::Text( "Failed to load descriptor!" );
            }
            else
            {
                {
                    ImGuiX::ScopedFont sf( ImGuiX::Font::Medium );
                    ImGui::Text( "Descriptor: %s", m_descriptorID.c_str() );
                }

                ImGui::BeginDisabled( !m_descriptorPropertyGrid.IsDirty() );
                if ( ImGuiX::ColoredButton( ImGuiX::ConvertColor( Colors::ForestGreen ), ImGuiX::ConvertColor( Colors::White ), EE_ICON_CONTENT_SAVE" Save", ImVec2( -1, 0 ) ) )
                {
                    Save();
                }
                ImGui::EndDisabled();

                m_descriptorPropertyGrid.DrawGrid();
            }
        }
        ImGui::End();
    }

    void GenericResourceWorkspace::DrawWorkspaceToolbarItems( UpdateContext const& context )
    {
        EditorWorkspace::DrawWorkspaceToolbarItems( context );

        if ( ImGui::MenuItem( EE_ICON_CONTENT_COPY"##Copy Path" ) )
        {
            ImGui::SetClipboardText( m_descriptorID.c_str() );
        }
        ImGuiX::ItemTooltip( "Copy Resource Path" );
    }

    bool GenericResourceWorkspace::DrawDescriptorEditorWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        bool hasFocus = false;
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_descriptorWindowName.c_str() ) )
        {
            if ( m_pDescriptor == nullptr )
            {
                ImGui::Text( "Failed to load descriptor!" );
            }
            else
            {
                m_descriptorPropertyGrid.DrawGrid();
            }

            hasFocus = ImGui::IsWindowFocused( ImGuiFocusedFlags_RootAndChildWindows );
        }
        ImGui::End();

        return hasFocus;
    }

    bool GenericResourceWorkspace::Save()
    {
        EE_ASSERT( m_descriptorID.IsValid() && m_descriptorPath.IsFilePath() );
        EE_ASSERT( m_pDescriptor != nullptr );

        // Serialize descriptor
        //-------------------------------------------------------------------------

        Serialization::JsonArchiveWriter descriptorWriter;
        auto pWriter = descriptorWriter.GetWriter();

        pWriter->StartObject();
        Serialization::WriteNativeTypeContents( *m_pToolsContext->m_pTypeRegistry, m_pDescriptor, *pWriter );
        SerializeCustomDescriptorData( *m_pToolsContext->m_pTypeRegistry, *pWriter );
        pWriter->EndObject();

        // Save to file
        //-------------------------------------------------------------------------

        if ( descriptorWriter.WriteToFile( m_descriptorPath ) )
        {
            m_descriptorPropertyGrid.ClearDirty();
            m_isDirty = false;
            return true;
        }

        return false;
    }

    void GenericResourceWorkspace::PreEdit( PropertyEditInfo const& info )
    {
        EE_ASSERT( m_pActiveUndoableAction == nullptr );
        BeginModification();
    }

    void GenericResourceWorkspace::PostEdit( PropertyEditInfo const& info )
    {
        EE_ASSERT( m_pActiveUndoableAction != nullptr );
        EndModification();
    }

    void GenericResourceWorkspace::BeginModification()
    {
        if ( m_beginModificationCallCount == 0 )
        {
            auto pUndoableAction = EE::New<ResourceDescriptorUndoableAction>( *m_pToolsContext->m_pTypeRegistry, this );
            pUndoableAction->SerializeBeforeState();
            m_pActiveUndoableAction = pUndoableAction;
        }
        m_beginModificationCallCount++;
    }

    void GenericResourceWorkspace::EndModification()
    {
        EE_ASSERT( m_beginModificationCallCount > 0 );
        EE_ASSERT( m_pActiveUndoableAction != nullptr );

        m_beginModificationCallCount--;

        if ( m_beginModificationCallCount == 0 )
        {
            auto pUndoableAction = static_cast<ResourceDescriptorUndoableAction*>( m_pActiveUndoableAction );
            pUndoableAction->SerializeAfterState();
            m_undoStack.RegisterAction( pUndoableAction );
            m_pActiveUndoableAction = nullptr;
            m_isDirty = true;
        }
    }

    //-------------------------------------------------------------------------

    bool ResourceWorkspaceFactory::HasCustomWorkspace( ResourceTypeID const& resourceTypeID )
    {
        auto pCurrentFactory = s_pHead;
        while ( pCurrentFactory != nullptr )
        {
            if ( resourceTypeID == pCurrentFactory->GetSupportedResourceTypeID() )
            {
                return true;
            }

            pCurrentFactory = pCurrentFactory->GetNextItem();
        }

        return false;
    }

    EditorWorkspace* ResourceWorkspaceFactory::TryCreateWorkspace( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID )
    {
        EE_ASSERT( resourceID.IsValid() );
        auto resourceTypeID = resourceID.GetResourceTypeID();
        EE_ASSERT( resourceTypeID.IsValid() );

        // Check if we have a custom workspace for this type
        //-------------------------------------------------------------------------

        auto pCurrentFactory = s_pHead;
        while ( pCurrentFactory != nullptr )
        {
            if ( resourceTypeID == pCurrentFactory->GetSupportedResourceTypeID() )
            {
                return pCurrentFactory->CreateWorkspace( pToolsContext, pWorld, resourceID );
            }

            pCurrentFactory = pCurrentFactory->GetNextItem();
        }

        // Create generic descriptor workspace
        //-------------------------------------------------------------------------

        auto const resourceDescriptorTypes = pToolsContext->m_pTypeRegistry->GetAllDerivedTypes( Resource::ResourceDescriptor::GetStaticTypeID(), false, false );
        for ( auto pResourceDescriptorTypeInfo : resourceDescriptorTypes )
        {
            auto pDefaultInstance = Cast<Resource::ResourceDescriptor>( pResourceDescriptorTypeInfo->GetDefaultInstance() );
            if ( pDefaultInstance->GetCompiledResourceTypeID() == resourceID.GetResourceTypeID() )
            {
                return EE::New<GenericResourceWorkspace>( pToolsContext, pWorld, resourceID );
            }
        }

        return nullptr;
    }
}