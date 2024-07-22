#include "../impl/sincWithCache.h"
#include <sys/resource.h>

#define NUM_ROWS 100
#define NUM_COLS 4
#define NUM_CONSTS 100
#define NUM_CBS 10000
#define NUM_OPRS 5000

using sinc::CompliedBlock;

int** genRecords(int rows, int cols, int constants) {
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

void measureOnlyCreation() {
    int** init_rows[NUM_CBS];
    for (int i = 0; i < NUM_CBS; i++) {
        init_rows[i] = genRecords(NUM_ROWS, NUM_COLS, NUM_CONSTS);
    }
    CompliedBlock* init_cbs[NUM_CBS];
    long mem_begin = getMaxRss();
    for (int i = 0; i < NUM_CBS; i++) {
        init_cbs[i] = CompliedBlock::create(init_rows[i], NUM_ROWS, NUM_COLS, false);
        init_cbs[i]->buildIndices();
    }
    long mem_finished = getMaxRss();
    long measured = CompliedBlock::totalCbMemoryCost() / 1024;
    std::cout << "Actual:   " << (mem_finished - mem_begin) << std::endl;
    std::cout << "Measured: " << measured << std::endl;
}

void measureOprGetSlices() {
    int** init_rows[NUM_CBS];
    for (int i = 0; i < NUM_CBS; i++) {
        init_rows[i] = genRecords(NUM_ROWS, NUM_COLS, NUM_CONSTS);
    }
    CompliedBlock* init_cbs[NUM_CBS];
    long mem_begin = getMaxRss();
    for (int i = 0; i < NUM_CBS; i++) {
        init_cbs[i] = CompliedBlock::create(init_rows[i], NUM_ROWS, NUM_COLS, false);
        init_cbs[i]->buildIndices();
    }
    for (int i = 0; i < NUM_CBS; i++) {
        for (int j = 0; j < NUM_COLS; j++) {
            for (int r = 0; r < 10; r++) {
                CompliedBlock::getSlice(*(init_cbs[i]), j, init_rows[i][r][j]);
            }
        }
    }
    long mem_finished = getMaxRss();
    long measured = CompliedBlock::totalCbMemoryCost() / 1024;
    std::cout << "Actual:   " << (mem_finished - mem_begin) << std::endl;
    std::cout << "Measured: " << measured << std::endl;
}

void measureOprSplitSlices() {
    int** init_rows[NUM_CBS];
    for (int i = 0; i < NUM_CBS; i++) {
        init_rows[i] = genRecords(NUM_ROWS, NUM_COLS, NUM_CONSTS);
    }
    CompliedBlock* init_cbs[NUM_CBS];
    long mem_begin = getMaxRss();
    for (int i = 0; i < NUM_CBS; i++) {
        init_cbs[i] = CompliedBlock::create(init_rows[i], NUM_ROWS, NUM_COLS, false);
        init_cbs[i]->buildIndices();
    }
    for (int i = 0; i < NUM_CBS; i++) {
        for (int j = 0; j < NUM_COLS; j++) {
            CompliedBlock::splitSlices(*(init_cbs[i]), j);
        }
    }
    long mem_finished = getMaxRss();
    long measured = CompliedBlock::totalCbMemoryCost() / 1024;
    std::cout << "Actual:   " << (mem_finished - mem_begin) << std::endl;
    std::cout << "Measured: " << measured << std::endl;
}

void measureOprMatchOneSlices() {
    int** init_rows[NUM_CBS];
    for (int i = 0; i < NUM_CBS; i++) {
        init_rows[i] = genRecords(NUM_ROWS, NUM_COLS, NUM_CONSTS);
    }
    CompliedBlock* init_cbs[NUM_CBS];
    long mem_begin = getMaxRss();
    for (int i = 0; i < NUM_CBS; i++) {
        init_cbs[i] = CompliedBlock::create(init_rows[i], NUM_ROWS, NUM_COLS, false);
        init_cbs[i]->buildIndices();
    }
    for (int i = 0; i < NUM_CBS; i++) {
        for (int c1 = 0; c1 < NUM_COLS; c1++) {
            for (int c2 = c1 + 1; c2 < NUM_COLS; c2++) {
                CompliedBlock::matchSlices(*(init_cbs[i]), c1, c2);
            }
        }
    }
    long mem_finished = getMaxRss();
    long measured = CompliedBlock::totalCbMemoryCost() / 1024;
    std::cout << "Actual:   " << (mem_finished - mem_begin) << std::endl;
    std::cout << "Measured: " << measured << std::endl;
}

void measureOprMatchTwoSlices() {
    int** init_rows[NUM_CBS];
    for (int i = 0; i < NUM_CBS; i++) {
        init_rows[i] = genRecords(NUM_ROWS, NUM_COLS, NUM_CONSTS);
    }
    CompliedBlock* init_cbs[NUM_CBS];
    long mem_begin = getMaxRss();
    for (int i = 0; i < NUM_CBS; i++) {
        init_cbs[i] = CompliedBlock::create(init_rows[i], NUM_ROWS, NUM_COLS, false);
        init_cbs[i]->buildIndices();
    }
    for (int i = 0; i < NUM_OPRS; i++) {
        int x = rand() % NUM_CBS;
        int cx = rand() % NUM_COLS;
        int y = rand() % NUM_CBS;
        int cy = rand() % NUM_COLS;
        CompliedBlock::matchSlices(*(init_cbs[x]), cx, *(init_cbs[y]), cy);
    }
    long mem_finished = getMaxRss();
    long measured = CompliedBlock::totalCbMemoryCost() / 1024;
    std::cout << "Actual:   " << (mem_finished - mem_begin) << std::endl;
    std::cout << "Measured: " << measured << std::endl;
}

int main(int argc, char const *argv[]) {
    measureOnlyCreation();
    // measureOprGetSlices();
    // measureOprSplitSlices();
    // measureOprMatchOneSlices();
    // measureOprMatchTwoSlices();
    return 0;
}
