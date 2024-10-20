#pragma once

#include "Animation_RuntimeGraph_SampledEvents.h"
#include "Animation_RuntimeGraph_ValueTypes.h"
#include "Animation_RuntimeGraph_Context.h"
#include "Animation_RuntimeGraph_Recording.h"
#include "Engine/Animation/AnimationSyncTrack.h"
#include "Engine/Animation/AnimationTarget.h"
#include "Base/TypeSystem/ReflectedType.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/Types/Color.h"
#include "Base/Time/Time.h"

//-------------------------------------------------------------------------

namespace EE::Drawing { class DrawContext; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphContext;
    struct BoneMaskTaskList;

    //-------------------------------------------------------------------------
    // Instantiation Context
    //-------------------------------------------------------------------------

    enum class InstantiationOptions
    {
        CreateNode,                 // Instruct the instantiate function to actually create the node
        NodeAlreadyCreated          // Informs the instantiate function that the node has been created (via derived class) and so it should only get it's ptrs set
    };

    struct InstantiationContext final
    {
        template<typename T>
        EE_FORCE_INLINE void SetNodePtrFromIndex( int16_t nodeIdx, T*& pTargetPtr ) const
        {
            EE_ASSERT( nodeIdx >= 0 && nodeIdx < m_nodePtrs.size() );
            pTargetPtr = reinterpret_cast<T*>( m_nodePtrs[nodeIdx] );
        }

        template<typename T>
        EE_FORCE_INLINE void SetNodePtrFromIndex( int16_t nodeIdx, T const*& pTargetPtr ) const
        {
            EE_ASSERT( nodeIdx >= 0 && nodeIdx < m_nodePtrs.size() );
            pTargetPtr = reinterpret_cast<T const*>( m_nodePtrs[nodeIdx] );
        }

        template<typename T>
        EE_FORCE_INLINE void SetOptionalNodePtrFromIndex( int16_t nodeIdx, T*& pTargetPtr ) const
        {
            if ( nodeIdx == InvalidIndex )
            {
                pTargetPtr = nullptr;
            }
            else
            {
                EE_ASSERT( nodeIdx >= 0 && nodeIdx < m_nodePtrs.size() );
                pTargetPtr = reinterpret_cast<T*>( m_nodePtrs[nodeIdx] );
            }
        }

        template<typename T>
        EE_FORCE_INLINE void SetOptionalNodePtrFromIndex( int16_t nodeIdx, T const*& pTargetPtr ) const
        {
            if ( nodeIdx == InvalidIndex )
            {
                pTargetPtr = nullptr;
            }
            else
            {
                EE_ASSERT( nodeIdx >= 0 && nodeIdx < m_nodePtrs.size() );
                pTargetPtr = reinterpret_cast<T const*>( m_nodePtrs[nodeIdx] );
            }
        }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void LogWarning( char const* pFormat, ... ) const;
        #endif

    public:

        int16_t                                     m_currentNodeIdx;
        TVector<GraphNode*> const&                  m_nodePtrs;
        TInlineVector<GraphInstance*, 20> const&    m_childGraphInstances;
        THashMap<StringID, int16_t> const&          m_parameterLookupMap;
        GraphDataSet const*                         m_pDataSet;
        uint64_t                                    m_userID;

