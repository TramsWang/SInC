#include <gtest/gtest.h>
#include "../../src/impl/sincWithCache.h"
#include <filesystem>

#define MEM_DIR "/dev/shm"

using namespace sinc;

class TestCacheFragment : public testing::Test {
protected:
    static int*** relations;
    static std::string relNames[2];
    static int arities[2];
    static int totalRows[2];
    static const char* kbName;
    static SimpleKb* kb;
    static int NumP;
    static int NumQ;

    static void SetUpTestSuite() {
        std::cout << "Set up test KB" << std::endl;
        relations = new int**[2] {
            new int*[8] {   // p
                new int[3] {1, 1, 1},
                new int[3] {1, 1, 2},
                new int[3] {1, 2, 3},
                new int[3] {2, 1, 3},
                new int[3] {4, 4, 6},
                new int[3] {5, 5, 1},
                new int[3] {1, 3, 2},
                new int[3] {2, 4, 4},
            },
            new int*[7] {   // q
                new int[3] {1, 1, 2},
                new int[3] {3, 1, 4},
                new int[3] {2, 4, 1},
                new int[3] {2, 7, 8},
                new int[3] {5, 1, 4},
                new int[3] {6, 7, 2},
                new int[3] {3, 3, 9},
            }
        };
        kb = new SimpleKb(kbName, relations, relNames, arities, totalRows, 2);
        NumP = kb->getRelation("p")->id;
        NumQ = kb->getRelation("q")->id;
    }

    static void TearDownTestSuite() {
        std::cout << "Destroy test KB" << std::endl;
        delete kb;
        for (int i = 0; i < 7; i++) {
            delete[] relations[0][i];
            delete[] relations[1][i];
        }
        delete[] relations[0][7];
        delete[] relations[0];
        delete[] relations[1];
        delete[] relations;
    }

    void checkEntries(CacheFragment::entriesType& expectedEntries, CacheFragment& actualFragment) {
        ASSERT_EQ(expectedEntries.size(), actualFragment.getEntries().size());
        ASSERT_EQ(expectedEntries[0]->size(), actualFragment.getPartAssignedRule().size());
        for (int i = 0; i < actualFragment.getPartAssignedRule().size(); i++) {
            ASSERT_EQ(
                    (*expectedEntries[0])[i]->getTotalCols(),
                    (*(actualFragment.getEntries()[0]))[i]->getTotalCols()
            );
        }
        for (CacheFragment::entryType* const& expected_entry: expectedEntries) {
            for (CompliedBlock* const& cb : *expected_entry) {
                cb->buildIndices();
            }
        }
        for (CacheFragment::entryType* const& actual_entry: actualFragment.getEntries()) {
            for (CompliedBlock* const& cb: *actual_entry) {
                cb->buildIndices();
            }
        }
        for (CacheFragment::entryType* const& expected_entry: expectedEntries) {
            bool found = false;
            for (CacheFragment::entryType* const& actual_entry: actualFragment.getEntries()) {
                bool entry_equals = true;
                for (int i = 0; i < expected_entry->size(); i++) {
                    if (!cbEqual(*((*expected_entry)[i]), *((*actual_entry)[i]))) {
                        entry_equals = false;
                        break;
                    }
                }
                if (entry_equals) {
                    found = true;
                    break;
                }
            }
            ASSERT_TRUE(found);
        }
    }

    bool cbEqual(CompliedBlock const& expectedCb, CompliedBlock const& actualCb) {
        if (expectedCb.getTotalRows() != actualCb.getTotalRows()) {
            return false;
        }
        if (expectedCb.getTotalCols() != actualCb.getTotalCols()) {
            return false;
        }
        std::unordered_set<Record> expected_rows;
        std::unordered_set<Record> actual_rows;
        for (int i = 0; i < expectedCb.getTotalRows(); i++) {
            expected_rows.emplace(expectedCb.getComplianceSet()[i], expectedCb.getTotalCols());
            actual_rows.emplace(actualCb.getComplianceSet()[i], actualCb.getTotalCols());
        }
        return expected_rows == actual_rows;
    }

    bool tableEqual(IntTable const& expectedTable, IntTable const& actualTable) {
        if (expectedTable.getTotalRows() != actualTable.getTotalRows()) {
            return false;
        }
        if (expectedTable.getTotalCols() != actualTable.getTotalCols()) {
            return false;
        }
        const int* const* expected_rows = expectedTable.getAllRows();
        const int* const* actual_rows = actualTable.getAllRows();
        int cols = expectedTable.getTotalCols();
        for (int i = 0; i < expectedTable.getTotalRows(); i++) {
            const int* expected_row = expected_rows[i];
            const int* actual_row = actual_rows[i];
            for (int j = 0; j < cols; j++) {
                if (expected_row[j] != actual_row[j]) {
                    return false;
                }
            }
        }
        return true;
    }

    std::string rule2String(std::vector<Predicate> const& rule) {
        std::ostringstream os;
        os << rule[0].toString(kb->getRelationNames());
        for (int i = 1; i < rule.size(); i++) {
            os << ',' << rule[i].toString(kb->getRelationNames());
        }
        return os.str();
    }

    void clearEntries(CacheFragment::entriesType& entries) {
        for (CacheFragment::entryType* entry: entries) {
            delete entry;
        }
        entries.clear();
    }

    void checkCombinations(
        int** const expCombinations, int const arity, std::unordered_set<Record>* const actualCombinations, int const numCombinations
    ) {
        EXPECT_EQ(actualCombinations->size(), numCombinations);
        for (int i = 0; i < numCombinations; i++) {
            Record r(expCombinations[i], arity);
            EXPECT_TRUE(actualCombinations->find(r) != actualCombinations->end());
        }
    }

    void releaseCombinationSet(std::unordered_set<Record>* const combinations) {
        for (Record const& r: *combinations) {
            delete[] r.getArgs();
        }
        delete combinations;
    }
};

int*** TestCacheFragment::relations = nullptr;
std::string TestCacheFragment::relNames[2] {"p", "q"};
int TestCacheFragment::arities[2] {3, 3};
int TestCacheFragment::totalRows[2] {8, 7};
const char* TestCacheFragment::kbName = "CacheFragmentTestKb";
SimpleKb* TestCacheFragment::kb = nullptr;
int TestCacheFragment::NumP = 0;
int TestCacheFragment::NumQ = 0;

TEST_F(TestCacheFragment, TestCase1aTest1) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(?, ?, ?) */
    CacheFragment fragment(kb->getRelation(NumP), NumP);

    /* p(X, ?, ?) */
    fragment.updateCase1a(0, 0, 0);
    EXPECT_STREQ("p(X0,?,?)", rule2String(fragment.getPartAssignedRule()).c_str());
    CacheFragment::entriesType expected_entries({
        new CacheFragment::entryType({
            CompliedBlock::create(rel_p->getAllRows(), rel_p->getTotalRows(), rel_p->getTotalCols(), false)
        })
    });
    checkEntries(expected_entries, fragment);
    std::vector<VarInfo> exp_var_info;
    exp_var_info.emplace_back(0, 0, true);
    EXPECT_EQ(exp_var_info, fragment.getVarInfoList());
    clearEntries(expected_entries);

    /* p(X, X, ?) */
    fragment.buildIndices();
    fragment.updateCase1a(0, 1, 0);
    EXPECT_STREQ("p(X0,X0,?)", rule2String(fragment.getPartAssignedRule()).c_str());
    int** cs1 = new int*[2] {
        new int[3]{1, 1, 1},
        new int[3]{1, 1, 2}
    };
    int** cs2 = new int*[1] {
        new int[3]{4, 4, 6}
    };
    int** cs3 = new int*[1] {
        new int[3]{5, 5, 1}
    };
    expected_entries.push_back(new CacheFragment::entryType({CompliedBlock::create(cs1, 2, 3, false)}));
    expected_entries.push_back(new CacheFragment::entryType({CompliedBlock::create(cs2, 1, 3, false)}));
    expected_entries.push_back(new CacheFragment::entryType({CompliedBlock::create(cs3, 1, 3, false)}));
    checkEntries(expected_entries, fragment);
    exp_var_info.clear();
    exp_var_info.emplace_back(0, 0, false);
    EXPECT_EQ(exp_var_info, fragment.getVarInfoList());
    clearEntries(expected_entries);

    CompliedBlock::clearPool();
    delete[] cs1[0];
    delete[] cs1[1];
    delete[] cs1;
    delete[] cs2[0];
    delete[] cs2;
    delete[] cs3[0];
    delete[] cs3;
}

TEST_F(TestCacheFragment, case1aTest2) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(?, ?, ?) */
    CacheFragment fragment(kb->getRelation(NumP), NumP);

    /* p(X, ?, ?) */
    fragment.updateCase1a(0, 0, 0);

    /* p(X, ?, Z) */
    fragment.buildIndices();
    fragment.updateCase1a(0, 2, 2);
    EXPECT_STREQ("p(X0,?,X2)", rule2String(fragment.getPartAssignedRule()).c_str());
    CacheFragment::entriesType expected_entries({
        new CacheFragment::entryType({
            CompliedBlock::create(rel_p->getAllRows(), rel_p->getTotalRows(), rel_p->getTotalCols(), false)
        })
    });
    checkEntries(expected_entries, fragment);
    clearEntries(expected_entries);

    /* p(X, X, Z) */
    fragment.buildIndices();
    fragment.updateCase1a(0, 1, 0);
    EXPECT_STREQ("p(X0,X0,X2)", rule2String(fragment.getPartAssignedRule()).c_str());
    int** cs1 = new int*[2] {
        new int[3]{1, 1, 1},
        new int[3]{1, 1, 2}
    };
    int** cs2 = new int*[1] {
        new int[3]{4, 4, 6}
    };
    int** cs3 = new int*[1] {
        new int[3]{5, 5, 1}
    };
    expected_entries.push_back(new CacheFragment::entryType({CompliedBlock::create(cs1, 2, 3, false)}));
    expected_entries.push_back(new CacheFragment::entryType({CompliedBlock::create(cs2, 1, 3, false)}));
    expected_entries.push_back(new CacheFragment::entryType({CompliedBlock::create(cs3, 1, 3, false)}));
    checkEntries(expected_entries, fragment);
    std::vector<VarInfo> exp_var_info;
    exp_var_info.emplace_back(0, 0, false);
    exp_var_info.emplace_back(-1, -1, false);
    exp_var_info.emplace_back(0, 2, true);
    EXPECT_EQ(exp_var_info, fragment.getVarInfoList());
    clearEntries(expected_entries);

    CompliedBlock::clearPool();
    delete[] cs1[0];
    delete[] cs1[1];
    delete[] cs1;
    delete[] cs2[0];
    delete[] cs2;
    delete[] cs3[0];
    delete[] cs3;
}

TEST_F(TestCacheFragment, case1bTest1) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(?, ?, ?) */
    CacheFragment fragment(rel_p, NumP);

    /* p(X, ?, ?) */
    fragment.updateCase1a(0, 0, 0);

    /* Copy and: p(X, ?, ?), q(?, X, ?) */
    fragment.buildIndices();
    CacheFragment fragment2(fragment);
    fragment2.updateCase1b(rel_q, NumQ, 1, 0);
    EXPECT_STREQ("p(X0,?,?),q(?,X0,?)", rule2String(fragment2.getPartAssignedRule()).c_str());
    int** cs11 = new int*[4] {
        new int[3]{1, 1, 1},
        new int[3]{1, 1, 2},
        new int[3]{1, 2, 3},
        new int[3]{1, 3, 2}
    };
    int** cs12 = new int*[3] {
        new int[3]{1, 1, 2},
        new int[3]{3, 1, 4},
        new int[3]{5, 1, 4}
    };
    int** cs21 = new int*[1] {
        new int[3]{4, 4, 6}
    };
    int** cs22 = new int*[1] {
        new int[3]{2, 4, 1}
    };
    CacheFragment::entriesType expected_entries({
        new CacheFragment::entryType({
            CompliedBlock::create(cs11, 4, 3, false),
            CompliedBlock::create(cs12, 3, 3, false)
        }),
        new CacheFragment::entryType({
            CompliedBlock::create(cs21, 1, 3, false),
            CompliedBlock::create(cs22, 1, 3, false)
        }),
    });
    checkEntries(expected_entries, fragment2);
    clearEntries(expected_entries);

    /* p(X, X, ?), q(?, X, ?) */
    fragment2.buildIndices();
    fragment2.updateCase1a(0, 1, 0);
    EXPECT_STREQ("p(X0,X0,?),q(?,X0,?)", rule2String(fragment2.getPartAssignedRule()).c_str());
    delete[] cs11[0];
    delete[] cs11[1];
    delete[] cs11[2];
    delete[] cs11[3];
    delete[] cs11;
    cs11 = new int*[2] {
        new int[3]{1, 1, 1},
        new int[3]{1, 1, 2},
    };
    delete[] cs21[0];
    delete[] cs22[0];
    cs21[0] = new int[3]{4, 4, 6};
    cs22[0] = new int[3]{2, 4, 1};
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs11, 2, 3, false),
        CompliedBlock::create(cs12, 3, 3, false)}
    ));
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs21, 1, 3, false),
        CompliedBlock::create(cs22, 1, 3, false)}
    ));
    checkEntries(expected_entries, fragment2);
    std::vector<VarInfo> exp_var_info;
    exp_var_info.emplace_back(0, 0, false);
    EXPECT_EQ(exp_var_info, fragment2.getVarInfoList());
    clearEntries(expected_entries);

    /* frag1 */
    EXPECT_STREQ("p(X0,?,?)", rule2String(fragment.getPartAssignedRule()).c_str());
    expected_entries.push_back(new CacheFragment::entryType({
            CompliedBlock::create(rel_p->getAllRows(), rel_p->getTotalRows(), rel_p->getTotalCols(), false)
    }));
    checkEntries(expected_entries, fragment);
    clearEntries(expected_entries);
    exp_var_info[0].isPlv = true;
    EXPECT_EQ(exp_var_info, fragment.getVarInfoList());
    clearEntries(expected_entries);

    CompliedBlock::clearPool();
    delete[] cs11[0];
    delete[] cs11[1];
    delete[] cs11;
    delete[] cs12[0];
    delete[] cs12[1];
    delete[] cs12[2];
    delete[] cs12;
    delete[] cs21[0];
    delete[] cs21;
    delete[] cs22[0];
    delete[] cs22;
}

TEST_F(TestCacheFragment, case1cTest1) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* F1: p(?, ?, ?) */
    /* F2: p(?, ?, ?) */
    CacheFragment fragment1(rel_p, NumP);
    CacheFragment* fragment2 = new CacheFragment(rel_p, NumP);

    /* F1: p(X, ?, ?) */
    fragment1.updateCase1a(0, 0, 0);

    /* F2: p(W, W, ?) */
    fragment2->updateCase1a(0, 0, 3);
    fragment2->buildIndices();
    fragment2->updateCase1a(0, 1, 3);

    /* F1 + F2: p(X, ?, ?), p(W, W, X) */
    fragment1.buildIndices();
    fragment2->buildIndices();
    fragment1.updateCase1c(*fragment2, 0, 2, 0);

    /* frag2 */
    EXPECT_STREQ("p(X3,X3,?)", rule2String(fragment2->getPartAssignedRule()).c_str());
    int** cs1 = new int*[2] {
        new int[3]{1, 1, 1},
        new int[3]{1, 1, 2}
    };
    int** cs2 = new int*[1] {
        new int[3]{4, 4, 6}
    };
    int** cs3 = new int*[1] {
        new int[3]{5, 5, 1}
    };
    CacheFragment::entriesType expected_entries({
        new CacheFragment::entryType({CompliedBlock::create(cs1, 2, 3, false)}),
        new CacheFragment::entryType({CompliedBlock::create(cs2, 1, 3, false)}),
        new CacheFragment::entryType({CompliedBlock::create(cs3, 1, 3, false)})
    });
    checkEntries(expected_entries, *fragment2);
    std::vector<VarInfo> exp_var_info;
    exp_var_info.emplace_back(-1, -1, false);
    exp_var_info.emplace_back(-1, -1, false);
    exp_var_info.emplace_back(-1, -1, false);
    exp_var_info.emplace_back(0, 0, false);
    EXPECT_EQ(exp_var_info, fragment2->getVarInfoList());
    clearEntries(expected_entries);
    exp_var_info.clear();
    delete fragment2;
    delete[] cs1[0];
    delete[] cs1[1];
    delete[] cs1;
    delete[] cs2[0];
    delete[] cs2;
    delete[] cs3[0];
    delete[] cs3;

    /* frag1 */
    int** cs11 = new int*[4] {
        new int[3]{1, 1, 1},
        new int[3]{1, 1, 2},
        new int[3]{1, 2, 3},
        new int[3]{1, 3, 2}
    };
    int** cs12 = new int*[1] {
        new int[3]{1, 1, 1}
    };
    int** cs21 = new int*[2] {
        new int[3]{2, 1, 3},
        new int[3]{2, 4, 4}
    };
    int** cs22 = new int*[1] {
        new int[3]{1, 1, 2}
    };
    int** cs31 = new int*[4] {
        new int[3]{1, 1, 1},
        new int[3]{1, 1, 2},
        new int[3]{1, 2, 3},
        new int[3]{1, 3, 2}
    };
    int** cs32 = new int*[1] {
        new int[3]{5, 5, 1}
    };
    EXPECT_STREQ("p(X0,?,?),p(X3,X3,X0)", rule2String(fragment1.getPartAssignedRule()).c_str());
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs11, 4, 3, false),
        CompliedBlock::create(cs12, 1, 3, false)
    }));
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs21, 2, 3, false),
        CompliedBlock::create(cs22, 1, 3, false)
    }));
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs31, 4, 3, false),
        CompliedBlock::create(cs32, 1, 3, false)
    }));
    checkEntries(expected_entries, fragment1);
    exp_var_info.emplace_back(0, 0, false);
    exp_var_info.emplace_back(-1, -1, false);
    exp_var_info.emplace_back(-1, -1, false);
    exp_var_info.emplace_back(1, 0, false);
    EXPECT_EQ(exp_var_info, fragment1.getVarInfoList());
    clearEntries(expected_entries);

    CompliedBlock::clearPool();
    delete[] cs11[0];
    delete[] cs11[1];
    delete[] cs11[2];
    delete[] cs11[3];
    delete[] cs11;
    delete[] cs12[0];
    delete[] cs12;
    delete[] cs21[0];
    delete[] cs21[1];
    delete[] cs21;
    delete[] cs22[0];
    delete[] cs22;
    delete[] cs31[0];
    delete[] cs31[1];
    delete[] cs31[2];
    delete[] cs31[3];
    delete[] cs31;
    delete[] cs32[0];
    delete[] cs32;
}

