#include <gtest/gtest.h>
#include "../../src/rule/rule.h"
#include "../../src/kb/simpleKb.h"
#include <vector>

using namespace sinc;

TEST(RuleTest, TestParseStructure1) {
    std::string str("p(X0,?,X0):-");
    ParsedPred head("p", new ParsedArg*[3]{ParsedArg::variable(0), nullptr, ParsedArg::variable(0)}, 3);
    std::vector<sinc::ParsedPred *>* actual_structure = Rule::parseStructure(str);

    EXPECT_EQ(actual_structure->size(), 1);
    EXPECT_EQ(*(*actual_structure)[0], head);
    EXPECT_EQ(str, Rule::toString(*actual_structure));

    Rule::releaseParsedStructure(actual_structure);
}

TEST(RuleTest, TestParseStructure2) {
    std::string str("pred(X0,tom,X0):-body(X1),another(X0,?,X1)");
    ParsedPred head("pred", new ParsedArg*[3]{ParsedArg::variable(0), ParsedArg::constant("tom"), ParsedArg::variable(0)}, 3);
    ParsedPred body1("body", new ParsedArg*[1]{ParsedArg::variable(1)}, 1);
    ParsedPred body2("another", new ParsedArg*[3]{ParsedArg::variable(0), nullptr, ParsedArg::variable(1)}, 3);
    std::vector<sinc::ParsedPred *>* actual_structure = Rule::parseStructure(str);

    EXPECT_EQ(actual_structure->size(), 3);
    EXPECT_EQ(*(*actual_structure)[0], head);
    EXPECT_EQ(*(*actual_structure)[1], body1);
    EXPECT_EQ(*(*actual_structure)[2], body2);
    EXPECT_EQ(str, Rule::toString(*actual_structure));

    Rule::releaseParsedStructure(actual_structure);
}

TEST(RuleTest, TestParseStructure3) {
    /* compound(Na, H, C, O3) :- compound(Na, C, O3), compound(H2, O), compound(C, O2) */
    /*          0   1  2  3               0   2  3             4   5            2  6   */
    /*          0   -  2  3               0   2  3             -   -            2  -   */
    /*          0   -  2  1               0   2  1             -   -            2  -   */
    std::string str("compound(Na,H,C,O3):-compound(Na,C,O3),compound(H2,O),compound(C,O2)");
    ParsedPred head("compound", new ParsedArg*[4]{ParsedArg::variable(0), nullptr, ParsedArg::variable(2), ParsedArg::variable(1)}, 4);
    ParsedPred body1("compound", new ParsedArg*[3]{ParsedArg::variable(0), ParsedArg::variable(2), ParsedArg::variable(1)}, 3);
    ParsedPred body2("compound", new ParsedArg*[2]{nullptr, nullptr}, 2);
    ParsedPred body3("compound", new ParsedArg*[2]{ParsedArg::variable(2), nullptr}, 2);
    std::vector<sinc::ParsedPred *>* actual_structure = Rule::parseStructure(str);

    EXPECT_EQ(actual_structure->size(), 4);
    EXPECT_EQ(*(*actual_structure)[0], head);
    EXPECT_EQ(*(*actual_structure)[1], body1);
    EXPECT_EQ(*(*actual_structure)[2], body2);
    EXPECT_EQ(*(*actual_structure)[3], body3);
    EXPECT_EQ("compound(X0,?,X2,X1):-compound(X0,X2,X1),compound(?,?),compound(X2,?)", Rule::toString(*actual_structure));

    Rule::releaseParsedStructure(actual_structure);
}

