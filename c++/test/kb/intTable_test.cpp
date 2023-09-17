#include <gtest/gtest.h>
#include "../../src/kb/intTable.h"

using namespace sinc;

TEST(TestIntArrayComparator, TestCompare) {
    IntArrayComparator cmp(3);
    int** arrays = new int*[4] {
        new int[3]{1, 2, 4},
        new int[3]{2, 5, 3},
        new int[3]{2, 5, 4},
        new int[3]{2, 9, 8}
    };

    for (int i = 0; i < 4; i++) {
        EXPECT_FALSE(cmp(arrays[i], arrays[i]));
        for (int j = i + 1; j < 4; j++) {
            EXPECT_TRUE(cmp(arrays[i], arrays[j]));
            EXPECT_FALSE(cmp(arrays[j], arrays[i]));
        }
    }

    delete[] arrays[0];
    delete[] arrays[1];
    delete[] arrays[2];
    delete[] arrays[3];
    delete[] arrays;
}

class IntTableTester : public IntTable {
public:
    IntTableTester(int** const rows, int const totalRows, int const totalCols) : IntTable(rows, totalRows, totalCols) {}
    using IntTable::sortedRowsByCols;
    using IntTable::valuesByCols;
    using IntTable::startOffsetsByCols;
    using IntTable::valuesByColsLengths;
    using IntTable::whereIs;
};

void expectArrayEquals(int* const a, int* const b, int const length) {
    for (int i = 0; i < length; i++) {
        EXPECT_EQ(a[i], b[i]);
    }
};

void expectRowsEquals(int** const rows1, int** rows2, int const totalRows, int const totalCols) {
    for (int i = 0; i < totalRows; i++) {
        for (int j = 0; j < totalCols; j++) {
            EXPECT_EQ(rows1[i][j], rows2[i][j]);
        }
    }
};

void expectTablesEquals(const IntTableTester& tab1, const IntTableTester& tab2) {
    EXPECT_EQ(tab1.getTotalRows(), tab2.getTotalRows());
    EXPECT_EQ(tab1.getTotalCols(), tab2.getTotalCols());
    expectRowsEquals(tab1.getAllRows(), tab2.getAllRows(), tab1.getTotalRows(), tab1.getTotalCols());
}

void expectTablesEquals(const IntTable& tab1, const IntTable& tab2) {
    EXPECT_EQ(tab1.getTotalRows(), tab2.getTotalRows());
    EXPECT_EQ(tab1.getTotalCols(), tab2.getTotalCols());
    expectRowsEquals(tab1.getAllRows(), tab2.getAllRows(), tab1.getTotalRows(), tab1.getTotalCols());
}

bool rowsEquals(int** const rows1, int** rows2, int const totalRows, int const totalCols) {
    for (int i = 0; i < totalRows; i++) {
        for (int j = 0; j < totalCols; j++) {
            if (rows1[i][j] != rows2[i][j]) {
                return false;
            }
        }
    }
    return true;
};

