#include "EditorTool_RawFileInspector.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/Core/CommonDialogs.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Base/TypeSystem/ResourceInfo.h"
#include "Base/Serialization/TypeSerialization.h"
#include "EngineTools/ThirdParty/pfd/portable-file-dialogs.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    RawFileInspectorEditorTool::RawFileInspectorEditorTool( ToolsContext const* pToolsContext, FileSystem::Path const& rawFilePath )
        : EditorTool( pToolsContext, rawFilePath.GetFilename().c_str() )
        , m_filePath( rawFilePath )
    {}

    void RawFileInspectorEditorTool::Initialize( UpdateContext const& context )
    {
        EditorTool::Initialize( context );

        // If we need a special case inspector with multiple windows, then that tool needs to initialize all of its windows
        if ( IsSingleWindowTool() )
        {
            CreateToolWindow( "Inspector", [this] ( UpdateContext const& context, bool isFocused ) { UpdateAndDrawInspectorWindow( context, isFocused ); } );
        }
    }

    void RawFileInspectorEditorTool::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID leftDockID = 0, rightDockID = 0;
        ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.5f, &leftDockID, &rightDockID );

        // Dock windows
        //-------------------------------------------------------------------------

        ImGui::DockBuilderDockWindow( GetToolWindowName( "Overview" ).c_str(), leftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Request History" ).c_str(), rightDockID );
    }

    bool RawFileInspectorEditorTool::TryGetDestinationFilenameForDescriptor( Resource::ResourceDescriptor const* pDescriptor, FileSystem::Path& outPath )
    {
        EE_ASSERT( pDescriptor != nullptr );
        ResourceTypeID const resourceTypeID = pDescriptor->GetCompiledResourceTypeID();
        EE_ASSERT( resourceTypeID.IsValid() );

        //-------------------------------------------------------------------------

        TInlineString<5> extension = resourceTypeID.ToString();
        extension.make_lower();

        FileSystem::Path newDescriptorPath = m_filePath;
        newDescriptorPath.ReplaceExtension( extension );

        //-------------------------------------------------------------------------

        TypeSystem::ResourceInfo const* pResourceInfo = m_pToolsContext->m_pTypeRegistry->GetResourceInfoForResourceType( resourceTypeID );
        return SaveDialog( resourceTypeID, outPath, newDescriptorPath, pResourceInfo->m_friendlyName );
    }

    bool RawFileInspectorEditorTool::TrySaveDescriptorToDisk( Resource::ResourceDescriptor const* pDescriptor, FileSystem::Path const filePath, bool openAfterSave ) const
    {
        EE_ASSERT( pDescriptor != nullptr );
        EE_ASSERT( filePath.IsValid() );

        ResourceID resourceID = ResourcePath::FromFileSystemPath( m_pToolsContext->GetRawResourceDirectory(), filePath );
        EE_ASSERT( resourceID.GetResourceTypeID() == pDescriptor->GetCompiledResourceTypeID() );

        //-------------------------------------------------------------------------

        Serialization::TypeArchiveWriter typeWriter( *m_pToolsContext->m_pTypeRegistry );
        typeWriter << pDescriptor;
        if ( !typeWriter.WriteToFile( filePath ) )
        {
            pfd::message( "Failed to save descriptor!", "Failed to write descriptor to disk!", pfd::choice::ok, pfd::icon::error ).result();
            return false;
        }

        //-------------------------------------------------------------------------

        if ( openAfterSave )
        {
            m_pToolsContext->TryOpenResource( ResourcePath::FromFileSystemPath( m_pToolsContext->GetRawResourceDirectory(), filePath ) );
        }

        return true;
    }

    //-------------------------------------------------------------------------

    EE_GLOBAL_REGISTRY( RawFileInspectorFactory );

    //-------------------------------------------------------------------------

    bool RawFileInspectorFactory::CanCreateInspector( FileSystem::Path const& filePath )
    {
        EE_ASSERT( filePath.IsValid() );

        auto pCurrentFactory = s_pHead;
        while ( pCurrentFactory != nullptr )
        {
            if ( pCurrentFactory->IsSupportedFile( filePath ) )
            {
                return true;
            }

            pCurrentFactory = pCurrentFactory->GetNextItem();
        }

        return false;
    }

    RawFileInspectorEditorTool* RawFileInspectorFactory::TryCreateInspector( ToolsContext const* pToolsContext, FileSystem::Path const& filePath )
    {
        EE_ASSERT( filePath.IsValid() );

        auto pCurrentFactory = s_pHead;
        while ( pCurrentFactory != nullptr )
        {
            if ( pCurrentFactory->IsSupportedFile( filePath ) )
            {
                return pCurrentFactory->CreateInspector( pToolsContext, filePath );
            }

            pCurrentFactory = pCurrentFactory->GetNextItem();
        }

        return nullptr;
    }
}