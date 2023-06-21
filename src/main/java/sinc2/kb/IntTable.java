package sinc2.kb;

import sinc2.common.Record;
import sinc2.util.ArrayOperation;

import java.util.*;

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
 * Todo: I think most of the algorithms here can benefit from "divide and conquer" routine. It may worth a try in the future
 *
 * @since 2.1
 */
public class IntTable implements Iterable<int[]> {

    /**
     * This class is used for comparing int arrays with the same length.
     */
    static class IntArrayComparator implements Comparator<int[]> {
        @Override
        public int compare(int[] arr1, int[] arr2) {
            for (int i = 0; i < arr1.length; i++) {
                int diff = arr1[i] - arr2[i];
                if (0 != diff) {
                    return diff;
                }
            }
            return 0;
        }
    }

    /** The comparator of rows */
    protected static final IntArrayComparator rowComparator = new IntArrayComparator();

    /** Row references sorted by each column in ascending order. Note that the rows are sorted by each column from the
     *  last to the first, and the sorting algorithm is stable. Thus, the rows are sorted alphabetically if we look up
     *  at sortedRowsByCols[0].
     */
    protected final int[][][] sortedRowsByCols;
    /** The index values of each column */
    protected final int[][] valuesByCols;
    /** The starting offset of each index value */
    protected final int[][] startOffsetsByCols;
    /** Total rows in the table */
    protected final int totalRows;
    /** Total cols in the table */
    protected final int totalCols;

    /**
     * Creating a IntTable by an array of rows. There should NOT be any duplicated rows in the array, and all the rows
     * should be in the same length. The array should NOT be empty.
     */
    public IntTable(int[][] rows) {
        totalCols = rows[0].length;
        totalRows = rows.length;
        sortedRowsByCols = new int[totalCols][][];
        valuesByCols = new int[totalCols][];
        startOffsetsByCols = new int[totalCols][];
        for (int col = totalCols - 1; col >= 0; col--) {
            /* Sort by values in the column */
            final int _col = col;
            Arrays.sort(rows, Comparator.comparingInt(e -> e[_col]));
            int[][] sorted_rows = rows.clone();
            List<Integer> values = new ArrayList<>(totalRows);
            List<Integer> start_offset = new ArrayList<>(totalRows);

            /* Find the position of each value */
            int current_val = sorted_rows[0][col];
            values.add(current_val);
            start_offset.add(0);
            for (int i = 1; i < totalRows; i++) {
                if (current_val != sorted_rows[i][col]) {
                    current_val = sorted_rows[i][col];
                    values.add(current_val);
                    start_offset.add(i);
                }
            }
            start_offset.add(totalRows);
            sortedRowsByCols[col] = sorted_rows;
            valuesByCols[col] = ArrayOperation.toArray(values);
            startOffsetsByCols[col] = ArrayOperation.toArray(start_offset);
        }
    }

    /**
     * Check whether a row is in the table.
     */
    public boolean hasRow(int[] row) {
        return 0 <= whereIs(row);
    }

    /**
     * Find the offset, w.r.t. the sorted column used for queries, of the row in the table.
     *
     * @return The offset of the row. If the row is not in the table, return a negative value i, such that (-i - 1) is
     * the index of the first element that is larger than it.
     */
    protected int whereIs(int[] row) {
        return Arrays.binarySearch(sortedRowsByCols[0], row, rowComparator);
    }

    @Override
    public Iterator<int[]> iterator() {
        return Arrays.stream(sortedRowsByCols[0]).iterator();
    }

    /**
     * Get a slice of the table where for every row r in the slice, r[col]=val.
     */
    public int[][] getSlice(int col, int val) {
        int idx = Arrays.binarySearch(valuesByCols[col], val);
        if (0 > idx) {
            return new int[0][];
        }
        final int[] start_offsets = startOffsetsByCols[col];
        int start_offset = start_offsets[idx];
        int length = start_offsets[idx + 1] - start_offset;
        int[][] slice = new int[length][];
        System.arraycopy(sortedRowsByCols[col], start_offset, slice, 0, length);
        return slice;
    }

    /**
     * Select and create a new IntTable. For every row r in the new table, r[col]=val.
     * @return The new table, or NULL if no such row in the original table.
     */
    public IntTable select(int col, int val) {
        int[][] slice = getSlice(col, val);
        if (0 == slice.length) {
            return null;
        }
        return new IntTable(slice);
    }