bool arrayEquals(int* const a, int* const b, int const length) {
    for (int i = 0; i < length; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
};

bool tablesEquals(const IntTableTester& tab1, const IntTableTester& tab2) {
    return tab1.getTotalRows() == tab2.getTotalRows() &&
        tab1.getTotalCols() == tab2.getTotalCols() &&
        rowsEquals(tab1.getAllRows(), tab2.getAllRows(), tab1.getTotalRows(), tab1.getTotalCols());
};

bool tablesEquals(const IntTable& tab1, const IntTable& tab2) {
    return tab1.getTotalRows() == tab2.getTotalRows() &&
        tab1.getTotalCols() == tab2.getTotalCols() &&
        rowsEquals(tab1.getAllRows(), tab2.getAllRows(), tab1.getTotalRows(), tab1.getTotalCols());
};

void releaseRows(int** rows, int length) {
    for (int i =0; i < length; i++) {
        delete[] rows[i];
    }
    delete[] rows;
};

TEST(TestIntTable, TestCreation) {
    int** const rows = new int*[5] {
        new int[3] {1, 5, 3},
        new int[3] {2, 4, 3},
        new int[3] {1, 2, 9},
        new int[3] {5, 3, 3},
        new int[3] {1, 5, 2},
    };
    int** const expected_sorted_rows = new int*[5] {
        rows[2], rows[4], rows[0], rows[1], rows[3]
    };
    int** const expected_values_by_cols = new int*[3] {
        new int[3] {1, 2, 5},
        new int[4] {2, 3, 4, 5},
        new int[3] {2, 3, 9}
    };
    int** const expected_start_offsets_by_cols = new int*[3] {
        new int[4] {0, 3, 4, 5},
        new int[5] {0, 1, 2, 3, 5},
        new int[4] {0, 1, 4, 5}
    };
    int* const expected_values_by_cols_lengths = new int[3] {3, 4, 3};

    IntTableTester table(rows, 5, 3);
    EXPECT_EQ(table.getTotalRows(), 5);
    EXPECT_EQ(table.getTotalCols(), 3);
    expectRowsEquals(table.getAllRows(), expected_sorted_rows, 5, 3);
    expectArrayEquals(table.valuesByCols[0], expected_values_by_cols[0], 3);
    expectArrayEquals(table.valuesByCols[1], expected_values_by_cols[1], 3);
    expectArrayEquals(table.valuesByCols[2], expected_values_by_cols[2], 3);
    expectArrayEquals(table.startOffsetsByCols[0], expected_start_offsets_by_cols[0], 3);
    expectArrayEquals(table.startOffsetsByCols[1], expected_start_offsets_by_cols[1], 3);
    expectArrayEquals(table.startOffsetsByCols[2], expected_start_offsets_by_cols[2], 3);
    expectArrayEquals(table.valuesByColsLengths, expected_values_by_cols_lengths, 3);
    
    releaseRows(rows, 5);
    delete[] expected_sorted_rows;
    releaseRows(expected_values_by_cols, 3);
    releaseRows(expected_start_offsets_by_cols, 3);
    delete[] expected_values_by_cols_lengths;
}

TEST(TestIntTable, TestExistenceQuery) {
    int** const rows = new int*[5] {
        new int[3] {1, 5, 3},
        new int[3] {2, 4, 3},
        new int[3] {1, 2, 9},
        new int[3] {5, 3, 3},
        new int[3] {1, 5, 2},
    };
    int** const rows_copy = new int*[5] {rows[0], rows[1], rows[2], rows[3], rows[4]};
    int* rows_order = new int[5] {2, 3, 0, 4, 1};
    int* rows_order_reverse = new int[5] {2, 4, 0, 1, 3};
    int** const non_existing_rows = new int*[5] {
        new int[3] {1, 2, 2},
        new int[3] {2, 2, 2},
        new int[3] {1, 1, 9},
        new int[3] {5, 5, 3},
        new int[3] {1, 5, 1},
    };
    int* const non_existing_rows_order = new int[5]{-1, -4, -1, -6, -2};

    IntTableTester table(rows, 5, 3);
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(table.hasRow(rows[i]));
        EXPECT_FALSE(table.hasRow(non_existing_rows[i]));
        expectArrayEquals(table[i], rows_copy[rows_order_reverse[i]], 3);
        EXPECT_EQ(table.whereIs(rows_copy[i]), rows_order[i]);
        EXPECT_EQ(table.whereIs(non_existing_rows[i]), non_existing_rows_order[i]);
    }

    releaseRows(rows, 5);
    delete[] rows_copy;
    delete[] rows_order;
    delete[] rows_order_reverse;
    releaseRows(non_existing_rows, 5);
    delete[] non_existing_rows_order;
}