TEST_F(TestCacheFragment, case1cTest2) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* F1: p(?, ?, ?) */
    /* F2: p(?, ?, ?) */
    CacheFragment fragment1(rel_p, NumP);
    CacheFragment fragment2(rel_p, NumP);

    /* F1: p(X, X, ?) */
    fragment1.updateCase1a(0, 0, 0);
    fragment1.buildIndices();
    fragment1.updateCase1a(0, 1, 0);

    /* F2: p(Y, Y, ?) */
    fragment2.updateCase1a(0, 0, 1);
    fragment2.buildIndices();
    fragment2.updateCase1a(0, 1, 1);

    /* F1 + F2: p(X, X, ?), q(Y, Y, X) */
    fragment1.buildIndices();
    fragment2.buildIndices();
    fragment1.updateCase1c(fragment2, 0, 2, 0);
    int** cs11 = new int*[4] {
        new int[3]{1, 1, 1},
        new int[3]{1, 1, 2}
    };
    int** cs12 = new int*[1] {
        new int[3]{1, 1, 1}
    };
    int** cs21 = new int*[2] {
        new int[3]{1, 1, 1},
        new int[3]{1, 1, 2}
    };
    int** cs22 = new int*[1] {
        new int[3]{5, 5, 1}
    };
    CacheFragment::entriesType expected_entries({
        new CacheFragment::entryType({
            CompliedBlock::create(cs11, 2, 3, false),
            CompliedBlock::create(cs12, 1, 3, false)
        }),
        new CacheFragment::entryType({
            CompliedBlock::create(cs21, 2, 3, false),
            CompliedBlock::create(cs22, 1, 3, false)
        }),
    });
    EXPECT_STREQ("p(X0,X0,?),p(X1,X1,X0)", rule2String(fragment1.getPartAssignedRule()).c_str());
    checkEntries(expected_entries, fragment1);
    std::vector<VarInfo> exp_var_info;
    exp_var_info.emplace_back(0, 0, false);
    exp_var_info.emplace_back(1, 0, false);
    EXPECT_EQ(exp_var_info, fragment1.getVarInfoList());
    clearEntries(expected_entries);

    CompliedBlock::clearPool();
    delete[] cs11[0];
    delete[] cs11[1];
    delete[] cs11[2];
    delete[] cs11[3];
    delete[] cs11;
    delete[] cs12[0];
    delete[] cs12;
    delete[] cs21[0];
    delete[] cs21[1];
    delete[] cs21;
    delete[] cs22[0];
    delete[] cs22;
}

TEST_F(TestCacheFragment, case2aTest1) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(Y, Y, ?) */
    CacheFragment fragment(rel_p, NumP);
    fragment.updateCase2a(0, 0, 0, 1, 1);
    int** cs1 = new int*[2] {
        new int[3]{1, 1, 1},
        new int[3]{1, 1, 2}
    };
    int** cs2 = new int*[1] {
        new int[3]{4, 4, 6}
    };
    int** cs3 = new int*[1] {
        new int[3]{5, 5, 1}
    };
    EXPECT_STREQ("p(X1,X1,?)", rule2String(fragment.getPartAssignedRule()).c_str());
    CacheFragment::entriesType expected_entries;
    expected_entries.push_back(new CacheFragment::entryType({CompliedBlock::create(cs1, 2, 3, false)}));
    expected_entries.push_back(new CacheFragment::entryType({CompliedBlock::create(cs2, 1, 3, false)}));
    expected_entries.push_back(new CacheFragment::entryType({CompliedBlock::create(cs3, 1, 3, false)}));
    checkEntries(expected_entries, fragment);
    std::vector<VarInfo> exp_var_info;
    exp_var_info.emplace_back(-1, -1, false);
    exp_var_info.emplace_back(0, 0, false);
    EXPECT_EQ(exp_var_info, fragment.getVarInfoList());
    clearEntries(expected_entries);

    /* p(Y, Y, Y) */
    fragment.buildIndices();
    fragment.updateCase1a(0, 2, 1);
    delete[] cs1[0];
    delete[] cs1[1];
    delete[] cs1;
    cs1 = new int*[1] {
        new int[3]{1, 1, 1}
    };
    EXPECT_STREQ("p(X1,X1,X1)", rule2String(fragment.getPartAssignedRule()).c_str());
    expected_entries.push_back(new CacheFragment::entryType({CompliedBlock::create(cs1, 1, 3, false)}));
    checkEntries(expected_entries, fragment);
    EXPECT_EQ(exp_var_info, fragment.getVarInfoList());
    clearEntries(expected_entries);

    CompliedBlock::clearPool();
    delete[] cs1[0];
    delete[] cs1;
    delete[] cs2[0];
    delete[] cs2;
    delete[] cs3[0];
    delete[] cs3;
}

TEST_F(TestCacheFragment, case2aTest2) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(?, X, X) */
    CacheFragment fragment(rel_p, NumP);
    fragment.updateCase2a(0, 1, 0, 2, 0);
    int** cs1 = new int*[1] {
        new int[3]{1, 1, 1}
    };
    int** cs2 = new int*[1] {
        new int[3]{2, 4, 4}
    };
    EXPECT_STREQ("p(?,X0,X0)", rule2String(fragment.getPartAssignedRule()).c_str());
    CacheFragment::entriesType expected_entries;
    expected_entries.push_back(new CacheFragment::entryType({CompliedBlock::create(cs1, 1, 3, false)}));
    expected_entries.push_back(new CacheFragment::entryType({CompliedBlock::create(cs2, 1, 3, false)}));
    checkEntries(expected_entries, fragment);
    std::vector<VarInfo> exp_var_info;
    exp_var_info.emplace_back(0, 1, false);
    EXPECT_EQ(exp_var_info, fragment.getVarInfoList());
    clearEntries(expected_entries);

    /* p(X, X, X) */
    fragment.buildIndices();
    fragment.updateCase1a(0, 0, 0);
    EXPECT_STREQ("p(X0,X0,X0)", rule2String(fragment.getPartAssignedRule()).c_str());
    expected_entries.push_back(new CacheFragment::entryType({CompliedBlock::create(cs1, 1, 3, false)}));
    checkEntries(expected_entries, fragment);
    EXPECT_EQ(exp_var_info, fragment.getVarInfoList());
    clearEntries(expected_entries);

    CompliedBlock::clearPool();
    delete[] cs1[0];
    delete[] cs1;
    delete[] cs2[0];
    delete[] cs2;
}

TEST_F(TestCacheFragment, case2bTest1) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(?, ?, ?) */
    CacheFragment fragment(rel_p, NumP);

    /* p(X, ?, ?), q(X, ?, ?) */
    fragment.updateCase2b(rel_q, NumQ, 0, 0, 0, 0);
    int** cs11 = new int*[4] {
        new int[3]{1, 1, 1},
        new int[3]{1, 1, 2},
        new int[3]{1, 2, 3},
        new int[3]{1, 3, 2}
    };
    int** cs12 = new int*[1] {
        new int[3]{1, 1, 2}
    };
    int** cs21 = new int*[2] {
        new int[3]{2, 1, 3},
        new int[3]{2, 4, 4}
    };
    int** cs22 = new int*[2] {
        new int[3]{2, 4, 1},
        new int[3]{2, 7, 8}
    };
    int** cs31 = new int*[1] {
        new int[3]{5, 5, 1}
    };
    int** cs32 = new int*[1] {
        new int[3]{5, 1, 4}
    };
    EXPECT_STREQ("p(X0,?,?),q(X0,?,?)", rule2String(fragment.getPartAssignedRule()).c_str());
    CacheFragment::entriesType expected_entries;
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs11, 4, 3, false),
        CompliedBlock::create(cs12, 1, 3, false)
    }));
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs21, 2, 3, false),
        CompliedBlock::create(cs22, 2, 3, false)
    }));
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs31, 1, 3, false),
        CompliedBlock::create(cs32, 1, 3, false)
    }));
    checkEntries(expected_entries, fragment);
    std::vector<VarInfo> exp_var_info;
    exp_var_info.emplace_back(0, 0, false);
    EXPECT_EQ(exp_var_info, fragment.getVarInfoList());
    clearEntries(expected_entries);

    /* p(X, X, ?), q(X, ?, ?) */
    fragment.buildIndices();
    fragment.updateCase1a(0, 1, 0);
    delete[] cs11[0];
    delete[] cs11[1];
    delete[] cs11[2];
    delete[] cs11[3];
    delete[] cs11;
    cs11 = new int*[4] {
        new int[3]{1, 1, 1},
        new int[3]{1, 1, 2},
    };
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs11, 2, 3, false),
        CompliedBlock::create(cs12, 1, 3, false)
    }));
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs31, 1, 3, false),
        CompliedBlock::create(cs32, 1, 3, false)
    }));
    EXPECT_STREQ("p(X0,X0,?),q(X0,?,?)", rule2String(fragment.getPartAssignedRule()).c_str());
    checkEntries(expected_entries, fragment);
    EXPECT_EQ(exp_var_info, fragment.getVarInfoList());
    clearEntries(expected_entries);

    CompliedBlock::clearPool();
    delete[] cs11[0];
    delete[] cs11[1];
    delete[] cs11;
    delete[] cs12[0];
    delete[] cs12;
    delete[] cs21[0];
    delete[] cs21[1];
    delete[] cs21;
    delete[] cs22[0];
    delete[] cs22[1];
    delete[] cs22;
    delete[] cs31[0];
    delete[] cs31;
    delete[] cs32[0];
    delete[] cs32;
}

TEST_F(TestCacheFragment, case2bTest2) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(?, ?, ?) */
    CacheFragment fragment(rel_p, NumP);

    /* p(X, ?, ?) */
    fragment.updateCase1a(0, 0, 0);

    /* p(X, Y, ?), q(?, ?, Y) */
    fragment.buildIndices();
    fragment.updateCase2b(rel_q, NumQ, 2, 0, 1, 1);
    int** cs11 = new int*[3] {
        new int[3]{1, 1, 1},
        new int[3]{1, 1, 2},
        new int[3]{2, 1, 3},
    };
    int** cs12 = new int*[1] {
        new int[3]{2, 4, 1}
    };
    int** cs21 = new int*[1] {
        new int[3]{1, 2, 3},
    };
    int** cs22 = new int*[2] {
        new int[3]{1, 1, 2},
        new int[3]{6, 7, 2}
    };
    int** cs31 = new int*[2] {
        new int[3]{4, 4, 6},
        new int[3]{2, 4, 4}
    };
    int** cs32 = new int*[2] {
        new int[3]{5, 1, 4},
        new int[3]{3, 1, 4}
    };
    EXPECT_STREQ("p(X0,X1,?),q(?,?,X1)", rule2String(fragment.getPartAssignedRule()).c_str());
    CacheFragment::entriesType expected_entries;
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs11, 3, 3, false),
        CompliedBlock::create(cs12, 1, 3, false)
    }));
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs21, 1, 3, false),
        CompliedBlock::create(cs22, 2, 3, false)
    }));
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs31, 2, 3, false),
        CompliedBlock::create(cs32, 2, 3, false)
    }));
    checkEntries(expected_entries, fragment);
    std::vector<VarInfo> exp_var_info;
    exp_var_info.emplace_back(0, 0, true);
    exp_var_info.emplace_back(0, 1, false);
    EXPECT_EQ(exp_var_info, fragment.getVarInfoList());
    clearEntries(expected_entries);

    /* p(X, Y, ?), q(?, Z, Y), q(?, Z, ?) */
    fragment.buildIndices();
    fragment.updateCase2b(rel_q, NumQ, 1, 1, 1, 2);
    delete[] cs11[0];
    delete[] cs11[1];
    delete[] cs11[2];
    delete[] cs11;
    delete[] cs12[0];
    delete[] cs12;
    delete[] cs21[0];
    delete[] cs21;
    delete[] cs22[0];
    delete[] cs22[1];
    delete[] cs22;
    delete[] cs31[0];
    delete[] cs31[1];
    delete[] cs31;
    delete[] cs32[0];
    delete[] cs32[1];
    delete[] cs32;
    cs11 = new int*[3] {
        new int[3]{1, 1, 1},
        new int[3]{1, 1, 2},
        new int[3]{2, 1, 3},
    };
    cs12 = new int*[1] {
        new int[3]{2, 4, 1}
    };
    int** cs13 = new int*[1] {
        new int[3]{2, 4, 1}
    };
    cs21 = new int*[1] {
        new int[3]{1, 2, 3},
    };
    cs22 = new int*[1] {
        new int[3]{1, 1, 2},
    };
    int** cs23 = new int*[3] {
        new int[3]{1, 1, 2},
        new int[3]{3, 1, 4},
        new int[3]{5, 1, 4}
    };
    cs31 = new int*[1] {
        new int[3]{1, 2, 3},
    };
    cs32 = new int*[1] {
        new int[3]{6, 7, 2},
    };
    int** cs33 = new int*[2] {
        new int[3]{2, 7, 8},
        new int[3]{6, 7, 2},
    };
    int** cs41 = new int*[2] {
        new int[3]{4, 4, 6},
        new int[3]{2, 4, 4},
    };
    int** cs42 = new int*[2] {
        new int[3]{5, 1, 4},
        new int[3]{3, 1, 4},
    };
    int** cs43 = new int*[3] {
        new int[3]{1, 1, 2},
        new int[3]{3, 1, 4},
        new int[3]{5, 1, 4},
    };
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs11, 3, 3, false),
        CompliedBlock::create(cs12, 1, 3, false),
        CompliedBlock::create(cs13, 1, 3, false)
    }));
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs21, 1, 3, false),
        CompliedBlock::create(cs22, 1, 3, false),
        CompliedBlock::create(cs23, 3, 3, false)
    }));
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs31, 1, 3, false),
        CompliedBlock::create(cs32, 1, 3, false),
        CompliedBlock::create(cs33, 2, 3, false),
    }));
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs41, 2, 3, false),
        CompliedBlock::create(cs42, 2, 3, false),
        CompliedBlock::create(cs43, 3, 3, false)
    }));
    EXPECT_STREQ("p(X0,X1,?),q(?,X2,X1),q(?,X2,?)", rule2String(fragment.getPartAssignedRule()).c_str());
    checkEntries(expected_entries, fragment);
    exp_var_info.emplace_back(1, 1, false);
    EXPECT_EQ(exp_var_info, fragment.getVarInfoList());
    clearEntries(expected_entries);

    CompliedBlock::clearPool();
    delete[] cs11[0];
    delete[] cs11[1];
    delete[] cs11[2];
    delete[] cs11;
    delete[] cs12[0];
    delete[] cs12;
    delete[] cs13[0];
    delete[] cs13;
    delete[] cs21[0];
    delete[] cs21;
    delete[] cs22[0];
    delete[] cs22;
    delete[] cs23[0];
    delete[] cs23[1];
    delete[] cs23[2];
    delete[] cs23;
    delete[] cs31[0];
    delete[] cs31;
    delete[] cs32[0];
    delete[] cs32;
    delete[] cs33[0];
    delete[] cs33[1];
    delete[] cs33;
    delete[] cs41[0];
    delete[] cs41[1];
    delete[] cs41;
    delete[] cs42[0];
    delete[] cs42[1];
    delete[] cs42;
    delete[] cs43[0];
    delete[] cs43[1];
    delete[] cs43[2];
    delete[] cs43;
}

