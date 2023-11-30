#include <gtest/gtest.h>
#include "../../src/base/sinc.h"
#include <filesystem>
#include "../kb/testKbUtils.h"

#define MEM_DIR "/dev/shm"

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
        for (const Fingerprint* const& fp: cache) {
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
        for (const Fingerprint* const& fp: cache) {
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

    void selectAsBeam(Rule* r) override {}
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
        "family(X0,?,X0):-",
        "family(?,X0,X0):-",
        // "family(X0,?,?):-family(X0,?,?)",
        "family(X0,?,?):-family(?,X0,?)",
        "family(X0,?,?):-family(?,?,X0)",
        "family(?,X0,?):-family(X0,?,?)",
        // "family(?,X0,?):-family(?,X0,?)",
        "family(?,X0,?):-family(?,?,X0)",
        "family(?,?,X0):-family(X0,?,?)",
        "family(?,?,X0):-family(?,X0,?)",
        // "family(?,?,X0):-family(?,?,X0)",
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
        "family(?,X0,?):-isMale(X0)",
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

    for (const Fingerprint* const& fp: cache) {
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
        // "family(?,X0,?):-father(X0,?),family(?,X0,?)",
        "family(?,X0,?):-father(X0,?),family(?,?,X0)",
        "family(?,X0,?):-father(X0,?),mother(X0,?)",
        "family(?,X0,?):-father(X0,?),mother(?,X0)",
        "family(?,X0,?):-father(X0,?),father(X0,?)",
        "family(?,X0,?):-father(X0,?),father(?,X0)",
        "family(?,X0,?):-father(X0,?),isMale(X0)",
        "family(X1,X0,X1):-father(X0,?)",
        "family(X1,X0,?):-father(X0,X1)",
        "family(?,X0,X1):-father(X0,X1)",
        // "family(X1,X0,?):-father(X0,?),family(X1,?,?)",
        "family(X1,X0,?):-father(X0,?),family(?,X1,?)",
        "family(X1,X0,?):-father(X0,?),family(?,?,X1)",
        "family(?,X0,X1):-father(X0,?),family(X1,?,?)",
        "family(?,X0,X1):-father(X0,?),family(?,X1,?)",
        // "family(?,X0,X1):-father(X0,?),family(?,?,X1)",
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

    for (const Fingerprint* const& fp: cache) {
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

    for (const Fingerprint* const& fp: cache) {
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

using sinc::test::TestKbManager;

class TestSinc : public testing::Test {
public:
    static TestKbManager* testKb;

protected:
    static void SetUpTestSuite() {
        testKb = new TestKbManager();
        std::cout << "Create Test KB: " << testKb->getKbPath() << std::endl;
    }

    static void TearDownTestSuite() {
        std::cout << "Remove Test KB: " << testKb->getKbPath() << std::endl;
        testKb->cleanUpKb();
        delete testKb;
        testKb = nullptr;
    }
};

TestKbManager* TestSinc::testKb = nullptr;

class Sinc4Test : public SInC {
public:
    Sinc4Test(const char* basePath, const char* kbName) : SInC(new SincConfig(
        basePath, kbName, MEM_DIR, "Sinc4TestComp", 1, false, 0, 5, EvalMetric::Value::CompressionCapacity, 0.05, 0.25, 1.0, 0,
        "", "", 0, true
    )) {
        // std::cout << "HERE\n";
    }

    ~Sinc4Test() {
        fs::remove_all(fs::path(MEM_DIR) / fs::path("Sinc4TestComp"));
    }

    SimpleKb& getKb() const {
        return *kb;
    }

protected:
    SincRecovery* createRecovery() override {
        return nullptr;
    }

    void finish() override {}

    class RelationMiner4SincTest : public RelationMiner {
    public:
        Rule::fingerprintCacheType cache;
        Rule::tabuMapType tabuMap;

        RelationMiner4SincTest(
                SimpleKb& kb, int const targetRelation, EvalMetric::Value evalMetric, int const beamwidth, double const stopCompressionRatio,
                nodeMapType& predicate2NodeMap, depGraphType& dependencyGraph, std::vector<Rule*>& hypothesis,
                std::unordered_set<Record>& counterexamples, std::ostream& logger
        ) : RelationMiner(
            kb, targetRelation, evalMetric, beamwidth, stopCompressionRatio, predicate2NodeMap, dependencyGraph, hypothesis,
            counterexamples, logger
        ) {
            SimpleRelation* rel_mother = kb.getRelation("mother");
            SimpleRelation* rel_father = kb.getRelation("father");
            SimpleRelation* rel_family = kb.getRelation("family");

            /* Set entailments */
            int* rec = rel_mother->getAllRows()[0]; // (4, 6)
            EXPECT_EQ(rec[0], 4);
            EXPECT_EQ(rec[1], 6);
            entailedRecordLists[rel_mother->id].push_back(rec);
            rec = rel_mother->getAllRows()[1]; // (7, 9)
            EXPECT_EQ(rec[0], 7);
            EXPECT_EQ(rec[1], 9);
            entailedRecordLists[rel_mother->id].push_back(rec);
            rec = rel_father->getAllRows()[0]; // (5, 6)
            EXPECT_EQ(rec[0], 5);
            EXPECT_EQ(rec[1], 6);
            entailedRecordLists[rel_father->id].push_back(rec);
            rec = rel_father->getAllRows()[3]; // (16, 17)
            EXPECT_EQ(rec[0], 16);
            EXPECT_EQ(rec[1], 17);
            entailedRecordLists[rel_father->id].push_back(rec);
            rec = rel_family->getAllRows()[0]; // (4, 5, 6)
            EXPECT_EQ(rec[0], 4);
            EXPECT_EQ(rec[1], 5);
            EXPECT_EQ(rec[2], 6);
            entailedRecordLists[rel_family->id].push_back(rec);

            rec = rel_family->getAllRows()[3]; // (13, 14, 15)
            EXPECT_EQ(rec[0], 13);
            EXPECT_EQ(rec[1], 14);
            EXPECT_EQ(rec[2], 15);
            entailingRecordLists[rel_mother->id].push_back(rec);
            entailingRecordLists[rel_father->id].push_back(rec);
            entailingRecordLists[rel_father->id].push_back(rec);
            rec = rel_father->getAllRows()[2]; // (11, 12)
            EXPECT_EQ(rec[0], 11);
            EXPECT_EQ(rec[1], 12);
            entailingRecordLists[rel_mother->id].push_back(rec);
            entailingRecordLists[rel_family->id].push_back(rec);
            rec = rel_mother->getAllRows()[3]; // (13, 15)
            EXPECT_EQ(rec[0], 13);
            EXPECT_EQ(rec[1], 15);
            entailingRecordLists[rel_family->id].push_back(rec);

            counterexampleLists[rel_family->id].push_back(new int[3] {1, 2, 3});

            /* Add rules to hypotheses */
            /* mother */
            /* mother(X0,X1):-family(X0,?,X1) */
            BareRule* rule_mother1 = new BareRule(rel_mother->id, rel_mother->getTotalCols(), cache, tabuMap);
            rule_mother1->specializeCase4(rel_family->id, rel_family->getTotalCols(), 0, 0, 0);
            rule_mother1->specializeCase3(0, 1, 1, 2);
            rule_mother1->returningEvidence = new EvidenceBatch(2);
            rule_mother1->returningEvidence->predicateSymbolsInRule[0] = rel_mother->id;
            rule_mother1->returningEvidence->predicateSymbolsInRule[1] = rel_family->id;
            rule_mother1->returningEvidence->aritiesInRule[0] = rel_mother->getTotalCols();
            rule_mother1->returningEvidence->aritiesInRule[1] = rel_family->getTotalCols();
            rule_mother1->returningEvidence->evidenceList.push_back(
                new int*[2]{entailedRecordLists[rel_mother->id][0], entailingRecordLists[rel_mother->id][0]}
            );
            rule_mother1->returningCounterexamples = new std::unordered_set<Record>();

            /* mother(8,X0):-father(?,X0) */
            BareRule* rule_mother2 = new BareRule(rel_mother->id, rel_mother->getTotalCols(), cache, tabuMap);
            rule_mother2->specializeCase5(0, 0, 8);
            rule_mother2->specializeCase4(rel_father->id, rel_father->getTotalCols(), 1, 0, 1);
            rule_mother2->returningEvidence = new EvidenceBatch(2);
            rule_mother2->returningEvidence->predicateSymbolsInRule[0] = rel_mother->id;
            rule_mother2->returningEvidence->predicateSymbolsInRule[1] = rel_father->id;
            rule_mother2->returningEvidence->aritiesInRule[0] = rel_mother->getTotalCols();
            rule_mother2->returningEvidence->aritiesInRule[1] = rel_father->getTotalCols();
            rule_mother2->returningEvidence->evidenceList.push_back(
                new int*[2]{entailedRecordLists[rel_mother->id][1], entailingRecordLists[rel_mother->id][1]}
            );
            rule_mother2->returningCounterexamples = new std::unordered_set<Record>();

            hypothesesLists[rel_mother->id].push_back(rule_mother1);
            hypothesesLists[rel_mother->id].push_back(rule_mother2);

            /* father */
            /* father(X0,X1):-family(?,X0,X1) */
            BareRule* rule_father = new BareRule(rel_father->id, rel_father->getTotalCols(), cache, tabuMap);
            rule_father->specializeCase4(rel_family->id, rel_family->getTotalCols(), 1, 0, 0);
            rule_father->specializeCase3(0, 1, 1, 2);
            rule_father->returningEvidence = new EvidenceBatch(2);
            rule_father->returningEvidence->predicateSymbolsInRule[0] = rel_father->id;
            rule_father->returningEvidence->predicateSymbolsInRule[1] = rel_family->id;
            rule_father->returningEvidence->aritiesInRule[0] = rel_father->getTotalCols();
            rule_father->returningEvidence->aritiesInRule[1] = rel_family->getTotalCols();
            rule_father->returningEvidence->evidenceList.push_back(
                new int*[2]{entailedRecordLists[rel_father->id][0], entailingRecordLists[rel_father->id][0]}
            );
            rule_father->returningCounterexamples = new std::unordered_set<Record>();

            hypothesesLists[rel_father->id].push_back(rule_father);

            /* family */
            /* family(X1,X0,X2):-father(X0,X2),mother(X1,X2) */
            BareRule* rule_family = new BareRule(rel_family->id, rel_family->getTotalCols(), cache, tabuMap);
            rule_family->specializeCase4(rel_father->id, rel_father->getTotalCols(), 0, 0, 1);
            rule_family->specializeCase4(rel_mother->id, rel_mother->getTotalCols(), 0, 0, 0);
            rule_family->specializeCase3(1, 1, 2, 1);
            rule_family->specializeCase1(0, 2, 2);
            rule_family->returningEvidence = new EvidenceBatch(3);
            rule_family->returningEvidence->predicateSymbolsInRule[0] = rel_family->id;
            rule_family->returningEvidence->predicateSymbolsInRule[1] = rel_father->id;
            rule_family->returningEvidence->predicateSymbolsInRule[2] = rel_mother->id;
            rule_family->returningEvidence->aritiesInRule[0] = rel_family->getTotalCols();
            rule_family->returningEvidence->aritiesInRule[1] = rel_father->getTotalCols();
            rule_family->returningEvidence->aritiesInRule[2] = rel_mother->getTotalCols();
            rule_family->returningEvidence->evidenceList.push_back(
                new int*[3]{entailedRecordLists[rel_family->id][0], entailingRecordLists[rel_family->id][0], entailingRecordLists[rel_family->id][1]}
            );
            rule_family->returningCounterexamples = new std::unordered_set<Record>();
            rule_family->returningCounterexamples->emplace(counterexampleLists[rel_family->id][0], rel_family->getTotalCols());

            hypothesesLists[rel_family->id].push_back(rule_family);
        }

        ~RelationMiner4SincTest() {
            for (const Fingerprint* const& fp: cache) {
                delete fp;
            }
            for (std::pair<const sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabuMap) {
                delete kv.first;
                delete kv.second;
            }
            SimpleRelation* rel_mother = kb.getRelation("mother");
            SimpleRelation* rel_father = kb.getRelation("father");
            SimpleRelation* rel_family = kb.getRelation("family");
            BareRule* rule_mother1 = hypothesesLists[rel_mother->id][0];
            BareRule* rule_mother2 = hypothesesLists[rel_mother->id][1];
            BareRule* rule_father = hypothesesLists[rel_father->id][0];
            BareRule* rule_family = hypothesesLists[rel_family->id][0];
            if (rel_family->id != targetRelation) {
                delete[] counterexampleLists[rel_family->id][0];
            }
            if (rel_mother->id != targetRelation) {
                delete rule_mother1->returningEvidence;
                delete rule_mother2->returningEvidence;
                delete rule_mother1->returningCounterexamples;
                delete rule_mother2->returningCounterexamples;
                delete rule_mother1;
                delete rule_mother2;
            }
            if (rel_father->id != targetRelation) {
                delete rule_father->returningEvidence;
                delete rule_father->returningCounterexamples;
                delete rule_father;
            }
            if (rel_family->id != targetRelation) {
                delete rule_family->returningEvidence;
                delete rule_family->returningCounterexamples;
                delete rule_family;
            }
            // delete counterexampleLists[rel_family->id][0];   // records in the counterexample set will be released by `compressedKb`
        }

    protected:
        std::vector<BareRule*> hypothesesLists[3];
        std::vector<int*> entailedRecordLists[3];
        std::vector<int*> entailingRecordLists[3];
        std::vector<int*> counterexampleLists[3];
        int idx = 0;

        Rule* getStartRule() override {
            return nullptr; 
        }

        Rule* findRule() {
            /* Mark Entailment */
            if (0 == idx) {
                SimpleRelation* relation = kb.getRelation(targetRelation);
                for (int* const& record: entailedRecordLists[targetRelation]) {
                    relation->setAsEntailed(record);
                }
            }

            std::vector<BareRule*> const& rules = hypothesesLists[targetRelation];
            if (idx < rules.size()) {
                Rule* ret = rules[idx];
                idx++;
                return ret;
            }
            return nullptr;
        }

        int checkThenAddRule(UpdateStatus updateStatus, Rule* const updatedRule, Rule& originalRule, Rule** candidates) override {
            return 1;
        }

        void selectAsBeam(Rule* r) override {}
    };

    RelationMiner* createRelationMiner(int const targetRelationId) override {
        return new RelationMiner4SincTest(
            *kb, targetRelationId, config->evalMetric, config->beamwidth, config->stopCompressionRatio, predicate2NodeMap, dependencyGraph,
            compressedKb->getHypothesis(), compressedKb->getCounterexampleSet(targetRelationId), *logger
        );
    }
};

TEST_F(TestSinc, TestCompression) {
    Sinc4Test sinc(TestKbManager::MEM_DIR_PATH.c_str(), testKb->getKbName());
    sinc.run();

    SimpleKb& kb = sinc.getKb();
    SimpleCompressedKb& ckb = sinc.getCompressedKb();

    EXPECT_EQ(kb.totalRelations(), 3);
    EXPECT_EQ(kb.totalRecords(), 12);
    EXPECT_EQ(kb.totalConstants(), 17);

    /* Check entailment */
    SimpleRelation* rel_mother = kb.getRelation("mother");
    SimpleRelation* rel_father = kb.getRelation("father");
    SimpleRelation* rel_family = kb.getRelation("family");
    int** entailed_records[3];
    entailed_records[rel_mother->id] = new int*[2] {
        new int[2] {4, 6},
        new int[2] {7, 9}
    };
    entailed_records[rel_father->id] = new int*[2] {
        new int[2] {5, 6},
        new int[2] {16, 17}
    };
    entailed_records[rel_family->id] = new int*[1] {
        new int[3] {4, 5, 6}
    };
    EXPECT_EQ(rel_mother->totalEntailedRecords(), 2);
    EXPECT_EQ(rel_father->totalEntailedRecords(), 2);
    EXPECT_EQ(rel_family->totalEntailedRecords(), 1);
    EXPECT_TRUE(rel_mother->isEntailed(entailed_records[rel_mother->id][0]));
    EXPECT_TRUE(rel_mother->isEntailed(entailed_records[rel_mother->id][1]));
    EXPECT_TRUE(rel_father->isEntailed(entailed_records[rel_father->id][0]));
    EXPECT_TRUE(rel_father->isEntailed(entailed_records[rel_father->id][1]));
    EXPECT_TRUE(rel_family->isEntailed(entailed_records[rel_family->id][0]));

    /* Check compressed KB */
    EXPECT_EQ(ckb.totalNecessaryRecords(), 7);
    EXPECT_EQ(ckb.totalFvsRecords(), 0);
    EXPECT_EQ(ckb.totalCounterexamples(), 1);
    EXPECT_EQ(ckb.totalHypothesisSize(), 10);
    EXPECT_EQ(ckb.totalSupplementaryConstants(), 5);    // In this test, numbers 1, 2, and 3 should be taken into consideration

    std::unordered_set<Record>& actual_counterexamples = ckb.getCounterexampleSet(rel_family->id);
    int actual_counterexample_args[3] {1, 2, 3};
    Record actual_counterexample_record(actual_counterexample_args, 3);
    EXPECT_NE(actual_counterexamples.find(actual_counterexample_record), actual_counterexamples.end());

    std::unordered_set<std::string> expect_rules({
        "mother(X0,X1):-family(X0,?,X1)",
        "mother(8,X0):-father(?,X0)",
        "father(X0,X1):-family(?,X0,X1)",
        "family(X1,X0,X2):-father(X0,X2),mother(X1,X2)"
    });
    std::unordered_set<std::string> actual_rules;
    for (Rule* const& rule: ckb.getHypothesis()) {
        actual_rules.insert(rule->toDumpString(kb.getRelationNames()));
    }
    EXPECT_EQ(actual_rules, expect_rules);

    std::vector<int> const& actual_supplementary_constants = ckb.getSupplementaryConstants();
    EXPECT_EQ(actual_supplementary_constants[0], 4);
    EXPECT_EQ(actual_supplementary_constants[1], 5);
    EXPECT_EQ(actual_supplementary_constants[2], 6);
    EXPECT_EQ(actual_supplementary_constants[3], 16);
    EXPECT_EQ(actual_supplementary_constants[4], 17);

    delete[] entailed_records[rel_mother->id][0];
    delete[] entailed_records[rel_mother->id][1];
    delete[] entailed_records[rel_father->id][0];
    delete[] entailed_records[rel_father->id][1];
    delete[] entailed_records[rel_family->id][0];
    delete[] entailed_records[0];
    delete[] entailed_records[1];
    delete[] entailed_records[2];
}