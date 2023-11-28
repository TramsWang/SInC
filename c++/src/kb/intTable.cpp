#include "intTable.h"
#include <algorithm>
#include "../util/util.h"
#include <iostream>

using sinc::IntArrayComparator;

IntArrayComparator::IntArrayComparator(int const _length) : length(_length) {}

bool IntArrayComparator::operator()(int* const a, int* const b) const {
    for (int i = 0; i < length; i++) {
        if (a[i] < b[i]) {
            return true;
        } else if (a[i] > b[i]) {
            return false;
        }
    }
    return false;
}

using sinc::IntTable;
using sinc::MatchedSubTables;

MatchedSubTables::MatchedSubTables(): slices1(new IntTable::slicesType()), slices2(new IntTable::slicesType()) {}

MatchedSubTables::~MatchedSubTables() {
    IntTable::releaseSlices(slices1);
    IntTable::releaseSlices(slices2);
}

IntTable::IntTable(int** rows, int const _totalRows, int const _totalCols) : 
    totalRows(_totalRows), totalCols(_totalCols),
    sortedRowsByCols(new int**[_totalCols]), valuesByCols(new int*[_totalCols]), startOffsetsByCols(new int*[_totalCols]),
    valuesByColsLengths(new int[_totalCols]), comparator(IntArrayComparator(_totalCols))
{
    for (int col = totalCols - 1; col >= 0; col--) {
        /* Sort by values in the column */
        std::stable_sort(rows, rows + totalRows, [col](int* const& a, int* const& b) -> bool {return a[col] < b[col];});
        int** sorted_rows = new int*[totalRows];
        std::copy(rows, rows + totalRows, sorted_rows);
        std::vector<int> values;
        values.reserve(totalRows);
        std::vector<int> start_offset;
        start_offset.reserve(totalRows+1);

        /* Find the position of each value */
        int current_val = sorted_rows[0][col];
        values.push_back(current_val);
        start_offset.push_back(0);
        for (int i = 1; i < totalRows; i++) {
            if (current_val != sorted_rows[i][col]) {
                current_val = sorted_rows[i][col];
                values.push_back(current_val);
                start_offset.push_back(i);
            }
        }
        start_offset.push_back(totalRows);
        sortedRowsByCols[col] = sorted_rows;
        valuesByCols[col] = sinc::toArray(values);
        startOffsetsByCols[col] = sinc::toArray(start_offset);
        valuesByColsLengths[col] = values.size();
    }
}

IntTable::IntTable(int** const rows, int const totalRows, int const totalCols, bool releaseRowArray) :
    IntTable(rows, totalRows, totalCols)
{
    if (releaseRowArray) {
        delete[] rows;
    }
}

IntTable::~IntTable() {
    for (int col = 0; col < totalCols; col++) {
        delete[] sortedRowsByCols[col];
        delete[] valuesByCols[col];
        delete[] startOffsetsByCols[col];
    }
    delete[] sortedRowsByCols;
    delete[] valuesByCols;
    delete[] startOffsetsByCols;
    delete[] valuesByColsLengths;
}

bool IntTable::hasRow(int* const row) const {
    int** const sorted_rows = sortedRowsByCols[0];
    int idx = std::lower_bound(sorted_rows, sorted_rows + totalRows, row, comparator) - sorted_rows;
    if (totalRows == idx) {
        return false;
    }
    int* const found_row = sorted_rows[idx];
    for (int i = 0; i < totalCols; i++) {
        if (found_row[i] != row[i]) {
            return false;
        }
    }
    return true;
}

int IntTable::whereIs(int* const row) const {
    int** const sorted_rows = sortedRowsByCols[0];
    int idx = std::lower_bound(sorted_rows, sorted_rows + totalRows, row, comparator) - sorted_rows;
    if (totalRows == idx) {
        return -idx - 1;
    }
    int* const found_row = sorted_rows[idx];
    bool equals = true;
    for (int i = 0; i < totalCols; i++) {
        if (found_row[i] != row[i]) {
            equals = false;
            break;
        }
    }
    return equals ? idx : (-idx - 1);
}

int* IntTable::operator[](int i) const {
    return sortedRowsByCols[0][i];
}