TEST_F(TestCacheFragment, case2cTest1) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* F1: p(?, ?, ?) */
    /* F2: q(?, ?, ?) */
    CacheFragment fragment1(rel_p, NumP);
    CacheFragment fragment2(rel_q, NumQ);

    /* F1 + F2: p(X, ?, ?), q(X, ?, ?) */
    fragment1.updateCase2c(0, 0, fragment2, 0, 0, 0);
    int** cs11 = new int*[4] {
        new int[3]{1, 1, 1},
        new int[3]{1, 1, 2},
        new int[3]{1, 2, 3},
        new int[3]{1, 3, 2},
    };
    int** cs12 = new int*[1] {
        new int[3]{1, 1, 2}
    };
    int** cs21 = new int*[2] {
        new int[3]{2, 1, 3},
        new int[3]{2, 4, 4},
    };
    int** cs22 = new int*[2] {
        new int[3]{2, 4, 1},
        new int[3]{2, 7, 8}
    };
    int** cs31 = new int*[1] {
        new int[3]{5, 5, 1},
    };
    int** cs32 = new int*[1] {
        new int[3]{5, 1, 4},
    };
    EXPECT_STREQ("p(X0,?,?),q(X0,?,?)", rule2String(fragment1.getPartAssignedRule()).c_str());
    CacheFragment::entriesType expected_entries;
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs11, 4, 3, false),
        CompliedBlock::create(cs12, 1, 3, false)
    }));
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs21, 2, 3, false),
        CompliedBlock::create(cs22, 2, 3, false)
    }));
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs31, 1, 3, false),
        CompliedBlock::create(cs32, 1, 3, false)
    }));
    checkEntries(expected_entries, fragment1);
    std::vector<VarInfo> exp_var_info;
    exp_var_info.emplace_back(0, 0, false);
    EXPECT_EQ(exp_var_info, fragment1.getVarInfoList());
    clearEntries(expected_entries);

    CompliedBlock::clearPool();
    delete[] cs11[0];
    delete[] cs11[1];
    delete[] cs11[2];
    delete[] cs11[3];
    delete[] cs11;
    delete[] cs12[0];
    delete[] cs12;
    delete[] cs21[0];
    delete[] cs21[1];
    delete[] cs21;
    delete[] cs22[0];
    delete[] cs22[1];
    delete[] cs22;
    delete[] cs31[0];
    delete[] cs31;
    delete[] cs32[0];
    delete[] cs32;
}

TEST_F(TestCacheFragment, case2cTest2) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* F1: p(?, ?, ?) */
    /* F2: q(?, ?, ?) */
    CacheFragment fragment1(rel_p, NumP);
    CacheFragment fragment2(rel_q, NumQ);

    /* F1: p(X, X, ?) */
    fragment1.updateCase2a(0, 0, 0, 1, 0);

    /* F2: q(?, Y, ?) */
    fragment2.updateCase1a(0, 1, 1);

    /* F1 + F2: p(X, X, Z), q(Z, Y, ?) */
    fragment1.buildIndices();
    fragment2.buildIndices();
    fragment1.updateCase2c(0, 2, fragment2, 0, 0, 2);
    int** cs11 = new int*[1] {
        new int[3]{1, 1, 1},
    };
    int** cs12 = new int*[1] {
        new int[3]{1, 1, 2}
    };
    int** cs21 = new int*[1] {
        new int[3]{1, 1, 2}
    };
    int** cs22 = new int*[2] {
        new int[3]{2, 4, 1},
        new int[3]{2, 7, 8}
    };
    int** cs31 = new int*[1] {
        new int[3]{4, 4, 6},
    };
    int** cs32 = new int*[1] {
        new int[3]{6, 7, 2},
    };
    int** cs41 = new int*[1] {
        new int[3]{5, 5, 1},
    };
    int** cs42 = new int*[1] {
        new int[3]{1, 1, 2},
    };
    EXPECT_STREQ("p(X0,X0,X2),q(X2,X1,?)", rule2String(fragment1.getPartAssignedRule()).c_str());
    CacheFragment::entriesType expected_entries;
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs11, 1, 3, false),
        CompliedBlock::create(cs12, 1, 3, false)
    }));
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs21, 1, 3, false),
        CompliedBlock::create(cs22, 2, 3, false)
    }));
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs31, 1, 3, false),
        CompliedBlock::create(cs32, 1, 3, false)
    }));
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs41, 1, 3, false),
        CompliedBlock::create(cs42, 1, 3, false)
    }));
    checkEntries(expected_entries, fragment1);
    std::vector<VarInfo> exp_var_info;
    exp_var_info.emplace_back(0, 0, false);
    exp_var_info.emplace_back(1, 1, true);
    exp_var_info.emplace_back(0, 2, false);
    EXPECT_EQ(exp_var_info, fragment1.getVarInfoList());
    clearEntries(expected_entries);

    CompliedBlock::clearPool();
    delete[] cs11[0];
    delete[] cs11;
    delete[] cs12[0];
    delete[] cs12;
    delete[] cs21[0];
    delete[] cs21;
    delete[] cs22[0];
    delete[] cs22[1];
    delete[] cs22;
    delete[] cs31[0];
    delete[] cs31;
    delete[] cs32[0];
    delete[] cs32;
    delete[] cs41[0];
    delete[] cs41;
    delete[] cs42[0];
    delete[] cs42;
}

TEST_F(TestCacheFragment, case3Test1) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(?, ?, ?) */
    CacheFragment fragment(rel_p, NumP);

    /* p(1, ?, ?) */
    fragment.updateCase3(0, 0, 1);
    int** cs11 = new int*[4] {
        new int[3]{1, 1, 1},
        new int[3]{1, 1, 2},
        new int[3]{1, 2, 3},
        new int[3]{1, 3, 2},
    };
    EXPECT_STREQ("p(1,?,?)", rule2String(fragment.getPartAssignedRule()).c_str());
    CacheFragment::entriesType expected_entries;
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs11, 4, 3, false),
    }));
    checkEntries(expected_entries, fragment);
    EXPECT_TRUE(fragment.getVarInfoList().empty());
    clearEntries(expected_entries);

    CompliedBlock::clearPool();
    delete[] cs11[0];
    delete[] cs11[1];
    delete[] cs11[2];
    delete[] cs11[3];
    delete[] cs11;
}

TEST_F(TestCacheFragment, case3Test2) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(?, ?, ?) */
    CacheFragment fragment(rel_p, NumP);

    /* p(X, ?, ?), q(X, ?, ?) */
    fragment.updateCase2b(rel_q, NumQ, 0, 0, 0, 0);

    /* p(X, ?, ?), q(X, ?, 8) */
    fragment.buildIndices();
    fragment.updateCase3(1, 2, 8);
    int** cs11 = new int*[2] {
        new int[3]{2, 1, 3},
        new int[3]{2, 4, 4},
    };
    int** cs12 = new int*[1] {
        new int[3]{2, 7, 8}
    };
    EXPECT_STREQ("p(X0,?,?),q(X0,?,8)", rule2String(fragment.getPartAssignedRule()).c_str());
    CacheFragment::entriesType expected_entries;
    expected_entries.push_back(new CacheFragment::entryType({
        CompliedBlock::create(cs11, 2, 3, false),
        CompliedBlock::create(cs12, 1, 3, false)
    }));
    checkEntries(expected_entries, fragment);
    std::vector<VarInfo> exp_var_info;
    exp_var_info.emplace_back(0, 0, false);
    EXPECT_EQ(exp_var_info, fragment.getVarInfoList());
    clearEntries(expected_entries);

    CompliedBlock::clearPool();
    delete[] cs11[0];
    delete[] cs11[1];
    delete[] cs11;
    delete[] cs12[0];
    delete[] cs12;
}

TEST_F(TestCacheFragment, TestCountTableSize1) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(?, ?, ?) */
    CacheFragment fragment(rel_p, NumP);
    EXPECT_EQ(8, fragment.countTableSize(0));

    /* p(1, ?, ?) */
    fragment.updateCase3(0, 0, 1);
    EXPECT_EQ(4, fragment.countTableSize(0));
    CompliedBlock::clearPool();
}

TEST_F(TestCacheFragment, TestCountTableSize2) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(?, ?, ?) */
    CacheFragment fragment(rel_p, NumP);

    /* p(X, ?, ?) */
    fragment.updateCase1a(0, 0, 0);

    /* p(X, Y, ?), q(?, ?, Y) */
    fragment.buildIndices();
    fragment.updateCase2b(rel_q, NumQ, 2, 0, 1, 1);

    /* p(X, Y, ?), q(?, Z, Y), q(?, Z, ?) */
    fragment.buildIndices();
    fragment.updateCase2b(rel_q, NumQ, 1, 1, 1, 2);
    EXPECT_EQ(6, fragment.countTableSize(2));
    CompliedBlock::clearPool();
//        expected_entries = List.of(
//                List.of(
//                        new CB(new int[][]{
//                                new int[]{1, 1, 1},
//                                new int[]{1, 1, 2},
//                                new int[]{2, 1, 3},
//                        }), new CB(new int[][]{
//                                new int[]{2, 4, 1}
//                        }), new CB(new int[][]{
//                                new int[]{2, 4, 1}
//                        })
//                ), List.of(
//                        new CB(new int[][]{
//                                new int[]{1, 2, 3}
//                        }), new CB(new int[][]{
//                                new int[]{1, 1, 2},
//                        }), new CB(new int[][]{
//                                new int[]{1, 1, 2},
//                                new int[]{3, 1, 4},
//                                new int[]{5, 1, 4}
//                        })
//                ),List.of(
//                        new CB(new int[][]{
//                                new int[]{1, 2, 3}
//                        }), new CB(new int[][]{
//                                new int[]{6, 7, 2}
//                        }), new CB(new int[][]{
//                                new int[]{2, 7, 8},
//                                new int[]{6, 7, 2}
//                        })
//                ), List.of(
//                        new CB(new int[][]{
//                                new int[]{4, 4, 6},
//                                new int[]{2, 4, 4}
//                        }), new CB(new int[][]{
//                                new int[]{5, 1, 4},
//                                new int[]{3, 1, 4}
//                        }), new CB(new int[][]{
//                                new int[]{1, 1, 2},
//                                new int[]{3, 1, 4},
//                                new int[]{5, 1, 4}
//                        })
//                )
//        );
}

TEST_F(TestCacheFragment, TestCountCombinations1) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(X, ?, ?) [X] */
    CacheFragment fragment(rel_p, NumP);
    fragment.updateCase1a(0, 0, 0);
    EXPECT_STREQ("p(X0,?,?)", rule2String(fragment.getPartAssignedRule()).c_str());
    std::vector<int> vids({0});
    EXPECT_EQ(4, fragment.countCombinations(vids));
    CompliedBlock::clearPool();
}

TEST_F(TestCacheFragment, TestCountCombinations2) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(X, Y, ?) [X,Y] */
    CacheFragment fragment(rel_p, NumP);
    fragment.updateCase1a(0, 0, 0);
    fragment.buildIndices();
    fragment.updateCase1a(0, 1, 1);
    EXPECT_STREQ("p(X0,X1,?)", rule2String(fragment.getPartAssignedRule()).c_str());
    std::vector<int> vids({0, 1});
    EXPECT_EQ(7, fragment.countCombinations(vids));
    CompliedBlock::clearPool();
}

TEST_F(TestCacheFragment, TestCountCombinations3) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(Y, Y, ?) [Y] */
    CacheFragment fragment(rel_p, NumP);
    fragment.updateCase2a(0, 0, 0, 1, 1);
    EXPECT_STREQ("p(X1,X1,?)", rule2String(fragment.getPartAssignedRule()).c_str());
    std::vector<int> vids({1});
    EXPECT_EQ(3, fragment.countCombinations(vids));
    CompliedBlock::clearPool();
}

TEST_F(TestCacheFragment, TestCountCombinations4) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(X, Y, ?), q(?, Z, Y), q(?, Z, ?) [Y, Z] */
    CacheFragment fragment(rel_p, NumP);
    fragment.updateCase1a(0, 0, 0);
    fragment.buildIndices();
    fragment.updateCase2b(rel_q, NumQ, 2, 0, 1, 1);
    fragment.buildIndices();
    fragment.updateCase2b(rel_q, NumQ, 1, 1, 1, 2);
    EXPECT_STREQ("p(X0,X1,?),q(?,X2,X1),q(?,X2,?)", rule2String(fragment.getPartAssignedRule()).c_str());
    std::vector<int> vids({1, 2});
    EXPECT_EQ(4, fragment.countCombinations(vids));
    CompliedBlock::clearPool();
}

TEST_F(TestCacheFragment, TestCountCombinations5) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(X, Y, ?), q(?, Z, Y), q(?, Z, ?) [X, Z] */
    CacheFragment fragment(rel_p, NumP);
    fragment.updateCase1a(0, 0, 0);
    fragment.buildIndices();
    fragment.updateCase2b(rel_q, NumQ, 2, 0, 1, 1);
    fragment.buildIndices();
    fragment.updateCase2b(rel_q, NumQ, 1, 1, 1, 2);
    EXPECT_STREQ("p(X0,X1,?),q(?,X2,X1),q(?,X2,?)", rule2String(fragment.getPartAssignedRule()).c_str());
    std::vector<int> vids({0, 2});
    EXPECT_EQ(6, fragment.countCombinations(vids));
    CompliedBlock::clearPool();
}

TEST_F(TestCacheFragment, TestEnumerateCombinations1) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(X, ?, ?) [X] */
    CacheFragment fragment(rel_p, NumP);
    fragment.updateCase1a(0, 0, 0);
    EXPECT_STREQ("p(X0,?,?)", rule2String(fragment.getPartAssignedRule()).c_str());
    int** exp_comb = new int*[4] {
        new int[1]{1},
        new int[1]{2},
        new int[1]{4},
        new int[1]{5},
    };
    std::vector<int> vids({0});
    std::unordered_set<Record>* actual_set = fragment.enumerateCombinations(vids);
    checkCombinations(exp_comb, 1, actual_set, 4);
    delete[] exp_comb[0];
    delete[] exp_comb[1];
    delete[] exp_comb[2];
    delete[] exp_comb[3];
    delete[] exp_comb;
    releaseCombinationSet(actual_set);
    CompliedBlock::clearPool();
}

TEST_F(TestCacheFragment, TestEnumerateCombinations2) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(Y, Y, ?) [Y] */
    CacheFragment fragment(rel_p, NumP);
    fragment.updateCase2a(0, 0, 0, 1, 1);
    EXPECT_STREQ("p(X1,X1,?)", rule2String(fragment.getPartAssignedRule()).c_str());
    int** exp_comb = new int*[3] {
        new int[1]{1},
        new int[1]{4},
        new int[1]{5},
    };
    std::vector<int> vids({1});
    std::unordered_set<Record>* actual_set = fragment.enumerateCombinations(vids);
    checkCombinations(exp_comb, 1, actual_set, 3);
    delete[] exp_comb[0];
    delete[] exp_comb[1];
    delete[] exp_comb[2];
    delete[] exp_comb;
    releaseCombinationSet(actual_set);
    CompliedBlock::clearPool();
}