TEST(RuleTest, TestConstructor) {
    /* 1(X, Y, c) <- 2(X), 3(?, Y), 3(c, X) */
    Rule::MinFactCoverage = 0;
    Rule::fingerprintCacheType cache;
    Rule::tabuMapType tabuMap;
    Rule* r = new BareRule(1, 3, cache, tabuMap);
    ASSERT_STREQ(r->toDumpString().c_str(), "1(?,?,?):-");
    ASSERT_EQ(r->getLength(), MIN_LENGTH);
    ASSERT_EQ(r->numPredicates(), 1);
    ASSERT_EQ(r->usedLimitedVars(), 0);
    ASSERT_EQ(cache.size(), 1);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(2, 1, 0, 0, 0));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,?,?):-2(X0)");
    ASSERT_EQ(r->getLength(), 1);
    ASSERT_EQ(r->numPredicates(), 2);
    ASSERT_EQ(r->usedLimitedVars(), 1);
    ASSERT_EQ(cache.size(), 2);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(3, 2, 1, 0, 1));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X1,?):-2(X0),3(?,X1)");
    ASSERT_EQ(r->getLength(), 2);
    ASSERT_EQ(r->numPredicates(), 3);
    ASSERT_EQ(r->usedLimitedVars(), 2);
    ASSERT_EQ(cache.size(), 3);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase2(3, 2, 1, 0));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X1,?):-2(X0),3(?,X1),3(?,X0)");
    ASSERT_EQ(r->getLength(), 3);
    ASSERT_EQ(r->numPredicates(), 4);
    ASSERT_EQ(r->usedLimitedVars(), 2);
    ASSERT_EQ(cache.size(), 4);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase5(3, 0, 3));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X1,?):-2(X0),3(?,X1),3(3,X0)");
    ASSERT_EQ(r->getLength(), 4);
    ASSERT_EQ(r->numPredicates(), 4);
    ASSERT_EQ(r->usedLimitedVars(), 2);
    ASSERT_EQ(cache.size(), 5);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase5(0, 2, 3));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X1,3):-2(X0),3(?,X1),3(3,X0)");
    ASSERT_EQ(r->getLength(), 5);
    ASSERT_EQ(r->numPredicates(), 4);
    ASSERT_EQ(r->usedLimitedVars(), 2);
    ASSERT_EQ(cache.size(), 6);

    Predicate predicate_head(1, 3);
    predicate_head.setArg(0, ARG_VARIABLE(0));
    predicate_head.setArg(1, ARG_VARIABLE(1));
    predicate_head.setArg(2, ARG_CONSTANT(3));
    Predicate predicate_body1(2, 1);
    predicate_body1.setArg(0, ARG_VARIABLE(0));
    Predicate predicate_body2(3, 2);
    predicate_body2.setArg(1, ARG_VARIABLE(1));
    Predicate predicate_body3(3, 2);
    predicate_body3.setArg(0, ARG_CONSTANT(3));
    predicate_body3.setArg(1, ARG_VARIABLE(0));

    ASSERT_EQ(predicate_head, r->getPredicate(0));
    ASSERT_EQ(predicate_head, r->getHead());
    ASSERT_EQ(predicate_body1, r->getPredicate(1));
    ASSERT_EQ(predicate_body2, r->getPredicate(2));
    ASSERT_EQ(predicate_body3, r->getPredicate(3));

    delete r;
    for (const Fingerprint* const& fp: cache) {
        delete fp;
    }
    for (std::pair<const sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabuMap) {
        delete kv.first;
        delete kv.second;
    }
}

TEST(RuleTest, TestCopyConstructor) {
    Rule::MinFactCoverage = 0;
    Rule::fingerprintCacheType cache;
    Rule::tabuMapType tabu_set;
    Rule* r1 = new BareRule(1, 3, cache, tabu_set);
    ASSERT_EQ(UpdateStatus::Normal, r1->specializeCase4(2, 1, 0, 0, 0));
    ASSERT_EQ(UpdateStatus::Normal, r1->specializeCase4(3, 2, 1, 0, 1));
    ASSERT_EQ(UpdateStatus::Normal, r1->specializeCase2(3, 2, 1, 0));
    ASSERT_STREQ(r1->toDumpString().c_str(), "1(X0,X1,?):-2(X0),3(?,X1),3(?,X0)");
    ASSERT_EQ(3, r1->getLength());
    ASSERT_EQ(4, r1->numPredicates());
    ASSERT_EQ(2, r1->usedLimitedVars());
    ASSERT_EQ(4, cache.size());

    Rule* r2 = r1->clone();
    ASSERT_EQ(UpdateStatus::Normal, r2->generalize(1, 0));
    ASSERT_STREQ(r2->toDumpString().c_str(), "1(X0,X1,?):-3(?,X1),3(?,X0)");
    ASSERT_EQ(UpdateStatus::Normal, r2->generalize(0, 0));
    ASSERT_STREQ(r2->toDumpString().c_str(), "1(?,X0,?):-3(?,X0)");
    ASSERT_EQ(1, r2->getLength());
    ASSERT_EQ(2, r2->numPredicates());
    ASSERT_EQ(1, r2->usedLimitedVars());
    ASSERT_EQ(6, cache.size());

    ASSERT_STREQ(r1->toDumpString().c_str(), "1(X0,X1,?):-2(X0),3(?,X1),3(?,X0)");
    ASSERT_EQ(3, r1->getLength());
    ASSERT_EQ(4, r1->numPredicates());
    ASSERT_EQ(2, r1->usedLimitedVars());
    ASSERT_EQ(6, cache.size());

    delete r1;
    delete r2;
    for (const Fingerprint* const& fp: cache) {
        delete fp;
    }
    for (std::pair<sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabu_set) {
        delete kv.first;
        delete kv.second;
    }
}

