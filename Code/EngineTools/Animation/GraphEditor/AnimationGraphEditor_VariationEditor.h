#pragma once
#include "System/Types/StringID.h"
#include "EngineTools/Resource/ResourceFilePicker.h"

//-------------------------------------------------------------------------

struct ImGuiWindowClass;
namespace EE { class UpdateContext; }
namespace EE::Resource { class ResourceDatabase; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class VariationHierarchy;
    class GraphEditorContext;

    //-------------------------------------------------------------------------

    class GraphVariationEditor final
    {
        enum class OperationType
        {
            None,
            Create,
            Rename,
            Delete
        };

    public:

        GraphVariationEditor( GraphEditorContext& editorContext );

        void UpdateAndDraw( UpdateContext const& context, ImGuiWindowClass* pWindowClass, char const* pWindowName );

    private:

        void StartCreate( StringID variationID );
        void StartRename( StringID variationID );
        void StartDelete( StringID variationID );

        void DrawVariationSelector();
        void DrawVariationTreeNode( VariationHierarchy const& variationHierarchy, StringID variationID );
        void DrawOverridesTable();
        void DrawDialogs();
        bool DrawVariationNameEditUI();

    private:

        GraphEditorContext&                 m_editorContext;
        StringID                            m_activeOperationVariationID;
        char                                m_buffer[255] = {0};
        Resource::ResourceFilePicker        m_resourcePicker;
        OperationType                       m_activeOperation = OperationType::None;
    };
}