    /**
     * Split the table according to values in a column. The values at the column are the same in each slice.
     */
    public int[][][] splitSlices(int col) {
        int[][] sorted_rows = sortedRowsByCols[col];
        int[] start_offsets = startOffsetsByCols[col];
        int[][][] slices = new int[start_offsets.length - 1][][];
        for (int i = 0; i < slices.length; i++) {
            int start_offset = start_offsets[i];
            int length = start_offsets[i+1] - start_offset;
            int[][] slice = new int[length][];
            System.arraycopy(sorted_rows, start_offset, slice, 0, length);
            slices[i] = slice;
        }
        return slices;
    }

    static public class MatchedSubTables {
        public final List<int[][]> slices1;
        public final List<int[][]> slices2;

        public MatchedSubTables(List<int[][]> slices1, List<int[][]> slices2) {
            this.slices1 = slices1;
            this.slices2 = slices2;
        }
    }

    /**
     * Match the values of two columns in two slices. For each of the matched value v, derive a pair of slices
     * sub_tab1 and sub_tab2, such that: 1) for each row r1 in sub_tab1, r1 is in tab1, r1[col1]=v, and there is no row
     * r1' in tab1 but not in sub_tab1 that r1'[col]=v; 2) for each row r2 in sub_tab2, r2 is in tab2, t2[col2]=v, and
     * there is no row r2' in tab2 but not in sub_tab2 that r2'[col]=v.
     *
     * @return Two arrays of matched sub-tables. Each pair of sub-tables, subTables1[i] and subTables2[i], satisfies the
     * above restrictions.
     */
    public static MatchedSubTables matchSlices(IntTable tab1, int col1, IntTable tab2, int col2) {
        final int[] values1 = tab1.valuesByCols[col1];
        final int[] values2 = tab2.valuesByCols[col2];
        final int[] start_offsets1 = tab1.startOffsetsByCols[col1];
        final int[] start_offsets2 = tab2.startOffsetsByCols[col2];
        final int[][] sorted_rows1 = tab1.sortedRowsByCols[col1];
        final int[][] sorted_rows2 = tab2.sortedRowsByCols[col2];
        List<int[][]> sub_tables1 = new ArrayList<>();
        List<int[][]> sub_tables2 = new ArrayList<>();
        MatchedSubTables result = new MatchedSubTables(sub_tables1, sub_tables2);

        int idx1 = 0;
        int idx2 = 0;
        while (idx1 < values1.length && idx2 < values2.length) {
            int val1 = values1[idx1];
            int val2 = values2[idx2];
            if (val1 < val2) {
                idx1 = Arrays.binarySearch(values1, idx1 + 1, values1.length, val2);
                idx1 = (0 > idx1) ? (-idx1-1) : idx1;
            } else if (val1 > val2) {
                idx2 = Arrays.binarySearch(values2, idx2 + 1, values2.length, val1);
                idx2 = (0 > idx2) ? (-idx2-1) : idx2;
            } else {    // val1 == val2
                int start_idx1 = start_offsets1[idx1];
                int length1 = start_offsets1[idx1+1] - start_idx1;
                int[][] slice1 = new int[length1][];
                System.arraycopy(sorted_rows1, start_idx1, slice1, 0, length1);
                sub_tables1.add(slice1);

                int start_idx2 = start_offsets2[idx2];
                int length2 = start_offsets2[idx2+1] - start_idx2;
                int[][] slice2 = new int[length2][];
                System.arraycopy(sorted_rows2, start_idx2, slice2, 0, length2);
                sub_tables2.add(slice2);

                idx1++;
                idx2++;
            }
        }
        return result;
    }