TEST(RuleTest, TestSpecCase1) {
    /* 1(X, X, ?) <- 2(X) */
    Rule::MinFactCoverage = 0;
    Rule::fingerprintCacheType cache;
    Rule::tabuMapType tabuMap;
    Rule* r = new BareRule(1, 3, cache, tabuMap);
    ASSERT_STREQ(r->toDumpString().c_str(), "1(?,?,?):-");
    ASSERT_EQ(r->getLength(), MIN_LENGTH);
    ASSERT_EQ(r->numPredicates(), 1);
    ASSERT_EQ(r->usedLimitedVars(), 0);
    ASSERT_EQ(cache.size(), 1);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(2, 1, 0, 0, 0));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,?,?):-2(X0)");
    ASSERT_EQ(r->getLength(), 1);
    ASSERT_EQ(r->numPredicates(), 2);
    ASSERT_EQ(r->usedLimitedVars(), 1);
    ASSERT_EQ(cache.size(), 2);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase1(0, 1, 0));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,?):-2(X0)");
    ASSERT_EQ(r->getLength(), 2);
    ASSERT_EQ(r->numPredicates(), 2);
    ASSERT_EQ(r->usedLimitedVars(), 1);
    ASSERT_EQ(cache.size(), 3);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase1(0, 2, 0));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,X0):-2(X0)");
    ASSERT_EQ(r->getLength(), 3);
    ASSERT_EQ(r->numPredicates(), 2);
    ASSERT_EQ(r->usedLimitedVars(), 1);
    ASSERT_EQ(cache.size(), 4);

    delete r;
    for (const Fingerprint* const& fp: cache) {
        delete fp;
    }
    for (std::pair<sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabuMap) {
        delete kv.first;
        delete kv.second;
    }
}

TEST(RuleTest, TestSpecCase2) {
    /* 1(X0,X0,?):-2(X0),3(?,X0),5(?,?,?,X0,?) */
    Rule::MinFactCoverage = 0;
    Rule::fingerprintCacheType cache;
    Rule::tabuMapType tabuMap;
    Rule* r = new BareRule(1, 3, cache, tabuMap);
    ASSERT_STREQ(r->toDumpString().c_str(), "1(?,?,?):-");
    ASSERT_EQ(r->getLength(), MIN_LENGTH);
    ASSERT_EQ(r->numPredicates(), 1);
    ASSERT_EQ(r->usedLimitedVars(), 0);
    ASSERT_EQ(cache.size(), 1);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase3(0, 0, 0, 1));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,?):-");
    ASSERT_EQ(r->getLength(), 1);
    ASSERT_EQ(r->numPredicates(), 1);
    ASSERT_EQ(r->usedLimitedVars(), 1);
    ASSERT_EQ(cache.size(), 2);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase2(2, 1, 0, 0));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,?):-2(X0)");
    ASSERT_EQ(r->getLength(), 2);
    ASSERT_EQ(r->numPredicates(), 2);
    ASSERT_EQ(r->usedLimitedVars(), 1);
    ASSERT_EQ(cache.size(), 3);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase2(3, 2, 1, 0));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,?):-2(X0),3(?,X0)");
    ASSERT_EQ(r->getLength(), 3);
    ASSERT_EQ(r->numPredicates(), 3);
    ASSERT_EQ(r->usedLimitedVars(), 1);
    ASSERT_EQ(cache.size(), 4);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase2(5, 5, 3, 0));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,?):-2(X0),3(?,X0),5(?,?,?,X0,?)");
    ASSERT_EQ(r->getLength(), 4);
    ASSERT_EQ(r->numPredicates(), 4);
    ASSERT_EQ(r->usedLimitedVars(), 1);
    ASSERT_EQ(cache.size(), 5);

    delete r;
    for (const Fingerprint* const& fp: cache) {
        delete fp;
    }
    for (std::pair<sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabuMap) {
        delete kv.first;
        delete kv.second;
    }
}

