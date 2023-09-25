#include <gtest/gtest.h>
#include "../../src/util/common.h"

using namespace sinc;

TEST(TestCommon, TestMacros) {
    EXPECT_EQ(ARG_CONSTANT(1), 1);
    EXPECT_EQ(ARG_CONSTANT(2), 2);
    EXPECT_EQ(ARG_CONSTANT(3), 3);

    EXPECT_EQ(ARG_VARIABLE(0), 0x80000000);
    EXPECT_EQ(ARG_VARIABLE(7), 0x80000007);
    EXPECT_EQ(ARG_VARIABLE(10), 0x8000000a);

    EXPECT_TRUE(ARG_IS_EMPTY(0));
    EXPECT_FALSE(ARG_IS_EMPTY(ARG_VARIABLE(0)));

    EXPECT_TRUE(ARG_IS_VARIABLE(ARG_VARIABLE(0)));
    EXPECT_TRUE(ARG_IS_VARIABLE(ARG_VARIABLE(5)));
    EXPECT_FALSE(ARG_IS_VARIABLE(ARG_CONSTANT(1)));
    EXPECT_FALSE(ARG_IS_VARIABLE(ARG_CONSTANT(10)));

    EXPECT_TRUE(ARG_IS_CONSTANT(ARG_CONSTANT(1)));
    EXPECT_TRUE(ARG_IS_CONSTANT(ARG_CONSTANT(6)));
    EXPECT_FALSE(ARG_IS_CONSTANT(ARG_VARIABLE(0)));
    EXPECT_FALSE(ARG_IS_CONSTANT(ARG_VARIABLE(15)));

    EXPECT_EQ(ARG_DECODE(ARG_CONSTANT(1)), 1);
    EXPECT_EQ(ARG_DECODE(ARG_CONSTANT(99)), 99);
    EXPECT_EQ(ARG_DECODE(ARG_CONSTANT(1013)), 1013);
    EXPECT_EQ(ARG_DECODE(ARG_VARIABLE(0)), 0);
    EXPECT_EQ(ARG_DECODE(ARG_VARIABLE(128)), 128);
    EXPECT_EQ(ARG_DECODE(ARG_VARIABLE(825)), 825);
}

TEST(TestCommon, TestRecord) {
    /* Test Constructor */
    int a1[3]{0};
    Record r1(a1, 3);
    EXPECT_EQ(r1.getArity(), 3);
    EXPECT_EQ(r1.getArgs()[0], 0);
    EXPECT_EQ(r1.getArgs()[1], 0);
    EXPECT_EQ(r1.getArgs()[2], 0);

    int a2[2]{10, 13};
    Record r2(a2, 2);
    EXPECT_EQ(r2.getArity(), 2);
    EXPECT_EQ(r2.getArgs()[0], 10);
    EXPECT_EQ(r2.getArgs()[1], 13);

    int a3[2]{10, 13};
    Record *r3 = new Record(a3, 2);
    EXPECT_EQ(r3->getArity(), 2);
    EXPECT_EQ(r3->getArgs()[0], 10);
    EXPECT_EQ(r3->getArgs()[1], 13);

    int a4[3]{10, 13, 0};
    Record *r4 = new Record(a4, 3);
    EXPECT_EQ(r4->getArity(), 3);
    EXPECT_EQ(r4->getArgs()[0], 10);
    EXPECT_EQ(r4->getArgs()[1], 13);
    EXPECT_EQ(r4->getArgs()[2], 0);

    /* Test Equivalence */
    EXPECT_FALSE(r1 == r2);
    EXPECT_FALSE(r1 == *r3);
    EXPECT_FALSE(*r3 == *r4);
    EXPECT_TRUE(r2 == *r3);

    std::equal_to<Record*> equals;
    EXPECT_TRUE(equals(&r2, r3));
    EXPECT_TRUE(equals(&r1, &r1));
    EXPECT_FALSE(equals(&r1, &r2));
    EXPECT_FALSE(equals(&r2, r4));
    EXPECT_FALSE(equals(r3, r4));

    /* Test Hash */
    std::hash<Record> hasher;
    std::hash<Record*> pointer_hasher;
    EXPECT_EQ(hasher(r1), pointer_hasher(&r1));
    EXPECT_EQ(hasher(r2), pointer_hasher(&r2));
    EXPECT_EQ(hasher(*r3), pointer_hasher(r3));
    EXPECT_EQ(hasher(*r4), pointer_hasher(r4));
    EXPECT_EQ(hasher(r2), pointer_hasher(r3));
    EXPECT_NE(hasher(r1), hasher(r2));
    EXPECT_NE(hasher(r1), pointer_hasher(r4));
    EXPECT_NE(hasher(r2), pointer_hasher(r4));

    delete r3;
    delete r4;
}

