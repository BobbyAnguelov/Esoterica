#include "EditorTool_RawFileInspector_Fbx.h"
#include "EngineTools/RawAssets/RawAssetReader.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderMesh.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderTexture.h"
#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsCollisionMesh.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationClip.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderMaterial.h"
#include "Engine/Render/Mesh/StaticMesh.h"
#include "Engine/Render/Mesh/SkeletalMesh.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Engine/Animation/AnimationClip.h"
#include "Engine/Physics/PhysicsCollisionMesh.h"
#include "Base/Math/MathUtils.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    EE_RAW_FILE_INSPECTOR_FACTORY( InspectorFactoryFbx, "fbx", FbxFileInspectorEditorTool );

    //-------------------------------------------------------------------------

    FbxFileInspectorEditorTool::FbxFileInspectorEditorTool( ToolsContext const* pToolsContext, FileSystem::Path const& rawFilePath )
        : RawFileInspectorEditorTool( pToolsContext, rawFilePath )
        , m_refreshFileInfo( true )
        , m_skeletonPicker( *pToolsContext, Animation::Skeleton::GetStaticResourceTypeID() )
    {}

    void FbxFileInspectorEditorTool::UpdateAndDrawInspectorWindow( UpdateContext const& context, bool isFocused )
    {
        // Load File
        //-------------------------------------------------------------------------

        if ( m_refreshFileInfo )
        {
            m_sceneContext.LoadFile( m_filePath );

            if ( m_sceneContext.IsValid() )
            {
                ReadFileContents();
            }

            m_refreshFileInfo = false;
        }

        // Display File Data
        //-------------------------------------------------------------------------

        if ( !m_sceneContext.IsValid() )
        {
            ImGui::Text( "Invalid FBX File" );
            ImGui::Text( m_sceneContext.GetErrorMessage().c_str() );
        }
        else
        {
            // File Info
            //-------------------------------------------------------------------------

            if ( ImGui::CollapsingHeader( "File Info", ImGuiTreeNodeFlags_DefaultOpen ) )
            {
                ImGui::Text( "Path: %s", m_filePath.c_str() );

                if ( m_sceneContext.IsValid() )
                {
                    ImGui::Text( "Up Axis: %s", Math::ToString( m_sceneContext.GetOriginalUpAxis() ) );
                    ImGui::Text( "Scale: %.2f (1 = M, 0.01 = CM)", m_sceneContext.GetScaleConversionMultiplier() );
                }

                if ( ImGuiX::IconButton( EE_ICON_REFRESH, "Reload File" ) )
                {
                    m_refreshFileInfo = true;
                }

                if ( m_sceneContext.HasWarningOccurred() )
                {
                    {
                        ImGuiX::ScopedFont sf( ImGuiX::Font::Large, Colors::Orange );
                        ImGui::Text( EE_ICON_ALERT_OUTLINE );
                        ImGui::SameLine();
                    }

                    {
                        ImGuiX::ScopedFont sf( ImGuiX::Font::MediumBold, Colors::Orange );
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text( "Warning: %s", m_sceneContext.GetWarningMessage().c_str());
                    }
                }
            }

            // Draw file contents
            //-------------------------------------------------------------------------

            if ( ImGui::CollapsingHeader( "File Contents", ImGuiTreeNodeFlags_DefaultOpen ) )
            {
                ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 8, 8 ) );
                ImGui::PushStyleColor( ImGuiCol_WindowBg, Colors::Red );
                if ( ImGui::BeginTabBar( "FileContents", ImGuiTabBarFlags_NoTabListScrollingButtons ) )
                {
                    if ( !m_meshes.empty() )
                    {
                        if ( ImGui::BeginTabItem( "Static Meshes" ) )
                        {
                            DrawStaticMeshes();
                            ImGui::EndTabItem();
                        }
                    }

                    if ( m_hasSkeletalMeshes )
                    {
                        if ( ImGui::BeginTabItem( "Skeletal Meshes" ) )
                        {
                            DrawSkeletalMeshes();
                            ImGui::EndTabItem();
                        }
                    }

                    bool hasSkeletons = !m_skeletonRoots.empty();
                    if ( hasSkeletons )
                    {
                        if ( ImGui::BeginTabItem( "Skeletons" ) )
                        {
                            DrawSkeletons();
                            ImGui::EndTabItem();
                        }
                    }

                    if ( hasSkeletons && !m_animations.empty() )
                    {
                        if ( ImGui::BeginTabItem( "Animations" ) )
                        {
                            DrawAnimations();
                            ImGui::EndTabItem();
                        }
                    }

                    ImGui::EndTabBar();
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
            }
        }
    }

    void FbxFileInspectorEditorTool::ReadFileContents()
    {
        // Clear info
        //-------------------------------------------------------------------------

        m_meshes.clear();
        m_skeletonRoots.clear();
        m_animations.clear();
        m_hasSkeletalMeshes = false;
        m_refreshFileInfo = false;

        //-------------------------------------------------------------------------
        // Meshes
        //-------------------------------------------------------------------------

        TInlineVector<FbxMesh*, 20> meshes;
        auto numGeometries = m_sceneContext.m_pScene->GetGeometryCount();
        for ( auto i = 0; i < numGeometries; i++ )
        {
            auto pGeometry = m_sceneContext.m_pScene->GetGeometry( i );
            if ( pGeometry->Is<FbxMesh>() )
            {
                FbxMesh* pMesh = reinterpret_cast<FbxMesh*>( pGeometry );

                auto& meshInfo = m_meshes.emplace_back();
                meshInfo.m_nameID = StringID( Fbx::GetNameWithoutNamespace( pMesh->GetNode() ) );

                // Skinned mesh
                meshInfo.m_isSkinned = pMesh->GetDeformerCount( FbxDeformer::eSkin ) > 0;
                m_hasSkeletalMeshes |= meshInfo.m_isSkinned;

                // Get material name
                int32_t const numMaterials = pMesh->GetElementMaterialCount();
                if ( numMaterials == 1 )
                {
                    FbxGeometryElementMaterial* pMaterialElement = pMesh->GetElementMaterial( 0 );
                    FbxSurfaceMaterial* pMaterial = pMesh->GetNode()->GetMaterial( pMaterialElement->GetIndexArray().GetAt( 0 ) );
                    meshInfo.m_materialID = StringID( pMaterial->GetName() );
                }
                else if ( numMaterials > 1 )
                {
                    meshInfo.m_extraInfo = "More than one material detected - This is not supported.";
                }
                else
                {
                    meshInfo.m_extraInfo = "No material assigned.";
                }
            }
        }

        //-------------------------------------------------------------------------
        // Skeletons
        //-------------------------------------------------------------------------

        TVector<FbxNode*> skeletonRootNodes;
        m_sceneContext.FindAllNodesOfType( FbxNodeAttribute::eSkeleton, skeletonRootNodes );

        for ( auto& pSkeletonNode : skeletonRootNodes )
        {
            bool hasNullOrLocatorParent = false;
            auto pParentNode = pSkeletonNode->GetParent();
            if ( pParentNode != nullptr )
            {
                if ( auto pNodeAttribute = pParentNode->GetNodeAttribute() )
                {
                    hasNullOrLocatorParent = pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eNull;
                }
            }

            //-------------------------------------------------------------------------

            StringID const skeletonID = StringID( pSkeletonNode->GetName() );

            if ( hasNullOrLocatorParent )
            {
                StringID const parentRootID( pParentNode->GetName() );

                // Try to find existing null root
                SkeletonRootInfo* pExistingNullRoot = nullptr;
                for ( auto& existingSkeletonRoot : m_skeletonRoots )
                {
                    if ( existingSkeletonRoot.m_nameID == parentRootID )
                    {
                        EE_ASSERT( existingSkeletonRoot.IsNullOrLocatorNode() );
                        pExistingNullRoot = &existingSkeletonRoot;
                        break;
                    }
                }

                // Add root if it doesnt exist
                if ( pExistingNullRoot == nullptr )
                {
                    pExistingNullRoot =  &m_skeletonRoots.emplace_back( SkeletonRootInfo( parentRootID ) );
                }

                pExistingNullRoot->m_childSkeletonRoots.emplace_back( skeletonID );
            }
            else // No parent so just add it
            {
                m_skeletonRoots.emplace_back( SkeletonRootInfo( skeletonID ) );
            }
        }

        //-------------------------------------------------------------------------
        // Animations
        //-------------------------------------------------------------------------

        TVector<FbxAnimStack*> stacks;
        m_sceneContext.FindAllAnimStacks( stacks );
        for ( auto pAnimStack : stacks )
        {
            auto pTakeInfo = m_sceneContext.m_pScene->GetTakeInfo( Fbx::GetNameWithoutNamespace( pAnimStack ) );
            if ( pTakeInfo != nullptr )
            {
                auto& animInfo = m_animations.emplace_back();
                animInfo.m_nameID = StringID( pAnimStack->GetName() );
                animInfo.m_duration = Seconds( (float) pTakeInfo->mLocalTimeSpan.GetDuration().GetSecondDouble() );

                FbxTime const duration = pTakeInfo->mLocalTimeSpan.GetDuration();
                FbxTime::EMode mode = duration.GetGlobalTimeMode();
                animInfo.m_frameRate = (float) duration.GetFrameRate( mode );
            }
        }
    }

    //-------------------------------------------------------------------------

    void FbxFileInspectorEditorTool::DrawStaticMeshes()
    {
        ImGui::Indent();

        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 8, 4 ) );
        if ( ImGui::BeginTable( "StaticMeshes", 4, ImGuiTableFlags_Borders ) )
        {
            ImGui::TableSetupColumn( "##Include", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 20 );
            ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Material", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Info", ImGuiTableColumnFlags_WidthStretch );

            //-------------------------------------------------------------------------

            ImGui::TableNextRow( ImGuiTableRowFlags_Headers );

            for ( int32_t column = 0; column < 4; column++ )
            {
                ImGui::TableSetColumnIndex( column );
                ImGui::PushID( column );

                if ( column == 0 )
                {
                    bool globalMeshSelectionState = AreAllMeshesSelected();
                    if ( ImGuiX::Checkbox( "##checkall", &globalMeshSelectionState ) )
                    {
                        for ( auto& meshInfo : m_meshes )
                        {
                            meshInfo.m_isSelected = globalMeshSelectionState;
                        }
                    }
                }
                else
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( ImGui::TableGetColumnName( column ) );
                }

                ImGui::PopID();
            }

            //-------------------------------------------------------------------------

            for ( auto& meshInfo : m_meshes )
            {
                ImGui::PushID( &meshInfo );

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGuiX::Checkbox( "##Include", &meshInfo.m_isSelected );

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( meshInfo.m_nameID.c_str() );

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( meshInfo.m_materialID.IsValid() ? meshInfo.m_materialID.c_str() : "No Material" );

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( meshInfo.m_extraInfo.c_str() );

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();

        // Import
        //-------------------------------------------------------------------------

        ImGuiX::TextSeparator( "Import Options" );
        ImGui::Indent();
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Setup Materials: " );
            ImGui::SameLine();
            ImGuiX::Checkbox( "##Set materials", &m_setupMaterials );
            ImGui::SameLine();
            ImGuiX::HelpMarker( "Will try to automatically set materials for this mesh descriptor.\n\nFirst it will look for a material with a matching name in the current directory, if that fails we will create an empty material descriptor with a matching name and use that instead." );

            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Merge Meshes By Material: " );
            ImGui::SameLine();
            ImGuiX::Checkbox( "##MergeMeshes", &m_mergesMeshesWithSameMaterial );
            ImGui::SameLine();
            ImGuiX::HelpMarker( "Will merge all sub-meshes/mesh section that share the same material." );

            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Scale: " );
            ImGui::SameLine();
            ImGuiX::InputFloat3( "Scale", m_meshScale, 300 );
        }
        ImGui::Unindent();

        ImGuiX::TextSeparator( "Import" );
        ImGui::Indent();
        {
            if ( ImGuiX::ColoredIconButton( Colors::Green, Colors::White, Colors::White, EE_ICON_FILE_IMPORT, "Create Static Mesh Descriptor", ImVec2( 250, 0 ) ) )
            {
                CreateStaticMeshDescriptor();
            }

            ImGui::SameLine();

            if( ImGuiX::ColoredIconButton( Colors::Green, Colors::White, Colors::White, EE_ICON_FILE_IMPORT, "Create Physics Mesh Descriptor", ImVec2( 250, 0 ) ) )
            {
                CreatePhysicsMeshDescriptor();
            }
        }
        ImGui::Unindent();
    }

    void FbxFileInspectorEditorTool::DrawSkeletalMeshes()
    {
        ImGui::Indent();

        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 8, 4 ) );
        if ( ImGui::BeginTable( "StaticMeshes", 4, ImGuiTableFlags_Borders ) )
        {
            ImGui::TableSetupColumn( "##Include", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 20 );
            ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Material", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Info", ImGuiTableColumnFlags_WidthStretch );

            //-------------------------------------------------------------------------

            ImGui::TableNextRow( ImGuiTableRowFlags_Headers );

            for ( int32_t column = 0; column < 4; column++ )
            {
                ImGui::TableSetColumnIndex( column );
                ImGui::PushID( column );

                if ( column == 0 )
                {
                    bool globalMeshSelectionState = true;
                    for ( auto& meshInfo : m_meshes )
                    {
                        if ( !meshInfo.m_isSelected )
                        {
                            globalMeshSelectionState = false;
                            break;
                        }
                    }

                    if ( ImGuiX::Checkbox( "##checkall", &globalMeshSelectionState ) )
                    {
                        for ( auto& meshInfo : m_meshes )
                        {
                            meshInfo.m_isSelected = globalMeshSelectionState;
                        }
                    }
                }
                else
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text( ImGui::TableGetColumnName( column ) );
                }

                ImGui::PopID();
            }

            //-------------------------------------------------------------------------

            for ( auto& meshInfo : m_meshes )
            {
                if ( !meshInfo.m_isSkinned )
                {
                    continue;
                }

                ImGui::PushID( &meshInfo );

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGuiX::Checkbox( "##Include", &meshInfo.m_isSelected );

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( meshInfo.m_nameID.c_str() );

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( meshInfo.m_materialID.IsValid() ? meshInfo.m_materialID.c_str() : "No Material" );

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( meshInfo.m_extraInfo.c_str() );

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();

        // Import
        //-------------------------------------------------------------------------

        ImGuiX::TextSeparator( "Import Options" );
        ImGui::Indent();
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Setup Materials: " );
            ImGui::SameLine();
            ImGuiX::Checkbox( "##Set materials", &m_setupMaterials );
            ImGui::SameLine();
            ImGuiX::HelpMarker( "Will try to automatically set materials for this mesh descriptor.\n\nFirst it will look for a material with a matching name in the current directory, if that fails we will create an empty material descriptor with a matching name and use that instead." );

            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Merge Meshes By Material: " );
            ImGui::SameLine();
            ImGuiX::Checkbox( "##MergeMeshes", &m_mergesMeshesWithSameMaterial );
            ImGui::SameLine();
            ImGuiX::HelpMarker( "Will merge all sub-meshes/mesh section that share the same material." );
        }
        ImGui::Unindent();

        ImGuiX::TextSeparator( "Import" );
        ImGui::Indent();
        {
            if ( ImGuiX::ColoredIconButton( Colors::Green, Colors::White, Colors::White, EE_ICON_FILE_IMPORT, "Create Mesh Descriptor", ImVec2( 250, 0 ) ) )
            {
                CreateSkeletalMeshDescriptor();
            }
        }
        ImGui::Unindent();
    }

    //-------------------------------------------------------------------------

    void FbxFileInspectorEditorTool::DrawSkeletons()
    {
        ImGui::Indent();

        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 8, 4 ) );
        if ( ImGui::BeginTable( "Skeletons", 3, ImGuiTableFlags_Borders ) )
        {
            ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Type", ImGuiTableColumnFlags_WidthFixed, 100 );
            ImGui::TableSetupColumn( "##Controls", ImGuiTableColumnFlags_WidthFixed, 140 );

            //-------------------------------------------------------------------------

            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            enum class Type { NullOrLocator, Skeleton, ChildSkeleton };

            auto DrawSkeletonRow = [&] ( StringID skeletonID, Type type )
            {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                if ( type == Type::ChildSkeleton )
                {
                    ImGui::Text( EE_ICON_ARROW_RIGHT_BOTTOM );
                    ImGui::SameLine();
                }
                ImGui::Text( skeletonID.c_str() );

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( ( type == Type::NullOrLocator ) ? "Null/Locator" : "Skeleton" );

                ImGui::TableNextColumn();
                if ( ImGuiX::ColoredIconButton( Colors::Green, Colors::White, Colors::White, EE_ICON_FILE_IMPORT, "Create Skeleton", ImVec2( 140, 0 ) ) )
                {
                    CreateSkeletonDescriptor( skeletonID );
                }
            };

            for ( auto& skeletonInfo : m_skeletonRoots )
            {
                ImGui::PushID( &skeletonInfo );
                DrawSkeletonRow( skeletonInfo.m_nameID, skeletonInfo.IsNullOrLocatorNode() ? Type::NullOrLocator : Type::Skeleton );
                ImGui::PopID();

                //-------------------------------------------------------------------------

                for ( auto& ID : skeletonInfo.m_childSkeletonRoots )
                {
                    ImGui::PushID( &ID );
                    DrawSkeletonRow( ID, Type::ChildSkeleton );
                    ImGui::PopID();
                }
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
        ImGui::Unindent();
    }

    void FbxFileInspectorEditorTool::DrawAnimations()
    {
        ImGui::Indent();

        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 8, 4 ) );
        if ( ImGui::BeginTable( "Animations", 4, ImGuiTableFlags_Borders ) )
        {
            ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Duration", ImGuiTableColumnFlags_WidthFixed, 100 );
            ImGui::TableSetupColumn( "FPS", ImGuiTableColumnFlags_WidthFixed, 50 );
            ImGui::TableSetupColumn( "##Controls", ImGuiTableColumnFlags_WidthFixed, 175 );

            //-------------------------------------------------------------------------

            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            for ( auto& animInfo : m_animations )
            {
                ImGui::PushID( &animInfo );

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( animInfo.m_nameID.c_str() );

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "%.2fs", animInfo.m_duration.ToFloat() );

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::Text( "%.2f", animInfo.m_frameRate );

                ImGui::TableNextColumn();
                ImGui::BeginDisabled( !m_skeletonPicker.GetResourceID().IsValid() );
                if ( ImGuiX::ColoredIconButton( Colors::Green, Colors::White, Colors::White, EE_ICON_FILE_IMPORT, "Create Clip Descriptor", ImVec2( 175, 0 ) ) )
                {
                    CreateAnimationDescriptor( animInfo.m_nameID );
                }
                ImGui::EndDisabled();

                ImGui::PopID();
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();

        // Import
        //-------------------------------------------------------------------------

        ImGuiX::TextSeparator( "Import Options" );
        ImGui::Indent();
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text( "Skeleton: " );
            ImGui::SameLine();
            m_skeletonPicker.UpdateAndDraw();
        }
        ImGui::Unindent();
    }

    //-------------------------------------------------------------------------

    void FbxFileInspectorEditorTool::CreateStaticMeshDescriptor()
    {
        Render::StaticMeshResourceDescriptor resourceDesc;

        FileSystem::Path savePath;
        if ( !TryGetDestinationFilenameForDescriptor( &resourceDesc, savePath ) )
        {
            return;
        }

        //-------------------------------------------------------------------------

        resourceDesc.m_meshPath = ResourcePath::FromFileSystemPath( m_pToolsContext->GetRawResourceDirectory(), m_filePath );
        resourceDesc.m_mergeSectionsByMaterial = m_mergesMeshesWithSameMaterial;
        resourceDesc.m_scale = m_meshScale;

        if ( !AreAllMeshesSelected() )
        {
            for ( auto const& meshInfo : m_meshes )
            {
                if ( meshInfo.m_isSelected )
                {
                    resourceDesc.m_meshesToInclude.emplace_back( meshInfo.m_nameID.c_str() );
                }
            }
        }

        //-------------------------------------------------------------------------

        if ( m_setupMaterials )
        {
            SetupMaterialsForMeshDescriptor( savePath.GetParentDirectory(), resourceDesc );
        }

        //-------------------------------------------------------------------------

        TrySaveDescriptorToDisk( &resourceDesc, savePath );
    }

    void FbxFileInspectorEditorTool::CreateSkeletalMeshDescriptor()
    {
        Render::SkeletalMeshResourceDescriptor resourceDesc;

        FileSystem::Path savePath;
        if ( !TryGetDestinationFilenameForDescriptor( &resourceDesc, savePath ) )
        {
            return;
        }

        //-------------------------------------------------------------------------

        resourceDesc.m_meshPath = ResourcePath::FromFileSystemPath( m_pToolsContext->GetRawResourceDirectory(), m_filePath );
        resourceDesc.m_mergeSectionsByMaterial = m_mergesMeshesWithSameMaterial;

        if ( !AreAllMeshesSelected() )
        {
            for ( auto const& meshInfo : m_meshes )
            {
                resourceDesc.m_meshesToInclude.emplace_back( meshInfo.m_nameID.c_str() );
            }
        }

        //-------------------------------------------------------------------------

        if ( m_setupMaterials )
        {
            SetupMaterialsForMeshDescriptor( savePath.GetParentDirectory(), resourceDesc );
        }

        //-------------------------------------------------------------------------

        TrySaveDescriptorToDisk( &resourceDesc, savePath );
    }

    void FbxFileInspectorEditorTool::CreatePhysicsMeshDescriptor()
    {
        Physics::PhysicsCollisionMeshResourceDescriptor resourceDesc;

        FileSystem::Path savePath;
        if ( !TryGetDestinationFilenameForDescriptor( &resourceDesc, savePath ) )
        {
            return;
        }

        resourceDesc.m_sourcePath = ResourcePath::FromFileSystemPath( m_pToolsContext->GetRawResourceDirectory(), m_filePath );
        resourceDesc.m_scale = m_meshScale;

        if ( !AreAllMeshesSelected() )
        {
            for ( auto const& meshInfo : m_meshes )
            {
                resourceDesc.m_meshesToInclude.emplace_back( meshInfo.m_nameID.c_str() );
            }
        }

        //-------------------------------------------------------------------------

        TrySaveDescriptorToDisk( &resourceDesc, savePath );
    }

    void FbxFileInspectorEditorTool::CreateSkeletonDescriptor( StringID skeletonID )
    {
        EE_ASSERT( skeletonID.IsValid() );

        Animation::SkeletonResourceDescriptor resourceDesc;

        FileSystem::Path savePath;
        if ( !TryGetDestinationFilenameForDescriptor( &resourceDesc, savePath ) )
        {
            return;
        }

        resourceDesc.m_skeletonPath = ResourcePath::FromFileSystemPath( m_pToolsContext->GetRawResourceDirectory(), m_filePath );
        resourceDesc.m_skeletonRootBoneName = skeletonID.c_str();

        //-------------------------------------------------------------------------

        TrySaveDescriptorToDisk( &resourceDesc, savePath );
    }

    void FbxFileInspectorEditorTool::CreateAnimationDescriptor( StringID animTakeID )
    {
        EE_ASSERT( animTakeID.IsValid() );
        EE_ASSERT( m_skeletonPicker.GetResourceID().IsValid() );

        Animation::AnimationClipResourceDescriptor resourceDesc;

        FileSystem::Path savePath;
        if ( !TryGetDestinationFilenameForDescriptor( &resourceDesc, savePath ) )
        {
            return;
        }

        resourceDesc.m_animationPath = ResourcePath::FromFileSystemPath( m_pToolsContext->GetRawResourceDirectory(), m_filePath );
        resourceDesc.m_animationName = animTakeID.c_str();
        resourceDesc.m_skeleton = m_skeletonPicker.GetResourceID();

        //-------------------------------------------------------------------------

        TrySaveDescriptorToDisk( &resourceDesc, savePath );
    }

    void FbxFileInspectorEditorTool::SetupMaterialsForMeshDescriptor( FileSystem::Path const& destinationDirectory, Render::MeshResourceDescriptor& meshDescriptor )
    {
        EE_ASSERT( destinationDirectory.IsValid() && destinationDirectory.IsDirectoryPath() );

        auto AddMaterial = [&] ( MeshInfo const& meshInfo )
        {
            FileSystem::Path materialPath;
            if ( meshInfo.m_materialID.IsValid() )
            {
                materialPath = FileSystem::Path( destinationDirectory.ToString() + meshInfo.m_materialID.c_str() + "." + Render::Material::GetStaticResourceTypeID().ToString().c_str() );
            }

            //-------------------------------------------------------------------------

            if ( materialPath.IsValid() )
            {
                if ( !materialPath.Exists() )
                {
                    Render::MaterialResourceDescriptor materialDesc;
                    TrySaveDescriptorToDisk( &materialDesc, materialPath );
                }

                meshDescriptor.m_materials.emplace_back( ResourcePath::FromFileSystemPath( m_pToolsContext->GetRawResourceDirectory(), materialPath ) );
            }
            else
            {
                meshDescriptor.m_materials.emplace_back( ResourcePath() );
            }
        };

        //-------------------------------------------------------------------------

        TInlineVector<StringID, 25> uniqueMaterialIDs;

        for ( auto const& meshInfo : m_meshes )
        {
            if ( !meshInfo.m_isSelected )
            {
                continue;
            }

            if ( m_mergesMeshesWithSameMaterial )
            {
                if ( VectorContains( uniqueMaterialIDs, meshInfo.m_materialID ) )
                {
                    continue;
                }

                uniqueMaterialIDs.emplace_back( meshInfo.m_materialID );
            }

            AddMaterial( meshInfo );
        }
    }
}