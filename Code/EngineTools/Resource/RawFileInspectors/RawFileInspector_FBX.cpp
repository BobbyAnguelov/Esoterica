#include "RawFileInspector_FBX.h"
#include "EngineTools/RawAssets/RawAssetReader.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderMesh.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderTexture.h"
#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsMesh.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationClip.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "Engine/Render/Mesh/StaticMesh.h"
#include "Engine/Render/Mesh/SkeletalMesh.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Engine/Animation/AnimationClip.h"
#include "Engine/Physics/PhysicsMesh.h"
#include "System/Imgui/ImguiX.h"
#include "System/Imgui/ImguiStyle.h"
#include "System/Math/MathStringHelpers.h"
#include "System/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

using namespace fbxsdk;

//-------------------------------------------------------------------------

namespace EE::Resource
{
    EE_RAW_FILE_INSPECTOR_FACTORY( InspectorFactoryFbx, "fbx", RawFileInspectorFBX );

    //-------------------------------------------------------------------------

    RawFileInspectorFBX::RawFileInspectorFBX( ToolsContext const* pToolsContext, FileSystem::Path const& filePath )
        : RawFileInspector( pToolsContext, filePath )
        , m_sceneContext( filePath )
    {
        EE_ASSERT( FileSystem::Exists( filePath ) );

        if ( m_sceneContext.IsValid() )
        {
            ReadFileContents();
        }
    }

