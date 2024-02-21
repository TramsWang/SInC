#include "../kb/intTable.h"
#include "../util/util.h"
#include "../impl/sincWithCache.h"
#include <cstdlib>
#include <sys/resource.h>

#define DUPLICATIONS 100
#define MAX_ROWS 10000
#define MAX_COLS 10
#define MAX_CONSTANTS 10000
#define IDX_CONSTRUCTION 0
#define IDX_GET 1
#define IDX_SPLIT 2
#define IDX_MATCH 3
#define IDX_JOIN 4
#define IDX_EXIST 5
#define NUM_ITEMS 6
#define FILE_TIME_FACTOR_ROWS "Time(Rows).txt"
#define FILE_TIME_FACTOR_COLS "Time(Cols).txt"
#define FILE_TIME_FACTOR_CONSTANTS "Time(Consts).txt"
#define FILE_MEM_FACTOR_ROWS "Mem(Rows).txt"
#define FILE_MEM_FACTOR_COLS "Mem(Cols).txt"
#define FILE_MEM_FACTOR_CONSTANTS "Mem(Consts).txt"

using sinc::currentTimeInNano;
using sinc::IntTable;
using sinc::CompliedBlock;

int** genRecords(int rows, int cols, int constants) {
    srand(currentTimeInNano());
    int** records = new int*[rows];
    for (int i = 0; i < rows; i++) {
        int* record = new int[cols];
        records[i] = record;
        for (int j = 0; j < cols; j++) {
            record[j] = rand() % constants;
        }
    }
    return records;
}

void releaseRecords(int** records, int rows) {
    for (int i = 0; i < rows; i++) {
        delete[] records[i];
    }
    delete[] records;
}

long getMaxRss() {
    rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
}

