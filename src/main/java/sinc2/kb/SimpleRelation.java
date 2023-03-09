package sinc2.kb;

import sinc2.util.ArrayOperation;
import sinc2.util.io.IntReader;
import sinc2.util.io.IntWriter;
import sinc2.util.kb.KbRelation;

import java.io.File;
import java.io.IOException;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Set;

/**
 * This class stores the records in a relation as an integer table.
 *
 * @since 2.1
 */
public class SimpleRelation extends IntTable {

    /** The threshold for pruning useful constants */
    public static double MIN_CONSTANT_COVERAGE = 0.25;
    protected static final int BITS_PER_INT = Integer.BYTES * 8;

    /** Relation name */
    public final String name;
    /** The ID number of the relation */
    public final int id;
    /** The flags are used to denote whether a record has been marked entailed */
    protected final int[] entailmentFlags;

    /**
     * This method loads a relation file as a 2D array of integers. Please refer to "KbRelation" for the file format.
     *
     * @param file         The file containing the relation data
     * @param arity        The arity of the relation
     * @param totalRecords The number of records in the relation
     * @throws IOException
     * @see KbRelation
     */
    static public int[][] loadFile(File file, int arity, int totalRecords) throws IOException {
        IntReader reader = new IntReader(file);
        byte[] buffer = new byte[Integer.BYTES];
        int[][] records = new int[totalRecords][];
        for (int i = 0; i < totalRecords; i++) {
            int[] record = new int[arity];
            for (int arg_idx = 0; arg_idx < arity; arg_idx++) {
                record[arg_idx] = reader.next();
            }
            records[i] = record;
        }
        reader.close();
        return records;
    }

    /**
     * Create a relation directly from a list of records
     */
    public SimpleRelation(String name, int id, int[][] records) {
        super(records);
        this.name = name;
        this.id = id;
        entailmentFlags = new int[totalRows / BITS_PER_INT + ((0 == totalRows % BITS_PER_INT) ? 0 : 1)];
    }

    /**
     * Create a relation from a relation file
     * @throws IOException
     */
    public SimpleRelation(String name, int id, int arity, int totalRecords, String fileName, String kbPtah) throws IOException {
        super(loadFile(Paths.get(kbPtah, fileName).toFile(), arity, totalRecords));
        this.name = name;
        this.id = id;
        entailmentFlags = new int[totalRows / BITS_PER_INT + ((0 == totalRows % BITS_PER_INT) ? 0 : 1)];
    }

    /**
     * Set a record as entailed if it is in the relation.
     */
    public void setAsEntailed(int[] record) {
        int idx = whereIs(record);
        if (0 <= idx) {
            setEntailmentFlag(idx);
        }
    }

    /**
     * Set the idx-th bit corresponding as true.
     */
    protected void setEntailmentFlag(int idx) {
        entailmentFlags[idx / BITS_PER_INT] |= 0x1 << (idx % BITS_PER_INT);
    }

    /**
     * Set a record as not entailed if it is in the relation.
     */
    public void setAsNotEntailed(int[] record) {
        int idx = whereIs(record);
        if (0 <= idx) {
            unsetEntailmentFlag(idx);
        }
    }

    /**
     * Set the idx-th bit corresponding as false.
     */
    protected void unsetEntailmentFlag(int idx) {
        entailmentFlags[idx / BITS_PER_INT] &= ~(0x1 << (idx % BITS_PER_INT));
    }

    /**
     * Set all records in the list as entailed in the relation if presented. The method WILL sort the records by the
     * alphabetical order.
     */
    public void setAllAsEntailed(int[][] records) {
        if (0 == records.length || totalCols != records[0].length) {
            return;
        }
        Arrays.sort(records, rowComparator);
        int[][] this_rows = sortedRowsByCols[0];
        int idx = 0;
        int idx2 = 0;
        while (idx < totalRows && idx2 < records.length) {
            int[] row = this_rows[idx];
            int[] row2 = records[idx2];
            int diff = rowComparator.compare(row, row2);
            if (0 > diff) {
                idx = Arrays.binarySearch(this_rows, idx + 1, totalRows, row2, rowComparator);
                idx = (0 > idx) ? (-idx-1) : idx;
            } else if (0 < diff) {
                idx2 = Arrays.binarySearch(records, idx2 + 1, records.length, row, rowComparator);
                idx2 = (0 > idx2) ? (-idx2-1) : idx2;
            } else {    // row == row2
                setEntailmentFlag(idx);
                idx++;
                idx2++;
            }
        }
    }

    /**
     * Check whether a record is in the relation and is entailed.
     */
    public boolean isEntailed(int[] record) {
        int idx = whereIs(record);
        return (0 <= idx) && 0 != entailment(idx);
    }

    /**
     * If the record is in the relation and has not been marked as entailed, mark the record as entailed and return true.
     * Otherwise, return false.
     */
    public boolean entailIfNot(int[] record) {
        int idx = whereIs(record);
        if (0 <= idx && 0 == entailment(idx)) {
            setEntailmentFlag(idx);
            return true;
        }
        return false;
    }