    /**
     * Extend the binary "matchSlices()" to n tables.
     *
     * @param tables The n tables
     * @param cols   n column numbers, each of the corresponding table
     * @return n lists of slices. Slices in a list is from the same table.
     */
    static public List<int[][]>[] matchSlices(IntTable[] tables, int[] cols) {
        List<int[][]>[] slices_lists = new List[tables.length];
        final int[][] values_arr = new int[tables.length][];
        final int[][] start_offsets_arr = new int[tables.length][];
        final int[][][] sorted_rows_arr = new int[tables.length][][];
        final int[] idxs = new int[tables.length];
        for (int i = 0; i < tables.length; i++) {
            slices_lists[i] = new ArrayList<>();
            IntTable table = tables[i];
            int col = cols[i];
            values_arr[i] = table.valuesByCols[col];
            start_offsets_arr[i] = table.startOffsetsByCols[col];
            sorted_rows_arr[i] = table.sortedRowsByCols[col];
        }
        while (true) {
            /* Locate the maximum value */
            int max_val = values_arr[0][idxs[0]];
            int max_idx = 0;
            boolean all_match = true;
            for (int i = 1; i < tables.length; i++) {
                int val = values_arr[i][idxs[i]];
                all_match &= val == max_val;
                if (val > max_val) {
                    max_val = val;
                    max_idx = i;
                }
            }

            /* Match */
            if (all_match) {
                for (int i = 0; i < tables.length; i++) {
                    int idx = idxs[i];
                    int[] start_offsets = start_offsets_arr[i];
                    int start_idx = start_offsets[idx];
                    int length = start_offsets[idx+1] - start_idx;
                    int[][] slice = new int[length][];
                    System.arraycopy(sorted_rows_arr[i], start_idx, slice, 0, length);
                    slices_lists[i].add(slice);
                    idxs[i]++;
                }
            } else {
                /* Update idxs */
                for (int i = 0; i < tables.length; i++) {
                    if (i == max_idx) {
                        continue;
                    }
                    int new_idx = Arrays.binarySearch(values_arr[i], idxs[i], values_arr[i].length, max_val);
                    idxs[i] = (0 > new_idx) ? (-new_idx-1) : new_idx;
                }
            }
            for (int i = 0; i < tables.length; i++) {
                if (idxs[i] >= values_arr[i].length) {
                    return slices_lists;
                }
            }
        }
    }

    /**
     * Split the current table into slices, and in each slice, the arguments of the two columns are the same.
     */
    public List<int[][]> matchSlices(int col1, int col2) {
        List<int[][]> slices = new ArrayList<>();
        int[][] sorted_rows = sortedRowsByCols[col1];
        int[] values = valuesByCols[col1];
        int[] start_offsets = startOffsetsByCols[col1];
        for (int i = 0; i < values.length; i++) {
            List<int[]> slice = new ArrayList<>();
            int val = values[i];
            int end_offset = start_offsets[i+1];
            for (int offset = start_offsets[i]; offset < end_offset; offset++) {
                int[] row = sorted_rows[offset];
                if (val == row[col2]) {
                    slice.add(row);
                }
            }
            if (!slice.isEmpty()) {
                slices.add(slice.toArray(new int[0][]));
            }
        }
        return slices;
    }

    /**
     * Split the current table into slices, and in each slice, the arguments of the given columns are the same.
     */
    public List<int[][]> matchSlices(int[] cols) {
        List<int[][]> slices = new ArrayList<>();
        final int first_col = cols[0];
        int[][] sorted_rows = sortedRowsByCols[first_col];
        int[] values = valuesByCols[first_col];
        int[] start_offsets = startOffsetsByCols[first_col];
        for (int i = 0; i < values.length; i++) {
            List<int[]> slice = new ArrayList<>();
            final int val = values[i];
            int end_offset = start_offsets[i+1];
            for (int offset = start_offsets[i]; offset < end_offset; offset++) {
                int[] row = sorted_rows[offset];
                boolean all_match = true;
                for (int j = 1; j < cols.length; j++) {
                    if (val != row[cols[j]]) {
                        all_match = false;
                        break;
                    }
                }
                if (all_match) {
                    slice.add(row);
                }
            }
            if (!slice.isEmpty()) {
                slices.add(slice.toArray(new int[0][]));
            }
        }
        return slices;
    }

    /**
     * Find the intersection of this table and another list of rows. The two should have the same number of columns, and
     * the list of rows should NOT contain repeated elements.
     * Note that this method WILL sort the 1D arrays in "rows" to some order.
     */
    public int[][] intersection(int[][] rows) {
        if (0 == rows.length || totalCols != rows[0].length) {
            return new int[0][];
        }
        Arrays.sort(rows, rowComparator);
        return intersectionHandler(rows);
    }