void testTimeFactorRows() {
    int const NUM_COLS = 5;
    std::ofstream ofile(FILE_TIME_FACTOR_ROWS, std::ios::out);
    // for (int num_rows: {10, 50, 100, 500, 1000, 5000, 10000}) {
    for (int num_rows = 100; num_rows < 20000; num_rows *= 2) {
        uint64_t time_it[NUM_ITEMS]{};
        uint64_t time_ht[NUM_ITEMS]{};
        for (int i = 0; i < DUPLICATIONS; i++) {
            int** const records1 = genRecords(num_rows, NUM_COLS, num_rows);
            int** const records2 = genRecords(num_rows, NUM_COLS, num_rows);
            int const get_col1 = 1;
            int const get_val1 = records1[1][1];
            int const get_col2 = 2;
            int const get_val2 = records2[2][2];

            /* Test Hash tables */
            uint64_t time_start = currentTimeInNano();
            CompliedBlock* cb1 = CompliedBlock::create(records1, num_rows, NUM_COLS, false);
            cb1->buildIndices();
            CompliedBlock* cb2 = CompliedBlock::create(records2, num_rows, NUM_COLS, false);
            cb2->buildIndices();
            uint64_t time_constructed = currentTimeInNano();

            CompliedBlock* cbr = CompliedBlock::getSlice(*cb1, get_col1, get_val1);
            cbr = CompliedBlock::getSlice(*cb2, get_col2, get_val2);
            uint64_t time_get_done = currentTimeInNano();

            for (int col_idx = 0; col_idx < NUM_COLS; col_idx++) {
                std::vector<sinc::CompliedBlock*>* split_result1 = CompliedBlock::splitSlices(*cb1, col_idx);
                std::vector<sinc::CompliedBlock*>* split_result2 = CompliedBlock::splitSlices(*cb2, col_idx);
                delete split_result1;
                delete split_result2;
            }
            uint64_t time_split_done = currentTimeInNano();

            for (int col_idx = 1; col_idx < NUM_COLS; col_idx++) {
                const std::vector<sinc::CompliedBlock*>* match_result1 = CompliedBlock::matchSlices(*cb1, 0, col_idx);
                const std::vector<sinc::CompliedBlock*>* match_result2 = CompliedBlock::matchSlices(*cb2, 0, col_idx);
                delete match_result1;
                delete match_result2;
            }
            uint64_t time_match_done = currentTimeInNano();

            for (int col_idx = 0; col_idx < NUM_COLS; col_idx++) {
                const sinc::MatchedSubCbs* join_result = CompliedBlock::matchSlices(*cb1, col_idx, *cb2, col_idx);
                delete join_result;
            }
            uint64_t time_join_done = currentTimeInNano();

            for (int row_idx = 0; row_idx < 100; row_idx++) {
                int* row = records1[row_idx];
                int _idx = cb1->hasRow(row);
            }
            uint64_t time_exist_done = currentTimeInNano();

            time_ht[IDX_CONSTRUCTION] += time_constructed - time_start;
            time_ht[IDX_GET] += time_get_done - time_constructed;
            time_ht[IDX_SPLIT] += time_split_done - time_get_done;
            time_ht[IDX_MATCH] += time_match_done - time_split_done;
            time_ht[IDX_JOIN] += time_join_done - time_match_done;
            time_ht[IDX_EXIST] += time_exist_done - time_join_done;

            /* Test IntTables */
            time_start = currentTimeInNano();
            IntTable it1(records1, num_rows, NUM_COLS);
            IntTable it2(records2, num_rows, NUM_COLS);
            time_constructed = currentTimeInNano();

            IntTable::sliceType* slice1 = it1.getSlice(get_col1, get_val1);
            IntTable::sliceType* slice2 = it2.getSlice(get_col2, get_val2);
            time_get_done = currentTimeInNano();

            for (int col_idx = 0; col_idx < NUM_COLS; col_idx++) {
                IntTable::slicesType* split_result1 = it1.splitSlices(col_idx);
                IntTable::slicesType* split_result2 = it2.splitSlices(col_idx);
                IntTable::releaseSlices(split_result1);
                IntTable::releaseSlices(split_result2);
            }
            time_split_done = currentTimeInNano();

            for (int col_idx = 1; col_idx < NUM_COLS; col_idx++) {
                IntTable::slicesType* match_result1 = it1.matchSlices(0, col_idx);
                IntTable::slicesType* match_result2 = it2.matchSlices(0, col_idx);
                IntTable::releaseSlices(match_result1);
                IntTable::releaseSlices(match_result2);
            }
            time_match_done = currentTimeInNano();

            for (int col_idx = 0; col_idx < NUM_COLS; col_idx++) {
                sinc::MatchedSubTables* join_result = IntTable::matchSlices(it1, col_idx, it2, col_idx);
                delete join_result;
            }
            time_join_done = currentTimeInNano();

            for (int row_idx = 0; row_idx < 100; row_idx++) {
                int* row = records1[row_idx];
                int _idx = it1.hasRow(row);
            }
            time_exist_done = currentTimeInNano();

            time_it[IDX_CONSTRUCTION] += time_constructed - time_start;
            time_it[IDX_GET] += time_get_done - time_constructed;
            time_it[IDX_SPLIT] += time_split_done - time_get_done;
            time_it[IDX_MATCH] += time_match_done - time_split_done;
            time_it[IDX_JOIN] += time_join_done - time_match_done;
            time_it[IDX_EXIST] += time_exist_done - time_join_done;

            /* Release resources */
            IntTable::releaseSlice(slice1);
            IntTable::releaseSlice(slice2);
            CompliedBlock::clearPool();
            releaseRecords(records1, num_rows);
            releaseRecords(records2, num_rows);
        }

        ofile << "Rows=" << num_rows << std::endl;
        for (int item_idx = 0; item_idx < NUM_ITEMS; item_idx++) {
            ofile << time_ht[item_idx] << ',';
        }
        ofile << std::endl;
        for (int item_idx = 0; item_idx < NUM_ITEMS; item_idx++) {
            ofile << time_it[item_idx] << ',';
        }
        ofile << std::endl;
    }
    ofile.close();
}

