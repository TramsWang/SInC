package sinc2.kb;

import sinc2.common.Record;
import sinc2.util.ArrayOperation;
import sinc2.util.LittleEndianIntIO;
import sinc2.util.kb.KbRelation;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
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
     * @param name         The name of the relation
     * @param arity        The arity of the relation
     * @param totalRecords The number of records in the relation
     * @param kbPtah       The path to the KB that the relation belongs to
     * @throws IOException
     * @see KbRelation
     */
    static protected int[][] loadFile(String name, int arity, int totalRecords, String kbPtah) throws IOException {
        File rel_file = KbRelation.getRelFilePath(kbPtah, name, arity, totalRecords).toFile();
        FileInputStream fis = new FileInputStream(rel_file);
        byte[] buffer = new byte[Integer.BYTES];
        int[][] records = new int[totalRecords][];
        for (int i = 0; i < totalRecords; i++) {
            int[] record = new int[arity];
            for (int arg_idx = 0; arg_idx < arity && Integer.BYTES == fis.read(buffer); arg_idx++) {
                record[arg_idx] = LittleEndianIntIO.byteArray2LeInt(buffer);
            }
            records[i] = record;
        }
        fis.close();
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
    public SimpleRelation(String name, int id, int arity, int totalRecords, String kbPtah) throws IOException {
        super(loadFile(name, arity, totalRecords, kbPtah));
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
        if (0 < idx && 0 == entailment(idx)) {
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
     * @throws KbException File writing failure
     * @see KbRelation
     */
    public void dump(String basePath) throws KbException {
        try {
            FileOutputStream fos = new FileOutputStream(KbRelation.getRelFilePath(
                    basePath, name, totalCols, totalRows
            ).toFile());
            for (int [] record: sortedRowsByCols[0]) {
                for (int i : record) {
                    fos.write(LittleEndianIntIO.leInt2ByteArray(i));
                }
            }
            fos.close();
        } catch (IOException e) {
            throw new KbException(e);
        }
    }

    /**
     * Write the records that are not entailed and identified by FVS to a binary file. The format is the same as
     * "KbRelation".
     *
     * @param basePath The path to where the relation file should be stored.
     * @throws KbException File writing failure
     * @see KbRelation
     */
    public void dumpNecessaryRecords(String basePath, List<int[]> fvsRecords) throws KbException {
        try {
            FileOutputStream fos = new FileOutputStream(KbRelation.getRelFilePath(
                    basePath, name, totalCols, totalRows
            ).toFile());
            int[][] records = sortedRowsByCols[0];
            for (int idx = 0; idx < totalRows; idx++) {
                if (0 == entailment(idx)) {
                    for (int i : records[idx]) {
                        fos.write(LittleEndianIntIO.leInt2ByteArray(i));
                    }
                }
            }
            for (int [] record: fvsRecords) {
                for (int i : record) {
                    fos.write(LittleEndianIntIO.leInt2ByteArray(i));
                }
            }
            fos.close();
        } catch (IOException e) {
            throw new KbException(e);
        }
    }

    public void dumpCounterexamples(String basePath, Set<Record> records) throws KbException {
        if (0 < records.size()) {
            /* Dump only non-empty relations */
            try {
                FileOutputStream fos = new FileOutputStream(SimpleCompressedKb.getCounterexampleFilePath(
                        basePath, name, totalCols, records.size()
                ).toFile());
                for (Record counterexample: records) {
                    for (int arg: counterexample.args) {
                        fos.write(LittleEndianIntIO.leInt2ByteArray(arg));
                    }
                }
                fos.close();
            } catch (IOException e) {
                throw new KbException(e);
            }
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
    public void removeReservedConstants(Set<Integer> constants) {
        int[][] records = sortedRowsByCols[0];
        for (int idx = 0; idx < totalRows; idx++) {
            if (0 == entailment(idx)) {
                for (int i : records[idx]) {
                    constants.remove(i);
                }
            }
        }
    }
}
