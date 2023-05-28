#include "Workspace_Skeleton.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Animation/AnimationPose.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/UpdateContext.h"
#include "System/Math/MathStringHelpers.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    EE_RESOURCE_WORKSPACE_FACTORY( SkeletonWorkspaceFactory, Skeleton, SkeletonWorkspace );

    //-------------------------------------------------------------------------

    static bool IsValidWeight( float weight )
    {
        return ( weight >= 0.0f && weight <= 1.0f ) || weight == -1.0f;
    }

    static Color GetColorForWeight( float w )
    {
        EE_ASSERT( IsValidWeight( w ) );

        // 0%
        if ( w <= 0.0f )
        {
            return Colors::Gray;
        }
        // 1~20%
        else if ( w > 0.0f && w < 0.2f )
        {
            return Color( 0xFF0D0DFF );
        }
        // 20~40%
        else if ( w >= 0.2f && w < 0.4f )
        {
            return Color( 0xFF4E11FF );
        }
        // 40~60%
        else if ( w >= 0.4f && w < 0.6f )
        {
            return Color( 0xFF8E15FF );
        }
        // 60~80%
        else if ( w >= 0.6f && w < 0.8f )
        {
            return Color( 0xFAB733FF );
        }
        // 80~99%
        else if ( w >= 0.08f && w < 1.0f )
        {
            return Color( 0xACB334FF );
        }
        // 100%
        else if ( w == 1.0f )
        {
            return Color( 0x69B34CFF );
        }

        return Colors::White;
    }

    //-------------------------------------------------------------------------

    SkeletonWorkspace::~SkeletonWorkspace()
    {
        EE_ASSERT( m_pSkeletonTreeRoot == nullptr );
    }

    void SkeletonWorkspace::InitializeDockingLayout( ImGuiID dockspaceID ) const
    {
        ImGuiID viewportDockID = 0;
        ImGuiID leftDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.3f, nullptr, &viewportDockID );
        ImGuiID rightDockID = ImGui::DockBuilderSplitNode( viewportDockID, ImGuiDir_Right, 0.2f, nullptr, &viewportDockID );
        ImGuiID bottomRightDockID = ImGui::DockBuilderSplitNode( rightDockID, ImGuiDir_Down, 0.33f, nullptr, &rightDockID );

        // Dock windows
        //-------------------------------------------------------------------------

        ImGui::DockBuilderDockWindow( GetViewportWindowID(), viewportDockID );
        ImGui::DockBuilderDockWindow( m_skeletonWindowName.c_str(), leftDockID );
        ImGui::DockBuilderDockWindow( m_boneMasksWindowName.c_str(), rightDockID );
        ImGui::DockBuilderDockWindow( m_descriptorWindowName.c_str(), bottomRightDockID );
        ImGui::DockBuilderDockWindow( m_boneInfoWindowName.c_str(), bottomRightDockID );
    }

    //-------------------------------------------------------------------------

    void SkeletonWorkspace::Initialize( UpdateContext const& context )
    {
        EE_ASSERT( m_pPreviewEntity == nullptr );

        TWorkspace<Skeleton>::Initialize( context );

        m_skeletonWindowName.sprintf( "Skeleton##%u", GetID() );
        m_boneInfoWindowName.sprintf( "Bone Info##%u", GetID() );
        m_boneMasksWindowName.sprintf( "Bone Masks##%u", GetID() );

        //-------------------------------------------------------------------------

        if ( IsDescriptorLoaded() )
        {
            CreatePreviewEntity();
        }
    }

    void SkeletonWorkspace::Shutdown( UpdateContext const& context )
    {
        DestroySkeletonTree();
        TWorkspace<Skeleton>::Shutdown( context );
    }

    void SkeletonWorkspace::CreatePreviewEntity()
    {
        EE_ASSERT( m_pPreviewEntity == nullptr );

        auto pSkeletonDescriptor = GetDescriptor<SkeletonResourceDescriptor>();
        if ( pSkeletonDescriptor->m_previewMesh.IsSet() )
        {
            auto pMeshComponent = EE::New<Render::SkeletalMeshComponent>( StringID( "Mesh Component" ) );
            pMeshComponent->SetSkeleton( m_descriptorID );
            pMeshComponent->SetMesh( pSkeletonDescriptor->m_previewMesh.GetResourceID() );

            m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
            m_pPreviewEntity->AddComponent( pMeshComponent );
            AddEntityToWorld( m_pPreviewEntity );
        }
    }

    void SkeletonWorkspace::PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        TWorkspace<Skeleton>::PostUndoRedo( operation, pAction );

        if ( !IsResourceLoaded() )
        {
            if ( IsEditingBoneMask() )
            {
                StopEditingMask();
            }
        }
    }

    void SkeletonWorkspace::OnHotReloadStarted( bool descriptorNeedsReload, TInlineVector<Resource::ResourcePtr*, 10> const& resourcesToBeReloaded )
    {
        TWorkspace<Skeleton>::OnHotReloadStarted( descriptorNeedsReload, resourcesToBeReloaded );

        if ( m_pPreviewEntity != nullptr  )
        {
            DestroyEntityInWorld( m_pPreviewEntity );
        }
    }

    void SkeletonWorkspace::OnHotReloadComplete()
    {
        TWorkspace<Skeleton>::OnHotReloadComplete();

        if ( IsDescriptorLoaded() )
        {
            CreatePreviewEntity();
        }

        if ( IsResourceLoaded() )
        {
            ValidateDescriptorBoneMaskDefinitions();
        }
    }

    //-------------------------------------------------------------------------

    void SkeletonWorkspace::Update( UpdateContext const& context, ImGuiWindowClass* pWindowClass, bool isFocused )
    {
        TWorkspace::Update( context, pWindowClass, isFocused );

        // Wait for resource to load
        //-------------------------------------------------------------------------

        if ( IsResourceLoaded() && m_pSkeletonTreeRoot == nullptr )
        {
            ValidateDescriptorBoneMaskDefinitions();
            CreateSkeletonTree();
        }

        // Debug drawing in Viewport
        //-------------------------------------------------------------------------

        if ( IsResourceLoaded() )
        {
            if ( IsEditingBoneMask() )
            {
                DrawBoneMaskPreview();
            }
            else
            {
                DrawSkeletonPreview();
            }
        }

        // Windows
        //-------------------------------------------------------------------------

        DrawSkeletonWindow( context, pWindowClass );
        DrawBoneInfoWindow( context, pWindowClass );
        DrawBoneMaskWindow( context, pWindowClass );

        // Dialogs
        //-------------------------------------------------------------------------

        DrawDialogs( context );
    }

    static int FilterMaskNameChars( ImGuiInputTextCallbackData* data )
    {
        if ( isalnum( data->EventChar ) || data->EventChar == '_' || data->EventChar == ' ' )
        {
            return 0;
        }
        return 1;
    }

    void SkeletonWorkspace::DrawDialogs( UpdateContext const& context )
    {
        bool isDialogOpen = m_activeOperation != OperationType::None;

        //-------------------------------------------------------------------------

        auto EscCancelCheck = [&] ()
        {
            if ( ImGui::IsKeyPressed( ImGuiKey_Escape ) )
            {
                isDialogOpen = false;
                ImGui::CloseCurrentPopup();
            }
        };

        //-------------------------------------------------------------------------

        switch ( m_activeOperation )
        {
            case OperationType::RenameMask:
            {
                if ( ImGuiX::BeginViewportPopupModal( "Rename Mask", &isDialogOpen, ImVec2( 300, -1 ) ) )
                {
                    SkeletonResourceDescriptor* pDescriptor = GetDescriptor<SkeletonResourceDescriptor>();

                    auto ValidateName = [this, pDescriptor] ( StringID ID )
                    {
                        // Invalid IDs are invalid :P
                        if ( !ID.IsValid() )
                        {
                            return false;
                        }

                        // This is our original name
                        if ( ID == m_operationID )
                        {
                            return true;
                        }

                        // Check for uniqueness
                        for ( auto const& def : pDescriptor->m_boneMaskDefinitions )
                        {
                            if ( def.m_ID == ID )
                            {
                                return false;
                            }
                        }

                        return true;
                    };

                    bool renameRequested = false;
                    bool isValidName = ValidateName( StringID( m_renameBuffer ) );

                    ImGui::PushStyleColor( ImGuiCol_Text, isValidName ? ImGuiX::Style::s_colorText.Value : ImGuiX::ImColors::Red.Value );

                    if ( ImGui::InputText( "##MaskName", m_renameBuffer, 255, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCharFilter, FilterMaskNameChars ) )
                    {
                        isValidName = ValidateName( StringID( m_renameBuffer ) );
                        if ( isValidName )
                        {
                            renameRequested = true;
                        }
                    }

                    ImGui::PopStyleColor();

                    //-------------------------------------------------------------------------

                    ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 4 );
                    ImGui::NewLine();

                    float const dialogWidth = ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ).x;
                    ImGui::SameLine( 0, dialogWidth - 104 );

                    ImGui::BeginDisabled( !isValidName );
                    if ( ImGui::Button( "Ok", ImVec2( 50, 0 ) ) || renameRequested )
                    {
                        RenameBoneMask( m_operationID, StringID( m_renameBuffer ) );
                        m_operationID.Clear();
                        m_activeOperation = OperationType::None;
                    }
                    ImGui::EndDisabled();

                    ImGui::SameLine( 0, 4 );

                    if ( ImGui::Button( "Cancel", ImVec2( 50, 0 ) ) )
                    {
                        m_operationID.Clear();
                        m_activeOperation = OperationType::None;
                    }

                    EscCancelCheck();
                    ImGui::EndPopup();
                }
            }
            break;

            case OperationType::DeleteMask:
            {
                if ( ImGuiX::BeginViewportPopupModal( "Delete Mask", &isDialogOpen, ImVec2( 500, -1 ) ) )
                {
                    ImGui::Text( "Are you sure you want to delete this bone mask?" );
                    ImGui::NewLine();

                    float const dialogWidth = ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ).x;
                    ImGui::SameLine( 0, dialogWidth - 64 );

                    if ( ImGui::Button( "Yes", ImVec2( 30, 0 ) ) )
                    {
                        DestroyBoneMask( m_operationID );
                        m_operationID.Clear();
                        m_activeOperation = OperationType::None;
                    }

                    ImGui::SameLine( 0, 4 );

                    if ( ImGui::Button( "No", ImVec2( 30, 0 ) ) )
                    {
                        m_operationID.Clear();
                        m_activeOperation = OperationType::None;
                    }

                    EscCancelCheck();
                    ImGui::EndPopup();
                }
            }
            break;

            default:
            break;
        }

        //-------------------------------------------------------------------------

        // If the dialog was closed (i.e. operation canceled)
        if ( !isDialogOpen )
        {
            m_operationID.Clear();
            m_activeOperation = OperationType::None;
        }
    }

    // Skeleton
    //-------------------------------------------------------------------------

    void SkeletonWorkspace::DrawSkeletonWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_skeletonWindowName.c_str() ) )
        {
            if ( IsResourceLoaded() )
            {
                EE_ASSERT( m_pSkeletonTreeRoot != nullptr );

                static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;
                if ( ImGui::BeginTable( "WeightEditor", IsEditingBoneMask() ? 2 : 1, flags ) )
                {
                    // The first column will use the default _WidthStretch when ScrollX is Off and _WidthFixed when ScrollX is On
                    ImGui::TableSetupColumn( "Bone", ImGuiTableColumnFlags_NoHide );

                    if ( IsEditingBoneMask() )
                    {
                        ImGui::TableSetupColumn( "Weight", ImGuiTableColumnFlags_WidthFixed, 120.0f );
                    }

                    ImGui::TableHeadersRow();

                    //-------------------------------------------------------------------------

                    DrawBoneRow( m_pSkeletonTreeRoot );

                    ImGui::EndTable();
                }
            }
        }
        ImGui::End();
    }

    void SkeletonWorkspace::CreateSkeletonTree()
    {
        TVector<BoneInfo*> boneInfos;

        // Create all infos
        Skeleton const* pSkeleton = m_workspaceResource.GetPtr();
        int32_t const numBones = pSkeleton->GetNumBones();
        for ( auto i = 0; i < numBones; i++ )
        {
            auto& pBoneInfo = boneInfos.emplace_back( EE::New<BoneInfo>() );
            pBoneInfo->m_boneIdx = i;
        }

        // Create hierarchy
        for ( auto i = 1; i < numBones; i++ )
        {
            int32_t const parentBoneIdx = pSkeleton->GetParentBoneIndex( i );
            EE_ASSERT( parentBoneIdx != InvalidIndex );
            boneInfos[parentBoneIdx]->m_children.emplace_back( boneInfos[i] );
        }

        // Set root
        m_pSkeletonTreeRoot = boneInfos[0];
    }

    void SkeletonWorkspace::DestroySkeletonTree()
    {
        if ( m_pSkeletonTreeRoot != nullptr )
        {
            m_pSkeletonTreeRoot->DestroyChildren();
            EE::Delete( m_pSkeletonTreeRoot );
        }
    }

    void SkeletonWorkspace::DrawBoneRow( BoneInfo* pBone )
    {
        EE_ASSERT( pBone != nullptr );

        Skeleton const* pSkeleton = m_workspaceResource.GetPtr();
        int32_t const boneIdx = pBone->m_boneIdx;
        StringID const currentBoneID = pSkeleton->GetBoneID( boneIdx );
        ImColor const rowColor = IsEditingBoneMask() ? ImGuiX::ToIm( GetColorForWeight( m_editedBoneWeights[boneIdx] ) ) : ImGuiX::Style::s_colorText;

        //-------------------------------------------------------------------------

        ImGui::TableNextRow();

        //-------------------------------------------------------------------------
        // Draw Label
        //-------------------------------------------------------------------------

        InlineString const boneLabel( InlineString::CtorSprintf(), EE_ICON_BONE" %d. %s", boneIdx, currentBoneID.c_str() );

        ImGui::TableNextColumn();

        int32_t treeNodeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if ( pBone->m_children.empty() )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
        }

        ImGui::SetNextItemOpen( pBone->m_isExpanded );
        ImGui::AlignTextToFramePadding();
        ImGui::PushStyleColor( ImGuiCol_Text, rowColor.Value );
        pBone->m_isExpanded = ImGui::TreeNodeEx( boneLabel.c_str(), treeNodeFlags );

        // Handle bone selection
        if ( ImGui::IsItemClicked() )
        {
            m_selectedBoneID = currentBoneID;
        }

        ImGui::PopStyleColor();

        //-------------------------------------------------------------------------
        // Draw Context Menu
        //-------------------------------------------------------------------------

        InlineString const contextMenuID( InlineString::CtorSprintf(), "##%s_ctx", currentBoneID.c_str() );
        if ( ImGui::BeginPopupContextItem( contextMenuID.c_str() ) )
        {
            if ( ImGui::MenuItem( EE_ICON_IDENTIFIER" Copy Bone ID" ) )
            {
                ImGui::SetClipboardText( currentBoneID.c_str() );
            }

            //-------------------------------------------------------------------------

            if ( IsEditingBoneMask() )
            {
                ImGuiX::TextSeparator( "Propagate");

                if ( ImGui::MenuItem( EE_ICON_CONTENT_COPY" Propagate Current Weight" ) )
                {
                    SetAllChildWeights( boneIdx, m_editedBoneWeights[boneIdx] );
                }

                //-------------------------------------------------------------------------

                ImGuiX::TextSeparator( "Fully Masked" );

                if ( boneIdx != 0 && ImGui::MenuItem( EE_ICON_NUMERIC_0_BOX_MULTIPLE_OUTLINE" Set bone and all children to 0" ) )
                {
                    SetAllChildWeights( boneIdx, 0.0f, true );
                }

                if ( ImGui::MenuItem( EE_ICON_NUMERIC_0_BOX_MULTIPLE_OUTLINE" Set all children to 0" ) )
                {
                    SetAllChildWeights( boneIdx, 0.0f );
                }

                //-------------------------------------------------------------------------

                ImGuiX::TextSeparator( "No Mask" );

                if ( boneIdx != 0 && ImGui::MenuItem( EE_ICON_NUMERIC_1_BOX_MULTIPLE_OUTLINE" Set bone and all children to 1" ) )
                {
                    SetAllChildWeights( boneIdx, 1.0f, true );
                }

                if ( ImGui::MenuItem( EE_ICON_NUMERIC_1_BOX_MULTIPLE_OUTLINE" Set all children to 1" ) )
                {
                    SetAllChildWeights( boneIdx, 1.0f );
                }

                //-------------------------------------------------------------------------

                ImGuiX::TextSeparator( "Clear" );

                if ( boneIdx != 0 && ImGui::MenuItem( EE_ICON_CLOSE_CIRCLE_MULTIPLE_OUTLINE" Unset bone and all children" ) )
                {
                    SetAllChildWeights( boneIdx, -1.0f, true );
                }

                if ( ImGui::MenuItem( EE_ICON_CLOSE_CIRCLE_MULTIPLE_OUTLINE" Unset all children" ) )
                {
                    SetAllChildWeights( boneIdx, -1.0f );
                }
            }

            ImGui::EndPopup();
        }

        //-------------------------------------------------------------------------
        // Draw Weight Editor
        //-------------------------------------------------------------------------

        if ( IsEditingBoneMask() )
        {
            ImGui::TableNextColumn();

            if ( boneIdx != 0 )
            {
                InlineString checkboxID;
                checkboxID.sprintf( "##%s_cb", currentBoneID.c_str() );

                bool isSet = m_editedBoneWeights[boneIdx] != -1.0f;
                if ( ImGui::Checkbox( checkboxID.c_str(), &isSet ) )
                {
                    SetWeight( boneIdx, isSet ? 1.0f : -1.0f );
                }

                if ( isSet )
                {
                    InlineString weightEditorID;
                    weightEditorID.sprintf( "##%s", currentBoneID.c_str() );

                    ImGui::SameLine();
                    ImGui::SetNextItemWidth( -1 );
                    ImGui::SliderFloat( weightEditorID.c_str(), &m_editedBoneWeights[boneIdx], 0.0f, 1.0f, "%.2f" );
                    if ( ImGui::IsItemDeactivatedAfterEdit() )
                    {
                        SetWeight( boneIdx, m_editedBoneWeights[boneIdx] );
                    }
                }
                else
                {
                    ImGui::SameLine();
                    float const demoWeight = m_previewBoneMask.GetWeight( boneIdx );
                    ImGui::PushStyleColor( ImGuiCol_Text, rowColor.Value );
                    ImGui::Text( "%.2f", demoWeight );
                    ImGui::PopStyleColor();
                }
            }
        }

        //-------------------------------------------------------------------------
        // Draw Children
        //-------------------------------------------------------------------------

        if ( pBone->m_isExpanded )
        {
            for ( BoneInfo* pChildBone : pBone->m_children )
            {
                DrawBoneRow( pChildBone );
            }
            ImGui::TreePop();
        }
    }

    void SkeletonWorkspace::DrawBoneInfoWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        auto DrawTransform = [] ( Transform transform )
        {
            Vector const& translation = transform.GetTranslation();
            Quaternion const& rotation = transform.GetRotation();
            EulerAngles const angles = rotation.ToEulerAngles();
            Float4 const quatValues = rotation.ToFloat4();

            ImGui::Text( "Rot (Quat): X: %.3f, Y: %.3f, Z: %.3f, W: %.3f", quatValues.m_x, quatValues.m_y, quatValues.m_z, quatValues.m_w );
            ImGui::Text( "Rot (Euler): X: %.3f, Y: %.3f, Z: %.3f", angles.m_x.ToDegrees().ToFloat(), angles.m_y.ToDegrees().ToFloat(), angles.m_z.ToDegrees().ToFloat() );
            ImGui::Text( "Trans: X: %.3f, Y: %.3f, Z: %.3f", translation.GetX(), translation.GetY(), translation.GetZ() );
            ImGui::Text( "Scl: %.3f", transform.GetScale() );
        };

        //-------------------------------------------------------------------------

        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 8, 8 ) );
        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_boneInfoWindowName.c_str() ) )
        {
            if ( IsResourceLoaded() && m_selectedBoneID.IsValid() )
            {
                Skeleton const* pSkeleton = m_workspaceResource.GetPtr();
                int32_t const selectedBoneIdx = pSkeleton->GetBoneIndex( m_selectedBoneID );
                EE_ASSERT( selectedBoneIdx != InvalidIndex );

                {
                    ImGuiX::ScopedFont sf( ImGuiX::Font::LargeBold );
                    ImGui::Text( "%d. %s", selectedBoneIdx, pSkeleton->GetBoneID( selectedBoneIdx ).c_str() );
                }

                ImGui::NewLine();
                ImGuiX::TextSeparator( "Local Transform" );
                Transform const& localBoneTransform = pSkeleton->GetLocalReferencePose()[selectedBoneIdx];
                DrawTransform( localBoneTransform );

                ImGui::NewLine();
                ImGuiX::TextSeparator( "Global Transform" );
                Transform const& globalBoneTransform = pSkeleton->GetGlobalReferencePose()[selectedBoneIdx];
                DrawTransform( globalBoneTransform );
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void SkeletonWorkspace::DrawSkeletonPreview()
    {
        EE_ASSERT( IsResourceLoaded() );

        auto drawingCtx = GetDrawingContext();

        // Draw skeleton
        Skeleton const* pSkeleton = m_workspaceResource.GetPtr();
        drawingCtx.Draw( *pSkeleton, Transform::Identity );

        // Draw selected bone
        if ( m_selectedBoneID.IsValid() )
        {
            int32_t const boneIdx = pSkeleton->GetBoneIndex( m_selectedBoneID );
            if ( boneIdx != InvalidIndex )
            {
                Transform const globalBoneTransform = pSkeleton->GetBoneGlobalTransform( boneIdx );
                drawingCtx.DrawAxis( globalBoneTransform, 0.25f, 3.0f );

                Vector textLocation = globalBoneTransform.GetTranslation();
                Vector const textLineLocation = textLocation - Vector( 0, 0, 0.01f );
                drawingCtx.DrawTextBox3D( textLocation, m_selectedBoneID.c_str(), Colors::Lime );
            }
        }
    }

    // Bone Masks
    //-------------------------------------------------------------------------

    void SkeletonWorkspace::StartEditingMask( StringID maskID )
    {
        EE_ASSERT( maskID.IsValid() );
        EE_ASSERT( m_editedMaskIdx == InvalidIndex );

        BoneMaskDefinition* pEditedBoneMaskDefinition = nullptr;
        auto pDescriptor = GetDescriptor<SkeletonResourceDescriptor>();
        for ( auto i = 0; i < pDescriptor->m_boneMaskDefinitions.size(); i++ )
        {
            if ( pDescriptor->m_boneMaskDefinitions[i].m_ID == maskID )
            {
                pEditedBoneMaskDefinition = &pDescriptor->m_boneMaskDefinitions[i];
                m_editedMaskIdx = i;
                break;
            }
        }

        EE_ASSERT( pEditedBoneMaskDefinition != nullptr );

        // Reset edited weights
        //-------------------------------------------------------------------------

        Skeleton const* pSkeleton = m_workspaceResource.GetPtr();
        auto const numBones = pSkeleton->GetNumBones();
        m_editedBoneWeights.resize( numBones );
        for ( auto boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            m_editedBoneWeights[boneIdx] = -1.0f;
        }

        // Transfer definition weights
        //-------------------------------------------------------------------------

        for ( auto const& boneWeight : pEditedBoneMaskDefinition->m_weights )
        {
            int32_t const boneIdx = pSkeleton->GetBoneIndex( boneWeight.m_boneID );
            EE_ASSERT( boneIdx != InvalidIndex );
            m_editedBoneWeights[boneIdx] = boneWeight.m_weight;
        }

        //-------------------------------------------------------------------------

        UpdatePreviewBoneMask();
    }

    void SkeletonWorkspace::StopEditingMask()
    {
        m_editedMaskIdx = InvalidIndex;
    }

    void SkeletonWorkspace::CreateBoneMask()
    {
        EE_ASSERT( IsDescriptorLoaded() );

        ScopedDescriptorModification const sdm( this );
        auto pDescriptor = GetDescriptor<SkeletonResourceDescriptor>();
        pDescriptor->m_boneMaskDefinitions.emplace_back( StringID( "Bone Mask" ) );
        GenerateUniqueBoneMaskName( (int32_t) pDescriptor->m_boneMaskDefinitions.size() - 1 );
    }

    void SkeletonWorkspace::DestroyBoneMask( StringID maskID )
    {
        EE_ASSERT( IsDescriptorLoaded() );

        if ( IsEditingBoneMask() )
        {
            StopEditingMask();
        }

        //-------------------------------------------------------------------------

        ScopedDescriptorModification const sdm( this );
        auto pDescriptor = GetDescriptor<SkeletonResourceDescriptor>();
        for ( auto iter = pDescriptor->m_boneMaskDefinitions.begin(); iter != pDescriptor->m_boneMaskDefinitions.end(); ++iter )
        {
            if ( iter->m_ID == maskID )
            {
                pDescriptor->m_boneMaskDefinitions.erase( iter );
                return;
            }
        }

        EE_UNREACHABLE_CODE();
    }

    void SkeletonWorkspace::RenameBoneMask( StringID oldID, StringID newID )
    {
        EE_ASSERT( IsDescriptorLoaded() );

        ScopedDescriptorModification const sdm( this );
        auto pDescriptor = GetDescriptor<SkeletonResourceDescriptor>();
        for ( auto i = 0; i < pDescriptor->m_boneMaskDefinitions.size(); i++ )
        {
            if ( pDescriptor->m_boneMaskDefinitions[i].m_ID == oldID )
            {
                pDescriptor->m_boneMaskDefinitions[i].m_ID = newID;
                GenerateUniqueBoneMaskName( i );
                return;
            }
        }

        EE_UNREACHABLE_CODE();
    }

    void SkeletonWorkspace::SetWeight( int32_t boneIdx, float weight )
    {
        EE_ASSERT( IsValidWeight( weight ) );
        EE_ASSERT( IsDescriptorLoaded() );
        EE_ASSERT( IsEditingBoneMask() );

        ScopedDescriptorModification const sm( this );
        m_editedBoneWeights[boneIdx] = weight;

        ReflectWeightsToEditedBoneMask();
        UpdatePreviewBoneMask();
    }

    void SkeletonWorkspace::SetAllChildWeights( int32_t parentBoneIdx, float weight, bool bIncludeParent )
    {
        EE_ASSERT( IsValidWeight( weight ) );
        EE_ASSERT( IsDescriptorLoaded() );
        EE_ASSERT( IsEditingBoneMask() );

        ScopedDescriptorModification const sm( this );

        if ( bIncludeParent )
        {
            m_editedBoneWeights[parentBoneIdx] = weight;
        }

        Skeleton const* pSkeleton = m_workspaceResource.GetPtr();
        auto const numBones = pSkeleton->GetNumBones();
        for ( auto boneIdx = 1; boneIdx < numBones; boneIdx++ )
        {
            if ( pSkeleton->IsChildBoneOf( parentBoneIdx, boneIdx ) )
            {
                m_editedBoneWeights[boneIdx] = weight;
            }
        }

        ReflectWeightsToEditedBoneMask();
        UpdatePreviewBoneMask();
    }

    void SkeletonWorkspace::ReflectWeightsToEditedBoneMask()
    {
        EE_ASSERT( IsDescriptorLoaded() );
        EE_ASSERT( IsEditingBoneMask() );

        ScopedDescriptorModification const sm( this );
        auto pDescriptor = GetDescriptor<SkeletonResourceDescriptor>();
        auto pEditedBoneMaskDefinition = &pDescriptor->m_boneMaskDefinitions[m_editedMaskIdx];

        // Clear existing weights
        pEditedBoneMaskDefinition->m_weights.clear();

        // Reflect the current edited weights
        Skeleton const* pSkeleton = m_workspaceResource.GetPtr();
        auto const numBones = pSkeleton->GetNumBones();
        for ( auto boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            if ( m_editedBoneWeights[boneIdx] >= 0.0f )
            {
                pEditedBoneMaskDefinition->m_weights.emplace_back( pSkeleton->GetBoneID( boneIdx ), m_editedBoneWeights[boneIdx] );
            }
        }
    }

    void SkeletonWorkspace::UpdatePreviewBoneMask()
    {
        EE_ASSERT( IsEditingBoneMask() );
        Skeleton const* pSkeleton = m_workspaceResource.GetPtr();

        auto pDescriptor = GetDescriptor<SkeletonResourceDescriptor>();
        auto pEditedBoneMaskDefinition = &pDescriptor->m_boneMaskDefinitions[m_editedMaskIdx];
        m_previewBoneMask = BoneMask( pSkeleton, *pEditedBoneMaskDefinition );
    }

    void SkeletonWorkspace::DrawBoneMaskPreview()
    {
        EE_ASSERT( IsResourceLoaded() );
        EE_ASSERT( IsEditingBoneMask() );
        EE_ASSERT( m_previewBoneMask.IsValid() );

        //-------------------------------------------------------------------------

        auto drawingCtx = GetDrawingContext();

        Skeleton const* pSkeleton = m_workspaceResource.GetPtr();
        auto const& localRefPose = pSkeleton->GetLocalReferencePose();
        auto const& parentIndices = pSkeleton->GetParentBoneIndices();

        auto const numBones = localRefPose.size();
        if ( numBones > 0 )
        {
            TInlineVector<Transform, 256> globalTransforms;
            globalTransforms.resize( numBones );

            globalTransforms[0] = localRefPose[0];
            for ( auto i = 1; i < numBones; i++ )
            {
                auto const& parentIdx = parentIndices[i];
                auto const& parentTransform = globalTransforms[parentIdx];
                globalTransforms[i] = localRefPose[i] * parentTransform;
            }

            //-------------------------------------------------------------------------

            for ( auto boneIdx = 1; boneIdx < numBones; boneIdx++ )
            {
                auto const& parentIdx = parentIndices[boneIdx];
                auto const& parentTransform = globalTransforms[parentIdx];
                auto const& boneTransform = globalTransforms[boneIdx];

                Color const boneColor = GetColorForWeight( m_previewBoneMask.GetWeight( boneIdx ) );
                drawingCtx.DrawLine( boneTransform.GetTranslation().ToFloat3(), parentTransform.GetTranslation().ToFloat3(), boneColor, 5.0f );
            }
        }
    }

    void SkeletonWorkspace::DrawBoneMaskWindow( UpdateContext const& context, ImGuiWindowClass* pWindowClass )
    {
        StringID maskToDelete;

        //-------------------------------------------------------------------------

        ImGui::SetNextWindowClass( pWindowClass );
        if ( ImGui::Begin( m_boneMasksWindowName.c_str() ) )
        {
            auto pDescriptor = TryCast<SkeletonResourceDescriptor>( m_pDescriptor );

            ImGui::BeginDisabled( !IsDescriptorLoaded() );
            if ( ImGuiX::ColoredIconButton( ImGuiX::ImColors::Green, ImGuiX::ImColors::White, ImGuiX::ImColors::White, EE_ICON_PLUS, "Add New Mask", ImVec2( -1, 0 ) ) )
            {
                CreateBoneMask();
            }
            ImGui::EndDisabled();

            //-------------------------------------------------------------------------

            static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
            if ( ImGui::BeginTable( "BoneMasks", 2, flags, ImGui::GetContentRegionAvail() ) )
            {
                static ImVec2 const buttonSize( 30, 0 );

                ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_NoHide );
                ImGui::TableSetupColumn( "Controls", ImGuiTableColumnFlags_WidthFixed, ( buttonSize.x * 3 + ( ImGui::GetStyle().ItemSpacing.x * 2 ) ) );

                //-------------------------------------------------------------------------
                if ( pDescriptor != nullptr )
                {
                    for ( auto i = 0; i < pDescriptor->m_boneMaskDefinitions.size(); i++ )
                    {
                        auto const& boneMaskDefinition = pDescriptor->m_boneMaskDefinitions[i];
                        ImGui::PushID( &boneMaskDefinition );
                        ImGui::TableNextRow();

                        //-------------------------------------------------------------------------

                        ImGui::TableNextColumn();

                        ImGui::AlignTextToFramePadding();

                        bool const isSelected = ( m_editedMaskIdx != InvalidIndex ) ? ( m_editedMaskIdx == i ) : false;
                        ImGui::PushStyleColor( ImGuiCol_Text, isSelected ? ImGuiX::ImColors::Lime.Value : ImGuiX::Style::s_colorText );
                        InlineString const label( InlineString::CtorSprintf(), EE_ICON_DRAMA_MASKS" %s", boneMaskDefinition.m_ID.c_str() );
                        if ( ImGui::Selectable( label.c_str(), isSelected) )
                        {
                            if ( m_editedMaskIdx != i )
                            {
                                if ( IsEditingBoneMask() )
                                {
                                    StopEditingMask();
                                }

                                StartEditingMask( boneMaskDefinition.m_ID );
                            }
                        }
                        ImGui::PopStyleColor();

                        //-------------------------------------------------------------------------

                        if ( ImGui::BeginPopupContextItem( "MaskContextMenu" ) )
                        {
                            if ( ImGui::MenuItem( EE_ICON_IDENTIFIER" Copy Bone Mask ID" ) )
                            {
                                ImGui::SetClipboardText( boneMaskDefinition.m_ID.c_str() );
                            }

                            if ( ImGui::MenuItem( EE_ICON_SQUARE_EDIT_OUTLINE" Rename" ) )
                            {
                                EE::Printf( m_renameBuffer, 255, boneMaskDefinition.m_ID.c_str() );
                                m_activeOperation = OperationType::RenameMask;
                                m_operationID = boneMaskDefinition.m_ID;
                            }

                            if ( ImGui::MenuItem( EE_ICON_DELETE" Delete" ) )
                            {
                                m_activeOperation = OperationType::DeleteMask;
                                m_operationID = boneMaskDefinition.m_ID;
                            }

                            ImGui::EndPopup();
                        }

                        //-------------------------------------------------------------------------

                        ImGui::TableNextColumn();

                        if ( isSelected )
                        {
                            if ( ImGuiX::IconButton( EE_ICON_CHECK, "##StopEdit", ImGuiX::ImColors::Lime, buttonSize ) )
                            {
                                StopEditingMask();
                            }
                            ImGuiX::ItemTooltip( "Stop Editing Mask" );
                        }
                        else
                        {
                            ImGui::Dummy( buttonSize );
                        }

                        ImGui::SameLine();

                        if ( ImGui::Button( EE_ICON_SQUARE_EDIT_OUTLINE"##Rename", buttonSize ) )
                        {
                            if ( m_activeOperation == OperationType::None )
                            {
                                EE::Printf( m_renameBuffer, 255, boneMaskDefinition.m_ID.c_str() );
                                m_activeOperation = OperationType::RenameMask;
                                m_operationID = boneMaskDefinition.m_ID;
                            }
                        }
                        ImGuiX::ItemTooltip( "Rename" );

                        ImGui::SameLine();

                        if ( ImGui::Button( EE_ICON_DELETE"##Delete", buttonSize ) )
                        {
                            if ( m_activeOperation == OperationType::None )
                            {
                                m_activeOperation = OperationType::DeleteMask;
                                m_operationID = boneMaskDefinition.m_ID;
                            }
                        }
                        ImGuiX::ItemTooltip( "Delete" );

                        //-------------------------------------------------------------------------

                        ImGui::PopID();
                    }
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }

    void SkeletonWorkspace::ValidateDescriptorBoneMaskDefinitions()
    {
        EE_ASSERT( IsDescriptorLoaded() );
        EE_ASSERT( IsResourceLoaded() );

        bool fixupPerformed = false;

        //-------------------------------------------------------------------------

        Skeleton const* pSkeleton = m_workspaceResource.GetPtr();
        auto pDescriptor = GetDescriptor<SkeletonResourceDescriptor>();
        for ( auto& descriptorDefinition : pDescriptor->m_boneMaskDefinitions )
        {
            // Remove invalid weights;
            for ( auto i = 0; i < descriptorDefinition.m_weights.size(); i++ )
            {
                int32_t const boneIdx = pSkeleton->GetBoneIndex( descriptorDefinition.m_weights[i].m_boneID );
                if ( boneIdx == InvalidIndex )
                {
                    descriptorDefinition.m_weights.erase( descriptorDefinition.m_weights.begin() + i );
                    i--;
                    fixupPerformed = true;
                    continue;
                }

                //-------------------------------------------------------------------------

                if ( descriptorDefinition.m_weights[i].m_weight < 0.0f || descriptorDefinition.m_weights[i].m_weight > 1.0f )
                {
                    descriptorDefinition.m_weights[i].m_weight = Math::Clamp( descriptorDefinition.m_weights[i].m_weight, 0.0f, 1.0f );
                    fixupPerformed = true;
                }
            }

            // Check for unique name
            for ( auto i = 0; i < pDescriptor->m_boneMaskDefinitions.size(); i++ )
            {
                if ( &pDescriptor->m_boneMaskDefinitions[i] == &descriptorDefinition )
                {
                    continue;
                }

                // Duplicate name detected!
                if ( descriptorDefinition.m_ID == pDescriptor->m_boneMaskDefinitions[i].m_ID )
                {

                    fixupPerformed = true;
                }
            }
        }

        //-------------------------------------------------------------------------

        if ( fixupPerformed )
        {
            MarkDirty();
        }
    }

    void SkeletonWorkspace::GenerateUniqueBoneMaskName( int32_t boneMaskIdx )
    {
        // Do not used a scoped descriptor modification here as this function is used as part of the validation

        EE_ASSERT( IsDescriptorLoaded() );
        auto pDescriptor = GetDescriptor<SkeletonResourceDescriptor>();
        EE_ASSERT( boneMaskIdx >= 0 && boneMaskIdx < pDescriptor->m_boneMaskDefinitions.size() );

        InlineString desiredName = pDescriptor->m_boneMaskDefinitions[boneMaskIdx].m_ID.c_str();
        InlineString finalName = desiredName;

        uint32_t counter = 0;
        bool isUniqueName = false;
        while ( !isUniqueName )
        {
            isUniqueName = true;

            for ( auto i = 0; i < pDescriptor->m_boneMaskDefinitions.size(); i++ )
            {
                if ( boneMaskIdx == i )
                {
                    continue;
                }

                if ( finalName.comparei( pDescriptor->m_boneMaskDefinitions[i].m_ID.c_str()) == 0 )
                {
                    isUniqueName = false;
                    break;
                }
            }

            if ( !isUniqueName )
            {
                // Check if the last three characters are a numeric set, if so then increment the value and replace them
                if ( desiredName.length() > 3 && isdigit( desiredName[desiredName.length() - 1] ) && isdigit( desiredName[desiredName.length() - 2] ) && isdigit( desiredName[desiredName.length() - 3] ) )
                {
                    finalName.sprintf( "%s%03u", desiredName.substr( 0, desiredName.length() - 3 ).c_str(), counter );
                }
                else // Default name generation
                {
                    finalName.sprintf( "%s %03u", desiredName.c_str(), counter );
                }

                counter++;
            }
        }

        pDescriptor->m_boneMaskDefinitions[boneMaskIdx].m_ID = StringID( finalName.c_str() );
    }
}