void testTimeFactorCols() {
    int const NUM_ROWS = 500;
    std::ofstream ofile(FILE_TIME_FACTOR_COLS, std::ios::out);
    for (int num_cols = 2; num_cols <= MAX_COLS; num_cols++) {
        uint64_t time_it[NUM_ITEMS]{};
        uint64_t time_ht[NUM_ITEMS]{};
        for (int i = 0; i < DUPLICATIONS; i++) {
            int** const records1 = genRecords(NUM_ROWS, num_cols, NUM_ROWS);
            int** const records2 = genRecords(NUM_ROWS, num_cols, NUM_ROWS);
            int const get_col1 = 0;
            int const get_val1 = records1[0][0];
            int const get_col2 = 1;
            int const get_val2 = records2[1][1];

            /* Test Hash tables */
            uint64_t time_start = currentTimeInNano();
            CompliedBlock* cb1 = CompliedBlock::create(records1, NUM_ROWS, num_cols, false);
            cb1->buildIndices();
            CompliedBlock* cb2 = CompliedBlock::create(records2, NUM_ROWS, num_cols, false);
            cb2->buildIndices();
            uint64_t time_constructed = currentTimeInNano();

            CompliedBlock* cbr = CompliedBlock::getSlice(*cb1, get_col1, get_val1);
            cbr = CompliedBlock::getSlice(*cb2, get_col2, get_val2);
            uint64_t time_get_done = currentTimeInNano();

            for (int col_idx = 0; col_idx < num_cols; col_idx++) {
                std::vector<sinc::CompliedBlock*>* split_result1 = CompliedBlock::splitSlices(*cb1, col_idx);
                std::vector<sinc::CompliedBlock*>* split_result2 = CompliedBlock::splitSlices(*cb2, col_idx);
                delete split_result1;
                delete split_result2;
            }
            uint64_t time_split_done = currentTimeInNano();

            for (int col_idx = 1; col_idx < num_cols; col_idx++) {
                const std::vector<sinc::CompliedBlock*>* match_result1 = CompliedBlock::matchSlices(*cb1, 0, col_idx);
                const std::vector<sinc::CompliedBlock*>* match_result2 = CompliedBlock::matchSlices(*cb2, 0, col_idx);
                delete match_result1;
                delete match_result2;
            }
            uint64_t time_match_done = currentTimeInNano();

            for (int col_idx = 0; col_idx < num_cols; col_idx++) {
                const sinc::MatchedSubCbs* join_result = CompliedBlock::matchSlices(*cb1, col_idx, *cb2, col_idx);
                delete join_result;
            }
            uint64_t time_join_done = currentTimeInNano();

            for (int row_idx = 0; row_idx < 100; row_idx++) {
                int* row = records1[row_idx];
                int _idx = cb1->hasRow(row);
            }
            uint64_t time_exist_done = currentTimeInNano();

            time_ht[IDX_CONSTRUCTION] += time_constructed - time_start;
            time_ht[IDX_GET] += time_get_done - time_constructed;
            time_ht[IDX_SPLIT] += time_split_done - time_get_done;
            time_ht[IDX_MATCH] += time_match_done - time_split_done;
            time_ht[IDX_JOIN] += time_join_done - time_match_done;
            time_ht[IDX_EXIST] += time_exist_done - time_join_done;

            /* Test IntTables */
            time_start = currentTimeInNano();
            IntTable it1(records1, NUM_ROWS, num_cols);
            IntTable it2(records2, NUM_ROWS, num_cols);
            time_constructed = currentTimeInNano();

            IntTable::sliceType* slice1 = it1.getSlice(get_col1, get_val1);
            IntTable::sliceType* slice2 = it2.getSlice(get_col2, get_val2);
            time_get_done = currentTimeInNano();

            for (int col_idx = 0; col_idx < num_cols; col_idx++) {
                IntTable::slicesType* split_result1 = it1.splitSlices(col_idx);
                IntTable::slicesType* split_result2 = it2.splitSlices(col_idx);
                IntTable::releaseSlices(split_result1);
                IntTable::releaseSlices(split_result2);
            }
            time_split_done = currentTimeInNano();

            for (int col_idx = 1; col_idx < num_cols; col_idx++) {
                IntTable::slicesType* match_result1 = it1.matchSlices(0, col_idx);
                IntTable::slicesType* match_result2 = it2.matchSlices(0, col_idx);
                IntTable::releaseSlices(match_result1);
                IntTable::releaseSlices(match_result2);
            }
            time_match_done = currentTimeInNano();

            for (int col_idx = 0; col_idx < num_cols; col_idx++) {
                sinc::MatchedSubTables* join_result = IntTable::matchSlices(it1, col_idx, it2, col_idx);
                delete join_result;
            }
            time_join_done = currentTimeInNano();

            for (int row_idx = 0; row_idx < 100; row_idx++) {
                int* row = records1[row_idx];
                int _idx = it1.hasRow(row);
            }
            time_exist_done = currentTimeInNano();

            time_it[IDX_CONSTRUCTION] += time_constructed - time_start;
            time_it[IDX_GET] += time_get_done - time_constructed;
            time_it[IDX_SPLIT] += time_split_done - time_get_done;
            time_it[IDX_MATCH] += time_match_done - time_split_done;
            time_it[IDX_JOIN] += time_join_done - time_match_done;
            time_it[IDX_EXIST] += time_exist_done - time_join_done;

            /* Release resources */
            IntTable::releaseSlice(slice1);
            IntTable::releaseSlice(slice2);
            CompliedBlock::clearPool();
            releaseRecords(records1, NUM_ROWS);
            releaseRecords(records2, NUM_ROWS);
        }

        ofile << "Cols=" << num_cols << std::endl;
        for (int item_idx = 0; item_idx < NUM_ITEMS; item_idx++) {
            ofile << time_ht[item_idx] << ',';
        }
        ofile << std::endl;
        for (int item_idx = 0; item_idx < NUM_ITEMS; item_idx++) {
            ofile << time_it[item_idx] << ',';
        }
        ofile << std::endl;
    }
    ofile.close();
}

