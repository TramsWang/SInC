#include <gtest/gtest.h>
#include "../../src/base/sinc.h"
#include <filesystem>

using namespace sinc;

class RelationMiner4Test : public RelationMiner {
public:
    Rule::fingerprintCacheType cache;
    Rule::tabuMapType tabuMap;
    BareRule* badRule;

    RelationMiner4Test(
            SimpleKb& kb, int const targetRelation, EvalMetric::Value evalMetric, int const beamwidth, double const stopCompressionRatio,
            nodeMapType& predicate2NodeMap, depGraphType& dependencyGraph, std::vector<Rule*>& hypothesis,
            std::unordered_set<Record>& counterexamples, std::ostream& logger
    ) : RelationMiner(
        kb, targetRelation, evalMetric, beamwidth, stopCompressionRatio, predicate2NodeMap, dependencyGraph, hypothesis,
        counterexamples, logger
    ) {
        badRule = new BareRule(-1, 1, cache, tabuMap);
        badRule->returningEval = Eval(-9999, 9999, 9999);
    }

    ~RelationMiner4Test() {
        for (Fingerprint* const& fp: cache) {
            delete fp;
        }
        for (std::pair<const sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabuMap) {
            delete kv.first;
            delete kv.second;
        }
        delete badRule;
    }

    using RelationMiner::findSpecializations;
    using RelationMiner::findGeneralizations;

protected:
    Rule* getStartRule() override {
        for (Fingerprint* const& fp: cache) {
            delete fp;
        }
        for (std::pair<const sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabuMap) {
            delete kv.first;
            delete kv.second;
        }
        cache.clear();
        tabuMap.clear();
        return new BareRule(targetRelation, kb.getRelation(targetRelation)->getTotalCols(), cache, tabuMap);
    }

    int checkThenAddRule(UpdateStatus updateStatus, Rule* const updatedRule, Rule& originalRule, Rule** candidates) override {
        return RelationMiner::checkThenAddRule(updateStatus, updatedRule, *badRule, candidates);
    }

    void selectAsBeam(Rule& r) override {}
};

class TestRelationMiner : public testing::Test {
public:
    /* KB:
     * family       father  mother  isMale
     * 1    2   3   2   3   1   3   2
     * 1    2   4   4   6   2   4
     */
    static int*** relations;
    static std::string relNames[4];
    static int arities[4];
    static int totalRows[4];
    static const char* kbName;
    static SimpleKb* kb;

    static void SetUpTestSuite() {
        relations = new int**[4] {
            new int*[2] {   // family
                new int[3] {1, 2, 3},
                new int[3] {1, 2, 4}
            },
            new int*[2] {   // father
                new int[2] {2, 3},
                new int[2] {4, 6}
            },
            new int*[2] {   // mother
                new int[2] {1, 3},
                new int[2] {2, 4}
            },
            new int*[1] {   // isMale
                new int[1] {2}
            }
        };
        SimpleRelation::minConstantCoverage = 0.6;
        kb = new SimpleKb(kbName, relations, relNames, arities, totalRows, 4);
        kb->updatePromisingConstants();
    }

    static void TearDownTestSuite() {
        delete kb;
        delete[] relations[0][0];
        delete[] relations[0][1];
        delete[] relations[0];
        delete[] relations[1][0];
        delete[] relations[1][1];
        delete[] relations[1];
        delete[] relations[2][0];
        delete[] relations[2][1];
        delete[] relations[2];
        delete[] relations[3][0];
        delete[] relations[3];
        delete[] relations;
    }
};

int*** TestRelationMiner::relations = nullptr;
std::string TestRelationMiner::relNames[4] {"family", "father", "mother", "isMale"};
int TestRelationMiner::arities[4] {3, 2, 2, 1};
int TestRelationMiner::totalRows[4] {2, 2, 2, 1};
const char* TestRelationMiner::kbName = "RelationMinerTestKb";
SimpleKb* TestRelationMiner::kb = nullptr;