TEST(TestCommon, TestPredicate) {
    /* Test Constructor */
    Predicate p1(1, new int[2]{1, 2}, 2);
    EXPECT_EQ(p1.getPredSymbol(), 1);
    EXPECT_EQ(p1.getArgs()[0], 1);
    EXPECT_EQ(p1.getArgs()[1], 2);
    EXPECT_EQ(p1.getArity(), 2);
    p1.maintainArgs();

    Predicate p2(1, 2);
    EXPECT_EQ(p2.getPredSymbol(), 1);
    EXPECT_EQ(p2.getArgs()[0], 0);
    EXPECT_EQ(p2.getArgs()[1], 0);
    EXPECT_EQ(p2.getArity(), 2);

    Predicate *p3 = new Predicate(1, new int[2]{1, 2}, 2);
    EXPECT_EQ(p3->getPredSymbol(), 1);
    EXPECT_EQ(p3->getArgs()[0], 1);
    EXPECT_EQ(p3->getArgs()[1], 2);
    EXPECT_EQ(p3->getArity(), 2);
    p3->maintainArgs();

    Predicate *p4 = new Predicate(p1);
    EXPECT_EQ(p4->getPredSymbol(), 1);
    EXPECT_EQ(p4->getArgs()[0], 1);
    EXPECT_EQ(p4->getArgs()[1], 2);
    EXPECT_EQ(p4->getArity(), 2);

    /* Test Equivalence */
    EXPECT_TRUE(p1 == *p3);
    EXPECT_TRUE(p1 == *p4);
    EXPECT_FALSE(p1 == p2);
    EXPECT_FALSE(*p4 == p2);

    std::equal_to<Predicate*> equals;
    EXPECT_TRUE(equals(&p1, p3));
    EXPECT_TRUE(equals(&p1, p4));
    EXPECT_FALSE(equals(&p1, &p2));
    EXPECT_FALSE(equals(p4, &p2));

    /* Test Hash */
    std::hash<Predicate> hasher;
    std::hash<Predicate*> pointer_hasher;
    EXPECT_EQ(hasher(p1), pointer_hasher(&p1));
    EXPECT_EQ(hasher(p2), pointer_hasher(&p2));
    EXPECT_EQ(hasher(*p3), pointer_hasher(p3));
    EXPECT_EQ(hasher(*p4), pointer_hasher(p4));
    EXPECT_NE(hasher(p1), hasher(p2));
    EXPECT_EQ(hasher(p1), pointer_hasher(p3));
    EXPECT_EQ(hasher(p1), pointer_hasher(p4));

    /* Test toString */
    EXPECT_EQ(p1.toString(), "1(1,2)");
    p2.getArgs()[1] = 5;
    EXPECT_EQ(p2.toString(), "1(?,5)");
    EXPECT_EQ(p3->toString(), "1(1,2)");
    EXPECT_EQ(p4->toString(), "1(1,2)");

    delete p3;
    delete p4;
}

TEST(TestCommon, TestPredicateVector) {
    std::vector<Predicate> v;
    v.emplace_back(1, 1);
    v.emplace_back(2, 2);
    v.emplace_back(3, 3);
    v.emplace_back(4, 4);

    std::vector<Predicate>::iterator itr = v.begin();
    itr++;
    while (itr != v.end()) {
        if (itr->getPredSymbol() == 2) {
            itr = v.erase(itr);
        } else {
            itr++;
        }
    }

    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v[0].getPredSymbol(), 1);
    EXPECT_EQ(v[1].getPredSymbol(), 3);
    EXPECT_EQ(v[2].getPredSymbol(), 4);
}

TEST(TestCommon, TestSincException) {
    std::string str = "Some Test STR";
    SincException exp(str);
    EXPECT_STREQ(exp.what(), str.c_str());
}

TEST(TestCommon, TestInterruptionSignal) {
    std::string str = "Interruption Test STR";
    InterruptionSignal sgn(str);
    EXPECT_STREQ(sgn.what(), str.c_str());
}

