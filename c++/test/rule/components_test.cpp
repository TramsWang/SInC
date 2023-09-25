#include <gtest/gtest.h>
#include "../../src/rule/components.h"
#include "../../src/util/util.h"
#include <vector>

using namespace sinc;

TEST(TestEvalMetric, TestGetters) {
    EXPECT_EQ(EvalMetric::getSymbol(EvalMetric::CompressionRatio), std::string("τ"));
    EXPECT_EQ(EvalMetric::getSymbol(EvalMetric::CompressionCapacity), std::string("δ"));
    EXPECT_EQ(EvalMetric::getSymbol(EvalMetric::InfoGain), std::string("h"));

    EXPECT_EQ(EvalMetric::getDescription(EvalMetric::CompressionRatio), std::string("Compression Rate"));
    EXPECT_EQ(EvalMetric::getDescription(EvalMetric::CompressionCapacity), std::string("Compression Capacity"));
    EXPECT_EQ(EvalMetric::getDescription(EvalMetric::InfoGain), std::string("Information Gain"));

    EXPECT_EQ(EvalMetric::getBySymbol("τ"), EvalMetric::CompressionRatio);
    EXPECT_EQ(EvalMetric::getBySymbol("δ"), EvalMetric::CompressionCapacity);
    EXPECT_EQ(EvalMetric::getBySymbol("h"), EvalMetric::InfoGain);
}

TEST(TestEval, TestConstructor) {
    Eval e1(0, 0, 0);
    EXPECT_EQ(e1.getAllEtls(), 0);
    EXPECT_EQ(e1.getPosEtls(), 0);
    EXPECT_EQ(e1.getNegEtls(), 0);
    EXPECT_EQ(e1.getRuleLength(), 0);
    EXPECT_EQ(e1.value(EvalMetric::CompressionRatio), 0.0);
    EXPECT_EQ(e1.value(EvalMetric::CompressionCapacity), 0.0);
    EXPECT_EQ(e1.value(EvalMetric::InfoGain), -1.0/0.0);
    EXPECT_FALSE(e1.useful());

    Eval e2(6, 7, 3);
    EXPECT_EQ(e2.getAllEtls(), 7);
    EXPECT_EQ(e2.getPosEtls(), 6);
    EXPECT_EQ(e2.getNegEtls(), 1);
    EXPECT_EQ(e2.getRuleLength(), 3);
    EXPECT_EQ(e2.value(EvalMetric::CompressionRatio), 0.6);
    EXPECT_EQ(e2.value(EvalMetric::CompressionCapacity), 2);
    EXPECT_TRUE(e2.useful());
}

TEST(TestPredicateWithClass, TestComparisons) {
    ArgIndicator*** const arg_indicators = new ArgIndicator**[5] {
        new ArgIndicator*[2]{ArgIndicator::variableIndicator(0, 0), ArgIndicator::variableIndicator(1, 1)},
        new ArgIndicator*[2]{ArgIndicator::constantIndicator(0), ArgIndicator::variableIndicator(1, 2)},

        new ArgIndicator*[2]{ArgIndicator::variableIndicator(0, 0), ArgIndicator::variableIndicator(1, 1)},
        new ArgIndicator*[2]{ArgIndicator::constantIndicator(0), ArgIndicator::variableIndicator(1, 2)},

        new ArgIndicator*[2]{ArgIndicator::variableIndicator(1, 0), ArgIndicator::variableIndicator(1, 1)},
    };
    Fingerprint::equivalenceClassType eqc11;
    EXPECT_EQ(eqc11.add(arg_indicators[0][0]), 1);
    EXPECT_EQ(eqc11.add(arg_indicators[0][1]), 1);
    Fingerprint::equivalenceClassType eqc12;
    EXPECT_EQ(eqc12.add(arg_indicators[1][0]), 1);
    EXPECT_EQ(eqc12.add(arg_indicators[1][1]), 1);
    Fingerprint::equivalenceClassType eqc21;
    EXPECT_EQ(eqc21.add(arg_indicators[2][0]), 1);
    EXPECT_EQ(eqc21.add(arg_indicators[2][1]), 1);
    Fingerprint::equivalenceClassType eqc22;
    EXPECT_EQ(eqc22.add(arg_indicators[3][0]), 1);
    EXPECT_EQ(eqc22.add(arg_indicators[3][1]), 1);
    Fingerprint::equivalenceClassType eqc31;
    EXPECT_EQ(eqc31.add(arg_indicators[0][0]), 1);
    EXPECT_EQ(eqc31.add(arg_indicators[0][1]), 1);
    Fingerprint::equivalenceClassType eqc32;
    EXPECT_EQ(eqc32.add(arg_indicators[4][0]), 1);
    EXPECT_EQ(eqc32.add(arg_indicators[4][1]), 1);

    PredicateWithClass p1(0, 2);
    p1.classArgs[0] = &eqc11;
    p1.classArgs[1] = &eqc12;
    PredicateWithClass p2(0, 2);
    p2.classArgs[0] = &eqc21;
    p2.classArgs[1] = &eqc22;
    PredicateWithClass p3(0, 2);
    p3.classArgs[0] = &eqc31;
    p3.classArgs[1] = &eqc32;

    EXPECT_TRUE(p1 == p2);
    EXPECT_FALSE(p1 == p3);
    EXPECT_FALSE(p3 == p2);
    EXPECT_EQ(p1.hash(), p2.hash());
    EXPECT_NE(p1.hash(), p3.hash());

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 2; j++) {
            delete arg_indicators[i][j];
        }
        delete[] arg_indicators[i];
    }
    delete[] arg_indicators;
}

