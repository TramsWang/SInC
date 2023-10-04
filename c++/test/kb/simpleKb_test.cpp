#include <gtest/gtest.h>
#include "../../src/kb/simpleKb.h"
#include "../../src/util/util.h"
#include <vector>
#include "testKbUtils.h"

using namespace sinc;

#define TEST_DIR "/dev/shm/"
#define SIMPLE_RELATION_TEST_DIR "/dev/shm/SimpleRelation"

class TestSimpleRelation : public testing::Test {
protected:
    static void SetUpTestSuite() {
        std::cout << "Create Dir: " << SIMPLE_RELATION_TEST_DIR << std::endl;
        path dir(SIMPLE_RELATION_TEST_DIR);
        EXPECT_FALSE(!std::filesystem::exists(dir) && !std::filesystem::create_directories(dir));
    }

    static void TearDownTestSuite() {
        path dir(SIMPLE_RELATION_TEST_DIR);
        EXPECT_TRUE(std::filesystem::remove(dir));
        std::cout << "Remove Dir: " << SIMPLE_RELATION_TEST_DIR << std::endl;
    }

    void releaseRows(int** rows, int length) {
        for (int i =0; i < length; i++) {
            delete[] rows[i];
        }
        delete[] rows;
    };

};

TEST_F(TestSimpleRelation, TestLoadFile) {
    path relation_file_path = path(SIMPLE_RELATION_TEST_DIR) / path("TestLoadFile.rel");
    IntWriter writer(relation_file_path.c_str());
    writer.write(444);
    writer.write(555);
    writer.write(666);
    writer.write(777);
    writer.write(888);
    writer.write(999);
    writer.write(0xaaa);
    writer.write(0xbbb);
    writer.write(0xccc);
    writer.write(0xddd);
    writer.write(0xeee);
    writer.write(0xdeadbeef);
    writer.close();
    int exp_rows[3][4] {
        {444, 555, 666, 777},
        {888, 999, 0xaaa, 0xbbb},
        {0xccc, 0xddd, 0xeee, (int)0xdeadbeef}
    };

    int** const rows = SimpleRelation::loadFile(relation_file_path, 4, 3);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            EXPECT_EQ(rows[i][j], exp_rows[i][j]) << "@(" << i << ',' << j << ')';
        }
        delete[] rows[i];
    }
    delete[] rows;
    std::filesystem::remove(relation_file_path);
}

TEST_F(TestSimpleRelation, TestConstructByLoad) {
    path relation_file_path = path(SIMPLE_RELATION_TEST_DIR) / path("TestConstructByLoad.rel");
    IntWriter writer(relation_file_path.c_str());
    writer.write(4);
    writer.write(5);
    writer.write(6);
    writer.write(7);
    writer.write(8);
    writer.write(9);
    writer.write(0xa);
    writer.write(0xb);
    writer.write(0xc);
    writer.write(0xd);
    writer.write(0xe);
    writer.write(0xf);
    writer.close();

    SimpleRelation relation("family", 0, 3, 4, relation_file_path);
    EXPECT_EQ(relation.getTotalRows(), 4);
    EXPECT_EQ(relation.getTotalCols(), 3);
    int row1[3] {4, 5, 6};
    int row2[3] {7, 8, 9};
    int row3[3] {10, 11, 12};
    int row4[3] {13, 14, 15};
    EXPECT_TRUE(relation.hasRow(row1));
    EXPECT_TRUE(relation.hasRow(row2));
    EXPECT_TRUE(relation.hasRow(row3));
    EXPECT_TRUE(relation.hasRow(row4));
    std::filesystem::remove(relation_file_path);
}

TEST_F(TestSimpleRelation, TestConstructByAssign) {
    int** const exp_rows = new int*[3] {
        new int[4]{444, 555, 666, 777},
        new int[4]{888, 999, 0xaaa, 0xbbb},
        new int[4]{0xccc, 0xddd, 0xeee, (int)0xdeadbeef}
    };
    SimpleRelation relation("family", 0, (int**)exp_rows, 4, 3);

    EXPECT_TRUE(relation.hasRow(exp_rows[0]));
    EXPECT_TRUE(relation.hasRow(exp_rows[1]));
    EXPECT_TRUE(relation.hasRow(exp_rows[2]));

    releaseRows(exp_rows, 3);
}

