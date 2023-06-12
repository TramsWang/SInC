package sinc2.impl.negsamp;

import org.junit.jupiter.api.Test;
import sinc2.common.Predicate;
import sinc2.kb.IntTable;
import sinc2.kb.SimpleKb;

import java.util.ArrayList;
import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

class CacheFragmentTest {
    static final SimpleKb KB = new SimpleKb("test", new int[][][]{
            new int[][] {
                    new int[] {1, 1, 1},
                    new int[] {1, 1, 2},
                    new int[] {1, 2, 3},
                    new int[] {2, 1, 3},
                    new int[] {4, 4, 6},
                    new int[] {5, 5, 1},
                    new int[] {1, 3, 2},
                    new int[] {2, 4, 4},
            },
            new int[][] {
                    new int[] {1, 1, 2},
                    new int[] {3, 1, 4},
                    new int[] {2, 4, 1},
                    new int[] {2, 7, 8},
                    new int[] {5, 1, 4},
                    new int[] {6, 7, 2},
                    new int[] {3, 3, 9}
            }
    }, new String[]{"p", "q"});
    static final int NUM_P = KB.getRelation("p").id;
    static final int NUM_Q = KB.getRelation("q").id;

    @Test
    void case1aTest1() {
        /* p(?, ?, ?) */
        CacheFragment fragment = new CacheFragment(KB.getRelation(NUM_P), NUM_P);

        /* p(X, ?, ?) */
        fragment.updateCase1a(0, 0, 0);
        List<List<CB>> expected_entries = List.of(
                List.of(new CB(KB.getRelation(NUM_P).getAllRows(), KB.getRelation(NUM_P)))
        );
        checkEntries(expected_entries, fragment);
        assertEquals(new ArrayList<>(List.of(
                new CacheFragment.VarInfo(0, 0, true)
        )), fragment.varInfoList);

        /* p(X, X, ?) */
        fragment.buildIndices();
        fragment.updateCase1a(0, 1, 0);
        expected_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1},
                                new int[]{1, 1, 2}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{4, 4, 6}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{5, 5, 1}
                        })
                )
        );
        checkEntries(expected_entries, fragment);
        assertEquals("p(X0,X0,?)", rule2String(fragment.partAssignedRule));
        assertEquals(new ArrayList<>(List.of(
                new CacheFragment.VarInfo(0, 0, false)
        )), fragment.varInfoList);
    }


    @Test
    void case1aTest2() {
        /* p(?, ?, ?) */
        CacheFragment fragment = new CacheFragment(KB.getRelation(NUM_P), NUM_P);

        /* p(X, ?, ?) */
        fragment.updateCase1a(0, 0, 0);

        /* p(X, ?, Z) */
        fragment.buildIndices();
        fragment.updateCase1a(0, 2, 2);
        List<List<CB>> expected_entries = List.of(
                List.of(new CB(KB.getRelation(NUM_P).getAllRows(), KB.getRelation(NUM_P)))
        );
        checkEntries(expected_entries, fragment);

        /* p(X, X, Z) */
        fragment.buildIndices();
        fragment.updateCase1a(0, 1, 0);
        expected_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1},
                                new int[]{1, 1, 2}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{4, 4, 6}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{5, 5, 1}
                        })
                )
        );
        checkEntries(expected_entries, fragment);
        assertEquals("p(X0,X0,X2)", rule2String(fragment.partAssignedRule));
        List<CacheFragment.VarInfo> expected_var_list = new ArrayList<>();
        expected_var_list.add(new CacheFragment.VarInfo(0, 0, false));
        expected_var_list.add(null);
        expected_var_list.add(new CacheFragment.VarInfo(0, 2, true));
        assertEquals(expected_var_list, fragment.varInfoList);
    }

    @Test
    void case1bTest1() {
        /* p(?, ?, ?) */
        CacheFragment fragment = new CacheFragment(KB.getRelation(NUM_P), NUM_P);

        /* p(X, ?, ?) */
        fragment.updateCase1a(0, 0, 0);

        /* Copy and: p(X, ?, ?), q(?, X, ?) */
        fragment.buildIndices();
        CacheFragment fragment2 = new CacheFragment(fragment);
        fragment2.updateCase1b(KB.getRelation(NUM_Q), NUM_Q, 1, 0);
        List<List<CB>> expected_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1},
                                new int[]{1, 1, 2},
                                new int[]{1, 2, 3},
                                new int[]{1, 3, 2}
                        }), new CB(new int[][]{
                                new int[]{1, 1, 2},
                                new int[]{3, 1, 4},
                                new int[]{5, 1, 4}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{4, 4, 6}
                        }), new CB(new int[][]{
                                new int[]{2, 4, 1}
                        })
                )
        );
        checkEntries(expected_entries, fragment2);

        /* p(X, X, ?), q(?, X, ?) */
        fragment2.buildIndices();
        fragment2.updateCase1a(0, 1, 0);
        expected_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1},
                                new int[]{1, 1, 2},
                        }), new CB(new int[][]{
                                new int[]{1, 1, 2},
                                new int[]{3, 1, 4},
                                new int[]{5, 1, 4}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{4, 4, 6}
                        }), new CB(new int[][]{
                                new int[]{2, 4, 1}
                        })
                )
        );
        checkEntries(expected_entries, fragment2);
        assertEquals("p(X0,X0,?),q(?,X0,?)", rule2String(fragment2.partAssignedRule));
        assertEquals(new ArrayList<>(List.of(
                new CacheFragment.VarInfo(0, 0, true)
        )), fragment.varInfoList);
    }

    @Test
    void case1cTest1() {
        /* F1: p(?, ?, ?) */
        /* F2: p(?, ?, ?) */
        CacheFragment fragment1 = new CacheFragment(KB.getRelation(NUM_P), NUM_P);
        CacheFragment fragment2 = new CacheFragment(KB.getRelation(NUM_P), NUM_P);

        /* F1: p(X, ?, ?) */
        fragment1.updateCase1a(0, 0, 0);

        /* F2: p(W, W, ?) */
        fragment2.updateCase1a(0, 0, 3);
        fragment2.buildIndices();
        fragment2.updateCase1a(0, 1, 3);

        /* F1 + F2: p(X, ?, ?), p(W, W, X) */
        fragment1.buildIndices();
        fragment2.buildIndices();
        fragment1.updateCase1c(fragment2, 0, 2, 0);
        List<List<CB>> expected_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1},
                                new int[]{1, 1, 2},
                                new int[]{1, 2, 3},
                                new int[]{1, 3, 2}
                        }), new CB(new int[][]{
                                new int[]{1, 1, 1}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{2, 1, 3},
                                new int[]{2, 4, 4}
                        }), new CB(new int[][]{
                                new int[]{1, 1, 2}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1},
                                new int[]{1, 1, 2},
                                new int[]{1, 2, 3},
                                new int[]{1, 3, 2}
                        }), new CB(new int[][]{
                                new int[]{5, 5, 1}
                        })
                )
        );
        checkEntries(expected_entries, fragment1);
        assertEquals("p(X0,?,?),p(X3,X3,X0)", rule2String(fragment1.partAssignedRule));
        List<CacheFragment.VarInfo> expected_var_list = new ArrayList<>();
        expected_var_list.add(new CacheFragment.VarInfo(0, 0, false));
        expected_var_list.add(null);
        expected_var_list.add(null);
        expected_var_list.add(new CacheFragment.VarInfo(1, 0, false));
        assertEquals(expected_var_list, fragment1.varInfoList);

        expected_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1},
                                new int[]{1, 1, 2}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{4, 4, 6}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{5, 5, 1}
                        })
                )
        );
        checkEntries(expected_entries, fragment2);
        assertEquals("p(X3,X3,?)", rule2String(fragment2.partAssignedRule));
        expected_var_list = new ArrayList<>();
        expected_var_list.add(null);
        expected_var_list.add(null);
        expected_var_list.add(null);
        expected_var_list.add(new CacheFragment.VarInfo(0, 0, false));
        assertEquals(expected_var_list, fragment2.varInfoList);
    }

    @Test
    void case1cTest2() {
        /* F1: p(?, ?, ?) */
        /* F2: p(?, ?, ?) */
        CacheFragment fragment1 = new CacheFragment(KB.getRelation(NUM_P), NUM_P);
        CacheFragment fragment2 = new CacheFragment(KB.getRelation(NUM_P), NUM_P);

        /* F1: p(X, X, ?) */
        fragment1.updateCase1a(0, 0, 0);
        fragment1.buildIndices();
        fragment1.updateCase1a(0, 1, 0);

        /* F2: p(Y, Y, ?) */
        fragment2.updateCase1a(0, 0, 1);
        fragment2.buildIndices();
        fragment2.updateCase1a(0, 1, 1);

        /* F1 + F2: p(X, X, ?), q(Y, Y, X) */
        fragment1.buildIndices();
        fragment2.buildIndices();
        fragment1.updateCase1c(fragment2, 0, 2, 0);
        List<List<CB>> expected_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1},
                                new int[]{1, 1, 2},
                        }), new CB(new int[][]{
                                new int[]{1, 1, 1}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1},
                                new int[]{1, 1, 2},
                        }), new CB(new int[][]{
                                new int[]{5, 5, 1}
                        })
                )
        );
        checkEntries(expected_entries, fragment1);
        assertEquals("p(X0,X0,?),p(X1,X1,X0)", rule2String(fragment1.partAssignedRule));
        assertEquals(new ArrayList<>(List.of(
                new CacheFragment.VarInfo(0, 0, false),
                new CacheFragment.VarInfo(1, 0, false)
        )), fragment1.varInfoList);
    }

    @Test
    void case2aTest1() {
        /* p(?, ?, ?) */
        CacheFragment fragment = new CacheFragment(KB.getRelation(NUM_P), NUM_P);

        /* p(Y, Y, ?) */
        fragment.updateCase2a(0, 0, 0, 1, 1);
        List<List<CB>> expected_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1},
                                new int[]{1, 1, 2}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{4, 4, 6}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{5, 5, 1}
                        })
                )
        );
        checkEntries(expected_entries, fragment);
        assertEquals("p(X1,X1,?)", rule2String(fragment.partAssignedRule));
        List<CacheFragment.VarInfo> expected_var_list = new ArrayList<>();
        expected_var_list.add(null);
        expected_var_list.add(new CacheFragment.VarInfo(0, 0, false));
        assertEquals(expected_var_list, fragment.varInfoList);

        /* p(Y, Y, Y) */
        fragment.buildIndices();
        fragment.updateCase1a(0, 2, 1);
        expected_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1}
                        })
                )
        );
        checkEntries(expected_entries, fragment);
        assertEquals("p(X1,X1,X1)", rule2String(fragment.partAssignedRule));
        expected_var_list = new ArrayList<>();
        expected_var_list.add(null);
        expected_var_list.add(new CacheFragment.VarInfo(0, 0, false));
        assertEquals(expected_var_list, fragment.varInfoList);
    }

    @Test
    void case2aTest2() {
        /* p(?, ?, ?) */
        CacheFragment fragment = new CacheFragment(KB.getRelation(NUM_P), NUM_P);

        /* p(?, X, X) */
        fragment.updateCase2a(0, 1, 0, 2, 0);
        List<List<CB>> expected_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1},
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{2, 4, 4}
                        })
                )
        );
        checkEntries(expected_entries, fragment);
        assertEquals("p(?,X0,X0)", rule2String(fragment.partAssignedRule));
        List<CacheFragment.VarInfo> expected_var_list = new ArrayList<>();
        expected_var_list.add(new CacheFragment.VarInfo(0, 1, false));
        assertEquals(expected_var_list, fragment.varInfoList);

        /* p(X, X, X) */
        fragment.buildIndices();
        fragment.updateCase1a(0, 0, 0);
        expected_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1}
                        })
                )
        );
        checkEntries(expected_entries, fragment);
        assertEquals("p(X0,X0,X0)", rule2String(fragment.partAssignedRule));
        expected_var_list = new ArrayList<>();
        expected_var_list.add(new CacheFragment.VarInfo(0, 1, false));
        assertEquals(expected_var_list, fragment.varInfoList);
    }

    @Test
    void case2bTest1() {
        /* p(?, ?, ?) */
        CacheFragment fragment = new CacheFragment(KB.getRelation(NUM_P), NUM_P);

        /* p(X, ?, ?), q(X, ?, ?) */
        fragment.updateCase2b(KB.getRelation(NUM_Q), NUM_Q, 0, 0, 0, 0);
        List<List<CB>> expected_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1},
                                new int[]{1, 1, 2},
                                new int[]{1, 2, 3},
                                new int[]{1, 3, 2}
                        }), new CB(new int[][]{
                                new int[]{1, 1, 2}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{2, 1, 3},
                                new int[]{2, 4, 4},
                        }), new CB(new int[][]{
                                new int[]{2, 4, 1},
                                new int[]{2, 7, 8},
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{5, 5, 1}
                        }), new CB(new int[][]{
                                new int[]{5, 1, 4}
                        })
                )
        );
        checkEntries(expected_entries, fragment);
        assertEquals("p(X0,?,?),q(X0,?,?)", rule2String(fragment.partAssignedRule));
        assertEquals(new ArrayList<>(List.of(
                new CacheFragment.VarInfo(0, 0, false)
        )), fragment.varInfoList);

        /* p(X, X, ?), q(X, ?, ?) */
        fragment.buildIndices();
        fragment.updateCase1a(0, 1, 0);
        expected_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1},
                                new int[]{1, 1, 2},
                        }), new CB(new int[][]{
                                new int[]{1, 1, 2}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{5, 5, 1}
                        }), new CB(new int[][]{
                                new int[]{5, 1, 4}
                        })
                )
        );
        checkEntries(expected_entries, fragment);
        assertEquals("p(X0,X0,?),q(X0,?,?)", rule2String(fragment.partAssignedRule));
        assertEquals(new ArrayList<>(List.of(
                new CacheFragment.VarInfo(0, 0, false)
        )), fragment.varInfoList);
    }

    @Test
    void case2bTest2() {
        /* p(?, ?, ?) */
        CacheFragment fragment = new CacheFragment(KB.getRelation(NUM_P), NUM_P);

        /* p(X, ?, ?) */
        fragment.updateCase1a(0, 0, 0);

        /* p(X, Y, ?), q(?, ?, Y) */
        fragment.buildIndices();
        fragment.updateCase2b(KB.getRelation(NUM_Q), NUM_Q, 2, 0, 1, 1);
        List<List<CB>> expected_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1},
                                new int[]{1, 1, 2},
                                new int[]{2, 1, 3},
                        }), new CB(new int[][]{
                                new int[]{2, 4, 1}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{1, 2, 3}
                        }), new CB(new int[][]{
                                new int[]{1, 1, 2},
                                new int[]{6, 7, 2}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{4, 4, 6},
                                new int[]{2, 4, 4}
                        }), new CB(new int[][]{
                                new int[]{5, 1, 4},
                                new int[]{3, 1, 4}
                        })
                )
        );
        checkEntries(expected_entries, fragment);
        assertEquals("p(X0,X1,?),q(?,?,X1)", rule2String(fragment.partAssignedRule));
        assertEquals(new ArrayList<>(List.of(
                new CacheFragment.VarInfo(0, 0, true),
                new CacheFragment.VarInfo(0, 1, false)
        )), fragment.varInfoList);

        /* p(X, Y, ?), q(?, Z, Y), q(?, Z, ?) */
        fragment.buildIndices();
        fragment.updateCase2b(KB.getRelation(NUM_Q), NUM_Q, 1, 1, 1, 2);
        expected_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1},
                                new int[]{1, 1, 2},
                                new int[]{2, 1, 3},
                        }), new CB(new int[][]{
                                new int[]{2, 4, 1}
                        }), new CB(new int[][]{
                                new int[]{2, 4, 1}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{1, 2, 3}
                        }), new CB(new int[][]{
                                new int[]{1, 1, 2},
                        }), new CB(new int[][]{
                                new int[]{1, 1, 2},
                                new int[]{3, 1, 4},
                                new int[]{5, 1, 4}
                        })
                ),List.of(
                        new CB(new int[][]{
                                new int[]{1, 2, 3}
                        }), new CB(new int[][]{
                                new int[]{6, 7, 2}
                        }), new CB(new int[][]{
                                new int[]{2, 7, 8},
                                new int[]{6, 7, 2}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{4, 4, 6},
                                new int[]{2, 4, 4}
                        }), new CB(new int[][]{
                                new int[]{5, 1, 4},
                                new int[]{3, 1, 4}
                        }), new CB(new int[][]{
                                new int[]{1, 1, 2},
                                new int[]{3, 1, 4},
                                new int[]{5, 1, 4}
                        })
                )
        );
        checkEntries(expected_entries, fragment);
        assertEquals("p(X0,X1,?),q(?,X2,X1),q(?,X2,?)", rule2String(fragment.partAssignedRule));
        assertEquals(new ArrayList<>(List.of(
                new CacheFragment.VarInfo(0, 0, true),
                new CacheFragment.VarInfo(0, 1, false),
                new CacheFragment.VarInfo(1, 1, false)
        )), fragment.varInfoList);
    }

    @Test
    void case2cTest1() {
        /* F1: p(?, ?, ?) */
        /* F2: q(?, ?, ?) */
        CacheFragment fragment1 = new CacheFragment(KB.getRelation(NUM_P), NUM_P);
        CacheFragment fragment2 = new CacheFragment(KB.getRelation(NUM_Q), NUM_Q);

        /* F1 + F2: p(X, ?, ?), q(X, ?, ?) */
        fragment1.updateCase2c(0, 0, fragment2, 0, 0, 0);
        List<List<CB>> expected_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1},
                                new int[]{1, 1, 2},
                                new int[]{1, 2, 3},
                                new int[]{1, 3, 2}
                        }), new CB(new int[][]{
                                new int[]{1, 1, 2}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{2, 1, 3},
                                new int[]{2, 4, 4},
                        }), new CB(new int[][]{
                                new int[]{2, 4, 1},
                                new int[]{2, 7, 8},
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{5, 5, 1}
                        }), new CB(new int[][]{
                                new int[]{5, 1, 4}
                        })
                )
        );
        checkEntries(expected_entries, fragment1);
        assertEquals("p(X0,?,?),q(X0,?,?)", rule2String(fragment1.partAssignedRule));
        assertEquals(new ArrayList<>(List.of(
                new CacheFragment.VarInfo(0, 0, false)
        )), fragment1.varInfoList);
    }

    @Test
    void case2cTest2() {
        /* F1: p(?, ?, ?) */
        /* F2: q(?, ?, ?) */
        CacheFragment fragment1 = new CacheFragment(KB.getRelation(NUM_P), NUM_P);
        CacheFragment fragment2 = new CacheFragment(KB.getRelation(NUM_Q), NUM_Q);

        /* F1: p(X, X, ?) */
        fragment1.updateCase2a(0, 0, 0, 1, 0);

        /* F2: q(?, Y, ?) */
        fragment2.updateCase1a(0, 1, 1);

        /* F1 + F2: p(X, X, Z), q(Z, Y, ?) */
        fragment1.buildIndices();
        fragment2.buildIndices();
        fragment1.updateCase2c(0, 2, fragment2, 0, 0, 2);
        List<List<CB>> expected_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1},
                        }), new CB(new int[][]{
                                new int[]{1, 1, 2}
                        })
                ),List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 2},
                        }), new CB(new int[][]{
                                new int[]{2, 4, 1},
                                new int[]{2, 7, 8}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{4, 4, 6}
                        }), new CB(new int[][]{
                                new int[]{6, 7, 2}
                        })
                ), List.of(
                        new CB(new int[][]{
                                new int[]{5, 5, 1}
                        }), new CB(new int[][]{
                                new int[]{1, 1, 2}
                        })
                )
        );
        checkEntries(expected_entries, fragment1);
        assertEquals("p(X0,X0,X2),q(X2,X1,?)", rule2String(fragment1.partAssignedRule));
        assertEquals(new ArrayList<>(List.of(
                new CacheFragment.VarInfo(0, 0, false),
                new CacheFragment.VarInfo(1, 1, true),
                new CacheFragment.VarInfo(0, 2, false)
        )), fragment1.varInfoList);
    }

    @Test
    void case3Test1() {
        /* p(?, ?, ?) */
        CacheFragment fragment = new CacheFragment(KB.getRelation(NUM_P), NUM_P);

        /* p(1, ?, ?) */
        fragment.updateCase3(0, 0, 1);
        List<List<CB>> expected_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[]{1, 1, 1},
                                new int[]{1, 1, 2},
                                new int[]{1, 2, 3},
                                new int[]{1, 3, 2}
                        })
                )
        );
        checkEntries(expected_entries, fragment);
        assertEquals("p(1,?,?)", rule2String(fragment.partAssignedRule));
        assertTrue(fragment.varInfoList.isEmpty());
    }

    @Test
    void case3Test2() {
        /* p(?, ?, ?) */
        CacheFragment fragment = new CacheFragment(KB.getRelation(NUM_P), NUM_P);

        /* p(X, ?, ?), q(X, ?, ?) */
        fragment.updateCase2b(KB.getRelation(NUM_Q), NUM_Q, 0, 0, 0, 0);

        /* p(X, ?, ?), q(X, ?, 8) */
        fragment.buildIndices();
        fragment.updateCase3(1, 2, 8);
        List<List<CB>> expected_entries = List.of(
                List.of(
                        new CB(new int[][]{
                                new int[]{2, 1, 3},
                                new int[]{2, 4, 4},
                        }), new CB(new int[][]{
                                new int[]{2, 7, 8},
                        })
                )
        );
        checkEntries(expected_entries, fragment);
        assertEquals("p(X0,?,?),q(X0,?,8)", rule2String(fragment.partAssignedRule));
        assertEquals(new ArrayList<>(List.of(
                new CacheFragment.VarInfo(0, 0, false)
        )), fragment.varInfoList);
    }

    void checkEntries(List<List<CB>> expectedEntries, CacheFragment actualFragment) {
        assertEquals(expectedEntries.size(), actualFragment.entries.size());
        assertEquals(expectedEntries.get(0).size(), actualFragment.partAssignedRule.size());
        for (int i = 0; i < actualFragment.partAssignedRule.size(); i++) {
            assertEquals(
                    expectedEntries.get(0).get(i).complianceSet[0].length,
                    actualFragment.entries.get(0).get(i).complianceSet[0].length
            );
        }
        for (List<CB> expected_entry: expectedEntries) {
            for (CB cb : expected_entry) {
                cb.buildIndices();
            }
        }
        for (List<CB> actual_entry: actualFragment.entries) {
            for (CB cb: actual_entry) {
                cb.buildIndices();
            }
        }
        for (List<CB> expected_entry: expectedEntries) {
            boolean found = false;
            for (List<CB> actual_entry: actualFragment.entries) {
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

    String rule2String(List<Predicate> rule) {
        StringBuilder builder = new StringBuilder(rule.get(0).toString(KB));
        for (int i = 1; i < rule.size(); i++) {
            builder.append(',').append(rule.get(i).toString(KB));
        }
        return builder.toString();
    }

    @Test
    void testCountTableSize1() {
        /* p(?, ?, ?) */
        CacheFragment fragment = new CacheFragment(KB.getRelation(NUM_P), NUM_P);
        assertEquals(8, fragment.countTableSize(0));

        /* p(1, ?, ?) */
        fragment.updateCase3(0, 0, 1);
        assertEquals(4, fragment.countTableSize(0));
    }

    @Test
    void testCountTableSize2() {
        /* p(?, ?, ?) */
        CacheFragment fragment = new CacheFragment(KB.getRelation(NUM_P), NUM_P);

        /* p(X, ?, ?) */
        fragment.updateCase1a(0, 0, 0);

        /* p(X, Y, ?), q(?, ?, Y) */
        fragment.buildIndices();
        fragment.updateCase2b(KB.getRelation(NUM_Q), NUM_Q, 2, 0, 1, 1);

        /* p(X, Y, ?), q(?, Z, Y), q(?, Z, ?) */
        fragment.buildIndices();
        fragment.updateCase2b(KB.getRelation(NUM_Q), NUM_Q, 1, 1, 1, 2);
        assertEquals(6, fragment.countTableSize(2));
//        expected_entries = List.of(
//                List.of(
//                        new CB(new int[][]{
//                                new int[]{1, 1, 1},
//                                new int[]{1, 1, 2},
//                                new int[]{2, 1, 3},
//                        }), new CB(new int[][]{
//                                new int[]{2, 4, 1}
//                        }), new CB(new int[][]{
//                                new int[]{2, 4, 1}
//                        })
//                ), List.of(
//                        new CB(new int[][]{
//                                new int[]{1, 2, 3}
//                        }), new CB(new int[][]{
//                                new int[]{1, 1, 2},
//                        }), new CB(new int[][]{
//                                new int[]{1, 1, 2},
//                                new int[]{3, 1, 4},
//                                new int[]{5, 1, 4}
//                        })
//                ),List.of(
//                        new CB(new int[][]{
//                                new int[]{1, 2, 3}
//                        }), new CB(new int[][]{
//                                new int[]{6, 7, 2}
//                        }), new CB(new int[][]{
//                                new int[]{2, 7, 8},
//                                new int[]{6, 7, 2}
//                        })
//                ), List.of(
//                        new CB(new int[][]{
//                                new int[]{4, 4, 6},
//                                new int[]{2, 4, 4}
//                        }), new CB(new int[][]{
//                                new int[]{5, 1, 4},
//                                new int[]{3, 1, 4}
//                        }), new CB(new int[][]{
//                                new int[]{1, 1, 2},
//                                new int[]{3, 1, 4},
//                                new int[]{5, 1, 4}
//                        })
//                )
//        );
    }

    @Test
    void testCountCombinations1() {
        /* p(X, ?, ?) [X] */
        CacheFragment fragment = new CacheFragment(KB.getRelation(NUM_P), NUM_P);
        fragment.updateCase1a(0, 0, 0);
        assertEquals("p(X0,?,?)", rule2String(fragment.partAssignedRule));
        assertEquals(4, fragment.countCombinations(new int[]{0}));
    }

    @Test
    void testCountCombinations2() {
        /* p(X, Y, ?) [X,Y] */
        CacheFragment fragment = new CacheFragment(KB.getRelation(NUM_P), NUM_P);
        fragment.updateCase1a(0, 0, 0);
        fragment.buildIndices();
        fragment.updateCase1a(0, 1, 1);
        assertEquals("p(X0,X1,?)", rule2String(fragment.partAssignedRule));
        assertEquals(7, fragment.countCombinations(new int[]{0, 1}));
    }

    @Test
    void testCountCombinations3() {
        /* p(Y, Y, ?) [Y] */
        CacheFragment fragment = new CacheFragment(KB.getRelation(NUM_P), NUM_P);
        fragment.updateCase2a(0, 0, 0, 1, 1);
        assertEquals("p(X1,X1,?)", rule2String(fragment.partAssignedRule));
        assertEquals(3, fragment.countCombinations(new int[]{1}));
    }

    @Test
    void testCountCombinations4() {
        /* p(X, Y, ?), q(?, Z, Y), q(?, Z, ?) [Y, Z] */
        CacheFragment fragment = new CacheFragment(KB.getRelation(NUM_P), NUM_P);
        fragment.updateCase1a(0, 0, 0);
        fragment.buildIndices();
        fragment.updateCase2b(KB.getRelation(NUM_Q), NUM_Q, 2, 0, 1, 1);
        fragment.buildIndices();
        fragment.updateCase2b(KB.getRelation(NUM_Q), NUM_Q, 1, 1, 1, 2);
        assertEquals("p(X0,X1,?),q(?,X2,X1),q(?,X2,?)", rule2String(fragment.partAssignedRule));
        assertEquals(4, fragment.countCombinations(new int[]{1, 2}));
    }

    @Test
    void testCountCombinations5() {
        /* p(X, Y, ?), q(?, Z, Y), q(?, Z, ?) [X, Z] */
        CacheFragment fragment = new CacheFragment(KB.getRelation(NUM_P), NUM_P);
        fragment.updateCase1a(0, 0, 0);
        fragment.buildIndices();
        fragment.updateCase2b(KB.getRelation(NUM_Q), NUM_Q, 2, 0, 1, 1);
        fragment.buildIndices();
        fragment.updateCase2b(KB.getRelation(NUM_Q), NUM_Q, 1, 1, 1, 2);
        assertEquals("p(X0,X1,?),q(?,X2,X1),q(?,X2,?)", rule2String(fragment.partAssignedRule));
        assertEquals(6, fragment.countCombinations(new int[]{0, 2}));
    }
}