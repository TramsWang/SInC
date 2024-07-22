#include <gtest/gtest.h>
#include "../../src/util/graphAlg.h"
#include <vector>

using namespace sinc;

class TarjanWithAppointedNodes : public Tarjan<int> {
public:
    TarjanWithAppointedNodes(graphType* graph, bool clearMarkers, std::vector<GraphNode<int>*>* _nodeList) :
        Tarjan(graph, clearMarkers), nodeList(_nodeList) {};

    ~TarjanWithAppointedNodes() {
        delete nodeList;
    };

    const resultType& run() {
        for (GraphNode<int>* const& node: (*nodeList)) {
            if (NO_TARJAN_INDEX == node->index) {
                strongConnect(node);
            }
        }
        return result;
    };

private:
    std::vector<GraphNode<int>*>* nodeList;
};

void releaseGraph(Tarjan<int>::graphType graph, GraphNode<int>** nodes, int length) {
    for (const std::pair<GraphNode<int>*, Tarjan<int>::nodeSetType*>& kv: graph) {
        delete kv.second;
    }
    for (int i = 0; i < length; i++) {
        delete nodes[i]->content;
        delete nodes[i];
    }
    delete[] nodes;
};

bool contains(std::unordered_set<GraphNode<int>*>* set, GraphNode<int>* node) {
    return set->find(node) != set->end();
}

TEST(TestGraphAlg, TestTarjanRun1) {
    const int node_cnt = 8;
    GraphNode<int>** nodes = new GraphNode<int>*[node_cnt];
    for (int i = 0; i < node_cnt; i++) {
        nodes[i] = new GraphNode<int>(new int(i));
    }
    Tarjan<int>::graphType graph;
    graph.emplace(nodes[0], new Tarjan<int>::nodeSetType({nodes[1], nodes[3]}));
    graph.emplace(nodes[1], new Tarjan<int>::nodeSetType({nodes[2], nodes[4]}));
    graph.emplace(nodes[2], new Tarjan<int>::nodeSetType({nodes[0]}));
    graph.emplace(nodes[3], new Tarjan<int>::nodeSetType({nodes[2]}));
    graph.emplace(nodes[4], new Tarjan<int>::nodeSetType({nodes[5], nodes[6]}));
    graph.emplace(nodes[5], new Tarjan<int>::nodeSetType({nodes[4]}));
    graph.emplace(nodes[6], new Tarjan<int>::nodeSetType({nodes[4]}));
    graph.emplace(nodes[7], new Tarjan<int>::nodeSetType());

    Tarjan<int> tarjan(&graph, false);
    Tarjan<int>::resultType sccs = tarjan.run();
    EXPECT_EQ(sccs.size(), 2);
    for (int i = 0; i < 2; i++) {
        Tarjan<int>::nodeSetType* scc = sccs[i];
        switch (scc->size()) {
            case 3:
                EXPECT_TRUE(contains(scc, nodes[4]));
                EXPECT_TRUE(contains(scc, nodes[5]));
                EXPECT_TRUE(contains(scc, nodes[6]));
                break;
            case 4:
                EXPECT_TRUE(contains(scc, nodes[0]));
                EXPECT_TRUE(contains(scc, nodes[1]));
                EXPECT_TRUE(contains(scc, nodes[2]));
                EXPECT_TRUE(contains(scc, nodes[3]));
                break;
            default:
                FAIL();
        }
    }
    releaseGraph(graph, nodes, node_cnt);
}