TEST(RuleTest, TestSpecCase3) {
    /* 1(X0,X0,X5,X3,X1):-2(X2,X4,X1,X4),2(X5,X2,?,X3) */
    Rule::MinFactCoverage = 0;
    Rule::fingerprintCacheType cache;
    Rule::tabuMapType tabuMap;
    Rule* r = new BareRule(1, 5, cache, tabuMap);
    ASSERT_STREQ(r->toDumpString().c_str(), "1(?,?,?,?,?):-");
    ASSERT_EQ(r->getLength(), MIN_LENGTH);
    ASSERT_EQ(r->numPredicates(), 1);
    ASSERT_EQ(r->usedLimitedVars(), 0);
    ASSERT_EQ(cache.size(), 1);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase3(0, 0, 0, 1));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,?,?,?):-");
    ASSERT_EQ(r->getLength(), 1);
    ASSERT_EQ(r->numPredicates(), 1);
    ASSERT_EQ(r->usedLimitedVars(), 1);
    ASSERT_EQ(cache.size(), 2);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(2, 4, 2, 0, 4));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,?,?,X1):-2(?,?,X1,?)");
    ASSERT_EQ(r->getLength(), 2);
    ASSERT_EQ(r->numPredicates(), 2);
    ASSERT_EQ(r->usedLimitedVars(), 2);
    ASSERT_EQ(cache.size(), 3);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(2, 4, 1, 1, 0));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,?,?,X1):-2(X2,?,X1,?),2(?,X2,?,?)");
    ASSERT_EQ(r->getLength(), 3);
    ASSERT_EQ(r->numPredicates(), 3);
    ASSERT_EQ(r->usedLimitedVars(), 3);
    ASSERT_EQ(cache.size(), 4);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase3(0, 3, 2, 3));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,?,X3,X1):-2(X2,?,X1,?),2(?,X2,?,X3)");
    ASSERT_EQ(r->getLength(), 4);
    ASSERT_EQ(r->numPredicates(), 3);
    ASSERT_EQ(r->usedLimitedVars(), 4);
    ASSERT_EQ(cache.size(), 5);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase3(1, 3, 1, 1));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,?,X3,X1):-2(X2,X4,X1,X4),2(?,X2,?,X3)");
    ASSERT_EQ(r->getLength(), 5);
    ASSERT_EQ(r->numPredicates(), 3);
    ASSERT_EQ(r->usedLimitedVars(), 5);
    ASSERT_EQ(cache.size(), 6);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase3(0, 2, 2, 0));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,X5,X3,X1):-2(X2,X4,X1,X4),2(X5,X2,?,X3)");
    ASSERT_EQ(r->getLength(), 6);
    ASSERT_EQ(r->numPredicates(), 3);
    ASSERT_EQ(r->usedLimitedVars(), 6);
    ASSERT_EQ(cache.size(), 7);

    delete r;
    for (const Fingerprint* const& fp: cache) {
        delete fp;
    }
    for (std::pair<sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabuMap) {
        delete kv.first;
        delete kv.second;
    }
}

TEST(RuleTest, TestSpecCase4) {
    /* 1(X0,X0,?,?,X1):-2(X2,X3,X1,?),2(?,X2,?,?),3(X3) */
    Rule::MinFactCoverage = 0;
    Rule::fingerprintCacheType cache;
    Rule::tabuMapType tabuMap;
    Rule* r = new BareRule(1, 5, cache, tabuMap);
    ASSERT_STREQ(r->toDumpString().c_str(), "1(?,?,?,?,?):-");
    ASSERT_EQ(r->getLength(), MIN_LENGTH);
    ASSERT_EQ(r->numPredicates(), 1);
    ASSERT_EQ(r->usedLimitedVars(), 0);
    ASSERT_EQ(cache.size(), 1);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase3(0, 0, 0, 1));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,?,?,?):-");
    ASSERT_EQ(r->getLength(), 1);
    ASSERT_EQ(r->numPredicates(), 1);
    ASSERT_EQ(r->usedLimitedVars(), 1);
    ASSERT_EQ(cache.size(), 2);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(2, 4, 2, 0, 4));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,?,?,X1):-2(?,?,X1,?)");
    ASSERT_EQ(r->getLength(), 2);
    ASSERT_EQ(r->numPredicates(), 2);
    ASSERT_EQ(r->usedLimitedVars(), 2);
    ASSERT_EQ(cache.size(), 3);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(2, 4, 1, 1, 0));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,?,?,X1):-2(X2,?,X1,?),2(?,X2,?,?)");
    ASSERT_EQ(r->getLength(), 3);
    ASSERT_EQ(r->numPredicates(), 3);
    ASSERT_EQ(r->usedLimitedVars(), 3);
    ASSERT_EQ(cache.size(), 4);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(3, 1, 0, 1, 1));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,?,?,X1):-2(X2,X3,X1,?),2(?,X2,?,?),3(X3)");
    ASSERT_EQ(r->getLength(), 4);
    ASSERT_EQ(r->numPredicates(), 4);
    ASSERT_EQ(r->usedLimitedVars(), 4);
    ASSERT_EQ(cache.size(), 5);

    delete r;
    for (const Fingerprint* const& fp: cache) {
        delete fp;
    }
    for (std::pair<sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabuMap) {
        delete kv.first;
        delete kv.second;
    }
}

