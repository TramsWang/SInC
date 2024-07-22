#include <gtest/gtest.h>
#include "../../src/impl/sincWithEstimation.h"
#include <filesystem>

using namespace sinc;

class TestBodyVarLinkManager : public testing::Test {
protected:
    static int const p;
    static int const q;
    static int const r;
    static int const s;
    static int const t;
    static int const u;

    void assertGraphEqual(BodyVarLinkManager::varLinkGraphType const& expected, BodyVarLinkManager::varLinkGraphType const& actual) {
        EXPECT_EQ(expected.size(), actual.size());
        for (int i = 0; i < expected.size(); i++) {
            std::unordered_set<VarLink> const& expected_links = expected[i];
            std::unordered_set<VarLink> const& actual_links = actual[i];
            EXPECT_EQ(expected_links.size(), actual_links.size()) << "Different links @ " << i << std::endl;
            for (VarLink expected_link: expected_links) {
                bool found = false;
                std::unordered_set<VarLink>::const_iterator itr = actual_links.find(expected_link);
                ASSERT_NE(actual_links.end(), itr);
                VarLink const& actual_link = *itr;
                EXPECT_EQ(expected_link.predIdx, actual_link.predIdx);
                EXPECT_EQ(expected_link.fromVid, actual_link.fromVid);
                EXPECT_EQ(expected_link.fromArgIdx, actual_link.fromArgIdx);
                EXPECT_EQ(expected_link.toVid, actual_link.toVid);
                EXPECT_EQ(expected_link.toArgIdx, actual_link.toArgIdx);
            }
        }
    }

    std::vector<Predicate> copyRule(std::vector<Predicate> const& rule) {
        std::vector<Predicate> rule2;
        for (Predicate const& p: rule) {
            rule2.emplace_back(p);
        }
        return rule2;
    }

    void assertPathEquals(std::vector<VarLink> const& expected, std::vector<VarLink> const& actual) {
        EXPECT_EQ(expected.size(), actual.size());
        for (int i = 0; i < expected.size(); i++) {
            VarLink const& expected_link = expected[i];
            VarLink const& actual_link = actual[i];
            EXPECT_EQ(expected_link.predIdx, actual_link.predIdx);
            EXPECT_EQ(expected_link.fromVid, actual_link.fromVid);
            EXPECT_EQ(expected_link.fromArgIdx, actual_link.fromArgIdx);
            EXPECT_EQ(expected_link.toVid, actual_link.toVid);
            EXPECT_EQ(expected_link.toArgIdx, actual_link.toArgIdx);
        }
    }

    void assertEqualVarPairs(std::unordered_set<VarPair> const& expected, std::unordered_set<VarPair> const& actual) {
        EXPECT_EQ(expected.size(), actual.size());
        for (VarPair expected_pair: expected) {
            bool found = false;
            for (VarPair actual_pair: actual) {
                if ((expected_pair.vid1 == actual_pair.vid1 && expected_pair.vid2 == actual_pair.vid2) ||
                        (expected_pair.vid1 == actual_pair.vid2 && expected_pair.vid2 == actual_pair.vid1)) {
                    found = true;
                    break;
                }
            }
            EXPECT_TRUE(found);
        }
    }

    void releaseRuleArgs(std::vector<Predicate>& rule) {
        for (int pred_idx = 0; pred_idx < rule.size(); pred_idx++) {
            delete[] rule[pred_idx].getArgs();
        }
    }
};

int const TestBodyVarLinkManager::p = 0;
int const TestBodyVarLinkManager::q = 1;
int const TestBodyVarLinkManager::r = 2;
int const TestBodyVarLinkManager::s = 3;
int const TestBodyVarLinkManager::t = 4;
int const TestBodyVarLinkManager::u = 5;

class BodyVarLinkManager4Test : public BodyVarLinkManager {
public:
    using BodyVarLinkManager::rule;
    using BodyVarLinkManager::varLabels;
    using BodyVarLinkManager::varLinkGraph;

    /**
     * Construct an instance
     *
     * @param rule        The current structure of the rule
     * @param currentVars The number of LVs used in the rule
     */
    BodyVarLinkManager4Test(std::vector<Predicate>* const rule, int const currentVars) : BodyVarLinkManager(rule, currentVars) {}

    /**
     * Copy constructor.
     *
     * NOTE: the var link manager instance should be linked to the copied rule structure, instead of the original one
     *
     * @param another Another instance
     * @param newRule The copied rule structure in the copied rule
     */
    BodyVarLinkManager4Test(const BodyVarLinkManager& another, std::vector<Predicate>* const newRule) : BodyVarLinkManager(another, newRule) {}
};