TEST_F(TestCacheFragment, TestEnumerateCombinations3) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(Y, X, ?) [X, Y] */
    CacheFragment fragment(rel_p, NumP);
    fragment.updateCase1a(0, 0, 1);
    fragment.buildIndices();
    fragment.updateCase1a(0, 1, 0);
    EXPECT_STREQ("p(X1,X0,?)", rule2String(fragment.getPartAssignedRule()).c_str());
    int** exp_comb = new int*[7] {
        new int[2]{1, 1},
        new int[2]{2, 1},
        new int[2]{1, 2},
        new int[2]{4, 4},
        new int[2]{5, 5},
        new int[2]{3, 1},
        new int[2]{4, 2}
    };
    std::vector<int> vids({0, 1});
    std::unordered_set<Record>* actual_set = fragment.enumerateCombinations(vids);
    checkCombinations(exp_comb, 2, actual_set, 7);
    delete[] exp_comb[0];
    delete[] exp_comb[1];
    delete[] exp_comb[2];
    delete[] exp_comb[3];
    delete[] exp_comb[4];
    delete[] exp_comb[5];
    delete[] exp_comb[6];
    delete[] exp_comb;
    releaseCombinationSet(actual_set);
    CompliedBlock::clearPool();
}

TEST_F(TestCacheFragment, TestEnumerateCombinations4) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(Y, Y, W) [Y, W] */
    CacheFragment fragment(rel_p, NumP);
    fragment.updateCase2a(0, 0, 0, 1, 1);
    fragment.buildIndices();
    fragment.updateCase1a(0, 2, 3);
    EXPECT_STREQ("p(X1,X1,X3)", rule2String(fragment.getPartAssignedRule()).c_str());
    int** exp_comb = new int*[4] {
        new int[2]{1, 1},
        new int[2]{1, 2},
        new int[2]{4, 6},
        new int[2]{5, 1},
    };
    std::vector<int> vids({1, 3});
    std::unordered_set<Record>* actual_set = fragment.enumerateCombinations(vids);
    checkCombinations(exp_comb, 2, actual_set, 4);
    delete[] exp_comb[0];
    delete[] exp_comb[1];
    delete[] exp_comb[2];
    delete[] exp_comb[3];
    delete[] exp_comb;
    releaseCombinationSet(actual_set);
    CompliedBlock::clearPool();
}

TEST_F(TestCacheFragment, TestEnumerateCombinations5) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(X, Y, ?), q(?, Z, Y), q(?, Z, ?) [X, Z] */
    CacheFragment fragment(rel_p, NumP);
    fragment.updateCase1a(0, 0, 0);
    fragment.buildIndices();
    fragment.updateCase2b(rel_q, NumQ, 2, 0, 1, 1);
    fragment.buildIndices();
    fragment.updateCase2b(rel_q, NumQ, 1, 1, 1, 2);
    EXPECT_STREQ("p(X0,X1,?),q(?,X2,X1),q(?,X2,?)", rule2String(fragment.getPartAssignedRule()).c_str());
    int** exp_comb = new int*[6] {
        new int[2]{1, 4},
        new int[2]{2, 4},
        new int[2]{1, 1},
        new int[2]{1, 7},
        new int[2]{4, 1},
        new int[2]{2, 1},
    };
    std::vector<int> vids({0, 2});
    std::unordered_set<Record>* actual_set = fragment.enumerateCombinations(vids);
    checkCombinations(exp_comb, 2, actual_set, 6);
    delete[] exp_comb[0];
    delete[] exp_comb[1];
    delete[] exp_comb[2];
    delete[] exp_comb[3];
    delete[] exp_comb[4];
    delete[] exp_comb[5];
    delete[] exp_comb;
    releaseCombinationSet(actual_set);
    CompliedBlock::clearPool();
}

TEST_F(TestCacheFragment, TestEnumerateCombinations6) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(X, Y, ?), q(?, Z, Y), q(?, Z, ?) [Z, X] */
    CacheFragment fragment(rel_p, NumP);
    fragment.updateCase1a(0, 0, 0);
    fragment.buildIndices();
    fragment.updateCase2b(rel_q, NumQ, 2, 0, 1, 1);
    fragment.buildIndices();
    fragment.updateCase2b(rel_q, NumQ, 1, 1, 1, 2);
    EXPECT_STREQ("p(X0,X1,?),q(?,X2,X1),q(?,X2,?)", rule2String(fragment.getPartAssignedRule()).c_str());
    int** exp_comb = new int*[6] {
        new int[2]{4, 1},
        new int[2]{4, 2},
        new int[2]{1, 1},
        new int[2]{7, 1},
        new int[2]{1, 4},
        new int[2]{1, 2},
    };
    std::vector<int> vids({2, 0});
    std::unordered_set<Record>* actual_set = fragment.enumerateCombinations(vids);
    checkCombinations(exp_comb, 2, actual_set, 6);
    delete[] exp_comb[0];
    delete[] exp_comb[1];
    delete[] exp_comb[2];
    delete[] exp_comb[3];
    delete[] exp_comb[4];
    delete[] exp_comb[5];
    delete[] exp_comb;
    releaseCombinationSet(actual_set);
    CompliedBlock::clearPool();
}

TEST_F(TestCacheFragment, TestEnumerateCombinations7) {
    SimpleRelation* rel_p = kb->getRelation(NumP);
    SimpleRelation* rel_q = kb->getRelation(NumQ);

    /* p(X, Y, ?), q(?, Z, Y), q(?, Z, ?) [Z, X] */
    CacheFragment fragment(rel_p, NumP);
    fragment.updateCase1a(0, 0, 0);
    fragment.buildIndices();
    fragment.updateCase2b(rel_q, NumQ, 2, 0, 1, 1);
    fragment.buildIndices();
    fragment.updateCase2b(rel_q, NumQ, 1, 1, 1, 2);
    fragment.buildIndices();
    fragment.updateCase1a(2, 2, 3);
    EXPECT_STREQ("p(X0,X1,?),q(?,X2,X1),q(?,X2,X3)", rule2String(fragment.getPartAssignedRule()).c_str());
    int** exp_comb = new int*[9] {
        new int[2]{1, 1},
        new int[2]{2, 1},
        new int[2]{1, 2},
        new int[2]{1, 4},
        new int[2]{1, 8},
        new int[2]{4, 2},
        new int[2]{4, 4},
        new int[2]{2, 2},
        new int[2]{2, 4},
    };
    std::vector<int> vids({0, 3});
    std::unordered_set<Record>* actual_set = fragment.enumerateCombinations(vids);
    checkCombinations(exp_comb, 2, actual_set, 9);
    delete[] exp_comb[0];
    delete[] exp_comb[1];
    delete[] exp_comb[2];
    delete[] exp_comb[3];
    delete[] exp_comb[4];
    delete[] exp_comb[5];
    delete[] exp_comb[6];
    delete[] exp_comb[7];
    delete[] exp_comb[8];
    delete[] exp_comb;
    releaseCombinationSet(actual_set);
    CompliedBlock::clearPool();
}

class TestCachedRule : public testing::Test {
protected:
    static const std::string KB_NAME;
    static const int NUM_FATHER;
    static const int NUM_PARENT;
    static const int NUM_GRANDPARENT;
    static const int NUM_G1;
    static const int NUM_G2;
    static const int NUM_G3;
    static const int NUM_G4;
    static const int NUM_F1;
    static const int NUM_F2;
    static const int NUM_F3;
    static const int NUM_F4;
    static const int NUM_M2;
    static const int NUM_S1;
    static const int NUM_S2;
    static const int NUM_S3;
    static const int NUM_S4;
    static const int NUM_D1;
    static const int NUM_D2;
    static const int NUM_D4;
    static const Record FATHER1;
    static const Record FATHER2;
    static const Record FATHER3;
    static const Record FATHER4;
    static const Record FATHER5;
    static const Record PARENT1;
    static const Record PARENT2;
    static const Record PARENT3;
    static const Record PARENT4;
    static const Record PARENT5;
    static const Record PARENT6;
    static const Record PARENT7;
    static const Record PARENT8;
    static const Record PARENT9;
    static const Record GRAND1;
    static const Record GRAND2;
    static const Record GRAND3;

    static int*** relations;
    static std::string relNames[3];
    static int arities[3];
    static int totalRows[3];
    static Rule::fingerprintCacheType cache;
    static Rule::tabuMapType tabuMap;

    typedef std::unordered_set<std::vector<Record>> groundingSetType;

    static void SetUpTestSuite() {
        std::cout << "Set up KB resources" << std::endl;
        relations = new int**[3] {
            new int*[5] {   // father
                /* father(X, Y):
                *   f1, s1
                *   f2, s2
                *   f2, d2
                *   f3, s3
                *   f4, d4
                */
                FATHER1.getArgs(),
                FATHER2.getArgs(),
                FATHER3.getArgs(),
                FATHER4.getArgs(),
                FATHER5.getArgs(),
            },
            new int*[9] {   // parent
                /* parent(X, Y):
                *   f1, s1
                *   f1, d1
                *   f2, s2
                *   f2, d2
                *   m2, d2
                *   g1, f1
                *   g2, f2
                *   g2, m2
                *   g3, f3
                */
                PARENT1.getArgs(),
                PARENT2.getArgs(),
                PARENT3.getArgs(),
                PARENT4.getArgs(),
                PARENT5.getArgs(),
                PARENT6.getArgs(),
                PARENT7.getArgs(),
                PARENT8.getArgs(),
                PARENT9.getArgs(),
            },
            new int*[3] {   // grandParent
                /* grandParent(X, Y):
                *   g1, s1
                *   g2, d2
                *   g4, s4
                */
                GRAND1.getArgs(),
                GRAND2.getArgs(),
                GRAND3.getArgs(),
            }
        };
        /* Constants(16):
         *   g1, g2, g3, g4
         *   f1, f2, f3, f4
         *   m2
         *   s1, s2, s3, s4
         *   d1, d2, d4
         */
    }

    static void TearDownTestSuite() {
        std::cout << "Destroy KB resources" << std::endl;
        for (int i = 0; i < 5; i++) {
            delete[] relations[0][i];
        }
        for (int i = 0; i < 9; i++) {
            delete[] relations[1][i];
        }
        for (int i = 0; i < 3; i++) {
            delete[] relations[2][i];
        }
        delete[] relations[0];
        delete[] relations[1];
        delete[] relations[2];
        delete[] relations;
    }

    void SetUp() {
        Rule::MinFactCoverage = -1.0;
    }

    void TearDown() {
        CompliedBlock::clearPool();
    }

    SimpleKb* kbFamily() const {
        return new SimpleKb(KB_NAME, relations, relNames, arities, totalRows, 3);
    }

    void releaseCacheAndTabuMap() {
        for (const Fingerprint* const& fp: cache) {
            delete fp;
        }
        cache.clear();
        for (std::pair<MultiSet<int> const*, Rule::fingerprintCacheType*> const& kv: tabuMap) {
            delete kv.first;
            delete kv.second;
        }
        tabuMap.clear();
    }

    void releaseGroundingSets(groundingSetType** const sets, int const numSets) {
        for (int i = 0; i < numSets; i++) {
            sets[i]->clear();
        }
    }

    void releaseCounterexampleSet(std::unordered_set<Record>& set) {
        for (Record const& r: set) {
            delete[] r.getArgs();
        }
        set.clear();
    }

    void checkEvidence(
        EvidenceBatch const& actualEvidence, int* const expectedRelationsInRule, int* const expectedAritiesInRule,
        int const numRelations, groundingSetType** const expectedGroundingSets, int const numSets
    ) {
        ASSERT_EQ(numRelations, actualEvidence.numPredicates);
        for (int i = 0; i < numRelations; i++) {
            EXPECT_EQ(expectedRelationsInRule[i], actualEvidence.predicateSymbolsInRule[i]) << "predSymbol@" << i;
            EXPECT_EQ(expectedAritiesInRule[i], actualEvidence.aritiesInRule[i]) << "arity@" << i;
        }
        groundingSetType actual_grounding_set;
        actual_grounding_set.reserve(actualEvidence.evidenceList.size());
        for (int i = 0; i < actualEvidence.evidenceList.size(); i++) {
            int** const grounding = actualEvidence.evidenceList[i];
            std::vector<Record> grounding_vector;
            grounding_vector.reserve(numRelations);
            for (int j = 0; j < numRelations; j++) {
                grounding_vector.emplace_back(grounding[j], expectedAritiesInRule[j]);
            }
            actual_grounding_set.insert(grounding_vector);
        }
        bool match_found = false;
        for (int i = 0; i < numSets; i++) {
            if (actual_grounding_set == *(expectedGroundingSets[i])) {
                match_found = true;
                break;
            }
        }
        EXPECT_TRUE(match_found);
    }
};

const std::string TestCachedRule::KB_NAME = "TestCachedRuleKB";
const int TestCachedRule::NUM_FATHER = 0;
const int TestCachedRule::NUM_PARENT = 1;
const int TestCachedRule::NUM_GRANDPARENT = 2;
const int TestCachedRule::NUM_G1 = 1;
const int TestCachedRule::NUM_G2 = 2;
const int TestCachedRule::NUM_G3 = 3;
const int TestCachedRule::NUM_G4 = 4;
const int TestCachedRule::NUM_F1 = 5;
const int TestCachedRule::NUM_F2 = 6;
const int TestCachedRule::NUM_F3 = 7;
const int TestCachedRule::NUM_F4 = 8;
const int TestCachedRule::NUM_M2 = 9;
const int TestCachedRule::NUM_S1 = 10;
const int TestCachedRule::NUM_S2 = 11;
const int TestCachedRule::NUM_S3 = 12;
const int TestCachedRule::NUM_S4 = 13;
const int TestCachedRule::NUM_D1 = 14;
const int TestCachedRule::NUM_D2 = 15;
const int TestCachedRule::NUM_D4 = 16;
const Record TestCachedRule::FATHER1 = Record(new int[2]{NUM_F1, NUM_S1}, 2);
const Record TestCachedRule::FATHER2 = Record(new int[2]{NUM_F2, NUM_S2}, 2);
const Record TestCachedRule::FATHER3 = Record(new int[2]{NUM_F2, NUM_D2}, 2);
const Record TestCachedRule::FATHER4 = Record(new int[2]{NUM_F3, NUM_S3}, 2);
const Record TestCachedRule::FATHER5 = Record(new int[2]{NUM_F4, NUM_D4}, 2);
const Record TestCachedRule::PARENT1 = Record(new int[2]{NUM_F1, NUM_S1}, 2);
const Record TestCachedRule::PARENT2 = Record(new int[2]{NUM_F1, NUM_D1}, 2);
const Record TestCachedRule::PARENT3 = Record(new int[2]{NUM_F2, NUM_S2}, 2);
const Record TestCachedRule::PARENT4 = Record(new int[2]{NUM_F2, NUM_D2}, 2);
const Record TestCachedRule::PARENT5 = Record(new int[2]{NUM_M2, NUM_D2}, 2);
const Record TestCachedRule::PARENT6 = Record(new int[2]{NUM_G1, NUM_F1}, 2);
const Record TestCachedRule::PARENT7 = Record(new int[2]{NUM_G2, NUM_F2}, 2);
const Record TestCachedRule::PARENT8 = Record(new int[2]{NUM_G2, NUM_M2}, 2);
const Record TestCachedRule::PARENT9 = Record(new int[2]{NUM_G3, NUM_F3}, 2);
const Record TestCachedRule::GRAND1 = Record(new int[2]{NUM_G1, NUM_S1}, 2);
const Record TestCachedRule::GRAND2 = Record(new int[2]{NUM_G2, NUM_D2}, 2);
const Record TestCachedRule::GRAND3 = Record(new int[2]{NUM_G4, NUM_S4}, 2);
int*** TestCachedRule::relations;
std::string TestCachedRule::relNames[3] {"father", "parent", "grandParent"};
int TestCachedRule::arities[3] {2, 2, 2};
int TestCachedRule::totalRows[3] {5, 9, 3};
Rule::fingerprintCacheType TestCachedRule::cache;
Rule::tabuMapType TestCachedRule::tabuMap;

