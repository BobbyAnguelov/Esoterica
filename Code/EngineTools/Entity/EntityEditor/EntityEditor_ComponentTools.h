#pragma once

#include "EngineTools/_Module/API.h"
#include "EntityEditor_EntityItem.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "Engine/Entity/EntityComponent.h"
#include "Base/Utils/GlobalRegistryBase.h"
#include "Base/TypeSystem/TypeID.h"
#include "Base/Types/Event.h"
#include "Base/Types/Color.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Viewport;
}

//-------------------------------------------------------------------------
// Component Editor
//-------------------------------------------------------------------------
// These classes provide additional visualization and in-viewport editing tool for components
// DO NOT DELETE the edited component from within the component editor!!!

namespace EE::EntityModel
{
    class EditorContext;

    //-------------------------------------------------------------------------
    // Component Editor Base
    //-------------------------------------------------------------------------

    class ComponentTools
    {

    public:

        ComponentTools() = default;
        virtual ~ComponentTools() = default;

        virtual EntityComponent const* GetEditedComponent() const = 0;
        virtual TypeSystem::TypeInfo const* GetRequiredComponentType() const = 0;

        // Viewport Selection
        virtual bool HasViewportSelectionWidget() const { return false; }
        virtual char const* GetViewportSelectionWidgetIcon() const { return ""; }
        virtual Color GetViewportSelectionWidgetColor() const { return Colors::White; }

        // Draw the viewport editor/visualization, return true if we are manipulating or interacting with this editor
        virtual bool DrawViewportEditor( EditorContext &context, Viewport const* pViewport, bool isFocused ) { return false; };
    };

    //-------------------------------------------------------------------------

    template<typename T>
    class TComponentTools : public ComponentTools
    {

    public:

        TComponentTools() = default;
        TComponentTools( T* pComponent ) : m_pComponent( pComponent ) {}

        virtual EntityComponent const* GetEditedComponent() const override { return m_pComponent; }
        virtual TypeSystem::TypeInfo const* GetRequiredComponentType() const override { return T::GetStaticTypeInfo(); }

    protected:

        T* m_pComponent = nullptr;
    };

    //-------------------------------------------------------------------------
    // Factory
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API ComponentToolsFactory : public TGlobalRegistryBase<ComponentToolsFactory>
    {
        EE_GLOBAL_REGISTRY( ComponentToolsFactory );

    public:

        virtual ~ComponentToolsFactory() = default;

        static ComponentTools* TryCreateTools( EditorContext &context, EntityComponent* pComponent );

        static void ForEachFactory( TFunction<void( ComponentToolsFactory const* )>&& fn );

    public:

        // Create a default instance of the component tools for global operations
        virtual EntityModel::ComponentTools* CreateDefaultInstance() const = 0;

    protected:

        // Get the type that that this factory can create an editor for
        virtual TypeSystem::TypeID GetSupportedTypeID() const = 0;

        // Virtual method that will create a component editor if the type ID matches the appropriate type
        virtual ComponentTools* TryCreateInternal( EditorContext &context, EntityComponent* pComponent ) const = 0;
    };

    //-------------------------------------------------------------------------
    //  Macro to create a factory
    //-------------------------------------------------------------------------
    // Use in a CPP to define a factory e.g., EE_COMPONENT_TOOLS( componentType, editorType );

    #define EE_COMPONENT_TOOLS( componentType, toolsClass )\
    class toolsClass##_##componentType##_##CEFactory final : public EntityModel::ComponentToolsFactory\
    {\
        virtual TypeSystem::TypeID GetSupportedTypeID() const override { return componentType::GetStaticTypeID(); }\
        virtual EntityModel::ComponentTools* CreateDefaultInstance() const { return EE::New<toolsClass>(); }\
        virtual EntityModel::ComponentTools* TryCreateInternal( EntityModel::EditorContext &context, EntityComponent* pComponent ) const override\
        {\
            EE_ASSERT( pComponent != nullptr );\
            EE_ASSERT( IsOfType<componentType>( pComponent ) );\
            return EE::New<toolsClass>( static_cast<componentType*>( pComponent ) );\
        }\
    };\
    static toolsClass##_##componentType##_##CEFactory g_##toolsClass##_##componentType##_##CEFactory;

    //-------------------------------------------------------------------------
    // Component Tools Manager
    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API ComponentToolsManager
    {
        struct ViewportWidgetLists
        {
            ViewportWidgetLists() = default;
            ViewportWidgetLists( ComponentTools const* pComponentTools );

        public:

            ComponentTools const*                       m_pComponentTools = nullptr;
            TypeSystem::TypeInfo const*                 m_pRequiredComponentTypeInfo = nullptr;
            TVector<EntityEditorItem>                   m_entityItems;
        };

    public:

        inline void Initialize( EditorContext &context );
        inline void Shutdown();
        inline bool IsInitialized() const { return m_pContext != nullptr; }

        inline bool IsManipulatingComponentEditor() const { return m_isManipulating; }

        void RegisterComponent( Entity* pEntity, EntityComponent* pComponent );
        void UnregisterComponent( Entity* pEntity, EntityComponent* pComponent );

        void Update();
        void DrawViewportEditors( Viewport const* pViewport, bool isFocused );
        void DrawViewportWidgets( Viewport const* pViewport, bool isFocused );

        void DestroyCreatedComponentTools();

    private:

        EditorContext*                                  m_pContext = nullptr;
        TVector<ComponentTools*>                        m_componentTools;
        EventBindingID                                  m_postWorldChangedEventID;
        bool                                            m_isManipulating = false;

        TVector<ComponentTools*>                        m_defaultToolInstances;
        TVector<ViewportWidgetLists>                    m_viewportWidgetLists;
    };
}