    /**
     * Find the intersection of this table and another list of rows. The two should have the same number of columns, and
     * the list of rows should NOT contain repeated elements.
     * Note that this method will NOT sort "rows", as it assumes the rows are already sorted alphabetically.
     */
    public int[][] intersectionWithSortedRows(int[][] rows) {
        if (0 == rows.length || totalCols != rows[0].length) {
            return new int[0][];
        }
        return intersectionHandler(rows);
    }

    /**
     * Handler of the intersection operation.
     */
    protected int[][] intersectionHandler(int[][] rows) {
        int[][] this_rows = sortedRowsByCols[0];
        List<int[]> results = new ArrayList<>();
        int idx = 0;
        int idx2 = 0;
        while (idx < totalRows && idx2 < rows.length) {
            int[] row = this_rows[idx];
            int[] row2 = rows[idx2];
            int diff = rowComparator.compare(row, row2);
            if (0 > diff) {
                idx = Arrays.binarySearch(this_rows, idx + 1, totalRows, row2, rowComparator);
                idx = (0 > idx) ? (-idx-1) : idx;
            } else if (0 < diff) {
                idx2 = Arrays.binarySearch(rows, idx2 + 1, rows.length, row, rowComparator);
                idx2 = (0 > idx2) ? (-idx2-1) : idx2;
            } else {    // row == row2
                results.add(row2);
                idx++;
                idx2++;
            }
        }
        return results.toArray(new int[0][]);
    }

    /**
     * Find the intersection of this table and another. The two tables should have the same number of columns.
     */
    public int[][] intersection(IntTable another) {
        if (totalCols != another.totalCols) {
            return new int[0][];
        }
        int[][] sorted_rows = sortedRowsByCols[0];
        int[][] sorted_rows2 = another.sortedRowsByCols[0];
        List<int[]> results = new ArrayList<>();

        int idx = 0;
        int idx2 = 0;
        while (idx < totalRows && idx2 < another.totalRows) {
            int[] row = sorted_rows[idx];
            int[] row2 = sorted_rows2[idx2];
            int diff = rowComparator.compare(row, row2);
            if (0 > diff) {
                idx = Arrays.binarySearch(sorted_rows, idx + 1, totalRows, row2, rowComparator);
                idx = (0 > idx) ? (-idx-1) : idx;
            } else if (0 < diff) {
                idx2 = Arrays.binarySearch(sorted_rows2, idx2 + 1, another.totalRows, row, rowComparator);
                idx2 = (0 > idx2) ? (-idx2-1) : idx2;
            } else {    // row == row2
                results.add(row2);
                idx++;
                idx2++;
            }
        }
        return results.toArray(new int[0][]);
    }

    /**
     * Get the list of different values in a certain column. The returned array SHALL NOT be modified.
     */
    public int[] valuesInColumn(int col) {
        return valuesByCols[col];
    }

    /**
     * Get all the rows in the table. The returned array SHALL NOT be modified.
     */
    public int[][] getAllRows() {
        return sortedRowsByCols[0];
    }