    /**
     * Get the entailment bit of the idx-th record. The parameter should satisfy: 0 <= idx < totalRows.
     *
     * @return 0 if the bit is 0, non-zero otherwise.
     */
    protected int entailment(int idx) {
        return entailmentFlags[idx / BITS_PER_INT] & (0x1 << (idx % BITS_PER_INT));
    }

    /**
     * Return the total number of entailed records in this relation.
     */
    public int totalEntailedRecords() {
        int cnt = 0;
        for (int i: entailmentFlags) {
            cnt += Integer.bitCount(i);
        }
        return cnt;
    }

    /**
     * Find the promising constants according to current records.
     */
    public int[][] getPromisingConstants() {
        int[][] promising_constants_by_cols = new int[totalCols][];
        final int threshold = (int) Math.ceil(totalRows * MIN_CONSTANT_COVERAGE);
        for (int col = 0; col < totalCols; col++) {
            List<Integer> promising_constants = new ArrayList<>();
            int[] values = valuesByCols[col];
            int[] start_offsets = startOffsetsByCols[col];
            for (int i = 0; i < values.length; i++) {
                if (threshold <= start_offsets[i + 1] - start_offsets[i]) {
                    promising_constants.add(values[i]);
                }
            }
            promising_constants_by_cols[col] = ArrayOperation.toArray(promising_constants);
        }
        return promising_constants_by_cols;
    }

    /**
     * Dump all records to a binary file. The format is the same as "KbRelation".
     *
     * @param basePath The path to where the relation file should be stored.
     * @see KbRelation
     */
    public void dump(String basePath, String fileName) throws IOException {
        IntWriter writer = new IntWriter(Paths.get(basePath, fileName).toFile());
        for (int[] record: sortedRowsByCols[0]) {
            for (int arg : record) {
                writer.write(arg);
            }
        }
        writer.close();
    }

    /**
     * Write the records that are not entailed and identified by FVS to a binary file. The format is the same as
     * "KbRelation".
     *
     * @param basePath The path to where the relation file should be stored.
     * @throws KbException File writing failure
     * @see KbRelation
     */
    public void dumpNecessaryRecords(String basePath, String fileName, List<int[]> fvsRecords) throws KbException {
        try (IntWriter writer = new IntWriter(Paths.get(basePath, fileName).toFile())) {
            int[][] records = sortedRowsByCols[0];
            for (int idx = 0; idx < totalRows; idx++) {
                if (0 == entailment(idx)) {
                    for (int arg : records[idx]) {
                        writer.write(arg);
                    }
                }
            }
            for (int[] record: fvsRecords) {
                for (int arg : record) {
                    writer.write(arg);
                }
            }
        } catch (IOException e) {
            throw new KbException(e);
        }
    }

    /**
     * Add all constants in the relation to the set "constants".
     */
    public void collectConstants(Set<Integer> constants) {
        for (int[] values: valuesByCols) {
            for (int value: values) {
                constants.add(value);
            }
        }
    }

    /**
     * Remove constants of non-entailed records from the set "constants".
     */
    public void setFlagOfReservedConstants(int[] flags) {
        int[][] records = sortedRowsByCols[0];
        for (int idx = 0; idx < totalRows; idx++) {
            if (0 == entailment(idx)) {
                for (int arg : records[idx]) {
                    flags[arg / BITS_PER_INT] |= 1 << (arg % BITS_PER_INT);
                }
            }
        }
    }

    public int[] getRowAt(int idx) {
        return sortedRowsByCols[0][idx];
    }

    /**
     * Split rows in the relation by whether they are flagged as entailed.
     */
    public SplitRecords splitByEntailment() {
        int already_entailed_cnt = totalEntailedRecords();
        if (0 == already_entailed_cnt) {
            return new SplitRecords(new int[0][], getAllRows());
        }
        if (totalRows() == already_entailed_cnt) {
            return new SplitRecords(getAllRows(), new int[0][]);
        }
        int[][] entailed_records = new int[already_entailed_cnt][];
        int[][] non_entailed_records = new int[totalRows() - already_entailed_cnt][];
        int idx = 0;
        int idx_ent = 0;
        int idx_non_ent = 0;
        int[][] rows = sortedRowsByCols[0];
        for (int flags : entailmentFlags) {
            int limit = Math.min(idx + BITS_PER_INT, totalRows);
            int mask = 1;
            while (idx < limit) {
                if (0 == (flags & mask)) {
                    /* Not entailed */
                    non_entailed_records[idx_non_ent] = rows[idx];
                    idx_non_ent++;
                } else {
                    /* Entailed */
                    entailed_records[idx_ent] = rows[idx];
                    idx_ent++;
                }
                idx++;
                mask = mask << 1;
            }
        }
        return new SplitRecords(entailed_records, non_entailed_records);
    }
}
