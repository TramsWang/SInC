#include <gtest/gtest.h>
#include "../../src/impl/cachedSinc.h"
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
                    if (!tableEqual((*expected_entry)[i]->getIndices(), (*actual_entry)[i]->getIndices())) {
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
            CompliedBlock::create(rel_p->getAllRows(), rel_p->getTotalRows(), rel_p->getTotalCols(), rel_p, false, false)
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
            CompliedBlock::create(rel_p->getAllRows(), rel_p->getTotalRows(), rel_p->getTotalCols(), rel_p, false, false)
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
            CompliedBlock::create(rel_p->getAllRows(), rel_p->getTotalRows(), rel_p->getTotalCols(), rel_p, false, false)
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
