#include "ResourceEditor_Mesh.h"
#include "EngineTools/Core/EditorTool.h"
#include "EngineTools/Core/CommonToolTypes.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderMesh.h"
#include "Engine/Render/Components/Component_RenderMesh.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/UpdateContext.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    EE_RESOURCE_EDITOR_FACTORY( MeshEditorFactory, StaticMesh, MeshEditor );
    EE_RESOURCE_EDITOR_FACTORY( SkeletalMeshEditorFactory, SkeletalMesh, MeshEditor );

    //-------------------------------------------------------------------------

    MeshEditor::MeshEditor( ToolsContext const* pToolsContext, ResourceID const& resourceID, EntityWorld* pWorld )
        : DataFileEditor( pToolsContext, resourceID.GetDataPath(), pWorld )
        , m_editedResource( resourceID )
        , m_socketPropertyGrid( pToolsContext )
        , m_decalPropertyGrid( pToolsContext )
        , m_isViewingSkeletalMesh( resourceID.GetResourceTypeID() == SkeletalMesh::GetStaticResourceTypeID() )
    {
        EE_ASSERT( resourceID.IsValid() );

        auto OnPreEdit = [this] ( PropertyEditInfo const& info ) { BeginDataFileModification(); };
        auto OnPostEdit = [this] ( PropertyEditInfo const& info ) { EE_ASSERT( m_pActiveUndoableAction != nullptr ); EndDataFileModification(); };

        m_preEditSocketBindingID = m_socketPropertyGrid.OnPreEdit().Bind( OnPreEdit );
        m_postEditSocketBindingID = m_socketPropertyGrid.OnPostEdit().Bind( OnPostEdit );

        m_preEditDecalBindingID = m_decalPropertyGrid.OnPreEdit().Bind( OnPreEdit );
        m_postEditDecalBindingID = m_decalPropertyGrid.OnPostEdit().Bind( OnPostEdit );

        m_socketGizmo.SetOption( Gizmo::Options::AllowScale, false );
        m_decalGizmo.SetOption( Gizmo::Options::AllowNonUniformScale, true );
    }

    MeshEditor::~MeshEditor()
    {
        m_socketPropertyGrid.OnPreEdit().Unbind( m_preEditSocketBindingID );
        m_socketPropertyGrid.OnPostEdit().Unbind( m_postEditSocketBindingID );

        m_decalPropertyGrid.OnPreEdit().Unbind( m_preEditDecalBindingID );
        m_decalPropertyGrid.OnPostEdit().Unbind( m_postEditDecalBindingID );

        for ( auto pPicker : m_materialPickers )
        {
            EE::Delete( pPicker );
        }
        m_materialPickers.clear();

        EE_ASSERT( m_pSkeletonTreeRoot == nullptr );
    }

    void MeshEditor::SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID leftDockID0 = 0;
        ImGuiID leftDockID1 = 0;
        ImGuiID centerDockID0 = 0;
        ImGuiID centerDockID1 = 0;
        ImGuiID rightDockID0 = 0;
        ImGuiID rightDockID1 = 0;

        ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.25f, &leftDockID0, &centerDockID0 );
        ImGui::DockBuilderSplitNode( centerDockID0, ImGuiDir_Right, 0.33f, &rightDockID0, &centerDockID0 );

        ImGui::DockBuilderSplitNode( leftDockID0, ImGuiDir_Down, 0.33f, &leftDockID1, &leftDockID0 );
        ImGui::DockBuilderSplitNode( centerDockID0, ImGuiDir_Down, 0.33f, &centerDockID1, &centerDockID0 );
        ImGui::DockBuilderSplitNode( rightDockID0, ImGuiDir_Down, 0.33f, &rightDockID1, &rightDockID0 );

        //-------------------------------------------------------------------------

        ImGui::DockBuilderDockWindow( GetToolWindowName( "Skeleton" ).c_str(), leftDockID0 );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Sockets" ).c_str(), leftDockID1 );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Mesh Decals" ).c_str(), leftDockID1 );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Mesh Info" ).c_str(), rightDockID0 );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Submeshes" ).c_str(), centerDockID1 );
        ImGui::DockBuilderDockWindow( GetToolWindowName( s_dataFileWindowName ).c_str(), rightDockID1 );
    }

    void MeshEditor::ExtendViewportToolBar( UpdateContext const& context, Viewport* pViewport )
    {
        if ( !IsResourceLoaded() )
        {
            return;
        }

        auto pMesh = m_editedResource.GetPtr<Mesh>();

        //-------------------------------------------------------------------------

        auto DrawMenu = [pMesh, this] ()
        {
            InlineString str;

            // -1 for default LOD behavior
            if ( ImGui::MenuItem( "Automatic" ) )
            {
                m_pMeshComponent->SetForcedLOD( -1 );
            }

            // Force specific LOD on the component
            for ( int32_t lodIndex = 0; lodIndex < int32_t( pMesh->GetNumLODs() ); lodIndex++ )
            {
                str.sprintf( "LOD %d", lodIndex );
                if ( ImGui::MenuItem( str.c_str() ) )
                {
                    m_pMeshComponent->SetForcedLOD( lodIndex );
                }
            }
        };

        int32_t const forcedLOD = m_pMeshComponent->GetForcedLOD();

        InlineString str;
        if ( m_pMeshComponent->HasForcedLOD() )
        {
            str.sprintf( "LOD %d", m_pMeshComponent->GetForcedLOD() );
        }
        else
        {
            str = "Automatic LOD";
        }

        DrawViewportToolbarComboIcon( "##LOD", str.c_str(), "Level of Detail", DrawMenu );

        //-------------------------------------------------------------------------

        auto PrintDetails = [this, pMesh] ( Color color )
        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold, color );

            float totalMemoryFootprint = 0.0F;
            uint32_t totalNumVertices = 0;
            uint32_t totalNumTriangles = 0;
            uint32_t totalNumClusters = 0;
            uint32_t totalNumBones = 0;
            for ( Geometry const& geo : pMesh->GetGeometry() )
            {
                totalMemoryFootprint += float( geo.GetMemoryFootprint() ) / ( 1024.0F * 1024.0F );
                totalNumVertices += geo.GetNumVertices();
                totalNumTriangles += geo.GetNumTriangles();
                totalNumClusters += geo.GetNumClusters();
            }

            ImGui::Text( "Total memory footprint: %.2fMB", totalMemoryFootprint );
            ImGui::Text( "Num vertices: %i", totalNumVertices );
            ImGui::Text( "Num triangles: %i", totalNumTriangles );
            ImGui::Text( "Num clusters: %i", totalNumClusters );

            ImGui::NewLine();

            // LOD
            //-------------------------------------------------------------------------

            ImGui::Text( "Current LOD: %d", m_LOD );
            ImGui::Text( "Camera distance: %.2f", m_cameraDistance );

            if ( m_isViewingSkeletalMesh )
            {
                ImGui::Text( "Num Bones: %d", m_editedResource.GetPtr<SkeletalMesh>()->GetNumBones() );
            }
        };

        ImGui::NewLine();
        ImGuiX::DrawShadowedText( Colors::Yellow, PrintDetails );
    }

    bool MeshEditor::ExtendViewportToolBar_VisualizationControls( UpdateContext const& context, Viewport* pViewport )
    {
        ImGui::MenuItem( "Show Origin", nullptr, &m_showOrigin );
        ImGui::MenuItem( "Show Bounds", nullptr, &m_showBounds );
        ImGui::MenuItem( "Show Normals", nullptr, &m_showNormals );
        ImGui::MenuItem( "Show Vertices", nullptr, &m_showVertices );

        if ( m_isViewingSkeletalMesh )
        {
            ImGui::MenuItem( "Show Bind Pose", nullptr, &m_showBindPose );
        }

        ImGui::Separator();
        ImGui::MenuItem( "Edit Mesh Decals", nullptr, &m_isEditingMeshDecals );

        return true;
    }

    void MeshEditor::DrawHelpMenu() const
    {
        DrawHelpTextRow( "(Viewport) Reset Camera", "Backspace" );
        DrawHelpTextRow( "(Viewport) Focus Preview Entity", "F" );
    }

    void MeshEditor::DrawViewportUI( UpdateContext const& context, Viewport const* pViewport, bool isFocused )
    {
        DataFileEditor::DrawViewportUI( context, pViewport, isFocused );

        //-------------------------------------------------------------------------

        if ( m_editedResource.HasLoadingFailed() )
        {
            ViewportNotificationWindows::DrawResourceCompilationFailedWindow( m_editedResource.GetResourceCompilationLog() );
        }

        if ( !m_isEditingMeshDecals )
        {
            DrawSocketGizmo( pViewport, isFocused );
        }

        DrawMeshDecalGizmo( pViewport, isFocused );
    }

    //-------------------------------------------------------------------------

    void MeshEditor::ClearAllSelections()
    {
        m_selectedSubmeshes.clear();
        m_selectedBoneIDs.clear();
        m_selectedDecalIndex = InvalidIndex;
    }

    void MeshEditor::Initialize( UpdateContext const& context )
    {
        EE_ASSERT( m_pPreviewEntity == nullptr );

        DataFileEditor::Initialize( context );

        LoadResource( &m_editedResource );

        //-------------------------------------------------------------------------

        CreateToolWindow( "Skeleton", [this] ( UpdateContext const& context, bool isFocused ) { DrawSkeletonWindow( context, isFocused ); }, ImVec2( -1, -1 ), true );
        CreateToolWindow( "Mesh Info", [this] ( UpdateContext const& context, bool isFocused ) { DrawMeshInfoWindow( context, isFocused ); } );
        CreateToolWindow( "Submeshes", [this] ( UpdateContext const& context, bool isFocused ) { DrawSubmeshesWindow( context, isFocused ); } );
        CreateToolWindow( "Sockets", [this] ( UpdateContext const& context, bool isFocused ) { DrawSocketsWindow( context, isFocused ); } );
        CreateToolWindow( "Mesh Decals", [this] ( UpdateContext const& context, bool isFocused ) { DrawMeshDecalsWindow( context, isFocused ); } );
    }

    void MeshEditor::Shutdown( UpdateContext const& context )
    {
        if ( m_editedResource.WasRequested() )
        {
            UnloadResource( &m_editedResource );
        }

        DataFileEditor::Shutdown( context );
    }

    void MeshEditor::OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr )
    {
        if ( pResourcePtr == &m_editedResource )
        {
            ClearAllSelections();
            m_uniqueMaterialNames.clear();
            m_hiddenSubmeshes.clear();

            //-------------------------------------------------------------------------

            if ( IsResourceLoaded() )
            {
                auto pMeshDescriptor = GetDataFile<MeshResourceDescriptor>();
                if ( pMeshDescriptor->FixUpMaterialMappings( m_editedResource.GetPtr<Mesh>() ) )
                {
                    MarkDirty();
                }

                if ( m_isViewingSkeletalMesh )
                {
                    CreateSkeletonTree();
                }

                Mesh const* pMesh = m_editedResource.GetPtr<Mesh>();
                int32_t const numSubmeshes = pMesh->GetNumSubmeshes();
                for ( int16_t submeshIdx = 0; submeshIdx < numSubmeshes; submeshIdx++ )
                {
                    auto const& submesh = pMesh->GetSubmesh( submeshIdx );
                    VectorEmplaceBackUnique( m_uniqueMaterialNames, submesh.m_materialNameID );
                }

                CreatePreviewEntity();
            }
        }
    }

    void MeshEditor::OnResourceUnload( Resource::ResourcePtr* pResourcePtr )
    {
        if ( pResourcePtr == &m_editedResource )
        {
            DestroyPreviewEntity();

            ClearAllSelections();
            DestroySkeletonTree();
        }
    }

    void MeshEditor::OnDataFileLoadCompleted()
    {
        DataFileEditor::OnDataFileLoadCompleted();
        ClearAllSelections();
    }

    void MeshEditor::OnDataFileUnload()
    {
        ClearAllSelections();
        DataFileEditor::OnDataFileUnload();
    }

    void MeshEditor::CreateSkeletonTree()
    {
        EE_ASSERT( m_isViewingSkeletalMesh );
        SkeletalMesh const* pSkelMesh = m_editedResource.GetPtr<SkeletalMesh>();

        //-------------------------------------------------------------------------

        TVector<BoneItem*> boneItems;

        // Create all infos
        int32_t const numBones = pSkelMesh->GetNumBones();
        for ( auto i = 0; i < numBones; i++ )
        {
            auto& pBoneInfo = boneItems.emplace_back( EE::New<BoneItem>() );
            pBoneInfo->m_boneIdx = i;
        }

        // Create hierarchy
        for ( auto i = 1; i < numBones; i++ )
        {
            int32_t const parentBoneIdx = pSkelMesh->GetParentBoneIndex( i );
            EE_ASSERT( parentBoneIdx != InvalidIndex );
            boneItems[parentBoneIdx]->m_children.emplace_back( boneItems[i] );
        }

        // Set root
        m_pSkeletonTreeRoot = boneItems[0];
    }

    void MeshEditor::DestroySkeletonTree()
    {
        if ( m_pSkeletonTreeRoot != nullptr )
        {
            m_pSkeletonTreeRoot->DestroyChildren();
            EE::Delete( m_pSkeletonTreeRoot );
        }
    }

    void MeshEditor::CreatePreviewEntity()
    {
        EE_ASSERT( IsDataFileLoaded() );

        if ( m_isViewingSkeletalMesh )
        {
            auto pMeshComponent = EE::New<Render::SkeletalMeshComponent>( StringID( "Mesh Component" ) );
            pMeshComponent->SetMesh( m_editedResource.GetResourceID() );

            m_pMeshComponent = pMeshComponent;
        }
        else
        {
            auto pMeshComponent = EE::New<Render::StaticMeshComponent>( StringID( "Mesh Component" ) );
            pMeshComponent->SetMesh( m_editedResource.GetResourceID() );

            m_pMeshComponent = pMeshComponent;
        }

        m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
        m_pPreviewEntity->AddComponent( m_pMeshComponent );
        AddEntityToWorld( m_pPreviewEntity );
    }

    void MeshEditor::DestroyPreviewEntity()
    {
        if ( m_pPreviewEntity != nullptr )
        {
            DestroyEntityInWorld( m_pPreviewEntity );
            m_pMeshComponent = nullptr;
        }
    }

    //-------------------------------------------------------------------------

    void MeshEditor::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        DataFileEditor::Update( context, isVisible, isFocused );

        //-------------------------------------------------------------------------

        m_commandStack.ExecuteCommands();

        //-------------------------------------------------------------------------

        if ( m_isViewportFocused && m_pPreviewEntity != nullptr )
        {
            if ( ImGui::IsKeyPressed( ImGuiKey_Backspace ) )
            {
                ResetCameraView();
            }

            if ( ImGui::IsKeyPressed( ImGuiKey_F ) )
            {
                FocusCameraView( m_pPreviewEntity );
            }
        }

        //-------------------------------------------------------------------------

        if ( IsResourceLoaded() )
        {
            auto drawingContext = GetDebugDrawContext();

            Mesh const* pMesh = m_editedResource.GetPtr<Mesh>();

            // LOD
            //-------------------------------------------------------------------------
            // This calculation is a guess and should be adjusted to match "InstanceCulling.esf" in the future

            m_cameraDistance = ( GetCameraTransform().GetTranslation() - m_pMeshComponent->GetWorldTransform().GetTranslation() ).GetLength3();

            bool const isUsingForcedLOD = m_pMeshComponent->HasForcedLOD();
            m_LOD = isUsingForcedLOD ? m_pMeshComponent->GetForcedLOD() : 0;

            if ( !isUsingForcedLOD && pMesh->GetNumLODs() > 1 )
            {
                for ( int32_t lod = 0; lod < (int32_t) pMesh->GetNumLODs(); ++lod )
                {
                    m_LOD = lod;

                    if ( m_cameraDistance < pMesh->GetLODDistances()[lod] )
                    {
                        break;
                    }
                }
            }

            // Debug
            //-------------------------------------------------------------------------

            if ( m_showOrigin )
            {
                drawingContext.DrawAxis( Transform::Identity, 0.15f, 1.0f );
            }

            // Shared Mesh
            //-------------------------------------------------------------------------

            if ( m_showBounds )
            {
                drawingContext.DrawWireBox( pMesh->GetBounds(), Colors::Cyan );
            }

            //-------------------------------------------------------------------------

            if ( ( m_showVertices || m_showNormals ) )
            {
                for ( Geometry const& geo : pMesh->GetGeometry() )
                {
                    geo.IterateAllTriangles( [&geo, this, &drawingContext] ( uint32_t clusterIndex, uint32_t vertexIndex0, uint32_t vertexIndex1, uint32_t vertexIndex2 )
                    {
                        // Fetch compressed vertices
                        MeshCluster::VertexNormalAttribute vertex0 = geo.GetVertexNormal( clusterIndex, vertexIndex0 );
                        MeshCluster::VertexNormalAttribute vertex1 = geo.GetVertexNormal( clusterIndex, vertexIndex1 );
                        MeshCluster::VertexNormalAttribute vertex2 = geo.GetVertexNormal( clusterIndex, vertexIndex2 );

                        // Decompress positions and normals
                        Float3 vertexPosition0 = geo.GetVertexPosition( clusterIndex, vertexIndex0 );
                        Float3 vertexNormal0 = vertex0.GetNormal();

                        Float3 vertexPosition1 = geo.GetVertexPosition( clusterIndex, vertexIndex1 );
                        Float3 vertexNormal1 = vertex1.GetNormal();

                        Float3 vertexPosition2 = geo.GetVertexPosition( clusterIndex, vertexIndex2 );
                        Float3 vertexNormal2 = vertex2.GetNormal();

                        if ( m_showVertices )
                        {
                            drawingContext.DrawPoint( vertexPosition0, Colors::Cyan, 1.0f );
                            drawingContext.DrawPoint( vertexPosition1, Colors::Cyan, 1.0f );
                            drawingContext.DrawPoint( vertexPosition2, Colors::Cyan, 1.0f );
                        }

                        if ( m_showNormals )
                        {
                            drawingContext.DrawLine( vertexPosition0, vertexPosition0 + ( vertexNormal0 * 0.15f ), Colors::Yellow, 1.0f );
                            drawingContext.DrawLine( vertexPosition1, vertexPosition1 + ( vertexNormal1 * 0.15f ), Colors::Yellow, 1.0f );
                            drawingContext.DrawLine( vertexPosition2, vertexPosition2 + ( vertexNormal2 * 0.15f ), Colors::Yellow, 1.0f );
                        }
                    } );
                }
            }

            // Submesh Visibility
            //-------------------------------------------------------------------------

            if ( m_pMeshComponent != nullptr && m_pMeshComponent->IsMeshLoaded() )
            {
                m_pMeshComponent->SetSubmeshVisibility( m_hiddenSubmeshes );
            }

            // Skeletal Mesh
            //-------------------------------------------------------------------------

            SkeletalMesh const* pSkelMesh = m_isViewingSkeletalMesh ? m_editedResource.GetPtr<SkeletalMesh>() : nullptr;

            if ( m_isViewingSkeletalMesh )
            {
                if ( m_showBindPose )
                {
                    pSkelMesh->DrawBindPose( drawingContext, Transform::Identity );

                    if ( m_pMeshComponent != nullptr && m_pMeshComponent->IsInitialized() )
                    {
                        Cast<SkeletalMeshComponent>( m_pMeshComponent )->DrawPose( drawingContext );
                    }
                }

                //-------------------------------------------------------------------------

                for ( StringID selectedBoneID : m_selectedBoneIDs )
                {
                    int32_t const selectedBoneIdx = pSkelMesh->GetBoneIndex( selectedBoneID );
                    if ( selectedBoneIdx != InvalidIndex )
                    {
                        Transform const& boneTransform = pSkelMesh->GetModelSpaceBindPoseTransform( selectedBoneIdx );
                        drawingContext.DrawAxis( boneTransform, 0.02f, 2.0f );

                        Vector textLocation = boneTransform.GetTranslation();
                        Vector const textLineLocation = textLocation - Vector( 0, 0, 0.01f );
                        drawingContext.DrawText3D( textLocation, selectedBoneID.c_str(), Colors::Yellow );

                        int32_t const parentBoneIdx = pSkelMesh->GetParentBoneIndex( selectedBoneIdx );
                        if ( parentBoneIdx != InvalidIndex )
                        {
                            Transform const& parentBoneTransform = pSkelMesh->GetModelSpaceBindPoseTransform( parentBoneIdx );

                            drawingContext.DrawLine( boneTransform.GetTranslation(), parentBoneTransform.GetTranslation(), Colors::Yellow.GetAlphaVersion( 0.75f ), 1.0f );
                        }
                    }
                }
            }

            // Sockets
            //-------------------------------------------------------------------------

            if ( m_selectedSocketIdx != InvalidIndex )
            {
                MeshResourceDescriptor* pMeshDescriptor = GetDataFile<MeshResourceDescriptor>();
                MeshSocketDefinition const& socketDef = pMeshDescriptor->m_sockets[m_selectedSocketIdx];

                Transform const socketTransform = GetSelectedSocketTransform();

                if ( socketDef.m_ID.IsValid() )
                {
                    drawingContext.DrawText3D( socketTransform.GetTranslation(), socketDef.m_ID.c_str(), Colors::Cyan );
                }

                drawingContext.DrawAxis( socketTransform, 0.1f, 2.0f );
            }

            // Mesh Decals
            //-------------------------------------------------------------------------

            static Color const s_meshDecalColor = Color( 66, 133, 244 );
            static Color const s_meshDecalSelectedColor = Color( 255, 215, 0 );
            static float const s_meshDecalLineThickness = 4.0F;

            if ( IsDataFileLoaded() && ( m_isEditingMeshDecals || m_selectedDecalIndex != InvalidIndex ) )
            {
                MeshResourceDescriptor const* pMeshDescriptor = GetDataFile<MeshResourceDescriptor>();
                Transform const meshWorldTransform = ( m_pMeshComponent != nullptr ) ? m_pMeshComponent->GetWorldTransform() : Transform::Identity;

                int32_t const numDecals = int32_t( pMeshDescriptor->m_meshDecals.size() );
                for ( int32_t decalIdx = 0; decalIdx < numDecals; decalIdx++ )
                {
                    bool const isSelected = decalIdx == m_selectedDecalIndex;
                    if ( !m_isEditingMeshDecals && !isSelected )
                    {
                        continue;
                    }

                    MeshDecalDefinition const& decal = pMeshDescriptor->m_meshDecals[decalIdx];

                    Transform const decalWorldTransform = decal.m_localTransform * meshWorldTransform;

                    Float3 const halfExtents = ( Float3( 0.5F ) * decal.m_nonUniformScale ) * decalWorldTransform.GetScale();
                    OBB const decalBounds( decalWorldTransform.GetTranslationAsPoint(), Vector( halfExtents, 0.0F ), decalWorldTransform.GetRotation() );
                    drawingContext.DrawWireBox( decalBounds, isSelected ? s_meshDecalSelectedColor : s_meshDecalColor, s_meshDecalLineThickness, DebugDrawLayer::World );

                    if ( isSelected )
                    {
                        Vector const axisStart = decalWorldTransform.GetTranslationAsPoint();
                        Vector const axisEnd = axisStart + ( decalWorldTransform.GetAxisZ() * ( halfExtents.m_z + 0.1F ) );
                        drawingContext.DrawLine( axisStart, axisEnd, Colors::Cyan, 2.0F, DebugDrawLayer::World );
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    void MeshEditor::AddSocket()
    {
        EE_ASSERT( IsDataFileLoaded() );
        ScopedDataFileModification sm( this );
        auto pMeshDescriptor = GetDataFile<MeshResourceDescriptor>();
        m_selectedSocketIdx = (int32_t) pMeshDescriptor->m_sockets.size();
        pMeshDescriptor->m_sockets.emplace_back();
    }

    void MeshEditor::RemoveSocket( int32_t socketIdx )
    {
        EE_ASSERT( IsDataFileLoaded() );
        ScopedDataFileModification sm( this );

        auto pMeshDescriptor = GetDataFile<MeshResourceDescriptor>();
        EE_ASSERT( socketIdx >= 0 && socketIdx < pMeshDescriptor->m_sockets.size() );
        pMeshDescriptor->m_sockets.erase( pMeshDescriptor->m_sockets.begin() + socketIdx );

        m_selectedSocketIdx = InvalidIndex;
    }

    Transform MeshEditor::GetSelectedSocketTransform() const
    {
        EE_ASSERT( IsDataFileLoaded() && IsResourceLoaded() );

        if ( m_selectedSocketIdx == InvalidIndex )
        {
            return Transform::Identity;
        }

        MeshResourceDescriptor const* pMeshDescriptor = GetDataFile<MeshResourceDescriptor>();
        MeshSocketDefinition const& socketDef = pMeshDescriptor->m_sockets[m_selectedSocketIdx];

        if ( m_isViewingSkeletalMesh && socketDef.m_boneID.IsValid() )
        {
            SkeletalMesh const* pSkelMesh = m_editedResource.GetPtr<SkeletalMesh>();
            int32_t const boneIdx = pSkelMesh->GetBoneIndex( socketDef.m_boneID );
            if ( boneIdx == InvalidIndex )
            {
                return Transform::Identity;
            }

            return socketDef.m_offsetTransform * pSkelMesh->GetModelSpaceBindPoseTransform( boneIdx );
        }

        return socketDef.m_offsetTransform;
    }

    void MeshEditor::SetSelectedSocketTransform( Transform const& transform )
    {
        EE_ASSERT( IsDataFileLoaded() && IsResourceLoaded() );

        if ( m_selectedSocketIdx == InvalidIndex )
        {
            return;
        }

        MeshResourceDescriptor* pMeshDescriptor = GetDataFile<MeshResourceDescriptor>();
        MeshSocketDefinition& socketDef = pMeshDescriptor->m_sockets[m_selectedSocketIdx];

        if ( m_isViewingSkeletalMesh && socketDef.m_boneID.IsValid() )
        {
            SkeletalMesh const* pSkelMesh = m_editedResource.GetPtr<SkeletalMesh>();
            int32_t const boneIdx = pSkelMesh->GetBoneIndex( socketDef.m_boneID );
            if ( boneIdx == InvalidIndex )
            {
                return;
            }

            ScopedDataFileModification sm( this );
            socketDef.m_offsetTransform = transform * pSkelMesh->GetModelSpaceBindPoseTransform( boneIdx ).GetInverse();
        }
        else
        {
            ScopedDataFileModification sm( this );
            socketDef.m_offsetTransform = transform;
        }
    }

    //-------------------------------------------------------------------------

    void MeshEditor::DrawMeshInfoWindow( UpdateContext const& context, bool isFocused )
    {
        if ( IsWaitingForResource() )
        {
            ImGui::Text( "Loading:" );
            ImGui::SameLine();
            ImGuiX::DrawSpinner( "Loading" );
            return;
        }

        if ( HasLoadingFailed() )
        {
            ImGui::Text( "Loading Failed: %s", m_editedResource.GetResourceID().c_str() );
            return;
        }

        auto pMesh = m_editedResource.GetPtr<Mesh>();
        EE_ASSERT( pMesh != nullptr );

        TArrayView<float const> lodDistance = pMesh->GetLODDistances();
        uint32_t const numLODs = uint32_t( lodDistance.size() );

        float const labelColumnWidth = ImGui::CalcTextSize( "Vtx compression avg accuracy" ).x + ImGui::GetStyle().CellPadding.x * 2.0F;
        float const dataColumnWidth = ImGui::CalcTextSize( "999.999%" ).x;

        if ( ImGui::BeginTable( "LOD", int32_t( numLODs ) + 1, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, ImGui::GetContentRegionAvail() ) )
        {
            ImGui::TableSetupColumn( "Statistic", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_WidthFixed, labelColumnWidth );

            for ( uint32_t lodIndex = 0; lodIndex < numLODs; ++lodIndex )
            {
                InlineString columnName( InlineString::CtorSprintf(), "LOD %d", lodIndex );
                ImGui::TableSetupColumn( columnName.c_str(), ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_WidthFixed, dataColumnWidth );
            }

            ImGui::TableSetupScrollFreeze( 1, 1 );
            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::TextUnformatted( "Distance" );

            for ( uint32_t lodIndex = 0; lodIndex < numLODs; ++lodIndex )
            {
                ImGui::TableNextColumn();
                ImGui::Text( "%.2f", lodDistance[lodIndex] );
            }

            //-------------------------------------------------------------------------

            auto const DrawStatisticRow = [&] ( char const* label, char const* format, auto&& getValue )
            {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::TextUnformatted( label );

                for ( uint32_t lodIndex = 0; lodIndex < numLODs; ++lodIndex )
                {
                    ImGui::TableNextColumn();
                    ImGui::Text( format, getValue( pMesh->GetStatistics( lodIndex ) ) );
                }
            };

            DrawStatisticRow( "Clusters", "%u", [] ( Mesh::Statistics const& stats ) { return stats.m_numClusters; } );

            DrawStatisticRow( "Cluster utilization Min", "%.2f%%", [] ( Mesh::Statistics const& stats ) { return stats.m_minimumClusterUtilization * 100.0F; } );
            DrawStatisticRow( "Cluster utilization Max", "%.2f%%", [] ( Mesh::Statistics const& stats ) { return stats.m_maximumClusterUtilization * 100.0F; } );
            DrawStatisticRow( "Cluster utilization Avg", "%.2f%%", [] ( Mesh::Statistics const& stats ) { return stats.m_averageClusterUtilization * 100.0F; } );
            DrawStatisticRow( "Cluster utilization Median", "%.2f%%", [] ( Mesh::Statistics const& stats ) { return stats.m_medianClusterUtilization * 100.0F; } );
            DrawStatisticRow( "Cluster Vtx overhead", "%.2f%%", [] ( Mesh::Statistics const& stats ) { return stats.m_clusterVertexOverhead * 100.0F; } );
            DrawStatisticRow( "Vtx triangle reuse", "%.2f%%", [] ( Mesh::Statistics const& stats ) { return stats.m_vertexTriangleReuse * 100.0F; } );
            DrawStatisticRow( "Vtx compression min accuracy", "%.2f%%", [] ( Mesh::Statistics const& stats ) { return stats.m_minimumCompressionAccuracy * 100.0F; } );
            DrawStatisticRow( "Vtx compression avg accuracy", "%.2f%%", [] ( Mesh::Statistics const& stats ) { return stats.m_averageCompressionAccuracy * 100.0F; } );
            DrawStatisticRow( "Compression ratio", "%.2f%%", [] ( Mesh::Statistics const& stats ) { return stats.m_compressionRatio * 100.0F; } );

            DrawStatisticRow( "Mesh decals", "%u", [] ( Mesh::Statistics const& stats ) { return stats.m_numMeshDecals; } );
            DrawStatisticRow( "Decal projection bases", "%u", [] ( Mesh::Statistics const& stats ) { return stats.m_numMeshDecalProjectionBases; } );
            DrawStatisticRow( "Decal projections", "%u", [] ( Mesh::Statistics const& stats ) { return stats.m_numMeshDecalProjections; } );
            DrawStatisticRow( "Decal memory (KB)", "%.2f", [] ( Mesh::Statistics const& stats ) { return float( stats.m_meshDecalSizeBytes ) / 1024.0F; } );

            DrawStatisticRow( "Position bits X min", "%u", [] ( Mesh::Statistics const& stats ) { return stats.m_minimumPositionBitsPerAxisX; } );
            DrawStatisticRow( "Position bits X max", "%u", [] ( Mesh::Statistics const& stats ) { return stats.m_maximumPositionBitsPerAxisX; } );
            DrawStatisticRow( "Position bits X avg", "%.1f", [] ( Mesh::Statistics const& stats ) { return stats.m_averagePositionBitsPerAxisX; } );
            DrawStatisticRow( "Position bits X median", "%.1f", [] ( Mesh::Statistics const& stats ) { return stats.m_medianPositionBitsPerAxisX; } );

            DrawStatisticRow( "Position bits Y min", "%u", [] ( Mesh::Statistics const& stats ) { return stats.m_minimumPositionBitsPerAxisY; } );
            DrawStatisticRow( "Position bits Y max", "%u", [] ( Mesh::Statistics const& stats ) { return stats.m_maximumPositionBitsPerAxisY; } );
            DrawStatisticRow( "Position bits Y avg", "%.1f", [] ( Mesh::Statistics const& stats ) { return stats.m_averagePositionBitsPerAxisY; } );
            DrawStatisticRow( "Position bits Y median", "%.1f", [] ( Mesh::Statistics const& stats ) { return stats.m_medianPositionBitsPerAxisY; } );

            DrawStatisticRow( "Position bits Z min", "%u", [] ( Mesh::Statistics const& stats ) { return stats.m_minimumPositionBitsPerAxisZ; } );
            DrawStatisticRow( "Position bits Z max", "%u", [] ( Mesh::Statistics const& stats ) { return stats.m_maximumPositionBitsPerAxisZ; } );
            DrawStatisticRow( "Position bits Z avg", "%.1f", [] ( Mesh::Statistics const& stats ) { return stats.m_averagePositionBitsPerAxisZ; } );
            DrawStatisticRow( "Position bits Z median", "%.1f", [] ( Mesh::Statistics const& stats ) { return stats.m_medianPositionBitsPerAxisZ; } );

            ImGui::EndTable();
        }
    }

    void MeshEditor::DrawSubmeshesWindow( UpdateContext const& context, bool isFocused )
    {
        if ( !IsDataFileLoaded() )
        {
            return;
        }

        if ( !IsResourceLoaded() )
        {
            ImGui::Text( "Mesh Not Loaded!" );
            return;
        }

        MeshResourceDescriptor* pMeshDescriptor = GetDataFile<MeshResourceDescriptor>();
        Mesh const* pMesh = m_editedResource.GetPtr<Mesh>();
        int32_t const numSubmeshes = pMesh->GetNumSubmeshes();

        //-------------------------------------------------------------------------

        while ( m_materialPickers.size() < numSubmeshes )
        {
            m_materialPickers.emplace_back( EE::New<ResourcePicker>( *m_pToolsContext, Material::GetStaticResourceTypeID() ) );
        }

        if ( pMeshDescriptor->FixUpMaterialMappings( pMesh ) )
        {
            MarkDirty();
        }

        int32_t const numMappings = int32_t( pMeshDescriptor->m_materialMappings.size() );

        for ( int32_t i = 0; i < numSubmeshes; i++ )
        {
            m_materialPickers[i]->SetCompactMode( true );

            // Submesh with no mappings is shown as unset
            ResourceID const materialID = ( i < numMappings ) ? pMeshDescriptor->m_materialMappings[i].m_material.GetResourceID() : ResourceID();
            m_materialPickers[i]->SetResourceID( materialID );
        }

        //-------------------------------------------------------------------------

        auto ApplySelectionRequests = [this, numSubmeshes] ( ImGuiMultiSelectIO* pMSIO )
        {
            for ( ImGuiSelectionRequest const& req : pMSIO->Requests )
            {
                if ( req.Type == ImGuiSelectionRequestType_SetAll )
                {
                    m_selectedSubmeshes.clear();

                    if ( req.Selected )
                    {
                        for ( int16_t i = 0; i < numSubmeshes; i++ )
                        {
                            m_selectedSubmeshes.emplace_back( i );
                        }
                    }
                }
                else if ( req.Type == ImGuiSelectionRequestType_SetRange )
                {
                    if ( req.Selected )
                    {
                        for ( int64_t i = req.RangeFirstItem; i <= req.RangeLastItem; i++ )
                        {
                            VectorEmplaceBackUnique( m_selectedSubmeshes, int16_t( i ) );
                        }
                    }
                    else
                    {
                        for ( int64_t i = req.RangeFirstItem; i <= req.RangeLastItem; i++ )
                        {
                            m_selectedSubmeshes.erase_first_unsorted( int16_t( i ) );
                        }
                    }
                }
            }
        };

        constexpr float const lodItemSpacingX = 1;
        float const pickerHeight = ( ImGui::GetFrameHeight() * 2 ) + ImGui::GetStyle().ItemSpacing.y;
        float lodWidth = 0;
        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::Medium );
            lodWidth = ( ImGui::CalcTextSize( EE_ICON_NUMERIC_3_CIRCLE ).x + lodItemSpacingX ) * 8;
        }

        ImGui::PushStyleColor( ImGuiCol_Header, Colors::Transparent );
        ImGui::PushStyleColor( ImGuiCol_HeaderHovered, Colors::Transparent );
        ImGui::PushStyleColor( ImGuiCol_HeaderActive, Colors::Transparent );

        static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable;
        if ( ImGui::BeginTable( "SubMeshTable", 5, flags, ImGui::GetContentRegionAvail() - ImVec2( 0, pickerHeight ) ) )
        {
            ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoSort, 40.0f );
            ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "LOD", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoSort, lodWidth );
            ImGui::TableSetupColumn( "Material Name", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Material", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoSort );

            ImGui::TableSetupScrollFreeze( 0, 1 );

            //-------------------------------------------------------------------------

            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            ImGuiMultiSelectFlags const selectionFlags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d | ImGuiMultiSelectFlags_ClearOnClickVoid;
            ImGuiMultiSelectIO* pMSIO = ImGui::BeginMultiSelect( selectionFlags, -1, numSubmeshes );
            ApplySelectionRequests( pMSIO );

            bool toggleVisibilityForAllSelected = false;
            bool selectedVisible = false;

            for ( int16_t submeshIdx = 0; submeshIdx < numSubmeshes; submeshIdx++ )
            {
                ImGui::PushID( submeshIdx );

                auto const& submesh = pMesh->GetSubmesh( submeshIdx );
                auto& materialMapping = pMeshDescriptor->m_materialMappings[submeshIdx];

                bool isSelected = VectorContains( m_selectedSubmeshes, submeshIdx );

                //-------------------------------------------------------------------------

                ImGui::TableNextRow( ImGuiTableRowFlags_None, ImGui::GetFrameHeightWithSpacing() );

                if ( isSelected )
                {
                    ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg0, ImGuiX::Style::s_colorGray1 );
                }

                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();

                ImGuiX::ToggleButtonSettings settings;
                settings.m_pOnIcon = EE_ICON_EYE_OUTLINE;
                settings.m_pOffIcon = EE_ICON_EYE_OFF_OUTLINE;
                settings.m_pOnLabel = "";
                settings.m_pOffLabel = "";
                settings.m_onBackgroundColor = Colors::MediumSeaGreen;
                settings.m_offBackgroundColor = Colors::PaleVioletRed;
                settings.m_onForegroundColor = Colors::Black;
                settings.m_offForegroundColor = Colors::Black;
                settings.m_onIconColor = Colors::White;
                settings.m_offIconColor = Colors::White;
                settings.m_isFlatButton = false;

                bool isVisible = !VectorContains( m_hiddenSubmeshes, submeshIdx );
                if ( ImGuiX::ToggleButtonEx( settings, isVisible ) )
                {
                    if ( m_selectedSubmeshes.size() > 1 )
                    {
                        toggleVisibilityForAllSelected = true;
                        selectedVisible = isVisible;
                    }
                    else
                    {
                        if ( isVisible )
                        {
                            m_hiddenSubmeshes.erase_first_unsorted( submeshIdx );
                        }
                        else
                        {
                            VectorEmplaceBackUnique( m_hiddenSubmeshes, submeshIdx );
                        }
                    }
                }

                // Name
                //-------------------------------------------------------------------------

                InlineString name( InlineString::CtorSprintf(), "%d. %s", submeshIdx, submesh.m_ID.c_str() );

                ImGui::TableNextColumn();

                {
                    ImGuiX::ScopedFont const sf( isSelected ? ImGuiX::Style::s_colorAccent0 : ImGuiX::Style::s_colorText );
                    ImGui::SetNextItemSelectionUserData( submeshIdx );
                    ImGui::Selectable( name.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap );
                    if ( ImGui::IsItemHovered() && !isSelected )
                    {
                        ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg0, ImGuiX::Style::s_colorGray0 );
                    }
                }

                // LOD
                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();

                bool isRelevantForLOD = false;

                ImGui::AlignTextToFramePadding();

                {
                    ImGuiX::ScopedFont const sf( ImGuiX::Font::Medium );
                    ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( lodItemSpacingX, 0 ) );

                    isRelevantForLOD = ( submesh.m_lodMask & ( 1 << 0 ) ) != 0;
                    ImGui::TextColored( isRelevantForLOD ? Colors::Lime : ImGuiX::Style::s_colorText, EE_ICON_NUMERIC_0_CIRCLE );
                    ImGui::SameLine();

                    isRelevantForLOD = ( submesh.m_lodMask & ( 1 << 1 ) ) != 0;
                    ImGui::TextColored( isRelevantForLOD ? Colors::Lime : ImGuiX::Style::s_colorText, EE_ICON_NUMERIC_1_CIRCLE );
                    ImGui::SameLine();

                    isRelevantForLOD = ( submesh.m_lodMask & ( 1 << 2 ) ) != 0;
                    ImGui::TextColored( isRelevantForLOD ? Colors::Lime : ImGuiX::Style::s_colorText, EE_ICON_NUMERIC_2_CIRCLE );
                    ImGui::SameLine();

                    isRelevantForLOD = ( submesh.m_lodMask & ( 1 << 3 ) ) != 0;
                    ImGui::TextColored( isRelevantForLOD ? Colors::Lime : ImGuiX::Style::s_colorText, EE_ICON_NUMERIC_3_CIRCLE );
                    ImGui::SameLine();

                    isRelevantForLOD = ( submesh.m_lodMask & ( 1 << 4 ) ) != 0;
                    ImGui::TextColored( isRelevantForLOD ? Colors::Lime : ImGuiX::Style::s_colorText, EE_ICON_NUMERIC_4_CIRCLE );
                    ImGui::SameLine();

                    isRelevantForLOD = ( submesh.m_lodMask & ( 1 << 5 ) ) != 0;
                    ImGui::TextColored( isRelevantForLOD ? Colors::Lime : ImGuiX::Style::s_colorText, EE_ICON_NUMERIC_5_CIRCLE );
                    ImGui::SameLine();

                    isRelevantForLOD = ( submesh.m_lodMask & ( 1 << 6 ) ) != 0;
                    ImGui::TextColored( isRelevantForLOD ? Colors::Lime : ImGuiX::Style::s_colorText, EE_ICON_NUMERIC_6_CIRCLE );
                    ImGui::SameLine();

                    isRelevantForLOD = ( submesh.m_lodMask & ( 1 << 7 ) ) != 0;
                    ImGui::TextColored( isRelevantForLOD ? Colors::Lime : ImGuiX::Style::s_colorText, EE_ICON_NUMERIC_7_CIRCLE );

                    ImGui::PopStyleVar();
                }

                // Material Name
                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();
                ImGui::TextColored( Color::GetCategorizedColor( VectorFindIndex( m_uniqueMaterialNames, submesh.m_materialNameID ) ), submesh.m_materialNameID.c_str() );

                // Material Resource
                //-------------------------------------------------------------------------

                ImGui::TableNextColumn();

                if ( m_materialPickers[submeshIdx]->UpdateAndDraw() )
                {
                    if ( isSelected )
                    {
                        ScopedDataFileModification dfm( this );
                        for ( int16_t const& selectedSubMeshIdx : m_selectedSubmeshes )
                        {
                            pMeshDescriptor->m_materialMappings[selectedSubMeshIdx].m_material = m_materialPickers[submeshIdx]->GetResourceID();
                        }
                    }
                    else // Only update the single entry
                    {
                        ScopedDataFileModification dfm( this );
                        pMeshDescriptor->m_materialMappings[submeshIdx].m_material = m_materialPickers[submeshIdx]->GetResourceID();
                    }
                }

                ImGui::PopID();
            }

            pMSIO = ImGui::EndMultiSelect();
            ApplySelectionRequests( pMSIO );

            //-------------------------------------------------------------------------

            if ( toggleVisibilityForAllSelected )
            {
                if ( !selectedVisible )
                {
                    m_hiddenSubmeshes = m_selectedSubmeshes;
                }
                else
                {
                    for ( int16_t submeshIdx : m_selectedSubmeshes )
                    {
                        m_hiddenSubmeshes.erase_first_unsorted( submeshIdx );
                    }
                }

                toggleVisibilityForAllSelected = false;
            }

            ImGui::EndTable();
        }

        ImGui::PopStyleColor( 3 );
    }

    //-------------------------------------------------------------------------

    void MeshEditor::DrawSocketsWindow( UpdateContext const& context, bool isFocused )
    {
        if ( !IsDataFileLoaded() )
        {
            return;
        }

        auto pMeshDescriptor = GetDataFile<MeshResourceDescriptor>();

        SkeletalMesh const* pSkeletalMesh = IsResourceLoaded() ? m_editedResource.GetPtr<SkeletalMesh>() : nullptr;

        // Socket List
        //-------------------------------------------------------------------------

        bool isItemPopupOpen = false;
        float const maxListHeight = Math::Max( 0.0f, ImGui::GetContentRegionAvail().y - 100 );

        ImGui::SetNextWindowSizeConstraints( ImVec2( 0.0f, 0.0f ), ImVec2( FLT_MAX, maxListHeight ) );
        if ( ImGui::BeginChild( "Sockets", ImVec2( -1, maxListHeight ), ImGuiChildFlags_ResizeY ) )
        {
            if ( ImGui::BeginTable( "SocketTable", m_isViewingSkeletalMesh ? 2 : 1, ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollY, ImGui::GetContentRegionAvail() ) )
            {
                ImGui::TableSetupColumn( "ID", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );
                if ( m_isViewingSkeletalMesh )
                {
                    ImGui::TableSetupColumn( "BoneID", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );
                }
                ImGui::TableSetupScrollFreeze( 0, 1 );

                ImGui::TableHeadersRow();

                //-------------------------------------------------------------------------

                InlineString labelStr;
                InlineString tooltipStr;
                Color labelColor = ImGuiX::Style::s_colorText;

                int32_t const numSockets = (int32_t) pMeshDescriptor->m_sockets.size();
                for ( int32_t i = 0; i < numSockets; i++ )
                {
                    auto& socket = pMeshDescriptor->m_sockets[i];

                    ImGui::PushID( i );
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    if ( socket.m_ID.IsValid() )
                    {
                        labelStr = socket.m_ID.c_str();
                        tooltipStr = labelStr;
                        labelColor = ImGuiX::Style::s_colorText;
                    }
                    else
                    {
                        labelStr = "Invalid ID";
                        tooltipStr = labelStr;
                        labelColor = Colors::Red;
                    }

                    for ( int32_t j = i - 1; j >= 0; j-- )
                    {
                        if ( pMeshDescriptor->m_sockets[i].m_ID == pMeshDescriptor->m_sockets[j].m_ID )
                        {
                            labelColor = Colors::Red;
                            tooltipStr.append( " (Duplicate Name Detected)" );
                        }
                    }

                    ImGui::PushStyleColor( ImGuiCol_Text, labelColor );
                    if ( ImGui::Selectable( labelStr.c_str(), m_selectedSocketIdx == i, ImGuiSelectableFlags_SpanAllColumns ) )
                    {
                        m_selectedSocketIdx = i;
                    }
                    ImGui::PopStyleColor();

                    ImGuiX::ItemTooltip( tooltipStr.c_str() );

                    if ( ImGui::BeginPopupContextItem() )
                    {
                        isItemPopupOpen = true;
                        m_selectedSocketIdx = i;

                        if ( ImGui::MenuItem( EE_ICON_CLOSE" Remove Socket" ) )
                        {
                            m_commandStack.PushCommand( [this, i] () { RemoveSocket( i ); } );
                        }

                        ImGui::EndPopup();
                    }

                    //-------------------------------------------------------------------------

                    if ( m_isViewingSkeletalMesh )
                    {
                        ImGui::TableNextColumn();
                        if ( socket.m_boneID.IsValid() )
                        {
                            Color boneColor = ImGuiX::Style::s_colorText;

                            if ( pSkeletalMesh != nullptr )
                            {
                                boneColor = ( pSkeletalMesh->GetBoneIndex( socket.m_boneID ) != InvalidIndex ) ? Colors::Lime : Colors::Red;
                            }

                            ImGui::TextColored( boneColor, socket.m_boneID.c_str() );
                        }
                    }

                    ImGui::PopID();
                }

                //-------------------------------------------------------------------------

                ImGui::EndTable();

                if ( !isItemPopupOpen && ImGui::BeginPopupContextItem() )
                {
                    if ( ImGui::MenuItem( EE_ICON_PLUS" Add Socket" ) )
                    {
                        m_commandStack.PushCommand( [this] () { AddSocket(); } );
                    }

                    ImGui::EndPopup();
                }
            }
        }
        ImGui::EndChild();

        // Socket Editor
        //-------------------------------------------------------------------------

        if ( ImGui::BeginChild( "SocketEditor" ) )
        {
            ImGui::SeparatorText( "Selected Socket" );

            if ( m_selectedSocketIdx == InvalidIndex )
            {
                m_socketPropertyGrid.SetTypeToEdit( nullptr );
            }
            else
            {
                if ( m_selectedSocketIdx >= pMeshDescriptor->m_sockets.size() )
                {
                    m_selectedSocketIdx = InvalidIndex;
                    m_socketPropertyGrid.SetTypeToEdit( nullptr );
                }
                else
                {
                    auto pSocketToEdit = &pMeshDescriptor->m_sockets[m_selectedSocketIdx];
                    if ( pSocketToEdit != m_socketPropertyGrid.GetEditedType() )
                    {
                        m_socketPropertyGrid.SetTypeToEdit( &pMeshDescriptor->m_sockets[m_selectedSocketIdx] );
                    }
                }
            }

            m_socketPropertyGrid.UpdateAndDraw();
        }
        ImGui::EndChild();
    }

    void MeshEditor::DrawSocketGizmo( Viewport const* pViewport, bool isFocused )
    {
        if ( !IsDataFileLoaded() || !IsResourceLoaded() )
        {
            return;
        }

        if ( m_selectedSocketIdx == InvalidIndex )
        {
            return;
        }

        //-------------------------------------------------------------------------

        auto TryGetGizmoTransform = [this] ( Transform &outTransform ) -> bool
        {
            outTransform = GetSelectedSocketTransform();
            return true;
        };

        auto ApplyGizmoResult = [this] ( Gizmo::Result const& result )
        {
            Transform shapeTransform = GetSelectedSocketTransform();
            shapeTransform = result.GetModifiedTransform( shapeTransform );
            SetSelectedSocketTransform( shapeTransform );
        };

        //-------------------------------------------------------------------------

        Transform gizmoTransform;
        bool const wasAbleToGetGizmoTransform = TryGetGizmoTransform( gizmoTransform );

        if ( wasAbleToGetGizmoTransform )
        {
            m_socketGizmo.SetCoordinateSystemSpace( CoordinateSpace::Local );

            auto const gizmoResult = m_socketGizmo.UpdateAndDraw( gizmoTransform.GetTranslation(), gizmoTransform.GetRotation(), *pViewport, isFocused, isFocused );
            switch ( gizmoResult.m_state )
            {
                case GizmoState::StartedManipulating:
                {
                    BeginDataFileModification();
                    ApplyGizmoResult( gizmoResult );
                }
                break;

                case GizmoState::Manipulating:
                {
                    ApplyGizmoResult( gizmoResult );
                }
                break;

                case GizmoState::StoppedManipulating:
                {
                    ApplyGizmoResult( gizmoResult );
                    EndDataFileModification();
                }
                break;

                default:
                break;
            }
        }
        else
        {
            m_socketGizmo.Reset();
        }
    }

    //-------------------------------------------------------------------------

    void MeshEditor::AddMeshDecal( ResourceID const& materialResourceID )
    {
        EE_ASSERT( IsDataFileLoaded() );

        MeshResourceDescriptor* pMeshDescriptor = GetDataFile<MeshResourceDescriptor>();

        ScopedDataFileModification sm( this );
        m_selectedDecalIndex = int32_t( pMeshDescriptor->m_meshDecals.size() );

        MeshDecalDefinition& meshDecal = pMeshDescriptor->m_meshDecals.emplace_back();
        if ( materialResourceID.IsValid() )
        {
            meshDecal.m_material = materialResourceID;
        }
    }

    void MeshEditor::RemoveMeshDecal( int32_t decalIdx )
    {
        EE_ASSERT( IsDataFileLoaded() );

        MeshResourceDescriptor* pMeshDescriptor = GetDataFile<MeshResourceDescriptor>();
        EE_ASSERT( decalIdx >= 0 && decalIdx < int32_t( pMeshDescriptor->m_meshDecals.size() ) );

        ScopedDataFileModification sm( this );
        pMeshDescriptor->m_meshDecals.erase( pMeshDescriptor->m_meshDecals.begin() + decalIdx );

        m_selectedDecalIndex = InvalidIndex;
    }

    Transform MeshEditor::GetSelectedMeshDecalTransform() const
    {
        EE_ASSERT( IsDataFileLoaded() );

        if ( m_selectedDecalIndex == InvalidIndex )
        {
            return Transform::Identity;
        }

        MeshResourceDescriptor const* pMeshDescriptor = GetDataFile<MeshResourceDescriptor>();
        EE_ASSERT( m_selectedDecalIndex < int32_t( pMeshDescriptor->m_meshDecals.size() ) );

        Transform const meshWorldTransform = ( m_pMeshComponent != nullptr ) ? m_pMeshComponent->GetWorldTransform() : Transform::Identity;
        return pMeshDescriptor->m_meshDecals[m_selectedDecalIndex].m_localTransform * meshWorldTransform;
    }

    void MeshEditor::SetSelectedMeshDecalTransform( Transform const& transform )
    {
        EE_ASSERT( IsDataFileLoaded() );

        if ( m_selectedDecalIndex == InvalidIndex )
        {
            return;
        }

        MeshResourceDescriptor* pMeshDescriptor = GetDataFile<MeshResourceDescriptor>();
        EE_ASSERT( m_selectedDecalIndex < int32_t( pMeshDescriptor->m_meshDecals.size() ) );

        Transform const meshWorldTransform = ( m_pMeshComponent != nullptr ) ? m_pMeshComponent->GetWorldTransform() : Transform::Identity;

        ScopedDataFileModification sm( this );
        pMeshDescriptor->m_meshDecals[m_selectedDecalIndex].m_localTransform = transform * meshWorldTransform.GetInverse();
    }

    Float3 MeshEditor::GetSelectedMeshDecalNonUniformScale() const
    {
        EE_ASSERT( IsDataFileLoaded() );

        if ( m_selectedDecalIndex == InvalidIndex )
        {
            return Float3::One;
        }

        MeshResourceDescriptor const* pMeshDescriptor = GetDataFile<MeshResourceDescriptor>();
        EE_ASSERT( m_selectedDecalIndex < int32_t( pMeshDescriptor->m_meshDecals.size() ) );

        return pMeshDescriptor->m_meshDecals[m_selectedDecalIndex].m_nonUniformScale;
    }

    void MeshEditor::SetSelectedMeshDecalNonUniformScale( Float3 const& nonUniformScale )
    {
        EE_ASSERT( IsDataFileLoaded() );

        if ( m_selectedDecalIndex == InvalidIndex )
        {
            return;
        }

        MeshResourceDescriptor* pMeshDescriptor = GetDataFile<MeshResourceDescriptor>();
        EE_ASSERT( m_selectedDecalIndex < int32_t( pMeshDescriptor->m_meshDecals.size() ) );

        ScopedDataFileModification sm( this );
        pMeshDescriptor->m_meshDecals[m_selectedDecalIndex].m_nonUniformScale = nonUniformScale;
    }

    //-------------------------------------------------------------------------

    void MeshEditor::DrawMeshDecalsWindow( UpdateContext const& context, bool isFocused )
    {
        if ( !IsDataFileLoaded() )
        {
            return;
        }

        auto pMeshDescriptor = GetDataFile<MeshResourceDescriptor>();
        int32_t const numDecals = int32_t( pMeshDescriptor->m_meshDecals.size() );

        // Toolbar
        //-------------------------------------------------------------------------

        ImGui::Checkbox( "Edit Mesh Decals", &m_isEditingMeshDecals );
        ImGuiX::ItemTooltip( "Show the decal volumes in the viewport and manipulate them with the gizmo" );

        ImGui::SameLine();
        if ( ImGui::Button( EE_ICON_PLUS" Add Decal" ) )
        {
            m_commandStack.PushCommand( [this] () { AddMeshDecal( ResourceID() ); } );
        }

        // Dropping a decal material on the add button creates a new decal using that material
        if ( ImGui::BeginDragDropTarget() )
        {
            if ( ImGuiPayload const* pPayload = ImGui::AcceptDragDropPayload( DragAndDrop::s_filePayloadID, ImGuiDragDropFlags_AcceptBeforeDelivery ) )
            {
                if ( pPayload->IsDelivery() )
                {
                    InlineString payloadStr = static_cast<char*>( pPayload->Data );
                    ResourceID const materialResourceID( payloadStr.c_str() );

                    if ( materialResourceID.IsValid() && materialResourceID.GetResourceTypeID() == DecalMaterial::GetStaticResourceTypeID() && m_pToolsContext->m_pDataFileRegistry->DoesFileExist( materialResourceID ) )
                    {
                        m_commandStack.PushCommand( [this, materialResourceID] () { AddMeshDecal( materialResourceID ); } );
                    }
                }
            }

            ImGui::EndDragDropTarget();
        }

        ImGuiX::ItemTooltip( "Add a new mesh decal, or drop a decal material on this button" );

        // Decal List
        //-------------------------------------------------------------------------

        float const listHeight = Math::Max( 60.0F, ImGui::GetContentRegionAvail().y * 0.4F );
        if ( ImGui::BeginChild( "MeshDecalList", ImVec2( -1, listHeight ), ImGuiChildFlags_ResizeY ) )
        {
            for ( int32_t decalIndex = 0; decalIndex < numDecals; ++decalIndex )
            {
                ImGui::PushID( decalIndex );

                MeshDecalDefinition const& decal = pMeshDescriptor->m_meshDecals[decalIndex];
                ResourceID const& materialID = decal.m_material.GetResourceID();

                InlineString label( InlineString::CtorSprintf(), "%d. %s", decalIndex, materialID.IsValid() ? materialID.c_str() : "<no material>" );

                if ( ImGui::Selectable( label.c_str(), m_selectedDecalIndex == decalIndex ) )
                {
                    m_selectedDecalIndex = decalIndex;
                }

                if ( ImGui::BeginPopupContextItem() )
                {
                    m_selectedDecalIndex = decalIndex;

                    if ( ImGui::MenuItem( EE_ICON_CLOSE" Remove Decal" ) )
                    {
                        m_commandStack.PushCommand( [this, decalIndex] () { RemoveMeshDecal( decalIndex ); } );
                    }

                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }

            if ( numDecals == 0 )
            {
                ImGui::TextDisabled( "No mesh decals" );
            }
        }
        ImGui::EndChild();

        // Selected Decal
        //-------------------------------------------------------------------------

        ImGui::SeparatorText( "Selected Decal" );

        if ( m_selectedDecalIndex == InvalidIndex || m_selectedDecalIndex >= numDecals )
        {
            m_selectedDecalIndex = InvalidIndex;
            m_decalPropertyGrid.SetTypeToEdit( nullptr );
        }
        else
        {
            auto pDecalToEdit = &pMeshDescriptor->m_meshDecals[m_selectedDecalIndex];
            if ( pDecalToEdit != m_decalPropertyGrid.GetEditedType() )
            {
                m_decalPropertyGrid.SetTypeToEdit( pDecalToEdit );
            }
        }

        m_decalPropertyGrid.UpdateAndDraw();
    }

    //-------------------------------------------------------------------------

    void MeshEditor::DrawMeshDecalGizmo( Viewport const* pViewport, bool isFocused )
    {
        if ( !IsDataFileLoaded() || !m_isEditingMeshDecals || m_selectedDecalIndex == InvalidIndex )
        {
            m_decalGizmo.Reset();
            return;
        }

        //-------------------------------------------------------------------------

        Transform gizmoTransform = GetSelectedMeshDecalTransform();

        auto const gizmoResult = m_decalGizmo.UpdateAndDraw( gizmoTransform.GetTranslation(), gizmoTransform.GetRotation(), *pViewport, isFocused, isFocused );

        // Compute the manipulated decal transform
        //-------------------------------------------------------------------------

        Transform newTransform = gizmoTransform;
        Float3 newNonUniformScale = GetSelectedMeshDecalNonUniformScale();

        if ( gizmoResult.IsScale() )
        {
            // A decal is a unit cube with per axis extents, the gizmo delta is relative so scale the extents instead of the transform
            Float3 const scaleDelta = gizmoResult.m_delta.ToFloat3();
            newNonUniformScale.m_x = Math::Max( newNonUniformScale.m_x * ( 1.0F + scaleDelta.m_x ), 0.001F );
            newNonUniformScale.m_y = Math::Max( newNonUniformScale.m_y * ( 1.0F + scaleDelta.m_y ), 0.001F );
            newNonUniformScale.m_z = Math::Max( newNonUniformScale.m_z * ( 1.0F + scaleDelta.m_z ), 0.001F );
        }
        else
        {
            newTransform = gizmoResult.GetModifiedTransform( gizmoTransform );
        }

        switch ( gizmoResult.m_state )
        {
            case GizmoState::StartedManipulating:
            {
                BeginDataFileModification();
                SetSelectedMeshDecalTransform( newTransform );
                SetSelectedMeshDecalNonUniformScale( newNonUniformScale );
            }
            break;

            case GizmoState::Manipulating:
            {
                SetSelectedMeshDecalTransform( newTransform );
                SetSelectedMeshDecalNonUniformScale( newNonUniformScale );
            }
            break;

            case GizmoState::StoppedManipulating:
            {
                SetSelectedMeshDecalTransform( newTransform );
                SetSelectedMeshDecalNonUniformScale( newNonUniformScale );
                EndDataFileModification();
            }
            break;

            default:
            break;
        }
    }

    //-------------------------------------------------------------------------

    void MeshEditor::DrawSkeletonWindow( UpdateContext const& context, bool isFocused )
    {
        if ( !IsResourceLoaded() )
        {
            return;
        }

        if ( !m_isViewingSkeletalMesh )
        {
            ImGui::Text( "No Skeleton Present" );
            return;
        }

        EE_ASSERT( m_pSkeletonTreeRoot != nullptr );

        SkeletalMesh const* pSkeletalMesh = m_editedResource.GetPtr<SkeletalMesh>();
        int32_t const numBones = pSkeletalMesh->GetNumBones();

        // Skeleton
        //-------------------------------------------------------------------------

        float const maxOutlinerHeight = Math::Max( 0.0f, ImGui::GetContentRegionAvail().y - 100 );

        ImGui::SetNextWindowSizeConstraints( ImVec2( 0.0f, 0.0f ), ImVec2( FLT_MAX, maxOutlinerHeight ) );
        if ( ImGui::BeginChild( "SkeletonOutliner", ImVec2( -1, maxOutlinerHeight ), ImGuiChildFlags_ResizeY ) )
        {
            auto ApplySelectionRequests = [this, pSkeletalMesh, numBones] ( ImGuiMultiSelectIO* pMSIO )
            {
                for ( ImGuiSelectionRequest const& req : pMSIO->Requests )
                {
                    if ( req.Type == ImGuiSelectionRequestType_SetAll )
                    {
                        m_selectedBoneIDs.clear();

                        if ( req.Selected )
                        {
                            for ( int32_t i = 0; i < numBones; i++ )
                            {
                                m_selectedBoneIDs.emplace_back( pSkeletalMesh->GetBoneID( i ) );
                            }
                        }
                    }
                    else if ( req.Type == ImGuiSelectionRequestType_SetRange )
                    {
                        if ( req.Selected )
                        {
                            for ( int64_t i = req.RangeFirstItem; i <= req.RangeLastItem; i++ )
                            {
                                StringID const boneID = pSkeletalMesh->GetBoneID( (int32_t) i );
                                VectorEmplaceBackUnique( m_selectedBoneIDs, boneID );
                            }
                        }
                        else
                        {
                            for ( int64_t i = req.RangeFirstItem; i <= req.RangeLastItem; i++ )
                            {
                                StringID const boneID = pSkeletalMesh->GetBoneID( (int32_t) i );
                                m_selectedBoneIDs.erase_first_unsorted( boneID );
                            }
                        }
                    }
                }
            };

            ImVec2 const tableSize = ImGui::GetContentRegionAvail();
            static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollY;
            if ( ImGui::BeginTable( "SkeletonTreeTable", 1, flags, tableSize ) )
            {
                ImGui::TableSetupColumn( "Bone", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize );

                ImGuiMultiSelectFlags const selectionFlags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d | ImGuiMultiSelectFlags_ClearOnClickVoid;
                ImGuiMultiSelectIO* pMSIO = ImGui::BeginMultiSelect( selectionFlags, -1, numBones );
                ApplySelectionRequests( pMSIO );

                DrawSkeletonTreeRow( m_pSkeletonTreeRoot );

                pMSIO = ImGui::EndMultiSelect();
                ApplySelectionRequests( pMSIO );

                ImGui::EndTable();
            }
        }
        ImGui::EndChild();

        // Bone Info
        //-------------------------------------------------------------------------

        if ( ImGui::BeginChild( "SkeletonBoneInfo" ) )
        {
            if ( ImGui::BeginTable( "BoneTreeTable", 2, ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollY, ImGui::GetContentRegionAvail() ) )
            {
                ImGui::TableSetupColumn( "Bone", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupColumn( "Bind Pose", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );
                ImGui::TableSetupScrollFreeze( 0, 1 );

                ImGui::TableHeadersRow();

                //-------------------------------------------------------------------------

                for ( StringID const& boneID : m_selectedBoneIDs )
                {
                    int32_t const selectedBoneIdx = pSkeletalMesh->GetBoneIndex( boneID );
                    EE_ASSERT( selectedBoneIdx != InvalidIndex );
                    Transform const& bindPoseBoneTransform = pSkeletalMesh->GetModelSpaceBindPoseTransform( selectedBoneIdx );

                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text( "%d. %s", selectedBoneIdx, boneID.c_str() );

                    ImGui::TableNextColumn();
                    ImGuiX::DrawTransform( bindPoseBoneTransform );
                }

                //-------------------------------------------------------------------------

                ImGui::EndTable();
            }
        }
        ImGui::EndChild();
    }

    void MeshEditor::DrawSkeletonTreeRow( BoneItem* pBone )
    {
        EE_ASSERT( m_isViewingSkeletalMesh );
        EE_ASSERT( pBone != nullptr );

        SkeletalMesh const* pSkeletalMesh = m_editedResource.GetPtr<SkeletalMesh>();
        int32_t const boneIdx = pBone->m_boneIdx;
        StringID const currentBoneID = pSkeletalMesh->GetBoneID( boneIdx );
        bool const isSelected = VectorContains( m_selectedBoneIDs, currentBoneID );

        //-------------------------------------------------------------------------

        ImGui::TableNextRow();

        //-------------------------------------------------------------------------
        // Draw Label
        //-------------------------------------------------------------------------

        InlineString const boneLabel( InlineString::CtorSprintf(), "%d. %s", boneIdx, currentBoneID.c_str() );

        ImGui::TableNextColumn();

        int32_t treeNodeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow;
        if ( pBone->m_children.empty() )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
        }

        if ( isSelected )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Selected;
        }

        ImGui::SetNextItemOpen( pBone->m_isExpanded );
        ImGui::AlignTextToFramePadding();

        ImGui::SetNextItemSelectionUserData( boneIdx );
        {
            ImGuiX::ScopedFont const sf( isSelected ? ImGuiX::Font::MediumBold : ImGuiX::Font::Default );
            pBone->m_isExpanded = ImGui::TreeNodeEx( boneLabel.c_str(), treeNodeFlags );
        }

        //-------------------------------------------------------------------------
        // Draw Children
        //-------------------------------------------------------------------------

        if ( pBone->m_isExpanded )
        {
            for ( BoneItem* pChildBone : pBone->m_children )
            {
                DrawSkeletonTreeRow( pChildBone );
            }
            ImGui::TreePop();
        }
    }
}