void testTimeFactorConstants() {
    int const NUM_ROWS = 500;
    int const NUM_COLS = 5;
    std::ofstream ofile(FILE_TIME_FACTOR_CONSTANTS, std::ios::out);
    for (int num_constants = 200; num_constants < 20000; num_constants *= 2) {
        uint64_t time_it[NUM_ITEMS]{};
        uint64_t time_ht[NUM_ITEMS]{};
        for (int i = 0; i < DUPLICATIONS; i++) {
            int** const records1 = genRecords(NUM_ROWS, NUM_COLS, num_constants);
            int** const records2 = genRecords(NUM_ROWS, NUM_COLS, num_constants);
            int const get_col1 = 0;
            int const get_val1 = records1[0][0];
            int const get_col2 = 1;
            int const get_val2 = records2[1][1];

            /* Test Hash tables */
            uint64_t time_start = currentTimeInNano();
            CompliedBlock* cb1 = CompliedBlock::create(records1, NUM_ROWS, NUM_COLS, false);
            cb1->buildIndices();
            CompliedBlock* cb2 = CompliedBlock::create(records2, NUM_ROWS, NUM_COLS, false);
            cb2->buildIndices();
            uint64_t time_constructed = currentTimeInNano();

            CompliedBlock* cbr = CompliedBlock::getSlice(*cb1, get_col1, get_val1);
            cbr = CompliedBlock::getSlice(*cb2, get_col2, get_val2);
            uint64_t time_get_done = currentTimeInNano();

            for (int col_idx = 0; col_idx < NUM_COLS; col_idx++) {
                std::vector<sinc::CompliedBlock*>* split_result1 = CompliedBlock::splitSlices(*cb1, col_idx);
                std::vector<sinc::CompliedBlock*>* split_result2 = CompliedBlock::splitSlices(*cb2, col_idx);
                delete split_result1;
                delete split_result2;
            }
            uint64_t time_split_done = currentTimeInNano();

            for (int col_idx = 1; col_idx < NUM_COLS; col_idx++) {
                const std::vector<sinc::CompliedBlock*>* match_result1 = CompliedBlock::matchSlices(*cb1, 0, col_idx);
                const std::vector<sinc::CompliedBlock*>* match_result2 = CompliedBlock::matchSlices(*cb2, 0, col_idx);
                delete match_result1;
                delete match_result2;
            }
            uint64_t time_match_done = currentTimeInNano();

            for (int col_idx = 0; col_idx < NUM_COLS; col_idx++) {
                const sinc::MatchedSubCbs* join_result = CompliedBlock::matchSlices(*cb1, col_idx, *cb2, col_idx);
                delete join_result;
            }
            uint64_t time_join_done = currentTimeInNano();

            for (int row_idx = 0; row_idx < 100; row_idx++) {
                int* row = records1[row_idx];
                int _idx = cb1->hasRow(row);
            }
            uint64_t time_exist_done = currentTimeInNano();

            time_ht[IDX_CONSTRUCTION] += time_constructed - time_start;
            time_ht[IDX_GET] += time_get_done - time_constructed;
            time_ht[IDX_SPLIT] += time_split_done - time_get_done;
            time_ht[IDX_MATCH] += time_match_done - time_split_done;
            time_ht[IDX_JOIN] += time_join_done - time_match_done;
            time_ht[IDX_EXIST] += time_exist_done - time_join_done;

            /* Test IntTables */
            time_start = currentTimeInNano();
            IntTable it1(records1, NUM_ROWS, NUM_COLS);
            IntTable it2(records2, NUM_ROWS, NUM_COLS);
            time_constructed = currentTimeInNano();

            IntTable::sliceType* slice1 = it1.getSlice(get_col1, get_val1);
            IntTable::sliceType* slice2 = it2.getSlice(get_col2, get_val2);
            time_get_done = currentTimeInNano();

            for (int col_idx = 0; col_idx < NUM_COLS; col_idx++) {
                IntTable::slicesType* split_result1 = it1.splitSlices(col_idx);
                IntTable::slicesType* split_result2 = it2.splitSlices(col_idx);
                IntTable::releaseSlices(split_result1);
                IntTable::releaseSlices(split_result2);
            }
            time_split_done = currentTimeInNano();

            for (int col_idx = 1; col_idx < NUM_COLS; col_idx++) {
                IntTable::slicesType* match_result1 = it1.matchSlices(0, col_idx);
                IntTable::slicesType* match_result2 = it2.matchSlices(0, col_idx);
                IntTable::releaseSlices(match_result1);
                IntTable::releaseSlices(match_result2);
            }
            time_match_done = currentTimeInNano();

            for (int col_idx = 0; col_idx < NUM_COLS; col_idx++) {
                sinc::MatchedSubTables* join_result = IntTable::matchSlices(it1, col_idx, it2, col_idx);
                delete join_result;
            }
            time_join_done = currentTimeInNano();

            for (int row_idx = 0; row_idx < 100; row_idx++) {
                int* row = records1[row_idx];
                int _idx = it1.hasRow(row);
            }
            time_exist_done = currentTimeInNano();

            time_it[IDX_CONSTRUCTION] += time_constructed - time_start;
            time_it[IDX_GET] += time_get_done - time_constructed;
            time_it[IDX_SPLIT] += time_split_done - time_get_done;
            time_it[IDX_MATCH] += time_match_done - time_split_done;
            time_it[IDX_JOIN] += time_join_done - time_match_done;
            time_it[IDX_EXIST] += time_exist_done - time_join_done;

            /* Release resources */
            IntTable::releaseSlice(slice1);
            IntTable::releaseSlice(slice2);
            CompliedBlock::clearPool();
            releaseRecords(records1, NUM_ROWS);
            releaseRecords(records2, NUM_ROWS);
        }

        ofile << "Constants=" << num_constants << std::endl;
        for (int item_idx = 0; item_idx < NUM_ITEMS; item_idx++) {
            ofile << time_ht[item_idx] << ',';
        }
        ofile << std::endl;
        for (int item_idx = 0; item_idx < NUM_ITEMS; item_idx++) {
            ofile << time_it[item_idx] << ',';
        }
        ofile << std::endl;
    }
    ofile.close();
}