        #if EE_DEVELOPMENT_TOOLS
        TVector<GraphLogEntry>*                     m_pLog;
        #endif
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API GraphNode
    {
        friend class PoseNode;
        friend class ValueNode;

    public:

        // This is the base for each node's individual definition
        // The definition are all shared for all graph instances since they are immutable, the nodes themselves contain the actual graph state
        struct EE_ENGINE_API Definition : public IReflectedType
        {
            EE_REFLECT_TYPE( Definition );

        public:

            Definition() = default;
            Definition( Definition const& ) = default;
            virtual ~Definition() = default;

            Definition& operator=( Definition const& rhs ) = default;

            // Factory method, will create the node instance and set all necessary node ptrs
            // NOTE!!! Node ptrs are not guaranteed to contain a constructed node so DO NOT ACCESS them in this function!!!
            virtual void InstantiateNode( InstantiationContext const& context, InstantiationOptions options ) const = 0;

            // Serialization methods
            virtual void Load( Serialization::BinaryInputArchive& archive );
            virtual void Save( Serialization::BinaryOutputArchive& archive ) const;

        protected:

            template<typename T>
            EE_FORCE_INLINE T* CreateNode( InstantiationContext const& context, InstantiationOptions options ) const
            {
                T* pNode = reinterpret_cast<T*>( context.m_nodePtrs[m_nodeIdx] );

                if ( options == InstantiationOptions::CreateNode )
                {
                    new ( pNode ) T();
                    pNode->m_pDefinition = this;
                }

                return pNode;
            }

        public:

            int16_t                      m_nodeIdx = InvalidIndex; // The index of this node in the graph, we currently only support graphs with max of 32k nodes
        };

    public:

        GraphNode() = default;
        GraphNode( GraphNode const& ) = default;
        virtual ~GraphNode();

        GraphNode& operator=( GraphNode const& rhs ) = default;

        // Node state management
        //-------------------------------------------------------------------------

        virtual bool IsValid() const { return true; }
        virtual GraphValueType GetValueType() const = 0;
        inline int16_t GetNodeIndex() const { return m_pDefinition->m_nodeIdx; }

        inline bool WasInitialized() const { return m_initializationCount > 0; }
        virtual void Initialize( GraphContext& context );
        void Shutdown( GraphContext& context );

        // Update
        //-------------------------------------------------------------------------

        // Is this node active i.e. was it updated this frame
        EE_FORCE_INLINE bool IsNodeActive( uint32_t updateID ) const { return m_lastUpdateID == updateID; }

        // Was this node updated this frame, this is syntactic sugar for value nodes
        EE_FORCE_INLINE bool WasUpdated( GraphContext& context ) const { return IsNodeActive( context.m_updateID ); }

        // Mark a node as being updated - value nodes will use this to cache values
        void MarkNodeActive( GraphContext& context );

        // Debugging
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState );
        virtual void RestoreGraphState( RecordedGraphState const& inState );
        #endif

    protected:

        virtual void InitializeInternal( GraphContext& context );
        virtual void ShutdownInternal( GraphContext& context );

        template<typename T>
        EE_FORCE_INLINE typename T::Definition const* GetDefinition() const
        {
            return reinterpret_cast<typename T::Definition const*>( m_pDefinition );
        }

    private:

        Definition const*                 m_pDefinition = nullptr;
        uint32_t                        m_lastUpdateID = 0xFFFFFFFF;
        int32_t                         m_initializationCount = 0;
    };

    //-------------------------------------------------------------------------
    // Animation Nodes
    //-------------------------------------------------------------------------

    struct GraphPoseNodeResult
    {
        EE_FORCE_INLINE bool HasRegisteredTasks() const { return m_taskIdx != InvalidIndex; }

    public:

        int8_t                          m_taskIdx = InvalidIndex;
        Transform                       m_rootMotionDelta = Transform::Identity;
        SampledEventRange               m_sampledEventRange;
    };

    #if EE_DEVELOPMENT_TOOLS
    struct PoseNodeDebugInfo
    {
        int32_t                         m_loopCount = 0;
        Seconds                         m_duration = 0.0f;
        Percentage                      m_currentTime = 0.0f;       // Clamped percentage over the duration
        Percentage                      m_previousTime = 0.0f;      // Clamped percentage over the duration
        SyncTrack const*                m_pSyncTrack = nullptr;
        SyncTrackTime                   m_currentSyncTime;
    };
    #endif

    class EE_ENGINE_API PoseNode : public GraphNode
    {

    public:

        // Get internal animation state
        virtual SyncTrack const& GetSyncTrack() const = 0;
        inline int32_t GetLoopCount() const { return m_loopCount; }
        inline Percentage const& GetPreviousTime() const { return m_previousTime; }
        inline Percentage const& GetCurrentTime() const { return m_currentTime; }
        inline Seconds GetDuration() const { return m_duration; }

        // Initialize an animation node with a specific start time
        void Initialize( GraphContext& context, SyncTrackTime const& initialTime = SyncTrackTime() );
        virtual void InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime );

