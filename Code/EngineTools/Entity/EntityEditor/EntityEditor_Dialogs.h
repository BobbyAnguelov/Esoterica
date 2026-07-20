#pragma once
#include "EngineTools/Core/DialogManager.h"
#include "Base/Imgui/ImguiTextBuffer.h"
#include "Base/Types/StringID.h"
#include "Engine/Entity/EntityIDs.h"

//-------------------------------------------------------------------------

namespace EE
{
    class UpdateContext;
    class Entity;
    class EntityComponent;
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EditorContext;
    class EntityMap;

    //-------------------------------------------------------------------------

    class RenameEntityDialog : public ModalDialog
    {

    public:

        RenameEntityDialog( EditorContext& editorContext, Entity* pEntity );

        virtual char const* GetTitle() const { return "Rename Entity"; }
        virtual bool Draw() override;
        virtual ImVec2 GetSize() const { return ImVec2( 500, 120 ); }

    private:

        EditorContext&      m_editorContext;
        Entity*             m_pEntity = nullptr;
        ImGuiX::TextBuffer  m_buffer;
    };

    //-------------------------------------------------------------------------

    class RenameComponentDialog : public ModalDialog
    {

    public:

        RenameComponentDialog( EditorContext& editorContext, Entity* pEntity, EntityComponent* pComponent );

        virtual char const* GetTitle() const { return "Rename Entity Component"; }
        virtual bool Draw() override;
        virtual ImVec2 GetSize() const { return ImVec2( 500, 120 ); }

    private:

        EditorContext&      m_editorContext;
        Entity*             m_pEntity = nullptr;
        EntityComponent*    m_pComponent = nullptr;
        ImGuiX::TextBuffer  m_buffer;
    };

    //-------------------------------------------------------------------------

    class CreateOrRenameGroupDialog : public ModalDialog
    {
    public:

        enum class Type
        {
            Create,
            Rename
        };

    public:

        CreateOrRenameGroupDialog( EditorContext& editorContext, Type type, EntityMapID const& mapID, StringID groupOrBasePathID );

        virtual char const* GetTitle() const { return "Rename Entity Group"; }
        virtual bool Draw() override;
        virtual ImVec2 GetSize() const { return ImVec2( 500, 120 ); }

    private:

        StringID GetDesiredGroupName();

    private:

        EditorContext&      m_editorContext;
        EntityMapID         m_mapID;
        String              m_basePath;
        StringID            m_groupID;
        ImGuiX::TextBuffer  m_buffer;
        Type                m_type;
    };
}