IntTable::sliceType* IntTable::getSlice(int const col, int const val) const {
    int* const values_by_cols = valuesByCols[col];
    int const idx = std::lower_bound(values_by_cols, values_by_cols + valuesByColsLengths[col], val) - values_by_cols;
    if (idx >= valuesByColsLengths[col] || val != values_by_cols[idx]) {
        /* Not found. Return nullptr */
        return nullptr;
    }
    int* const start_offsets = startOffsetsByCols[col];
    int** const begin = sortedRowsByCols[col] + start_offsets[idx];
    int** const end = sortedRowsByCols[col] + start_offsets[idx+1];
    IntTable::sliceType* slice = new IntTable::sliceType(begin, end);
    return slice;
}

IntTable* IntTable::select(int const col, int const val) const {
    IntTable::sliceType* slice = getSlice(col, val);
    if (nullptr == slice) {
        return nullptr;
    }
    IntTable* table = new IntTable(slice->data(), slice->size(), totalCols);
    releaseSlice(slice);
    return table;
}

IntTable::slicesType* IntTable::splitSlices(int const col) const {
    int** const sorted_rows = sortedRowsByCols[col];
    int* const start_offsets = startOffsetsByCols[col];
    int const num_values = valuesByColsLengths[col];
    IntTable::slicesType* slices = new IntTable::slicesType();
    slices->reserve(num_values);
    for (int i = 0; i < num_values; i++) {
        int** const begin = sorted_rows + start_offsets[i];
        int** const end = sorted_rows + start_offsets[i+1];
        slices->push_back(new IntTable::sliceType(begin, end));
    }
    return slices;
}

MatchedSubTables* IntTable::matchSlices(const IntTable& tab1, int const col1, const IntTable& tab2, int const col2) {
    int** const sorted_rows1 = tab1.sortedRowsByCols[col1];
    int* const values1 = tab1.valuesByCols[col1];
    int* const start_offsets1 = tab1.startOffsetsByCols[col1];
    int const num_values1 = tab1.valuesByColsLengths[col1];
    int** const sorted_rows2 = tab2.sortedRowsByCols[col2];
    int* const values2 = tab2.valuesByCols[col2];
    int* const start_offsets2 = tab2.startOffsetsByCols[col2];
    int const num_values2 = tab2.valuesByColsLengths[col2];
    MatchedSubTables* result = new MatchedSubTables();

    int idx1 = 0;
    int idx2 = 0;
    while (idx1 < num_values1 && idx2 < num_values2) {
        int val1 = values1[idx1];
        int val2 = values2[idx2];
        if (val1 < val2) {
            idx1 = std::lower_bound(values1 + idx1 + 1, values1 + num_values1, val2) - values1;
        } else if (val1 > val2) {
            idx2 = std::lower_bound(values2 + idx2 + 1, values2 + num_values2, val1) - values2;
        } else {    // val1 == val2
            int** const begin1 = sorted_rows1 + start_offsets1[idx1];
            int** const end1 = sorted_rows1 + start_offsets1[++idx1];
            result->slices1->push_back(new IntTable::sliceType(begin1, end1));

            int** const begin2 = sorted_rows2 + start_offsets2[idx2];
            int** const end2 = sorted_rows2 + start_offsets2[++idx2];
            result->slices2->push_back(new IntTable::sliceType(begin2, end2));
        }
    }
    return result;
}

IntTable::slicesType** IntTable::matchSlices(IntTable** const tables, int* const cols, int const numTables) {
    IntTable::slicesType** slices_lists = new IntTable::slicesType*[numTables];
    int*** const sorted_rows_arr = new int**[numTables];
    int** const values_arr = new int*[numTables];
    int** const start_offsets_arr = new int*[numTables];
    int* const num_values_arr = new int[numTables];
    int* const idxs = new int[numTables]{0};
    for (int i = 0; i < numTables; i++) {
        slices_lists[i] = new IntTable::slicesType();
        IntTable* table = tables[i];
        int col = cols[i];
        sorted_rows_arr[i] = table->sortedRowsByCols[col];
        values_arr[i] = table->valuesByCols[col];
        start_offsets_arr[i] = table->startOffsetsByCols[col];
        num_values_arr[i] = table->valuesByColsLengths[col];
    }

    bool not_finished = true;
    while (not_finished) {
        /* Locate the maximum value */
        int max_val = values_arr[0][idxs[0]];
        int max_idx = 0;
        bool all_match = true;
        for (int i = 1; i < numTables; i++) {
            int val = values_arr[i][idxs[i]];
            all_match &= (val == max_val);
            if (val > max_val) {
                max_val = val;
                max_idx = i;
            }
        }

        /* Match */
        if (all_match) {
            for (int i = 0; i < numTables; i++) {
                int** const sorted_rows = sorted_rows_arr[i];
                int** const begin = sorted_rows + start_offsets_arr[i][idxs[i]];
                int** const end = sorted_rows + start_offsets_arr[i][++idxs[i]];
                slices_lists[i]->push_back(new IntTable::sliceType(begin, end));
                if (idxs[i] >= num_values_arr[i]) {
                    not_finished = false;
                }
            }
        } else {
            /* Update idxs */
            for (int i = 0; i < numTables; i++) {
                if (i == max_idx) {
                    continue;
                }
                int* const values = values_arr[i];
                idxs[i] = std::lower_bound(values + idxs[i], values + num_values_arr[i], max_val) - values;
                if (idxs[i] >= num_values_arr[i]) {
                    not_finished = false;
                    break;
                }
            }
        }
    }

    /* Release resources */
    delete[] sorted_rows_arr;
    delete[] values_arr;
    delete[] start_offsets_arr;
    delete[] num_values_arr;
    delete[] idxs;
    return slices_lists;
}