TEST(TestCommon, TestParsedArg) {
    /* Test Construction */
    ParsedArg *c1 = ParsedArg::constant("c");
    EXPECT_STREQ(c1->getName(), "c");
    EXPECT_EQ(c1->getId(), 0);
    EXPECT_TRUE(c1->isConstant());
    EXPECT_FALSE(c1->isVariable());
    EXPECT_EQ(c1->toString(), "c");

    ParsedArg *c2 = ParsedArg::constant("c");
    EXPECT_STREQ(c2->getName(), "c");
    EXPECT_EQ(c2->getId(), 0);
    EXPECT_TRUE(c2->isConstant());
    EXPECT_FALSE(c2->isVariable());
    EXPECT_EQ(c2->toString(), "c");

    ParsedArg *c3 = ParsedArg::constant("d");
    EXPECT_STREQ(c3->getName(), "d");
    EXPECT_EQ(c3->getId(), 0);
    EXPECT_TRUE(c3->isConstant());
    EXPECT_FALSE(c3->isVariable());
    EXPECT_EQ(c3->toString(), "d");

    ParsedArg *v1 = ParsedArg::variable(0);
    EXPECT_EQ(v1->getName(), nullptr);
    EXPECT_EQ(v1->getId(), 0);
    EXPECT_FALSE(v1->isConstant());
    EXPECT_TRUE(v1->isVariable());
    EXPECT_EQ(v1->toString(), "X0");

    ParsedArg *v2 = ParsedArg::variable(0);
    EXPECT_EQ(v2->getName(), nullptr);
    EXPECT_EQ(v2->getId(), 0);
    EXPECT_FALSE(v2->isConstant());
    EXPECT_TRUE(v2->isVariable());
    EXPECT_EQ(v2->toString(), "X0");

    ParsedArg *v3 = ParsedArg::variable(10);
    EXPECT_EQ(v3->getName(), nullptr);
    EXPECT_EQ(v3->getId(), 10);
    EXPECT_FALSE(v3->isConstant());
    EXPECT_TRUE(v3->isVariable());
    EXPECT_EQ(v3->toString(), "X10");

    /* Test Equivalence */
    std::equal_to<ParsedArg*> equals;
    EXPECT_TRUE(equals(c1, c2));
    EXPECT_FALSE(equals(c1, c3));
    EXPECT_FALSE(equals(c2, v1));
    EXPECT_FALSE(equals(c3, v2));
    EXPECT_FALSE(equals(c1, v3));
    EXPECT_TRUE(equals(v1, v2));
    EXPECT_FALSE(equals(v1, v3));

    /* Test Hash */
    std::hash<ParsedArg*> hasher;
    EXPECT_EQ(hasher(c1), hasher(c2));
    EXPECT_NE(hasher(c1), hasher(c3));
    EXPECT_NE(hasher(c2), hasher(v1));
    EXPECT_NE(hasher(c3), hasher(v2));
    EXPECT_NE(hasher(c1), hasher(v3));
    EXPECT_EQ(hasher(v1), hasher(v2));
    EXPECT_NE(hasher(v2), hasher(v3));

    delete c1;
    delete c2;
    delete c3;
    delete v1;
    delete v2;
    delete v3;
}

void expectParsedPredArgEq(const ParsedPred &p, ParsedArg** const args, int const arity) {
    ASSERT_EQ(p.getArity(), arity);
    std::equal_to<ParsedArg*> equals;
    for (int i = 0; i < arity; i++) {
        EXPECT_TRUE((nullptr == args[i] && nullptr == p.getArgs()[i]) || equals(p.getArgs()[i], args[i]));
    }
}

TEST(TestCommon, TestParsedPred) {
    /* Test Constructor */
    ParsedArg** const args = new ParsedArg*[3] {
        ParsedArg::constant("c"), ParsedArg::variable(1), nullptr
    };
    ParsedPred p1("p", new ParsedArg*[3]{ParsedArg::constant("c"), ParsedArg::variable(1), nullptr}, 3);
    EXPECT_EQ(p1.getPredSymbol(), "p");
    expectParsedPredArgEq(p1, args, 3);
    EXPECT_EQ(p1.toString(), "p(c,X1,?)");

    ParsedArg** const args2 = new ParsedArg*[3] {nullptr, nullptr, nullptr};
    ParsedPred p2("p", 3);
    EXPECT_EQ(p2.getPredSymbol(), "p");
    expectParsedPredArgEq(p2, args2, 3);
    EXPECT_EQ(p2.toString(), "p(?,?,?)");

    ParsedPred *p3 = new ParsedPred(p1);
    EXPECT_EQ(p3->getPredSymbol(), "p");
    expectParsedPredArgEq(*p3, args, 3);
    EXPECT_EQ(p3->toString(), "p(c,X1,?)");

    /* Test Equivalence */
    EXPECT_TRUE(p1 == *p3);
    EXPECT_FALSE(p1 == p2);
    std::equal_to<ParsedPred*> equals;
    EXPECT_TRUE(equals(&p1, p3));
    EXPECT_FALSE(equals(&p1, &p2));

    /* Test Hash */
    std::hash<ParsedPred> hasher;
    std::hash<ParsedPred*> pointer_hasher;
    EXPECT_EQ(hasher(p1), pointer_hasher(p3));
    EXPECT_NE(hasher(p1), hasher(p2));

    delete p3;
    delete args[0];
    delete args[1];
    delete[] args;
    delete[] args2;
}

