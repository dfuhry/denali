#ifndef DENALI_SIMPLIFY_H
#define DENALI_SIMPLIFY_H

#include <denali/graph_mixins.h>
#include <denali/graph_iterators.h>
#include <denali/folded.h>

#include <cmath>
#include <boost/shared_ptr.hpp>
#include <queue>
#include <stdexcept>
#include <vector>

namespace denali {

////////////////////////////////////////////////////////////////////////////////
//
// Priority
//
////////////////////////////////////////////////////////////////////////////////

class Priority
{
    double _priority;

public:
    Priority(double priority) : _priority(priority) {}

    double priority() const {
        return _priority;
    }
};

bool operator<(const Priority& lhs, const Priority& rhs) {
    return lhs.priority() < rhs.priority();
}

////////////////////////////////////////////////////////////////////////////////
//
// Persistence Simplifier
//
////////////////////////////////////////////////////////////////////////////////

class PersistenceSimplifier
{
    double _threshold;

    template <typename Node>
    class PersistencePriority : public Priority
    {
        double _persistence;
        Node _leaf;

    public:
        PersistencePriority(Node leaf, double persistence)
            : Priority(1/(persistence+1)), _persistence(persistence),
              _leaf(leaf) {}

        double persistence() const {
            return _persistence;
        }

        Node leaf() const {
            return _leaf;
        }
    };

    template <typename Tree>
    static double computePersistence(
            const Tree& tree, 
            typename Tree::Edge edge)
    {
        typename Tree::Node u = tree.u(edge);
        typename Tree::Node v = tree.v(edge);

        return abs(tree.getValue(u) - tree.getValue(v));
    }

    template <typename Tree>
    static typename Tree::Node getLeaf(const Tree& tree, typename Tree::Edge edge) {
        return tree.degree(tree.u(edge)) == 1 ? tree.u(edge) : tree.v(edge);
    }

public:
    PersistenceSimplifier(double threshold) {
        setThreshold(threshold);
    }

    double getThreshold() const {
        return _threshold;
    }

    void setThreshold(double threshold) {
        if (threshold <= 0) {
            throw std::runtime_error("Threshold must be positive.");
        }
        _threshold = threshold;
    }

    template <typename Tree>
    static unsigned int upDegree(const Tree& tree, typename Tree::Node node)
    {
        unsigned int n = 0;
        const double node_value = tree.getValue(node);

        for (UndirectedNeighborIterator<Tree> it(tree, node); !it.done(); ++it)
        {
            if (tree.getValue(it.neighbor()) > node_value) {
                n++;
            }
        }
        return n;
    }

    template <typename Tree>
    static unsigned int downDegree(const Tree& tree, typename Tree::Node node)
    {
        unsigned int n = 0;
        const double node_value = tree.getValue(node);

        for (UndirectedNeighborIterator<Tree> it(tree, node); !it.done(); ++it)
        {
            if (tree.getValue(it.neighbor()) <= node_value) {
                n++;
            }
        }
        return n;
    }

    template <typename Tree>
    static bool preserveForReduction(
            const Tree& tree, 
            typename Tree::Edge edge)
    {
        typename Tree::Node child = getLeaf(tree, edge);
        typename Tree::Node parent = tree.opposite(child, edge);

        double parent_value = tree.getValue(parent);
        double child_value = tree.getValue(child);

        if ((child_value <= parent_value) && (downDegree(tree, parent) == 1)) {
            return true;
        } else if ((child_value > parent_value) && (upDegree(tree, parent) == 1)) {
            return true;
        } else {
            return false;
        }
    }

    /// \brief Simplifies the contour tree in the context.
    template <typename Context>
    void simplify(Context& context)
    {
        typedef PersistencePriority<typename Context::Node> Priority;
        typedef typename Context::Node Node;
        typedef typename Context::Edge Edge;

        // make a priority queue of PersistencePriorities
        std::priority_queue<Priority> simplify_queue;

        // add every leaf edge to the queue
        for (EdgeIterator<Context> it(context); !it.done(); ++it)
        {
            Node u = context.u(it.edge());
            Node v = context.v(it.edge());

            if ((context.degree(u) == 1) || (context.degree(v) == 1)) 
            {
                // determine which is the leaf
                Node leaf = getLeaf(context, it.edge());

                // compute the persistence
                double persistence = computePersistence(context, it.edge());

                // enqueue
                simplify_queue.push(Priority(leaf, persistence));
            }
        }

        while (simplify_queue.size() > 0)
        {
            // get the leaf off of the queue
            Node leaf           = simplify_queue.top().leaf();
            double persistence  = simplify_queue.top().persistence();
            simplify_queue.pop();

            // get the leaf edge and the parent
            UndirectedNeighborIterator<Context> neighbor_it(context, leaf);
            Edge edge = neighbor_it.edge();
            Node parent = context.opposite(leaf, edge);

            if (preserveForReduction(context, edge)) {
                continue;
            }

            if (persistence > _threshold) {
                break;
            }

            // collapse the edge
            context.collapse(edge);

            // if the parent is reducible, reduce it now
            if (context.degree(parent) == 2) {
                context.reduce(parent);
            }
        }
    }
};




} // namespace denali

#endif
