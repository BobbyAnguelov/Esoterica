#pragma once
#include "EngineTools/Core/DialogManager.h"
#include "EngineTools/PropertyGrid/PropertyGrid.h"
#include "Base/FileSystem/FileSystemPath.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ToolsContext;
}

//-------------------------------------------------------------------------

namespace EE::Resource
{
    struct ResourceDescriptor;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API ResourceDataFileCreatorDialog : public ModalDialog
    {

    public:

        ResourceDataFileCreatorDialog( ToolsContext const* pToolsContext, TypeSystem::TypeID const descriptorTypeID, FileSystem::Path const& startingDir );
        ~ResourceDataFileCreatorDialog();

        virtual char const* GetTitle() const override { return "Create Descriptor/Data File"; }
        virtual bool IsResizeable() const override { return true; }
        virtual ImVec2 GetSize() const override { return ImVec2( 600, 800 ); }
        virtual bool Draw() override;

    private:

        void SaveDescriptor();

    private:

        ToolsContext const*                         m_pToolsContext = nullptr;
        IDataFile*                                  m_pDataFile = nullptr;
        PropertyGrid                                m_propertyGrid;
        FileSystem::Path                            m_startingPath;
        FileSystem::Extension                       m_extension;
        bool                                        m_showEditor = true;
    };
}