#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Widgets/Pickers/DataPathPicker.h"
#include "EngineTools/Widgets/Pickers/ResourcePickers.h"
#include "EngineTools/Widgets/Pickers/TypeInfoPicker.h"
#include "Base/Imgui/ImguiX.h"
#include "Base/TypeSystem/TypeInfo.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/TypeSystem/PropertyPath.h"
#include "Base/Types/Event.h"

//-------------------------------------------------------------------------
// Property grid
//-------------------------------------------------------------------------
// Allows you to edit registered types

namespace EE
{
    class ToolsContext;
    class FileRegistry;

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

        constexpr static float const s_absoluteMinimumHeight = 100;

        struct VisualState
        {
            inline void Clear()
            {
                m_editedTypeID = TypeSystem::TypeID();
                m_expandedCategories.clear();
                m_expandedPaths.clear();
                m_scrollPosY = 0;
            }

        public:

            TypeSystem::TypeID                      m_editedTypeID;
            TVector<String>                         m_expandedCategories;
            TVector<TypeSystem::PropertyPath>       m_expandedPaths;
            float                                   m_scrollPosY = 0;
        };

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
        void SetTypeToEdit( IReflectedType* pTypeInstance, VisualState const* pVisualStateToRestore = nullptr );

        // Display the grid
        void UpdateAndDraw();

        // Has the type been modified
        inline bool IsDirty() const { return m_isDirty; }

        // Clear the modified flag i.e. when changes have been committed/saved
        void ClearDirty() { m_isDirty = false; }

        // Manually flag the grid as dirty
        void MarkDirty() { m_isDirty = true; }

        // Visual Settings
        //-------------------------------------------------------------------------

        // Expand all properties
        void ExpandAll();

        // Collapse all properties
        void CollapseAll();

        // Get the current expansion state for the entire grid
        void GetCurrentVisualState( VisualState& outExpansionState );

        // Should the control bar be visible?
        inline void SetControlBarVisible( bool isVisible ) { m_isControlBarVisible = isVisible; }

        // Should the property grid fill all available vertical space?
        inline void SetFillAvailableVerticalSpace( bool shouldFillSpace ) { m_fillRemainingSpace = shouldFillSpace; }

        // Set the minimum height for the property grid, any values less than 100 will be clamped to s_absoluteMinimumHeight
        inline void SetMinimumHeight( float minimumHeight ) { EE_ASSERT( minimumHeight > 0 ); m_minimumHeight = Math::Max( s_absoluteMinimumHeight, minimumHeight ); }

        // Get the minimum height for the property grid
        inline float GetMinimumHeight() const { return m_minimumHeight; }

        // Events
        //-------------------------------------------------------------------------

        // Event fired just before a property is modified
        inline TEventHandle<PropertyEditInfo const&> OnPreEdit() { return m_preEditEvent; }

        // Event fired just after a property was modified
        inline TEventHandle<PropertyEditInfo const&> OnPostEdit() { return m_postEditEvent; }

    private:

        PropertyGrid( PropertyGrid const& ) = delete;
        PropertyGrid& operator=( PropertyGrid const& rhs ) = delete;

        // Rebuild the grid (optionally maintain expansion and scroll position)
        void RebuildGrid( IReflectedType* pTypeInstance, VisualState const* pVisualStateToRestore );

        // Apply the user specified filter to rows
        void ApplyFilter();

    private:

        PG::GridContext*                                            m_pGridContext = nullptr;
        TypeSystem::TypeInfo const*                                 m_pTypeInfo = nullptr;
        IReflectedType*                                             m_pTypeInstance = nullptr;
        bool                                                        m_isDirty = false;
        bool                                                        m_isReadOnly = false;
        ImGuiX::FilterWidget                                        m_filterWidget;

        float                                                       m_minimumHeight = 400.0f;
        float                                                       m_scrollPosY = 0;
        bool                                                        m_restoreScrollPosY = false;
        bool                                                        m_isControlBarVisible = true;
        bool                                                        m_showReadOnlyProperties = false;
        bool                                                        m_fillRemainingSpace = true;

        TInlineVector<PG::CategoryRow*, 10>                         m_categories;

        TEvent<PropertyEditInfo const&>                             m_preEditEvent; // Fired just before we change a property value
        TEvent<PropertyEditInfo const&>                             m_postEditEvent; // Fired just after we change a property value
    };
}

//-------------------------------------------------------------------------

namespace EE::PG
{
    struct PropertyChainElement
    {
        PropertyChainElement() = default;

        PropertyChainElement( IReflectedType* pTypeInstance, TypeSystem::PropertyInfo const* pPropertyInfo )
            : m_pTypeInstance( pTypeInstance )
            , m_pPropertyInfo( pPropertyInfo )
        {}

    public:

        IReflectedType*                     m_pTypeInstance = nullptr;
        TypeSystem::PropertyInfo const*     m_pPropertyInfo = nullptr;
    };

    //-------------------------------------------------------------------------

    class GridRow
    {
    public:

        GridRow( GridRow* pParentRow, GridContext const& context, bool isDeclaredReadOnly = false, bool isDeclaredHidden = false ) : m_pParent( pParentRow ), m_context( context ), m_isDeclaredReadOnly( isDeclaredReadOnly ), m_isDeclaredHidden( isDeclaredHidden ) {}
        GridRow( GridRow* pParentRow, GridContext const& context, String const& name, bool isDeclaredReadOnly = false, bool isDeclaredHidden = false ) : m_pParent( pParentRow ), m_name( name ), m_context( context ), m_isDeclaredReadOnly( isDeclaredReadOnly ), m_isDeclaredHidden( isDeclaredHidden ) {}
        virtual ~GridRow() = default;

        String const& GetName() const { return m_name; }
        TypeSystem::PropertyPath const& GetPath() const { return m_path; }

        inline bool IsHidden() const { return m_isDeclaredHidden || m_isHidden; }
        inline bool IsReadOnly() const { return m_isDeclaredReadOnly || m_isReadOnly; }
        inline bool IsExpanded() const { return m_isExpanded; }

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

        // Is this row declared read-only via metadata
        inline bool IsDeclaredReadOnly() const { return m_isDeclaredReadOnly; }

        // Is this row declared hidden via metadata
        inline bool IsDeclaredHidden() const { return m_isDeclaredHidden; }

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

        // Fill the expansion state from myself and my children
        void FillExpansionInfo( PropertyGrid::VisualState& expansionState );

        virtual void GeneratePropertyChangedNotificationChain( TVector<PropertyChainElement>& outChain ) const;

    protected:

        virtual void Update() {}

        virtual void UpdateName() {}

        virtual void DrawHeaderSection( float currentHeaderOffset ) {}
        virtual void DrawEditorSection() {}

        virtual bool HasExtraControls() const { return false; }
        virtual float GetExtraControlsSectionWidth() const { return 0; }
        virtual void DrawExtraControlsSection() {}

        virtual bool HasResetSection() const { return false; }
        virtual void DrawResetSection() {}

    protected:

        GridRow*                        m_pParent = nullptr;
        TVector<GridRow*>               m_children;
        String                          m_name;
        GridContext const&              m_context;
        TypeSystem::PropertyPath        m_path;
        bool const                      m_isDeclaredReadOnly = false; // Is this property EXPLICITLY marked as read-only?
        bool const                      m_isDeclaredHidden = false; // Is this property EXPLICITLY marked as hidden?

        bool                            m_isReadOnly = false; // Some time properties need to be declared read-only based on the helper logic
        bool                            m_isExpanded = true; // Is the row expanded, i.e. do we draw our children
        bool                            m_isHidden = false; // Should we draw this row, only applies to this row and not its children
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
            Remove,
            PushBack,
            RemoveAll,
            Reset,
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
        enum class OperationType
        {
            None,
            Update,
            Reset,
        };

    public:

        PropertyRow( GridRow* pParentRow, GridContext const& context, TypeSystem::PropertyInfo const& propertyInfo, IReflectedType* pParentTypeInstance, int32_t arrayElementIndex = InvalidIndex );
        ~PropertyRow();

    private:

        virtual bool ShouldDrawRow() const override;

        virtual void Update() override;
        virtual void UpdateName() override;

        virtual void DrawHeaderSection( float currentHeaderOffset ) override;
        virtual void DrawEditorSection() override;

        virtual bool HasExtraControls() const override;
        virtual float GetExtraControlsSectionWidth() const override;
        virtual void DrawExtraControlsSection() override;

        virtual bool HasResetSection() const override;
        virtual void DrawResetSection() override;

        void RebuildChildren();

        inline bool HasPropertyEditor() const { return m_pPropertyEditor != nullptr; }

        virtual void GeneratePropertyChangedNotificationChain( TVector<PropertyChainElement>& outChain ) const override;

    private:

        TypeSystem::PropertyInfo const&     m_propertyInfo;
        IReflectedType*                     m_pParentTypeInstance = nullptr; // Our direct parent type instance
        int32_t                             m_arrayElementIdx = InvalidIndex; // If we are an array element this will be set
        void*                               m_pPropertyInstance = nullptr; // Either a core type instance or a structure instance
        PropertyEditor*                     m_pPropertyEditor = nullptr; // A core type or custom property editor, if this is set we dont display struct children
        TypeEditingRules*                   m_pTypeEditingRules = nullptr; // Helper that allows for complex visibility/read-only rules for a given type
        OperationType                       m_operationType = OperationType::None;
    };
}