TEST_F(TestRelationMiner, TestFindSpecialization1) {
    SimpleRelation* rel_family = kb->getRelation("family");
    SimpleRelation* rel_father = kb->getRelation("father");
    SimpleRelation* rel_mother = kb->getRelation("mother");
    SimpleRelation* rel_is_male = kb->getRelation("isMale");
    EXPECT_NE(rel_family, nullptr);
    EXPECT_NE(rel_mother, nullptr);
    EXPECT_NE(rel_mother, nullptr);
    EXPECT_NE(rel_is_male, nullptr);

    Rule::fingerprintCacheType cache;
    Rule::tabuMapType tabuMap;
    BareRule base_rule(rel_family->id, 3, cache, tabuMap);
    const char* rel_names[4] {relNames[0].c_str(), relNames[1].c_str(), relNames[2].c_str(), relNames[3].c_str()};
    EXPECT_STREQ("family(?,?,?):-", base_rule.toDumpString(rel_names).c_str());

    std::unordered_set<std::string> expected_specs({
        "family(X0,X0,?):-",
        "family(X0,?,X0):-",//
        "family(?,X0,X0):-",
        "family(X0,?,?):-family(X0,?,?)",
        "family(X0,?,?):-family(?,X0,?)",
        "family(X0,?,?):-family(?,?,X0)",
        "family(?,X0,?):-family(X0,?,?)",//
        "family(?,X0,?):-family(?,X0,?)",
        "family(?,X0,?):-family(?,?,X0)",
        "family(?,?,X0):-family(X0,?,?)",
        "family(?,?,X0):-family(?,X0,?)",
        "family(?,?,X0):-family(?,?,X0)",
        "family(X0,?,?):-mother(X0,?)",
        "family(X0,?,?):-mother(?,X0)",
        "family(?,X0,?):-mother(X0,?)",
        "family(?,X0,?):-mother(?,X0)",
        "family(?,?,X0):-mother(X0,?)",
        "family(?,?,X0):-mother(?,X0)",
        "family(X0,?,?):-father(X0,?)",
        "family(X0,?,?):-father(?,X0)",
        "family(?,X0,?):-father(X0,?)",
        "family(?,X0,?):-father(?,X0)",
        "family(?,?,X0):-father(X0,?)",
        "family(?,?,X0):-father(?,X0)",
        "family(X0,?,?):-isMale(X0)",
        "family(?,X0,?):-isMale(X0)",//
        "family(?,?,X0):-isMale(X0)",
        "family(1,?,?):-",
        "family(?,2,?):-"
    });

    RelationMiner::nodeMapType node_map;
    RelationMiner::depGraphType dependency_graph;
    std::vector<Rule*> hypothesis;
    std::unordered_set<Record> counterexamples;
    int beamwidth = expected_specs.size() * 2;
    RelationMiner4Test miner(
        *kb, rel_family->id, EvalMetric::Value::CompressionCapacity, beamwidth, 1.0,
        node_map, dependency_graph, hypothesis, counterexamples, std::cout
    );
    Rule* spec_rules[beamwidth] {};
    int actual_spec_cnt = miner.findSpecializations(base_rule, spec_rules);
    std::unordered_set<std::string> actual_specs;
    for (int i = 0; i < beamwidth; i++) {
        Rule* rule = spec_rules[i];
        if (nullptr == rule) {
            break;
        }
        actual_specs.insert(rule->toDumpString(rel_names));
    }
    EXPECT_EQ(expected_specs.size(), actual_spec_cnt);
    EXPECT_EQ(expected_specs, actual_specs);

    for (Fingerprint* const& fp: cache) {
        delete fp;
    }
    for (std::pair<sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabuMap) {
        delete kv.first;
        delete kv.second;
    }
    for (int i = 0; i < beamwidth; i++) {
        Rule* rule = spec_rules[i];
        if (nullptr == rule) {
            break;
        }
        delete rule;
    }
}

