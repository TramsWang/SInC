#pragma once

#include <functional>
#include <stack>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#define NO_TARJAN_INDEX -1
#define NO_TARJAN_LOW_LINK -1
#define NO_FVS_INDEX -1

namespace sinc {
    /**
     * Base class for graph nodes. The information in base graph nodes is used for graph algorithms, such as Tarjan and Minimum
     * Feedback Vertex Set (MFVS). The class can be specified by a user defined content. The comparison between two graph
     * nodes are determined by the user defined content.
     *
     * @param <T> User defined content type
     *
     * @since 1.0
     */
    template<class T>
    class GraphNode {
    public:
        /* parameters for Tarjan */
        /** Tarjan index */
        int index;

        /** Tarjan lowLink */
        int lowLink;

        /** Tarjan onStack */
        bool onStack;

        /* parameters for fvs */
        /** FVS index */
        int fvsIdx;

        /**
         * User defined content.
         * 
         * NOTE: The content SHOULD be maintained by user.
         */
        const T* const content;

        /**
         * Initialize by the user defined content.
         * 
         * NOTE: The content SHOULD be maintained by user.
         */
        GraphNode(const T* content);

        /**
         * The equivalence comparison relies only on the user defined content.
         */
        bool operator==(const GraphNode &another) const;

        /**
         * The hashing relies only on the user defined content.
         */
        size_t hash() const;
    };
}

/**
 * This is for hashing "GraphNode<T>" in unordered containers.
 */
template<class T>
struct std::hash<sinc::GraphNode<T>> {
    size_t operator()(const sinc::GraphNode<T>& r) const;
};

template<class T>
struct std::hash<const sinc::GraphNode<T>> {
    size_t operator()(const sinc::GraphNode<T>& r) const;
};

/**
 * This is for hashing "GraphNode<T>*" in unordered containers.
 */
template<class T>
struct std::hash<sinc::GraphNode<T>*> {
    size_t operator()(const sinc::GraphNode<T> *r) const;
};

template<class T>
struct std::hash<const sinc::GraphNode<T>*> {
    size_t operator()(const sinc::GraphNode<T> *r) const;
};

/**
 * This is for checking equivalence "GraphNode<T>*" in unordered containers.
 */
template<class T>
struct std::equal_to<sinc::GraphNode<T>*> {
    bool operator()(const sinc::GraphNode<T> *r1, const sinc::GraphNode<T> *r2) const;
};

template<class T>
struct std::equal_to<const sinc::GraphNode<T>*> {
    bool operator()(const sinc::GraphNode<T> *r1, const sinc::GraphNode<T> *r2) const;
};

namespace sinc {
    /**
     * The Tarjan algorithm. The algorithm returns Strongly Connected Components (SCCs) that contains at least an edge. That
     * is, either the SCC contains more than one node or is a single node with self-loops.
     *
     * @param <T> The type of graph node.
     *
     * @since 1.0
     */
    template <class T>
    class Tarjan {
    public:
        typedef std::unordered_set<GraphNode<T>*> nodeSetType;
        typedef std::unordered_map<GraphNode<T>*, nodeSetType*> graphType;
        typedef std::vector<nodeSetType*> resultType;

        /**
         * Initialize the algorithm with a graph.
         *
         * @param graph The graph as a adjacent list.
         * @param clearMarkers Whether the marks on the nodes should be cleared before the algorithm runs. This parameter should
         *                   be true if the graph has undergone the Tarjan before. Otherwise, the output may be incorrect.
         */
        Tarjan(graphType* graph, bool clearMarkers);

        /**
         * The destructor releases resources for results
        */
        ~Tarjan();

        /**
         * Run the Tarjan algorithm.
         * 
         * NOTE: The returned results will be unavailable if the "Tarjan" object destructs.
         *
         * @return A list of strongly connected components (SCCs). An SCC is a set of graph nodes.
         */
        const resultType& run();

    protected:
        int index;

        /** NOTE: Pointers in this stack SHOULD be maintained by USER. */
        std::stack<GraphNode<T>*> stack;

        /** NOTE: Sets in this vector are maintained by the "Tarjan" object itself. */
        resultType result;

        /** NOTE: The pointer SHOULD be maintained by USER. */
        graphType* const graph;

        /**
         * Clear the markers on the graph nodes that are used by Tarjan algorithm.
         */
        void clearMarkers();

        void strongConnect(GraphNode<T>* node);
    };

    /**
     * The Minimum Feedback Vertex Set (MFVS) algorithm. The algorithm returns a set of graph nodes that all the cycles in a
     * Strongly Connected Component (SCC) are broken if the set of nodes are removed.
     *
     * @param <T> The type of graph node.
     *
     * @since 1.0
     */
    template <class T>
    class FeedbackVertexSetSolver {
    public:
        typedef std::unordered_set<GraphNode<T>*> nodeSetType;
        typedef std::unordered_map<GraphNode<T>*, nodeSetType*> graphType;

        /**
         * Initialize the solver by a graph and a SCC in the graph.
         *
         * @param graph A complete graph, in the form of a adjacent list
         * @param scc An SCC in the graph
         */
        FeedbackVertexSetSolver(const graphType& graph, const nodeSetType& scc);

        ~FeedbackVertexSetSolver();

        /**
         * Run the MFVS algorithm.
         * 
         * NOTE: The returned pointer SHOULD be maintained by USER.
         *
         * @return A set of graph nodes that breaks all the cycles in the SCC
         */
        nodeSetType* run();

    protected:
        /** The set of nodes in the SCC */
        std::vector<GraphNode<T>*> nodes;

        /** The connection matrix of the SCC */
        int** const matrix;

        /** The size of the SCC */
        int const size;

        /**
         * Remove the node at "idx" from the scc and return the number of edges removed with it.
         */
        int removeNode(int idx);
    };
}