TEST_F(TestCachedRule, TestFamilyRule1) {
    SimpleKb* kb = kbFamily();
    Eval eval(0, 0, 0);

    /* parent(?, ?) :- */
    CachedRule rule(NUM_PARENT, 2, cache, tabuMap, *kb);
    EXPECT_STREQ("parent(?,?):-", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(9, 16 * 16, 0);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(0, rule.usedLimitedVars());
    EXPECT_EQ(0, rule.getLength());
    EXPECT_EQ(1, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* parent(X, ?) :- father(X, ?) */
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase4(NUM_FATHER, 2, 0, 0, 0));
    EXPECT_STREQ("parent(X0,?):-father(X0,?)", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(4, 4 * 16, 1);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(1, rule.usedLimitedVars());
    EXPECT_EQ(1, rule.getLength());
    EXPECT_EQ(2, cache.size());
    EXPECT_EQ(0, tabuMap.size());
    groundingSetType expected_grounding_set1;
    expected_grounding_set1.emplace(std::vector<Record>({PARENT1, FATHER1}));
    expected_grounding_set1.emplace(std::vector<Record>({PARENT2, FATHER1}));
    expected_grounding_set1.emplace(std::vector<Record>({PARENT3, FATHER2}));
    expected_grounding_set1.emplace(std::vector<Record>({PARENT4, FATHER2}));
    groundingSetType expected_grounding_set2;
    expected_grounding_set2.emplace(std::vector<Record>({PARENT1, FATHER1}));
    expected_grounding_set2.emplace(std::vector<Record>({PARENT2, FATHER1}));
    expected_grounding_set2.emplace(std::vector<Record>({PARENT3, FATHER2}));
    expected_grounding_set2.emplace(std::vector<Record>({PARENT4, FATHER3}));
    groundingSetType expected_grounding_set3;
    expected_grounding_set3.emplace(std::vector<Record>({PARENT1, FATHER1}));
    expected_grounding_set3.emplace(std::vector<Record>({PARENT2, FATHER1}));
    expected_grounding_set3.emplace(std::vector<Record>({PARENT3, FATHER3}));
    expected_grounding_set3.emplace(std::vector<Record>({PARENT4, FATHER2}));
    groundingSetType expected_grounding_set4;
    expected_grounding_set4.emplace(std::vector<Record>({PARENT1, FATHER1}));
    expected_grounding_set4.emplace(std::vector<Record>({PARENT2, FATHER1}));
    expected_grounding_set4.emplace(std::vector<Record>({PARENT3, FATHER3}));
    expected_grounding_set4.emplace(std::vector<Record>({PARENT4, FATHER3}));
    std::unordered_set<Record> expected_counter_examples;
    for (int arg1: {NUM_F1, NUM_F2, NUM_F3, NUM_F4}) {
        for (int arg2 = 1; arg2 <= kb->totalConstants(); arg2++) {
            expected_counter_examples.emplace(new int[2]{arg1, arg2}, 2);
        }
    }
    for (Record const& r: {PARENT1, PARENT2, PARENT3, PARENT4}) {
        std::unordered_set<Record>::iterator itr = expected_counter_examples.find(r);
        if (expected_counter_examples.end() != itr) {
            delete[] itr->getArgs();
            expected_counter_examples.erase(itr);
        }
    }
    EvidenceBatch* actual_evidence = rule.getEvidenceAndMarkEntailment();
    int exp_relation_nums[2] {NUM_PARENT, NUM_FATHER};
    int exp_arities[2] {2, 2};
    groundingSetType* exp_groundings[4] {
        &expected_grounding_set1, &expected_grounding_set2, &expected_grounding_set3, &expected_grounding_set4
    };
    checkEvidence(*actual_evidence, exp_relation_nums, exp_arities, 2 , exp_groundings, 4);
    std::unordered_set<Record>* actual_counterexamples = rule.getCounterexamples();
    EXPECT_EQ(expected_counter_examples, *actual_counterexamples);
    releaseGroundingSets(exp_groundings, 4);
    releaseCounterexampleSet(expected_counter_examples);
    delete actual_evidence;
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_counterexamples;

    /* parent(X, Y) :- father(X, Y) */
    releaseCacheAndTabuMap();
    CachedRule rule2(NUM_PARENT, 2, cache, tabuMap, *kb);
    rule2.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule2.specializeCase4(NUM_FATHER, 2, 0, 0, 0));
    rule2.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule2.specializeCase3(0, 1, 1, 1));
    EXPECT_STREQ("parent(X0,X1):-father(X0,X1)", rule2.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(0, 2, 2);
    EXPECT_EQ(eval, rule2.getEval());
    EXPECT_EQ(2, rule2.usedLimitedVars());
    EXPECT_EQ(2, rule2.getLength());
    EXPECT_EQ(3, cache.size());
    EXPECT_EQ(0, tabuMap.size());
    expected_counter_examples.emplace(new int[2]{NUM_F3, NUM_S3}, 2);
    expected_counter_examples.emplace(new int[2]{NUM_F4, NUM_D4}, 2);
    actual_evidence = rule2.getEvidenceAndMarkEntailment();
    EXPECT_TRUE(actual_evidence->evidenceList.empty());
    EXPECT_EQ(actual_evidence->numPredicates, 2);
    EXPECT_EQ(actual_evidence->predicateSymbolsInRule[0], NUM_PARENT);
    EXPECT_EQ(actual_evidence->predicateSymbolsInRule[1], NUM_FATHER);
    EXPECT_EQ(actual_evidence->aritiesInRule[0], 2);
    EXPECT_EQ(actual_evidence->aritiesInRule[1], 2);
    actual_counterexamples = rule2.getCounterexamples();
    EXPECT_EQ(expected_counter_examples, *actual_counterexamples);
    delete actual_evidence;
    releaseCounterexampleSet(expected_counter_examples);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_counterexamples;

    delete kb;
    releaseCacheAndTabuMap();
}

TEST_F(TestCachedRule, TestFamilyRule2) {
    SimpleKb* kb = kbFamily();
    Eval eval(0, 0, 0);

    /* parent(?, ?) :- */
    CachedRule rule(NUM_PARENT, 2, cache, tabuMap, *kb);
    EXPECT_STREQ("parent(?,?):-", rule.toDumpString(kb->getRelationNames()).c_str());
    EXPECT_EQ(Eval(9, 16 * 16, 0), rule.getEval());
    EXPECT_EQ(0, rule.usedLimitedVars());
    EXPECT_EQ(0, rule.getLength());
    EXPECT_EQ(1, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* parent(?, X) :- father(?, X) */
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase4(NUM_FATHER, 2, 1, 0, 1));
    EXPECT_STREQ("parent(?,X0):-father(?,X0)", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(4, 5 * 16, 1);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(1, rule.usedLimitedVars());
    EXPECT_EQ(1, rule.getLength());
    EXPECT_EQ(2, cache.size());
    EXPECT_EQ(0, tabuMap.size());
    groundingSetType expected_grounding_set1;
    expected_grounding_set1.emplace(std::vector<Record>({PARENT1, FATHER1}));
    expected_grounding_set1.emplace(std::vector<Record>({PARENT3, FATHER2}));
    expected_grounding_set1.emplace(std::vector<Record>({PARENT4, FATHER3}));
    expected_grounding_set1.emplace(std::vector<Record>({PARENT5, FATHER3}));
    std::unordered_set<Record> expected_counter_examples;
    for (int arg1 = 1; arg1 <= kb->totalConstants(); arg1++) {
        for (int arg2: {NUM_S1, NUM_S2, NUM_D2, NUM_S3, NUM_D4}) {
            expected_counter_examples.emplace(new int[2]{arg1, arg2}, 2);
        }
    }
    for (Record const& r: {PARENT1, PARENT3, PARENT4, PARENT5}) {
        std::unordered_set<Record>::iterator itr = expected_counter_examples.find(r);
        if (expected_counter_examples.end() != itr) {
            delete[] itr->getArgs();
            expected_counter_examples.erase(itr);
        }
    }
    EvidenceBatch* actual_evidence = rule.getEvidenceAndMarkEntailment();
    int exp_relation_nums[2] {NUM_PARENT, NUM_FATHER};
    int exp_arities[2] {2, 2};
    groundingSetType* exp_groundings[1] {&expected_grounding_set1};
    checkEvidence(*actual_evidence, exp_relation_nums, exp_arities, 2 , exp_groundings, 1);
    std::unordered_set<Record>* actual_counterexamples = rule.getCounterexamples();
    EXPECT_EQ(expected_counter_examples, *actual_counterexamples);
    releaseGroundingSets(exp_groundings, 1);
    releaseCounterexampleSet(expected_counter_examples);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_evidence;
    delete actual_counterexamples;

    /* parent(Y, X) :- father(Y, X) */
    releaseCacheAndTabuMap();
    CachedRule rule2(NUM_PARENT, 2, cache, tabuMap, *kb);
    rule2.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule2.specializeCase4(NUM_FATHER, 2, 1, 0, 1));
    rule2.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule2.specializeCase3(0, 0, 1, 0));
    EXPECT_STREQ("parent(X1,X0):-father(X1,X0)", rule2.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(0, 2, 2);
    EXPECT_EQ(eval, rule2.getEval());
    EXPECT_EQ(2, rule2.usedLimitedVars());
    EXPECT_EQ(2, rule2.getLength());
    EXPECT_EQ(3, cache.size());
    EXPECT_EQ(0, tabuMap.size());
    actual_evidence = rule2.getEvidenceAndMarkEntailment();
    EXPECT_EQ(actual_evidence->numPredicates, 2);
    EXPECT_EQ(actual_evidence->predicateSymbolsInRule[0], NUM_PARENT);
    EXPECT_EQ(actual_evidence->predicateSymbolsInRule[1], NUM_FATHER);
    EXPECT_EQ(actual_evidence->aritiesInRule[0], 2);
    EXPECT_EQ(actual_evidence->aritiesInRule[1], 2);
    EXPECT_TRUE(actual_evidence->evidenceList.empty());
    expected_counter_examples.emplace(new int[2]{NUM_F3, NUM_S3}, 2);
    expected_counter_examples.emplace(new int[2]{NUM_F4, NUM_D4}, 2);
    actual_counterexamples = rule2.getCounterexamples();
    EXPECT_EQ(expected_counter_examples, *actual_counterexamples);
    releaseCounterexampleSet(expected_counter_examples);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_evidence;
    delete actual_counterexamples;

    delete kb;
    releaseCacheAndTabuMap();
}

TEST_F(TestCachedRule, TestFamilyRule3) {
    SimpleKb* kb = kbFamily();
    Eval eval(0, 0, 0);

    /* grandParent(?, ?) :- */
    CachedRule rule(NUM_GRANDPARENT, 2, cache, tabuMap, *kb);
    EXPECT_STREQ("grandParent(?,?):-", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(3, 16 * 16, 0);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(0, rule.usedLimitedVars());
    EXPECT_EQ(0, rule.getLength());
    EXPECT_EQ(1, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* grandParent(X, ?) :- parent(X, ?) */
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase4(NUM_PARENT, 2, 0, 0, 0));
    EXPECT_STREQ("grandParent(X0,?):-parent(X0,?)", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(2, 6 * 16, 1);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(1, rule.usedLimitedVars());
    EXPECT_EQ(1, rule.getLength());
    EXPECT_EQ(2, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* grandParent(X, Y) :- parent(X, ?), parent(?, Y) */
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase4(NUM_PARENT, 2, 1, 0, 1));
    EXPECT_STREQ("grandParent(X0,X1):-parent(X0,?),parent(?,X1)", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(2, 6 * 8, 2);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(2, rule.usedLimitedVars());
    EXPECT_EQ(2, rule.getLength());
    EXPECT_EQ(3, cache.size());
    EXPECT_EQ(0, tabuMap.size());
    groundingSetType expected_grounding_set1;
    expected_grounding_set1.emplace(std::vector<Record>({GRAND1, PARENT6, PARENT1}));
    expected_grounding_set1.emplace(std::vector<Record>({GRAND2, PARENT7, PARENT4}));
    groundingSetType expected_grounding_set2;
    expected_grounding_set2.emplace(std::vector<Record>({GRAND1, PARENT6, PARENT1}));
    expected_grounding_set2.emplace(std::vector<Record>({GRAND2, PARENT7, PARENT5}));
    groundingSetType expected_grounding_set3;
    expected_grounding_set3.emplace(std::vector<Record>({GRAND1, PARENT6, PARENT1}));
    expected_grounding_set3.emplace(std::vector<Record>({GRAND2, PARENT8, PARENT4}));
    groundingSetType expected_grounding_set4;
    expected_grounding_set4.emplace(std::vector<Record>({GRAND1, PARENT6, PARENT1}));
    expected_grounding_set4.emplace(std::vector<Record>({GRAND2, PARENT8, PARENT5}));
    std::unordered_set<Record> expected_counter_examples;
    for (int arg1 : {NUM_F1, NUM_F2, NUM_M2, NUM_G1, NUM_G2, NUM_G3}) {
        for (int arg2: {NUM_S1, NUM_S2, NUM_D1, NUM_D2, NUM_F1, NUM_F2, NUM_M2, NUM_F3}) {
            expected_counter_examples.emplace(new int[2]{arg1, arg2}, 2);
        }
    }
    for (Record const& r: {GRAND1, GRAND2}) {
        std::unordered_set<Record>::iterator itr = expected_counter_examples.find(r);
        if (expected_counter_examples.end() != itr) {
            delete[] itr->getArgs();
            expected_counter_examples.erase(itr);
        }
    }
    EvidenceBatch* actual_evidence = rule.getEvidenceAndMarkEntailment();
    int exp_relation_nums[3] {NUM_GRANDPARENT, NUM_PARENT, NUM_PARENT};
    int exp_arities[3] {2, 2, 2};
    groundingSetType* exp_groundings[4] {
        &expected_grounding_set1, &expected_grounding_set2, &expected_grounding_set3, &expected_grounding_set4
    };
    checkEvidence(*actual_evidence, exp_relation_nums, exp_arities, 3 , exp_groundings, 4);
    std::unordered_set<Record>* actual_counterexamples = rule.getCounterexamples();
    EXPECT_EQ(expected_counter_examples, *actual_counterexamples);
    releaseGroundingSets(exp_groundings, 4);
    releaseCounterexampleSet(expected_counter_examples);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_evidence;
    delete actual_counterexamples;

    /* grandParent(X, Y) :- parent(X, Z), parent(Z, Y) */
    releaseCacheAndTabuMap();
    CachedRule rule2(NUM_GRANDPARENT, 2, cache, tabuMap, *kb);
    EXPECT_EQ(UpdateStatus::Normal, rule2.specializeCase4(NUM_PARENT, 2, 0, 0, 0));
    rule2.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule2.specializeCase4(NUM_PARENT, 2, 1, 0, 1));
    rule2.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule2.specializeCase3(1, 1, 2, 0));
    EXPECT_STREQ("grandParent(X0,X1):-parent(X0,X2),parent(X2,X1)", rule2.toDumpString(kb->getRelationNames()).c_str());
    EXPECT_EQ(Eval(0, 2, 3), rule2.getEval());
    EXPECT_EQ(3, rule2.usedLimitedVars());
    EXPECT_EQ(3, rule2.getLength());
    EXPECT_EQ(4, cache.size());
    EXPECT_EQ(0, tabuMap.size());
    actual_evidence = rule2.getEvidenceAndMarkEntailment();
    EXPECT_EQ(actual_evidence->numPredicates, 3);
    EXPECT_EQ(actual_evidence->predicateSymbolsInRule[0], NUM_GRANDPARENT);
    EXPECT_EQ(actual_evidence->predicateSymbolsInRule[1], NUM_PARENT);
    EXPECT_EQ(actual_evidence->predicateSymbolsInRule[2], NUM_PARENT);
    EXPECT_EQ(actual_evidence->aritiesInRule[0], 2);
    EXPECT_EQ(actual_evidence->aritiesInRule[1], 2);
    EXPECT_EQ(actual_evidence->aritiesInRule[2], 2);
    EXPECT_TRUE(actual_evidence->evidenceList.empty());
    expected_counter_examples.emplace(new int[2]{NUM_G1, NUM_D1}, 2);
    expected_counter_examples.emplace(new int[2]{NUM_G2, NUM_S2}, 2);
    actual_counterexamples = rule2.getCounterexamples();
    EXPECT_EQ(expected_counter_examples, *actual_counterexamples);
    releaseCounterexampleSet(expected_counter_examples);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_evidence;
    delete actual_counterexamples;

    delete kb;
    releaseCacheAndTabuMap();
}

TEST_F(TestCachedRule, TestFamilyRule4) {
    SimpleKb* kb = kbFamily();
    Eval eval(0, 0, 0);

    /* grandParent(?, ?) :- */
    CachedRule rule(NUM_GRANDPARENT, 2, cache, tabuMap, *kb);
    EXPECT_STREQ("grandParent(?,?):-", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(3, 16 * 16, 0);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(0, rule.usedLimitedVars());
    EXPECT_EQ(0, rule.getLength());
    EXPECT_EQ(1, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* grandParent(X, ?) :- parent(X, ?) */
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase4(NUM_PARENT, 2, 0, 0, 0));
    EXPECT_STREQ("grandParent(X0,?):-parent(X0,?)", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(2, 6 * 16, 1);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(1, rule.usedLimitedVars());
    EXPECT_EQ(1, rule.getLength());
    EXPECT_EQ(2, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* grandParent(X, ?) :- parent(X, Y), parent(Y, ?) */
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase4(NUM_PARENT, 2, 0, 1, 1));
    EXPECT_STREQ("grandParent(X0,?):-parent(X0,X1),parent(X1,?)", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(2, 2 * 16, 2);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(2, rule.usedLimitedVars());
    EXPECT_EQ(2, rule.getLength());
    EXPECT_EQ(3, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* grandParent(X, Z) :- parent(X, Y), parent(Y, Z) */
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase3(2, 1, 0, 1));
    EXPECT_STREQ("grandParent(X0,X2):-parent(X0,X1),parent(X1,X2)", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(2, 4, 3);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(3, rule.usedLimitedVars());
    EXPECT_EQ(3, rule.getLength());
    EXPECT_EQ(4, cache.size());
    EXPECT_EQ(0, tabuMap.size());
    groundingSetType expected_grounding_set1;
    expected_grounding_set1.emplace(std::vector<Record>({GRAND1, PARENT6, PARENT1}));
    expected_grounding_set1.emplace(std::vector<Record>({GRAND2, PARENT7, PARENT4}));
    groundingSetType expected_grounding_set2;
    expected_grounding_set2.emplace(std::vector<Record>({GRAND1, PARENT6, PARENT1}));
    expected_grounding_set2.emplace(std::vector<Record>({GRAND2, PARENT7, PARENT5}));
    groundingSetType expected_grounding_set3;
    expected_grounding_set3.emplace(std::vector<Record>({GRAND1, PARENT6, PARENT1}));
    expected_grounding_set3.emplace(std::vector<Record>({GRAND2, PARENT8, PARENT4}));
    groundingSetType expected_grounding_set4;
    expected_grounding_set4.emplace(std::vector<Record>({GRAND1, PARENT6, PARENT1}));
    expected_grounding_set4.emplace(std::vector<Record>({GRAND2, PARENT8, PARENT5}));
    std::unordered_set<Record> expected_counter_examples;
    expected_counter_examples.emplace(new int[2]{NUM_G1, NUM_D1}, 2);
    expected_counter_examples.emplace(new int[2]{NUM_G2, NUM_S2}, 2);
    EvidenceBatch* actual_evidence = rule.getEvidenceAndMarkEntailment();
    int exp_relation_nums[3] {NUM_GRANDPARENT, NUM_PARENT, NUM_PARENT};
    int exp_arities[3] {2, 2, 2};
    groundingSetType* exp_groundings[4] {
        &expected_grounding_set1, &expected_grounding_set2, &expected_grounding_set3, &expected_grounding_set4
    };
    checkEvidence(*actual_evidence, exp_relation_nums, exp_arities, 3 , exp_groundings, 4);
    std::unordered_set<Record>* actual_counterexamples = rule.getCounterexamples();
    EXPECT_EQ(expected_counter_examples, *actual_counterexamples);
    releaseGroundingSets(exp_groundings, 4);
    releaseCounterexampleSet(expected_counter_examples);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_evidence;
    delete actual_counterexamples;

    delete kb;
    releaseCacheAndTabuMap();
}

TEST_F(TestCachedRule, TestFamilyRule5) {
    SimpleKb* kb = kbFamily();
    Eval eval(0, 0, 0);

    /* grandParent(?, ?) :- */
    CachedRule rule(NUM_GRANDPARENT, 2, cache, tabuMap, *kb);
    EXPECT_STREQ("grandParent(?,?):-", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(3, 16 * 16, 0);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(0, rule.usedLimitedVars());
    EXPECT_EQ(0, rule.getLength());
    EXPECT_EQ(1, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* grandParent(X, ?) :- parent(X, ?) */
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase4(NUM_PARENT, 2, 0, 0, 0));
    EXPECT_STREQ("grandParent(X0,?):-parent(X0,?)", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(2, 6 * 16, 1);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(1, rule.usedLimitedVars());
    EXPECT_EQ(1, rule.getLength());
    EXPECT_EQ(2, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* grandParent(X, ?) :- parent(X, Y), father(Y, ?) */
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase4(NUM_FATHER, 2, 0, 1, 1));
    EXPECT_STREQ("grandParent(X0,?):-parent(X0,X1),father(X1,?)", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(2, 3 * 16, 2);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(2, rule.usedLimitedVars());
    EXPECT_EQ(2, rule.getLength());
    EXPECT_EQ(3, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* grandParent(X, Z) :- parent(X, Y), father(Y, Z) */
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase3(2, 1, 0, 1));
    EXPECT_STREQ("grandParent(X0,X2):-parent(X0,X1),father(X1,X2)", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(2, 4, 3);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(3, rule.usedLimitedVars());
    EXPECT_EQ(3, rule.getLength());
    EXPECT_EQ(4, cache.size());
    EXPECT_EQ(0, tabuMap.size());
    groundingSetType expected_grounding_set1;
    expected_grounding_set1.emplace(std::vector<Record>({GRAND1, PARENT6, FATHER1}));
    expected_grounding_set1.emplace(std::vector<Record>({GRAND2, PARENT7, FATHER3}));
    std::unordered_set<Record> expected_counter_examples;
    expected_counter_examples.emplace(new int[2]{NUM_G3, NUM_S3}, 2);
    expected_counter_examples.emplace(new int[2]{NUM_G2, NUM_S2}, 2);
    EvidenceBatch* actual_evidence = rule.getEvidenceAndMarkEntailment();
    int exp_relation_nums[3] {NUM_GRANDPARENT, NUM_PARENT, NUM_FATHER};
    int exp_arities[3] {2, 2, 2};
    groundingSetType* exp_groundings[1] {&expected_grounding_set1};
    checkEvidence(*actual_evidence, exp_relation_nums, exp_arities, 3 , exp_groundings, 1);
    std::unordered_set<Record>* actual_counterexamples = rule.getCounterexamples();
    EXPECT_EQ(expected_counter_examples, *actual_counterexamples);
    releaseGroundingSets(exp_groundings, 1);
    releaseCounterexampleSet(expected_counter_examples);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_evidence;
    delete actual_counterexamples;

    delete kb;
    releaseCacheAndTabuMap();
}

TEST_F(TestCachedRule, TestFamilyRule6) {
    SimpleKb* kb = kbFamily();
    Eval eval(0, 0, 0);

    /* grandParent(?, ?) :- */
    CachedRule rule(NUM_GRANDPARENT, 2, cache, tabuMap, *kb);
    EXPECT_STREQ("grandParent(?,?):-", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(3, 16 * 16, 0);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(0, rule.usedLimitedVars());
    EXPECT_EQ(0, rule.getLength());
    EXPECT_EQ(1, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* grandParent(?, X) :- father(?, X) */
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase4(NUM_FATHER, 2, 1, 0, 1));
    EXPECT_STREQ("grandParent(?,X0):-father(?,X0)", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(2, 5 * 16, 1);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(1, rule.usedLimitedVars());
    EXPECT_EQ(1, rule.getLength());
    EXPECT_EQ(2, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* grandParent(g1, X) :- father(?, X) */
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase5(0, 0, NUM_G1));
    EXPECT_STREQ("grandParent(1,X0):-father(?,X0)", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(1, 5, 2);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(1, rule.usedLimitedVars());
    EXPECT_EQ(2, rule.getLength());
    EXPECT_EQ(3, cache.size());
    EXPECT_EQ(0, tabuMap.size());
    groundingSetType expected_grounding_set1;
    expected_grounding_set1.emplace(std::vector<Record>({GRAND1, FATHER1}));
    std::unordered_set<Record> expected_counter_examples;
    for (int arg2: {NUM_S2, NUM_D2, NUM_S3, NUM_D4}) {
        expected_counter_examples.emplace(new int[2]{NUM_G1, arg2}, 2);
    }
    EvidenceBatch* actual_evidence = rule.getEvidenceAndMarkEntailment();
    int exp_relation_nums[2] {NUM_GRANDPARENT, NUM_FATHER};
    int exp_arities[2] {2, 2};
    groundingSetType* exp_groundings[1] {&expected_grounding_set1};
    checkEvidence(*actual_evidence, exp_relation_nums, exp_arities, 2 , exp_groundings, 1);
    std::unordered_set<Record>* actual_counterexamples = rule.getCounterexamples();
    EXPECT_EQ(expected_counter_examples, *actual_counterexamples);
    releaseGroundingSets(exp_groundings, 1);
    releaseCounterexampleSet(expected_counter_examples);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_evidence;
    delete actual_counterexamples;

    /* grandParent(g1, X) :- father(f2, X) */
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase5(1, 0, NUM_F2));
    EXPECT_STREQ("grandParent(1,X0):-father(6,X0)", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(0, 2, 3);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(1, rule.usedLimitedVars());
    EXPECT_EQ(3, rule.getLength());
    EXPECT_EQ(4, cache.size());
    EXPECT_EQ(0, tabuMap.size());
    actual_evidence = rule.getEvidenceAndMarkEntailment();
    EXPECT_EQ(actual_evidence->numPredicates, 2);
    EXPECT_EQ(actual_evidence->predicateSymbolsInRule[0], NUM_GRANDPARENT);
    EXPECT_EQ(actual_evidence->predicateSymbolsInRule[1], NUM_FATHER);
    EXPECT_EQ(actual_evidence->aritiesInRule[0], 2);
    EXPECT_EQ(actual_evidence->aritiesInRule[1], 2);
    EXPECT_TRUE(actual_evidence->evidenceList.empty());
    expected_counter_examples.emplace(new int[2]{NUM_G1, NUM_S2}, 2);
    expected_counter_examples.emplace(new int[2]{NUM_G1, NUM_D2}, 2);
    actual_counterexamples = rule.getCounterexamples();
    EXPECT_EQ(expected_counter_examples, *actual_counterexamples);
    releaseCounterexampleSet(expected_counter_examples);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_evidence;
    delete actual_counterexamples;

    delete kb;
    releaseCacheAndTabuMap();
}

TEST_F(TestCachedRule, TestFamilyRule7) {
    SimpleKb* kb = kbFamily();
    Eval eval(0, 0, 0);

    /* parent(?, ?) :- */
    CachedRule rule(NUM_PARENT, 2, cache, tabuMap, *kb);
    EXPECT_STREQ("parent(?,?):-", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(9, 16 * 16, 0);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(0, rule.usedLimitedVars());
    EXPECT_EQ(0, rule.getLength());
    EXPECT_EQ(1, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* parent(X, X) :- */
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase3(0, 0, 0, 1));
    EXPECT_STREQ("parent(X0,X0):-", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(0, 16, 1);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(1, rule.usedLimitedVars());
    EXPECT_EQ(1, rule.getLength());
    EXPECT_EQ(2, cache.size());
    EXPECT_EQ(0, tabuMap.size());
    std::unordered_set<Record> expected_counter_examples;
    for (int arg = 1; arg <= kb->totalConstants(); arg++) {
        expected_counter_examples.emplace(new int[2]{arg, arg}, 2);
    }
    EvidenceBatch* actual_evidence = rule.getEvidenceAndMarkEntailment();
    EXPECT_EQ(actual_evidence->numPredicates, 1);
    EXPECT_EQ(actual_evidence->predicateSymbolsInRule[0], NUM_PARENT);
    EXPECT_EQ(actual_evidence->aritiesInRule[0], 2);
    EXPECT_TRUE(actual_evidence->evidenceList.empty());
    std::unordered_set<Record>* actual_counterexamples = rule.getCounterexamples();
    EXPECT_EQ(expected_counter_examples, *actual_counterexamples);
    releaseCounterexampleSet(expected_counter_examples);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_evidence;
    delete actual_counterexamples;

    delete kb;
    releaseCacheAndTabuMap();
}

TEST_F(TestCachedRule, TestFamilyRule8) {
    SimpleKb* kb = kbFamily();
    Eval eval(0, 0, 0);

    /* father(?, ?) :- */
    CachedRule rule(NUM_FATHER, 2, cache, tabuMap, *kb);
    EXPECT_STREQ("father(?,?):-", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(5, 16 * 16, 0);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(0, rule.usedLimitedVars());
    EXPECT_EQ(0, rule.getLength());
    EXPECT_EQ(1, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* father(X, ?):- parent(?, X) */
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase4(NUM_PARENT, 2, 1, 0, 0));
    EXPECT_STREQ("father(X0,?):-parent(?,X0)", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(4, 8 * 16, 1);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(1, rule.usedLimitedVars());
    EXPECT_EQ(1, rule.getLength());
    EXPECT_EQ(2, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* father(X, ?):- parent(?, X), parent(X, ?) */
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase2(NUM_PARENT, 2, 0, 0));
    EXPECT_STREQ("father(X0,?):-parent(?,X0),parent(X0,?)", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(3, 3 * 16, 2);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(1, rule.usedLimitedVars());
    EXPECT_EQ(2, rule.getLength());
    EXPECT_EQ(3, cache.size());
    EXPECT_EQ(0, tabuMap.size());
    groundingSetType expected_grounding_set1;
    expected_grounding_set1.emplace(std::vector<Record>({FATHER1, PARENT6, PARENT1}));
    expected_grounding_set1.emplace(std::vector<Record>({FATHER2, PARENT7, PARENT3}));
    expected_grounding_set1.emplace(std::vector<Record>({FATHER3, PARENT7, PARENT3}));
    groundingSetType expected_grounding_set2;
    expected_grounding_set2.emplace(std::vector<Record>({FATHER1, PARENT6, PARENT1}));
    expected_grounding_set2.emplace(std::vector<Record>({FATHER2, PARENT7, PARENT3}));
    expected_grounding_set2.emplace(std::vector<Record>({FATHER3, PARENT7, PARENT4}));
    groundingSetType expected_grounding_set3;
    expected_grounding_set3.emplace(std::vector<Record>({FATHER1, PARENT6, PARENT1}));
    expected_grounding_set3.emplace(std::vector<Record>({FATHER2, PARENT7, PARENT4}));
    expected_grounding_set3.emplace(std::vector<Record>({FATHER3, PARENT7, PARENT3}));
    groundingSetType expected_grounding_set4;
    expected_grounding_set4.emplace(std::vector<Record>({FATHER1, PARENT6, PARENT1}));
    expected_grounding_set4.emplace(std::vector<Record>({FATHER2, PARENT7, PARENT4}));
    expected_grounding_set4.emplace(std::vector<Record>({FATHER3, PARENT7, PARENT4}));
    groundingSetType expected_grounding_set5;
    expected_grounding_set5.emplace(std::vector<Record>({FATHER1, PARENT6, PARENT2}));
    expected_grounding_set5.emplace(std::vector<Record>({FATHER2, PARENT7, PARENT3}));
    expected_grounding_set5.emplace(std::vector<Record>({FATHER3, PARENT7, PARENT3}));
    groundingSetType expected_grounding_set6;
    expected_grounding_set6.emplace(std::vector<Record>({FATHER1, PARENT6, PARENT2}));
    expected_grounding_set6.emplace(std::vector<Record>({FATHER2, PARENT7, PARENT3}));
    expected_grounding_set6.emplace(std::vector<Record>({FATHER3, PARENT7, PARENT4}));
    groundingSetType expected_grounding_set7;
    expected_grounding_set7.emplace(std::vector<Record>({FATHER1, PARENT6, PARENT2}));
    expected_grounding_set7.emplace(std::vector<Record>({FATHER2, PARENT7, PARENT4}));
    expected_grounding_set7.emplace(std::vector<Record>({FATHER3, PARENT7, PARENT3}));
    groundingSetType expected_grounding_set8;
    expected_grounding_set8.emplace(std::vector<Record>({FATHER1, PARENT6, PARENT2}));
    expected_grounding_set8.emplace(std::vector<Record>({FATHER2, PARENT7, PARENT4}));
    expected_grounding_set8.emplace(std::vector<Record>({FATHER3, PARENT7, PARENT4}));
    std::unordered_set<Record> expected_counter_examples;
    for (int arg1 : {NUM_F1, NUM_F2, NUM_M2}) {
        for (int arg2 = 1; arg2 <= kb->totalConstants(); arg2++) {
            expected_counter_examples.emplace(new int[2]{arg1, arg2}, 2);
        }
    }
    for (Record const& r: {FATHER1, FATHER2, FATHER3}) {
        std::unordered_set<Record>::iterator itr = expected_counter_examples.find(r);
        if (expected_counter_examples.end() != itr) {
            delete[] itr->getArgs();
            expected_counter_examples.erase(itr);
        }
    }
    EvidenceBatch* actual_evidence = rule.getEvidenceAndMarkEntailment();
    int exp_relation_nums[3] {NUM_FATHER, NUM_PARENT, NUM_PARENT};
    int exp_arities[3] {2, 2, 2};
    groundingSetType* exp_groundings[8] {
        &expected_grounding_set1, &expected_grounding_set2, &expected_grounding_set3, &expected_grounding_set4,
        &expected_grounding_set5, &expected_grounding_set6, &expected_grounding_set7, &expected_grounding_set8,
    };
    checkEvidence(*actual_evidence, exp_relation_nums, exp_arities, 3 , exp_groundings, 8);
    std::unordered_set<Record>* actual_counterexamples = rule.getCounterexamples();
    EXPECT_EQ(expected_counter_examples, *actual_counterexamples);
    releaseGroundingSets(exp_groundings, 8);
    releaseCounterexampleSet(expected_counter_examples);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_evidence;
    delete actual_counterexamples;

    delete kb;
    releaseCacheAndTabuMap();
}

TEST_F(TestCachedRule, TestFamilyRule9) {
    SimpleKb* kb = kbFamily();
    Eval eval(0, 0, 0);

    /* father(?, ?) :- */
    CachedRule rule(NUM_FATHER, 2, cache, tabuMap, *kb);
    EXPECT_STREQ("father(?,?):-", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(5, 16 * 16, 0);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(0, rule.usedLimitedVars());
    EXPECT_EQ(0, rule.getLength());
    EXPECT_EQ(1, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* #1: father(f2,?):- */
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase5(0, 0, NUM_F2));
    EXPECT_STREQ("father(6,?):-", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(2, 16, 1);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(0, rule.usedLimitedVars());
    EXPECT_EQ(1, rule.getLength());
    EXPECT_EQ(2, cache.size());
    EXPECT_EQ(0, tabuMap.size());
    delete rule.getEvidenceAndMarkEntailment();

    /* father(?, ?) :- */
    releaseCacheAndTabuMap();
    CachedRule rule2(NUM_FATHER, 2, cache, tabuMap, *kb);
    EXPECT_STREQ("father(?,?):-", rule2.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(3, 16 * 16 - 2, 0);
    EXPECT_EQ(eval, rule2.getEval());
    EXPECT_EQ(0, rule2.usedLimitedVars());
    EXPECT_EQ(0, rule2.getLength());
    EXPECT_EQ(1, cache.size());
    EXPECT_EQ(0, tabuMap.size());
    groundingSetType expected_grounding_set1;
    expected_grounding_set1.emplace(std::vector<Record>({FATHER1}));
    expected_grounding_set1.emplace(std::vector<Record>({FATHER4}));
    expected_grounding_set1.emplace(std::vector<Record>({FATHER5}));
    std::unordered_set<Record> expected_counter_examples;
    for (int arg1 = 1; arg1 <= kb->totalConstants(); arg1++) {
        for (int arg2 = 1; arg2 <= kb->totalConstants(); arg2++) {
            expected_counter_examples.emplace(new int[2]{arg1, arg2}, 2);
        }
    }
    for (Record const& r: {FATHER1, FATHER2, FATHER3, FATHER4, FATHER5}) {
        std::unordered_set<Record>::iterator itr = expected_counter_examples.find(r);
        if (expected_counter_examples.end() != itr) {
            delete[] itr->getArgs();
            expected_counter_examples.erase(itr);
        }
    }
    EvidenceBatch* actual_evidence = rule2.getEvidenceAndMarkEntailment();
    int exp_relation_nums[1] {NUM_FATHER};
    int exp_arities[1] {2};
    groundingSetType* exp_groundings[1] {&expected_grounding_set1};
    checkEvidence(*actual_evidence, exp_relation_nums, exp_arities, 1 , exp_groundings, 1);
    std::unordered_set<Record>* actual_counterexamples = rule2.getCounterexamples();
    EXPECT_EQ(expected_counter_examples, *actual_counterexamples);
    releaseGroundingSets(exp_groundings, 1);
    releaseCounterexampleSet(expected_counter_examples);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_evidence;
    delete actual_counterexamples;

    delete kb;
    releaseCacheAndTabuMap();
}

TEST_F(TestCachedRule, TestCounterexample) {
    SimpleKb* kb = kbFamily();

    /* father(?, ?) :- */
    CachedRule rule(NUM_FATHER, 2, cache, tabuMap, *kb);
    EXPECT_STREQ("father(?,?):-", rule.toDumpString(kb->getRelationNames()).c_str());
    Eval eval(5, 16 * 16, 0);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(0, rule.usedLimitedVars());
    EXPECT_EQ(0, rule.getLength());
    EXPECT_EQ(1, cache.size());
    EXPECT_EQ(0, tabuMap.size());
    std::unordered_set<Record> expected_counter_examples;
    for (int arg1 = 1; arg1 <= kb->totalConstants(); arg1++) {
        for (int arg2 = 1; arg2 <= kb->totalConstants(); arg2++) {
            expected_counter_examples.emplace(new int[2]{arg1, arg2}, 2);
        }
    }
    for (Record const& r: {FATHER1, FATHER2, FATHER3, FATHER4, FATHER5}) {
        std::unordered_set<Record>::iterator itr = expected_counter_examples.find(r);
        if (expected_counter_examples.end() != itr) {
            delete[] itr->getArgs();
            expected_counter_examples.erase(itr);
        }
    }
    std::unordered_set<Record>* actual_counterexamples = rule.getCounterexamples();
    EXPECT_EQ(expected_counter_examples, *actual_counterexamples);
    releaseCounterexampleSet(expected_counter_examples);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_counterexamples;

    delete kb;
    releaseCacheAndTabuMap();
}

TEST_F(TestCachedRule, TestCopy1) {
    SimpleKb* kb = kbFamily();
    Eval eval(0, 0, 0);

    /* grandParent(?, ?) :- */
    CachedRule rule(NUM_GRANDPARENT, 2, cache, tabuMap, *kb);
    EXPECT_STREQ("grandParent(?,?):-", rule.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(3, 16 * 16, 0);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(0, rule.usedLimitedVars());
    EXPECT_EQ(0, rule.getLength());
    EXPECT_EQ(1, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* #1: grandParent(X, ?) :- parent(X, ?) */
    CachedRule* rule1 = rule.clone();
    rule1->updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule1->specializeCase4(NUM_PARENT, 2, 0, 0, 0));
    EXPECT_STREQ("grandParent(X0,?):-parent(X0,?)", rule1->toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(2, 6 * 16, 1);
    EXPECT_EQ(eval, rule1->getEval());
    EXPECT_EQ(1, rule1->usedLimitedVars());
    EXPECT_EQ(1, rule1->getLength());
    EXPECT_EQ(2, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* #1: grandParent(X, ?) :- parent(X, Y), father(Y, ?) */
    rule1->updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule1->specializeCase4(NUM_FATHER, 2, 0, 1, 1));
    EXPECT_STREQ("grandParent(X0,?):-parent(X0,X1),father(X1,?)", rule1->toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(2, 3 * 16, 2);
    EXPECT_EQ(eval, rule1->getEval());
    EXPECT_EQ(2, rule1->usedLimitedVars());
    EXPECT_EQ(2, rule1->getLength());
    EXPECT_EQ(3, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* #1: grandParent(X, Z) :- parent(X, Y), father(Y, Z) */
    rule1->updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule1->specializeCase3(0, 1, 2, 1));
    EXPECT_STREQ("grandParent(X0,X2):-parent(X0,X1),father(X1,X2)", rule1->toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(2, 4, 3);
    EXPECT_EQ(eval, rule1->getEval());
    EXPECT_EQ(3, rule1->usedLimitedVars());
    EXPECT_EQ(3, rule1->getLength());
    EXPECT_EQ(4, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* #2: grandParent(X, ?) :- parent(X, ?) */
    CachedRule* rule2 = rule.clone();
    rule2->updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Duplicated, rule2->specializeCase4(NUM_PARENT, 2, 0, 0, 0));
    EXPECT_EQ(4, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* #3: grandParent(?, X) :- father(?, X) */
    CachedRule* rule3 = rule.clone();
    rule3->updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule3->specializeCase4(NUM_FATHER, 2, 1, 0, 1));
    EXPECT_STREQ("grandParent(?,X0):-father(?,X0)", rule3->toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(2, 5 * 16, 1);
    EXPECT_EQ(eval, rule3->getEval());
    EXPECT_EQ(1, rule3->usedLimitedVars());
    EXPECT_EQ(1, rule3->getLength());
    EXPECT_EQ(5, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* #3: grandParent(Y, X) :- father(?, X), parent(Y, ?) */
    rule3->updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule3->specializeCase4(NUM_PARENT, 2, 0, 0, 0));
    EXPECT_STREQ("grandParent(X1,X0):-father(?,X0),parent(X1,?)", rule3->toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(2, 5 * 6, 2);
    EXPECT_EQ(eval, rule3->getEval());
    EXPECT_EQ(2, rule3->usedLimitedVars());
    EXPECT_EQ(2, rule3->getLength());
    EXPECT_EQ(6, cache.size());
    EXPECT_EQ(0, tabuMap.size());
    std::unordered_set<Record> expected_counter_examples;
    for (int arg1 : {NUM_F1, NUM_F2, NUM_M2, NUM_G1, NUM_G2, NUM_G3}) {
        for (int arg2: {NUM_S1, NUM_S2, NUM_D2, NUM_S3, NUM_D4}) {
            expected_counter_examples.emplace(new int[2]{arg1, arg2}, 2);
        }
    }
    for (Record const& r: {GRAND1, GRAND2}) {
        std::unordered_set<Record>::iterator itr = expected_counter_examples.find(r);
        if (expected_counter_examples.end() != itr) {
            delete[] itr->getArgs();
            expected_counter_examples.erase(itr);
        }
    }
    std::unordered_set<Record>* actual_counterexamples = rule3->getCounterexamples();
    EXPECT_EQ(expected_counter_examples, *actual_counterexamples);
    releaseCounterexampleSet(expected_counter_examples);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_counterexamples;

    /* #3: grandParent(Y, X) :- father(Z, X), parent(Y, Z) */
    rule3->updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Duplicated, rule3->specializeCase3(1, 0, 2, 1));
    EXPECT_EQ(6, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* Test rule1 */
    groundingSetType expected_grounding_set1;
    expected_grounding_set1.emplace(std::vector<Record>({GRAND1, PARENT6, FATHER1}));
    expected_grounding_set1.emplace(std::vector<Record>({GRAND2, PARENT7, FATHER3}));
    expected_counter_examples.emplace(new int[2]{NUM_G2, NUM_S2}, 2);
    expected_counter_examples.emplace(new int[2]{NUM_G3, NUM_S3}, 2);
    EvidenceBatch* actual_evidence = rule1->getEvidenceAndMarkEntailment();
    int exp_relation_nums[3] {NUM_GRANDPARENT, NUM_PARENT, NUM_FATHER};
    int exp_arities[3] {2, 2, 2};
    groundingSetType* exp_groundings[1] {&expected_grounding_set1};
    checkEvidence(*actual_evidence, exp_relation_nums, exp_arities, 3 , exp_groundings, 1);
    actual_counterexamples = rule1->getCounterexamples();
    EXPECT_EQ(expected_counter_examples, *actual_counterexamples);
    releaseGroundingSets(exp_groundings, 1);
    releaseCounterexampleSet(expected_counter_examples);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_evidence;
    delete actual_counterexamples;

    delete rule1;
    delete rule2;
    delete rule3;

    delete kb;
    releaseCacheAndTabuMap();
}

TEST_F(TestCachedRule, TestCopy2) {
    SimpleKb* kb = kbFamily();
    Eval eval(0, 0, 0);

    /* parent(?, ?) :- */
    CachedRule rule1(NUM_PARENT, 2, cache, tabuMap, *kb);
    EXPECT_STREQ("parent(?,?):-", rule1.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(9, 16 * 16, 0);
    EXPECT_EQ(eval, rule1.getEval());
    EXPECT_EQ(0, rule1.usedLimitedVars());
    EXPECT_EQ(0, rule1.getLength());
    EXPECT_EQ(1, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* #1: parent(f2, ?) :- */
    rule1.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule1.specializeCase5(0, 0, NUM_F2));
    EXPECT_STREQ("parent(6,?):-", rule1.toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(2, 16, 1);
    EXPECT_EQ(eval, rule1.getEval());
    EXPECT_EQ(0, rule1.usedLimitedVars());
    EXPECT_EQ(1, rule1.getLength());
    EXPECT_EQ(2, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* #2: parent(f2, d2) :- */
    CachedRule* rule2 = rule1.clone();
    rule2->updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule2->specializeCase5(0, 1, NUM_D2));
    EXPECT_STREQ("parent(6,15):-", rule2->toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(1, 1, 2);
    EXPECT_EQ(eval, rule2->getEval());
    EXPECT_EQ(0, rule2->usedLimitedVars());
    EXPECT_EQ(2, rule2->getLength());
    EXPECT_EQ(3, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* #3: parent(f2, X) :- father(?, X) */
    CachedRule* rule3 = rule1.clone();
    rule3->updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule3->specializeCase4(NUM_FATHER, 2, 1, 0, 1));
    EXPECT_STREQ("parent(6,X0):-father(?,X0)", rule3->toDumpString(kb->getRelationNames()).c_str());
    eval = Eval(2, 5, 2);
    EXPECT_EQ(eval, rule3->getEval());
    EXPECT_EQ(1, rule3->usedLimitedVars());
    EXPECT_EQ(2, rule3->getLength());
    EXPECT_EQ(4, cache.size());
    EXPECT_EQ(0, tabuMap.size());

    /* Test Rule 2 */
    groundingSetType expected_grounding_set1;
    expected_grounding_set1.emplace(std::vector<Record>({PARENT4}));
    EvidenceBatch* actual_evidence = rule2->getEvidenceAndMarkEntailment();
    int exp_relation_nums[1] {NUM_PARENT};
    int exp_arities[1] {2};
    groundingSetType* exp_groundings[1] {&expected_grounding_set1};
    checkEvidence(*actual_evidence, exp_relation_nums, exp_arities, 1 , exp_groundings, 1);
    std::unordered_set<Record>* actual_counterexamples = rule2->getCounterexamples();
    EXPECT_TRUE(actual_counterexamples->empty());
    releaseGroundingSets(exp_groundings, 1);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_evidence;
    delete actual_counterexamples;

    delete rule2;
    delete rule3;

    delete kb;
    releaseCacheAndTabuMap();
}

TEST_F(TestCachedRule, TestValidity1) {
    SimpleKb* kb = kbFamily();

    /* father(?,?):- */
    CachedRule rule(NUM_FATHER, 2, cache, tabuMap, *kb);
    EXPECT_STREQ("father(?,?):-", rule.toDumpString(kb->getRelationNames()).c_str());

    /* #1: father(X,?) :- father(?,X) */
    CachedRule rule1(rule);
    rule1.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule1.specializeCase4(NUM_FATHER, 2, 1, 0, 0));
    EXPECT_STREQ("father(X0,?):-father(?,X0)", rule1.toDumpString(kb->getRelationNames()).c_str());

    /* #1: father(X,Y) :- father(Y,X) */
    rule1.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule1.specializeCase3(0, 1, 1, 0));
    EXPECT_STREQ("father(X0,X1):-father(X1,X0)", rule1.toDumpString(kb->getRelationNames()).c_str());

    /* #2: father(X,?) :- father(X,?) [now should be invalid] */
    CachedRule rule2(rule);
    rule2.updateCacheIndices();
    // EXPECT_EQ(UpdateStatus::Normal, rule2.specializeCase4(NUM_FATHER, 2, 0, 0, 0));
    EXPECT_EQ(UpdateStatus::Invalid, rule2.specializeCase4(NUM_FATHER, 2, 0, 0, 0));

    delete kb;
    releaseCacheAndTabuMap();
}

TEST_F(TestCachedRule, TestValidity2) {
    SimpleKb* kb = kbFamily();

    /* father(?,?):- */
    CachedRule rule(NUM_FATHER, 2, cache, tabuMap, *kb);
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase4(NUM_FATHER, 2, 1, 0, 0));
    EXPECT_STREQ("father(X0,?):-father(?,X0)", rule.toDumpString(kb->getRelationNames()).c_str());

    /* father(X,?) :- father(?,X), father(?,X) */
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase2(NUM_FATHER, 2, 1, 0));
    EXPECT_STREQ("father(X0,?):-father(?,X0),father(?,X0)", rule.toDumpString(kb->getRelationNames()).c_str());

    /* father(X,?) :- father(Y,X), father(Y,X) [invalid] */
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Invalid, rule.specializeCase3(1, 0, 2, 0));

    delete kb;
    releaseCacheAndTabuMap();
}

TEST_F(TestCachedRule, TestRcPruning1) {
    Rule::MinFactCoverage = 0.44;

    SimpleKb* kb = kbFamily();

    /* parent(X, ?) :- father(X, ?) */
    CachedRule rule(NUM_PARENT, 2, cache, tabuMap, *kb);
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase4(NUM_FATHER, 2, 0, 0, 0));
    EXPECT_STREQ("parent(X0,?):-father(X0,?)", rule.toDumpString(kb->getRelationNames()).c_str());
    Eval eval(4, 4 * 16, 1);
    EXPECT_EQ(eval, rule.getEval());

    delete kb;
    releaseCacheAndTabuMap();
}

TEST_F(TestCachedRule, TestRcPruning2) {
    Rule::MinFactCoverage = 0.45;

    SimpleKb* kb = kbFamily();

    class CachedRule4Test : public CachedRule {
    public:
        CachedRule4Test(
            int const headPredSymbol, int const arity, fingerprintCacheType& fingerprintCache, tabuMapType& category2TabuSetMap, SimpleKb& _kb
        ) : CachedRule(headPredSymbol, arity, fingerprintCache, category2TabuSetMap, _kb) {}
        using CachedRule::recordCoverage;
    };

    /* parent(X, ?) :- father(X, ?) */
    CachedRule4Test rule(NUM_PARENT, 2, cache, tabuMap, *kb);
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::InsufficientCoverage, rule.specializeCase4(NUM_FATHER, 2, 0, 0, 0));
    EXPECT_EQ(4.0 / 9.0, rule.recordCoverage());
    EXPECT_EQ(1, tabuMap.size());
    MultiSet<int> category;
    category.add(NUM_FATHER);
    Rule::tabuMapType::iterator tabu_itr = tabuMap.find(&category);
    ASSERT_NE(tabu_itr, tabuMap.end());
    Rule::fingerprintCacheType::iterator cache_itr = tabu_itr->second->find(&(rule.getFingerprint()));
    ASSERT_NE(cache_itr, tabu_itr->second->end());

    delete kb;
    releaseCacheAndTabuMap();
}

TEST_F(TestCachedRule, TestAnyRule1) {
    int const a = 1;
    int const A = 2;
    int const plus = 3;
    int const b = 4;
    int const B = 5;
    int const c = 6;
    int const C = 7;
    int const minus = 8;
    int* const p1 = new int[3]{a, A, plus};
    int* const p2 = new int[3]{b, B, plus};
    int* const p3 = new int[3]{c, C, minus};
    int* const h1 = new int[4]{a, a, A, A};
    int* const h2 = new int[4]{b, b, B, B};
    int*** relations = new int**[2] {
            new int*[3] {p1, p2, p3},
            new int*[2] {h1, h2}
    };
    int total_rows[2] {3, 2};
    int arities[2] {3, 4};
    std::string names[2] {"p", "h"};
    SimpleKb kb("TestCachedRule", relations, names, arities, total_rows, 2);
    SimpleRelation& rel_p = *(kb.getRelation("p"));
    SimpleRelation& rel_h = *(kb.getRelation("h"));

    /* h(X, X, Y, Y) :- p(X, Y, +) */
    CachedRule rule(rel_h.id, 4, cache, tabuMap, kb);
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase4(rel_p.id, 3, 0, 0, 0));
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase3(0, 2, 1, 1));
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase1(0, 1, 0));
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase1(0, 3, 1));
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase5(1, 2, plus));
    EXPECT_STREQ("h(X0,X0,X1,X1):-p(X0,X1,3)", rule.toDumpString(kb.getRelationNames()).c_str());
    Eval eval(2, 2, 5);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(2, rule.usedLimitedVars());
    EXPECT_EQ(5, rule.getLength());
    groundingSetType expected_grounding_set;
    expected_grounding_set.emplace(std::vector<Record>({Record(h1, 4), Record(p1, 3)}));
    expected_grounding_set.emplace(std::vector<Record>({Record(h2, 4), Record(p2, 3)}));
    EvidenceBatch* actual_evidence = rule.getEvidenceAndMarkEntailment();
    int exp_relation_nums[2] {rel_h.id, rel_p.id};
    int exp_arities[2] {4, 3};
    groundingSetType* exp_groundings[1] {&expected_grounding_set};
    checkEvidence(*actual_evidence, exp_relation_nums, exp_arities, 2 , exp_groundings, 1);
    std::unordered_set<Record>* actual_counterexamples = rule.getCounterexamples();
    EXPECT_TRUE(actual_counterexamples->empty());
    releaseGroundingSets(exp_groundings, 1);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_evidence;
    delete actual_counterexamples;

    delete[] p1;
    delete[] p2;
    delete[] p3;
    delete[] h1;
    delete[] h2;
    delete[] relations[0];
    delete[] relations[1];
    delete[] relations;
    releaseCacheAndTabuMap();
}

TEST_F(TestCachedRule, TestAnyRule2) {
    int const a = 1;
    int const A = 2;
    int const b = 3;
    int* const p1 = new int[2]{A, a};
    int* const p2 = new int[2]{a, a};
    int* const p3 = new int[2]{A, A};
    int* const p4 = new int[2]{b, a};
    int* const q1 = new int[1]{b};
    int* const q2 = new int[1]{a};
    int* const h1 = new int[1]{a};
    int*** relations = new int**[3] {
        new int*[4] {p1, p2, p3, p4},
        new int*[2] {q1, q2},
        new int*[1] {h1}
    };
    int total_rows[3] {4, 2, 1};
    int arities[3] {2, 1, 1};
    std::string names[3] {"p", "q", "h"};
    SimpleKb kb("test", relations, names, arities, total_rows, 3);
    SimpleRelation& rel_p = *(kb.getRelation("p"));
    SimpleRelation& rel_q = *(kb.getRelation("q"));
    SimpleRelation& rel_h = *(kb.getRelation("h"));

    /* h(X) :- p(X, X), q(X) */
    CachedRule rule(rel_h.id, 1, cache, tabuMap, kb);
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase4(rel_p.id, 2, 0, 0, 0));
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase2(rel_q.id, 1, 0, 0));
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase1(1, 1, 0));
    EXPECT_STREQ("h(X0):-p(X0,X0),q(X0)", rule.toDumpString(kb.getRelationNames()).c_str());
    Eval eval(1, 1, 3);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(1, rule.usedLimitedVars());
    EXPECT_EQ(3, rule.getLength());
    groundingSetType expected_grounding_set;
    expected_grounding_set.emplace(std::vector<Record>{Record(h1, 1), Record(p2, 2), Record(q2, 1)});
    EvidenceBatch* actual_evidence = rule.getEvidenceAndMarkEntailment();
    int exp_relation_nums[3] {rel_h.id, rel_p.id, rel_q.id};
    int exp_arities[3] {1, 2, 1};
    groundingSetType* exp_groundings[1] {&expected_grounding_set};
    checkEvidence(*actual_evidence, exp_relation_nums, exp_arities, 3 , exp_groundings, 1);
    std::unordered_set<Record>* actual_counterexamples = rule.getCounterexamples();
    EXPECT_TRUE(actual_counterexamples->empty());
    releaseGroundingSets(exp_groundings, 1);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_evidence;
    delete actual_counterexamples;

    delete[] p1;
    delete[] p2;
    delete[] p3;
    delete[] p4;
    delete[] q1;
    delete[] q2;
    delete[] h1;
    delete[] relations[0];
    delete[] relations[1];
    delete[] relations[2];
    delete[] relations;
    releaseCacheAndTabuMap();
}

TEST_F(TestCachedRule, TestAnyRule3) {
    int const a = 1;
    int const b = 2;
    int const c = 3;
    int* const h1 = new int[2]{a, a};
    int* const h2 = new int[2]{b, b};
    int* const h3 = new int[2]{a, c};
    int*** relations = new int**[1] {
        new int*[3] {h1, h2, h3}
    };
    int total_rows[1] {3};
    int arities[1] {2};
    std::string names[1] {"h"};
    SimpleKb kb("test", relations, names, arities, total_rows, 1);
    SimpleRelation& rel_h = *(kb.getRelation("h"));

    /* h(X, X) :- */
    CachedRule rule(rel_h.id, 2, cache, tabuMap, kb);
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase3(0, 0, 0, 1));
    EXPECT_STREQ("h(X0,X0):-", rule.toDumpString(kb.getRelationNames()).c_str());
    Eval eval(2, 3, 1);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(1, rule.usedLimitedVars());
    EXPECT_EQ(1, rule.getLength());
    groundingSetType expected_grounding_set;
    expected_grounding_set.emplace(std::vector<Record>({Record(h1, 2)}));
    expected_grounding_set.emplace(std::vector<Record>({Record(h2, 2)}));
    std::unordered_set<Record> expected_counter_examples;
    expected_counter_examples.emplace(new int[2]{c, c}, 2);
    EvidenceBatch* actual_evidence = rule.getEvidenceAndMarkEntailment();
    int exp_relation_nums[1] {rel_h.id};
    int exp_arities[1] {2};
    groundingSetType* exp_groundings[1] {&expected_grounding_set};
    checkEvidence(*actual_evidence, exp_relation_nums, exp_arities, 1 , exp_groundings, 1);
    std::unordered_set<Record>* actual_counterexamples = rule.getCounterexamples();
    EXPECT_EQ(expected_counter_examples, *actual_counterexamples);
    releaseGroundingSets(exp_groundings, 1);
    releaseCounterexampleSet(expected_counter_examples);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_evidence;
    delete actual_counterexamples;

    EXPECT_TRUE(rel_h.isEntailed(h1));
    EXPECT_TRUE(rel_h.isEntailed(h2));
    EXPECT_FALSE(rel_h.isEntailed(h3));

    delete[] h1;
    delete[] h2;
    delete[] h3;
    delete[] relations[0];
    delete[] relations;
    releaseCacheAndTabuMap();
}

TEST_F(TestCachedRule, TestAnyRule4) {
    int const a = 1;
    int const b = 2;
    int* const h1 = new int[3]{a, a, b};
    int* const h2 = new int[3]{b, b, a};
    int* const h3 = new int[3]{a, b, b};
    int*** relations = new int**[1] {
        new int*[3] {h1, h2, h3}
    };
    int total_rows[1] {3};
    int arities[1] {3};
    std::string names[1] {"h"};
    SimpleKb kb("test", relations, names, arities, total_rows, 1);
    SimpleRelation& rel_h = *(kb.getRelation("h"));

    /* h(X, X, ?) :- */
    CachedRule rule(rel_h.id, 3, cache, tabuMap, kb);
    rule.updateCacheIndices();
    EXPECT_EQ(UpdateStatus::Normal, rule.specializeCase3(0, 0, 0, 1));
    EXPECT_STREQ("h(X0,X0,?):-", rule.toDumpString(kb.getRelationNames()).c_str());
    Eval eval(2, 4, 1);
    EXPECT_EQ(eval, rule.getEval());
    EXPECT_EQ(1, rule.usedLimitedVars());
    EXPECT_EQ(1, rule.getLength());
    groundingSetType expected_grounding_set;
    expected_grounding_set.emplace(std::vector<Record>({Record(h1, 3)}));
    expected_grounding_set.emplace(std::vector<Record>({Record(h2, 3)}));
    std::unordered_set<Record> expected_counter_examples;
    expected_counter_examples.emplace(new int[3]{a, a, a}, 3);
    expected_counter_examples.emplace(new int[3]{b, b, b}, 3);
    EvidenceBatch* actual_evidence = rule.getEvidenceAndMarkEntailment();
    int exp_relation_nums[1] {rel_h.id};
    int exp_arities[1] {3};
    groundingSetType* exp_groundings[1] {&expected_grounding_set};
    checkEvidence(*actual_evidence, exp_relation_nums, exp_arities, 1 , exp_groundings, 1);
    std::unordered_set<Record>* actual_counterexamples = rule.getCounterexamples();
    EXPECT_EQ(expected_counter_examples, *actual_counterexamples);
    releaseGroundingSets(exp_groundings, 1);
    releaseCounterexampleSet(expected_counter_examples);
    releaseCounterexampleSet(*actual_counterexamples);
    delete actual_evidence;
    delete actual_counterexamples;

    EXPECT_TRUE(rel_h.isEntailed(h1));
    EXPECT_TRUE(rel_h.isEntailed(h2));
    EXPECT_FALSE(rel_h.isEntailed(h3));

    delete[] h1;
    delete[] h2;
    delete[] h3;
    delete[] relations[0];
    delete[] relations;
    releaseCacheAndTabuMap();
}

class TestSincWithCache : public testing::Test {
protected:
    void checkInducedRules(
        std::unordered_set<std::string> const& actualRuleStrings, std::string* expectedRuleStrings, int const numCandidates,
        int const expectHits
    ) {
        int hit = 0;
        for (int i = 0; i < numCandidates; i++) {
            if (actualRuleStrings.find(expectedRuleStrings[i]) != actualRuleStrings.end()) {
                hit++;
            }
        }
        EXPECT_EQ(expectHits, hit);
    }
};

TEST_F(TestSincWithCache, TestCompression1) {
    /*
     * KB:
     * 
     * p:
     * 1, 2
     * 3, 4
     * ...
     * 109, 110
     * 
     * q:
     * 2, 1
     * 4, 3
     * ..
     * 100, 99
     */
    int* relation_p[55]{};
    int* relation_q[50]{};
    for (int i = 0; i < 50; i++) {
        int a1 = i * 2 + 1;
        int a2 = i * 2 + 2;
        relation_p[i] = new int[2] {a1, a2};
        relation_q[i] = new int[2] {a2, a1};
    }
    for (int i = 50; i < 55; i++) {
        relation_p[i] = new int[2] {i * 2 + 1, i * 2 + 2};
    }
    int** relations[2] {relation_p, relation_q};
    std::string rel_names[2] {"p", "q"};
    int arities[2] {2, 2};
    int total_rows[2] {55, 50};
    SimpleKb* kb = new SimpleKb("TestSincWithCache", relations, rel_names, arities, total_rows, 2);

    SincWithCache sinc(new SincConfig(
        "", "", MEM_DIR, "TestSincWithCacheComp", 1, false, 0, 5, EvalMetric::Value::CompressionCapacity, 0.05, 0.25, 1.0, 0,
        "", "", 0, true
    ), kb);
    sinc.run();
    SimpleCompressedKb& ckb = sinc.getCompressedKb();
    std::vector<Rule*>& hypothesis = ckb.getHypothesis();
    std::unordered_set<Record>& counterexample_p = ckb.getCounterexampleSet(0);
    std::unordered_set<Record>& counterexample_q = ckb.getCounterexampleSet(1);
    std::vector<int> const& supplementary_constants = ckb.getSupplementaryConstants();

    EXPECT_EQ(2, hypothesis.size());
    std::unordered_set<std::string> actual_rule_strs;
    actual_rule_strs.reserve(2);
    actual_rule_strs.emplace(hypothesis[0]->toDumpString(kb->getRelationNames()).c_str());
    actual_rule_strs.emplace(hypothesis[1]->toDumpString(kb->getRelationNames()).c_str());
    std::string rules_p[2]{"p(X0,X1):-q(X1,X0)", "p(X1,X0):-q(X0,X1)"};
    std::string rules_q[2]{"q(X0,X1):-p(X1,X0)", "q(X1,X0):-p(X0,X1)"};
    checkInducedRules(actual_rule_strs, rules_p, 2, 1);
    checkInducedRules(actual_rule_strs, rules_q, 2, 1);
    for (Rule* const& rule: hypothesis) {
        int head_pred_symbol = rule->getHead().getPredSymbol();
        Eval const& actual_eval = rule->getEval();
        switch (head_pred_symbol) {
            case 0: {   // p
                Eval eval(50, 50, 2);
                if (!(eval == actual_eval)) {
                    FAIL();
                }
                // EXPECT_EQ(eval, actual_eval);
                break;
            }
            case 1: {   // q
                Eval eval(50, 55, 2);
                EXPECT_EQ(eval, actual_eval);
                break;
            }
            default:
                FAIL() << "Unknown predicate symbol: " << head_pred_symbol;
        }
    }

    EXPECT_TRUE(counterexample_p.empty());
    EXPECT_EQ(5, counterexample_q.size());
    for (int i = 50; i < 55; i++) {
        int row[2] {i * 2 + 2, i * 2 + 1};
        Record r(row, 2);
        EXPECT_NE(counterexample_q.end(), counterexample_q.find(r));
    }

    EXPECT_TRUE(supplementary_constants.empty());

    for (int i = 0; i < 50; i++) {
        delete[] relation_p[i];
        delete[] relation_q[i];
    }
    for (int i = 50; i < 55; i++) {
        delete[] relation_p[i];
    }
}