TEST(TestGraphAlg, TestTarjanRun2) {
    const int node_cnt = 24;
    GraphNode<int>** nodes = new GraphNode<int>*[node_cnt];
    for (int i = 0; i < node_cnt; i++) {
        nodes[i] = new GraphNode<int>(new int(i));
    }
    Tarjan<int>::graphType graph;
    graph.emplace(nodes[0], new Tarjan<int>::nodeSetType({nodes[8], nodes[14]}));
    graph.emplace(nodes[1], new Tarjan<int>::nodeSetType({nodes[8], nodes[15]}));
    graph.emplace(nodes[2], new Tarjan<int>::nodeSetType({nodes[9], nodes[16]}));
    graph.emplace(nodes[3], new Tarjan<int>::nodeSetType({nodes[10], nodes[17]}));
    graph.emplace(nodes[4], new Tarjan<int>::nodeSetType({nodes[11], nodes[18]}));
    graph.emplace(nodes[5], new Tarjan<int>::nodeSetType({nodes[11], nodes[19]}));
    graph.emplace(nodes[6], new Tarjan<int>::nodeSetType({nodes[12], nodes[20]}));
    graph.emplace(nodes[7], new Tarjan<int>::nodeSetType({nodes[13], nodes[21]}));
    graph.emplace(nodes[22], new Tarjan<int>::nodeSetType());
    graph.emplace(nodes[23], new Tarjan<int>::nodeSetType());

    Tarjan<int> tarjan(&graph, false);
    Tarjan<int>::resultType sccs = tarjan.run();
    EXPECT_EQ(sccs.size(), 0);
    releaseGraph(graph, nodes, node_cnt);
}

TEST(TestGraphAlg, TestTarjanRun3) {
    const int node_cnt = 3;
    GraphNode<int>** nodes = new GraphNode<int>*[node_cnt];
    for (int i = 0; i < node_cnt; i++) {
        nodes[i] = new GraphNode<int>(new int(i));
    }
    Tarjan<int>::graphType graph;
    graph.emplace(nodes[0], new Tarjan<int>::nodeSetType({nodes[1], nodes[2]}));

    Tarjan<int> tarjan(&graph, false);
    Tarjan<int>::resultType sccs = tarjan.run();
    EXPECT_EQ(sccs.size(), 0);
    releaseGraph(graph, nodes, node_cnt);
}

TEST(TestGraphAlg, TestTarjanRun4) {
    const int node_cnt = 2;
    GraphNode<int>** nodes = new GraphNode<int>*[node_cnt];
    for (int i = 0; i < node_cnt; i++) {
        nodes[i] = new GraphNode<int>(new int(i));
    }
    Tarjan<int>::graphType graph;
    graph.emplace(nodes[0], new Tarjan<int>::nodeSetType({nodes[0]}));
    graph.emplace(nodes[1], new Tarjan<int>::nodeSetType({nodes[1]}));

    Tarjan<int> tarjan(&graph, false);
    Tarjan<int>::resultType sccs = tarjan.run();
    EXPECT_EQ(sccs.size(), 2);
    EXPECT_EQ(sccs[0]->size(), 1);
    EXPECT_EQ(sccs[1]->size(), 1);
    if (sccs[0]->find(nodes[0]) != sccs[0]->end()) {
        EXPECT_TRUE(contains(sccs[1], nodes[1]));
    } else {
        EXPECT_TRUE(contains(sccs[0], nodes[1]));
        EXPECT_TRUE(contains(sccs[1], nodes[0]));
    }
    releaseGraph(graph, nodes, node_cnt);
}

TEST(TestGraphAlg, TestTarjanRun5) {
    const int node_cnt = 3;
    GraphNode<int>** nodes = new GraphNode<int>*[node_cnt];
    for (int i = 0; i < node_cnt; i++) {
        nodes[i] = new GraphNode<int>(new int(i));
    }
    Tarjan<int>::graphType graph;
    graph.emplace(nodes[0], new Tarjan<int>::nodeSetType({nodes[0]}));
    graph.emplace(nodes[1], new Tarjan<int>::nodeSetType({nodes[1], nodes[2]}));
    graph.emplace(nodes[2], new Tarjan<int>::nodeSetType({nodes[1]}));

    Tarjan<int> tarjan(&graph, false);
    Tarjan<int>::resultType sccs = tarjan.run();
    EXPECT_EQ(sccs.size(), 2);
    if (sccs[0]->size() == 1) {
        EXPECT_EQ(sccs[1]->size(), 2);
        EXPECT_TRUE(contains(sccs[0], nodes[0]));
        EXPECT_TRUE(contains(sccs[1], nodes[1]));
        EXPECT_TRUE(contains(sccs[1], nodes[2]));
    } else {
        EXPECT_EQ(sccs[0]->size(), 2);
        EXPECT_EQ(sccs[1]->size(), 1);
        EXPECT_TRUE(contains(sccs[0], nodes[1]));
        EXPECT_TRUE(contains(sccs[0], nodes[2]));
        EXPECT_TRUE(contains(sccs[1], nodes[0]));
    }
    releaseGraph(graph, nodes, node_cnt);
}