TEST_F(TestBodyVarLinkManager, TestConstruction1) {
    /* p(X, Y) :- q(X, ?), r(?, Y) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_EMPTY_VALUE}, 2);
    rule.emplace_back(r, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(1)}, 2);
    BodyVarLinkManager4Test bvm(&rule, 2);
    std::vector<int> expected_var_labels({0, 1});
    EXPECT_EQ(expected_var_labels, bvm.varLabels);
    BodyVarLinkManager::varLinkGraphType expected_graph;
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    assertGraphEqual(expected_graph, bvm.varLinkGraph);
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, TestConstruction2) {
    /* p(X, Y) :- q(X, Z), r(Z, Y) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(2)}, 2);
    rule.emplace_back(r, new int[2]{ARG_VARIABLE(2), ARG_VARIABLE(1)}, 2);
    BodyVarLinkManager4Test bvm(&rule, 3);
    std::vector<int> expected_var_labels({0, 0, 0});
    EXPECT_EQ(expected_var_labels, bvm.varLabels);
    BodyVarLinkManager::varLinkGraphType expected_graph;
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph[0].emplace(1, 0, 0, 2, 1);
    expected_graph[1].emplace(2, 1, 1, 2, 0);
    expected_graph[2].emplace(1, 2, 1, 0, 0);
    expected_graph[2].emplace(2, 2, 0, 1, 1);
    assertGraphEqual(expected_graph, bvm.varLinkGraph);
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, TestConstruction3) {
    /* p(X, ?, Z) :- q(?, X, Y, Y), r(Z, W), s(W) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[3]{ARG_VARIABLE(0), ARG_EMPTY_VALUE, ARG_VARIABLE(2)}, 3);
    rule.emplace_back(q, new int[4]{ARG_EMPTY_VALUE, ARG_VARIABLE(0), ARG_VARIABLE(1), ARG_VARIABLE(1)}, 4);
    rule.emplace_back(r, new int[2]{ARG_VARIABLE(2), ARG_VARIABLE(3)}, 2);
    rule.emplace_back(s, new int[1]{ARG_VARIABLE(3)}, 1);
    BodyVarLinkManager4Test bvm(&rule, 4);
    std::vector<int> expected_var_labels({0, 0, 2, 2});
    EXPECT_EQ(expected_var_labels, bvm.varLabels);
    BodyVarLinkManager::varLinkGraphType expected_graph;
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph[0].emplace(1, 0, 1, 1, 2);
    expected_graph[1].emplace(1, 1, 2, 0, 1);
    expected_graph[2].emplace(2, 2, 0, 3, 1);
    expected_graph[3].emplace(2, 3, 1, 2, 0);
    assertGraphEqual(expected_graph, bvm.varLinkGraph);
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, TestConstruction4) {
    /* p(X, ?, Z) :- q(W, X, Y, Y), r(Z, W), s(W) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[3]{ARG_VARIABLE(0), ARG_EMPTY_VALUE, ARG_VARIABLE(2)}, 3);
    rule.emplace_back(q, new int[4]{ARG_VARIABLE(3), ARG_VARIABLE(0), ARG_VARIABLE(1), ARG_VARIABLE(1)}, 4);
    rule.emplace_back(r, new int[2]{ARG_VARIABLE(2), ARG_VARIABLE(3)}, 2);
    rule.emplace_back(s, new int[1]{ARG_VARIABLE(3)}, 1);
    BodyVarLinkManager4Test bvm(&rule, 4);
    std::vector<int> expected_var_labels({2, 2, 2, 2});
    EXPECT_EQ(expected_var_labels, bvm.varLabels);
    BodyVarLinkManager::varLinkGraphType expected_graph;
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph[0].emplace(1, 0, 1, 1, 2);
    expected_graph[0].emplace(1, 0, 1, 3, 0);
    expected_graph[1].emplace(1, 1, 2, 0, 1);
    expected_graph[1].emplace(1, 1, 2, 3, 0);
    expected_graph[2].emplace(2, 2, 0, 3, 1);
    expected_graph[3].emplace(1, 3, 0, 0, 1);
    expected_graph[3].emplace(1, 3, 0, 1, 2);
    expected_graph[3].emplace(2, 3, 1, 2, 0);
    assertGraphEqual(expected_graph, bvm.varLinkGraph);
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, TestCopyConstruction) {
    /* p(X, Y) :- q(X, ?), r(?, Y) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_EMPTY_VALUE}, 2);
    rule.emplace_back(r, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(1)}, 2);
    BodyVarLinkManager4Test bvm(&rule, 2);
    std::vector<Predicate> rule2 = copyRule(rule);
    BodyVarLinkManager4Test bvm2(bvm, &rule2);
    std::vector<int> expected_var_labels({0, 1});
    EXPECT_EQ(expected_var_labels, bvm2.varLabels);
    BodyVarLinkManager::varLinkGraphType expected_graph;
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    assertGraphEqual(expected_graph, bvm2.varLinkGraph);

    /* p(X, Y) :- q(X, Z), r(Z, Y) */
    bvm2.specOprCase3(1, 1, 2, 0);
    std::vector<int> expected_var_labels2({0, 0, 0});
    EXPECT_EQ(expected_var_labels2, bvm2.varLabels);
    BodyVarLinkManager::varLinkGraphType expected_graph2;
    expected_graph2.emplace_back();
    expected_graph2.emplace_back();
    expected_graph2.emplace_back();
    expected_graph2[0].emplace(1, 0, 0, 2, 1);
    expected_graph2[1].emplace(2, 1, 1, 2, 0);
    expected_graph2[2].emplace(1, 2, 1, 0, 0);
    expected_graph2[2].emplace(2, 2, 0, 1, 1);
    EXPECT_EQ(expected_graph2, bvm2.varLinkGraph);

    EXPECT_EQ(expected_var_labels, bvm.varLabels);
    assertGraphEqual(expected_graph, bvm.varLinkGraph);
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, specCase1Test1) {
    /* p(X, Y) :- q(X, ?), r(?, Y) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_EMPTY_VALUE}, 2);
    rule.emplace_back(r, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(1)}, 2);
    BodyVarLinkManager4Test bvm(&rule, 2);

    /* p(X, Y) :- q(X, X), r(?, Y) */
    rule[1].setArg(1, ARG_VARIABLE(0));
    bvm.specOprCase1(1, 1, 0);
    std::vector<int> expected_var_labels({0, 1});
    EXPECT_EQ(expected_var_labels, bvm.varLabels);
    BodyVarLinkManager::varLinkGraphType expected_graph;
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    assertGraphEqual(expected_graph, bvm.varLinkGraph);
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, specCase1Test2) {
    /* p(X, Y) :- q(?, X), r(?, Y) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(q, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(0)}, 2);
    rule.emplace_back(r, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(1)}, 2);
    BodyVarLinkManager4Test bvm(&rule, 2);

    /* p(X, Y) :- q(Y, X), r(?, Y) */
    rule[1].setArg(0, ARG_VARIABLE(1));
    bvm.specOprCase1(1, 0, 1);
    std::vector<int> expected_var_labels({1, 1});
    EXPECT_EQ(expected_var_labels, bvm.varLabels);
    BodyVarLinkManager::varLinkGraphType expected_graph;
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph[0].emplace(1, 0, 1, 1, 0);
    expected_graph[1].emplace(1, 1, 0, 0, 1);
    assertGraphEqual(expected_graph, bvm.varLinkGraph);
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, specCase1Test3) {
    /* p(X, ?) :- q(X, Y), r(Y, Z), s(Z) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[2]{ARG_VARIABLE(0), ARG_EMPTY_VALUE}, 2);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(r, new int[2]{ARG_VARIABLE(1), ARG_VARIABLE(2)}, 2);
    rule.emplace_back(s, new int[1]{ARG_VARIABLE(2)}, 1);
    BodyVarLinkManager4Test bvm(&rule, 3);

    /* p(X, Z) :- q(X, Y), r(Y, Z), s(Z) */
    rule[0].setArg(1, ARG_VARIABLE(2));
    bvm.specOprCase1(0, 1, 2);
    std::vector<int> expected_var_labels({0, 0, 0});
    EXPECT_EQ(expected_var_labels, bvm.varLabels);
    BodyVarLinkManager::varLinkGraphType expected_graph;
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph[0].emplace(1, 0, 0, 1, 1);
    expected_graph[1].emplace(1, 1, 1, 0, 0);
    expected_graph[1].emplace(2, 1, 0, 2, 1);
    expected_graph[2].emplace(2, 2, 1, 1, 0);
    assertGraphEqual(expected_graph, bvm.varLinkGraph);
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, specCase3Test1) {
    /* p(X, Y) :- q(X, ?), r(?, Y) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_EMPTY_VALUE}, 2);
    rule.emplace_back(r, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(1)}, 2);
    BodyVarLinkManager4Test bvm(&rule, 2);

    /* p(X, Y) :- q(X, Z), r(Z, Y) */
    rule[1].setArg(1, ARG_VARIABLE(2));
    rule[2].setArg(0, ARG_VARIABLE(2));
    bvm.specOprCase3(1, 1, 2, 0);
    std::vector<int> expected_var_labels({0, 0, 0});
    EXPECT_EQ(expected_var_labels, bvm.varLabels);
    BodyVarLinkManager::varLinkGraphType expected_graph;
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph[0].emplace(1, 0, 0, 2, 1);
    expected_graph[1].emplace(2, 1, 1, 2, 0);
    expected_graph[2].emplace(1, 2, 1, 0, 0);
    expected_graph[2].emplace(2, 2, 0, 1, 1);
    assertGraphEqual(expected_graph, bvm.varLinkGraph);
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, specCase3Test2) {
    /* p(X, ?) :- q(X, Y), r(Y, ?) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[2]{ARG_VARIABLE(0), ARG_EMPTY_VALUE}, 2);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(r, new int[2]{ARG_VARIABLE(1), ARG_EMPTY_VALUE}, 2);
    BodyVarLinkManager4Test bvm(&rule, 2);

    /* p(X, Z) :- q(X, Y), r(Y, Z) */
    rule[0].setArg(1, ARG_VARIABLE(2));
    rule[2].setArg(1, ARG_VARIABLE(2));
    bvm.specOprCase3(0, 1, 2, 1);
    std::vector<int> expected_var_labels({0, 0, 0});
    EXPECT_EQ(expected_var_labels, bvm.varLabels);
    BodyVarLinkManager::varLinkGraphType expected_graph;
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph[0].emplace(1, 0, 0, 1, 1);
    expected_graph[1].emplace(1, 1, 1, 0, 0);
    expected_graph[1].emplace(2, 1, 0, 2, 1);
    expected_graph[2].emplace(2, 2, 1, 1, 0);
    assertGraphEqual(expected_graph, bvm.varLinkGraph);
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, specCase3Test3) {
    /* p(X, ?, ?) :- q(X, Y), r(Y, ?) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[3]{ARG_VARIABLE(0), ARG_EMPTY_VALUE, ARG_EMPTY_VALUE}, 3);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(r, new int[2]{ARG_VARIABLE(1), ARG_EMPTY_VALUE}, 2);
    BodyVarLinkManager4Test bvm(&rule, 2);

    /* p(X, Z, Z) :- q(X, Y), r(Y, ?) */
    rule[0].setArg(1, ARG_VARIABLE(2));
    rule[0].setArg(2, ARG_VARIABLE(2));
    bvm.specOprCase3(0, 1, 0, 2);
    std::vector<int> expected_var_labels({0, 0, 2});
    EXPECT_EQ(expected_var_labels, bvm.varLabels);
    BodyVarLinkManager::varLinkGraphType expected_graph;
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph[0].emplace(1, 0, 0, 1, 1);
    expected_graph[1].emplace(1, 1, 1, 0, 0);
    assertGraphEqual(expected_graph, bvm.varLinkGraph);
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, specCase3Test4) {
    /* p(X, ?, ?) :- q(X, Y), r(Y, ?, ?) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[3]{ARG_VARIABLE(0), ARG_EMPTY_VALUE, ARG_EMPTY_VALUE}, 3);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(r, new int[3]{ARG_VARIABLE(1), ARG_EMPTY_VALUE, ARG_EMPTY_VALUE}, 3);
    BodyVarLinkManager4Test bvm(&rule, 2);

    /* p(X, ?, ?) :- q(X, Y), r(Y, Z, Z) */
    rule[2].setArg(1, ARG_VARIABLE(2));
    rule[2].setArg(2, ARG_VARIABLE(2));
    bvm.specOprCase3(2, 1, 2, 2);
    std::vector<int> expected_var_labels({0, 0, 0});
    EXPECT_EQ(expected_var_labels, bvm.varLabels);
    BodyVarLinkManager::varLinkGraphType expected_graph;
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph[0].emplace(1, 0, 0, 1, 1);
    expected_graph[1].emplace(1, 1, 1, 0, 0);
    expected_graph[1].emplace(2, 1, 0, 2, 1);
    expected_graph[2].emplace(2, 2, 1, 1, 0);
    assertGraphEqual(expected_graph, bvm.varLinkGraph);
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, specCase4Test1) {
    /* p(X, ?, ?) :- q(X, Y), r(Y, ?, ?) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[3]{ARG_VARIABLE(0), ARG_EMPTY_VALUE, ARG_EMPTY_VALUE}, 3);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(r, new int[3]{ARG_VARIABLE(1), ARG_EMPTY_VALUE, ARG_EMPTY_VALUE}, 3);
    BodyVarLinkManager4Test bvm(&rule, 2);

    /* p(X, ?, ?) :- q(X, Y), r(Y, Z, ?), s(?, Z, ?) */
    rule[2].setArg(1, ARG_VARIABLE(2));
    rule.emplace_back(s, new int[3]{ARG_EMPTY_VALUE, ARG_VARIABLE(2), ARG_EMPTY_VALUE}, 3);
    bvm.specOprCase4(2, 1);
    std::vector<int> expected_var_labels({0, 0, 0});
    EXPECT_EQ(expected_var_labels, bvm.varLabels);
    BodyVarLinkManager::varLinkGraphType expected_graph;
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph[0].emplace(1, 0, 0, 1, 1);
    expected_graph[1].emplace(1, 1, 1, 0, 0);
    expected_graph[1].emplace(2, 1, 0, 2, 1);
    expected_graph[2].emplace(2, 2, 1, 1, 0);
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, specCase4Test2) {
    /* p(X, ?, ?) :- q(X, Y), r(Y, ?, ?) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[3]{ARG_VARIABLE(0), ARG_EMPTY_VALUE, ARG_EMPTY_VALUE}, 3);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(r, new int[3]{ARG_VARIABLE(1), ARG_EMPTY_VALUE, ARG_EMPTY_VALUE}, 3);
    BodyVarLinkManager4Test bvm(&rule, 2);

    /* p(X, ?, Z) :- q(X, Y), r(Y, ?, ?), s(?, Z, ?) */
    rule[0].setArg(2, ARG_VARIABLE(2));
    rule.emplace_back(s, new int[3]{ARG_EMPTY_VALUE, ARG_VARIABLE(2), ARG_EMPTY_VALUE}, 3);
    bvm.specOprCase4(0, 2);
    std::vector<int> expected_var_labels({0, 0, 2});
    EXPECT_EQ(expected_var_labels, bvm.varLabels);
    BodyVarLinkManager::varLinkGraphType expected_graph;
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph.emplace_back();
    expected_graph[0].emplace(1, 0, 0, 1, 1);
    expected_graph[1].emplace(1, 1, 1, 0, 0);
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, TestFindShortestPath1) {
    /* p(X, Z, W) :- q(X, Y, ?), r(Y, ?, Z), s(W) [X, Y] */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[3]{ARG_VARIABLE(0), ARG_VARIABLE(2), ARG_VARIABLE(3)}, 3);
    rule.emplace_back(q, new int[3]{ARG_VARIABLE(0), ARG_VARIABLE(1), ARG_EMPTY_VALUE}, 3);
    rule.emplace_back(r, new int[3]{ARG_VARIABLE(1), ARG_EMPTY_VALUE, ARG_VARIABLE(2)}, 3);
    rule.emplace_back(s, new int[1]{ARG_VARIABLE(3)}, 1);
    BodyVarLinkManager4Test bvm(&rule, 4);
    std::vector<VarLink> expected_path1;
    expected_path1.emplace_back(1, 0, 0, 1, 1);
    std::vector<VarLink>* actual_path1 = bvm.shortestPath(0, 1);
    assertPathEquals(expected_path1, *actual_path1);
    delete actual_path1;
    std::vector<VarLink> expected_path2;
    expected_path2.emplace_back(1, 1, 1, 0, 0);
    std::vector<VarLink>* actual_path2 = bvm.shortestPath(1, 0);
    assertPathEquals(expected_path2, *actual_path2);
    delete actual_path2;
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, TestFindShortestPath2) {
    /* p(X, Z, W) :- q(X, Y, ?), r(Y, ?, Z), s(W) [X, Z] */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[3]{ARG_VARIABLE(0), ARG_VARIABLE(2), ARG_VARIABLE(3)}, 3);
    rule.emplace_back(q, new int[3]{ARG_VARIABLE(0), ARG_VARIABLE(1), ARG_EMPTY_VALUE}, 3);
    rule.emplace_back(r, new int[3]{ARG_VARIABLE(1), ARG_EMPTY_VALUE, ARG_VARIABLE(2)}, 3);
    rule.emplace_back(s, new int[1]{ARG_VARIABLE(3)}, 1);
    BodyVarLinkManager4Test bvm(&rule, 4);
    std::vector<VarLink> expected_path1;
    expected_path1.emplace_back(1, 0, 0, 1, 1);
    expected_path1.emplace_back(2, 1, 0, 2, 2);
    std::vector<VarLink>* actual_path1 = bvm.shortestPath(0, 2);
    assertPathEquals(expected_path1, *actual_path1);
    delete actual_path1;
    std::vector<VarLink> expected_path2;
    expected_path2.emplace_back(2, 2, 2, 1, 0);
    expected_path2.emplace_back(1, 1, 1, 0, 0);
    std::vector<VarLink>* actual_path2 = bvm.shortestPath(2, 0);
    assertPathEquals(expected_path2, *actual_path2);
    delete actual_path2;
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, TestFindShortestPath3) {
    /* p(X, Z, W) :- q(X, Y, ?), r(Y, ?, Z), s(W) [X, W] */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[3]{ARG_VARIABLE(0), ARG_VARIABLE(2), ARG_VARIABLE(3)}, 3);
    rule.emplace_back(q, new int[3]{ARG_VARIABLE(0), ARG_VARIABLE(1), ARG_EMPTY_VALUE}, 3);
    rule.emplace_back(r, new int[3]{ARG_VARIABLE(1), ARG_EMPTY_VALUE, ARG_VARIABLE(2)}, 3);
    rule.emplace_back(s, new int[1]{ARG_VARIABLE(3)}, 1);
    BodyVarLinkManager4Test bvm(&rule, 4);
    EXPECT_EQ(nullptr, bvm.shortestPath(0, 3));
    EXPECT_EQ(nullptr, bvm.shortestPath(1, 3));
    EXPECT_EQ(nullptr, bvm.shortestPath(2, 3));
    EXPECT_EQ(nullptr, bvm.shortestPath(3, 0));
    EXPECT_EQ(nullptr, bvm.shortestPath(3, 1));
    EXPECT_EQ(nullptr, bvm.shortestPath(3, 2));
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, TestFindShortestPath4) {
    /* p(X, Z, W) :- q(X, Y, W), r(Y, ?, Z), s(W, R), s(R, Z) [X, Z] */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[3]{ARG_VARIABLE(0), ARG_VARIABLE(2), ARG_VARIABLE(3)}, 3);
    rule.emplace_back(q, new int[3]{ARG_VARIABLE(0), ARG_VARIABLE(1), ARG_VARIABLE(3)}, 3);
    rule.emplace_back(r, new int[3]{ARG_VARIABLE(1), ARG_EMPTY_VALUE, ARG_VARIABLE(2)}, 3);
    rule.emplace_back(s, new int[2]{ARG_VARIABLE(3), ARG_VARIABLE(4)}, 2);
    rule.emplace_back(s, new int[2]{ARG_VARIABLE(4), ARG_VARIABLE(2)}, 2);
    BodyVarLinkManager4Test bvm(&rule, 5);
    std::vector<VarLink> expected_path1;
    expected_path1.emplace_back(1, 0, 0, 1, 1);
    expected_path1.emplace_back(2, 1, 0, 2, 2);
    std::vector<VarLink>* actual_path1 = bvm.shortestPath(0, 2);
    assertPathEquals(expected_path1, *actual_path1);
    delete actual_path1;
    std::vector<VarLink> expected_path2;
    expected_path2.emplace_back(1, 3, 2, 0, 0);
    std::vector<VarLink>* actual_path2 = bvm.shortestPath(3, 0);
    assertPathEquals(expected_path2, *actual_path2);
    delete actual_path2;
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, assumeSpecCase1Test1) {
    /* p(X, Y) :- q(X, Z), r(?, Y), s(Z) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(2)}, 2);
    rule.emplace_back(r, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(1)}, 2);
    rule.emplace_back(s, new int[1]{ARG_VARIABLE(2)}, 1);
    BodyVarLinkManager4Test bvm(&rule, 3);

    /* p(X, Y) :- q(X, Z), r(Z, Y), s(Z) */
    std::unordered_set<VarPair> expected_pairs;
    expected_pairs.emplace(0, 1);
    expected_pairs.emplace(2, 1);
    std::unordered_set<VarPair>* actual_pairs = bvm.assumeSpecOprCase1(2, 0, 2);
    assertEqualVarPairs(expected_pairs, *actual_pairs);
    delete actual_pairs;
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, assumeSpecCase1Test2) {
    /* p(X, Y) :- q(X, Z), r(?, Y), s(Z) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(2)}, 2);
    rule.emplace_back(r, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(1)}, 2);
    rule.emplace_back(s, new int[1]{ARG_VARIABLE(2)}, 1);
    BodyVarLinkManager4Test bvm(&rule, 3);

    /* p(X, Y) :- q(X, Z), r(X, Y), s(Z) */
    std::unordered_set<VarPair> expected_pairs;
    expected_pairs.emplace(0, 1);
    expected_pairs.emplace(2, 1);
    std::unordered_set<VarPair>* actual_pairs = bvm.assumeSpecOprCase1(2, 0, 0);
    assertEqualVarPairs(expected_pairs, *actual_pairs);
    delete actual_pairs;
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, assumeSpecCase1Test3) {
    /* p(X, Y) :- q(X, Z), r(?, Y), s(Z) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(2)}, 2);
    rule.emplace_back(r, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(1)}, 2);
    rule.emplace_back(s, new int[1]{ARG_VARIABLE(2)}, 1);
    BodyVarLinkManager4Test bvm(&rule, 3);

    /* p(X, Y) :- q(X, Z), r(Y, Y), s(Z) */
    EXPECT_EQ(nullptr, bvm.assumeSpecOprCase1(2, 0, 1));
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, assumeSpecCase1Test4) {
    /* p(X, Y) :- q(X, W, Z), r(?, Y, ?), s(Z), t(W, R), u(W, R), u(Y, S) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(q, new int[3]{ARG_VARIABLE(0), ARG_VARIABLE(3), ARG_VARIABLE(2)}, 3);
    rule.emplace_back(r, new int[3]{ARG_EMPTY_VALUE, ARG_VARIABLE(1), ARG_EMPTY_VALUE}, 3);
    rule.emplace_back(s, new int[1]{ARG_VARIABLE(2)}, 1);
    rule.emplace_back(t, new int[2]{ARG_VARIABLE(3), ARG_VARIABLE(4)}, 2);
    rule.emplace_back(u, new int[2]{ARG_VARIABLE(3), ARG_VARIABLE(4)}, 2);
    rule.emplace_back(u, new int[2]{ARG_VARIABLE(1), ARG_VARIABLE(5)}, 2);
    BodyVarLinkManager4Test bvm(&rule, 6);

    /* p(X, Y) :- q(X, W, Z), r(Z, Y, R), s(Z), t(W, R), u(W, R) */
    std::unordered_set<VarPair> expected_pairs;
    expected_pairs.emplace(0, 1);
    expected_pairs.emplace(2, 1);
    expected_pairs.emplace(3, 1);
    expected_pairs.emplace(4, 1);
    expected_pairs.emplace(0, 5);
    expected_pairs.emplace(2, 5);
    expected_pairs.emplace(3, 5);
    expected_pairs.emplace(4, 5);
    std::unordered_set<VarPair>* actual_pairs = bvm.assumeSpecOprCase1(2, 0, 2);
    assertEqualVarPairs(expected_pairs, *actual_pairs);
    delete actual_pairs;
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, assumeSpecCase3Test1) {
    /* p(X, Y) :- q(X, Z), r(Z, Y), s(X, ?), t(?, Y)*/
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(2)}, 2);
    rule.emplace_back(r, new int[2]{ARG_VARIABLE(2), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(s, new int[2]{ARG_VARIABLE(0), ARG_EMPTY_VALUE}, 2);
    rule.emplace_back(t, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(1)}, 2);
    BodyVarLinkManager4Test bvm(&rule, 3);

    /* p(X, Y) :- q(X, Z), r(Z, Y), s(X, W), t(W, Y)*/
    EXPECT_EQ(nullptr, bvm.assumeSpecOprCase3(3, 1, 4, 0));
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, assumeSpecCase3Test2) {
    /* p(X, Y, Z) :- q(X, W), r(W, Y), s(Y, ?), t(?, Z)*/
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[3]{ARG_VARIABLE(0), ARG_VARIABLE(1), ARG_VARIABLE(2)}, 3);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(3)}, 2);
    rule.emplace_back(r, new int[2]{ARG_VARIABLE(3), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(s, new int[2]{ARG_VARIABLE(1), ARG_EMPTY_VALUE}, 2);
    rule.emplace_back(t, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(2)}, 2);
    BodyVarLinkManager4Test bvm(&rule, 4);

    /* p(X, Y, Z) :- q(X, W), r(W, Y), s(Y, R), t(R, Z)*/
    std::unordered_set<VarPair> expected_pairs;
    expected_pairs.emplace(0, 2);
    expected_pairs.emplace(1, 2);
    expected_pairs.emplace(3, 2);
    std::unordered_set<VarPair>* actual_pairs = bvm.assumeSpecOprCase3(3, 1, 4, 0);
    assertEqualVarPairs(expected_pairs, *actual_pairs);
    delete actual_pairs;
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, assumeShortestPathCase1Test1) {
    /* p(X, Y) :- q(X, Z), r(?, Y), s(Z) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(2)}, 2);
    rule.emplace_back(r, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(1)}, 2);
    rule.emplace_back(s, new int[1]{ARG_VARIABLE(2)}, 1);
    BodyVarLinkManager4Test bvm(&rule, 3);

    /* p(X, Y) :- q(X, Z), r(Z, Y), s(Z) */
    std::vector<VarLink> expected_path1;
    expected_path1.emplace_back(1, 0, 0, 2, 1);
    expected_path1.emplace_back(2, 2, 0, 1, 1);
    std::vector<VarLink>* actual_path1 = bvm.assumeShortestPathCase1(2, 0, 2, 0, 1);
    assertPathEquals(expected_path1, *actual_path1);
    delete actual_path1;
    std::vector<VarLink> expected_path2;
    expected_path2.emplace_back(2, 1, 1, 2, 0);
    expected_path2.emplace_back(1, 2, 1, 0, 0);
    std::vector<VarLink>* actual_path2 = bvm.assumeShortestPathCase1(2, 0, 2, 1, 0);
    assertPathEquals(expected_path2, *actual_path2);
    delete actual_path2;
    std::vector<VarLink> expected_path3;
    expected_path3.emplace_back(2, 2, 0, 1, 1);
    std::vector<VarLink>* actual_path3 = bvm.assumeShortestPathCase1(2, 0, 2, 2, 1);
    assertPathEquals(expected_path3, *actual_path3);
    delete actual_path3;
    std::vector<VarLink> expected_path4;
    expected_path4.emplace_back(2, 1, 1, 2, 0);
    std::vector<VarLink>* actual_path4 = bvm.assumeShortestPathCase1(2, 0, 2, 1, 2);
    assertPathEquals(expected_path4, *actual_path4);
    delete actual_path4;
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, assumeShortestPathCase1Test2) {
    /* p(X, Y) :- q(X, Z), r(?, Y), s(Z) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(2)}, 2);
    rule.emplace_back(r, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(1)}, 2);
    rule.emplace_back(s, new int[1]{ARG_VARIABLE(2)}, 1);
    BodyVarLinkManager4Test bvm(&rule, 3);

    /* p(X, Y) :- q(X, Z), r(X, Y), s(Z) */
    std::vector<VarLink> expected_path1;
    expected_path1.emplace_back(2, 1, 1, 0, 0);
    std::vector<VarLink>* actual_path1 = bvm.assumeShortestPathCase1(2, 0, 0, 1, 0);
    assertPathEquals(expected_path1, *actual_path1);
    delete actual_path1;
    std::vector<VarLink> expected_path2;
    expected_path2.emplace_back(2, 1, 1, 0, 0);
    expected_path2.emplace_back(1, 0, 0, 2, 1);
    std::vector<VarLink>* actual_path2 = bvm.assumeShortestPathCase1(2, 0, 0, 1, 2);
    assertPathEquals(expected_path2, *actual_path2);
    delete actual_path2;
    std::vector<VarLink> expected_path3;
    expected_path3.emplace_back(1, 2, 1, 0, 0);
    expected_path3.emplace_back(2, 0, 0, 1, 1);
    std::vector<VarLink>* actual_path3 = bvm.assumeShortestPathCase1(2, 0, 0, 2, 1);
    assertPathEquals(expected_path3, *actual_path3);
    delete actual_path3;
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, assumeShortestPathCase1Test3) {
    /* p(X, Y) :- q(X, Z), r(?, Y), s(Z) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(2)}, 2);
    rule.emplace_back(r, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(1)}, 2);
    rule.emplace_back(s, new int[1]{ARG_VARIABLE(2)}, 1);
    BodyVarLinkManager4Test bvm(&rule, 3);

    /* p(X, Y) :- q(X, Z), r(Y, Y), s(Z) */
    EXPECT_EQ(nullptr, bvm.assumeShortestPathCase1(2, 0, 1, 1, 0));
    EXPECT_EQ(nullptr, bvm.assumeShortestPathCase1(2, 0, 1, 0, 1));
    EXPECT_EQ(nullptr, bvm.assumeShortestPathCase1(2, 0, 1, 1, 2));
    EXPECT_EQ(nullptr, bvm.assumeShortestPathCase1(2, 0, 1, 2, 1));
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, assumeShortestPathCase1Test4) {
    /* p(X, Y) :- q(X, W, Z), r(?, Y, ?), s(Z), t(W, R), u(W, R) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(q, new int[3]{ARG_VARIABLE(0), ARG_VARIABLE(3), ARG_VARIABLE(2)}, 3);
    rule.emplace_back(r, new int[3]{ARG_EMPTY_VALUE, ARG_VARIABLE(1), ARG_EMPTY_VALUE}, 3);
    rule.emplace_back(s, new int[1]{ARG_VARIABLE(2)}, 1);
    rule.emplace_back(t, new int[2]{ARG_VARIABLE(3), ARG_VARIABLE(4)}, 2);
    rule.emplace_back(u, new int[2]{ARG_VARIABLE(3), ARG_VARIABLE(4)}, 2);
    BodyVarLinkManager4Test bvm(&rule, 5);

    /* p(X, Y) :- q(X, W, Z), r(Z, Y, ?), s(Z), t(W, R), u(W, R) */
    std::vector<VarLink> expected_path1;
    expected_path1.emplace_back(2, 1, 1, 2, 0);
    expected_path1.emplace_back(1, 2, 2, 0, 0);
    std::vector<VarLink>* actual_path1 = bvm.assumeShortestPathCase1(2, 0, 2, 1, 0);
    assertPathEquals(expected_path1, *actual_path1);
    delete actual_path1;
    std::vector<VarLink> expected_path2;
    expected_path2.emplace_back(2, 2, 0, 1, 1);
    std::vector<VarLink>* actual_path2 = bvm.assumeShortestPathCase1(2, 0, 2, 2, 1);
    assertPathEquals(expected_path2, *actual_path2);
    delete actual_path2;
    std::vector<VarLink> expected_path3;
    expected_path3.emplace_back(1, 3, 1, 2, 2);
    expected_path3.emplace_back(2, 2, 0, 1, 1);
    std::vector<VarLink>* actual_path3 = bvm.assumeShortestPathCase1(2, 0, 2, 3, 1);
    assertPathEquals(expected_path3, *actual_path3);
    delete actual_path3;
    std::vector<VarLink> expected_path4;
    expected_path4.emplace_back(4, 4, 1, 3, 0);
    expected_path4.emplace_back(1, 3, 1, 2, 2);
    expected_path4.emplace_back(2, 2, 0, 1, 1);
    std::vector<VarLink>* actual_path4 = bvm.assumeShortestPathCase1(2, 0, 2, 4, 1);
    assertPathEquals(expected_path4, *actual_path4);
    delete actual_path4;
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, assumeShortestPathCase3Test1) {
    /* p(X, Y) :- q(X, ?), r(?, Y) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_EMPTY_VALUE}, 2);
    rule.emplace_back(r, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(1)}, 2);
    BodyVarLinkManager4Test bvm(&rule, 2);

    /* p(X, Y) :- q(X, Z), r(Z, Y) */
    std::vector<VarLink> expected_path1;
    expected_path1.emplace_back(2, 1, 1, -1, 0);
    expected_path1.emplace_back(1, -1, 1, 0, 0);
    std::vector<VarLink>* actual_path1 = bvm.assumeShortestPathCase3(1, 1, 2, 0, 1, 0);
    assertPathEquals(expected_path1, *actual_path1);
    delete actual_path1;
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, assumeShortestPathCase3Test2) {
    /* p(X, Y, Z) :- q(X, W), r(W, Y), s(Y, ?), t(?, Z)*/
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[3]{ARG_VARIABLE(0), ARG_VARIABLE(1), ARG_VARIABLE(2)}, 3);
    rule.emplace_back(q, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(3)}, 2);
    rule.emplace_back(r, new int[2]{ARG_VARIABLE(3), ARG_VARIABLE(1)}, 2);
    rule.emplace_back(s, new int[2]{ARG_VARIABLE(1), ARG_EMPTY_VALUE}, 2);
    rule.emplace_back(t, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(2)}, 2);
    BodyVarLinkManager4Test bvm(&rule, 4);

    /* p(X, Y, Z) :- q(X, W), r(W, Y), s(Y, R), t(R, Z)*/
    std::vector<VarLink> expected_path1;
    expected_path1.emplace_back(1, 0, 0, 3, 1);
    expected_path1.emplace_back(2, 3, 0, 1, 1);
    expected_path1.emplace_back(3, 1, 0, -1, 1);
    expected_path1.emplace_back(4, -1, 0, 2, 1);
    std::vector<VarLink>* actual_path1 = bvm.assumeShortestPathCase3(3, 1, 4, 0, 0, 2);
    assertPathEquals(expected_path1, *actual_path1);
    delete actual_path1;
    releaseRuleArgs(rule);
}