        // Node update function
        // If the sync track update range is set, this will perform a synchronized update
        // If the sync track update range is not set, it will run unsynchronized and use the frame delta time instead
        virtual GraphPoseNodeResult Update( GraphContext& context, SyncTrackTimeRange const* pUpdateRange = nullptr ) = 0;

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        // Get the current state of the node
        PoseNodeDebugInfo GetDebugInfo() const;

        // Perform debug drawing for the pose node
        virtual void DrawDebug( GraphContext& graphContext, Drawing::DrawContext& drawCtx ) {}
        #endif

    protected:

        #if EE_DEVELOPMENT_TOOLS
        virtual void RecordGraphState( RecordedGraphState& outState ) override;
        virtual void RestoreGraphState( RecordedGraphState const& inState ) override;
        #endif

    private:

        virtual void Initialize( GraphContext& context ) override final { Initialize( context, SyncTrackTime() ); }
        virtual void InitializeInternal( GraphContext& context ) override final { Initialize( context, SyncTrackTime() ); }
        virtual GraphValueType GetValueType() const override final { return GraphValueType::Pose; }

    protected:

        int32_t                         m_loopCount = 0;
        Seconds                         m_duration = 0.0f;
        Percentage                      m_currentTime = 0.0f;       // Clamped percentage over the duration
        Percentage                      m_previousTime = 0.0f;      // Clamped percentage over the duration
    };

    //-------------------------------------------------------------------------
    // Value Nodes
    //-------------------------------------------------------------------------

    template<typename T> struct ValueTypeValidation { static GraphValueType const Type = GraphValueType::Unknown; };
    template<> struct ValueTypeValidation<bool> { static GraphValueType const Type = GraphValueType::Bool; };
    template<> struct ValueTypeValidation<StringID> { static GraphValueType const Type = GraphValueType::ID; };
    template<> struct ValueTypeValidation<float> { static GraphValueType const Type = GraphValueType::Float; };
    template<> struct ValueTypeValidation<Float3> { static GraphValueType const Type = GraphValueType::Vector; };
    template<> struct ValueTypeValidation<Target> { static GraphValueType const Type = GraphValueType::Target; };
    template<> struct ValueTypeValidation<BoneMaskTaskList const*> { static GraphValueType const Type = GraphValueType::BoneMask; };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API ValueNode : public GraphNode
    {
    public:

        template<typename T>
        EE_FORCE_INLINE T GetValue( GraphContext& context )
        {
            EE_ASSERT( ValueTypeValidation<T>::Type == GetValueType() );
            T value;
            GetValueInternal( context, &value );
            return value;
        }

        template<typename T>
        EE_FORCE_INLINE void SetValue( T const& outValue )
        {
            EE_ASSERT( ValueTypeValidation<T>::Type == GetValueType() );
            SetValueInternal( &outValue );
        }

    protected:

        virtual void GetValueInternal( GraphContext& context, void* pValue ) = 0;
        virtual void SetValueInternal( void const* pValue ) { EE_ASSERT( false ); };
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API BoolValueNode : public ValueNode
    {
        virtual GraphValueType GetValueType() const override final { return GraphValueType::Bool; }
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API IDValueNode : public ValueNode
    {
        virtual GraphValueType GetValueType() const override final { return GraphValueType::ID; }
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API FloatValueNode : public ValueNode
    {
        virtual GraphValueType GetValueType() const override final { return GraphValueType::Float; }
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API VectorValueNode : public ValueNode
    {
        virtual GraphValueType GetValueType() const override final { return GraphValueType::Vector; }
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API TargetValueNode : public ValueNode
    {
        virtual GraphValueType GetValueType() const override final { return GraphValueType::Target; }
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API BoneMaskValueNode : public ValueNode
    {
        virtual GraphValueType GetValueType() const override final { return GraphValueType::BoneMask; }
    };
}

//-------------------------------------------------------------------------

#define EE_SERIALIZE_GRAPHNODEDEFINITION( BaseClassTypename, ... ) \
virtual void Load( Serialization::BinaryInputArchive& archive ) override { BaseClassTypename::Load( archive ); archive.Serialize( __VA_ARGS__ ); }\
virtual void Save( Serialization::BinaryOutputArchive& archive ) const override { BaseClassTypename::Save( archive ); archive.Serialize( __VA_ARGS__ ); }