TEST(RuleTest, TestSpecCase5) {
    /* 1(X0,X0,X5,X3,X1):-2(X2,X4,X1,X4),2(X5,X2,?,X3) */
    Rule::MinFactCoverage = 0;
    Rule::fingerprintCacheType cache;
    Rule::tabuMapType tabuMap;
    Rule* r = new BareRule(1, 5, cache, tabuMap);
    ASSERT_STREQ(r->toDumpString().c_str(), "1(?,?,?,?,?):-");
    ASSERT_EQ(r->getLength(), MIN_LENGTH);
    ASSERT_EQ(r->numPredicates(), 1);
    ASSERT_EQ(r->usedLimitedVars(), 0);
    ASSERT_EQ(cache.size(), 1);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase5(0, 2, 99));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(?,?,99,?,?):-");
    ASSERT_EQ(r->getLength(), 1);
    ASSERT_EQ(r->numPredicates(), 1);
    ASSERT_EQ(r->usedLimitedVars(), 0);
    ASSERT_EQ(cache.size(), 2);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase3(0, 0, 0, 1));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,99,?,?):-");
    ASSERT_EQ(r->getLength(), 2);
    ASSERT_EQ(r->numPredicates(), 1);
    ASSERT_EQ(r->usedLimitedVars(), 1);
    ASSERT_EQ(cache.size(), 3);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(2, 4, 2, 0, 4));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,99,?,X1):-2(?,?,X1,?)");
    ASSERT_EQ(r->getLength(), 3);
    ASSERT_EQ(r->numPredicates(), 2);
    ASSERT_EQ(r->usedLimitedVars(), 2);
    ASSERT_EQ(cache.size(), 4);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(2, 4, 1, 1, 0));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,99,?,X1):-2(X2,?,X1,?),2(?,X2,?,?)");
    ASSERT_EQ(r->getLength(), 4);
    ASSERT_EQ(r->numPredicates(), 3);
    ASSERT_EQ(r->usedLimitedVars(), 3);
    ASSERT_EQ(cache.size(), 5);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase5(2, 3, 1));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,99,?,X1):-2(X2,?,X1,?),2(?,X2,?,1)");
    ASSERT_EQ(r->getLength(), 5);
    ASSERT_EQ(r->numPredicates(), 3);
    ASSERT_EQ(r->usedLimitedVars(), 3);
    ASSERT_EQ(cache.size(), 6);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase5(0, 3, 3));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,99,3,X1):-2(X2,?,X1,?),2(?,X2,?,1)");
    ASSERT_EQ(r->getLength(), 6);
    ASSERT_EQ(r->numPredicates(), 3);
    ASSERT_EQ(r->usedLimitedVars(), 3);
    ASSERT_EQ(cache.size(), 7);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase5(1, 3, 2));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X0,99,3,X1):-2(X2,?,X1,2),2(?,X2,?,1)");
    ASSERT_EQ(r->getLength(), 7);
    ASSERT_EQ(r->numPredicates(), 3);
    ASSERT_EQ(r->usedLimitedVars(), 3);
    ASSERT_EQ(cache.size(), 8);

    delete r;
    for (const Fingerprint* const& fp: cache) {
        delete fp;
    }
    for (std::pair<sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabuMap) {
        delete kv.first;
        delete kv.second;
    }
}

TEST(RuleTest, TestGeneralize1) {
    /* h(X, Y, c) <- p(X), q(?, Y), q(c, X) */
    Rule::MinFactCoverage = 0;
    Rule::fingerprintCacheType cache;
    Rule::tabuMapType tabu_set;
    Rule* r = new BareRule(1, 3, cache, tabu_set);
    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(2, 1, 0, 0, 0));
    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(3, 2, 1, 0, 1));
    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase2(3, 2, 1, 0));
    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase5(3, 0, 3));
    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase5(0, 2, 3));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X1,3):-2(X0),3(?,X1),3(3,X0)");
    ASSERT_EQ(5, r->getLength());
    ASSERT_EQ(4, r->numPredicates());
    ASSERT_EQ(2, r->usedLimitedVars());
    ASSERT_EQ(6, cache.size());

    for (const Fingerprint* const& fp: cache) {
        delete fp;
    }
    cache.clear();
    ASSERT_EQ(UpdateStatus::Normal, r->generalize(1, 0));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X1,3):-3(?,X1),3(3,X0)");
    ASSERT_EQ(4, r->getLength());
    ASSERT_EQ(3, r->numPredicates());
    ASSERT_EQ(2, r->usedLimitedVars());
    ASSERT_EQ(1, cache.size());

    ASSERT_EQ(UpdateStatus::Normal, r->generalize(2, 0));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X1,3):-3(?,X1),3(?,X0)");
    ASSERT_EQ(3, r->getLength());
    ASSERT_EQ(3, r->numPredicates());
    ASSERT_EQ(2, r->usedLimitedVars());
    ASSERT_EQ(2, cache.size());

    ASSERT_EQ(UpdateStatus::Normal, r->generalize(2, 1));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(?,X0,3):-3(?,X0)");
    ASSERT_EQ(2, r->getLength());
    ASSERT_EQ(2, r->numPredicates());
    ASSERT_EQ(1, r->usedLimitedVars());
    ASSERT_EQ(3, cache.size());

    ASSERT_EQ(UpdateStatus::Normal, r->generalize(0, 2));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(?,X0,?):-3(?,X0)");
    ASSERT_EQ(1, r->getLength());
    ASSERT_EQ(2, r->numPredicates());
    ASSERT_EQ(1, r->usedLimitedVars());
    ASSERT_EQ(4, cache.size());

    ASSERT_EQ(UpdateStatus::Normal, r->generalize(0, 1));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(?,?,?):-");
    ASSERT_EQ(0, r->getLength());
    ASSERT_EQ(1, r->numPredicates());
    ASSERT_EQ(0, r->usedLimitedVars());
    ASSERT_EQ(5, cache.size());
    Predicate head(1, 3);
    ASSERT_EQ(head, r->getHead());

    delete r;
    for (const Fingerprint* const& fp: cache) {
        delete fp;
    }
    for (std::pair<sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabu_set) {
        delete kv.first;
        delete kv.second;
    }
}