TEST_F(TestBodyVarLinkManager, assumeShortestPathCase3Test3) {
    /* p(X, Y, ?) :- q(?, Y), r(X, Y) */
    std::vector<Predicate> rule;
    rule.emplace_back(p, new int[3]{ARG_VARIABLE(0), ARG_VARIABLE(1), ARG_EMPTY_VALUE}, 3);
    rule.emplace_back(q, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(1)}, 2);
    rule.emplace_back(r, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    BodyVarLinkManager4Test bvm(&rule, 2);

    /* p(X, Y, Z) :- q(Z, Y), r(X, Y) */
    std::vector<VarLink> expected_path1;
    expected_path1.emplace_back(2, 0, 0, 1, 1);
    expected_path1.emplace_back(1, 1, 1, -1, 0);
    std::vector<VarLink>* actual_path1 = bvm.assumeShortestPathCase3(1, 0, 0);
    assertPathEquals(expected_path1, *actual_path1);
    delete actual_path1;
    std::vector<VarLink> expected_path2;
    expected_path2.emplace_back(1, 1, 1, -1, 0);
    std::vector<VarLink>* actual_path2 = bvm.assumeShortestPathCase3(1, 0, 1);
    assertPathEquals(expected_path2, *actual_path2);
    delete actual_path2;
    releaseRuleArgs(rule);
}

TEST(TestSpecOprWithScore, TestPushingIntoVector) {
    std::vector<SpecOprWithScore> *v = new std::vector<SpecOprWithScore>();
    v->emplace_back(new SpecOprCase1(0, 0, 0), Eval(0, 0, 0));
    v->emplace_back(new SpecOprCase2(0, 0, 0, 0), Eval(0, 0, 0));
    v->emplace_back(new SpecOprCase3(0, 0, 0, 0), Eval(0, 0, 0));
    v->emplace_back(new SpecOprCase4(0, 0, 0, 0, 0), Eval(0, 0, 0));
    v->emplace_back(new SpecOprCase5(0, 0, 0), Eval(0, 0, 0));
    delete v;
}