#pragma once

#include "EngineTools/_Module/API.h"
#include "System/Imgui/ImguiX.h"
#include "System/TypeSystem/TypeInfo.h"
#include "System/TypeSystem/ReflectedType.h"
#include "System/Types/Event.h"

//-------------------------------------------------------------------------
// Property grid
//-------------------------------------------------------------------------
// Allows you to edit registered types

namespace EE
{
    class ToolsContext;

    namespace Resource { class ResourceDatabase; }

    namespace PG 
    {
        struct GridContext;
        class CategoryRow;
        class PropertyEditor;
        class TypeEditingRules;
        struct ScopedChangeNotifier;
    }

    namespace TypeSystem
    {
        class TypeRegistry;
        class PropertyInfo;
    }

    //-------------------------------------------------------------------------

    struct PropertyEditInfo
    {
        enum class Action
        {
            Invalid = -1,
            Edit,
            AddArrayElement,
            MoveArrayElement,
            RemoveArrayElement,
        };

    public:

        IReflectedType*                         m_pOwnerTypeInstance = nullptr; // The type being edited by the property grid
        IReflectedType*                         m_pTypeInstanceEdited = nullptr; // The internal type instance that we modified - this is generally gonna be the same the owner type instance
        TypeSystem::PropertyInfo const*         m_pPropertyInfo = nullptr; // The info about the property that we edited (the property info refers to the typeInstanceEdited)
        Action                                  m_action = Action::Invalid;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API PropertyGrid
    {
        friend PG::ScopedChangeNotifier;

    public:

        PropertyGrid( ToolsContext const* pToolsContext );
        ~PropertyGrid();

        void SetUserContext( void* pContext );

        // Set this grid to be read only
        void SetReadOnly( bool isReadOnly ) { m_isReadOnly = isReadOnly; }

        // Get the current edited type
        IReflectedType const* GetEditedType() const { return m_pTypeInstance; }

        // Get the current edited type
        IReflectedType* GetEditedType() { return m_pTypeInstance; }

        // Set the type instance to edit, will reset dirty status
        void SetTypeToEdit( IReflectedType* pTypeInstance );

        // Set the type instance to edit, will reset dirty status
        void SetTypeToEdit( nullptr_t );

        // Display the grid
        void DrawGrid();

        // Has the type been modified
        inline bool IsDirty() const { return m_isDirty; }

        // Clear the modified flag i.e. when changes have been committed/saved
        void ClearDirty() { m_isDirty = false; }

        // Manually flag the grid as dirty
        void MarkDirty() { m_isDirty = true; }

        // Expand all properties
        void ExpandAllPropertyViews();

        // Collapse all properties
        void CollapseAllPropertyViews();

        // Should the control bar be visible?
        inline void SetControlBarVisible( bool isVisible ) { m_isControlBarVisible = isVisible; }

        //-------------------------------------------------------------------------

        // Event fired just before a property is modified
        inline TEventHandle<PropertyEditInfo const&> OnPreEdit() { return m_preEditEvent; }

        // Event fired just after a property was modified
        inline TEventHandle<PropertyEditInfo const&> OnPostEdit() { return m_postEditEvent; }

    private:

        void RebuildGrid();
        void ApplyFilter();

    private:

        PG::GridContext*                                            m_pGridContext = nullptr;
        TypeSystem::TypeInfo const*                                 m_pTypeInfo = nullptr;
        IReflectedType*                                             m_pTypeInstance = nullptr;
        bool                                                        m_isDirty = false;
        bool                                                        m_isControlBarVisible = true;
        bool                                                        m_showReadOnlyProperties = false;
        bool                                                        m_isReadOnly = false;
        ImGuiX::FilterWidget                                        m_filterWidget;

        TInlineVector<PG::CategoryRow*, 10>                         m_categories;

        TEvent<PropertyEditInfo const&>                             m_preEditEvent; // Fired just before we change a property value
        TEvent<PropertyEditInfo const&>                             m_postEditEvent; // Fired just after we change a property value
    };
}

//-------------------------------------------------------------------------

namespace EE::PG
{
    class GridRow
    {
    public:

        GridRow( GridRow* pParentRow, GridContext const& context, bool isDeclaredReadOnly = false ) : m_pParent( pParentRow ), m_context( context ), m_isDeclaredReadOnly( isDeclaredReadOnly ) {}
        GridRow( GridRow* pParentRow, GridContext const& context, String const& name, bool isDeclaredReadOnly = false ) : m_pParent( pParentRow ), m_name( name ), m_context( context ), m_isDeclaredReadOnly( isDeclaredReadOnly ) {}
        virtual ~GridRow() = default;

        String const& GetName() const { return m_name; }
        inline bool IsHidden() const { return m_isHidden; }
        inline bool IsReadOnly() const { return m_isDeclaredReadOnly || m_isReadOnly; }

        GridRow* GetParent() const { return m_pParent; }
        void SetParent( GridRow* pNewParent ) { m_pParent = pNewParent; }

        TVector<GridRow*> const& GetChildren() const { return m_children; }
        void DestroyChildren();

        // Update this rows and its children - note: children are updating before parents
        void UpdateRow();

        // Draw this row and it's children
        void DrawRow( float currentHeaderOffset );

