#include "graphAlg.h"
#include "common.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

/**
 * GraphNode
 */
using sinc::GraphNode;
template<class T>
GraphNode<T>::GraphNode(const T* _content) : 
    index(NO_TARJAN_INDEX), lowLink(NO_TARJAN_LOW_LINK), onStack(false), fvsIdx(NO_FVS_INDEX), content(_content) {}

template<class T>
bool GraphNode<T>::operator==(const GraphNode &another) const {
    return *content == *(another.content);
}

template<class T>
size_t GraphNode<T>::hash() const {
    std::hash<T> hasher;
    return hasher(*content);
}

template<class T>
size_t std::hash<GraphNode<T>>::operator()(const GraphNode<T>& r) const {
    return r.hash();
}

template<class T>
size_t std::hash<GraphNode<T>*>::operator()(const GraphNode<T> *r) const {
    return r->hash();
}

template<class T>
bool std::equal_to<GraphNode<T>*>::operator()(const GraphNode<T> *r1, const GraphNode<T> *r2) const {
    return (*r1) == (*r2);
}

template<class T>
size_t std::hash<const GraphNode<T>>::operator()(const GraphNode<T>& r) const {
    return r.hash();
}

template<class T>
size_t std::hash<const GraphNode<T>*>::operator()(const GraphNode<T> *r) const {
    return r->hash();
}

template<class T>
bool std::equal_to<const GraphNode<T>*>::operator()(const GraphNode<T> *r1, const GraphNode<T> *r2) const {
    return (*r1) == (*r2);
}

/* Instantiate GraphNode<T> by the following types, so the linker would not be confused: */
template class GraphNode<int>;
template class std::hash<GraphNode<int>>;
template class std::hash<GraphNode<int>*>;
template class std::equal_to<GraphNode<int>*>;
template class GraphNode<sinc::Predicate>;
template class std::hash<GraphNode<sinc::Predicate>>;
template class std::hash<GraphNode<sinc::Predicate>*>;
template class std::equal_to<GraphNode<sinc::Predicate>*>;

/**
 * Tarjan
 */
using sinc::Tarjan;
template <class T>
Tarjan<T>::Tarjan(Tarjan<T>::graphType* _graph, bool _clearMarkers) : graph(_graph), index(0) {
    if (_clearMarkers) {
        clearMarkers();
    }
}

template <class T>
Tarjan<T>::~Tarjan() {
    for (nodeSetType* const& scc: result) {
        delete scc;
    }
}

template <class T>
void Tarjan<T>::clearMarkers() {
    for (const std::pair<const GraphNode<T>*, typename Tarjan<T>::nodeSetType*>& kv: *graph) {
        GraphNode<T>* source_node = (GraphNode<T>*)kv.first;
        source_node->index = NO_TARJAN_INDEX;
        source_node->lowLink = NO_TARJAN_LOW_LINK;
        source_node->onStack = false;
        for (GraphNode<T>* const& n: *(kv.second)) {
            n->index = NO_TARJAN_INDEX;
            n->lowLink = NO_TARJAN_LOW_LINK;
            n->onStack = false;
        }
    }
}

template <class T>
const typename Tarjan<T>::resultType& Tarjan<T>::run() {
    for (const std::pair<const GraphNode<T>*, typename Tarjan<T>::nodeSetType*>& kv: *graph) {
        if (NO_TARJAN_INDEX == kv.first->index) {
            strongConnect((GraphNode<T>*)kv.first);
        }
    }
    return result;
}

template <class T>
void Tarjan<T>::strongConnect(GraphNode<T>* node) {
    node->index = index;
    node->lowLink = index;
    node->onStack = true;
    index++;
    stack.push(node);

    typename graphType::const_iterator itr = graph->find(node);
    if (graph->end() != itr) {
        for (GraphNode<T>* const& neighbour: *(itr->second)) {
            if (NO_TARJAN_INDEX == neighbour->index) {
                strongConnect(neighbour);
                node->lowLink = std::min(node->lowLink, neighbour->lowLink);
            } else if (neighbour->onStack) {
                node->lowLink = std::min(node->lowLink, neighbour->index);
            }
        }
    }

    if (node->lowLink == node->index) {
        Tarjan<T>::nodeSetType* scc = new Tarjan<T>::nodeSetType();
        GraphNode<T>* top;
        do {
            top = stack.top();
            stack.pop();
            top->onStack = false;
            scc->emplace(top);
        } while (node != top);  // Compare pointer value is sufficient here, as there are no two node pointers pointing to equal nodes
                                // in the graph

        /* Return only non-trivial SCC */
        if (1 < scc->size()) {
            result.push_back(scc);
        } else if (graph->end() != itr && (itr->second->find(node) != itr->second->end())) {
            /* Include the single-node SCC if it contains a self-loop */
            result.push_back(scc);
        } else {
            delete scc;
        }
    }
}