TEST(RuleTest, TestGeneralize2) {
    /* h(X, Y, Z) <- p(X), q(Z, Y), q(Z, X) */
    Rule::MinFactCoverage = 0;
    Rule::fingerprintCacheType cache;
    Rule::tabuMapType tabu_set;
    Rule* r = new BareRule(1, 3, cache, tabu_set);
    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(2, 1, 0, 0, 0));
    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(3, 2, 1, 0, 1));
    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase2(3, 2, 1, 0));
    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase3(2, 0, 0, 2));
    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase1(3, 0, 2));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,X1,X2):-2(X0),3(X2,X1),3(X2,X0)");
    ASSERT_EQ(5, r->getLength());
    ASSERT_EQ(4, r->numPredicates());
    ASSERT_EQ(3, r->usedLimitedVars());
    ASSERT_EQ(6, cache.size());

    for (const Fingerprint* const& fp: cache) {
        delete fp;
    }
    cache.clear();
    ASSERT_EQ(UpdateStatus::Normal, r->generalize(0, 0));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(?,X1,X2):-2(X0),3(X2,X1),3(X2,X0)");
    ASSERT_EQ(4, r->getLength());
    ASSERT_EQ(4, r->numPredicates());
    ASSERT_EQ(3, r->usedLimitedVars());
    ASSERT_EQ(1, cache.size());

    ASSERT_EQ(UpdateStatus::Normal, r->generalize(3, 1));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(?,X1,X0):-3(X0,X1),3(X0,?)");
    ASSERT_EQ(3, r->getLength());
    ASSERT_EQ(3, r->numPredicates());
    ASSERT_EQ(2, r->usedLimitedVars());
    ASSERT_EQ(2, cache.size());

    ASSERT_EQ(UpdateStatus::Normal, r->generalize(1, 1));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(?,?,X0):-3(X0,?),3(X0,?)");
    ASSERT_EQ(2, r->getLength());
    ASSERT_EQ(3, r->numPredicates());
    ASSERT_EQ(1, r->usedLimitedVars());
    ASSERT_EQ(3, cache.size());

    ASSERT_EQ(UpdateStatus::Invalid, r->generalize(0, 2));

    delete r;
    for (const Fingerprint* const& fp: cache) {
        delete fp;
    }
    for (std::pair<sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabu_set) {
        delete kv.first;
        delete kv.second;
    }
}