TEST_F(TestRelationMiner, TestFindSpecialization2) {
    SimpleRelation* rel_family = kb->getRelation("family");
    SimpleRelation* rel_father = kb->getRelation("father");
    SimpleRelation* rel_mother = kb->getRelation("mother");
    SimpleRelation* rel_is_male = kb->getRelation("isMale");
    EXPECT_NE(rel_family, nullptr);
    EXPECT_NE(rel_mother, nullptr);
    EXPECT_NE(rel_mother, nullptr);
    EXPECT_NE(rel_is_male, nullptr);

    Rule::fingerprintCacheType cache;
    Rule::tabuMapType tabuMap;
    BareRule base_rule(rel_family->id, 3, cache, tabuMap);
    EXPECT_EQ(base_rule.specializeCase4(rel_father->id, 2, 0, 0, 1), UpdateStatus::Normal);
    const char* rel_names[4] {relNames[0].c_str(), relNames[1].c_str(), relNames[2].c_str(), relNames[3].c_str()};
    EXPECT_STREQ("family(?,X0,?):-father(X0,?)", base_rule.toDumpString(rel_names).c_str());

    std::unordered_set<std::string> expected_specs({
        "family(X0,X0,?):-father(X0,?)",
        "family(?,X0,X0):-father(X0,?)",
        "family(?,X0,?):-father(X0,X0)",
        "family(?,X0,?):-father(X0,?),family(X0,?,?)",
        "family(?,X0,?):-father(X0,?),family(?,X0,?)",
        "family(?,X0,?):-father(X0,?),family(?,?,X0)",
        "family(?,X0,?):-father(X0,?),mother(X0,?)",
        "family(?,X0,?):-father(X0,?),mother(?,X0)",
        "family(?,X0,?):-father(X0,?),father(X0,?)",
        "family(?,X0,?):-father(X0,?),father(?,X0)",
        "family(?,X0,?):-father(X0,?),isMale(X0)",
        "family(X1,X0,X1):-father(X0,?)",
        "family(X1,X0,?):-father(X0,X1)",
        "family(?,X0,X1):-father(X0,X1)",
        "family(X1,X0,?):-father(X0,?),family(X1,?,?)",
        "family(X1,X0,?):-father(X0,?),family(?,X1,?)",
        "family(X1,X0,?):-father(X0,?),family(?,?,X1)",
        "family(?,X0,X1):-father(X0,?),family(X1,?,?)",
        "family(?,X0,X1):-father(X0,?),family(?,X1,?)",
        "family(?,X0,X1):-father(X0,?),family(?,?,X1)",
        "family(?,X0,?):-father(X0,X1),family(X1,?,?)",
        "family(?,X0,?):-father(X0,X1),family(?,X1,?)",
        "family(?,X0,?):-father(X0,X1),family(?,?,X1)",
        "family(X1,X0,?):-father(X0,?),father(X1,?)",
        "family(X1,X0,?):-father(X0,?),father(?,X1)",
        "family(?,X0,X1):-father(X0,?),father(X1,?)",
        "family(?,X0,X1):-father(X0,?),father(?,X1)",
        "family(?,X0,?):-father(X0,X1),father(X1,?)",
        "family(?,X0,?):-father(X0,X1),father(?,X1)",
        "family(X1,X0,?):-father(X0,?),mother(X1,?)",
        "family(X1,X0,?):-father(X0,?),mother(?,X1)",
        "family(?,X0,X1):-father(X0,?),mother(X1,?)",
        "family(?,X0,X1):-father(X0,?),mother(?,X1)",
        "family(?,X0,?):-father(X0,X1),mother(X1,?)",
        "family(?,X0,?):-father(X0,X1),mother(?,X1)",
        "family(X1,X0,?):-father(X0,?),isMale(X1)",
        "family(?,X0,X1):-father(X0,?),isMale(X1)",
        "family(?,X0,?):-father(X0,X1),isMale(X1)",
        "family(1,X0,?):-father(X0,?)"
    });

    RelationMiner::nodeMapType node_map;
    RelationMiner::depGraphType dependency_graph;
    std::vector<Rule*> hypothesis;
    std::unordered_set<Record> counterexamples;
    int beamwidth = expected_specs.size() * 2;
    RelationMiner4Test miner(
        *kb, rel_family->id, EvalMetric::Value::CompressionCapacity, beamwidth, 1.0,
        node_map, dependency_graph, hypothesis, counterexamples, std::cout
    );
    Rule* spec_rules[beamwidth]{};
    int actual_spec_cnt = miner.findSpecializations(base_rule, spec_rules);
    std::unordered_set<std::string> actual_specs;
    for (int i = 0; i < beamwidth; i++) {
        Rule* rule = spec_rules[i];
        if (nullptr == rule) {
            break;
        }
        actual_specs.insert(rule->toDumpString(rel_names));
    }
    EXPECT_EQ(expected_specs.size(), actual_spec_cnt);
    EXPECT_EQ(expected_specs, actual_specs);

    for (Fingerprint* const& fp: cache) {
        delete fp;
    }
    for (std::pair<sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabuMap) {
        delete kv.first;
        delete kv.second;
    }
    for (int i = 0; i < beamwidth; i++) {
        Rule* rule = spec_rules[i];
        if (nullptr == rule) {
            break;
        }
        delete rule;
    }
}