class TestFingerprint: public testing::Test {
protected:
    std::string rule2String(std::vector<Predicate> const& rule) {
        std::ostringstream os;
        os << rule[0].toString() << ":-";
        if (1 < rule.size()) {
            os << rule[1].toString();
            for (int i = 2; i < rule.size(); i++) {
                os << ',' << rule[i].toString();
            }
        }
        return os.str();
    }
};

TEST_F(TestFingerprint, TestConstructor) {
    /* 1(X, c) <- 2(X, Y), 3(Y, Z, e), 1(Z, ?), 1(X, ?) */
    /* Equivalence Classes:
     *    X: {1[0], 2[0], 1[0]}
     *    c: {1[1], c}
     *    Y: {2[1], 3[0]}
     *    Z: {3[1], 1[0]}
     *    e: {3[2], e}
     *    ?: {1[1]}
     *    ?: {1[1]}
     */
    Predicate head(1, new int[2]{ARG_VARIABLE(0), ARG_CONSTANT(3)}, 2);
    Predicate body1(2, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    Predicate body2(3, new int[3]{ARG_VARIABLE(1), ARG_VARIABLE(2), ARG_CONSTANT(5)}, 3);
    Predicate body3(1, new int[2]{ARG_VARIABLE(2), ARG_EMPTY_VALUE}, 2);
    Predicate body4(1, new int[2]{ARG_VARIABLE(0), ARG_EMPTY_VALUE}, 2);
    std::vector<Predicate> rule;
    rule.push_back(head);
    rule.push_back(body1);
    rule.push_back(body2);
    rule.push_back(body3);
    rule.push_back(body4);
    head.maintainArgs();
    body1.maintainArgs();
    body2.maintainArgs();
    body3.maintainArgs();
    body4.maintainArgs();
    EXPECT_EQ(rule2String(rule), "1(X0,3):-2(X0,X1),3(X1,X2,5),1(X2,?),1(X0,?)");

    Fingerprint::equivalenceClassType* eqc_x = new Fingerprint::equivalenceClassType();
    EXPECT_EQ(eqc_x->add(ArgIndicator::variableIndicator(1, 0)), 1);
    EXPECT_EQ(eqc_x->add(ArgIndicator::variableIndicator(2, 0)), 1);
    ArgIndicator* arg = ArgIndicator::variableIndicator(1, 0);
    EXPECT_EQ(eqc_x->add(arg), 2);
    delete arg;
    Fingerprint::equivalenceClassType* eqc_c = new Fingerprint::equivalenceClassType();
    EXPECT_EQ(eqc_c->add(ArgIndicator::variableIndicator(1, 1)), 1);
    EXPECT_EQ(eqc_c->add(ArgIndicator::constantIndicator(3)), 1);
    Fingerprint::equivalenceClassType* eqc_y = new Fingerprint::equivalenceClassType();
    EXPECT_EQ(eqc_y->add(ArgIndicator::variableIndicator(2, 1)), 1);
    EXPECT_EQ(eqc_y->add(ArgIndicator::variableIndicator(3, 0)), 1);
    Fingerprint::equivalenceClassType* eqc_z = new Fingerprint::equivalenceClassType();
    EXPECT_EQ(eqc_z->add(ArgIndicator::variableIndicator(3, 1)), 1);
    EXPECT_EQ(eqc_z->add(ArgIndicator::variableIndicator(1, 0)), 1);
    Fingerprint::equivalenceClassType* eqc_e = new Fingerprint::equivalenceClassType();
    EXPECT_EQ(eqc_e->add(ArgIndicator::variableIndicator(3, 2)), 1);
    EXPECT_EQ(eqc_e->add(ArgIndicator::constantIndicator(5)), 1);
    Fingerprint::equivalenceClassType* eqc_uv1 = new Fingerprint::equivalenceClassType();
    EXPECT_EQ(eqc_uv1->add(ArgIndicator::variableIndicator(1, 1)), 1);
    Fingerprint::equivalenceClassType* eqc_uv2 = new Fingerprint::equivalenceClassType();
    EXPECT_EQ(eqc_uv2->add(ArgIndicator::variableIndicator(1, 1)), 1);
    MultiSet<Fingerprint::equivalenceClassType*> expected_eqc_set;
    EXPECT_EQ(expected_eqc_set.add(eqc_x), 1);
    EXPECT_EQ(expected_eqc_set.add(eqc_c), 1);
    EXPECT_EQ(expected_eqc_set.add(eqc_y), 1);
    EXPECT_EQ(expected_eqc_set.add(eqc_z), 1);
    EXPECT_EQ(expected_eqc_set.add(eqc_e), 1);
    EXPECT_EQ(expected_eqc_set.add(eqc_uv1), 1);
    EXPECT_EQ(expected_eqc_set.add(eqc_uv2), 2);

    PredicateWithClass pwc_head(1, 2);
    pwc_head.classArgs[0] = eqc_x;
    pwc_head.classArgs[1] = eqc_c;
    PredicateWithClass pwc_body1(2, 2);
    pwc_body1.classArgs[0] = eqc_x;
    pwc_body1.classArgs[1] = eqc_y;
    PredicateWithClass pwc_body2(3, 3);
    pwc_body2.classArgs[0] = eqc_y;
    pwc_body2.classArgs[1] = eqc_z;
    pwc_body2.classArgs[2] = eqc_e;
    PredicateWithClass pwc_body3(1, 2);
    pwc_body3.classArgs[0] = eqc_z;
    pwc_body3.classArgs[1] = eqc_uv1;
    PredicateWithClass pwc_body4(1, 2);
    pwc_body4.classArgs[0] = eqc_x;
    pwc_body4.classArgs[1] = eqc_uv2;

    Fingerprint fingerprint(rule);
    EXPECT_EQ(fingerprint.getEquivalenceClasses(), expected_eqc_set);
    const std::vector<PredicateWithClass>& classed_structure = fingerprint.getClassedStructure();
    EXPECT_EQ(classed_structure[0], pwc_head);
    EXPECT_EQ(classed_structure[1], pwc_body1);
    EXPECT_EQ(classed_structure[2], pwc_body2);
    EXPECT_EQ(classed_structure[3], pwc_body3);
    EXPECT_EQ(classed_structure[4], pwc_body4);

    Fingerprint::releaseEquivalenceClass(eqc_x);
    Fingerprint::releaseEquivalenceClass(eqc_y);
    Fingerprint::releaseEquivalenceClass(eqc_z);
    Fingerprint::releaseEquivalenceClass(eqc_c);
    Fingerprint::releaseEquivalenceClass(eqc_e);
    Fingerprint::releaseEquivalenceClass(eqc_uv1);
    Fingerprint::releaseEquivalenceClass(eqc_uv2);
}

TEST_F(TestFingerprint, TestDup) {
    std::string rule_pair_strs[2][2] {{
            "1(X0,X1):-1(X1,X0)",
            "1(X1,X0):-1(X0,X1)"
        }, {
            "1(X0,X2):-2(X0,X1),3(X1,X2)",
            "1(X1,X0):-3(X2,X0),2(X1,X2)"
        }
    };
    std::vector<Predicate> rule_pairs[2][2];
    rule_pairs[0][0].emplace_back(1, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule_pairs[0][0].emplace_back(1, new int[2]{ARG_VARIABLE(1), ARG_VARIABLE(0)}, 2);
    rule_pairs[0][1].emplace_back(1, new int[2]{ARG_VARIABLE(1), ARG_VARIABLE(0)}, 2);
    rule_pairs[0][1].emplace_back(1, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);

    rule_pairs[1][0].emplace_back(1, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(2)}, 2);
    rule_pairs[1][0].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule_pairs[1][0].emplace_back(3, new int[2]{ARG_VARIABLE(1), ARG_VARIABLE(2)}, 2);
    rule_pairs[1][1].emplace_back(1, new int[2]{ARG_VARIABLE(1), ARG_VARIABLE(0)}, 2);
    rule_pairs[1][1].emplace_back(3, new int[2]{ARG_VARIABLE(2), ARG_VARIABLE(0)}, 2);
    rule_pairs[1][1].emplace_back(2, new int[2]{ARG_VARIABLE(1), ARG_VARIABLE(2)}, 2);
    for (int i = 0; i < 2; i++) {
        std::vector<Predicate>& rule1 = rule_pairs[i][0];
        std::vector<Predicate>& rule2 = rule_pairs[i][1];
        for (Predicate & p: rule1) {
            p.maintainArgs();
        }
        for (Predicate & p: rule2) {
            p.maintainArgs();
        }
        EXPECT_EQ(rule2String(rule1), rule_pair_strs[i][0]);
        EXPECT_EQ(rule2String(rule2), rule_pair_strs[i][1]);
        Fingerprint fp1(rule1);
        Fingerprint fp2(rule2);
        EXPECT_TRUE(fp1 == fp2) << '@' << i << ", fp1 -x-> fp2";
        EXPECT_TRUE(fp2 == fp1) << '@' << i << ", fp2 -x-> fp1";
    }
}

TEST_F(TestFingerprint, TestNotDup) {
    std::string rule_pair_strs[5][2] {{
            // If no independent fragment is introduced in the search, this will not happen in real world cases
            "1(X0):-1(X1)",
            "1(X0):-1(X1),1(X2)"
        }, {
            "1(X0):-2(X0,X1),3(X1,3)",
            "1(X0):-2(X0,X1),3(3,X1)"
        }, {
            // [Not discovered]
            "2(X0,X1):-3(X0,X0),3(?,X1)",
            "2(X0,X1):-3(X0,X1),3(?,X0)",
        }, {
            // [Not discovered]
            "2(X0,X1):-3(X0,?),3(X2,X1),3(?,X2)",
            "2(X0,X1):-3(X0,X2),3(?,X1),3(X2,?)"
        }, {
            "1(X0,X2):-2(X0,X1),3(X1,X2)",
            "1(X1,X0):-3(X2,X0),2(X2,X1)"
        }
    };
    std::vector<Predicate> rule_pairs[5][2];
    rule_pairs[0][0].emplace_back(1, new int[1]{ARG_VARIABLE(0)}, 1);
    rule_pairs[0][0].emplace_back(1, new int[1]{ARG_VARIABLE(1)}, 1);
    rule_pairs[0][1].emplace_back(1, new int[1]{ARG_VARIABLE(0)}, 1);
    rule_pairs[0][1].emplace_back(1, new int[1]{ARG_VARIABLE(1)}, 1);
    rule_pairs[0][1].emplace_back(1, new int[1]{ARG_VARIABLE(2)}, 1);

    rule_pairs[1][0].emplace_back(1, new int[1]{ARG_VARIABLE(0)}, 1);
    rule_pairs[1][0].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule_pairs[1][0].emplace_back(3, new int[2]{ARG_VARIABLE(1), ARG_CONSTANT(3)}, 2);
    rule_pairs[1][1].emplace_back(1, new int[1]{ARG_VARIABLE(0)}, 1);
    rule_pairs[1][1].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule_pairs[1][1].emplace_back(3, new int[2]{ARG_CONSTANT(3), ARG_VARIABLE(1)}, 2);

    rule_pairs[2][0].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule_pairs[2][0].emplace_back(3, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(0)}, 2);
    rule_pairs[2][0].emplace_back(3, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(1)}, 2);
    rule_pairs[2][1].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule_pairs[2][1].emplace_back(3, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule_pairs[2][1].emplace_back(3, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(0)}, 2);

    rule_pairs[3][0].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule_pairs[3][0].emplace_back(3, new int[2]{ARG_VARIABLE(0), ARG_EMPTY_VALUE}, 2);
    rule_pairs[3][0].emplace_back(3, new int[2]{ARG_VARIABLE(2), ARG_VARIABLE(1)}, 2);
    rule_pairs[3][0].emplace_back(3, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(2)}, 2);
    rule_pairs[3][1].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule_pairs[3][1].emplace_back(3, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(2)}, 2);
    rule_pairs[3][1].emplace_back(3, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(1)}, 2);
    rule_pairs[3][1].emplace_back(3, new int[2]{ARG_VARIABLE(2), ARG_EMPTY_VALUE}, 2);

    rule_pairs[4][0].emplace_back(1, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(2)}, 2);
    rule_pairs[4][0].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule_pairs[4][0].emplace_back(3, new int[2]{ARG_VARIABLE(1), ARG_VARIABLE(2)}, 2);
    rule_pairs[4][1].emplace_back(1, new int[2]{ARG_VARIABLE(1), ARG_VARIABLE(0)}, 2);
    rule_pairs[4][1].emplace_back(3, new int[2]{ARG_VARIABLE(2), ARG_VARIABLE(0)}, 2);
    rule_pairs[4][1].emplace_back(2, new int[2]{ARG_VARIABLE(2), ARG_VARIABLE(1)}, 2);

    for (int i = 0; i < 5; i++) {
        std::vector<Predicate>& rule1 = rule_pairs[i][0];
        std::vector<Predicate>& rule2 = rule_pairs[i][1];
        for (Predicate & p: rule1) {
            p.maintainArgs();
        }
        for (Predicate & p: rule2) {
            p.maintainArgs();
        }
        EXPECT_EQ(rule2String(rule1), rule_pair_strs[i][0]);
        EXPECT_EQ(rule2String(rule2), rule_pair_strs[i][1]);
        Fingerprint fp1(rule1);
        Fingerprint fp2(rule2);
        switch (i) {
            case 0:
            case 1:
            case 4:
                EXPECT_FALSE(fp1 == fp2) << '@' << i << "fp1 -x-> fp2";
                EXPECT_FALSE(fp2 == fp1) << '@' << i << "fp2 -x-> fp1";
                break;
            case 2:
            case 3:
                EXPECT_TRUE(fp1 == fp2) << '@' << i << "fp1 -x-> fp2";
                EXPECT_TRUE(fp2 == fp1) << '@' << i << "fp2 -x-> fp1";
                break;
        }
    }
}