TEST(RuleTest, TestToString) {
    Rule::MinFactCoverage = 0;
    int*** const rows = new int**[4] {
        new int*[1] {new int[1]{1}},
        new int*[1] {new int[1]{1}},
        new int*[1] {new int[1]{1}},
        new int*[1] {new int[1]{1}},
    };
    std::string names[4] {"o", "h", "p", "q"};
    int arities[4] {1, 1, 1, 1};
    int totalRows[4] {1, 1, 1, 1};
    SimpleKb kb("test", rows, names, arities, totalRows, 4);

    /* h(X, Y, c) <- p(X), q(?, Y), q(c, X) */
    const char* name_strs[4] {"o", "h", "p", "q"};
    Rule::fingerprintCacheType cache;
    Rule::tabuMapType tabuMap;
    Rule* r = new BareRule(1, 3, cache, tabuMap);
    ASSERT_STREQ(r->toDumpString(name_strs).c_str(), "h(?,?,?):-");
    ASSERT_EQ(r->getLength(), MIN_LENGTH);
    ASSERT_EQ(r->numPredicates(), 1);
    ASSERT_EQ(r->usedLimitedVars(), 0);
    ASSERT_EQ(cache.size(), 1);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(2, 1, 0, 0, 0));
    ASSERT_STREQ(r->toDumpString(name_strs).c_str(), "h(X0,?,?):-p(X0)");
    ASSERT_EQ(r->getLength(), 1);
    ASSERT_EQ(r->numPredicates(), 2);
    ASSERT_EQ(r->usedLimitedVars(), 1);
    ASSERT_EQ(cache.size(), 2);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(3, 2, 1, 0, 1));
    ASSERT_STREQ(r->toDumpString(name_strs).c_str(), "h(X0,X1,?):-p(X0),q(?,X1)");
    ASSERT_EQ(r->getLength(), 2);
    ASSERT_EQ(r->numPredicates(), 3);
    ASSERT_EQ(r->usedLimitedVars(), 2);
    ASSERT_EQ(cache.size(), 3);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase2(3, 2, 1, 0));
    ASSERT_STREQ(r->toDumpString(name_strs).c_str(), "h(X0,X1,?):-p(X0),q(?,X1),q(?,X0)");
    ASSERT_EQ(r->getLength(), 3);
    ASSERT_EQ(r->numPredicates(), 4);
    ASSERT_EQ(r->usedLimitedVars(), 2);
    ASSERT_EQ(cache.size(), 4);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase5(3, 0, 3));
    ASSERT_STREQ(r->toDumpString(name_strs).c_str(), "h(X0,X1,?):-p(X0),q(?,X1),q(3,X0)");
    ASSERT_EQ(r->getLength(), 4);
    ASSERT_EQ(r->numPredicates(), 4);
    ASSERT_EQ(r->usedLimitedVars(), 2);
    ASSERT_EQ(cache.size(), 5);

    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase5(0, 2, 3));
    ASSERT_STREQ(r->toDumpString(name_strs).c_str(), "h(X0,X1,3):-p(X0),q(?,X1),q(3,X0)");
    ASSERT_EQ(r->getLength(), 5);
    ASSERT_EQ(r->numPredicates(), 4);
    ASSERT_EQ(r->usedLimitedVars(), 2);
    ASSERT_EQ(cache.size(), 6);

    Predicate predicate_head(1, 3);
    predicate_head.setArg(0, ARG_VARIABLE(0));
    predicate_head.setArg(1, ARG_VARIABLE(1));
    predicate_head.setArg(2, ARG_CONSTANT(3));
    Predicate predicate_body1(2, 1);
    predicate_body1.setArg(0, ARG_VARIABLE(0));
    Predicate predicate_body2(3, 2);
    predicate_body2.setArg(1, ARG_VARIABLE(1));
    Predicate predicate_body3(3, 2);
    predicate_body3.setArg(0, ARG_CONSTANT(3));
    predicate_body3.setArg(1, ARG_VARIABLE(0));

    ASSERT_EQ(predicate_head, r->getPredicate(0));
    ASSERT_EQ(predicate_head, r->getHead());
    ASSERT_EQ(predicate_body1, r->getPredicate(1));
    ASSERT_EQ(predicate_body2, r->getPredicate(2));
    ASSERT_EQ(predicate_body3, r->getPredicate(3));

    for (int i = 0; i < 4; i++) {
        delete[] rows[i][0];
        delete[] rows[i];
    }
    delete[] rows;
    delete r;
    for (const Fingerprint* const& fp: cache) {
        delete fp;
    }
    for (std::pair<sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabuMap) {
        delete kv.first;
        delete kv.second;
    }
}

TEST(RuleTest, TestInvalid) {
    Rule::MinFactCoverage = 0;
    Rule::fingerprintCacheType cache;
    Rule::tabuMapType tabuMap;
    Rule* r = new BareRule(1, 3, cache, tabuMap);
    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(2, 2, 0, 0, 0));
    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(3, 3, 1, 1, 1));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,?,?):-2(X0,X1),3(?,X1,?)");

    Rule* r2 = r->clone();
    ASSERT_EQ(UpdateStatus::Invalid, r2->generalize(0, 0));
    ASSERT_STREQ(r2->toDumpString().c_str(), "1(?,?,?):-2(?,X0),3(?,X0,?)");

    Rule* r3 = r->clone();
    for (const Fingerprint* const& fp: cache) {
        delete fp;
    }
    cache.clear();
    ASSERT_EQ(UpdateStatus::Invalid, r3->generalize(1, 0));
    ASSERT_STREQ(r3->toDumpString().c_str(), "1(?,?,?):-2(?,X0),3(?,X0,?)");

    Rule* r4 = r->clone();
    ASSERT_EQ(UpdateStatus::Normal, r4->specializeCase1(0, 2, 0));
    ASSERT_STREQ(r4->toDumpString().c_str(), "1(X0,?,X0):-2(X0,X1),3(?,X1,?)");
    ASSERT_EQ(UpdateStatus::Invalid, r4->generalize(1, 0));

    Rule* r5 = r->clone();
    ASSERT_EQ(UpdateStatus::Normal, r5->specializeCase1(0, 2, 1));
    ASSERT_STREQ(r5->toDumpString().c_str(), "1(X0,?,X1):-2(X0,X1),3(?,X1,?)");
    ASSERT_EQ(UpdateStatus::Normal, r5->generalize(1, 0));
    ASSERT_STREQ(r5->toDumpString().c_str(), "1(?,?,X0):-2(?,X0),3(?,X0,?)");

    delete r;
    delete r2;
    delete r3;
    delete r4;
    delete r5;
    for (const Fingerprint* const& fp: cache) {
        delete fp;
    }
    for (std::pair<sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabuMap) {
        delete kv.first;
        delete kv.second;
    }
}