IntTable::slicesType* IntTable::matchSlices(int const col1, int const col2) const {
    IntTable::slicesType* slices = new IntTable::slicesType();
    int** const sorted_rows = sortedRowsByCols[col1];
    int* const values = valuesByCols[col1];
    int* const start_offsets = startOffsetsByCols[col1];
    int const num_values = valuesByColsLengths[col1];
    for (int i = 0; i < num_values; i++) {
        int const val = values[i];
        int** const end = sorted_rows + start_offsets[i + 1];
        IntTable::sliceType* slice = new IntTable::sliceType();
        for (int** row_itr = sorted_rows + start_offsets[i]; row_itr != end; row_itr++) {
            int* const row = *row_itr;
            if (val == row[col2]) {
                slice->push_back(row);
            }
        }
        if (slice->empty()) {
            delete slice;
        } else {
            slices->push_back(slice);
        }
    }
    return slices;
}


int* IntTable::valuesInColumn(int const col) const {
    return valuesByCols[col];
}

int IntTable::numValuesInColumn(int const col) const {
    return valuesByColsLengths[col];
}

int** IntTable::getAllRows() const {
    return sortedRowsByCols[0];
}

int IntTable::getTotalRows() const {
    return totalRows;
}

int IntTable::getTotalCols() const {
    return totalCols;
}

int IntTable::minValue(int const col) const {
    return valuesByCols[col][0];
}

int IntTable::maxValue(int const col) const {
    return valuesByCols[col][valuesByColsLengths[col]-1];
}

int IntTable::maxValue() const {
    int max_value = valuesByCols[0][0];
    for (int col = 0; col < totalCols; col++) {
        max_value = std::max(max_value, maxValue(col));
    }
    return max_value;
}

void IntTable::releaseSlice(sliceType* slicePointer) {
    delete slicePointer;
}

void IntTable::releaseSlices(slicesType* slicesPointer) {
    for (IntTable::sliceType* const& vp: *slicesPointer) {
        delete vp;
    }
    delete slicesPointer;
}


void IntTable::releaseSlicesArray(slicesType** slicesArray, int const length) {
    for (int i = 0; i < length; i++) {
        releaseSlices(slicesArray[i]);
    }
    delete[] slicesArray;
}

size_t IntTable::memoryCost() const {
    size_t size = sizeof(IntTable);
    size += (
        sizeof(int**) + sizeof(int*) * totalRows + sizeof(int) + // `sortedRowsByCols`
        sizeof(int) // `valuesByColsLength
    ) * totalCols; 
    for (int i = 0; i < totalCols; i++) {
        int length = valuesByColsLengths[i];
        size += (
            sizeof(int*) + sizeof(int) * length + sizeof(int) // `valuesByCols`
        ) * 2 + sizeof(int);    // `startOffsetsByCOls``
    }
    size += 4 * sizeof(int);    // memory allocation overhead of the four arrays in `IntTable`
    return size;
}

void IntTable::showRows() const {
    int** rows = sortedRowsByCols[0];
    std::cout << '{';
    for (int i = 0; i < totalRows; i++) {
        std::cout << '[';
        for (int j = 0; j < totalCols; j++) {
            std::cout << rows[i][j] << ',';
        }
        std::cout << ']';
        std::cout << std::endl;
    }
    std::cout << '}' << std::endl;
}
