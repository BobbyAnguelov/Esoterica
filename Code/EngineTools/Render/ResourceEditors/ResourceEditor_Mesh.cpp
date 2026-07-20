#include "ResourceEditor_Mesh.h"
#include "EngineTools/Core/EditorTool.h"
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
        , m_isViewingSkeletalMesh( resourceID.GetResourceTypeID() == SkeletalMesh::GetStaticResourceTypeID() )
    {
        EE_ASSERT( resourceID.IsValid() );

        auto OnPreEdit = [this] ( PropertyEditInfo const& info ) { BeginDataFileModification(); };
        auto OnPostEdit = [this] ( PropertyEditInfo const& info ) { EE_ASSERT( m_pActiveUndoableAction != nullptr ); EndDataFileModification(); };

        m_preEditSocketBindingID = m_socketPropertyGrid.OnPreEdit().Bind( OnPreEdit );
        m_postEditSocketBindingID = m_socketPropertyGrid.OnPostEdit().Bind( OnPostEdit );

        m_socketGizmo.SetOption( ImGuiX::Gizmo::Options::AllowScale, false );
    }

    MeshEditor::~MeshEditor()
    {
        m_socketPropertyGrid.OnPreEdit().Unbind( m_preEditSocketBindingID );
        m_socketPropertyGrid.OnPostEdit().Unbind( m_postEditSocketBindingID );

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
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Mesh Info" ).c_str(), centerDockID1 );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Submeshes" ).c_str(), rightDockID0 );
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
                totalNumVertices += geo.GetNumClusterVertices();
                totalNumTriangles += geo.GetNumClusterTriangles();
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

        DrawSocketGizmo( pViewport, isFocused );
    }

    //-------------------------------------------------------------------------

    void MeshEditor::ClearAllSelections()
    {
        m_selectedSubmeshes.clear();
        m_selectedBoneIDs.clear();
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
        if ( pResourcePtr == &m_editedResource && IsResourceLoaded() )
        {
            ClearAllSelections();

            auto pMeshDescriptor = GetDataFile<MeshResourceDescriptor>();
            if ( pMeshDescriptor->FixUpMaterialMappings( m_editedResource.GetPtr<Mesh>() ) )
            {
                MarkDirty();
            }

            if ( m_isViewingSkeletalMesh )
            {
                CreateSkeletonTree();
            }

            CreatePreviewEntity();
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
                drawingContext.DrawAxis( Transform::Identity, 0.25f, 3.0f );
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
                    geo.IterateAllTriangles( [&geo, this, &drawingContext] ( Int3 clusterAnchor, int32_t clusterExponent, uint32_t vertexIndex0, uint32_t vertexIndex1, uint32_t vertexIndex2 )
                    {
                        // Fetch compressed vertices
                        StaticMeshVertex const& vertex0 = geo.GetVertex<StaticMeshVertex>( vertexIndex0 );
                        StaticMeshVertex const& vertex1 = geo.GetVertex<StaticMeshVertex>( vertexIndex1 );
                        StaticMeshVertex const& vertex2 = geo.GetVertex<StaticMeshVertex>( vertexIndex2 );

                        // Decompress positions and normals
                        Float3 vertexPosition0 = vertex0.GetPosition( clusterAnchor, clusterExponent );
                        Float3 vertexNormal0 = vertex0.GetNormal();

                        Float3 vertexPosition1 = vertex1.GetPosition( clusterAnchor, clusterExponent );
                        Float3 vertexNormal1 = vertex1.GetNormal();

                        Float3 vertexPosition2 = vertex2.GetPosition( clusterAnchor, clusterExponent );
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
                MeshResourceDescriptor* pMeshDescriptor = GetDataFile<MeshResourceDescriptor>();

                if ( m_selectedSubmeshes.empty() )
                {
                    if ( !m_submeshVisibility.empty() )
                    {
                        m_submeshVisibility.clear();
                        m_pMeshComponent->SetAllSubmeshVisibility( true );
                    }
                }
                else
                {
                    TVector<TPair<int32_t, bool>> visibility;
                    int32_t const numSubmeshes = pMesh->GetNumSubmeshes();
                    for ( int32_t i = 0; i < numSubmeshes; i++ )
                    {
                        if ( VectorContains( m_selectedSubmeshes, i ) )
                        {
                            visibility.emplace_back( i, true );
                        }
                        else
                        {
                            visibility.emplace_back( i, false );
                        }
                    }

                    if ( m_submeshVisibility != visibility )
                    {
                        m_submeshVisibility.swap( visibility );
                        m_pMeshComponent->SetSubmeshVisibility( m_submeshVisibility );
                    }
                }
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
                        Transform const& globalBoneTransform = pSkelMesh->GetBindPose()[selectedBoneIdx];
                        drawingContext.DrawAxis( globalBoneTransform, 0.25f, 3.0f );

                        Vector textLocation = globalBoneTransform.GetTranslation();
                        Vector const textLineLocation = textLocation - Vector( 0, 0, 0.01f );
                        drawingContext.DrawText3D( textLocation, selectedBoneID.c_str(), Colors::Yellow );
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

            return socketDef.m_offsetTransform * pSkelMesh->GetBindPoseTransform( boneIdx );
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
            socketDef.m_offsetTransform = transform * pSkelMesh->GetBindPoseTransform( boneIdx ).GetInverse();
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

        float const indexColumnWidth = ImGui::CalcTextSize( "999" ).x;
        float const dataColumnWidth = ImGui::CalcTextSize( "999.999%" ).x;

        if ( ImGui::BeginTable( "LOD", 11, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, ImGui::GetContentRegionAvail() ) )
        {
            ImGui::TableSetupColumn( "", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_WidthFixed, indexColumnWidth );

            ImGui::TableSetupColumn( "Distance", ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed, dataColumnWidth );
            ImGui::TableSetupColumn( "Clusters", ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed, dataColumnWidth );

            #if 0 // TODO: MeshStatistics will be reworked
            ImGui::TableSetupColumn( "Cluster utilization Min", ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed, dataColumnWidth );
            ImGui::TableSetupColumn( "Cluster utilization Max", ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed, dataColumnWidth );
            ImGui::TableSetupColumn( "Cluster utilization Avg", ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed, dataColumnWidth );
            ImGui::TableSetupColumn( "Cluster utilization Median", ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed, dataColumnWidth );
            ImGui::TableSetupColumn( "Cluster Vtx overhead", ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed, dataColumnWidth );
            ImGui::TableSetupColumn( "Vtx triangle reuse", ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed, dataColumnWidth );
            ImGui::TableSetupColumn( "Vtx compression min accuracy", ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed, dataColumnWidth );
            ImGui::TableSetupColumn( "Vtx compression avg accuracy", ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed, dataColumnWidth );
            #endif

            ImGui::TableSetupScrollFreeze( 0, 1 );
            ImGui::TableAngledHeadersRow();

            //-------------------------------------------------------------------------

            TArrayView<float const> lodDistance = pMesh->GetLODDistances();
            for ( uint32_t lodIndex = 0; lodIndex < lodDistance.size(); ++lodIndex )
            {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text( "%d", lodIndex );

                ImGui::TableNextColumn();
                ImGui::Text( "%.2f", lodDistance[lodIndex] );

                #if 0 // TODO: MeshStatistics will be reworked
                ImGui::TableNextColumn();
                ImGui::Text( "%i", lod.m_numClusters );

                ImGui::TableNextColumn();
                ImGui::Text( "%.2f%%", pMesh->GetStatistics( lodIndex ).m_minimumClusterUtilization * 100.0F );

                ImGui::TableNextColumn();
                ImGui::Text( "%.2f%%", pMesh->GetStatistics( lodIndex ).m_maximumClusterUtilization * 100.0F );

                ImGui::TableNextColumn();
                ImGui::Text( "%.2f%%", pMesh->GetStatistics( lodIndex ).m_averageClusterUtilization * 100.0F );

                ImGui::TableNextColumn();
                ImGui::Text( "%.2f%%", pMesh->GetStatistics( lodIndex ).m_medianClusterUtilization * 100.0F );

                ImGui::TableNextColumn();
                ImGui::Text( "%.2f%%", pMesh->GetStatistics( lodIndex ).m_clusterVertexOverhead * 100.0F );

                ImGui::TableNextColumn();
                ImGui::Text( "%.2f%%", pMesh->GetStatistics( lodIndex ).m_vertexTriangleReuse * 100.0F );

                ImGui::TableNextColumn();
                ImGui::Text( "%.2f%%", pMesh->GetStatistics( lodIndex ).m_minimumCompressionAccuracy * 100.0F );

                ImGui::TableNextColumn();
                ImGui::Text( "%.2f%%", pMesh->GetStatistics( lodIndex ).m_averageCompressionAccuracy * 100.0F );
                #endif
            }

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

        while ( m_materialPickers.size() < numSubmeshes )
        {
            m_materialPickers.emplace_back( EE::New<ResourcePicker>( *m_pToolsContext, Material::GetStaticResourceTypeID() ) );
        }

        EE_ASSERT( pMeshDescriptor->m_materialMappings.size() == numSubmeshes );
        for ( size_t i = 0; i < numSubmeshes; i++ )
        {
            m_materialPickers[i]->SetResourceID( pMeshDescriptor->m_materialMappings[i].m_material.GetResourceID() );
        }

        //-------------------------------------------------------------------------

        float totalLabelWidth = 0;
        {
            ImGuiX::ScopedFont const sf( ImGuiX::FontType::Bold, 36 );
            totalLabelWidth = ImGui::CalcTextSize( "00 " ).x;
        }

        ImGuiStyle const& style = ImGui::GetStyle();
        InlineString str;

        //-------------------------------------------------------------------------

        for ( int32_t submeshIdx = 0; submeshIdx < numSubmeshes; submeshIdx++ )
        {
            auto const& submesh = pMesh->GetSubmesh( submeshIdx );
            auto& materialMapping = pMeshDescriptor->m_materialMappings[submeshIdx];

            InlineString childWindowName( InlineString::CtorSprintf(), "SM%d", submeshIdx );

            float const windowHeight = m_materialPickers[submeshIdx]->GetHeight() + ImGui::GetFrameHeight() * 3 + style.WindowPadding.y;
            ImGui::PushStyleVar( ImGuiStyleVar_ChildRounding, 4 );
            ImGui::PushStyleColor( ImGuiCol_ChildBg, ImGuiX::Style::s_colorGray2 );
            if ( ImGui::BeginChild( childWindowName.c_str(), ImVec2( -1, windowHeight ), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoDecoration ) )
            {
                bool isSelected = VectorContains( m_selectedSubmeshes, submeshIdx );

                // Draw label
                //-------------------------------------------------------------------------

                {
                    str.sprintf( "%d", submeshIdx );
                    ImGuiX::ScopedFont const sf( ImGuiX::FontType::Bold, 36 );
                    ImVec2 const labelSize = ImGui::CalcTextSize( str.c_str() );
                    float const offsetX = Math::Max( 0.0f, ( totalLabelWidth - labelSize.x ) / 2 );
                    float const offsetY = Math::Max( 0.0f, ( ImGui::GetContentRegionAvail().y - labelSize.y ) / 2 + style.WindowPadding.y );
                    auto pDrawList = ImGui::GetWindowDrawList();
                    pDrawList->AddRectFilled( ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImVec2( totalLabelWidth, windowHeight ), isSelected ? Colors::MediumSeaGreen : Colors::SlateGray, 4 );
                    pDrawList->AddText( ImGui::GetWindowPos() + ImVec2( offsetX, offsetY ), ImGuiX::Style::s_colorText, str.c_str() );
                }

                // Details
                //-------------------------------------------------------------------------

                ImGui::Indent( totalLabelWidth );
                ImGui::Text( EE_ICON_CUBE_OUTLINE" %s", submesh.m_ID.c_str() );
                ImGuiX::TextTooltip( "Source Mesh Node: %s", submesh.m_ID.c_str() );
                ImGui::Text( EE_ICON_PALETTE" %s", submesh.m_materialNameID.c_str() );
                ImGuiX::TextTooltip( "Source Material Name: %s", submesh.m_materialNameID.c_str() );

                // LOD and Isolate
                //-------------------------------------------------------------------------

                bool isRelevantForLOD = false;

                ImGui::AlignTextToFramePadding();
                ImGui::Text( "LOD:" );
                ImGui::SameLine();

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

                ImVec2 buttonWidth( 100, 0 );
                ImGui::SameLine();
                ImGui::Dummy( ImVec2( ImGui::GetContentRegionAvail().x - buttonWidth.x - style.ItemSpacing.x, 0 ) );
                ImGui::SameLine();

                ImGuiX::ToggleButtonSettings settings;
                settings.m_pOnIcon = EE_ICON_EYE_OUTLINE;
                settings.m_pOffIcon = EE_ICON_EYE_OFF_OUTLINE;
                settings.m_pOnLabel = "Visible";
                settings.m_pOffLabel = "Hidden";
                settings.m_onBackgroundColor = Colors::MediumSeaGreen;
                settings.m_offBackgroundColor = m_selectedSubmeshes.empty() ? Colors::SlateGray : Colors::PaleVioletRed;
                settings.m_onForegroundColor = Colors::Black;
                settings.m_offForegroundColor = Colors::Black;
                settings.m_onIconColor = Colors::White;
                settings.m_offIconColor = Colors::White;
                settings.m_isFlatButton = false;

                if ( ImGuiX::ToggleButtonEx( settings, isSelected, buttonWidth ) )
                {
                    if ( isSelected )
                    {
                        VectorEmplaceBackUnique( m_selectedSubmeshes, submeshIdx );
                    }
                    else
                    {
                        m_selectedSubmeshes.erase_first_unsorted( submeshIdx );
                    }
                }

                //-------------------------------------------------------------------------

                if ( m_materialPickers[submeshIdx]->UpdateAndDraw() )
                {
                    ScopedDataFileModification dfm( this );
                    pMeshDescriptor->m_materialMappings[submeshIdx].m_material = m_materialPickers[submeshIdx]->GetResourceID();
                }

                ImGui::Unindent( totalLabelWidth );
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }
    }

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

        auto ApplyGizmoResult = [this] ( ImGuiX::Gizmo::Result const& result )
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

            auto const gizmoResult = m_socketGizmo.Draw( gizmoTransform.GetTranslation(), gizmoTransform.GetRotation(), *pViewport );
            switch ( gizmoResult.m_state )
            {
                case ImGuiX::GizmoState::StartedManipulating:
                {
                    BeginDataFileModification();
                    ApplyGizmoResult( gizmoResult );
                }
                break;

                case ImGuiX::GizmoState::Manipulating:
                {
                    ApplyGizmoResult( gizmoResult );
                }
                break;

                case ImGuiX::GizmoState::StoppedManipulating:
                {
                    ApplyGizmoResult( gizmoResult );
                    EndDataFileModification();
                }
                break;

                default:
                break;
            }

            if ( isFocused )
            {
                if ( ImGui::IsKeyPressed( ImGuiKey_Space ) )
                {
                    m_socketGizmo.SwitchToNextMode();
                    if ( m_socketGizmo.GetMode() == ImGuiX::Gizmo::Mode::Scale )
                    {
                        m_socketGizmo.SetCoordinateSystemSpace( CoordinateSpace::Local );
                    }
                    else
                    {
                        m_socketGizmo.SetCoordinateSystemSpace( CoordinateSpace::World );
                    }
                }
            }
        }
        else
        {
            m_socketGizmo.Reset();
        }
    }

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

            ImGuiMultiSelectFlags const selectionFlags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d | ImGuiMultiSelectFlags_ClearOnClickVoid;
            ImGuiMultiSelectIO* pMSIO = ImGui::BeginMultiSelect( selectionFlags, -1, numBones );
            ApplySelectionRequests( pMSIO );

            ImVec2 const tableSize = ImGui::GetContentRegionAvail();
            static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollY;
            if ( ImGui::BeginTable( "SkeletonTreeTable", 1, flags, tableSize ) )
            {
                ImGui::TableSetupColumn( "Bone", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize );
                DrawSkeletonTreeRow( m_pSkeletonTreeRoot );
                ImGui::EndTable();
            }

            pMSIO = ImGui::EndMultiSelect();
            ApplySelectionRequests( pMSIO );
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
                    Transform const& bindPoseBoneTransform = pSkeletalMesh->GetBindPose()[selectedBoneIdx];

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