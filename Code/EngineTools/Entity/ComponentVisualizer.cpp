#include "ComponentVisualizer.h"
#include "Engine/Entity/EntityComponent.h"
#include "EngineTools/Core/ToolsContext.h"
#include "Base/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE
{
    void ComponentVisualizer::UpdateVisualizedComponent( EntityComponent* pComponent )
    {
        EE_ASSERT( pComponent->GetTypeInfo()->IsDerivedFrom( GetSupportedType() ) );
        m_pComponent = pComponent;
    }
}