TEST(TestIntTable, TestSlice) {
    int** const rows = new int*[5] {
        new int[3] {1, 5, 3},
        new int[3] {2, 4, 3},
        new int[3] {1, 2, 9},
        new int[3] {5, 3, 3},
        new int[3] {1, 5, 2},
    };
    IntTable table(rows, 5, 3);

    int** const exp_slice_rows_0_1 = new int*[3] {
        new int[3] {1, 2, 9},
        new int[3] {1, 5, 2},
        new int[3] {1, 5, 3}
    };
    std::vector<int*>* slice_0_1 = table.getSlice(0, 1);
    EXPECT_EQ(slice_0_1->size(), 3);
    for (int i = 0; i < 3; i++) {
        bool found = false;
        for (int j = 0; j < 3; j++) {
            if (arrayEquals(exp_slice_rows_0_1[i], (*slice_0_1)[j], 3)) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found);
    }

    releaseRows(rows, 5);
    releaseRows(exp_slice_rows_0_1, 3);
    delete slice_0_1;
}

TEST(TestIntTable, TestSelect) {
    int** const rows = new int*[5] {
        new int[3] {1, 5, 3},
        new int[3] {2, 4, 3},
        new int[3] {1, 2, 9},
        new int[3] {5, 3, 3},
        new int[3] {1, 5, 2},
    };
    IntTable table(rows, 5, 3);

    int** const exp_slice_rows_0_1 = new int*[3] {
        new int[3] {1, 2, 9},
        new int[3] {1, 5, 2},
        new int[3] {1, 5, 3}
    };
    IntTable exp_slice_0_1(exp_slice_rows_0_1, 3, 3);
    IntTable* slice_0_1 = table.select(0, 1);
    expectTablesEquals(*slice_0_1, exp_slice_0_1);
    int** const exp_slice_rows_0_2 = new int*[1] {
        new int[3] {2, 4, 3}
    };
    IntTable exp_slice_0_2(exp_slice_rows_0_2, 1, 3);
    IntTable* slice_0_2 = table.select(0, 2);
    expectTablesEquals(*slice_0_2, exp_slice_0_2);
    int** const exp_slice_rows_0_5 = new int*[1] {
        new int[3] {5, 3, 3}
    };
    IntTable exp_slice_0_5(exp_slice_rows_0_5, 1, 3);
    IntTable* slice_0_5 = table.select(0, 5);
    expectTablesEquals(*slice_0_5, exp_slice_0_5);
    releaseRows(exp_slice_rows_0_1, 3);
    releaseRows(exp_slice_rows_0_2, 1);
    releaseRows(exp_slice_rows_0_5, 1);
    delete slice_0_1;
    delete slice_0_2;
    delete slice_0_5;

    int** const exp_slice_rows_1_2 = new int*[1] {
        new int[3] {1, 2, 9}
    };
    IntTable exp_slice_1_2(exp_slice_rows_1_2, 1, 3);
    IntTable* slice_1_2 = table.select(1, 2);
    expectTablesEquals(*slice_1_2, exp_slice_1_2);
    int** const exp_slice_rows_1_3 = new int*[1] {
        new int[3] {5, 3, 3}
    };
    IntTable exp_slice_1_3(exp_slice_rows_1_3, 1, 3);
    IntTable* slice_1_3 = table.select(1, 3);
    expectTablesEquals(*slice_1_3, exp_slice_1_3);
    int** const exp_slice_rows_1_4 = new int*[1] {
        new int[3] {2, 4, 3}
    };
    IntTable exp_slice_1_4(exp_slice_rows_1_4, 1, 3);
    IntTable* slice_1_4 = table.select(1, 4);
    expectTablesEquals(*slice_1_4, exp_slice_1_4);
    int** const exp_slice_rows_1_5 = new int*[2] {
        new int[3] {1, 5, 2},
        new int[3] {1, 5, 3}
    };
    IntTable exp_slice_1_5(exp_slice_rows_1_5, 2, 3);
    IntTable* slice_1_5 = table.select(1, 5);
    expectTablesEquals(*slice_1_5, exp_slice_1_5);
    releaseRows(exp_slice_rows_1_2, 1);
    releaseRows(exp_slice_rows_1_3, 1);
    releaseRows(exp_slice_rows_1_4, 1);
    releaseRows(exp_slice_rows_1_5, 2);
    delete slice_1_2;
    delete slice_1_3;
    delete slice_1_4;
    delete slice_1_5;

    int** const exp_slice_rows_2_2 = new int*[1] {
        new int[3] {1, 5, 2}
    };
    IntTable exp_slice_2_2(exp_slice_rows_2_2, 1, 3);
    IntTable* slice_2_2 = table.select(2, 2);
    expectTablesEquals(*slice_2_2, exp_slice_2_2);
    int** const exp_slice_rows_2_3 = new int*[3] {
        new int[3] {1, 5, 3},
        new int[3] {2, 4, 3},
        new int[3] {5, 3, 3}
    };
    IntTable exp_slice_2_3(exp_slice_rows_2_3, 3, 3);
    IntTable* slice_2_3 = table.select(2, 3);
    expectTablesEquals(*slice_2_3, exp_slice_2_3);
    int** const exp_slice_rows_2_9 = new int*[1] {
        new int[3] {1, 2, 9}
    };
    IntTable exp_slice_2_9(exp_slice_rows_2_9, 1, 3);
    IntTable* slice_2_9 = table.select(2, 9);
    expectTablesEquals(*slice_2_9, exp_slice_2_9);
    releaseRows(exp_slice_rows_2_2, 1);
    releaseRows(exp_slice_rows_2_3, 3);
    releaseRows(exp_slice_rows_2_9, 1);
    delete slice_2_2;
    delete slice_2_3;
    delete slice_2_9;

    EXPECT_EQ(table.select(0, 8), nullptr);
    EXPECT_EQ(table.select(0, 3), nullptr);
    EXPECT_EQ(table.select(0, 4), nullptr);
    EXPECT_EQ(table.select(1, 1), nullptr);
    EXPECT_EQ(table.select(2, 5), nullptr);
    EXPECT_EQ(table.select(2, 6), nullptr);

    releaseRows(rows, 5);
}