TEST_F(TestRelationMiner, TestFindGeneralization) {
    SimpleRelation* rel_family = kb->getRelation("family");
    SimpleRelation* rel_father = kb->getRelation("father");
    SimpleRelation* rel_mother = kb->getRelation("mother");
    SimpleRelation* rel_is_male = kb->getRelation("isMale");
    EXPECT_NE(rel_family, nullptr);
    EXPECT_NE(rel_mother, nullptr);
    EXPECT_NE(rel_mother, nullptr);
    EXPECT_NE(rel_is_male, nullptr);

    Rule::fingerprintCacheType cache;
    Rule::tabuMapType tabuMap;
    BareRule base_rule(rel_family->id, 3, cache, tabuMap);
    EXPECT_EQ(base_rule.specializeCase4(rel_father->id, 2, 0, 0, 1), UpdateStatus::Normal);
    EXPECT_EQ(base_rule.specializeCase4(rel_mother->id, 2, 0, 0, 0), UpdateStatus::Normal);
    EXPECT_EQ(base_rule.specializeCase3(0, 2, 1, 1), UpdateStatus::Normal);
    EXPECT_EQ(base_rule.specializeCase2(rel_is_male->id, 1, 0, 2), UpdateStatus::Normal);
    const char* rel_names[4] {relNames[0].c_str(), relNames[1].c_str(), relNames[2].c_str(), relNames[3].c_str()};
    EXPECT_STREQ("family(X1,X0,X2):-father(X0,X2),mother(X1,?),isMale(X2)", base_rule.toDumpString(rel_names).c_str());

    std::unordered_set<std::string> expected_specs({
        "family(?,X0,X1):-father(X0,X1),isMale(X1)",
        "family(X1,?,X0):-father(?,X0),mother(X1,?),isMale(X0)",
        "family(X1,X0,?):-father(X0,X2),mother(X1,?),isMale(X2)",
        // "family(X1,X0,X2):-father(X0,X2),mother(X1,?)",  // This rule is in the cache
        "family(X1,X0,X2):-father(X0,?),mother(X1,?),isMale(X2)"
    });

    RelationMiner::nodeMapType node_map;
    RelationMiner::depGraphType dependency_graph;
    std::vector<Rule*> hypothesis;
    std::unordered_set<Record> counterexamples;
    int beamwidth = expected_specs.size() * 2;
    RelationMiner4Test miner(
        *kb, rel_family->id, EvalMetric::Value::CompressionCapacity, beamwidth, 1.0,
        node_map, dependency_graph, hypothesis, counterexamples, std::cout
    );
    Rule* spec_rules[beamwidth]{};
    int actual_spec_cnt = miner.findGeneralizations(base_rule, spec_rules);
    std::unordered_set<std::string> actual_specs;
    for (int i = 0; i < beamwidth; i++) {
        Rule* rule = spec_rules[i];
        if (nullptr == rule) {
            break;
        }
        actual_specs.insert(rule->toDumpString(rel_names));
    }
    EXPECT_EQ(expected_specs.size(), actual_spec_cnt);
    EXPECT_EQ(expected_specs, actual_specs);

    for (Fingerprint* const& fp: cache) {
        delete fp;
    }
    for (std::pair<sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabuMap) {
        delete kv.first;
        delete kv.second;
    }
    for (int i = 0; i < beamwidth; i++) {
        Rule* rule = spec_rules[i];
        if (nullptr == rule) {
            break;
        }
        delete rule;
    }
}