        // Set whether we are expanded (i.e. drawing our children or not)
        void SetExpansion( bool isExpanded );

        // Set whether this row is hidden. Note: This doesn't apply to its children!
        void SetHidden( bool isHidden ) { m_isHidden = isHidden; }

        // Override this to provide additional rules for whether we should draw this row
        virtual bool ShouldDrawRow() const { return !m_isHidden; }

        // Helpers
        //-------------------------------------------------------------------------

        // Apply some operation to this and all child rows
        inline void RecursiveOperation( TFunction<void( GridRow* pRow )> const& function )
        {
            function( this );

            for ( auto& pChild : m_children )
            {
                function( pChild );
                pChild->RecursiveOperation( function );
            }
        }

    protected:

        virtual void Update() {}

        virtual void DrawHeaderSection( float currentHeaderOffset ) {}
        virtual void DrawEditorSection() {}

        virtual bool HasExtraControls() const { return false; }
        virtual float GetExtraControlsSectionWidth() const { return 0; }
        virtual void DrawExtraControlsSection() {}

        virtual bool HasResetSection() const { return false; }
        virtual void DrawResetSection() {}

    protected:

        GridRow*            m_pParent = nullptr;
        TVector<GridRow*>   m_children;
        String              m_name;
        GridContext const&  m_context;
        bool const          m_isDeclaredReadOnly = false; // Is this property EXPLICITY marked as read-only?

        bool                m_isReadOnly = false; // Some time properties need to be declared readonly based on the helper logic
        bool                m_isExpanded = true; // Is the row expanded, i.e. do we draw our children
        bool                m_isHidden = false; // Should we draw this row, only applies to this row and not its children
    };

    //-------------------------------------------------------------------------

    class CategoryRow : public GridRow
    {
    public:

        static CategoryRow* FindOrCreateCategory( GridRow* pParentRow, GridContext const& context, TInlineVector<CategoryRow*, 10>& categories, String const& categoryName );

    public:

        CategoryRow( GridRow* pParentRow, GridContext const& context, String const& name );

        void AddProperty( IReflectedType* pTypeInstance, TypeSystem::PropertyInfo const& propertyInfo );

    private:

        virtual bool ShouldDrawRow() const override;
        virtual void DrawHeaderSection( float currentHeaderOffset ) override;
    };

    //-------------------------------------------------------------------------

    class ArrayRow : public GridRow
    {
        enum class OperationType
        {
            None,
            Insert,
            MoveUp,
            MoveDown,
            Remove
        };

    public:

        ArrayRow( GridRow* pParentRow, GridContext const& context, TypeSystem::PropertyInfo const& propertyInfo, IReflectedType* pParentTypeInstance );

        // Array Operations
        void InsertElement( int32_t insertIndex );
        void MoveElementUp( int32_t arrayElementIndex );
        void MoveElementDown( int32_t arrayElementIndex );
        void DestroyElement( int32_t arrayElementIndex );

    private:

        virtual bool ShouldDrawRow() const override;

        virtual void Update() override;

        virtual void DrawHeaderSection( float currentHeaderOffset ) override;
        virtual void DrawEditorSection() override;

        virtual bool HasExtraControls() const override;
        virtual float GetExtraControlsSectionWidth() const override;
        virtual void DrawExtraControlsSection() override;

        virtual bool HasResetSection() const override;
        virtual void DrawResetSection() override;

        void RebuildChildren();

    private:

        IReflectedType*                     m_pParentTypeInstance = nullptr;
        TypeSystem::PropertyInfo const&     m_propertyInfo;

        OperationType                       m_operationType = OperationType::None;
        int32_t                             m_operationElementIdx = InvalidIndex;
    };

    //-------------------------------------------------------------------------

    class PropertyRow : public GridRow
    {

    public:

        PropertyRow( GridRow* pParentRow, GridContext const& context, TypeSystem::PropertyInfo const& propertyInfo, IReflectedType* pParentTypeInstance, int32_t arrayElementIndex = InvalidIndex );
        ~PropertyRow();

    private:

        virtual bool ShouldDrawRow() const override;

        virtual void Update() override;

        virtual void DrawHeaderSection( float currentHeaderOffset ) override;
        virtual void DrawEditorSection() override;

        virtual bool HasExtraControls() const override;
        virtual float GetExtraControlsSectionWidth() const override;
        virtual void DrawExtraControlsSection() override;

        virtual bool HasResetSection() const override;
        virtual void DrawResetSection() override;

        void RebuildChildren();

        inline bool HasPropertyEditor() const { return m_pPropertyEditor != nullptr; }

    private:

        TypeSystem::PropertyInfo const&     m_propertyInfo;
        IReflectedType*                     m_pParentTypeInstance = nullptr; // Our direct parent type instance
        int32_t                             m_arrayElementIdx = InvalidIndex; // If we are an array element this will be set
        void*                               m_pPropertyInstance = nullptr; // Either a core type instance or a structure instance
        PropertyEditor*                     m_pPropertyEditor = nullptr; // A core type or custom property editor, if this is set we dont display struct children
        TypeEditingRules*                   m_pTypeEditingRules = nullptr; // Helper that allows for complex visibility/read-only rules for a given type
    };
}