/* Instantiate Tarjan<T> by the following types, so the linker would not be confused: */
template class Tarjan<int>;
template class Tarjan<sinc::Predicate>;

/**
 * FeedbackVertexSetSolver
 */
using sinc::FeedbackVertexSetSolver;
template <class T>
FeedbackVertexSetSolver<T>::FeedbackVertexSetSolver(
    const FeedbackVertexSetSolver<T>::graphType& graph, const FeedbackVertexSetSolver<T>::nodeSetType& scc
) : matrix(new int*[scc.size() + 1]), size(scc.size()) {
    for (int i = 0; i <= size; i++) {
        matrix[i] = new int[size + 1]{0};
    }

    /* Numeration the nodes */
    for (GraphNode<T>* const& node: scc) {
        node->fvsIdx = nodes.size();
        nodes.push_back(node);
    }

    /* Convert the SCC into a connection matrix */
    for (GraphNode<T>* const& node: nodes) {
        typename FeedbackVertexSetSolver<T>::graphType::const_iterator itr = graph.find(node);
        for (GraphNode<T>* const& successor: *(itr->second)) {
            if (NO_FVS_INDEX != successor->fvsIdx) {
                matrix[node->fvsIdx][successor->fvsIdx] = 1;
                matrix[node->fvsIdx][size]++;
                matrix[size][successor->fvsIdx]++;
            }
        }
    }

    /* Cancel the numeration on the nodes */
    for (GraphNode<T>* const& node: nodes) {
        node->fvsIdx = NO_FVS_INDEX;
    }
}

template <class T>
FeedbackVertexSetSolver<T>::~FeedbackVertexSetSolver() {
    for (int i = 0; i <= size; i++) {
        delete[] matrix[i];
    }
    delete[] matrix;
}

template <class T>
typename FeedbackVertexSetSolver<T>::nodeSetType* FeedbackVertexSetSolver<T>::run() {
    int edges = 0;
    for (int i = 0; i < size; i++) {
        edges += matrix[size][i];
    }
    nodeSetType* result = new nodeSetType();
    while (0 < edges) {
        /* Take the node with maximum in-degree x out-degree and remove the related cycle, until there is no cycle */
        int max_score = 0;
        int max_idx = -1;
        for (int i = 0; i < size; i++) {
            int score = matrix[i][size] * matrix[size][i];
            if (score > max_score) {
                max_score = score;
                max_idx = i;
            }
        }
        result->emplace(nodes[max_idx]);

        /* Remove the node from the SCC with related cycles */
        edges -= removeNode(max_idx);
        bool updated = true;
        while (updated) {
            updated = false;
            for (int i = 0; i < size; i++) {
                if ((0 == matrix[i][size]) != (0 == matrix[size][i])) {
                    edges -= removeNode(i);
                    updated = true;
                }
            }
        }
    }
    return result;
}

template <class T>
int FeedbackVertexSetSolver<T>::removeNode(int idx) {
    int removed_edges = matrix[idx][size] + matrix[size][idx] - matrix[idx][idx];
    for (int i = 0; i < size; i++) {
        if (1 == matrix[idx][i]) {
            matrix[idx][i] = 0;
            matrix[size][i]--;
        }
        if (1 == matrix[i][idx]) {
            matrix[i][idx] = 0;
            matrix[i][size]--;
        }
    }
    matrix[idx][size] = 0;
    matrix[size][idx] = 0;
    return removed_edges;
}

/* Instantiate FeedbackVertexSetSolver<T> by the following types, so the linker would not be confused: */
template class FeedbackVertexSetSolver<int>;
template class FeedbackVertexSetSolver<sinc::Predicate>;