TEST_F(TestFingerprint, TestGeneralizationOf) {
    std::string rule_pair_strs[3][2] {{
            "2(X0,?):-1(?,X0)",
            "2(X0,X1):-1(X1,X0)"
        }, {
            "2(X0,?):-1(?,X0)",
            "2(X0,?):-1(X1,X0),2(X1,?)"
        }, {
            "2(X0,?):-1(?,X0)",
            "2(X0,?):-1(3,X0)"
        }
    };
    std::vector<Predicate> rule_pairs[3][2];
    rule_pairs[0][0].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_EMPTY_VALUE}, 2);
    rule_pairs[0][0].emplace_back(1, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(0)}, 2);
    rule_pairs[0][1].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule_pairs[0][1].emplace_back(1, new int[2]{ARG_VARIABLE(1), ARG_VARIABLE(0)}, 2);

    rule_pairs[1][0].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_EMPTY_VALUE}, 2);
    rule_pairs[1][0].emplace_back(1, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(0)}, 2);
    rule_pairs[1][1].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_EMPTY_VALUE}, 2);
    rule_pairs[1][1].emplace_back(1, new int[2]{ARG_VARIABLE(1), ARG_VARIABLE(0)}, 2);
    rule_pairs[1][1].emplace_back(2, new int[2]{ARG_VARIABLE(1), ARG_EMPTY_VALUE}, 2);

    rule_pairs[2][0].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_EMPTY_VALUE}, 2);
    rule_pairs[2][0].emplace_back(1, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(0)}, 2);
    rule_pairs[2][1].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_EMPTY_VALUE}, 2);
    rule_pairs[2][1].emplace_back(1, new int[2]{ARG_CONSTANT(3), ARG_VARIABLE(0)}, 2);

    for (int i = 0; i < 3; i++) {
        std::vector<Predicate>& rule1 = rule_pairs[i][0];
        std::vector<Predicate>& rule2 = rule_pairs[i][1];
        for (Predicate & p: rule1) {
            p.maintainArgs();
        }
        for (Predicate & p: rule2) {
            p.maintainArgs();
        }
        EXPECT_EQ(rule2String(rule1), rule_pair_strs[i][0]);
        EXPECT_EQ(rule2String(rule2), rule_pair_strs[i][1]);
        Fingerprint fp1(rule1);
        Fingerprint fp2(rule2);
        EXPECT_TRUE(fp1.generalizationOf(fp2)) << '@' << i << "fp1 -x-> fp2";
        EXPECT_FALSE(fp2.generalizationOf(fp1)) << '@' << i << "fp2 --> fp1";
    }
}

