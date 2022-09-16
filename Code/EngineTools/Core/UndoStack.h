#pragma once
#include "EngineTools/_Module/API.h"
#include "System/TypeSystem/RegisteredType.h"
#include "System/Types/Arrays.h"
#include "System/Types/Event.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINETOOLS_API IUndoableAction : public IRegisteredType
    {
        friend class UndoStack;

        EE_REGISTER_TYPE( IUndoableAction );

    public:

        virtual ~IUndoableAction() = default;

    protected:

        virtual void Undo() = 0;
        virtual void Redo() = 0;
    };

    //-------------------------------------------------------------------------

    class CompoundStackAction final : public IUndoableAction
    {
        friend class UndoStack;

        EE_REGISTER_TYPE( CompoundStackAction );

    public:

        virtual ~CompoundStackAction()
        {
            for ( auto& pAction : m_actions )
            {
                EE::Delete( pAction );
            }
        }

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
        inline bool CanUndo() { return !m_recordedActions.empty(); }

        // Undo the last action - returns the action that we undid
        IUndoableAction const* Undo();

        // Do we have an action we can redo
        inline bool CanRedo() { return !m_undoneActions.empty(); }

        // Redoes the last action - returns the action that we redid
        IUndoableAction const* Redo();

        //-------------------------------------------------------------------------

        // Register a new action, this transfers ownership of the action memory to the stack
        void RegisterAction( IUndoableAction* pAction );

        // Begin a compound action stack, all subsequent registered undoable actions will be grouped together until you end the compound action
        void BeginCompoundAction();

        // Ends a compound action stack
        void EndCompoundAction();

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