void testMemFactorRows() {
    int const NUM_COLS = 5;
    // int num_rows_arr[7]{10, 50, 100, 500, 1000, 5000, 10000};
    int num_rows_arr[8]{100, 200, 400, 800, 1600, 3200, 6400, 12800};
    std::vector<int**> records_ptrs;
    records_ptrs.reserve(DUPLICATIONS * 10 * 7);
    long mem_original[8]{};
    for (int i = 0; i < 8; i++) {
        int num_rows = num_rows_arr[i];
        long mem_start = getMaxRss();
        for (int i = 0; i < DUPLICATIONS * 10; i++) {
            records_ptrs.push_back(genRecords(num_rows, NUM_COLS, num_rows));
        }
        mem_original[i] = getMaxRss() - mem_start;
    }

    /* Test Hash tables */
    std::vector<int**>::iterator itr = records_ptrs.begin();
    long mem_ht[8]{};
    for (int i = 0; i < 8; i++) {
        int num_rows = num_rows_arr[i];
        long mem_start = getMaxRss();
        for (int j = 0; j < DUPLICATIONS * 10; j++) {
            int** const records = *itr;
            itr++;
            CompliedBlock* cb = CompliedBlock::create(records, num_rows, NUM_COLS, false);
            cb->buildIndices();
        }
        long mem_done = getMaxRss();
        mem_ht[i] = mem_done - mem_start;
    }

    /* Test IntTables */
    itr = records_ptrs.begin();
    long mem_it[8]{};
    std::vector<IntTable*> it_ptrs;
    it_ptrs.reserve(records_ptrs.size());
    for (int i = 0; i < 8; i++) {
        int num_rows = num_rows_arr[i];
        long mem_start = getMaxRss();
        for (int j = 0; j < DUPLICATIONS * 10; j++) {
            int** const records = *itr;
            itr++;
            it_ptrs.push_back(new IntTable(records, num_rows, NUM_COLS));
        }
        long mem_done = getMaxRss();
        mem_it[i] = mem_done - mem_start;
    }

    /* Output */
    std::ofstream ofile(FILE_MEM_FACTOR_ROWS, std::ios::out);
    ofile << "Original" << std::endl;
    for (long mem_cost: mem_original) {
        ofile << mem_cost << ',';
    }
    ofile << std::endl;
    ofile << "Hash Table" << std::endl;
    for (long mem_cost: mem_ht) {
        ofile << mem_cost << ',';
    }
    ofile << std::endl;
    ofile << "IntTable" << std::endl;
    for (long mem_cost: mem_it) {
        ofile << mem_cost << ',';
    }
    ofile << std::endl;
    ofile.close();

    /* Release Memory */
    itr = records_ptrs.begin();
    std::vector<IntTable*>::iterator itr_it = it_ptrs.begin();
    for (int i = 0; i < 8; i++) {
        int num_rows = num_rows_arr[i];
        for (int i = 0; i < DUPLICATIONS * 10; i++) {
            releaseRecords(*itr, num_rows);
            itr++;
            delete *itr_it;
            itr_it++;
        }
    }
    CompliedBlock::clearPool();
}

