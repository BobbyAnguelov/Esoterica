#pragma once

#include "API.h"
#include "Engine/Entity/EntityWorldManager.h"
#include "Base/Resource/ResourceID.h"
#include "Base/Input/InputSystem.h"
#include "Base/Render/RenderDevice.h"
#include "Base/Resource/ResourceSystem.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Drawing/DebugDrawing.h"
#include "Base/Systems.h"
#include "Base/Application/Module.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_GAME_API GameModule final : public Module
    {
        EE_REFLECT_MODULE;

    public:

        virtual bool InitializeModule( ModuleContext const& context ) override;
        virtual void ShutdownModule( ModuleContext const& context ) override;

    private:

        bool                                            m_moduleInitialized = false;
    };
}