void expectSliceEqual(std::vector<int*>* const slice1, std::vector<int*>* const slice2, int const cols) {
    const IntTable::sliceType& s1 = *slice1;
    const IntTable::sliceType& s2 = *slice2;
    EXPECT_EQ(slice1->size(), slice2->size());
    for (int i = 0; i < slice1->size(); i++) {
        bool found = false;
        for (int j = 0; j < slice2->size(); j++) {
            if (arrayEquals((*slice1)[i], (*slice2)[j], cols)) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "@(" << i << ')';
    }
};

void expectSlicesEqual(std::vector<std::vector<int*>*>* const slices1, std::vector<std::vector<int*>*>* const slices2, int const cols) {
    // for (int i = 0; i < slices1->size(); i++) {
    //     std::cout << 
    // }
    EXPECT_EQ(slices1->size(), slices2->size());
    for (int i = 0; i < slices1->size(); i++) {
        expectSliceEqual((*slices1)[i], (*slices2)[i], cols);
    }
};

TEST(TestIntTable, TestSplitSlices) {
    int** const rows = new int*[5] {
        new int[3] {1, 5, 3},
        new int[3] {2, 4, 3},
        new int[3] {1, 2, 9},
        new int[3] {5, 3, 3},
        new int[3] {1, 5, 2},
    };
    typename IntTable::slicesType* exp_slice_0 = new IntTable::slicesType();
    exp_slice_0->push_back(new IntTable::sliceType({rows[0], rows[2], rows[4]}));
    exp_slice_0->push_back(new IntTable::sliceType({rows[1]}));
    exp_slice_0->push_back(new IntTable::sliceType({rows[3]}));
    typename IntTable::slicesType* exp_slice_1 = new IntTable::slicesType();
    exp_slice_1->push_back(new IntTable::sliceType({rows[2]}));
    exp_slice_1->push_back(new IntTable::sliceType({rows[3]}));
    exp_slice_1->push_back(new IntTable::sliceType({rows[1]}));
    exp_slice_1->push_back(new IntTable::sliceType({rows[0], rows[4]}));
    typename IntTable::slicesType* exp_slice_2 = new IntTable::slicesType();
    exp_slice_2->push_back(new IntTable::sliceType({rows[4]}));
    exp_slice_2->push_back(new IntTable::sliceType({rows[0], rows[1], rows[3]}));
    exp_slice_2->push_back(new IntTable::sliceType({rows[2]}));

    IntTable table(rows, 5, 3);
    IntTable::slicesType* slice_0 = table.splitSlices(0);
    IntTable::slicesType* slice_1 = table.splitSlices(1);
    IntTable::slicesType* slice_2 = table.splitSlices(2);
    expectSlicesEqual(slice_0, exp_slice_0, 3);
    expectSlicesEqual(slice_1, exp_slice_1, 3);
    expectSlicesEqual(slice_2, exp_slice_2, 3);

    releaseRows(rows, 5);
    IntTable::releaseSlices(exp_slice_0);
    IntTable::releaseSlices(exp_slice_1);
    IntTable::releaseSlices(exp_slice_2);
    IntTable::releaseSlices(slice_0);
    IntTable::releaseSlices(slice_1);
    IntTable::releaseSlices(slice_2);
}

TEST(TestIntTable, TestMatch) {
    int** const rows1 = new int*[5] {
        new int[3] {1, 5, 3},
        new int[3] {2, 4, 3},
        new int[3] {1, 2, 9},
        new int[3] {5, 3, 3},
        new int[3] {1, 5, 2},
    };
    int** const rows2 = new int*[6] {
        new int[2] {1, 1},
        new int[2] {1, 2},
        new int[2] {1, 3},
        new int[2] {2, 2},
        new int[2] {2, 3},
        new int[2] {3, 3},
    };
    MatchedSubTables* exp_matched_result1 = new MatchedSubTables();
    exp_matched_result1->slices1->push_back(new IntTable::sliceType({rows1[0], rows1[2], rows1[4]}));
    exp_matched_result1->slices1->push_back(new IntTable::sliceType({rows1[1]}));
    exp_matched_result1->slices2->push_back(new IntTable::sliceType({rows2[0], rows2[1], rows2[2]}));
    exp_matched_result1->slices2->push_back(new IntTable::sliceType({rows2[3], rows2[4]}));
    MatchedSubTables* exp_matched_result2 = new MatchedSubTables();
    exp_matched_result2->slices1->push_back(new IntTable::sliceType({rows1[4]}));
    exp_matched_result2->slices1->push_back(new IntTable::sliceType({rows1[0], rows1[1], rows1[3]}));
    exp_matched_result2->slices2->push_back(new IntTable::sliceType({rows2[1], rows2[3]}));
    exp_matched_result2->slices2->push_back(new IntTable::sliceType({rows2[2], rows2[4], rows2[5]}));

    IntTable table1(rows1, 5, 3);
    IntTable table2(rows2, 6, 2);
    MatchedSubTables* matched_result1 = IntTable::matchSlices(table1, 0, table2, 0);
    MatchedSubTables* matched_result2 = IntTable::matchSlices(table1, 2, table2, 1);
    expectSlicesEqual(matched_result1->slices1, exp_matched_result1->slices1, 3);
    expectSlicesEqual(matched_result1->slices2, exp_matched_result1->slices2, 2);
    expectSlicesEqual(matched_result2->slices1, exp_matched_result2->slices1, 3);
    expectSlicesEqual(matched_result2->slices2, exp_matched_result2->slices2, 2);

    releaseRows(rows1, 5);
    releaseRows(rows2, 6);
    delete exp_matched_result1;
    delete exp_matched_result2;
    delete matched_result1;
    delete matched_result2;
}

TEST(TestIntTable, TestMatchMultiple) {
    int** const rows1 = new int*[5] {
        new int[3] {1, 5, 3},
        new int[3] {2, 4, 3},
        new int[3] {1, 2, 9},
        new int[3] {5, 3, 3},
        new int[3] {1, 5, 2},
    };
    int** const rows2 = new int*[5] {
        new int[2] {1, 2},
        new int[2] {2, 4},
        new int[2] {2, 5},
        new int[2] {3, 6},
        new int[2] {4, 8},
    };
    int** const rows3 = new int*[4] {
        new int[1] {1},
        new int[1] {2},
        new int[1] {4},
        new int[1] {5},
    };
    IntTable::slicesType* exp_slices1 = new IntTable::slicesType();
    exp_slices1->push_back(new IntTable::sliceType({rows1[2]}));
    exp_slices1->push_back(new IntTable::sliceType({rows1[1]}));
    IntTable::slicesType* exp_slices2 = new IntTable::slicesType();
    exp_slices2->push_back(new IntTable::sliceType({rows2[1], rows2[2]}));
    exp_slices2->push_back(new IntTable::sliceType({rows2[4]}));
    IntTable::slicesType* exp_slices3 = new IntTable::slicesType();
    exp_slices3->push_back(new IntTable::sliceType({rows3[1]}));
    exp_slices3->push_back(new IntTable::sliceType({rows3[2]}));

    IntTable table1(rows1, 5, 3);
    IntTable table2(rows2, 5, 2);
    IntTable table3(rows3, 4, 1);
    IntTable** tabs = new IntTable*[3]{&table1, &table2, &table3};
    int* cols = new int[3]{1, 0, 0};
    IntTable::slicesType** matched_reulsts = IntTable::matchSlices(tabs, cols, 3);
    expectSlicesEqual(matched_reulsts[0], exp_slices1, 3);
    expectSlicesEqual(matched_reulsts[1], exp_slices2, 2);
    expectSlicesEqual(matched_reulsts[2], exp_slices3, 1);

    releaseRows(rows1, 5);
    releaseRows(rows2, 5);
    releaseRows(rows3, 4);
    IntTable::releaseSlices(exp_slices1);
    IntTable::releaseSlices(exp_slices2);
    IntTable::releaseSlices(exp_slices3);
    delete[] tabs;
    delete[] cols;
    IntTable::releaseSlices(matched_reulsts[0]);
    IntTable::releaseSlices(matched_reulsts[1]);
    IntTable::releaseSlices(matched_reulsts[2]);
    delete[] matched_reulsts;
}

TEST(TestIntTable, TestMatchWithinASingleTable1) {
    int** const rows = new int*[5] {
        new int[3] {1, 5, 1},
        new int[3] {2, 4, 3},
        new int[3] {1, 2, 1},
        new int[3] {3, 3, 3},
        new int[3] {1, 5, 2},
    };
    IntTable::slicesType* exp_match_result = new IntTable::slicesType();
    exp_match_result->push_back(new IntTable::sliceType({rows[0], rows[2]}));
    exp_match_result->push_back(new IntTable::sliceType({rows[3]}));

    IntTable table(rows, 5, 3);
    IntTable::slicesType* match_result = table.matchSlices(0, 2);
    expectSlicesEqual(match_result, exp_match_result, 3);

    releaseRows(rows, 5);
    IntTable::releaseSlices(exp_match_result);
    IntTable::releaseSlices(match_result);
}

TEST(TestIntTable, TestMinMaxValues) {
    int** const rows = new int*[5] {
        new int[3] {1, 5, 3},
        new int[3] {2, 4, 3},
        new int[3] {1, 2, 9},
        new int[3] {5, 3, 3},
        new int[3] {1, 5, 2},
    };
    IntTable table(rows, 5, 3);
    EXPECT_EQ(table.minValue(0), 1);
    EXPECT_EQ(table.minValue(1), 2);
    EXPECT_EQ(table.minValue(2), 2);
    EXPECT_EQ(table.maxValue(0), 5);
    EXPECT_EQ(table.maxValue(1), 5);
    EXPECT_EQ(table.maxValue(2), 9);
    EXPECT_EQ(table.maxValue(), 9);
    releaseRows(rows, 5);
}