void testMemFactorCols() {
    int const NUM_ROWS = 500;
    std::vector<int**> records_ptrs;
    records_ptrs.reserve(DUPLICATIONS * 10 * 9);
    long mem_original[9]{};
    for (int num_cols = 2; num_cols <= MAX_COLS; num_cols++) {
        long mem_start = getMaxRss();
        for (int i = 0; i < DUPLICATIONS * 10; i++) {
            records_ptrs.push_back(genRecords(NUM_ROWS, num_cols, NUM_ROWS));
        }
        mem_original[num_cols - 2] = getMaxRss() - mem_start;
    }

    /* Test Hash tables */
    std::vector<int**>::iterator itr = records_ptrs.begin();
    long mem_ht[9]{};
    for (int num_cols = 2; num_cols <= MAX_COLS; num_cols++) {
        long mem_start = getMaxRss();
        for (int j = 0; j < DUPLICATIONS * 10; j++) {
            int** const records = *itr;
            itr++;
            CompliedBlock* cb = CompliedBlock::create(records, NUM_ROWS, num_cols, false);
            cb->buildIndices();
        }
        long mem_done = getMaxRss();
        mem_ht[num_cols - 2] = mem_done - mem_start;
    }

    /* Test IntTables */
    itr = records_ptrs.begin();
    long mem_it[9]{};
    std::vector<IntTable*> it_ptrs;
    it_ptrs.reserve(records_ptrs.size());
    for (int num_cols = 2; num_cols <= MAX_COLS; num_cols++) {
        long mem_start = getMaxRss();
        for (int j = 0; j < DUPLICATIONS * 10; j++) {
            int** const records = *itr;
            itr++;
            it_ptrs.push_back(new IntTable(records, NUM_ROWS, num_cols));
        }
        long mem_done = getMaxRss();
        mem_it[num_cols - 2] = mem_done - mem_start;
    }

    /* Output */
    std::ofstream ofile(FILE_MEM_FACTOR_COLS, std::ios::out);
    ofile << "Original" << std::endl;
    for (long mem_cost: mem_original) {
        ofile << mem_cost << ',';
    }
    ofile << std::endl;
    ofile << "Hash Table" << std::endl;
    for (long mem_cost: mem_ht) {
        ofile << mem_cost << ',';
    }
    ofile << std::endl;
    ofile << "IntTable" << std::endl;
    for (long mem_cost: mem_it) {
        ofile << mem_cost << ',';
    }
    ofile << std::endl;
    ofile.close();

    /* Release Memory */
    itr = records_ptrs.begin();
    std::vector<IntTable*>::iterator itr_it = it_ptrs.begin();
    for (int num_cols = 2; num_cols <= MAX_COLS; num_cols++) {
        for (int i = 0; i < DUPLICATIONS * 10; i++) {
            releaseRecords(*itr, NUM_ROWS);
            itr++;
            delete *itr_it;
            itr_it++;
        }
    }
    CompliedBlock::clearPool();
}