// TEST(TestGraphAlg, TestTarjanWithAppointedNodes) {
//     ;
// }

TEST(TestGraphAlg, TestTarjanAppointedStartPoint1) {
    const int node_cnt = 3;
    GraphNode<int>** nodes = new GraphNode<int>*[node_cnt];
    for (int i = 0; i < node_cnt; i++) {
        nodes[i] = new GraphNode<int>(new int(i));
    }
    Tarjan<int>::graphType graph;
    graph.emplace(nodes[0], new Tarjan<int>::nodeSetType({nodes[1]}));
    graph.emplace(nodes[1], new Tarjan<int>::nodeSetType({nodes[2]}));
    graph.emplace(nodes[2], new Tarjan<int>::nodeSetType({nodes[1]}));

    TarjanWithAppointedNodes tarjan(&graph, false, new std::vector<GraphNode<int>*>({nodes[0]}));
    Tarjan<int>::resultType sccs = tarjan.run();
    EXPECT_EQ(sccs.size(), 1);
    EXPECT_EQ(sccs[0]->size(), 2);
    EXPECT_TRUE(contains(sccs[0], nodes[1]));
    EXPECT_TRUE(contains(sccs[0], nodes[2]));
    releaseGraph(graph, nodes, node_cnt);
}

TEST(TestGraphAlg, TestTarjanAppointedStartPoint2) {
    const int node_cnt = 3;
    GraphNode<int>** nodes = new GraphNode<int>*[node_cnt];
    for (int i = 0; i < node_cnt; i++) {
        nodes[i] = new GraphNode<int>(new int(i));
    }
    Tarjan<int>::graphType graph;
    graph.emplace(nodes[0], new Tarjan<int>::nodeSetType({nodes[1]}));
    graph.emplace(nodes[1], new Tarjan<int>::nodeSetType({nodes[2]}));
    graph.emplace(nodes[2], new Tarjan<int>::nodeSetType({nodes[1]}));

    TarjanWithAppointedNodes tarjan(&graph, false, new std::vector<GraphNode<int>*>({nodes[1]}));
    Tarjan<int>::resultType sccs = tarjan.run();
    EXPECT_EQ(sccs.size(), 1);
    EXPECT_EQ(sccs[0]->size(), 2);
    EXPECT_TRUE(contains(sccs[0], nodes[1]));
    EXPECT_TRUE(contains(sccs[0], nodes[2]));
    releaseGraph(graph, nodes, node_cnt);
}

TEST(TestGraphAlg, TestTarjanAppointedStartPoint3) {
    const int node_cnt = 4;
    GraphNode<int>** nodes = new GraphNode<int>*[node_cnt];
    for (int i = 0; i < node_cnt; i++) {
        nodes[i] = new GraphNode<int>(new int(i));
    }
    Tarjan<int>::graphType graph;
    graph.emplace(nodes[0], new Tarjan<int>::nodeSetType({nodes[1]}));
    graph.emplace(nodes[1], new Tarjan<int>::nodeSetType({nodes[2]}));
    graph.emplace(nodes[2], new Tarjan<int>::nodeSetType({nodes[1], nodes[3]}));
    graph.emplace(nodes[3], new Tarjan<int>::nodeSetType({nodes[2]}));

    TarjanWithAppointedNodes tarjan(&graph, false, new std::vector<GraphNode<int>*>({nodes[0]}));
    Tarjan<int>::resultType sccs = tarjan.run();
    EXPECT_EQ(sccs.size(), 1);
    EXPECT_EQ(sccs[0]->size(), 3);
    EXPECT_TRUE(contains(sccs[0], nodes[1]));
    EXPECT_TRUE(contains(sccs[0], nodes[2]));
    EXPECT_TRUE(contains(sccs[0], nodes[3]));
    releaseGraph(graph, nodes, node_cnt);
}

