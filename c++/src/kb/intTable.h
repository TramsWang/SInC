#pragma once

#include <vector>

namespace sinc {
    class MatchedSubTables;

    /**
     * This class is used for comparing int arrays with the same length.
     */
    class IntArrayComparator {
    public:
        /** The length restriction of the compared arrays */
        int const length;

        IntArrayComparator(int const length);

        /**
         * Compare two arrays "a" and "b".
         * 
         * @return true if a < b
         * 
         * NOTE: Both of the two array should have the length "length".
         */
        bool operator()(int* const a, int* const b) const;
    };

    /**
     * This class is for indexing a large 2D table of integers. The table is sorted according to each column. That is,
     * sortedRowsByCols[i] stores the references of the rows sorted, in ascending order, by the ith argument of each row.
     * valuesByCols[i] will be a 1D array of the values occur in the ith arguments of the rows, no duplication, sorted in
     * ascending order. The first n element (n is the number of rows in the table) in the 1D array startIdxByCols[i] stores
     * the first offset of the row in sortedRowsByCols[i] that the corresponding argument value occurs. That is, if
     * startIdxByCols[i][j]=d, startIdxByCols[i][j+1]=e, and valuesByCols[i][j]=v, that means for these rows:
     *   sortedRowsByCols[i][d-1]
     *   sortedRowsByCols[i][d]
     *   sortedRowsByCols[i][d+1]
     *   ...
     *   sortedRowsByCols[i][e-1]
     *   sortedRowsByCols[i][e]
     * the following holds:
     *   sortedRowsByCols[i][d-1][i]!=v
     *   sortedRowsByCols[i][d][i]=v
     *   sortedRowsByCols[i][d+1][i]=v
     *   ...
     *   sortedRowsByCols[i][e-1][i]=v
     *   sortedRowsByCols[i][e][i]!=v
     * We also append one more element, n, to startIdxByCols[i] indicating the end of the rows.
     *
     * Suppose the memory cost of all rows is M, the total space of this type of index will be no more than 3M. The weakness
     * of this data structure is the query time. The existence query time is about O(log n) if the values in the rows are
     * randomly distributed in at least one column. Therefore, we require that there are NO duplicated rows in the table.
     * 
     * NOTE: IntTables maintains only the internal arrays of the pointers to the input rows. They do NOT maintain the input rows.
     *
     * Todo: I think most of the algorithms here can benefit from "divide and conquer" routine. It may worth a try in the future
     *
     * @since 2.1
     */
    class IntTable {
    public:
        typedef std::vector<int*> sliceType;
        typedef std::vector<sliceType*> slicesType;

        /**
         * Creating a IntTable by an array of rows. There should NOT be any duplicated rows in the array, and all the rows
         * should be in the same length. The array should NOT be empty.
         * 
         * NOTE: The input rows SHOULD be maintained by USER.
         * 
         * NOTE: This method may reorder the elements in "rows"
         * 
         * @param rows An array of int arrays
         * @param totalRows The number of integers in "rows"
         * @param totalCols The number of integers in each array of "rows"
         */
        IntTable(int** const rows, int const totalRows, int const totalCols);

        virtual ~IntTable();

        /**
         * Check whether a row is in the table.
         * 
         * NOTE: The row SHOULD have the same length as the rows in the table.
         */
        bool hasRow(int* row) const;

        /**
         * Returns the i-th row in the table w.r.t. alphabetical order.
         */
        int* operator[](int i) const;

        /**
         * Get a slice of the table where for every row r in the slice, r[col]=val.
         * 
         * NOTE: The returned pointer SHOULD be maintained by USER.
         * 
         * @return A vector of rows that satisfies the requirement, or `nullptr` if non exists.
         */
        std::vector<int*>* getSlice(int const col, int const val) const;

        /**
         * Select and create a new IntTable. For every row r in the new table, r[col]=val.
         * 
         * NOTE: The returned pointer SHOULD be maintained by USER.
         * 
         * @return The new table, or `nullptr` if no such row in the original table.
         */
        IntTable* select(int const col, int const val) const;

        /**
         * Split the table according to values in a column. The values at the column are the same in each slice.
         * 
         * NOTE: The returned vector pointer AND the vector pointers in the vector in the vecotr SHOULD be maintained by USER.
         * 
         * @return Will NOT return `nullptr`
         */
        slicesType* splitSlices(int const col) const;