TEST_F(TestFingerprint, TestNotGeneralizationOf) {
    std::string rule_pair_strs[3][2] {{
            "2(X0,X1):-3(X0,X1)",
            "2(X0,X1):-3(X0,?),3(?,X1)"
        }, {
            "2(X0,?):-2(X0,X0)",
            "2(X0,X1):-2(X1,X0)"
        }, {
            "2(X0,X0):-",
            "2(X0,X1):-2(X1,X0)"
        }
    };
    std::vector<Predicate> rule_pairs[3][2];
    rule_pairs[0][0].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule_pairs[0][0].emplace_back(3, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule_pairs[0][1].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule_pairs[0][1].emplace_back(3, new int[2]{ARG_VARIABLE(0), ARG_EMPTY_VALUE}, 2);
    rule_pairs[0][1].emplace_back(3, new int[2]{ARG_EMPTY_VALUE, ARG_VARIABLE(1)}, 2);

    rule_pairs[1][0].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_EMPTY_VALUE}, 2);
    rule_pairs[1][0].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(0)}, 2);
    rule_pairs[1][1].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule_pairs[1][1].emplace_back(2, new int[2]{ARG_VARIABLE(1), ARG_VARIABLE(0)}, 2);

    rule_pairs[2][0].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(0)}, 2);
    rule_pairs[2][1].emplace_back(2, new int[2]{ARG_VARIABLE(0), ARG_VARIABLE(1)}, 2);
    rule_pairs[2][1].emplace_back(2, new int[2]{ARG_VARIABLE(1), ARG_VARIABLE(0)}, 2);

    for (int i = 0; i < 3; i++) {
        std::vector<Predicate>& rule1 = rule_pairs[i][0];
        std::vector<Predicate>& rule2 = rule_pairs[i][1];
        for (Predicate & p: rule1) {
            p.maintainArgs();
        }
        for (Predicate & p: rule2) {
            p.maintainArgs();
        }
        EXPECT_EQ(rule2String(rule1), rule_pair_strs[i][0]);
        EXPECT_EQ(rule2String(rule2), rule_pair_strs[i][1]);
        Fingerprint fp1(rule1);
        Fingerprint fp2(rule2);
        EXPECT_FALSE(fp1.generalizationOf(fp2)) << '@' << i << "fp1 --> fp2";
        EXPECT_FALSE(fp2.generalizationOf(fp1)) << '@' << i << "fp2 --> fp1";
    }
}