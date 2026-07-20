#include "EntityEditor_Dialogs.h"
#include "Engine/Entity/Entity.h"
#include "EntityEditor_Context.h"
#include "Base/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    RenameEntityDialog::RenameEntityDialog( EditorContext& editorContext, Entity* pEntity )
        : m_editorContext( editorContext )
        , m_pEntity( pEntity )
    {
        EE_ASSERT( m_pEntity != nullptr );
        EE_ASSERT( m_pEntity->GetNameID().IsValid() );
        m_buffer.Append( pEntity->GetNameID().c_str() );
    }

    bool RenameEntityDialog::Draw()
    {
        EntityMapID const& mapID = m_pEntity->GetMapID();
        EE_ASSERT( mapID.IsValid() );
        EntityMap* pMap = m_editorContext.GetEditedMap();
        EE_ASSERT( pMap != nullptr );

        //-------------------------------------------------------------------------

        auto ValidateName = [this, pMap] ()
        {
            bool isValidName = !m_buffer.Empty();
            if ( isValidName )
            {
                StringID const desiredNameID = StringID( m_buffer.GetString() );
                auto uniqueNameID = pMap->GenerateUniqueEntityNameID( desiredNameID );
                isValidName = ( desiredNameID == uniqueNameID );
            }

            return isValidName;
        };

        //-------------------------------------------------------------------------

        bool isDialogOpen = true;
        bool applyRename = false;
        bool isValidName = ValidateName();

        // Draw UI
        //-------------------------------------------------------------------------

        ImGui::AlignTextToFramePadding();
        ImGui::Text( "Name: " );
        ImGui::SameLine();

        ImGui::SetNextItemWidth( -1 );
        ImGui::PushStyleColor( ImGuiCol_Text, isValidName ? ImGuiX::Style::s_colorText : Colors::Red );
        if ( ImGui::InputText( "##Name", m_buffer.Data(), m_buffer.Size(), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCharFilter, ImGuiX::FilterNameIDChars ) )
        {
            applyRename = true;
            isDialogOpen = false;
        }

        isValidName = ValidateName();
        ImGui::PopStyleColor();

        ImGui::NewLine();
        ImGui::SameLine( ImGui::GetContentRegionAvail().x - 120 - ImGui::GetStyle().ItemSpacing.x );

        ImGui::BeginDisabled( !isValidName );
        if ( ImGui::Button( "OK", ImVec2( 60, 0 ) ) )
        {
            applyRename = true;
            isDialogOpen = false;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if ( ImGui::Button( "Cancel", ImVec2( 60, 0 ) ) )
        {
            isDialogOpen = false;
        }

        //-------------------------------------------------------------------------

        if ( applyRename && isValidName )
        {
            StringID const desiredNameID = StringID( m_buffer.GetString() );
            m_editorContext.RenameEntity( m_pEntity, desiredNameID );
            isDialogOpen = false;
        }

        return isDialogOpen;
    }

    //-------------------------------------------------------------------------

    RenameComponentDialog::RenameComponentDialog( EditorContext& editorContext, Entity* pEntity, EntityComponent* pComponent )
        : m_editorContext( editorContext )
        , m_pEntity( pEntity )
        , m_pComponent( pComponent )
    {
        EE_ASSERT( m_pEntity != nullptr );
        EE_ASSERT( m_pComponent != nullptr );
        EE_ASSERT( m_pComponent->GetNameID().IsValid() );
        m_buffer.Append( m_pComponent->GetNameID().c_str() );
    }

    bool RenameComponentDialog::Draw()
    {
        EE_ASSERT( m_pEntity != nullptr && m_pComponent != nullptr );

        //-------------------------------------------------------------------------

        auto ValidateName = [this] ()
        {
            bool isValidName = !m_buffer.Empty();
            if ( isValidName )
            {
                StringID const desiredNameID = StringID( m_buffer.GetString() );
                auto uniqueNameID = m_pEntity->GenerateUniqueComponentNameID( m_pComponent, desiredNameID );
                isValidName = ( desiredNameID == uniqueNameID );
            }

            return isValidName;
        };

        //-------------------------------------------------------------------------

        bool isDialogOpen = true;
        bool applyRename = false;
        bool isValidName = ValidateName();

        // Draw UI
        //-------------------------------------------------------------------------

        ImGui::AlignTextToFramePadding();
        ImGui::Text( "Name: " );
        ImGui::SameLine();

        ImGui::SetNextItemWidth( -1 );
        ImGui::PushStyleColor( ImGuiCol_Text, isValidName ? ImGuiX::Style::s_colorText : Colors::Red );
        applyRename = ImGui::InputText( "##Name", m_buffer.Data(), m_buffer.Size(), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCharFilter, ImGuiX::FilterNameIDChars );
        isValidName = ValidateName();
        ImGui::PopStyleColor();

        ImGui::NewLine();
        ImGui::SameLine( ImGui::GetContentRegionAvail().x - 120 - ImGui::GetStyle().ItemSpacing.x );

        ImGui::BeginDisabled( !isValidName );
        if ( ImGui::Button( "OK", ImVec2( 60, 0 ) ) )
        {
            applyRename = true;
            isDialogOpen = false;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if ( ImGui::Button( "Cancel", ImVec2( 60, 0 ) ) )
        {
            isDialogOpen = false;
        }

        //-------------------------------------------------------------------------

        if ( applyRename && isValidName )
        {
            StringID const desiredNameID = StringID( m_buffer.GetString() );
            m_editorContext.RenameComponent( m_pEntity, m_pComponent, desiredNameID );
            isDialogOpen = false;
        }

        return isDialogOpen;
    }

    //-------------------------------------------------------------------------

    CreateOrRenameGroupDialog::CreateOrRenameGroupDialog( EditorContext& editorContext, Type type, EntityMapID const& mapID, StringID groupOrBasePathID )
        : m_editorContext( editorContext )
        , m_mapID( mapID )
        , m_type( type )
    {
        EE_ASSERT( m_mapID.IsValid() );

        if ( m_type == Type::Create )
        {
            m_buffer.Append( "New Group" );

            if ( groupOrBasePathID.IsValid() )
            {
                m_basePath = groupOrBasePathID.c_str();

                if ( m_basePath.back() != '/' )
                {
                    m_basePath.append( "/" );
                }
            }
        }
        else
        {
            EE_ASSERT( groupOrBasePathID.IsValid() );
            m_groupID = groupOrBasePathID;
            m_basePath = groupOrBasePathID.c_str();
            EE_ASSERT( m_basePath.back() != '/' );

            size_t const basePathDelimiter = m_basePath.find_last_of( '/' );
            if ( basePathDelimiter != String::npos )
            {
                m_buffer.Append( &m_basePath[basePathDelimiter + 1] );
                m_basePath = m_basePath.substr( 0, basePathDelimiter + 1 );
            }
            else
            {
                m_buffer.Append( m_basePath.c_str() );
                m_basePath.clear();
            }
        }
    }

    bool CreateOrRenameGroupDialog::Draw()
    {
        EE_ASSERT( m_mapID.IsValid() );

        //-------------------------------------------------------------------------

        auto ValidateName = [this] ()
        {
            bool isValidName = !m_buffer.Empty();
            if ( isValidName )
            {
                StringID const desiredNameID = GetDesiredGroupName();
                isValidName = !m_editorContext.HasEntityGroup( m_mapID, desiredNameID );
            }

            return isValidName;
        };

        //-------------------------------------------------------------------------

        bool isDialogOpen = true;
        bool applyOperation = false;
        bool isValidName = ValidateName();

        // Draw UI
        //-------------------------------------------------------------------------

        ImGui::AlignTextToFramePadding();
        ImGui::Text( "Name: " );
        ImGui::SameLine();

        if ( !m_basePath.empty() )
        {
            ImGui::Text( m_basePath.c_str() );
            ImGui::SameLine();
        }

        ImGui::SetNextItemWidth( -1 );
        ImGui::PushStyleColor( ImGuiCol_Text, isValidName ? ImGuiX::Style::s_colorText : Colors::Red );
        applyOperation = ImGui::InputText( "##Name", m_buffer.Data(), m_buffer.Size(), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCharFilter, ImGuiX::FilterPathChars );
        isValidName = ValidateName();
        ImGui::PopStyleColor();

        ImGui::NewLine();
        ImGui::SameLine( ImGui::GetContentRegionAvail().x - 120 - ImGui::GetStyle().ItemSpacing.x );

        ImGui::BeginDisabled( !isValidName );
        if ( ImGui::Button( "OK", ImVec2( 60, 0 ) ) )
        {
            applyOperation = true;
            isDialogOpen = false;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if ( ImGui::Button( "Cancel", ImVec2( 60, 0 ) ) )
        {
            isDialogOpen = false;
        }

        //-------------------------------------------------------------------------

        if ( applyOperation && isValidName )
        {
            StringID const desiredNameID = GetDesiredGroupName();

            if ( m_type == Type::Rename )
            {
                m_editorContext.RenameEntityGroup( m_mapID, m_groupID, desiredNameID );
            }
            else
            {
                m_editorContext.CreateGroup( m_mapID, desiredNameID );
            }

            isDialogOpen = false;
        }

        return isDialogOpen;
    }

    StringID CreateOrRenameGroupDialog::GetDesiredGroupName()
    {
        InlineString str;
        if ( !m_basePath.empty() )
        {
            str.append( m_basePath.c_str() );
        }

        if ( !m_buffer.Empty() )
        {
            str.append( m_buffer.Data() );
        }

        StringUtils::ReplaceAllOccurrencesInPlace( str, "\\", "/" );

        return str.empty() ? StringID() : StringID( str.c_str() );
    }
}