void testMemFactorConstants() {
    int const NUM_ROWS = 500;
    int const NUM_COLS = 5;
    std::vector<int**> records_ptrs;
    records_ptrs.reserve(DUPLICATIONS * 10 * 9);
    std::vector<long> mem_original;
    for (int num_constants = 200; num_constants < 20000; num_constants *= 2) {
        long mem_start = getMaxRss();
        for (int i = 0; i < DUPLICATIONS * 10; i++) {
            records_ptrs.push_back(genRecords(NUM_ROWS, NUM_COLS, num_constants));
        }
        mem_original.push_back(getMaxRss() - mem_start);
    }

    /* Test Hash tables */
    std::vector<int**>::iterator itr = records_ptrs.begin();
    std::vector<long> mem_ht;
    for (int num_constants = 200; num_constants < 20000; num_constants *= 2) {
        long mem_start = getMaxRss();
        for (int j = 0; j < DUPLICATIONS * 10; j++) {
            int** const records = *itr;
            itr++;
            CompliedBlock* cb = CompliedBlock::create(records, NUM_ROWS, NUM_COLS, false);
            cb->buildIndices();
        }
        long mem_done = getMaxRss();
        mem_ht.push_back(mem_done - mem_start);
    }

    /* Test IntTables */
    itr = records_ptrs.begin();
    std::vector<long> mem_it;
    std::vector<IntTable*> it_ptrs;
    it_ptrs.reserve(records_ptrs.size());
    for (int num_constants = 200; num_constants < 20000; num_constants *= 2) {
        long mem_start = getMaxRss();
        for (int j = 0; j < DUPLICATIONS * 10; j++) {
            int** const records = *itr;
            itr++;
            it_ptrs.push_back(new IntTable(records, NUM_ROWS, NUM_COLS));
        }
        long mem_done = getMaxRss();
        mem_it.push_back(mem_done - mem_start);
    }

    /* Output */
    std::ofstream ofile(FILE_MEM_FACTOR_CONSTANTS, std::ios::out);
    ofile << "Original" << std::endl;
    for (long mem_cost: mem_original) {
        ofile << mem_cost << ',';
    }
    ofile << std::endl;
    ofile << "Hash Table" << std::endl;
    for (long mem_cost: mem_ht) {
        ofile << mem_cost << ',';
    }
    ofile << std::endl;
    ofile << "IntTable" << std::endl;
    for (long mem_cost: mem_it) {
        ofile << mem_cost << ',';
    }
    ofile << std::endl;
    ofile.close();

    /* Release Memory */
    itr = records_ptrs.begin();
    std::vector<IntTable*>::iterator itr_it = it_ptrs.begin();
    for (int num_constants = 200; num_constants < 20000; num_constants *= 2) {
        for (int i = 0; i < DUPLICATIONS * 10; i++) {
            releaseRecords(*itr, NUM_ROWS);
            itr++;
            delete *itr_it;
            itr_it++;
        }
    }
    CompliedBlock::clearPool();
}

