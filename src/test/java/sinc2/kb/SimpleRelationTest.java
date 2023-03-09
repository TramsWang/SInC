package sinc2.kb;

import org.junit.jupiter.api.Test;
import sinc2.common.Record;
import sinc2.util.LittleEndianIntIO;
import sinc2.util.kb.KbRelation;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import static org.junit.jupiter.api.Assertions.*;

class SimpleRelationTest {

    protected static final String TEST_DIR = "/dev/shm";

    @Test
    void testLoad() throws IOException {
        File relation_file = Paths.get(TEST_DIR, "test.rel").toFile();
        FileOutputStream fos = new FileOutputStream(relation_file);
        fos.write(LittleEndianIntIO.leInt2ByteArray(4));
        fos.write(LittleEndianIntIO.leInt2ByteArray(5));
        fos.write(LittleEndianIntIO.leInt2ByteArray(6));
        fos.write(LittleEndianIntIO.leInt2ByteArray(7));
        fos.write(LittleEndianIntIO.leInt2ByteArray(8));
        fos.write(LittleEndianIntIO.leInt2ByteArray(9));
        fos.write(LittleEndianIntIO.leInt2ByteArray(0xa));
        fos.write(LittleEndianIntIO.leInt2ByteArray(0xb));
        fos.write(LittleEndianIntIO.leInt2ByteArray(0xc));
        fos.write(LittleEndianIntIO.leInt2ByteArray(0xd));
        fos.write(LittleEndianIntIO.leInt2ByteArray(0xe));
        fos.write(LittleEndianIntIO.leInt2ByteArray(0xf));
        fos.close();

        SimpleRelation relation = new SimpleRelation("family", 0, 3, 4, "test.rel", TEST_DIR);
        assertEquals(4, relation.totalRows());
        assertEquals(3, relation.totalCols());
        assertTrue(relation.hasRow(new int[]{4, 5, 6}));
        assertTrue(relation.hasRow(new int[]{7, 8, 9}));
        assertTrue(relation.hasRow(new int[]{10, 11, 12}));
        assertTrue(relation.hasRow(new int[]{13, 14, 15}));
    }

    @Test
    void testSetEntailment() {
        int[][] records = new int[55][];
        for (int i = 0; i < 55; i++) {
            records[i] = new int[]{i, i, i};
        }
        SimpleRelation relation = new SimpleRelation("test", 0, records);

        relation.setAsEntailed(new int[]{0, 0, 0});
        relation.setAsEntailed(new int[]{1, 1, 1});
        relation.setAsEntailed(new int[]{31, 31, 31});
        relation.setAsEntailed(new int[]{47, 47, 47});
        relation.setAsEntailed(new int[]{1, 2, 3});
        assertTrue(relation.isEntailed(new int[]{0, 0, 0}));
        assertTrue(relation.isEntailed(new int[]{1, 1, 1}));
        assertTrue(relation.isEntailed(new int[]{31, 31, 31}));
        assertTrue(relation.isEntailed(new int[]{47, 47, 47}));
        assertFalse(relation.isEntailed(new int[]{1, 2, 3}));
        assertFalse(relation.isEntailed(new int[]{3, 3, 3}));
        assertEquals(4, relation.totalEntailedRecords());

        relation.setAsNotEntailed(new int[]{31, 31, 31});
        relation.setAsNotEntailed(new int[]{47, 47, 47});
        relation.setAsNotEntailed(new int[]{4, 5, 6});
        assertTrue(relation.isEntailed(new int[]{0, 0, 0}));
        assertTrue(relation.isEntailed(new int[]{1, 1, 1}));
        assertFalse(relation.isEntailed(new int[]{31, 31, 31}));
        assertFalse(relation.isEntailed(new int[]{47, 47, 47}));
        assertFalse(relation.isEntailed(new int[]{1, 2, 3}));
        assertFalse(relation.isEntailed(new int[]{3, 3, 3}));
        assertEquals(2, relation.totalEntailedRecords());
    }