    void RawFileInspectorFBX::ReadFileContents()
    {
        FbxGeometryConverter geomConverter( m_sceneContext.m_pManager );
        geomConverter.SplitMeshesPerMaterial( m_sceneContext.m_pScene, true );

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
                StringID t( pMesh->GetNode()->GetNameWithNameSpacePrefix() );
                meshInfo.m_nameID = StringID( pMesh->GetNode()->GetName() );
                meshInfo.m_isSkinned = pMesh->GetDeformerCount( FbxDeformer::eSkin ) > 0;
            }
        }

        //-------------------------------------------------------------------------
        // Skeletons
        //-------------------------------------------------------------------------

        TVector<FbxNode*> skeletonRootNodes;
        m_sceneContext.FindAllNodesOfType( FbxNodeAttribute::eSkeleton, skeletonRootNodes );

        for ( auto& pSkeletonNode : skeletonRootNodes )
        {
            BoneSkeletonRootInfo skeletonRootInfo;
            skeletonRootInfo.m_nameID = StringID( pSkeletonNode->GetName() );

            NullSkeletonRootInfo* pNullSkeletonRoot = nullptr;
            if ( auto pParentNode = pSkeletonNode->GetParent() )
            {
                if ( auto pNodeAttribute = pParentNode->GetNodeAttribute() )
                {
                    if ( pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eNull )
                    {
                        StringID const parentID( pParentNode->GetName() );

                        // Try find existing skeleton with this ID
                        for ( auto& nullSkeletonRoot : m_nullSkeletonRoots )
                        {
                            if ( nullSkeletonRoot.m_nameID == parentID )
                            {
                                pNullSkeletonRoot = &nullSkeletonRoot;
                                break;
                            }
                        }

                        // Create new parent skeleton
                        if ( pNullSkeletonRoot == nullptr )
                        {
                            pNullSkeletonRoot = &m_nullSkeletonRoots.emplace_back();
                            pNullSkeletonRoot->m_nameID = parentID;
                        }
                    }
                }
            }

            // Add to the list of skeletons
            if ( pNullSkeletonRoot != nullptr )
            {
                pNullSkeletonRoot->m_childSkeletons.push_back( skeletonRootInfo );
            }
            else
            {
                m_skeletonRoots.push_back( skeletonRootInfo );
            }
        }

        //-------------------------------------------------------------------------
        // Animations
        //-------------------------------------------------------------------------

        TVector<FbxAnimStack*> stacks;
        m_sceneContext.FindAllAnimStacks( stacks );
        for ( auto pAnimStack : stacks )
        {
            auto pTakeInfo = m_sceneContext.m_pScene->GetTakeInfo( pAnimStack->GetNameWithoutNameSpacePrefix() );
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

    void RawFileInspectorFBX::DrawFileInfo()
    {
        ImGuiX::TextSeparator( "File Info" );

        ImGui::Indent();
        ImGui::Text( "Raw File: %s", m_filePath.c_str() );

        if ( m_sceneContext.IsValid() )
        {
            ImGui::Text( "Original Up Axis: %s", Math::ToString( m_sceneContext.GetOriginalUpAxis() ) );
            ImGui::Text( "Scale: %.2f", m_sceneContext.GetScaleConversionMultiplier() == 1.0f ? "M" : "CM" );
        }
        ImGui::Unindent();
    }

    void RawFileInspectorFBX::DrawFileContents()
    {
        if ( m_sceneContext.IsValid() )
        {
            InlineString tmpString;

            //-------------------------------------------------------------------------
            // Meshes
            //-------------------------------------------------------------------------

            bool haveSkeletalMeshes = false;

            if ( !m_meshes.empty() )
            {
                ImGuiX::TextSeparator( "Static Meshes", 10, ImGui::GetColumnWidth() );

                bool const isCombinedMeshSelected = ( m_selectedItemType == InfoType::StaticMesh ) && !m_selectedItemID.IsValid();
                if ( ImGui::Selectable( EE_ICON_HOME_GROUP" Combined Static Mesh", isCombinedMeshSelected, ImGuiSelectableFlags_DontClosePopups ) )
                {
                    m_selectedItemType = InfoType::StaticMesh;
                    m_selectedItemID = StringID();
                    OnSwitchSelectedItem();
                }

                for ( auto const& meshInfo : m_meshes )
                {
                    tmpString.sprintf( EE_ICON_HOME" %s##StaticMesh", meshInfo.m_nameID.c_str() );
                    bool isSelected = ( m_selectedItemType == InfoType::StaticMesh ) && meshInfo.m_nameID == m_selectedItemID;
                    if ( ImGui::Selectable( tmpString.c_str(), isSelected, ImGuiSelectableFlags_DontClosePopups ) )
                    {
                        m_selectedItemType = InfoType::StaticMesh;
                        m_selectedItemID = meshInfo.m_nameID;
                        OnSwitchSelectedItem();
                    }

                    haveSkeletalMeshes |= meshInfo.m_isSkinned;
                }
            
                ImGuiX::TextSeparator( "Physics Meshes", 10, ImGui::GetColumnWidth() );

                bool const isCombinedPhysicsMeshSelected = ( m_selectedItemType == InfoType::PhysicsMesh ) && !m_selectedItemID.IsValid();
                if ( ImGui::Selectable( EE_ICON_HOME_GROUP" Combined Physics Mesh", isCombinedPhysicsMeshSelected, ImGuiSelectableFlags_DontClosePopups ) )
                {
                    m_selectedItemType = InfoType::PhysicsMesh;
                    m_selectedItemID = StringID();
                    OnSwitchSelectedItem();
                }

                for ( auto const& meshInfo : m_meshes )
                {
                    tmpString.sprintf( EE_ICON_CUBE" %s##PhysicsMesh", meshInfo.m_nameID.c_str() );
                    bool isSelected = ( m_selectedItemType == InfoType::PhysicsMesh ) && meshInfo.m_nameID == m_selectedItemID;
                    if ( ImGui::Selectable( tmpString.c_str(), isSelected, ImGuiSelectableFlags_DontClosePopups ) )
                    {
                        m_selectedItemType = InfoType::PhysicsMesh;
                        m_selectedItemID = meshInfo.m_nameID;
                        OnSwitchSelectedItem();
                    }
                }
            }

            if ( haveSkeletalMeshes )
            {
                ImGuiX::TextSeparator( "Skeletal Meshes", 10, ImGui::GetColumnWidth() );

                bool const isCombinedSkeletalMeshSelected = ( m_selectedItemType == InfoType::SkeletalMesh ) && !m_selectedItemID.IsValid();
                if ( ImGui::Selectable( EE_ICON_ACCOUNT_GROUP" Combined Skeletal Mesh", isCombinedSkeletalMeshSelected, ImGuiSelectableFlags_DontClosePopups ) )
                {
                    m_selectedItemType = InfoType::SkeletalMesh;
                    m_selectedItemID = StringID();
                    OnSwitchSelectedItem();
                }

                for ( auto const& meshInfo : m_meshes )
                {
                    if ( !meshInfo.m_isSkinned )
                    {
                        continue;
                    }

                    tmpString.sprintf( EE_ICON_ACCOUNT" %s", meshInfo.m_nameID.c_str() );
                    bool isSelected = ( m_selectedItemType == InfoType::SkeletalMesh ) && meshInfo.m_nameID == m_selectedItemID;
                    if ( ImGui::Selectable( tmpString.c_str(), isSelected, ImGuiSelectableFlags_DontClosePopups ) )
                    {
                        m_selectedItemType = InfoType::SkeletalMesh;
                        m_selectedItemID = meshInfo.m_nameID;
                        OnSwitchSelectedItem();
                    }
                }
            }

            //-------------------------------------------------------------------------
            // Skeletons
            //-------------------------------------------------------------------------

            if ( !m_nullSkeletonRoots.empty() || !m_skeletonRoots.empty() )
            {
                ImGuiX::TextSeparator( "Skeletons", 10, ImGui::GetColumnWidth() );

                for ( auto const& skeletonRoot : m_nullSkeletonRoots )
                {
                    tmpString.sprintf( EE_ICON_CIRCLE" %s", skeletonRoot.m_nameID.c_str() );

                    bool isSelected = ( m_selectedItemType == InfoType::Skeleton ) && skeletonRoot.m_nameID == m_selectedItemID;
                    if ( ImGui::Selectable( tmpString.c_str(), isSelected, ImGuiSelectableFlags_DontClosePopups ) )
                    {
                        m_selectedItemType = InfoType::Skeleton;
                        m_selectedItemID = skeletonRoot.m_nameID;
                        OnSwitchSelectedItem();
                    }

                    ImGui::Indent();
                    for ( auto const& childSkeletonRoot : skeletonRoot.m_childSkeletons )
                    {
                        tmpString.sprintf( EE_ICON_SKULL" %s", childSkeletonRoot.m_nameID.c_str() );

                        isSelected = ( m_selectedItemType == InfoType::Skeleton ) && childSkeletonRoot.m_nameID == m_selectedItemID;
                        if ( ImGui::Selectable( tmpString.c_str(), isSelected, ImGuiSelectableFlags_DontClosePopups ) )
                        {
                            m_selectedItemType = InfoType::Skeleton;
                            m_selectedItemID = childSkeletonRoot.m_nameID;
                            OnSwitchSelectedItem();
                        }
                    }
                    ImGui::Unindent();
                }

                for ( auto const& skeletonRoot : m_skeletonRoots )
                {
                    tmpString.sprintf( EE_ICON_SKULL" %s", skeletonRoot.m_nameID.c_str() );

                    bool isSelected = ( m_selectedItemType == InfoType::Skeleton ) && skeletonRoot.m_nameID == m_selectedItemID;
                    if ( ImGui::Selectable( tmpString.c_str(), isSelected, ImGuiSelectableFlags_DontClosePopups ) )
                    {
                        m_selectedItemType = InfoType::Skeleton;
                        m_selectedItemID = skeletonRoot.m_nameID;
                        OnSwitchSelectedItem();
                    }
                }
            }

            //-------------------------------------------------------------------------
            // Animations
            //-------------------------------------------------------------------------

            if ( !m_animations.empty() )
            {
                ImGuiX::TextSeparator( "Animations", 10, ImGui::GetColumnWidth() );

                for ( auto const& animationInfo : m_animations )
                {
                    tmpString.sprintf( EE_ICON_FILM" %s", animationInfo.m_nameID.c_str() );

                    bool isSelected = ( m_selectedItemType == InfoType::Animation ) && animationInfo.m_nameID == m_selectedItemID;
                    if( ImGui::Selectable( tmpString.c_str(), isSelected, ImGuiSelectableFlags_DontClosePopups ) )
                    {
                        m_selectedItemType = InfoType::Animation;
                        m_selectedItemID = animationInfo.m_nameID;
                        OnSwitchSelectedItem();
                    }
                }
            }
        }
    }

    void RawFileInspectorFBX::DrawResourceDescriptorCreator()
    {
        if ( m_selectedItemType != InfoType::None )
        {
            switch ( m_selectedItemType )
            {
                case InfoType::StaticMesh:
                {
                    ImGuiX::TextSeparator( "Create New Static Mesh" );
                }
                break;

                case InfoType::SkeletalMesh:
                {
                    ImGuiX::TextSeparator( "Create New Skeletal Mesh" );
                }
                break;

                case InfoType::PhysicsMesh:
                {
                    ImGuiX::TextSeparator( "Create New Physics Mesh" );
                }
                break;

                case InfoType::Skeleton:
                {
                    ImGuiX::TextSeparator( "Create New Skeleton" );
                }
                break;

                case InfoType::Animation:
                {
                    ImGuiX::TextSeparator( "Create New Animation" );
                }
                break;

                default:
                EE_UNREACHABLE_CODE();
                break;
            }

            //-------------------------------------------------------------------------

            m_propertyGrid.DrawGrid();

            //-------------------------------------------------------------------------

            if ( ImGui::Button( "Create New Resource", ImVec2( -1, 0 ) ) )
            {
                switch ( m_selectedItemType )
                {
                    case InfoType::StaticMesh:
                    {
                        CreateNewDescriptor( Render::StaticMesh::GetStaticResourceTypeID(), m_pDescriptor );
                    }
                    break;

                    case InfoType::SkeletalMesh:
                    {
                        CreateNewDescriptor( Render::SkeletalMesh::GetStaticResourceTypeID(), m_pDescriptor );
                    }
                    break;

                    case InfoType::PhysicsMesh:
                    {
                        CreateNewDescriptor( Physics::PhysicsMesh::GetStaticResourceTypeID(), m_pDescriptor );
                    }
                    break;

                    case InfoType::Skeleton:
                    {
                        CreateNewDescriptor( Animation::Skeleton::GetStaticResourceTypeID(), m_pDescriptor );
                    }
                    break;

                    case InfoType::Animation:
                    {
                        CreateNewDescriptor( Animation::AnimationClip::GetStaticResourceTypeID(), m_pDescriptor );
                    }
                    break;

                    default:
                    EE_UNREACHABLE_CODE();
                    break;
                }
            }
        }
    }

    void RawFileInspectorFBX::OnSwitchSelectedItem()
    {
        m_propertyGrid.SetTypeToEdit( nullptr );
        EE::Delete( m_pDescriptor );

        //-------------------------------------------------------------------------

        switch ( m_selectedItemType )
        {
            case InfoType::StaticMesh:
            {
                auto pDesc = EE::New<Render::StaticMeshResourceDescriptor>();
                pDesc->m_meshPath = ResourcePath::FromFileSystemPath( m_rawResourceDirectory, m_filePath );
                pDesc->m_meshName = m_selectedItemID.IsValid() ? m_selectedItemID.c_str() : "";
                m_pDescriptor = pDesc;
            }
            break;

            case InfoType::PhysicsMesh:
            {
                auto pDesc = EE::New<Physics::PhysicsMeshResourceDescriptor>();
                pDesc->m_meshPath = ResourcePath::FromFileSystemPath( m_rawResourceDirectory, m_filePath );
                pDesc->m_meshName = m_selectedItemID.IsValid() ? m_selectedItemID.c_str() : "";
                m_pDescriptor = pDesc;
            }
            break;

            case InfoType::SkeletalMesh:
            {
                auto pDesc = EE::New<Render::SkeletalMeshResourceDescriptor>();
                pDesc->m_meshPath = ResourcePath::FromFileSystemPath( m_rawResourceDirectory, m_filePath );
                pDesc->m_meshName = m_selectedItemID.IsValid() ? m_selectedItemID.c_str() : "";
                m_pDescriptor = pDesc;
            }
            break;

            case InfoType::Skeleton:
            {
                auto pDesc = EE::New<Animation::SkeletonResourceDescriptor>();
                pDesc->m_skeletonPath = ResourcePath::FromFileSystemPath( m_rawResourceDirectory, m_filePath );
                pDesc->m_skeletonRootBoneName = m_selectedItemID.c_str();
                m_pDescriptor = pDesc;
            }
            break;

            case InfoType::Animation:
            {
                auto pDesc = EE::New<Animation::AnimationClipResourceDescriptor>();
                pDesc->m_animationPath = ResourcePath::FromFileSystemPath( m_rawResourceDirectory, m_filePath );
                pDesc->m_animationName = m_selectedItemID.c_str();
                m_pDescriptor = pDesc;
            }
            break;

            default:
            EE_UNREACHABLE_CODE();
            break;
        }

        m_propertyGrid.SetTypeToEdit( m_pDescriptor );
    }
}