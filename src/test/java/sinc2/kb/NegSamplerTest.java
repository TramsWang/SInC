package sinc2.kb;

import org.junit.jupiter.api.Test;
import sinc2.common.Argument;
import sinc2.common.Predicate;
import sinc2.common.Record;
import sinc2.impl.negsamp.CB;
import sinc2.impl.negsamp.CacheFragment;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static org.junit.jupiter.api.Assertions.*;

class NegSamplerTest {
    @Test
    void testUniformSampling1() {
        final int TOTAL_CONSTANT = 10;
        final int BUDGET = 50;
        int[][] pos_records = new int[][] {
                new int[] {1, 2, 9},
                new int[] {1, 5, 2},
                new int[] {1, 5, 3},
                new int[] {2, 4, 3},
                new int[] {5, 3, 4},
        };
        IntTable pos_tab = new IntTable(pos_records);
        IntTable neg_tab = NegSampler.uniformSampling(pos_tab, TOTAL_CONSTANT, BUDGET);
        assertEquals(BUDGET, neg_tab.totalRows());
        assertEquals(3, neg_tab.totalCols());
        for (int[] neg_sample: neg_tab.getAllRows()) {
            assertFalse(pos_tab.hasRow(neg_sample));
            for (int arg: neg_sample) {
                assertTrue(arg >= 1 && arg <= TOTAL_CONSTANT);
            }
        }
    }
    @Test
    void testUniformSampling2() {
        final int TOTAL_CONSTANT = 10;
        final int BUDGET = 6;
        int[][] pos_records = new int[][] {
                new int[]{1},
                new int[]{2},
                new int[]{4},
                new int[]{8},
        };
        IntTable pos_tab = new IntTable(pos_records);
        IntTable neg_tab = NegSampler.uniformSampling(pos_tab, TOTAL_CONSTANT, BUDGET);

        int[][] expected_neg_samples = new int[][] {
                new int[]{3},
                new int[]{5},
                new int[]{6},
                new int[]{7},
                new int[]{9},
                new int[]{10},
        };
        assertEquals(expected_neg_samples.length, neg_tab.totalRows());
        int[][] actual_neg_samples = neg_tab.getAllRows();
        for (int i = 0; i < expected_neg_samples.length; i++) {
            assertArrayEquals(expected_neg_samples[i], actual_neg_samples[i]);
        }
    }

    @Test
    void testPositiveRelativeSampling() {
        final int TOTAL_CONSTANT = 10;
        final int BUDGET = 20;
        int[][] pos_records = new int[][] {
                new int[] {1, 2, 9},
                new int[] {1, 5, 2},
                new int[] {1, 5, 3},
                new int[] {2, 4, 3},
                new int[] {5, 3, 4},
        };
        IntTable pos_tab = new IntTable(pos_records);
        IntTable neg_tab = NegSampler.posRelativeSampling(pos_tab, TOTAL_CONSTANT, BUDGET);
        assertEquals(BUDGET, neg_tab.totalRows());
        assertEquals(3, neg_tab.totalCols());
        for (int[] neg_sample: neg_tab.getAllRows()) {
            assertFalse(pos_tab.hasRow(neg_sample));
            for (int arg: neg_sample) {
                assertTrue(arg >= 1 && arg <= TOTAL_CONSTANT);
            }
            boolean found_similar = false;
            for (int[] pos_rec: pos_records) {
                int same_arg = 0;
                for (int arg_idx = 0; arg_idx < pos_rec.length; arg_idx++) {
                    same_arg += (pos_rec[arg_idx] == neg_sample[arg_idx]) ? 1 : 0;
                }
                if (2 == same_arg) {
                    found_similar = true;
                    break;
                }
            }
            assertTrue(found_similar);
        }
    }

    @Test
    void testNegativeIntervalSampling() {
        final int TOTAL_CONSTANT = 10;
        int[][] pos_records = new int[][] {
                new int[] {1, 2, 9},
                new int[] {1, 5, 2},
                new int[] {1, 5, 3},
                new int[] {2, 4, 3},
                new int[] {2, 4, 5},
                new int[] {5, 3, 4},
                new int[] {5, 10, 10},
        };
        IntTable pos_tab = new IntTable(pos_records);
        IntTable neg_tab = NegSampler.negIntervalSampling(pos_tab, TOTAL_CONSTANT);

        int[][] expected_neg_samples = new int[][] {
                new int[] {1, 1, 1},
                new int[] {1, 2, 10},
                new int[] {1, 5, 4},
                new int[] {2, 4, 4},
                new int[] {2, 4, 6},
                new int[] {5, 3, 5},
                new int[] {6, 1, 1},
        };
        assertEquals(expected_neg_samples.length, neg_tab.totalRows());
        assertEquals(3, neg_tab.totalCols());
        int[][] actual_neg_samples = neg_tab.getAllRows();
        for (int i = 0; i < expected_neg_samples.length; i++) {
            assertArrayEquals(expected_neg_samples[i], actual_neg_samples[i]);
        }
    }