TEST(TestGraphAlg, TestTarjanAppointedStartPoint4) {
    const int node_cnt = 6;
    GraphNode<int>** nodes = new GraphNode<int>*[node_cnt];
    for (int i = 0; i < node_cnt; i++) {
        nodes[i] = new GraphNode<int>(new int(i));
    }
    Tarjan<int>::graphType graph;
    graph.emplace(nodes[0], new Tarjan<int>::nodeSetType({nodes[1]}));
    graph.emplace(nodes[1], new Tarjan<int>::nodeSetType({nodes[2]}));
    graph.emplace(nodes[2], new Tarjan<int>::nodeSetType({nodes[1], nodes[3], nodes[4], nodes[5]}));
    graph.emplace(nodes[3], new Tarjan<int>::nodeSetType({nodes[2]}));

    TarjanWithAppointedNodes tarjan(&graph, false, new std::vector<GraphNode<int>*>({nodes[0]}));
    Tarjan<int>::resultType sccs = tarjan.run();
    EXPECT_EQ(sccs.size(), 1);
    EXPECT_EQ(sccs[0]->size(), 3);
    EXPECT_TRUE(contains(sccs[0], nodes[1]));
    EXPECT_TRUE(contains(sccs[0], nodes[2]));
    EXPECT_TRUE(contains(sccs[0], nodes[3]));
    releaseGraph(graph, nodes, node_cnt);
}

TEST(TestGraphAlg, TestTarjanAppointedStartPoint5) {
    const int node_cnt = 7;
    GraphNode<int>** nodes = new GraphNode<int>*[node_cnt];
    for (int i = 0; i < node_cnt; i++) {
        nodes[i] = new GraphNode<int>(new int(i));
    }
    Tarjan<int>::graphType graph;
    graph.emplace(nodes[0], new Tarjan<int>::nodeSetType({nodes[3]}));
    graph.emplace(nodes[1], new Tarjan<int>::nodeSetType({nodes[2]}));
    graph.emplace(nodes[2], new Tarjan<int>::nodeSetType({nodes[3]}));
    graph.emplace(nodes[3], new Tarjan<int>::nodeSetType({nodes[2], nodes[4], nodes[5], nodes[6]}));
    graph.emplace(nodes[4], new Tarjan<int>::nodeSetType({nodes[3]}));

    TarjanWithAppointedNodes tarjan(&graph, false, new std::vector<GraphNode<int>*>({nodes[1], nodes[0]}));
    Tarjan<int>::resultType sccs = tarjan.run();
    EXPECT_EQ(sccs.size(), 1);
    EXPECT_EQ(sccs[0]->size(), 3);
    EXPECT_TRUE(contains(sccs[0], nodes[2]));
    EXPECT_TRUE(contains(sccs[0], nodes[3]));
    EXPECT_TRUE(contains(sccs[0], nodes[4]));
    releaseGraph(graph, nodes, node_cnt);
}

TEST(TestGraphAlg, TestFvs1) {
    /* 2 vertices cycle */
    const int node_cnt = 2;
    GraphNode<int>** nodes = new GraphNode<int>*[node_cnt];
    for (int i = 0; i < node_cnt; i++) {
        nodes[i] = new GraphNode<int>(new int(i));
    }
    Tarjan<int>::graphType graph;
    graph.emplace(nodes[0], new Tarjan<int>::nodeSetType({nodes[1]}));
    graph.emplace(nodes[1], new Tarjan<int>::nodeSetType({nodes[0]}));
    Tarjan<int>::nodeSetType scc({nodes[0], nodes[1]});

    FeedbackVertexSetSolver<int> solver(graph, scc);
    FeedbackVertexSetSolver<int>::nodeSetType* cover = solver.run();

    EXPECT_EQ(cover->size(), 1);
    EXPECT_TRUE(contains(cover, nodes[0]) || contains(cover, nodes[1]));
    releaseGraph(graph, nodes, node_cnt);
    delete cover;
}

