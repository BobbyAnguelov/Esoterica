#include "TopologicalSort.h"

//-------------------------------------------------------------------------

namespace EE
{
    static bool VisitNode( TVector<TopologicalSorter::Node>& sortedList, TopologicalSorter::Node& node )
    {
        if ( node.m_mark == TopologicalSorter::Node::Mark::Temporary )
        {
            return false;
        }

        if ( node.m_mark == TopologicalSorter::Node::Mark::None )
        {
            node.m_mark = TopologicalSorter::Node::Mark::Temporary;
            for ( auto pChild : node.m_children )
            {
                VisitNode( sortedList, *pChild );
            }
            node.m_mark = TopologicalSorter::Node::Mark::Permanent;
            sortedList.push_back( node );
        }

        return true;
    }

    bool TopologicalSorter::Sort( TVector<Node>& list )
    {
        TVector<Node> sortedList;
        auto const numNodes = (int32_t) list.size();

        for ( auto i = 0; i < numNodes; i++ )
        {
            if ( list[i].m_mark == Node::Mark::None )
            {
                if ( !VisitNode( sortedList, list[i] ) )
                {
                    return false; // list is not a DAG
                }

                i = InvalidIndex;
            }
        }

        list.swap( sortedList );
        return true;
    }
}