TEST_F(TestSimpleRelation, TestSetEntailment) {
    int** records = new int*[55];
    for (int i = 0; i < 55; i++) {
        records[i] = new int[3]{i, i, i};
    }
    SimpleRelation relation("test", 0, records, 3, 55);

    int rec1[3]{0, 0, 0};
    int rec2[3]{1, 1, 1};
    int rec3[3]{31, 31, 31};
    int rec4[3]{47, 47, 47};
    int rec5[3]{1, 2, 3};
    int rec6[3]{3, 3, 3};
    relation.setAsEntailed(rec1);
    relation.setAsEntailed(rec2);
    relation.setAsEntailed(rec3);
    relation.setAsEntailed(rec4);
    relation.setAsEntailed(rec5);
    EXPECT_TRUE(relation.isEntailed(rec1));
    EXPECT_TRUE(relation.isEntailed(rec2));
    EXPECT_TRUE(relation.isEntailed(rec3));
    EXPECT_TRUE(relation.isEntailed(rec4));
    EXPECT_FALSE(relation.isEntailed(rec5));
    EXPECT_FALSE(relation.isEntailed(rec6));
    EXPECT_EQ(relation.totalEntailedRecords(), 4);

    int rec7[3]{4, 5, 6};
    int rec8[3]{5, 5, 5};
    relation.setAsNotEntailed(rec3);
    relation.setAsNotEntailed(rec4);
    relation.setAsNotEntailed(rec7);
    relation.setAsNotEntailed(rec8);
    EXPECT_TRUE(relation.isEntailed(rec1));
    EXPECT_TRUE(relation.isEntailed(rec2));
    EXPECT_FALSE(relation.isEntailed(rec3));
    EXPECT_FALSE(relation.isEntailed(rec4));
    EXPECT_FALSE(relation.isEntailed(rec5));
    EXPECT_FALSE(relation.isEntailed(rec6));
    EXPECT_EQ(relation.totalEntailedRecords(), 2);

    releaseRows(records, 55);
}

TEST_F(TestSimpleRelation, TestSetAllEntailment) {
    int** records = new int*[55];
    for (int i = 0; i < 55; i++) {
        records[i] = new int[3]{i, i, i};
    }
    SimpleRelation relation("test", 0, records, 3, 55);

    int** const rows = new int*[5] {
        new int[3]{0, 0, 0},
        new int[3]{1, 1, 1},
        new int[3]{31, 31, 31},
        new int[3]{47, 47, 47},
        new int[3]{1, 2, 3}
    };
    relation.setAllAsEntailed((int**)rows, 5);

    int rec1[3]{0, 0, 0};
    int rec2[3]{1, 1, 1};
    int rec3[3]{31, 31, 31};
    int rec4[3]{47, 47, 47};
    int rec5[3]{1, 2, 3};
    int rec6[3]{3, 3, 3};
    EXPECT_TRUE(relation.isEntailed(rec1));
    EXPECT_TRUE(relation.isEntailed(rec2));
    EXPECT_TRUE(relation.isEntailed(rec3));
    EXPECT_TRUE(relation.isEntailed(rec4));
    EXPECT_FALSE(relation.isEntailed(rec5));
    EXPECT_FALSE(relation.isEntailed(rec6));
    EXPECT_EQ(relation.totalEntailedRecords(), 4);
    
    releaseRows(records, 55);
    releaseRows(rows, 5);
}