TEST(TestGraphAlg, TestFvs2) {
    /* 4 vertices cycle(with redundancy) */
    const int node_cnt = 7;
    GraphNode<int>** nodes = new GraphNode<int>*[node_cnt];
    for (int i = 0; i < node_cnt; i++) {
        nodes[i] = new GraphNode<int>(new int(i));
    }
    Tarjan<int>::graphType graph;
    graph.emplace(nodes[0], new Tarjan<int>::nodeSetType({nodes[1], nodes[4]}));
    graph.emplace(nodes[1], new Tarjan<int>::nodeSetType({nodes[2]}));
    graph.emplace(nodes[2], new Tarjan<int>::nodeSetType({nodes[3]}));
    graph.emplace(nodes[3], new Tarjan<int>::nodeSetType({nodes[0]}));
    graph.emplace(nodes[5], new Tarjan<int>::nodeSetType({nodes[3]}));
    graph.emplace(nodes[6], new Tarjan<int>::nodeSetType({nodes[3]}));
    Tarjan<int>::nodeSetType scc({nodes[0], nodes[1], nodes[2], nodes[3]});

    FeedbackVertexSetSolver<int> solver(graph, scc);
    FeedbackVertexSetSolver<int>::nodeSetType* cover = solver.run();

    EXPECT_EQ(cover->size(), 1);
    EXPECT_TRUE(contains(cover, nodes[0]) || contains(cover, nodes[1]) || contains(cover, nodes[2]) || contains(cover, nodes[3]));
    releaseGraph(graph, nodes, node_cnt);
    delete cover;
}

TEST(TestGraphAlg, TestFvs3) {
    /* 3 vertices cycle(with 2 self loops and redundancy) */
    const int node_cnt = 7;
    GraphNode<int>** nodes = new GraphNode<int>*[node_cnt];
    for (int i = 0; i < node_cnt; i++) {
        nodes[i] = new GraphNode<int>(new int(i));
    }
    Tarjan<int>::graphType graph;
    graph.emplace(nodes[0], new Tarjan<int>::nodeSetType({nodes[0], nodes[1]}));
    graph.emplace(nodes[1], new Tarjan<int>::nodeSetType({nodes[1], nodes[2]}));
    graph.emplace(nodes[2], new Tarjan<int>::nodeSetType({nodes[5], nodes[0]}));
    graph.emplace(nodes[3], new Tarjan<int>::nodeSetType({nodes[2]}));
    graph.emplace(nodes[4], new Tarjan<int>::nodeSetType({nodes[2]}));
    graph.emplace(nodes[5], new Tarjan<int>::nodeSetType({nodes[6]}));
    graph.emplace(nodes[6], new Tarjan<int>::nodeSetType({nodes[5]}));
    Tarjan<int>::nodeSetType scc({nodes[0], nodes[1], nodes[2]});

    FeedbackVertexSetSolver<int> solver(graph, scc);
    FeedbackVertexSetSolver<int>::nodeSetType* cover = solver.run();

    EXPECT_EQ(cover->size(), 2);
    EXPECT_TRUE(contains(cover, nodes[0]));
    EXPECT_TRUE(contains(cover, nodes[1]));
    releaseGraph(graph, nodes, node_cnt);
    delete cover;
}

TEST(TestGraphAlg, TestFvs4) {
    /* 6 vertices, 3 cycles */
    const int node_cnt = 6;
    GraphNode<int>** nodes = new GraphNode<int>*[node_cnt];
    for (int i = 0; i < node_cnt; i++) {
        nodes[i] = new GraphNode<int>(new int(i));
    }
    Tarjan<int>::graphType graph;
    graph.emplace(nodes[0], new Tarjan<int>::nodeSetType({nodes[1]}));
    graph.emplace(nodes[1], new Tarjan<int>::nodeSetType({nodes[2]}));
    graph.emplace(nodes[2], new Tarjan<int>::nodeSetType({nodes[0], nodes[3]}));
    graph.emplace(nodes[3], new Tarjan<int>::nodeSetType({nodes[1], nodes[4]}));
    graph.emplace(nodes[4], new Tarjan<int>::nodeSetType({nodes[5]}));
    graph.emplace(nodes[5], new Tarjan<int>::nodeSetType({nodes[3]}));
    Tarjan<int>::nodeSetType scc({nodes[0], nodes[1], nodes[2], nodes[3], nodes[4], nodes[5]});

    FeedbackVertexSetSolver<int> solver(graph, scc);
    FeedbackVertexSetSolver<int>::nodeSetType* cover = solver.run();

    EXPECT_EQ(cover->size(), 2);
    EXPECT_TRUE(contains(cover, nodes[3]));
    EXPECT_TRUE(contains(cover, nodes[0]) || contains(cover, nodes[1]) || contains(cover, nodes[2]));
    releaseGraph(graph, nodes, node_cnt);
    delete cover;
}