    @Test
    void testSetAllEntailment() {
        int[][] records = new int[55][];
        for (int i = 0; i < 55; i++) {
            records[i] = new int[]{i, i, i};
        }
        SimpleRelation relation = new SimpleRelation("test", 0, records);

        relation.setAllAsEntailed(new int[][] {
                new int[]{0, 0, 0},
                new int[]{1, 1, 1},
                new int[]{31, 31, 31},
                new int[]{47, 47, 47},
                new int[]{1, 2, 3}
        });
        assertTrue(relation.isEntailed(new int[]{0, 0, 0}));
        assertTrue(relation.isEntailed(new int[]{1, 1, 1}));
        assertTrue(relation.isEntailed(new int[]{31, 31, 31}));
        assertTrue(relation.isEntailed(new int[]{47, 47, 47}));
        assertFalse(relation.isEntailed(new int[]{1, 2, 3}));
        assertFalse(relation.isEntailed(new int[]{3, 3, 3}));
        assertEquals(4, relation.totalEntailedRecords());
    }

    @Test
    void testEntailIfNot() {
        int[][] records = new int[55][];
        for (int i = 0; i < 55; i++) {
            records[i] = new int[]{i, i, i};
        }
        SimpleRelation relation = new SimpleRelation("test", 0, records);

        relation.setAsEntailed(new int[]{1, 1, 1});
        assertTrue(relation.entailIfNot(new int[]{0, 0, 0}));
        assertFalse(relation.entailIfNot(new int[]{0, 0, 0}));
        assertFalse(relation.entailIfNot(new int[]{1, 1, 1}));
        assertTrue(relation.entailIfNot(new int[]{31, 31, 31}));
        assertTrue(relation.entailIfNot(new int[]{47, 47, 47}));
        assertFalse(relation.entailIfNot(new int[]{1, 2, 3}));

        assertTrue(relation.isEntailed(new int[]{0, 0, 0}));
        assertTrue(relation.isEntailed(new int[]{1, 1, 1}));
        assertTrue(relation.isEntailed(new int[]{31, 31, 31}));
        assertTrue(relation.isEntailed(new int[]{47, 47, 47}));
        assertFalse(relation.isEntailed(new int[]{1, 2, 3}));
        assertFalse(relation.isEntailed(new int[]{3, 3, 3}));
        assertEquals(4, relation.totalEntailedRecords());
    }

    @Test
    void testDump() throws KbException, IOException {
        int[][] rows = new int[][] {
                new int[]{1, 2, 3},
                new int[]{4, 5, 6},
                new int[]{7, 8, 9},
                new int[]{10, 11, 12},
                new int[]{13, 14, 15},
        };
        SimpleRelation relation = new SimpleRelation("test", 0, rows);
        relation.setAsEntailed(new int[]{1, 2, 3});
        relation.setAsEntailed(new int[]{4, 5, 6});
        relation.setAsEntailed(new int[]{7, 8, 9});
        relation.dump(TEST_DIR, "test.rel");

        SimpleRelation relation2 = new SimpleRelation(relation.name, 0, 3, 5, "test.rel", TEST_DIR);
        assertEquals(5, relation2.totalRows);
        assertTrue(relation2.hasRow(new int[]{1, 2, 3}));
        assertTrue(relation2.hasRow(new int[]{4, 5, 6}));
        assertTrue(relation2.hasRow(new int[]{7, 8, 9}));
        assertTrue(relation2.hasRow(new int[]{10, 11, 12}));
        assertTrue(relation2.hasRow(new int[]{13, 14, 15}));
        File rel_file = Paths.get(TEST_DIR, "test.rel").toFile();
        assertTrue(rel_file.delete());
    }