TEST_F(TestSimpleRelation, TestEntailIfNot) {
    int** records = new int*[55];
    for (int i = 0; i < 55; i++) {
        records[i] = new int[3]{i, i, i};
    }
    SimpleRelation relation("test", 0, records, 3, 55);

    int rec1[3]{0, 0, 0};
    int rec2[3]{1, 1, 1};
    int rec3[3]{31, 31, 31};
    int rec4[3]{47, 47, 47};
    int rec5[3]{1, 2, 3};
    int rec6[3]{3, 3, 3};
    relation.setAsEntailed(rec2);
    EXPECT_TRUE(relation.entailIfNot(rec1));
    EXPECT_FALSE(relation.entailIfNot(rec1));
    EXPECT_FALSE(relation.entailIfNot(rec2));
    EXPECT_TRUE(relation.entailIfNot(rec3));
    EXPECT_TRUE(relation.entailIfNot(rec4));
    EXPECT_FALSE(relation.entailIfNot(rec5));

    EXPECT_TRUE(relation.isEntailed(rec1));
    EXPECT_TRUE(relation.isEntailed(rec2));
    EXPECT_TRUE(relation.isEntailed(rec3));
    EXPECT_TRUE(relation.isEntailed(rec4));
    EXPECT_FALSE(relation.isEntailed(rec5));
    EXPECT_FALSE(relation.isEntailed(rec6));
    EXPECT_EQ(relation.totalEntailedRecords(), 4);

    for (int i = 0; i < 55; i++) {
        delete[] records[i];
    }
    delete[] records;
}

TEST_F(TestSimpleRelation, TestPromisingConstants) {
    int** const rows = new int*[5] {
        new int[3]{1, 5, 3},
        new int[3]{2, 4, 3},
        new int[3]{1, 3, 9},
        new int[3]{5, 3, 3},
        new int[3]{1, 5, 2},
    };
    SimpleRelation relation("test", 0, (int**)rows, 3, 5);

    SimpleRelation::minConstantCoverage = 0.5;
    std::vector<int>** promising_constants = relation.getPromisingConstants();
    EXPECT_EQ(promising_constants[0]->size(), 1);
    EXPECT_EQ(promising_constants[1]->size(), 0);
    EXPECT_EQ(promising_constants[2]->size(), 1);
    EXPECT_EQ((*(promising_constants[0]))[0], 1);
    EXPECT_EQ((*(promising_constants[2]))[0], 3);
    SimpleRelation::releasePromisingConstants(promising_constants, 3);

    SimpleRelation::minConstantCoverage = 0.3;
    promising_constants = relation.getPromisingConstants();
    EXPECT_EQ(promising_constants[0]->size(), 1);
    EXPECT_EQ(promising_constants[1]->size(), 2);
    EXPECT_EQ(promising_constants[2]->size(), 1);
    EXPECT_EQ((*(promising_constants[0]))[0], 1);
    EXPECT_EQ((*(promising_constants[1]))[0], 3);
    EXPECT_EQ((*(promising_constants[1]))[1], 5);
    EXPECT_EQ((*(promising_constants[2]))[0], 3);
    SimpleRelation::releasePromisingConstants(promising_constants, 3);

    releaseRows(rows, 5);
}

TEST_F(TestSimpleRelation, TestDump) {
    int** const rows = new int*[5] {
        new int[3]{1, 2, 3},
        new int[3]{4, 5, 6},
        new int[3]{7, 8, 9},
        new int[3]{10, 11, 12},
        new int[3]{13, 14, 15}
    };
    SimpleRelation relation("test", 0, (int**)rows, 3, 5);

    int rec1[3]{1, 2, 3};
    int rec2[3]{4, 5, 6};
    int rec3[3]{7, 8, 9};
    int rec4[3]{10, 11, 12};
    int rec5[3]{13, 14, 15};
    relation.setAsEntailed(rec1);
    relation.setAsEntailed(rec2);
    relation.setAsEntailed(rec3);
    path dump_path = path(SIMPLE_RELATION_TEST_DIR) / path("TestDump.rel");
    relation.dump(dump_path);

    SimpleRelation relation2("family", 0, 3, 4, dump_path);
    EXPECT_EQ(relation.getTotalRows(), 5);
    EXPECT_EQ(relation.getTotalCols(), 3);
    EXPECT_TRUE(relation.hasRow(rec1));
    EXPECT_TRUE(relation.hasRow(rec2));
    EXPECT_TRUE(relation.hasRow(rec3));
    EXPECT_TRUE(relation.hasRow(rec4));
    EXPECT_TRUE(relation.hasRow(rec5));
    std::filesystem::remove(dump_path);

    releaseRows(rows, 5);
}