    /**
     * Join two tables by "col1" in "tab1" and "col2" in "tab2". The semantics is the same as the following SQL query:
     * SELECT DISTINCT tab1.selectedCols1, tab2.selectedCols2
     * FROM tab1, tab2
     * WHERE tab1.col1=tab2.col2
     */
    public static int[][] join(
            IntTable tab1, int col1, int[] selectedCols1, IntTable tab2, int col2, int[] selectedCols2
    ) {
        final int result_arity = selectedCols1.length + selectedCols2.length;
        final int[] values1 = tab1.valuesByCols[col1];
        final int[] values2 = tab2.valuesByCols[col2];
        final int[] start_offsets1 = tab1.startOffsetsByCols[col1];
        final int[] start_offsets2 = tab2.startOffsetsByCols[col2];
        final int[][] sorted_rows1 = tab1.sortedRowsByCols[col1];
        final int[][] sorted_rows2 = tab2.sortedRowsByCols[col2];
        Set<Record> result_set = new HashSet<>();

        int idx1 = 0;
        int idx2 = 0;
        while (idx1 < values1.length && idx2 < values2.length) {
            int val1 = values1[idx1];
            int val2 = values2[idx2];
            if (val1 < val2) {
                idx1 = Arrays.binarySearch(values1, idx1 + 1, values1.length, val2);
                idx1 = (0 > idx1) ? (-idx1-1) : idx1;
            } else if (val1 > val2) {
                idx2 = Arrays.binarySearch(values2, idx2 + 1, values2.length, val1);
                idx2 = (0 > idx2) ? (-idx2-1) : idx2;
            } else {    // val1 == val2, note that repeated elements should be removed
                final int offset_end1 = start_offsets1[idx1+1];
                final int offset_end2 = start_offsets2[idx2+1];
                Set<Record> tab1_selected_cols = new HashSet<>();
                Set<Record> tab2_selected_cols = new HashSet<>();
                for (int offset1 = start_offsets1[idx1]; offset1 < offset_end1; offset1++) {
                    int[] row = sorted_rows1[offset1];
                    int[] selected_cols = new int[selectedCols1.length];
                    for (int i = 0; i < selected_cols.length; i++) {
                        selected_cols[i] = row[selectedCols1[i]];
                    }
                    tab1_selected_cols.add(new Record(selected_cols));
                }
                for (int offset2 = start_offsets2[idx2]; offset2 < offset_end2; offset2++) {
                    int[] row = sorted_rows2[offset2];
                    int[] selected_cols = new int[selectedCols2.length];
                    for (int i = 0; i < selected_cols.length; i++) {
                        selected_cols[i] = row[selectedCols2[i]];
                    }
                    tab2_selected_cols.add(new Record(selected_cols));
                }
                for (Record record1: tab1_selected_cols) {
                    for (Record record2: tab2_selected_cols) {
                        int[] combined = new int[result_arity];
                        System.arraycopy(record1.args, 0, combined, 0, selectedCols1.length);
                        System.arraycopy(record2.args, 0, combined, selectedCols1.length, selectedCols2.length);
                        result_set.add(new Record(combined));
                    }
                }

                idx1++;
                idx2++;
            }
        }
        int[][] results = new int[result_set.size()][];
        int idx = 0;
        for (Record record: result_set) {
            results[idx] = record.args;
            idx++;
        }
        return results;
    }


    /**
     * Join two tables by "col1" in "tab1" and "col2" in "tab2". The semantics is the same as the following SQL query:
     * SELECT DISTINCT tab1.selectedCol1, tab2.selectedCol2
     * FROM tab1, tab2
     * WHERE tab1.col1=tab2.col2
     */
    public static int[][] join(
            IntTable tab1, int col1, int selectedCol1, IntTable tab2, int col2, int selectedCol2
    ) {
        final int[] values1 = tab1.valuesByCols[col1];
        final int[] values2 = tab2.valuesByCols[col2];
        final int[] start_offsets1 = tab1.startOffsetsByCols[col1];
        final int[] start_offsets2 = tab2.startOffsetsByCols[col2];
        final int[][] sorted_rows1 = tab1.sortedRowsByCols[col1];
        final int[][] sorted_rows2 = tab2.sortedRowsByCols[col2];
        HashSet<Record> result_set = new HashSet<>();

        int idx1 = 0;
        int idx2 = 0;
        while (idx1 < values1.length && idx2 < values2.length) {
            int val1 = values1[idx1];
            int val2 = values2[idx2];
            if (val1 < val2) {
                idx1 = Arrays.binarySearch(values1, idx1 + 1, values1.length, val2);
                idx1 = (0 > idx1) ? (-idx1-1) : idx1;
            } else if (val1 > val2) {
                idx2 = Arrays.binarySearch(values2, idx2 + 1, values2.length, val1);
                idx2 = (0 > idx2) ? (-idx2-1) : idx2;
            } else {    // val1 == val2, note that repeated elements should be removed
                final int offset_end1 = start_offsets1[idx1+1];
                final int offset_end2 = start_offsets2[idx2+1];
                Set<Integer> tab1_selected_values = new HashSet<>();
                Set<Integer> tab2_selected_values = new HashSet<>();
                for (int offset1 = start_offsets1[idx1]; offset1 < offset_end1; offset1++) {
                    tab1_selected_values.add(sorted_rows1[offset1][selectedCol1]);
                }
                for (int offset2 = start_offsets2[idx2]; offset2 < offset_end2; offset2++) {
                    tab2_selected_values.add(sorted_rows2[offset2][selectedCol2]);
                }
                for (int selected_val1: tab1_selected_values) {
                    for (int selected_val2: tab2_selected_values) {
                        result_set.add(new Record(new int[]{selected_val1, selected_val2}));
                    }
                }

                idx1++;
                idx2++;
            }
        }
        int[][] results = new int[result_set.size()][];
        int idx = 0;
        for (Record record: result_set) {
            results[idx] = record.args;
            idx++;
        }
        return results;
    }