        /**
         * Match the values of two columns in two slices. For each of the matched value v, derive a pair of slices
         * sub_tab1 and sub_tab2, such that: 1) for each row r1 in sub_tab1, r1 is in tab1, r1[col1]=v, and there is no row
         * r1' in tab1 but not in sub_tab1 that r1'[col]=v; 2) for each row r2 in sub_tab2, r2 is in tab2, t2[col2]=v, and
         * there is no row r2' in tab2 but not in sub_tab2 that r2'[col]=v.
         * 
         * NOTE: The returned pointer SHOULD be maintained by USER.
         *
         * @return Two arrays of matched sub-tables. Each pair of sub-tables, subTables1[i] and subTables2[i], satisfies the
         * above restrictions. Will not return `nullptr`.
         */
        static MatchedSubTables* matchSlices(const IntTable& tab1, int const col1, const IntTable& tab2, int const col2);

        /**
         * Extend the binary "matchSlices()" to n tables.
         * 
         * NOTE: The returned pointer SHOULD be maintained by USER and should be freed by "delet[]" as it is an array. All
         * pointers in this array should ALSO be maintained by USER and should be freed by "delete".
         *
         * @param tables The n table pointers
         * @param cols   n column numbers, each of the corresponding table
         * @param numTables n
         * @return n pointers to vectors of slices. Slices in a vector is from the same table. Will not return `nullptr`.
         */
        static slicesType** matchSlices(IntTable** const tables, int* const cols, int const numTables);

        /**
         * Split the current table into slices, and in each slice, the arguments of the two columns are the same.
         * 
         * @return Will not return `nullptr`.
         */
        slicesType* matchSlices(int const col1, int const col2) const;

        /**
         * Get the list of different values in a certain column.
         * 
         * NOTE: The returned pointer shall NOT be freed and the array shall NOT be modified
         */
        int* valuesInColumn(int const col) const;

        /**
         * Return the number of different values in a column.
         */
        int numValuesInColumn(int const col) const;

        /**
         * Get all the rows in the table.
         * 
         * NOTE: The returned pointer shall NOT be freed and the array shall NOT be modified
         */
        int** getAllRows() const;

        int getTotalRows() const;

        int getTotalCols() const;

        int minValue(int const col) const;

        int maxValue(int const col) const;

        int maxValue() const;

        /**
         * This is a helper function that releases pointers in a slice.
         */
        static void releaseSlice(sliceType* slicePointer);

        /**
         * This is a helper function that releases pointers in slices.
         */
        static void releaseSlices(slicesType* slicesPointer);

        /**
         * This is a helper function that releases pointers in slices array.
         */
        static void releaseSlicesArray(slicesType** slicesArray, int const length);

        /**
         * Return the memory occupation of this object
         */
        virtual size_t memoryCost() const;

        void showRows() const;
        void showRows(int col) const;

    protected:
        /** Total rows in the table */
        int const totalRows;
        /** Total cols in the table */
        int const totalCols;
        /** Row references sorted by each column in ascending order. Note that the rows are sorted by each column from the
         *  last to the first, and the sorting algorithm is stable. Thus, the rows are sorted alphabetically if we look up
         *  at sortedRowsByCols[0].
         */
        int*** const sortedRowsByCols;
        /** The index values of each column */
        int** const valuesByCols;
        /** The starting offset of each index value */
        int** const startOffsetsByCols;
        /** The lengths of arrays in "valuesByCols" */
        int* const valuesByColsLengths;
        /** Comparator for rows in this table */
        IntArrayComparator const comparator;

        /**
         * If `releaseRowArray` is true, this constructor releases the array `rows` (but not the pointers in it) when finish.
         * 
         * NOTE: This should only be called where release of `rows` is not possible. E.g., in the load constructor of `SimpleRelation`.
         */
        IntTable(int** const rows, int const totalRows, int const totalCols, bool releaseRowArray);

        /**
         * Find the offset of the row in the table w.r.t. alphabetical order.
         * 
         * NOTE: The row SHOULD have the same length as the rows in the table.
         *
         * @return The offset of the row. If the row is not in the table, return a negative value i, such that (-i - 1) is
         * the index of the first element that is larger than it.
         */
        int whereIs(int* const row) const;
    };

    /**
     * This class is used for the return values of the "matchSlices" of the class "IntTable".
     * 
     * NOTE: The two vector pointers and vector pointers in the two vectors are maintained by the instances.
     * 
     * @since 2.1
     */
    class MatchedSubTables {
    public:
        IntTable::slicesType* const slices1;
        IntTable::slicesType* const slices2;

        MatchedSubTables();
        ~MatchedSubTables();
        void showSlices(int arity1, int arity2) const;
    };
}