TEST(TestCommon, TestArgLocation) {
    /* Test Constructor */
    ArgLocation a1(0, 1);
    EXPECT_EQ(a1.predIdx, 0);
    EXPECT_EQ(a1.argIdx, 1);

    ArgLocation a2(0, 5);
    EXPECT_EQ(a2.predIdx, 0);
    EXPECT_EQ(a2.argIdx, 5);

    ArgLocation *a3 = new ArgLocation(0, 5);
    EXPECT_EQ(a3->predIdx, 0);
    EXPECT_EQ(a3->argIdx, 5);

    /* Test Equivalence */
    EXPECT_FALSE(a1 == a2);
    EXPECT_TRUE(a2 == *a3);
    std::equal_to<ArgLocation*> equals;
    EXPECT_FALSE(equals(&a1, &a2));
    EXPECT_TRUE(equals(&a2, a3));

    /* Test Hash */
    std::hash<ArgLocation> hasher;
    std::hash<ArgLocation*> pointer_hasher;
    EXPECT_NE(hasher(a1), hasher(a2));
    EXPECT_EQ(hasher(a2), pointer_hasher(a3));

    delete a3;
}

TEST(TestCommon, TestArgIndicator) {
    /* Test Construction */
    ArgIndicator *c1 = ArgIndicator::constantIndicator(1);
    EXPECT_EQ(c1->getFunctor(), 1);
    EXPECT_EQ(c1->getIdx(), -1);

    ArgIndicator *c2 = ArgIndicator::constantIndicator(1);
    EXPECT_EQ(c2->getFunctor(), 1);
    EXPECT_EQ(c2->getIdx(), -1);

    ArgIndicator *c3 = ArgIndicator::constantIndicator(255);
    EXPECT_EQ(c3->getFunctor(), 255);
    EXPECT_EQ(c3->getIdx(), -1);

    ArgIndicator *v1 = ArgIndicator::variableIndicator(5, 3);
    EXPECT_EQ(v1->getFunctor(), 5);
    EXPECT_EQ(v1->getIdx(), 3);

    ArgIndicator *v2 = ArgIndicator::variableIndicator(5, 3);
    EXPECT_EQ(v2->getFunctor(), 5);
    EXPECT_EQ(v2->getIdx(), 3);

    ArgIndicator *v3 = ArgIndicator::variableIndicator(0, 0);
    EXPECT_EQ(v3->getFunctor(), 0);
    EXPECT_EQ(v3->getIdx(), 0);

    /* Test Equivalence */
    std::equal_to<ArgIndicator*> equals;
    EXPECT_TRUE(equals(c1, c2));
    EXPECT_FALSE(equals(c1, c3));
    EXPECT_FALSE(equals(c2, v1));
    EXPECT_FALSE(equals(c3, v2));
    EXPECT_FALSE(equals(c1, v3));
    EXPECT_TRUE(equals(v1, v2));
    EXPECT_FALSE(equals(v1, v3));

    /* Test Hash */
    std::hash<ArgIndicator*> hasher;
    EXPECT_EQ(hasher(c1), hasher(c2));
    EXPECT_NE(hasher(c1), hasher(c3));
    EXPECT_NE(hasher(c2), hasher(v1));
    EXPECT_NE(hasher(c3), hasher(v2));
    EXPECT_NE(hasher(c1), hasher(v3));
    EXPECT_EQ(hasher(v1), hasher(v2));
    EXPECT_NE(hasher(v2), hasher(v3));

    delete c1;
    delete c2;
    delete c3;
    delete v1;
    delete v2;
    delete v3;
}