    public int totalRows() {
        return totalRows;
    }

    public int totalCols() {
        return totalCols;
    }

    public int minValue(int col) {
        return valuesByCols[col][0];
    }

    public int maxValue(int col) {
        int[] values = valuesByCols[col];
        return values[values.length-1];
    }

    public int maxValue() {
        int max_value = valuesByCols[0][0];
        for (int[] values: valuesByCols) {
            max_value = Math.max(max_value, values[values.length-1]);
        }
        return max_value;
    }

    public static class SimInfo {
        public final double simIJ;
        public final double simJI;

        public SimInfo(double simIJ, double simJI) {
            this.simIJ = simIJ;
            this.simJI = simJI;
        }
    }

    /**
     * Calculate the similarities between two columns in two tables. The similarity is defined as:
     * sim(i, j) = intersection(i, j) / length(i)
     * where intersection is the number of shared elements in columns i and j; length(i) is the length of column i.
     */
    public static SimInfo columnSimilarity(IntTable tabi, int coli, IntTable tabj, int colj) {
        int[] values_i = tabi.valuesByCols[coli];
        int[] offsets_i = tabi.startOffsetsByCols[coli];
        int[] values_j = tabj.valuesByCols[colj];
        int[] offsets_j = tabj.startOffsetsByCols[colj];
        if (0 == values_i.length || 0 == values_j.length) {
            return new SimInfo(0, 0);
        }
        int matched_in_i = 0;
        int matched_in_j = 0;
        int idx_i = 0;
        int idx_j = 0;
        while (idx_i < values_i.length && idx_j < values_j.length) {
            int val_i = values_i[idx_i];
            int val_j = values_j[idx_j];
            if (val_i < val_j) {
                idx_i = Arrays.binarySearch(values_i, idx_i + 1, values_i.length, val_j);
                idx_i = (0 > idx_i) ? (-idx_i-1) : idx_i;
            } else if (val_i > val_j) {
                idx_j = Arrays.binarySearch(values_j, idx_j + 1, values_j.length, val_i);
                idx_j = (0 > idx_j) ? (-idx_j-1) : idx_j;
            } else {    // val_i == val_j
                matched_in_i += offsets_i[idx_i + 1] - offsets_i[idx_i];
                matched_in_j += offsets_j[idx_j + 1] - offsets_j[idx_j];
                idx_i++;
                idx_j++;
            }
        }
        return new SimInfo(((double) matched_in_i) / tabi.totalRows, ((double) matched_in_j) / tabj.totalRows);
    }

    /**
     * This method returns a list of indices. The i-th element of this list is the index, in this table, that the i-th
     * row in another table should be inserted to. If this or the other table is empty, or the two tables do not match
     * in arity, NULL is returned.
     *
     * NOTE: The given table should NOT have elements in common with this table
     */
    public int[] insertIndices(IntTable another) {
        if (0 == totalRows || 0 == another.totalRows || totalCols != another.totalCols) {
            return null;
        }
        int[][] sorted_rows = sortedRowsByCols[0];
        int[][] sorted_rows2 = another.sortedRowsByCols[0];
        int[] insert_indices = new int[sorted_rows2.length];
        int previous_inserting_index = 0;
        for (int i = 0; i < insert_indices.length; i++) {
            int[] inserting_row = sorted_rows2[i];
            previous_inserting_index = -Arrays.binarySearch(
                    sorted_rows, previous_inserting_index, totalRows, inserting_row, rowComparator
            ) - 1;  // The "inserting_row" is certainly not in the current table
            insert_indices[i] = previous_inserting_index;
            if (previous_inserting_index >= totalRows) {
                /* The remaining rows are all at the same inserting index */
                for (int j = i + 1; j < insert_indices.length; j++) {
                    insert_indices[j] = previous_inserting_index;
                }
                break;
            }
        }
        return insert_indices;
    }
}