    @Test
    void testAdversarialSampling1() {
        final int TOTAL_CONSTANT = 10;
        final int BUDGET = 10;
        final int NUM_P = 0;
        final int NUM_Q = 1;
        final int NUM_H = 2;
        SimpleKb kb = new SimpleKb("test", new int[][][]{
                new int[][]{
                        new int[]{1, 2},
                        new int[]{1, 3},
                        new int[]{1, 4},
                        new int[]{2, 2},
                        new int[]{2, 3},
                        new int[]{5, 6},
                }, new int[][]{
                new int[]{2, 9},
                new int[]{3, 10},
                new int[]{4, 9},
                new int[]{5, 7},
                new int[]{8, 1},
                new int[]{9, 9},
        }, new int[][]{
                new int[]{1, 9},
                new int[]{1, 3},
        }
        }, new String[]{"p", "q", "h"});
        IntTable tab_p = kb.getRelation(NUM_P);
        IntTable tab_q = kb.getRelation(NUM_Q);
        IntTable tab_h = kb.getRelation(NUM_H);
        IntTable all_neg = new IntTable(new int[][]{
                new int[]{1, 10},
                new int[]{2, 9},
                new int[]{2, 10},
        });

        /* h(X, Y) :- p(X, Z), q(Z, Y) */
        /* (1, 9), (1, 10), (2, 9), (2, 10) */
        List<Predicate> body_structure = List.of(
                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.variable(2)}),
                new Predicate(NUM_Q, new int[]{Argument.variable(2), Argument.variable(1)})
        );
        List<CacheFragment> all_cache = new ArrayList<>();
        all_cache.add(new CacheFragment(body_structure, new IntTable[]{tab_p, tab_q}));
        List<Predicate> rule_structure = List.of(
                new Predicate(NUM_H, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.variable(2)}),
                new Predicate(NUM_Q, new int[]{Argument.variable(2), Argument.variable(1)})
        );
        AdversarialSampledResult actual_result = NegSampler.adversarialSampling(
                tab_h, TOTAL_CONSTANT, BUDGET, all_cache, new CacheFragArgLoc[]{
                        CacheFragArgLoc.createLoc(0, 0, 0),
                        CacheFragArgLoc.createLoc(0, 1, 1),
                }, rule_structure, kb
        );

        IntTable actual_neg_samples = actual_result.negTable;
        assertEquals(2, actual_neg_samples.totalCols());
        assertTrue(3 >= actual_neg_samples.totalRows());
        for (int[] neg_sample: actual_neg_samples) {
            assertTrue(all_neg.hasRow(neg_sample));
        }

        List<List<CB>> expected_cache_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[] {1, 10}
                        }), new CB(new int[][]{
                                new int[] {1, 3}
                        }), new CB(new int[][]{
                                new int[] {3, 10}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[] {2, 9}
                        }), new CB(new int[][]{
                                new int[] {2, 2}
                        }), new CB(new int[][]{
                                new int[] {2, 9}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[] {2, 10}
                        }), new CB(new int[][]{
                                new int[] {2, 3}
                        }), new CB(new int[][]{
                                new int[] {3, 10}
                        })
                )
        );
        for (List<CB> expected_entry: expected_cache_entries) {
            for (CB cb: expected_entry) {
                cb.buildIndices();
            }
        }
        for (List<CB> actual_entry: actual_result.negCache) {
            boolean found = false;
            for (List<CB> expected_entry: expected_cache_entries) {
                boolean entry_equals = true;
                for (int i = 0; i < expected_entry.size(); i++) {
                    if (!tableEqual(expected_entry.get(i).indices, actual_entry.get(i).indices)) {
                        entry_equals = false;
                        break;
                    }
                }
                if (entry_equals) {
                    found = true;
                    break;
                }
            }
            assertTrue(found);
        }
    }

    @Test
    void testAdversarialSampling2() {
        final int TOTAL_CONSTANT = 10;
        final int BUDGET = 50;
        final int NUM_P = 0;
        final int NUM_Q = 1;
        final int NUM_H = 2;
        SimpleKb kb = new SimpleKb("test", new int[][][]{
                new int[][]{
                        new int[]{1, 2},
                        new int[]{1, 3},
                        new int[]{1, 4},
                        new int[]{2, 2},
                        new int[]{2, 3},
                        new int[]{5, 6},
                }, new int[][]{
                        new int[]{2, 9},
                        new int[]{3, 10},
                        new int[]{4, 9},
                        new int[]{5, 7},
                        new int[]{8, 1},
                        new int[]{9, 9},
                }, new int[][]{
                        new int[]{1, 8, 1, 9, 1, 1, 1},
                        new int[]{1, 8, 1, 3, 1, 1, 1},
                }
        }, new String[]{"p", "q", "h"});
        IntTable tab_p = kb.getRelation(NUM_P);
        IntTable tab_q = kb.getRelation(NUM_Q);
        IntTable tab_h = kb.getRelation(NUM_H);
        int[][] neg_templates = new int[][]{
                new int[]{1, 8, 1, 10, 0, 0, 0},
                new int[]{2, 8, 2, 9, 0, 0, 0},
                new int[]{2, 8, 2, 10, 0, 0, 0},
        };
        List<int[]> _all_neg = new ArrayList<>();
        for (int[] neg_template: neg_templates) {
            for (int val_w = 1; val_w <= TOTAL_CONSTANT; val_w++) {
                neg_template[4] = val_w;
                neg_template[5] = val_w;
                for (int val_uv = 1; val_uv <= TOTAL_CONSTANT; val_uv++) {
                    neg_template[6] = val_uv;
                    _all_neg.add(neg_template.clone());
                }
            }
        }
        IntTable all_neg = new IntTable(_all_neg.toArray(new int[0][]));
        List<int[]> _all_pos = new ArrayList<>();
        for (int[] pos_template: tab_h) {
            for (int val_w = 1; val_w <= TOTAL_CONSTANT; val_w++) {
                pos_template[4] = val_w;
                pos_template[5] = val_w;
                for (int val_uv = 1; val_uv <= TOTAL_CONSTANT; val_uv++) {
                    pos_template[6] = val_uv;
                    _all_pos.add(pos_template.clone());
                }
            }
        }
        IntTable all_pos = new IntTable(_all_pos.toArray(new int[0][]));

        /* h(X, 8, X, Y, W, W, ?) :- p(X, Z), q(Z, Y) */
        /* (1, 9), (1, 10), (2, 9), (2, 10) */
        List<Predicate> body_structure = List.of(
                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.variable(2)}),
                new Predicate(NUM_Q, new int[]{Argument.variable(2), Argument.variable(1)})
        );
        List<CacheFragment> all_cache = new ArrayList<>();
        all_cache.add(new CacheFragment(body_structure, new IntTable[]{tab_p, tab_q}));
        List<Predicate> rule_structure = List.of(
                new Predicate(NUM_H, new int[]{
                        Argument.variable(0), Argument.constant(8), Argument.variable(0),
                        Argument.variable(1), Argument.variable(3), Argument.variable(3), Argument.EMPTY_VALUE
                }),
                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.variable(2)}),
                new Predicate(NUM_Q, new int[]{Argument.variable(2), Argument.variable(1)})
        );
        AdversarialSampledResult actual_result = NegSampler.adversarialSampling(
                all_pos, TOTAL_CONSTANT, BUDGET, all_cache, new CacheFragArgLoc[]{
                        CacheFragArgLoc.createLoc(0, 0, 0),
                        CacheFragArgLoc.createConstant(8),
                        CacheFragArgLoc.createLoc(0, 0, 0),
                        CacheFragArgLoc.createLoc(0, 1, 1),
                        CacheFragArgLoc.createVid(3),
                        CacheFragArgLoc.createVid(3),
                        CacheFragArgLoc.createVid(4),
                }, rule_structure, kb
        );

        IntTable actual_neg_samples = actual_result.negTable;
        assertEquals(7, actual_neg_samples.totalCols());
        assertTrue(Math.min(300, BUDGET) >= actual_neg_samples.totalRows());
        for (int[] neg_sample: actual_neg_samples) {
            assertTrue(all_neg.hasRow(neg_sample));
        }
    }

    @Test
    void testCalcSampleWeight() {
        final int TOTAL_CONSTANT = 10;
        int[][] pos_records = new int[][] {
                new int[] {1, 2, 9},    // 18
                new int[] {1, 5, 2},    // 41
                new int[] {1, 5, 3},    // 42
                new int[] {2, 4, 3},    // 132
                new int[] {5, 3, 5},    // 424
        };
        int[][] neg_records = new int[][] {
                new int[] {1, 1, 1},    // 0
                new int[] {1, 1, 2},    // 1
                new int[] {2, 2, 2},    // 111
                new int[] {3, 3, 3},    // 222
                new int[] {5, 3, 6},    // 425
                new int[] {5, 5, 5},    // 444
        };
        float[] expected_weight_list = new float[] {9, 9, 89, 291, 287.5f, 287.5f};
        IntTable pos_tab = new IntTable(pos_records);
        IntTable neg_tab = new IntTable(neg_records);
        Map<Record, Float> expected_weight = new HashMap<>();
        for (int i = 0; i < neg_records.length; i++) {
            expected_weight.put(new Record(neg_records[i]), expected_weight_list[i]);
        }
        assertEquals(expected_weight, NegSampler.calcNegSampleWeight(pos_tab, neg_tab, TOTAL_CONSTANT));
    }

    @Test
    void testRecordsInSpan1() {
        final int TOTAL_CONSTANT = 10;
        int[][] records = new int[][] {
                new int[]{1, 1, 1},     // 0
                new int[]{3, 6, 1},     // 250
                new int[]{3, 6, 2},     // 251
                new int[]{3, 6, 10},    // 259
                new int[]{3, 10, 5},    // 294
                new int[]{10, 9, 10},   // 989
                new int[]{10, 10, 10},  // 999
        };
        int[] expected_num = new int[] {0, 249, 0, 7, 34, 694, 9, 0};
        for (int i = 0; i < expected_num.length; i++) {
            assertEquals(expected_num[i], NegSampler.recordsInInterval(records, i, TOTAL_CONSTANT));
        }
    }

    @Test
    void testRecordsInSpan2() {
        final int TOTAL_CONSTANT = 10;
        int[][] records = new int[][] {
                new int[]{3, 6, 1},     // 250
                new int[]{3, 6, 2},     // 251
                new int[]{3, 6, 10},    // 259
                new int[]{3, 10, 5},    // 294
                new int[]{10, 9, 10},   // 989
        };
        int[] expected_num = new int[] {250, 0, 7, 34, 694, 10};
        for (int i = 0; i < expected_num.length; i++) {
            assertEquals(expected_num[i], NegSampler.recordsInInterval(records, i, TOTAL_CONSTANT));
        }
    }

    @Test
    void testNextRecord() {
        final int TOTAL_CONSTANT = 10;
        int[][] records = new int[][] {
                new int[]{3, 6, 1},
                new int[]{1, 1, 1},
                new int[]{10, 10, 10},
                new int[]{3, 6, 10},
                new int[]{3, 10, 10},
                new int[]{10, 9, 10},
        };
        int[][] expected_next = new int[][] {
                new int[]{3, 6, 2},
                new int[]{1, 1, 2},
                new int[]{10, 10, 10},
                new int[]{3, 7, 1},
                new int[]{4, 1, 1},
                new int[]{10, 10, 1},
        };

        for (int i = 0; i < records.length; i++) {
            assertArrayEquals(expected_next[i], NegSampler.nextRecord(records[i], TOTAL_CONSTANT));
        }
    }

    boolean tableEqual(IntTable expectedTable, IntTable actualTable) {
        if (expectedTable.totalRows() != actualTable.totalRows()) {
            return false;
        }
        if (expectedTable.totalCols() != actualTable.totalCols()) {
            return false;
        }
        int[][] expected_rows = expectedTable.getAllRows();
        int[][] actual_rows = actualTable.getAllRows();
        int cols = expected_rows[0].length;
        for (int i = 0; i < expected_rows.length; i++) {
            int[] expected_row = expected_rows[i];
            int[] actual_row = actual_rows[i];
            for (int j = 0; j < cols; j++) {
                if (expected_row[j] != actual_row[j]) {
                    return false;
                }
            }
        }
        return true;
    }
}