TEST_F(TestSimpleRelation, TestDumpNecessaryRecords) {
    int** const rows = new int*[5] {
        new int[3]{1, 2, 3},
        new int[3]{4, 5, 6},
        new int[3]{7, 8, 9},
        new int[3]{10, 11, 12},
        new int[3]{13, 14, 15}
    };
    SimpleRelation relation("test", 0, (int**)rows, 3, 5);

    int rec1[3]{1, 2, 3};
    int rec2[3]{4, 5, 6};
    int rec3[3]{7, 8, 9};
    int rec4[3]{10, 11, 12};
    int rec5[3]{13, 14, 15};
    relation.setAsEntailed(rec1);
    relation.setAsEntailed(rec2);
    relation.setAsEntailed(rec3);
    path dump_path = path(SIMPLE_RELATION_TEST_DIR) / path("TestDumpNecessary.rel");
    std::vector<int*> v;
    v.push_back(rec1);
    v.push_back(rec2);
    relation.dumpNecessaryRecords(dump_path, v);

    SimpleRelation relation2("family", 0, 3, 4, dump_path);
    EXPECT_EQ(relation2.getTotalRows(), 4);
    EXPECT_EQ(relation2.getTotalCols(), 3);
    EXPECT_TRUE(relation2.hasRow(rec1));
    EXPECT_TRUE(relation2.hasRow(rec2));
    EXPECT_FALSE(relation2.hasRow(rec3));
    EXPECT_TRUE(relation2.hasRow(rec4));
    EXPECT_TRUE(relation2.hasRow(rec5));
    std::filesystem::remove(dump_path);

    releaseRows(rows, 5);
}

TEST_F(TestSimpleRelation, TestSetFlagOfReservedConstants) {
    int** const rows = new int*[3] {
        new int[3]{1, 2, 3},
        new int[3]{4, 5, 6},
        new int[3]{7, 8, 9},
    };
    SimpleRelation relation("test", 0, (int**)rows, 3, 3);
    int rec1[3]{1, 2, 3};
    relation.setAsEntailed(rec1);
    int flags[]{0};
    relation.setFlagOfReservedConstants(flags);
    EXPECT_EQ(flags[0], 1008);

    releaseRows(rows, 3);
}