TEST(RuleTest, TestCacheHit) {
    Rule::MinFactCoverage = 0;
    Rule::fingerprintCacheType cache;
    Rule::tabuMapType tabuMap;
    Rule* r = new BareRule(1, 3, cache, tabuMap);
    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(2, 2, 0, 0, 0));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,?,?):-2(X0,?)");
    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(3, 3, 1, 0, 2));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,?,X1):-2(X0,?),3(?,X1,?)");
    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase3(1, 1, 2, 0));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,?,X1):-2(X0,X2),3(X2,X1,?)");

    Rule* r2 = new BareRule(1, 3, cache, tabuMap);
    ASSERT_EQ(UpdateStatus::Duplicated, r2->specializeCase4(2, 2, 0, 0, 0));

    Rule* r3 = new BareRule(1, 3, cache, tabuMap);
    ASSERT_EQ(UpdateStatus::Normal, r3->specializeCase4(3, 3, 1, 0, 2));
    ASSERT_STREQ(r3->toDumpString().c_str(), "1(?,?,X0):-3(?,X0,?)");
    Rule* r4 = r3->clone();
    ASSERT_EQ(UpdateStatus::Duplicated, r3->specializeCase4(2, 2, 0, 0, 0));
    ASSERT_STREQ(r3->toDumpString().c_str(), "1(X1,?,X0):-3(?,X0,?),2(X1,?)");

    ASSERT_EQ(UpdateStatus::Normal, r4->specializeCase4(2, 2, 1, 1, 0));
    ASSERT_STREQ(r4->toDumpString().c_str(), "1(?,?,X0):-3(X1,X0,?),2(?,X1)");
    ASSERT_EQ(UpdateStatus::Duplicated, r4->specializeCase3(0, 0, 2, 0));
    ASSERT_STREQ(r4->toDumpString().c_str(), "1(X2,?,X0):-3(X1,X0,?),2(X2,X1)");

    delete r;
    delete r2;
    delete r3;
    delete r4;
    for (const Fingerprint* const& fp: cache) {
        delete fp;
    }
    for (std::pair<sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabuMap) {
        delete kv.first;
        delete kv.second;
    }
}

TEST(RuleTest, TestInsufficientCoverageAndTabuHit) {
    Rule::fingerprintCacheType cache;
    Rule::tabuMapType tabuMap;
    BareRule* r = new BareRule(1, 3, cache, tabuMap);
    r->coverage = 0.5;
    ASSERT_EQ(UpdateStatus::Normal, r->specializeCase4(2, 2, 0, 0, 0));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,?,?):-2(X0,?)");
    BareRule* r2 = r->clone();
    r->coverage = 0;
    ASSERT_EQ(UpdateStatus::InsufficientCoverage, r->specializeCase4(3, 3, 1, 0, 2));
    ASSERT_STREQ(r->toDumpString().c_str(), "1(X0,?,X1):-2(X0,?),3(?,X1,?)");

    ASSERT_EQ(UpdateStatus::Normal, r2->specializeCase4(3, 3, 0, 1, 1));
    ASSERT_STREQ(r2->toDumpString().c_str(), "1(X0,?,?):-2(X0,X1),3(X1,?,?)");
    ASSERT_EQ(UpdateStatus::TabuPruned, r2->specializeCase3(0, 2, 2, 1));
    ASSERT_STREQ(r2->toDumpString().c_str(), "1(X0,?,X2):-2(X0,X1),3(X1,X2,?)");

    delete r;
    delete r2;
    for (const Fingerprint* const& fp: cache) {
        delete fp;
    }
    for (std::pair<sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabuMap) {
        delete kv.first;
        delete kv.second;
    }
}
