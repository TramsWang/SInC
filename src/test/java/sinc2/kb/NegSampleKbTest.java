package sinc2.kb;

import org.junit.jupiter.api.Test;
import sinc2.common.Record;
import sinc2.util.kb.NumeratedKb;

import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

import static org.junit.jupiter.api.Assertions.*;

class NegSampleKbTest {
    public static final String MEM_DIR = "/dev/shm";

    @Test
    void testDump() throws IOException {
        String name = UUID.randomUUID().toString();
        IntTable[] neg_tables = new IntTable[]{
                new IntTable(new int[][]{
                        new int[]{1},
                        new int[]{2},
                        new int[]{3},
                }),
                new IntTable(new int[][]{
                        new int[]{1, 2},
                        new int[]{2, 4},
                        new int[]{3, 6},
                        new int[]{4, 8},
                        new int[]{5, 10}
                }),
                new IntTable(new int[][]{
                        new int[]{1, 3, 5},
                        new int[]{3, 5, 7},
                        new int[]{5, 7, 9},
                        new int[]{7, 9, 11},
                        new int[]{9, 11, 13},
                        new int[]{11, 13, 15},
                        new int[]{13, 15, 17}
                })
        };
        Map<Record, Float>[] weight_maps = new Map[]{
                new HashMap<Record, Float>(),
                new HashMap<Record, Float>(),
                new HashMap<Record, Float>()
        };
        weight_maps[0].put(new Record(new int[]{1}), 1.0f);
        weight_maps[0].put(new Record(new int[]{2}), 1.1f);
        weight_maps[0].put(new Record(new int[]{3}), 1.2f);
        weight_maps[1].put(new Record(new int[]{1, 2}), 5.5f);
        weight_maps[1].put(new Record(new int[]{2, 4}), 4.4f);
        weight_maps[1].put(new Record(new int[]{3, 6}), 3.3f);
        weight_maps[1].put(new Record(new int[]{4, 8}), 2.2f);
        weight_maps[1].put(new Record(new int[]{5, 10}), 1.1f);
        weight_maps[2].put(new Record(new int[]{1, 3, 5}), 13.135f);
        weight_maps[2].put(new Record(new int[]{3, 5, 7}), 35.357f);
        weight_maps[2].put(new Record(new int[]{5, 7, 9}), 57.579f);
        weight_maps[2].put(new Record(new int[]{7, 9, 11}), 79.7911f);
        weight_maps[2].put(new Record(new int[]{9, 11, 13}), 911.91113f);
        weight_maps[2].put(new Record(new int[]{11, 13, 15}), 1113.111315f);
        weight_maps[2].put(new Record(new int[]{13, 15, 17}), 1315.131517f);

        NegSampleKb kb = new NegSampleKb(name, neg_tables, weight_maps);
        kb.dump(MEM_DIR);

        NegSampleKb kb2 = new NegSampleKb(name, MEM_DIR);
        assertEquals(kb.name, kb2.name);
        assertEquals(kb.negSampleTabs.length, kb2.negSampleTabs.length);
        assertEquals(kb.negSampleWeightMaps.length, kb2.negSampleWeightMaps.length);
        for (int i = 0; i < kb.negSampleTabs.length; i++) {
            assertTableEqual(kb.negSampleTabs[i], kb2.negSampleTabs[i]);
            assertEquals(kb.negSampleWeightMaps[i], kb2.negSampleWeightMaps[i]);
        }
        removeDir(NumeratedKb.getKbPath(name, MEM_DIR).toString());
    }

    void assertTableEqual(IntTable expectedTable, IntTable actualTable) {
        assertEquals(expectedTable.totalRows(), actualTable.totalRows());
        assertEquals(expectedTable.totalCols(), actualTable.totalCols());
        int[][] expected_rows = expectedTable.getAllRows();
        int[][] actual_rows = actualTable.getAllRows();
        int cols = expected_rows[0].length;
        for (int i = 0; i < expected_rows.length; i++) {
            int[] expected_row = expected_rows[i];
            int[] actual_row = actual_rows[i];
            for (int j = 0; j < cols; j++) {
                assertEquals(expected_row[j], actual_row[j]);
            }
        }
    }

    protected void removeDir(String dirPath) {
        File dir = new File(dirPath);
        File[] files = dir.listFiles();
        if (null != files) {
            for (File f : files) {
                if (f.isDirectory()) {
                    removeDir(f.getAbsolutePath().toString());
                } else {
                    assertTrue(f.delete());
                }
            }
            assertTrue(dir.delete());
        }
    }
}