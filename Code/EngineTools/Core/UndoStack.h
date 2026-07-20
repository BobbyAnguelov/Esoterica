#pragma once
#include "EngineTools/_Module/API.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/Event.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINETOOLS_API IUndoableAction : public IReflectedType
    {
        friend class UndoStack;

        EE_REFLECT_TYPE( IUndoableAction );

    public:

        virtual ~IUndoableAction() = default;

    protected:

        virtual void Undo() = 0;
        virtual void Redo() = 0;
    };

    //-------------------------------------------------------------------------

    // Internal class used to track all active compound actions
    class CompoundStackAction final : public IUndoableAction
    {
        friend class UndoStack;

        EE_REFLECT_TYPE( CompoundStackAction );

    public:

        virtual ~CompoundStackAction()
        {
            for ( auto& pAction : m_actions )
            {
                EE::Delete( pAction );
            }
        }

        TVector<IUndoableAction*> const& GetActions() const { return m_actions; }

    private:

        inline void AddToStack( IUndoableAction* pAction )
        {
            EE_ASSERT( pAction != nullptr );
            m_actions.emplace_back( pAction );
        }

        virtual void Undo() override { EE_UNREACHABLE_CODE(); }
        virtual void Redo() override { EE_UNREACHABLE_CODE(); }

    private:

        TVector<IUndoableAction*> m_actions;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API UndoStack
    {
    public:

        enum class Operation : uint8_t
        {
            Undo,
            Redo
        };

    public:

        ~UndoStack();

        // Clear all registered actions
        void Reset();

        // Do we have an action to undo
        inline bool CanUndo() const { return !m_recordedActions.empty(); }

        // Undo the last action - returns the action that we undid
        IUndoableAction const* Undo();

        // Get the next action to be undone
        IUndoableAction const* GetNextUndoAction() const { return CanUndo() ? m_recordedActions.back() : nullptr; }

        // Do we have an action we can redo
        inline bool CanRedo() const { return !m_undoneActions.empty(); }

        // Redoes the last action - returns the action that we redid
        IUndoableAction const* Redo();

        // Get the next action to be redone
        IUndoableAction const* GetNextRedoAction() const { return CanRedo() ? m_undoneActions.back() : nullptr; }

        // Get the action for a given operation
        IUndoableAction const* GetActionForOperation( Operation operation ) const { return ( operation == Operation::Redo ) ? GetNextRedoAction() : GetNextUndoAction(); }

        //-------------------------------------------------------------------------

        // Register a new action, this transfers ownership of the action memory to the stack
        void RegisterAction( IUndoableAction* pAction );

        // Begin a compound action stack, all subsequent registered undoable actions will be grouped together until you end the compound action
        void BeginCompoundAction();

        // Ends a compound action stack
        void EndCompoundAction();

        // Is a compound action currently active
        inline bool IsCompoundActionActive() const { return m_pCompoundStackAction != nullptr; }

        //-------------------------------------------------------------------------

        // Fired before we execute an undo/redo action
        TEventHandle<UndoStack::Operation, IUndoableAction const*> OnPreUndoRedo() { return m_preUndoRedoEvent; }

        // Fired after we execute an undo/redo action
        TEventHandle<UndoStack::Operation, IUndoableAction const*> OnPostUndoRedo() { return m_postUndoRedoEvent; }

        // Fired whenever we perform an action (register, undo, redo)
        TEventHandle<> OnActionPerformed() { return m_actionPerformed; }

    private:

        void ExecuteRedo( IUndoableAction* pAction ) const;
        void ExecuteUndo( IUndoableAction* pAction ) const;

        void ClearUndoStack();
        void ClearRedoStack();

    private:

        TVector<IUndoableAction*>                                   m_recordedActions;
        TVector<IUndoableAction*>                                   m_undoneActions;
        TEvent<UndoStack::Operation, IUndoableAction const*>        m_preUndoRedoEvent;
        TEvent<UndoStack::Operation, IUndoableAction const*>        m_postUndoRedoEvent;
        TEvent<>                                                    m_actionPerformed;
        CompoundStackAction*                                        m_pCompoundStackAction = nullptr;
    };
}