    @Test
    void testDumpNecessaryRecords() throws KbException, IOException {
        int[][] rows = new int[][] {
                new int[]{1, 2, 3},
                new int[]{4, 5, 6},
                new int[]{7, 8, 9},
                new int[]{10, 11, 12},
                new int[]{13, 14, 15},
        };
        SimpleRelation relation = new SimpleRelation("test", 0, rows);
        relation.setAsEntailed(new int[]{1, 2, 3});
        relation.setAsEntailed(new int[]{4, 5, 6});
        relation.setAsEntailed(new int[]{7, 8, 9});
        relation.dumpNecessaryRecords(TEST_DIR, "test.rel", List.of(new int[]{1, 2, 3}, new int[]{4, 5, 6}));

        SimpleRelation relation2 = new SimpleRelation("test", 0, 3, 4, "test.rel", TEST_DIR);
        assertEquals(4, relation2.totalRows);
        assertTrue(relation2.hasRow(new int[]{1, 2, 3}));
        assertTrue(relation2.hasRow(new int[]{4, 5, 6}));
        assertTrue(relation2.hasRow(new int[]{10, 11, 12}));
        assertTrue(relation2.hasRow(new int[]{13, 14, 15}));
        File rel_file = Paths.get(TEST_DIR, "test.rel").toFile();
        assertTrue(rel_file.delete());
    }

    @Test
    void testCollectConstants() {
        int[][] rows = new int[][] {
                new int[]{1, 2, 3},
                new int[]{4, 5, 6},
                new int[]{7, 8, 9},
        };
        SimpleRelation relation = new SimpleRelation("test", 0, rows);
        relation.setAsEntailed(new int[]{1, 2, 3});
        relation.setAsEntailed(new int[]{4, 5, 6});
        relation.setAsEntailed(new int[]{7, 8, 9});
        Set<Integer> constants = new HashSet<>();
        relation.collectConstants(constants);
        assertEquals(new HashSet<>(List.of(1, 2, 3, 4, 5, 6, 7, 8, 9)), constants);
    }

    @Test
    void testSetFlagOfReservedConstants() {
        int[][] rows = new int[][] {
                new int[]{1, 2, 3},
                new int[]{4, 5, 6},
                new int[]{7, 8, 9},
        };
        SimpleRelation relation = new SimpleRelation("test", 0, rows);
        relation.setAsEntailed(new int[]{1, 2, 3});
        int[] flags = new int[]{0};
        relation.setFlagOfReservedConstants(flags);
        assertArrayEquals(new int[]{1008}, flags);
    }

    @Test
    void testPromisingConstants() {
        int[][] rows = new int[][] {
                new int[] {1, 5, 3},
                new int[] {2, 4, 3},
                new int[] {1, 2, 9},
                new int[] {5, 3, 3},
                new int[] {1, 5, 2},
        };
        SimpleRelation relation = new SimpleRelation("test", 0, rows);
        SimpleRelation.MIN_CONSTANT_COVERAGE = 0.5;
        int[][] promising_constants = relation.getPromisingConstants();
        assertArrayEquals(new int[][] {
                new int[]{1}, new int[]{}, new int[]{3}
        }, promising_constants);
    }

    @Test
    void testSplitByEntailment() {
        int num_rows = SimpleRelation.BITS_PER_INT * 2 + SimpleRelation.BITS_PER_INT / 3;
        List<int[]> _expected_non_entailed_rows = new ArrayList<>();
        List<int[]> _expected_entailed_rows = new ArrayList<>();
        int[][] rows = new int[num_rows][];
        for (int i = 0; i < num_rows; i++) {
            int[] record = new int[]{i, i, i};
            if (0 == i % 3) {
                _expected_entailed_rows.add(record);
            } else {
                _expected_non_entailed_rows.add(record);
            }
            rows[i] = record;
        }
        SimpleRelation relation = new SimpleRelation("test", 0, rows);
        for (int[] record: _expected_entailed_rows) {
            relation.setAsEntailed(record);
        }
        SplitRecords actual_split_records = relation.splitByEntailment();
        tableEqual(new IntTable(_expected_entailed_rows.toArray(new int[0][])), new IntTable(actual_split_records.entailedRecords));
        tableEqual(new IntTable(_expected_non_entailed_rows.toArray(new int[0][])), new IntTable(actual_split_records.nonEntailedRecords));
    }

    protected void tableEqual(IntTable table1, IntTable table2) {
        assertEquals(table1.totalRows, table2.totalRows);
        assertEquals(table1.totalCols, table2.totalCols);
        for (int[] row: table1) {
            assertTrue(table2.hasRow(row));
        }
        for (int[] row: table2) {
            assertTrue(table1.hasRow(row));
        }
    }

}