TEST(TestGraphAlg, TestFvs5) {
    /* 4 vertices complete graph */
    const int node_cnt = 4;
    GraphNode<int>** nodes = new GraphNode<int>*[node_cnt];
    for (int i = 0; i < node_cnt; i++) {
        nodes[i] = new GraphNode<int>(new int(i));
    }
    Tarjan<int>::graphType graph;
    graph.emplace(nodes[0], new Tarjan<int>::nodeSetType({nodes[1], nodes[2], nodes[3]}));
    graph.emplace(nodes[1], new Tarjan<int>::nodeSetType({nodes[0], nodes[2], nodes[3]}));
    graph.emplace(nodes[2], new Tarjan<int>::nodeSetType({nodes[0], nodes[1], nodes[3]}));
    graph.emplace(nodes[3], new Tarjan<int>::nodeSetType({nodes[0], nodes[1], nodes[2]}));
    Tarjan<int>::nodeSetType scc({nodes[0], nodes[1], nodes[2], nodes[3]});

    FeedbackVertexSetSolver<int> solver(graph, scc);
    FeedbackVertexSetSolver<int>::nodeSetType* cover = solver.run();

    EXPECT_EQ(cover->size(), 3);
    EXPECT_EQ(contains(cover, nodes[0]) + contains(cover, nodes[1]) + contains(cover, nodes[2]) + contains(cover, nodes[3]), 3);
    releaseGraph(graph, nodes, node_cnt);
    delete cover;
}

TEST(TestGraphAlg, TestFvs6) {
    /* 1 vertex self loop */
    const int node_cnt = 1;
    GraphNode<int>** nodes = new GraphNode<int>*[node_cnt];
    for (int i = 0; i < node_cnt; i++) {
        nodes[i] = new GraphNode<int>(new int(i));
    }
    Tarjan<int>::graphType graph;
    graph.emplace(nodes[0], new Tarjan<int>::nodeSetType({nodes[0]}));
    Tarjan<int>::nodeSetType scc({nodes[0]});

    FeedbackVertexSetSolver<int> solver(graph, scc);
    FeedbackVertexSetSolver<int>::nodeSetType* cover = solver.run();

    EXPECT_EQ(cover->size(), 1);
    EXPECT_TRUE(contains(cover, nodes[0]));
    releaseGraph(graph, nodes, node_cnt);
    delete cover;
}

TEST(TestGraphAlg, TestFvs7) {
    /* Test multiple in one graph */
    const int node_cnt = 5;
    GraphNode<int>** nodes = new GraphNode<int>*[node_cnt];
    for (int i = 0; i < node_cnt; i++) {
        nodes[i] = new GraphNode<int>(new int(i));
    }
    Tarjan<int>::graphType graph;
    graph.emplace(nodes[0], new Tarjan<int>::nodeSetType({nodes[1]}));
    graph.emplace(nodes[1], new Tarjan<int>::nodeSetType({nodes[2]}));
    graph.emplace(nodes[2], new Tarjan<int>::nodeSetType({nodes[0]}));
    graph.emplace(nodes[3], new Tarjan<int>::nodeSetType({nodes[0], nodes[1], nodes[2], nodes[4]}));
    graph.emplace(nodes[4], new Tarjan<int>::nodeSetType({nodes[3]}));

    Tarjan<int>::nodeSetType scc1({nodes[0], nodes[1], nodes[2]});
    FeedbackVertexSetSolver<int> solver1(graph, scc1);
    FeedbackVertexSetSolver<int>::nodeSetType* cover1 = solver1.run();
    EXPECT_EQ(cover1->size(), 1);
    EXPECT_TRUE(contains(cover1, nodes[0]) || contains(cover1, nodes[1]) || contains(cover1, nodes[2]));

    Tarjan<int>::nodeSetType scc2({nodes[3], nodes[4]});
    FeedbackVertexSetSolver<int> solver2(graph, scc2);
    FeedbackVertexSetSolver<int>::nodeSetType* cover2 = solver2.run();
    EXPECT_EQ(cover2->size(), 1);
    EXPECT_TRUE(contains(cover2, nodes[3]) || contains(cover2, nodes[4]));

    releaseGraph(graph, nodes, node_cnt);
    delete cover1;
    delete cover2;
}