void expectVectorEqual(const std::vector<int*>& v1, const std::vector<int*>& v2, int const arity) {
    EXPECT_EQ(v1.size(), v2.size());
    for (int i = 0; i < v1.size(); i++) {
        int* const row_i = v1[i];
        bool found = false;
        for (int j = 0; j < v2.size(); j++) {
            int* const row_j = v2[j];
            found = true;
            for (int arg_idx = 0; arg_idx < arity; arg_idx++) {
                if (row_i[arg_idx] != row_j[arg_idx]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        EXPECT_TRUE(found) << "Not Found @" << i;
    }
}

TEST_F(TestSimpleRelation, TestSplitByEntailment) {
    int num_rows = BITS_PER_INT * 2 + BITS_PER_INT / 3;
    std::vector<int*> _expected_non_entailed_rows;
    std::vector<int*> _expected_entailed_rows;
    int** rows = new int*[num_rows];
    for (int i = 0; i < num_rows; i++) {
        int* record = new int[3]{i, i, i};
        if (0 == i % 3) {
            _expected_entailed_rows.push_back(record);
        } else {
            _expected_non_entailed_rows.push_back(record);
        }
        rows[i] = record;
    }

    SimpleRelation relation("test", 0, rows, 3, num_rows);
    for (int* const& record: _expected_entailed_rows) {
        relation.setAsEntailed(record);
    }
    SplitRecords* actual_split_records = relation.splitByEntailment();
    expectVectorEqual(*(actual_split_records->entailedRecords), _expected_entailed_rows, 3);
    expectVectorEqual(*(actual_split_records->nonEntailedRecords), _expected_non_entailed_rows, 3);

    for (int i = 0; i < num_rows; i++) {
        delete[] rows[i];
    }
    delete[] rows;
    delete actual_split_records;
}

using sinc::test::TestKbManager;
class TestSimpleKb : public testing::Test {
protected:
    static TestKbManager* testKb;

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

    void releaseRows(int** rows, int length) {
        for (int i =0; i < length; i++) {
            delete[] rows[i];
        }
        delete[] rows;
    };

};

TestKbManager* TestSimpleKb::testKb = nullptr;

TEST_F(TestSimpleKb, TestGetPathMethods) {
    path base_path("/base");
    path kb_dir_path("/base/test_kb");
    path rel_info_file_path("/base/test_kb/Relations.tsv");
    path rel_data_file_path("/base/test_kb/1.rel");
    path map_file_path("/base/test_kb/map10.tsv");

    EXPECT_EQ(SimpleKb::getKbDirPath("test_kb", base_path), kb_dir_path);
    EXPECT_EQ(SimpleKb::getRelInfoFilePath("test_kb", base_path), rel_info_file_path);
    EXPECT_EQ(SimpleKb::getRelDataFilePath(1, "test_kb", base_path), rel_data_file_path);
    EXPECT_EQ(SimpleKb::getMapFilePath(kb_dir_path, 10), map_file_path);
}

TEST_F(TestSimpleKb, TestConstructByLoad) {
    SimpleKb kb(testKb->getKbName(), TestKbManager::MEM_DIR_PATH);
    EXPECT_STREQ(kb.getName(), testKb->getKbName());
    EXPECT_EQ(kb.totalRelations(), 3);
    EXPECT_EQ(kb.totalRecords(), 12);
    EXPECT_EQ(kb.totalConstants(), 17);

    int** const family_recs = new int*[4] {
        new int[3]{4, 5, 6},
        new int[3]{7, 8, 9},
        new int[3]{10, 11, 12},
        new int[3]{13, 14, 15}
    };
    int** const mother_recs = new int*[4] {
        new int[2]{4, 6},
        new int[2]{7, 9},
        new int[2]{10, 12},
        new int[2]{13, 15}
    };
    int** const father_recs = new int*[4] {
        new int[2]{5, 6},
        new int[2]{8, 9},
        new int[2]{11, 12},
        new int[2]{16, 17}
    };
    std::string names[3] {"family", "mother", "father"};
    int** const recs[3] {family_recs, mother_recs, father_recs};
    for (int rel_idx = 0; rel_idx < 3; rel_idx++) {
        for (int rec_idx = 0; rec_idx < 4; rec_idx++) {
            EXPECT_TRUE(kb.hasRecord(names[rel_idx], recs[rel_idx][rec_idx])) << "@(" << rel_idx << ',' << rec_idx << ')';
        }
    }

    releaseRows(family_recs, 4);
    releaseRows(mother_recs, 4);
    releaseRows(father_recs, 4);
}

TEST_F(TestSimpleKb, TestConstructByAssign) {
    int** const family_recs = new int*[4] {
        new int[3]{4, 5, 6},
        new int[3]{7, 8, 9},
        new int[3]{10, 11, 12},
        new int[3]{13, 14, 15}
    };
    int** const mother_recs = new int*[4] {
        new int[2]{4, 6},
        new int[2]{7, 9},
        new int[2]{10, 12},
        new int[2]{13, 15}
    };
    int** const father_recs = new int*[4] {
        new int[2]{5, 6},
        new int[2]{8, 9},
        new int[2]{11, 12},
        new int[2]{16, 17}
    };
    std::string names[3] {"family", "mother", "father"};
    int** const recs[3] {family_recs, mother_recs, father_recs};
    int arities[3] {3, 2, 2};
    int total_rows[3] {4, 4, 4};
    SimpleKb kb("test", (int***)recs, names, arities, total_rows, 3);

    EXPECT_STREQ(kb.getName(), "test");
    EXPECT_EQ(kb.totalRelations(), 3);
    EXPECT_EQ(kb.totalRecords(), 12);
    EXPECT_EQ(kb.totalConstants(), 17);

    for (int rel_idx = 0; rel_idx < 3; rel_idx++) {
        for (int rec_idx = 0; rec_idx < 4; rec_idx++) {
            EXPECT_TRUE(kb.hasRecord(names[rel_idx], recs[rel_idx][rec_idx])) << "@(" << rel_idx << ',' << rec_idx << ')';
        }
    }

    releaseRows(family_recs, 4);
    releaseRows(mother_recs, 4);
    releaseRows(father_recs, 4);
}

TEST_F(TestSimpleKb, TestDump) {
    int** const rel1 = new int*[2] {
        new int[6]{1, 2, 3, 4, 5, 6},
        new int[6]{7, 8, 9, 10, 11, 12},
    };
    int** const rel2 = new int*[4] {
        new int[1]{1},
        new int[1]{2},
        new int[1]{4},
        new int[1]{8}
    };
    std::string map_names[12] {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"};
    std::string names[2] {"rel62", "rel41"};
    int** const recs[2] {rel1, rel2};
    int arities[2] {6, 1};
    int total_rows[2] {2, 4};
    SimpleKb kb("testSimpleKbDump", (int***)recs, names, arities, total_rows, 2);
    kb.dump(TestKbManager::MEM_DIR_PATH, map_names);

    SimpleKb kb2("testSimpleKbDump", TestKbManager::MEM_DIR_PATH);
    EXPECT_STREQ(kb.getName(), "testSimpleKbDump");
    EXPECT_EQ(kb.totalRelations(), 2);
    EXPECT_EQ(kb.totalRecords(), 6);
    EXPECT_EQ(kb.totalConstants(), 12);

    for (int rec_idx = 0; rec_idx < 2; rec_idx++) {
        EXPECT_TRUE(kb.hasRecord(0, recs[0][rec_idx]));
    }
    for (int rec_idx = 0; rec_idx < 4; rec_idx++) {
        EXPECT_TRUE(kb.hasRecord(1, recs[1][rec_idx]));
    }

    releaseRows(rel1, 2);
    releaseRows(rel2, 4);
    EXPECT_NE(std::filesystem::remove_all(TestKbManager::MEM_DIR_PATH / path("testSimpleKbDump")), 0);
}

using sinc::SimpleCompressedKb;
class TestSimpleCompressedKb : public testing::Test {
protected:
    static TestKbManager* testKb;

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

    void releaseRows(int** rows, int length) {
        for (int i =0; i < length; i++) {
            delete[] rows[i];
        }
        delete[] rows;
    };
};

TestKbManager* TestSimpleCompressedKb::testKb = nullptr;

TEST_F(TestSimpleCompressedKb, TestConstructAndDump) {
    SimpleKb kb(testKb->getKbName(), testKb->MEM_DIR_PATH);
    int family_ent_recs[3][3] {
        {4, 5, 6},
        {7, 8, 9},
        {10, 11, 12},
    };
    int father_ent_recs[3][2] {
        {5, 6},
        {8, 9},
        {11, 12},
    };
    int mother_ent_recs[3][3] {
        {4, 6},
        {7, 9},
        {10, 12},
    };
    std::string rel_name_family("family");
    std::string rel_name_father("father");
    std::string rel_name_mother("mother");
    const char* names[3];
    kb.setAsEntailed(rel_name_family, family_ent_recs[0]);
    kb.setAsEntailed(rel_name_family, family_ent_recs[1]);
    kb.setAsEntailed(rel_name_family, family_ent_recs[2]);
    kb.setAsEntailed(rel_name_father, father_ent_recs[0]);
    kb.setAsEntailed(rel_name_father, father_ent_recs[1]);
    kb.setAsEntailed(rel_name_father, father_ent_recs[2]);
    kb.setAsEntailed(rel_name_mother, mother_ent_recs[0]);
    kb.setAsEntailed(rel_name_mother, mother_ent_recs[1]);
    kb.setAsEntailed(rel_name_mother, mother_ent_recs[2]);
    SimpleRelation* rel_family = kb.getRelation(rel_name_family);
    SimpleRelation* rel_father = kb.getRelation(rel_name_father);
    SimpleRelation* rel_mother = kb.getRelation(rel_name_mother);
    names[rel_family->id] = rel_family->name;
    names[rel_father->id] = rel_father->name;
    names[rel_mother->id] = rel_mother->name;

    const fs::path& ckb_path = testKb->createTmpDir();
    SimpleCompressedKb ckb(ckb_path.filename().string(), &kb);
    int fvs_record[3] {10, 11, 12};
    ckb.addFvsRecord(rel_family->id, fvs_record);

    Rule::fingerprintCacheType cache;
    Rule::tabuMapType tabuMap;
    BareRule* rule_father = new BareRule(rel_father->id, 2, cache, tabuMap);
    EXPECT_EQ(rule_father->specializeCase4(rel_family->id, 3, 1, 0, 0), UpdateStatus::Normal);
    EXPECT_EQ(rule_father->specializeCase3(1, 2, 0, 1), UpdateStatus::Normal);
    EXPECT_STREQ(rule_father->toDumpString(names).c_str(), "father(X0,X1):-family(?,X0,X1)");
    BareRule* rule_mother = new BareRule(rel_mother->id, 2, cache, tabuMap);
    EXPECT_EQ(rule_mother->specializeCase4(rel_family->id, 3, 0, 0, 0), UpdateStatus::Normal);
    EXPECT_EQ(rule_mother->specializeCase5(0, 1, 6), UpdateStatus::Normal);
    EXPECT_EQ(rule_mother->specializeCase5(1, 2, 6), UpdateStatus::Normal);
    EXPECT_STREQ(rule_mother->toDumpString(names).c_str(), "mother(X0,6):-family(X0,?,6)");
    std::vector<Rule*> rules;
    rules.push_back(rule_father);
    rules.push_back(rule_mother);
    ckb.addHypothesisRules(rules);

    int** counterexamples_mother = new int*[1] {
        new int[2] {5, 5}
    };
    int** counterexamples_father = new int*[2] {
        new int[2] {16, 17},
        new int[2] {14, 15}
    };
    std::unordered_set<Record> ceg_set_mother;
    std::unordered_set<Record> ceg_set_father;
    ceg_set_mother.emplace(counterexamples_mother[0], 2);
    ceg_set_father.emplace(counterexamples_father[0], 2);
    ceg_set_father.emplace(counterexamples_father[1], 2);
    ckb.addCounterexamples(rel_mother->id, ceg_set_mother);
    ckb.addCounterexamples(rel_father->id, ceg_set_father);
    ckb.updateSupplementaryConstants();

    EXPECT_STREQ(ckb.getName(), ckb_path.filename().c_str());
    EXPECT_EQ(ckb.totalNecessaryRecords(), 4);
    EXPECT_EQ(ckb.totalFvsRecords(), 1);
    EXPECT_EQ(ckb.totalCounterexamples(), 3);
    EXPECT_EQ(ckb.totalHypothesisSize(), 5);
    EXPECT_EQ(ckb.totalSupplementaryConstants(), 7);    // In this test, numbers 1, 2, and 3 should be taken into consideration
    int exp_supp_consts[7] {1, 2, 3, 4, 7, 8, 9};
    for (int i = 0; i < 7; i++) {
        EXPECT_EQ(ckb.getSupplementaryConstants()[i], exp_supp_consts[i]);
    }

    /* Dump */
    ckb.dump(testKb->MEM_DIR_PATH);
    SimpleKb kb2(ckb.getName(), testKb->MEM_DIR_PATH);
    EXPECT_EQ(kb2.totalRecords(), 4);
    int family_necessary_recs[2][3] {
        {10, 11, 12},
        {13, 14, 15}
    };
    int father_necessary_recs[1][2] {
        {16, 17}
    };
    int mother_necessary_recs[1][2] {
        {13, 15}
    };
    EXPECT_TRUE(kb2.hasRecord("family", family_necessary_recs[0]));
    EXPECT_TRUE(kb2.hasRecord("family", family_necessary_recs[1]));
    EXPECT_TRUE(kb2.hasRecord("father", father_necessary_recs[0]));
    EXPECT_TRUE(kb2.hasRecord("mother", mother_necessary_recs[0]));

    delete[] counterexamples_mother;
    delete[] counterexamples_father;
    for (const Fingerprint* const& fp: cache) {
        delete fp;
    }
    for (std::pair<sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabuMap) {
        delete kv.first;
        delete kv.second;
    }
}