extern int num_cmps_in_int_table_join;
extern int num_find_in_hash_map_join;
extern int num_cmps_hit_in_int_table_join;
extern int num_find_hit_in_hash_map_join;
void testJoinEfficiency() {
    int const NUM_ROWS = 500;
    int const NUM_COLS = 5;
    int const NUM_CONSTANTS = 200;
    uint64_t time_ht = 0;
    uint64_t time_it = 0;
    num_cmps_in_int_table_join = 0;
    num_find_in_hash_map_join = 0;
    double avg_cmp_ht = 0;
    double avg_cmp_it = 0;
    for (int i = 0; i < DUPLICATIONS; i++) {
        int** const records1 = genRecords(NUM_ROWS, NUM_COLS, NUM_CONSTANTS);
        int** const records2 = genRecords(NUM_ROWS, NUM_COLS, NUM_CONSTANTS);
        int const get_col1 = 0;
        int const get_val1 = records1[0][0];
        int const get_col2 = 1;
        int const get_val2 = records2[1][1];

        /* Test Hash tables */
        uint64_t time_start = currentTimeInNano();
        CompliedBlock* cb1 = CompliedBlock::create(records1, NUM_ROWS, NUM_COLS, false);
        cb1->buildIndices();
        CompliedBlock* cb2 = CompliedBlock::create(records2, NUM_ROWS, NUM_COLS, false);
        cb2->buildIndices();
        uint64_t time_constructed = currentTimeInNano();

        for (int col_idx = 0; col_idx < NUM_COLS; col_idx++) {
            const sinc::MatchedSubCbs* join_result = CompliedBlock::matchSlices(*cb1, col_idx, *cb2, col_idx);
            delete join_result;
        }
        uint64_t time_join_done = currentTimeInNano();
        time_ht += time_join_done - time_constructed;

        /* Test IntTables */
        time_start = currentTimeInNano();
        IntTable it1(records1, NUM_ROWS, NUM_COLS);
        IntTable it2(records2, NUM_ROWS, NUM_COLS);
        time_constructed = currentTimeInNano();

        for (int col_idx = 0; col_idx < NUM_COLS; col_idx++) {
            // num_cmps_in_int_table_join = 0;
            sinc::MatchedSubTables* join_result = IntTable::matchSlices(it1, col_idx, it2, col_idx);
            // avg_cmp_it += ((double) num_cmps_in_int_table_join) / it1.numValuesInColumn(col_idx);
            delete join_result;
        }
        time_join_done = currentTimeInNano();
        time_it += time_join_done - time_constructed;

        /* Release resources */
        CompliedBlock::clearPool();
        releaseRecords(records1, NUM_ROWS);
        releaseRecords(records2, NUM_ROWS);
    }

    std::cout << "#Cmp. in HT: " << num_find_in_hash_map_join << std::endl;
    std::cout << "#Cmp. in IT: " << num_cmps_in_int_table_join << std::endl;
    std::cout << "#Hit. in HT: " << num_find_hit_in_hash_map_join << std::endl;
    std::cout << "#Hit. in IT: " << num_cmps_hit_in_int_table_join << std::endl;
    std::cout << "#Avg. Cmp in IT: " << avg_cmp_it / (NUM_COLS * DUPLICATIONS) << std::endl;
    std::cout << "Time (ns) HT: " << time_ht << std::endl;
    std::cout << "Time (ns) IT: " << time_it << std::endl;
}

int main(int argc, char const *argv[]) {
    if (1 == argc) {
        std::cout << "Please indicate test (0 ~ 5)" << std::endl;
        return 0;
    }
    switch (argv[1][0]) {
        case '0':
            testTimeFactorRows();
            break;
        case '1':
            testTimeFactorCols();
            break;
        case '2':
            testTimeFactorConstants();
            break;
        case '3':
            testMemFactorRows();
            break;
        case '4':
            testMemFactorCols();
            break;
        case '5':
            testMemFactorConstants();
            break;
        case '6':
        default:
            testJoinEfficiency();
            break;
    }
    return 0;
}
