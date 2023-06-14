package sinc2.impl.negsamp;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import sinc2.common.ArgLocation;
import sinc2.common.Argument;
import sinc2.common.Predicate;
import sinc2.common.Record;
import sinc2.kb.KbException;
import sinc2.kb.SimpleKb;
import sinc2.kb.SimpleRelation;
import sinc2.rule.*;
import sinc2.util.ComparableArray;
import sinc2.util.MultiSet;
import sinc2.util.kb.NumerationMap;

import java.io.IOException;
import java.util.*;

import static org.junit.jupiter.api.Assertions.*;

class FragmentedFragmentedCachedRuleTest {

    static final String TEST_DIR = "/dev/shm";
    static final String KB_NAME = UUID.randomUUID().toString();
    static final int NUM_FATHER;
    static final int NUM_PARENT;
    static final int NUM_GRANDPARENT;
    static final int NUM_G1 = 1;
    static final int NUM_G2 = 2;
    static final int NUM_G3 = 3;
    static final int NUM_G4 = 4;
    static final int NUM_F1 = 5;
    static final int NUM_F2 = 6;
    static final int NUM_F3 = 7;
    static final int NUM_F4 = 8;
    static final int NUM_M2 = 9;
    static final int NUM_S1 = 10;
    static final int NUM_S2 = 11;
    static final int NUM_S3 = 12;
    static final int NUM_S4 = 13;
    static final int NUM_D1 = 14;
    static final int NUM_D2 = 15;
    static final int NUM_D4 = 16;
    static final Record FATHER1 = new Record(new int[]{NUM_F1, NUM_S1});
    static final Record FATHER2 = new Record(new int[]{NUM_F2, NUM_S2});
    static final Record FATHER3 = new Record(new int[]{NUM_F2, NUM_D2});
    static final Record FATHER4 = new Record(new int[]{NUM_F3, NUM_S3});
    static final Record FATHER5 = new Record(new int[]{NUM_F4, NUM_D4});
    static final Record PARENT1 = new Record(new int[]{NUM_F1, NUM_S1});
    static final Record PARENT2 = new Record(new int[]{NUM_F1, NUM_D1});
    static final Record PARENT3 = new Record(new int[]{NUM_F2, NUM_S2});
    static final Record PARENT4 = new Record(new int[]{NUM_F2, NUM_D2});
    static final Record PARENT5 = new Record(new int[]{NUM_M2, NUM_D2});
    static final Record PARENT6 = new Record(new int[]{NUM_G1, NUM_F1});
    static final Record PARENT7 = new Record(new int[]{NUM_G2, NUM_F2});
    static final Record PARENT8 = new Record(new int[]{NUM_G2, NUM_M2});
    static final Record PARENT9 = new Record(new int[]{NUM_G3, NUM_F3});
    static final Record GRAND1 = new Record(new int[]{NUM_G1, NUM_S1});
    static final Record GRAND2 = new Record(new int[]{NUM_G2, NUM_D2});
    static final Record GRAND3 = new Record(new int[]{NUM_G4, NUM_S4});
    static final SimpleKb KB;
    static final NumerationMap MAP;
    static {
        MAP = new NumerationMap();
        assertEquals(NUM_G1, MAP.mapName("g1"));
        assertEquals(NUM_G2, MAP.mapName("g2"));
        assertEquals(NUM_G3, MAP.mapName("g3"));
        assertEquals(NUM_G4, MAP.mapName("g4"));
        assertEquals(NUM_F1, MAP.mapName("f1"));
        assertEquals(NUM_F2, MAP.mapName("f2"));
        assertEquals(NUM_F3, MAP.mapName("f3"));
        assertEquals(NUM_F4, MAP.mapName("f4"));
        assertEquals(NUM_M2, MAP.mapName("m2"));
        assertEquals(NUM_S1, MAP.mapName("s1"));
        assertEquals(NUM_S2, MAP.mapName("s2"));
        assertEquals(NUM_S3, MAP.mapName("s3"));
        assertEquals(NUM_S4, MAP.mapName("s4"));
        assertEquals(NUM_D1, MAP.mapName("d1"));
        assertEquals(NUM_D2, MAP.mapName("d2"));
        assertEquals(NUM_D4, MAP.mapName("d4"));

        /* father(X, Y):
         *   f1, s1
         *   f2, s2
         *   f2, d2
         *   f3, s3
         *   f4, d4
         */
        int[][] rel_father = new int[][]{
                FATHER1.args, FATHER2.args, FATHER3.args, FATHER4.args, FATHER5.args
        };

        /* parent(X, Y):
         *   f1, s1
         *   f1, d1
         *   f2, s2
         *   f2, d2
         *   m2, d2
         *   g1, f1
         *   g2, f2
         *   g2, m2
         *   g3, f3
         */
        int[][] rel_parent = new int[][]{
                PARENT1.args, PARENT2.args, PARENT3.args, PARENT4.args, PARENT5.args, PARENT6.args, PARENT7.args,
                PARENT8.args, PARENT9.args
        };

        /* grandParent(X, Y):
         *   g1, s1
         *   g2, d2
         *   g4, s4
         */
        int[][] rel_grand = new int[][]{
                GRAND1.args, GRAND2.args, GRAND3.args
        };

        /* Constants(16):
         *   g1, g2, g3, g4
         *   f1, f2, f3, f4
         *   m2
         *   s1, s2, s3, s4
         *   d1, d2, d4
         */
        KB = new SimpleKb(
                KB_NAME,
                new int[][][]{rel_father, rel_parent, rel_grand},
                new String[]{"father", "parent", "grandParent"}
        );
        NUM_FATHER = KB.getRelation("father").id;
        NUM_PARENT = KB.getRelation("parent").id;
        NUM_GRANDPARENT = KB.getRelation("grandParent").id;
    }

    static SimpleKb kbFamily() {
        int[][][] relations = new int[KB.totalRelations()][][];
        String[] rel_names = new String[KB.totalRelations()];
        for (int i = 0; i < relations.length; i++) {
            SimpleRelation relation = KB.getRelation(i);
            relations[i] = relation.getAllRows().clone();
            rel_names[i] = relation.name;
        }
        return new SimpleKb(KB.getName(), relations, rel_names);
    }

    @BeforeEach
    void setParameters() {
        Rule.MIN_FACT_COVERAGE = -1.0;
    }

    @Test
    void testFamilyRule1()  {
        final SimpleKb kb = kbFamily();
        final Set<Fingerprint> fp_cache = new HashSet<>();
        final Map<MultiSet<Integer>, Set<Fingerprint>> tabu_map = new HashMap<>();

        /* parent(?, ?) :- */
        final FragmentedCachedRule rule = new FragmentedCachedRule(NUM_PARENT, 2, fp_cache, tabu_map, kb);
        assertEquals("parent(?,?):-", rule.toDumpString(KB));
        assertEquals(
                new Eval(9, 16 * 16, 0),
                rule.getEval()
        );
        assertEquals(0, rule.usedLimitedVars());
        assertEquals(0, rule.length());
        assertEquals(1, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* parent(X, ?) :- father(X, ?) */
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(NUM_FATHER, 2, 0, 0, 0));
        assertEquals("parent(X0,?):-father(X0,?)", rule.toDumpString(KB));
        assertEquals(
                new Eval(4, 4 * 16, 1),
                rule.getEval()
        );
        assertEquals(1, rule.usedLimitedVars());
        assertEquals(1, rule.length());
        assertEquals(2, fp_cache.size());
        assertEquals(0, tabu_map.size());
        final Set<ComparableArray<Record>> expected_grounding_set1 = new HashSet<>();
        expected_grounding_set1.add(new ComparableArray<>(new Record[]{PARENT1, FATHER1}));
        expected_grounding_set1.add(new ComparableArray<>(new Record[]{PARENT2, FATHER1}));
        expected_grounding_set1.add(new ComparableArray<>(new Record[]{PARENT3, FATHER2}));
        expected_grounding_set1.add(new ComparableArray<>(new Record[]{PARENT4, FATHER2}));
        final Set<ComparableArray<Record>> expected_grounding_set2 = new HashSet<>();
        expected_grounding_set2.add(new ComparableArray<>(new Record[]{PARENT1, FATHER1}));
        expected_grounding_set2.add(new ComparableArray<>(new Record[]{PARENT2, FATHER1}));
        expected_grounding_set2.add(new ComparableArray<>(new Record[]{PARENT3, FATHER2}));
        expected_grounding_set2.add(new ComparableArray<>(new Record[]{PARENT4, FATHER3}));
        final Set<ComparableArray<Record>> expected_grounding_set3 = new HashSet<>();
        expected_grounding_set3.add(new ComparableArray<>(new Record[]{PARENT1, FATHER1}));
        expected_grounding_set3.add(new ComparableArray<>(new Record[]{PARENT2, FATHER1}));
        expected_grounding_set3.add(new ComparableArray<>(new Record[]{PARENT3, FATHER3}));
        expected_grounding_set3.add(new ComparableArray<>(new Record[]{PARENT4, FATHER2}));
        final Set<ComparableArray<Record>> expected_grounding_set4 = new HashSet<>();
        expected_grounding_set4.add(new ComparableArray<>(new Record[]{PARENT1, FATHER1}));
        expected_grounding_set4.add(new ComparableArray<>(new Record[]{PARENT2, FATHER1}));
        expected_grounding_set4.add(new ComparableArray<>(new Record[]{PARENT3, FATHER3}));
        expected_grounding_set4.add(new ComparableArray<>(new Record[]{PARENT4, FATHER3}));
        final Set<Record> expected_counter_examples = new HashSet<>();
        for (int arg1: new int[]{NUM_F1, NUM_F2, NUM_F3, NUM_F4}) {
            for (int arg2 = 1; arg2 <= kb.totalConstants(); arg2++) {
                expected_counter_examples.add(new Record(new int[]{arg1, arg2}));
            }
        }
        expected_counter_examples.remove(PARENT1);
        expected_counter_examples.remove(PARENT2);
        expected_counter_examples.remove(PARENT3);
        expected_counter_examples.remove(PARENT4);
        EvidenceBatch actual_evidence = rule.getEvidenceAndMarkEntailment();
        checkEvidence(actual_evidence, new int[]{NUM_PARENT, NUM_FATHER}, new Set[]{
                expected_grounding_set1, expected_grounding_set2, expected_grounding_set3, expected_grounding_set4
        });
        assertEquals(expected_counter_examples, rule.getCounterexamples());

        /* parent(X, Y) :- father(X, Y) */
        fp_cache.clear();
        final FragmentedCachedRule rule2 = new FragmentedCachedRule(NUM_PARENT, 2, fp_cache, tabu_map, kb);
        rule2.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule2.cvt2Uvs2NewLv(NUM_FATHER, 2, 0, 0, 0));
        rule2.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule2.cvt2Uvs2NewLv(0, 1, 1, 1));
        assertEquals("parent(X0,X1):-father(X0,X1)", rule2.toDumpString(KB));
        assertEquals(
                new Eval(0, 2, 2),
                rule2.getEval()
        );
        assertEquals(2, rule2.usedLimitedVars());
        assertEquals(2, rule2.length());
        assertEquals(3, fp_cache.size());
        assertEquals(0, tabu_map.size());
        actual_evidence = rule2.getEvidenceAndMarkEntailment();
        assertArrayEquals(new int[]{NUM_PARENT, NUM_FATHER}, actual_evidence.predicateSymbolsInRule);
        assertTrue(actual_evidence.evidenceList.isEmpty());
        Record counter1 = new Record(new int[]{NUM_F3, NUM_S3});
        Record counter2 = new Record(new int[]{NUM_F4, NUM_D4});
        assertEquals(new HashSet<>(List.of(counter1, counter2)), rule2.getCounterexamples());
    }

    @Test
    void testFamilyRule2()  {
        final SimpleKb kb = kbFamily();
        final Set<Fingerprint> fp_cache = new HashSet<>();
        final Map<MultiSet<Integer>, Set<Fingerprint>> tabu_map = new HashMap<>();

        /* parent(?, ?) :- */
        final FragmentedCachedRule rule = new FragmentedCachedRule(NUM_PARENT, 2, fp_cache, tabu_map, kb);
        assertEquals("parent(?,?):-", rule.toDumpString(KB));
        assertEquals(
                new Eval(9, 16 * 16, 0),
                rule.getEval()
        );
        assertEquals(0, rule.usedLimitedVars());
        assertEquals(0, rule.length());
        assertEquals(1, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* parent(?, X) :- father(?, X) */
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(NUM_FATHER, 2, 1, 0, 1));
        assertEquals("parent(?,X0):-father(?,X0)", rule.toDumpString(KB));
        assertEquals(
                new Eval(4, 5 * 16, 1),
                rule.getEval()
        );
        assertEquals(1, rule.usedLimitedVars());
        assertEquals(1, rule.length());
        assertEquals(2, fp_cache.size());
        assertEquals(0, tabu_map.size());
        Set<ComparableArray<Record>> expected_grounding_set = new HashSet<>();
        expected_grounding_set.add(new ComparableArray<>(new Record[]{PARENT1, FATHER1}));
        expected_grounding_set.add(new ComparableArray<>(new Record[]{PARENT3, FATHER2}));
        expected_grounding_set.add(new ComparableArray<>(new Record[]{PARENT4, FATHER3}));
        expected_grounding_set.add(new ComparableArray<>(new Record[]{PARENT5, FATHER3}));
        Set<Record> expected_counter_examples = new HashSet<>();
        for (int arg1 = 1; arg1 <= kb.totalConstants(); arg1++) {
            for (int arg2: new int[]{NUM_S1, NUM_S2, NUM_D2, NUM_S3, NUM_D4}) {
                expected_counter_examples.add(new Record(new int[]{arg1, arg2}));
            }
        }
        expected_counter_examples.remove(PARENT1);
        expected_counter_examples.remove(PARENT3);
        expected_counter_examples.remove(PARENT4);
        expected_counter_examples.remove(PARENT5);
        EvidenceBatch actual_evidence = rule.getEvidenceAndMarkEntailment();
        checkEvidence(actual_evidence, new int[]{NUM_PARENT, NUM_FATHER}, new Set[]{expected_grounding_set});
        assertEquals(expected_counter_examples, rule.getCounterexamples());

        /* parent(Y, X) :- father(Y, X) */
        fp_cache.clear();
        final FragmentedCachedRule rule2 = new FragmentedCachedRule(NUM_PARENT, 2, fp_cache, tabu_map, kb);
        rule2.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule2.cvt2Uvs2NewLv(NUM_FATHER, 2, 1, 0, 1));
        rule2.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule2.cvt2Uvs2NewLv(0, 0, 1, 0));
        assertEquals("parent(X1,X0):-father(X1,X0)", rule2.toDumpString(KB));
        assertEquals(
                new Eval(0, 2, 2),
                rule2.getEval()
        );
        assertEquals(2, rule2.usedLimitedVars());
        assertEquals(2, rule2.length());
        assertEquals(3, fp_cache.size());
        assertEquals(0, tabu_map.size());
        actual_evidence = rule2.getEvidenceAndMarkEntailment();
        assertArrayEquals(new int[]{NUM_PARENT, NUM_FATHER}, actual_evidence.predicateSymbolsInRule);
        assertTrue(actual_evidence.evidenceList.isEmpty());
        Record counter1 = new Record(new int[]{NUM_F3, NUM_S3});
        Record counter2 = new Record(new int[]{NUM_F4, NUM_D4});
        assertEquals(new HashSet<>(List.of(counter1, counter2)), rule2.getCounterexamples());
    }

    @Test
    void testFamilyRule3()  {
        final SimpleKb kb = kbFamily();
        final Set<Fingerprint> fp_cache = new HashSet<>();
        final Map<MultiSet<Integer>, Set<Fingerprint>> tabu_map = new HashMap<>();

        /* parent(?, ?) :- */
        final FragmentedCachedRule rule = new FragmentedCachedRule(NUM_GRANDPARENT, 2, fp_cache, tabu_map, kb);
        assertEquals("grandParent(?,?):-", rule.toDumpString(KB));
        assertEquals(
                new Eval(3, 16 * 16, 0),
                rule.getEval()
        );
        assertEquals(0, rule.usedLimitedVars());
        assertEquals(0, rule.length());
        assertEquals(1, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* grandParent(X, ?) :- parent(X, ?) */
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(NUM_PARENT, 2, 0, 0, 0));
        assertEquals("grandParent(X0,?):-parent(X0,?)", rule.toDumpString(KB));
        assertEquals(
                new Eval(2, 6 * 16, 1),
                rule.getEval()
        );
        assertEquals(1, rule.usedLimitedVars());
        assertEquals(1, rule.length());
        assertEquals(2, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* grandParent(X, Y) :- parent(X, ?), parent(?, Y) */
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(NUM_PARENT, 2, 1, 0, 1));
        assertEquals("grandParent(X0,X1):-parent(X0,?),parent(?,X1)", rule.toDumpString(KB));
        assertEquals(
                new Eval(2, 6 * 8, 2),
                rule.getEval()
        );
        assertEquals(2, rule.usedLimitedVars());
        assertEquals(2, rule.length());
        assertEquals(3, fp_cache.size());
        assertEquals(0, tabu_map.size());
        final Set<ComparableArray<Record>> expected_grounding_set1 = new HashSet<>();
        expected_grounding_set1.add(new ComparableArray<>(new Record[]{GRAND1, PARENT6, PARENT1}));
        expected_grounding_set1.add(new ComparableArray<>(new Record[]{GRAND2, PARENT7, PARENT4}));
        final Set<ComparableArray<Record>> expected_grounding_set2 = new HashSet<>();
        expected_grounding_set2.add(new ComparableArray<>(new Record[]{GRAND1, PARENT6, PARENT1}));
        expected_grounding_set2.add(new ComparableArray<>(new Record[]{GRAND2, PARENT7, PARENT5}));
        final Set<ComparableArray<Record>> expected_grounding_set3 = new HashSet<>();
        expected_grounding_set3.add(new ComparableArray<>(new Record[]{GRAND1, PARENT6, PARENT1}));
        expected_grounding_set3.add(new ComparableArray<>(new Record[]{GRAND2, PARENT8, PARENT4}));
        final Set<ComparableArray<Record>> expected_grounding_set4 = new HashSet<>();
        expected_grounding_set4.add(new ComparableArray<>(new Record[]{GRAND1, PARENT6, PARENT1}));
        expected_grounding_set4.add(new ComparableArray<>(new Record[]{GRAND2, PARENT8, PARENT5}));
        Set<Record> expected_counter_examples = new HashSet<>();
        for (int arg1: new int[]{NUM_F1, NUM_F2, NUM_M2, NUM_G1, NUM_G2, NUM_G3}) {
            for (int arg2: new int[]{NUM_S1, NUM_S2, NUM_D1, NUM_D2, NUM_F1, NUM_F2, NUM_M2, NUM_F3}) {
                expected_counter_examples.add(new Record(new int[]{arg1, arg2}));
            }
        }
        expected_counter_examples.remove(GRAND1);
        expected_counter_examples.remove(GRAND2);
        EvidenceBatch actual_evidence = rule.getEvidenceAndMarkEntailment();
        checkEvidence(actual_evidence, new int[]{NUM_GRANDPARENT, NUM_PARENT, NUM_PARENT}, new Set[]{
                expected_grounding_set1, expected_grounding_set2, expected_grounding_set3, expected_grounding_set4
        });
        assertEquals(expected_counter_examples, rule.getCounterexamples());

        /* grandParent(X, Y) :- parent(X, Z), parent(Z, Y) */
        fp_cache.clear();
        final FragmentedCachedRule rule2 = new FragmentedCachedRule(NUM_GRANDPARENT, 2, fp_cache, tabu_map, kb);
        rule2.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule2.cvt2Uvs2NewLv(NUM_PARENT, 2, 0, 0, 0));
        rule2.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule2.cvt2Uvs2NewLv(NUM_PARENT, 2, 1, 0, 1));
        rule2.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule2.cvt2Uvs2NewLv(1, 1, 2, 0));
        assertEquals("grandParent(X0,X1):-parent(X0,X2),parent(X2,X1)", rule2.toDumpString(KB));
        assertEquals(
                new Eval(0, 2, 3),
                rule2.getEval()
        );
        assertEquals(3, rule2.usedLimitedVars());
        assertEquals(3, rule2.length());
        assertEquals(4, fp_cache.size());
        assertEquals(0, tabu_map.size());
        actual_evidence = rule2.getEvidenceAndMarkEntailment();
        assertArrayEquals(new int[]{NUM_GRANDPARENT, NUM_PARENT, NUM_PARENT}, actual_evidence.predicateSymbolsInRule);
        assertTrue(actual_evidence.evidenceList.isEmpty());
        Record counter1 = new Record(new int[]{NUM_G1, NUM_D1});
        Record counter2 = new Record(new int[]{NUM_G2, NUM_S2});
        assertEquals(new HashSet<>(List.of(counter1, counter2)), rule2.getCounterexamples());
    }

    @Test
    void testFamilyRule4()  {
        final SimpleKb kb = kbFamily();
        final Set<Fingerprint> fp_cache = new HashSet<>();
        final Map<MultiSet<Integer>, Set<Fingerprint>> tabu_map = new HashMap<>();

        /* parent(?, ?) :- */
        final FragmentedCachedRule rule = new FragmentedCachedRule(NUM_GRANDPARENT, 2, fp_cache, tabu_map, kb);
        assertEquals("grandParent(?,?):-", rule.toDumpString(KB));
        assertEquals(
                new Eval(3, 16 * 16, 0),
                rule.getEval()
        );
        assertEquals(0, rule.usedLimitedVars());
        assertEquals(0, rule.length());
        assertEquals(1, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* grandParent(X, ?) :- parent(X, ?) */
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(NUM_PARENT, 2, 0, 0, 0));
        assertEquals("grandParent(X0,?):-parent(X0,?)", rule.toDumpString(KB));
        assertEquals(
                new Eval(2, 6 * 16, 1),
                rule.getEval()
        );
        assertEquals(1, rule.usedLimitedVars());
        assertEquals(1, rule.length());
        assertEquals(2, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* grandParent(X, ?) :- parent(X, Y), parent(Y, ?) */
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(NUM_PARENT, 2, 0, 1, 1));
        assertEquals("grandParent(X0,?):-parent(X0,X1),parent(X1,?)", rule.toDumpString(KB));
        assertEquals(
                new Eval(2, 2 * 16, 2),
                rule.getEval()
        );
        assertEquals(2, rule.usedLimitedVars());
        assertEquals(2, rule.length());
        assertEquals(3, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* grandParent(X, Z) :- parent(X, Y), parent(Y, Z) */
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(2, 1, 0, 1));
        assertEquals("grandParent(X0,X2):-parent(X0,X1),parent(X1,X2)", rule.toDumpString(KB));
        assertEquals(
                new Eval(2, 4, 3),
                rule.getEval()
        );
        assertEquals(3, rule.usedLimitedVars());
        assertEquals(3, rule.length());
        assertEquals(4, fp_cache.size());
        assertEquals(0, tabu_map.size());
        final Set<ComparableArray<Record>> expected_grounding_set1 = new HashSet<>();
        expected_grounding_set1.add(new ComparableArray<>(new Record[]{GRAND1, PARENT6, PARENT1}));
        expected_grounding_set1.add(new ComparableArray<>(new Record[]{GRAND2, PARENT7, PARENT4}));
        final Set<ComparableArray<Record>> expected_grounding_set2 = new HashSet<>();
        expected_grounding_set2.add(new ComparableArray<>(new Record[]{GRAND1, PARENT6, PARENT1}));
        expected_grounding_set2.add(new ComparableArray<>(new Record[]{GRAND2, PARENT7, PARENT5}));
        final Set<ComparableArray<Record>> expected_grounding_set3 = new HashSet<>();
        expected_grounding_set3.add(new ComparableArray<>(new Record[]{GRAND1, PARENT6, PARENT1}));
        expected_grounding_set3.add(new ComparableArray<>(new Record[]{GRAND2, PARENT8, PARENT4}));
        final Set<ComparableArray<Record>> expected_grounding_set4 = new HashSet<>();
        expected_grounding_set4.add(new ComparableArray<>(new Record[]{GRAND1, PARENT6, PARENT1}));
        expected_grounding_set4.add(new ComparableArray<>(new Record[]{GRAND2, PARENT8, PARENT5}));
        Record counter1 = new Record(new int[]{NUM_G1, NUM_D1});
        Record counter2 = new Record(new int[]{NUM_G2, NUM_S2});
        EvidenceBatch actual_evidence = rule.getEvidenceAndMarkEntailment();
        checkEvidence(actual_evidence, new int[]{NUM_GRANDPARENT, NUM_PARENT, NUM_PARENT}, new Set[]{
                expected_grounding_set1, expected_grounding_set2, expected_grounding_set3, expected_grounding_set4
        });
        assertEquals(new HashSet<>(List.of(counter1, counter2)), rule.getCounterexamples());
    }

    @Test
    void testFamilyRule5()  {
        final SimpleKb kb = kbFamily();
        final Set<Fingerprint> fp_cache = new HashSet<>();
        final Map<MultiSet<Integer>, Set<Fingerprint>> tabu_map = new HashMap<>();

        /* parent(?, ?) :- */
        final FragmentedCachedRule rule = new FragmentedCachedRule(NUM_GRANDPARENT, 2, fp_cache, tabu_map, kb);
        assertEquals("grandParent(?,?):-", rule.toDumpString(KB));
        assertEquals(
                new Eval(3, 16 * 16, 0),
                rule.getEval()
        );
        assertEquals(0, rule.usedLimitedVars());
        assertEquals(0, rule.length());
        assertEquals(1, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* grandParent(X, ?) :- parent(X, ?) */
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(NUM_PARENT, 2, 0, 0, 0));
        assertEquals("grandParent(X0,?):-parent(X0,?)", rule.toDumpString(KB));
        assertEquals(
                new Eval(2, 6 * 16, 1),
                rule.getEval()
        );
        assertEquals(1, rule.usedLimitedVars());
        assertEquals(1, rule.length());
        assertEquals(2, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* grandParent(X, ?) :- parent(X, Y), father(Y, ?) */
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(NUM_FATHER, 2, 0, 1, 1));
        assertEquals("grandParent(X0,?):-parent(X0,X1),father(X1,?)", rule.toDumpString(KB));
        assertEquals(
                new Eval(2, 3 * 16, 2),
                rule.getEval()
        );
        assertEquals(2, rule.usedLimitedVars());
        assertEquals(2, rule.length());
        assertEquals(3, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* grandParent(X, Z) :- parent(X, Y), father(Y, Z) */
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(2, 1, 0, 1));
        assertEquals("grandParent(X0,X2):-parent(X0,X1),father(X1,X2)", rule.toDumpString(KB));
        assertEquals(
                new Eval(2, 4, 3),
                rule.getEval()
        );
        assertEquals(3, rule.usedLimitedVars());
        assertEquals(3, rule.length());
        assertEquals(4, fp_cache.size());
        assertEquals(0, tabu_map.size());
        final Set<ComparableArray<Record>> expected_grounding_set = new HashSet<>();
        expected_grounding_set.add(new ComparableArray<>(new Record[]{GRAND1, PARENT6, FATHER1}));
        expected_grounding_set.add(new ComparableArray<>(new Record[]{GRAND2, PARENT7, FATHER3}));
        Record counter1 = new Record(new int[]{NUM_G2, NUM_S2});
        Record counter2 = new Record(new int[]{NUM_G3, NUM_S3});
        EvidenceBatch actual_evidence = rule.getEvidenceAndMarkEntailment();
        checkEvidence(actual_evidence, new int[]{NUM_GRANDPARENT, NUM_PARENT, NUM_FATHER}, new Set[]{expected_grounding_set});
        assertEquals(new HashSet<>(List.of(counter1, counter2)), rule.getCounterexamples());
    }

    @Test
    void testFamilyRule6()  {
        final SimpleKb kb = kbFamily();
        final Set<Fingerprint> fp_cache = new HashSet<>();
        final Map<MultiSet<Integer>, Set<Fingerprint>> tabu_map = new HashMap<>();

        /* parent(?, ?) :- */
        final FragmentedCachedRule rule = new FragmentedCachedRule(NUM_GRANDPARENT, 2, fp_cache, tabu_map, kb);
        assertEquals("grandParent(?,?):-", rule.toDumpString(KB));
        assertEquals(
                new Eval(3, 16 * 16, 0),
                rule.getEval()
        );
        assertEquals(0, rule.usedLimitedVars());
        assertEquals(0, rule.length());
        assertEquals(1, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* grandParent(X, ?) :- parent(X, ?) */
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(NUM_FATHER, 2, 1, 0, 1));
        assertEquals("grandParent(?,X0):-father(?,X0)", rule.toDumpString(KB));
        assertEquals(
                new Eval(2, 5 * 16, 1),
                rule.getEval()
        );
        assertEquals(1, rule.usedLimitedVars());
        assertEquals(1, rule.length());
        assertEquals(2, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* grandParent(g1, X) :- father(?, X) */
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt1Uv2Const(0, 0, NUM_G1));
        assertEquals(String.format("grandParent(%d,X0):-father(?,X0)", NUM_G1), rule.toDumpString(KB));
        assertEquals(
                new Eval(1, 5, 2),
                rule.getEval()
        );
        assertEquals(1, rule.usedLimitedVars());
        assertEquals(2, rule.length());
        assertEquals(3, fp_cache.size());
        assertEquals(0, tabu_map.size());
        final Set<ComparableArray<Record>> expected_grounding_set = new HashSet<>();
        expected_grounding_set.add(new ComparableArray<>(new Record[]{GRAND1, FATHER1}));
        Set<Record> expected_counterexample_set = new HashSet<>();
        for (int arg2: new int[]{NUM_S2, NUM_D2, NUM_S3, NUM_D4}) {
            expected_counterexample_set.add(new Record(new int[]{NUM_G1, arg2}));
        }
        EvidenceBatch actual_evidence = rule.getEvidenceAndMarkEntailment();
        checkEvidence(actual_evidence, new int[]{NUM_GRANDPARENT, NUM_FATHER}, new Set[]{expected_grounding_set});
        assertEquals(expected_counterexample_set, rule.getCounterexamples());

        /* grandParent(g1, X) :- father(f2, X) */
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt1Uv2Const(1, 0, NUM_F2));
        assertEquals(String.format("grandParent(%d,X0):-father(%d,X0)", NUM_G1, NUM_F2), rule.toDumpString(KB));
        assertEquals(
                new Eval(0, 2, 3),
                rule.getEval()
        );
        assertEquals(1, rule.usedLimitedVars());
        assertEquals(3, rule.length());
        assertEquals(4, fp_cache.size());
        assertEquals(0, tabu_map.size());
        Record counter1 = new Record(new int[]{NUM_G1, NUM_S2});
        Record counter2 = new Record(new int[]{NUM_G1, NUM_D2});
        actual_evidence = rule.getEvidenceAndMarkEntailment();
        assertArrayEquals(new int[]{NUM_GRANDPARENT, NUM_FATHER}, actual_evidence.predicateSymbolsInRule);
        assertTrue(actual_evidence.evidenceList.isEmpty());
        assertEquals(new HashSet<>(List.of(counter1, counter2)), rule.getCounterexamples());
    }

    @Test
    void testFamilyRule7()  {
        final SimpleKb kb = kbFamily();
        final Set<Fingerprint> fp_cache = new HashSet<>();
        final Map<MultiSet<Integer>, Set<Fingerprint>> tabu_map = new HashMap<>();

        /* parent(?, ?) :- */
        final FragmentedCachedRule rule = new FragmentedCachedRule(NUM_PARENT, 2, fp_cache, tabu_map, kb);
        assertEquals("parent(?,?):-", rule.toDumpString(KB));
        assertEquals(
                new Eval(9, 16 * 16, 0),
                rule.getEval()
        );
        assertEquals(0, rule.usedLimitedVars());
        assertEquals(0, rule.length());
        assertEquals(1, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* parent(X, X) :- */
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(0, 0, 0, 1));
        assertEquals("parent(X0,X0):-", rule.toDumpString(KB));
        assertEquals(
                new Eval(0, 16, 1),
                rule.getEval()
        );
        assertEquals(1, rule.usedLimitedVars());
        assertEquals(1, rule.length());
        assertEquals(2, fp_cache.size());
        assertEquals(0, tabu_map.size());
        Set<Record> expected_counterexample_set = new HashSet<>();
        for (int arg = 1; arg <= kb.totalConstants(); arg++) {
            expected_counterexample_set.add(new Record(new int[]{arg, arg}));
        }
        EvidenceBatch actual_evidence = rule.getEvidenceAndMarkEntailment();
        assertArrayEquals(new int[]{NUM_PARENT}, actual_evidence.predicateSymbolsInRule);
        assertTrue(actual_evidence.evidenceList.isEmpty());
        assertEquals(expected_counterexample_set, rule.getCounterexamples());
    }

    @Test
    void testFamilyRule8()  {
        final SimpleKb kb = kbFamily();
        final Set<Fingerprint> fp_cache = new HashSet<>();
        final Map<MultiSet<Integer>, Set<Fingerprint>> tabu_map = new HashMap<>();

        /* father(?, ?) :- */
        final FragmentedCachedRule rule = new FragmentedCachedRule(NUM_FATHER, 2, fp_cache, tabu_map, kb);
        assertEquals("father(?,?):-", rule.toDumpString(KB));
        assertEquals(
                new Eval(5, 16 * 16, 0),
                rule.getEval()
        );
        assertEquals(0, rule.usedLimitedVars());
        assertEquals(0, rule.length());
        assertEquals(1, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* father(X, ?):- parent(?, X) */
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(NUM_PARENT, 2, 1, 0, 0));
        assertEquals("father(X0,?):-parent(?,X0)", rule.toDumpString(KB));
        assertEquals(
                new Eval(4, 8 * 16, 1),
                rule.getEval()
        );
        assertEquals(1, rule.usedLimitedVars());
        assertEquals(1, rule.length());
        assertEquals(2, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* father(X, ?):- parent(?, X), parent(X, ?) */
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt1Uv2ExtLv(NUM_PARENT, 2, 0, 0));
        assertEquals("father(X0,?):-parent(?,X0),parent(X0,?)", rule.toDumpString(KB));
        assertEquals(
                new Eval(3, 3 * 16, 2),
                rule.getEval()
        );
        assertEquals(1, rule.usedLimitedVars());
        assertEquals(2, rule.length());
        assertEquals(3, fp_cache.size());
        assertEquals(0, tabu_map.size());
        final Set<ComparableArray<Record>> expected_grounding_set1 = new HashSet<>();
        expected_grounding_set1.add(new ComparableArray<>(new Record[]{FATHER1, PARENT6, PARENT1}));
        expected_grounding_set1.add(new ComparableArray<>(new Record[]{FATHER2, PARENT7, PARENT3}));
        expected_grounding_set1.add(new ComparableArray<>(new Record[]{FATHER3, PARENT7, PARENT3}));
        final Set<ComparableArray<Record>> expected_grounding_set2 = new HashSet<>();
        expected_grounding_set2.add(new ComparableArray<>(new Record[]{FATHER1, PARENT6, PARENT1}));
        expected_grounding_set2.add(new ComparableArray<>(new Record[]{FATHER2, PARENT7, PARENT3}));
        expected_grounding_set2.add(new ComparableArray<>(new Record[]{FATHER3, PARENT7, PARENT4}));
        final Set<ComparableArray<Record>> expected_grounding_set3 = new HashSet<>();
        expected_grounding_set3.add(new ComparableArray<>(new Record[]{FATHER1, PARENT6, PARENT1}));
        expected_grounding_set3.add(new ComparableArray<>(new Record[]{FATHER2, PARENT7, PARENT4}));
        expected_grounding_set3.add(new ComparableArray<>(new Record[]{FATHER3, PARENT7, PARENT3}));
        final Set<ComparableArray<Record>> expected_grounding_set4 = new HashSet<>();
        expected_grounding_set4.add(new ComparableArray<>(new Record[]{FATHER1, PARENT6, PARENT1}));
        expected_grounding_set4.add(new ComparableArray<>(new Record[]{FATHER2, PARENT7, PARENT4}));
        expected_grounding_set4.add(new ComparableArray<>(new Record[]{FATHER3, PARENT7, PARENT4}));
        final Set<ComparableArray<Record>> expected_grounding_set5 = new HashSet<>();
        expected_grounding_set5.add(new ComparableArray<>(new Record[]{FATHER1, PARENT6, PARENT2}));
        expected_grounding_set5.add(new ComparableArray<>(new Record[]{FATHER2, PARENT7, PARENT3}));
        expected_grounding_set5.add(new ComparableArray<>(new Record[]{FATHER3, PARENT7, PARENT3}));
        final Set<ComparableArray<Record>> expected_grounding_set6 = new HashSet<>();
        expected_grounding_set6.add(new ComparableArray<>(new Record[]{FATHER1, PARENT6, PARENT2}));
        expected_grounding_set6.add(new ComparableArray<>(new Record[]{FATHER2, PARENT7, PARENT3}));
        expected_grounding_set6.add(new ComparableArray<>(new Record[]{FATHER3, PARENT7, PARENT4}));
        final Set<ComparableArray<Record>> expected_grounding_set7 = new HashSet<>();
        expected_grounding_set7.add(new ComparableArray<>(new Record[]{FATHER1, PARENT6, PARENT2}));
        expected_grounding_set7.add(new ComparableArray<>(new Record[]{FATHER2, PARENT7, PARENT4}));
        expected_grounding_set7.add(new ComparableArray<>(new Record[]{FATHER3, PARENT7, PARENT3}));
        final Set<ComparableArray<Record>> expected_grounding_set8 = new HashSet<>();
        expected_grounding_set8.add(new ComparableArray<>(new Record[]{FATHER1, PARENT6, PARENT2}));
        expected_grounding_set8.add(new ComparableArray<>(new Record[]{FATHER2, PARENT7, PARENT4}));
        expected_grounding_set8.add(new ComparableArray<>(new Record[]{FATHER3, PARENT7, PARENT4}));
        final Set<Record> expected_counterexample_set = new HashSet<>();
        for (int arg1: new int[]{NUM_F1, NUM_F2, NUM_M2}) {
            for (int arg2 = 1; arg2 <= kb.totalConstants(); arg2++) {
                expected_counterexample_set.add(new Record(new int[]{arg1, arg2}));
            }
        }
        expected_counterexample_set.remove(FATHER1);
        expected_counterexample_set.remove(FATHER2);
        expected_counterexample_set.remove(FATHER3);
        EvidenceBatch actual_evidence = rule.getEvidenceAndMarkEntailment();
        checkEvidence(actual_evidence, new int[]{NUM_FATHER, NUM_PARENT, NUM_PARENT}, new Set[]{
                expected_grounding_set1, expected_grounding_set2, expected_grounding_set3, expected_grounding_set4,
                expected_grounding_set5, expected_grounding_set6, expected_grounding_set7, expected_grounding_set8
        });
        assertEquals(expected_counterexample_set, rule.getCounterexamples());
    }

    @Test
    void testFamilyRule9()  {
        final SimpleKb kb = kbFamily();
        final Set<Fingerprint> fp_cache = new HashSet<>();
        final Map<MultiSet<Integer>, Set<Fingerprint>> tabu_map = new HashMap<>();

        /* father(?, ?) :- */
        final FragmentedCachedRule rule1 = new FragmentedCachedRule(NUM_FATHER, 2, fp_cache, tabu_map, kb);
        assertEquals("father(?,?):-", rule1.toDumpString(KB));
        assertEquals(
                new Eval(5, 16 * 16, 0),
                rule1.getEval()
        );
        assertEquals(0, rule1.usedLimitedVars());
        assertEquals(0, rule1.length());
        assertEquals(1, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* #1: father(f2,?):- */
        rule1.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule1.cvt1Uv2Const(0, 0, NUM_F2));
        assertEquals(String.format("father(%d,?):-", NUM_F2), rule1.toDumpString(KB));
        assertEquals(
                new Eval(2, 16, 1),
                rule1.getEval()
        );
        assertEquals(0, rule1.usedLimitedVars());
        assertEquals(1, rule1.length());
        assertEquals(2, fp_cache.size());
        assertEquals(0, tabu_map.size());
        rule1.getEvidenceAndMarkEntailment();

        /* father(?, ?) :- */
        final Set<Fingerprint> fp_cache2 = new HashSet<>();
        final Map<MultiSet<Integer>, Set<Fingerprint>> tabu_map2 = new HashMap<>();
        final FragmentedCachedRule rule2 = new FragmentedCachedRule(NUM_FATHER, 2, fp_cache2, tabu_map2, kb);
        assertEquals("father(?,?):-", rule2.toDumpString(KB));
        assertEquals(
                new Eval(3, 16 * 16 - 2, 0),
                rule2.getEval()
        );
        assertEquals(0, rule2.usedLimitedVars());
        assertEquals(0, rule2.length());
        assertEquals(1, fp_cache2.size());
        assertEquals(0, tabu_map2.size());
        final Set<ComparableArray<Record>> expected_grounding_set = new HashSet<>();
        expected_grounding_set.add(new ComparableArray<>(new Record[]{FATHER1}));
        expected_grounding_set.add(new ComparableArray<>(new Record[]{FATHER4}));
        expected_grounding_set.add(new ComparableArray<>(new Record[]{FATHER5}));
        final Set<Record> expected_counterexample_set = new HashSet<>();
        for (int arg1 = 1; arg1 <= kb.totalConstants(); arg1++) {
            for (int arg2 = 1; arg2 <= kb.totalConstants(); arg2++) {
                expected_counterexample_set.add(new Record(new int[]{arg1, arg2}));
            }
        }
        expected_counterexample_set.remove(FATHER1);
        expected_counterexample_set.remove(FATHER2);
        expected_counterexample_set.remove(FATHER3);
        expected_counterexample_set.remove(FATHER4);
        expected_counterexample_set.remove(FATHER5);
        checkEvidence(rule2.getEvidenceAndMarkEntailment(), new int[]{NUM_FATHER}, new Set[]{expected_grounding_set});
        assertEquals(expected_counterexample_set, rule2.getCounterexamples());
    }

    @Test
    void testFamilyRule10()  {
        final SimpleKb kb = kbFamily();
        final Set<Fingerprint> fp_cache = new HashSet<>();
        final Map<MultiSet<Integer>, Set<Fingerprint>> tabu_map = new HashMap<>();

        /* parent(?, ?) :- */
        final FragmentedCachedRule rule = new FragmentedCachedRule(NUM_PARENT, 2, fp_cache, tabu_map, kb);
        assertEquals("parent(?,?):-", rule.toDumpString(KB));
        assertEquals(
                new Eval(9, 16 * 16, 0),
                rule.getEval()
        );
        assertEquals(0, rule.usedLimitedVars());
        assertEquals(0, rule.length());
        assertEquals(1, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* parent(X, X):- */
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(0, 0, 0, 1));
        assertEquals("parent(X0,X0):-", rule.toDumpString(KB));
        assertEquals(
                new Eval(0, 16, 1),
                rule.getEval()
        );
        assertEquals(1, rule.usedLimitedVars());
        assertEquals(1, rule.length());
        assertEquals(2, fp_cache.size());
        assertEquals(0, tabu_map.size());
        Set<Record> expected_counterexample_set = new HashSet<>();
        for (int arg = 1; arg <= kb.totalConstants(); arg++) {
            expected_counterexample_set.add(new Record(new int[]{arg, arg}));
        }
        EvidenceBatch actual_evidence = rule.getEvidenceAndMarkEntailment();
        assertArrayEquals(new int[]{NUM_PARENT}, actual_evidence.predicateSymbolsInRule);
        assertTrue(actual_evidence.evidenceList.isEmpty());
        assertEquals(expected_counterexample_set, rule.getCounterexamples());
    }

    @Test
    void testCounterexample1()  {
        final SimpleKb kb = kbFamily();
        final Set<Fingerprint> fp_cache = new HashSet<>();
        final Map<MultiSet<Integer>, Set<Fingerprint>> tabu_map = new HashMap<>();

        /* father(?, ?) :- */
        final FragmentedCachedRule rule = new FragmentedCachedRule(NUM_FATHER, 2, fp_cache, tabu_map, kb);
        assertEquals("father(?,?):-", rule.toDumpString(KB));
        assertEquals(
                new Eval(5, 16 * 16, 0),
                rule.getEval()
        );
        assertEquals(0, rule.usedLimitedVars());
        assertEquals(0, rule.length());
        assertEquals(1, fp_cache.size());
        assertEquals(0, tabu_map.size());
        Set<Record> expected_counterexample_set = new HashSet<>();
        for (int arg1 = 1; arg1 <= kb.totalConstants(); arg1++) {
            for (int arg2 = 1; arg2 <= kb.totalConstants(); arg2++) {
                expected_counterexample_set.add(new Record(new int[]{arg1, arg2}));
            }
        }
        for (int[] row: kb.getRelation("father").getAllRows()) {
            expected_counterexample_set.remove(new Record(row));
        }
        assertEquals(expected_counterexample_set, rule.getCounterexamples());
    }

    @Test
    void testFamilyWithCopy1()  {
        final SimpleKb kb = kbFamily();
        final Set<Fingerprint> fp_cache = new HashSet<>();
        final Map<MultiSet<Integer>, Set<Fingerprint>> tabu_map = new HashMap<>();

        /* grandParent(?, ?) :- */
        final FragmentedCachedRule rule = new FragmentedCachedRule(NUM_GRANDPARENT, 2, fp_cache, tabu_map, kb);
        assertEquals("grandParent(?,?):-", rule.toDumpString(KB));
        assertEquals(
                new Eval(3, 16 * 16, 0),
                rule.getEval()
        );
        assertEquals(0, rule.usedLimitedVars());
        assertEquals(0, rule.length());
        assertEquals(1, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* #1: grandParent(X, ?) :- parent(X, ?) */
        final FragmentedCachedRule rule1 = new FragmentedCachedRule(rule);
        rule1.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule1.cvt2Uvs2NewLv(NUM_PARENT, 2, 0, 0, 0));
        assertEquals("grandParent(X0,?):-parent(X0,?)", rule1.toDumpString(KB));
        assertEquals(
                new Eval(2, 6 * 16, 1),
                rule1.getEval()
        );
        assertEquals(1, rule1.usedLimitedVars());
        assertEquals(1, rule1.length());
        assertEquals(2, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* #1: grandParent(X, ?) :- parent(X, Y), father(Y, ?) */
        rule1.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule1.cvt2Uvs2NewLv(NUM_FATHER, 2, 0, 1, 1));
        assertEquals("grandParent(X0,?):-parent(X0,X1),father(X1,?)", rule1.toDumpString(KB));
        assertEquals(
                new Eval(2, 3 * 16, 2),
                rule1.getEval()
        );
        assertEquals(2, rule1.usedLimitedVars());
        assertEquals(2, rule1.length());
        assertEquals(3, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* #1: grandParent(X, Z) :- parent(X, Y), father(Y, Z) */
        rule1.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule1.cvt2Uvs2NewLv(0, 1, 2, 1));
        assertEquals("grandParent(X0,X2):-parent(X0,X1),father(X1,X2)", rule1.toDumpString(KB));
        assertEquals(
                new Eval(2, 4, 3),
                rule1.getEval()
        );
        assertEquals(3, rule1.usedLimitedVars());
        assertEquals(3, rule1.length());
        assertEquals(4, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* #2: grandParent(X, ?) :- parent(X, ?) */
        final FragmentedCachedRule rule2 = new FragmentedCachedRule(rule);
        rule2.updateCacheIndices();
        assertEquals(UpdateStatus.DUPLICATED, rule2.cvt2Uvs2NewLv(NUM_PARENT, 2, 0, 0, 0));
        assertEquals(4, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* #3: grandParent(?, X) :- father(?, X) */
        final FragmentedCachedRule rule3 = new FragmentedCachedRule(rule);
        rule3.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule3.cvt2Uvs2NewLv(NUM_FATHER, 2, 1, 0, 1));
        assertEquals("grandParent(?,X0):-father(?,X0)", rule3.toDumpString(KB));
        assertEquals(
                new Eval(2, 5 * 16, 1),
                rule3.getEval()
        );
        assertEquals(1, rule3.usedLimitedVars());
        assertEquals(1, rule3.length());
        assertEquals(5, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* #3: grandParent(Y, X) :- father(?, X), parent(Y, ?) */
        rule3.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule3.cvt2Uvs2NewLv(NUM_PARENT, 2, 0, 0, 0));
        assertEquals("grandParent(X1,X0):-father(?,X0),parent(X1,?)", rule3.toDumpString(KB));
        assertEquals(
                new Eval(2, 5 * 6, 2),
                rule3.getEval()
        );
        assertEquals(2, rule3.usedLimitedVars());
        assertEquals(2, rule3.length());
        assertEquals(6, fp_cache.size());
        assertEquals(0, tabu_map.size());
        final Set<Record> expected_counterexample_set = new HashSet<>();
        for (int arg1: new int[]{NUM_F1, NUM_F2, NUM_M2, NUM_G1, NUM_G2, NUM_G3}) {
            for (int arg2: new int[]{NUM_S1, NUM_S2, NUM_D2, NUM_S3, NUM_D4}) {
                expected_counterexample_set.add(new Record(new int[]{arg1, arg2}));
            }
        }
        expected_counterexample_set.remove(GRAND1);
        expected_counterexample_set.remove(GRAND2);
        assertEquals(expected_counterexample_set, rule3.getCounterexamples());

        /* #3: grandParent(Y, X) :- father(Z, X), parent(Y, Z) */
        rule3.updateCacheIndices();
        assertEquals(UpdateStatus.DUPLICATED, rule3.cvt2Uvs2NewLv(1, 0, 2, 1));
        assertEquals(6, fp_cache.size());
        assertEquals(0, tabu_map.size());

        Set<ComparableArray<Record>> expected_grounding_set = new HashSet<>();
        expected_grounding_set.add(new ComparableArray<>(new Record[]{GRAND1, PARENT6, FATHER1}));
        expected_grounding_set.add(new ComparableArray<>(new Record[]{GRAND2, PARENT7, FATHER3}));
        Record counter1 = new Record(new int[]{NUM_G2, NUM_S2});
        Record counter2 = new Record(new int[]{NUM_G3, NUM_S3});
        checkEvidence(rule1.getEvidenceAndMarkEntailment(), new int[]{NUM_GRANDPARENT, NUM_PARENT, NUM_FATHER}, new Set[]{expected_grounding_set});
        assertEquals(new HashSet<>(List.of(counter1, counter2)), rule1.getCounterexamples());
    }

    @Test
    void testFamilyWithCopy2()  {
        final SimpleKb kb = kbFamily();
        final Set<Fingerprint> fp_cache = new HashSet<>();
        final Map<MultiSet<Integer>, Set<Fingerprint>> tabu_map = new HashMap<>();

        /* #1: parent(?, ?) :- */
        final FragmentedCachedRule rule1 = new FragmentedCachedRule(NUM_PARENT, 2, fp_cache, tabu_map, kb);
        assertEquals("parent(?,?):-", rule1.toDumpString(KB));
        assertEquals(
                new Eval(9, 16 * 16, 0),
                rule1.getEval()
        );
        assertEquals(0, rule1.usedLimitedVars());
        assertEquals(0, rule1.length());
        assertEquals(1, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* #1: parent(f2, ?) :- */
        rule1.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule1.cvt1Uv2Const(0, 0, NUM_F2));
        assertEquals(String.format("parent(%d,?):-", NUM_F2), rule1.toDumpString(KB));
        assertEquals(
                new Eval(2, 16, 1),
                rule1.getEval()
        );
        assertEquals(0, rule1.usedLimitedVars());
        assertEquals(1, rule1.length());
        assertEquals(2, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* #2: parent(f2, d2) :- */
        final FragmentedCachedRule rule2 = new FragmentedCachedRule(rule1);
        rule2.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule2.cvt1Uv2Const(0, 1, NUM_D2));
        assertEquals(String.format("parent(%d,%d):-", NUM_F2, NUM_D2), rule2.toDumpString(KB));
        assertEquals(
                new Eval(1, 1, 2),
                rule2.getEval()
        );
        assertEquals(0, rule2.usedLimitedVars());
        assertEquals(2, rule2.length());
        assertEquals(3, fp_cache.size());
        assertEquals(0, tabu_map.size());

        /* #3: parent(f2, X) :- father(?, X) */
        final FragmentedCachedRule rule3 = new FragmentedCachedRule(rule1);
        assertEquals(UpdateStatus.NORMAL, rule3.cvt2Uvs2NewLv(NUM_FATHER, 2, 1, 0, 1));
        assertEquals(String.format("parent(%d,X0):-father(?,X0)", NUM_F2), rule3.toDumpString(KB));
        assertEquals(
                new Eval(2, 5, 2),
                rule3.getEval()
        );
        assertEquals(1, rule3.usedLimitedVars());
        assertEquals(2, rule3.length());
        assertEquals(4, fp_cache.size());
        assertEquals(0, tabu_map.size());
//        final Set<ComparableArray<Record>> expected_grounding_set3 = new HashSet<>();
//        expected_grounding_set3.add(new ComparableArray<>(new Record[]{PARENT3, FATHER2}));
//        final Set<Record> expected_counterexample_set3 = new HashSet<>();
//        for (int arg: new int[]{NUM_S1, NUM_S3, NUM_D4}) {
//            expected_counterexample_set3.add(new Record(new int[]{NUM_F2, arg}));
//        }
//        checkEvidence(rule3.getEvidenceAndMarkEntailment(), new int[]{NUM_PARENT, NUM_FATHER}, new Set[]{expected_grounding_set3});
//        assertEquals(expected_counterexample_set3, rule3.getCounterexamples());

        final Set<ComparableArray<Record>> expected_grounding_set2 = new HashSet<>();
        expected_grounding_set2.add(new ComparableArray<>(new Record[]{PARENT4}));
        checkEvidence(rule2.getEvidenceAndMarkEntailment(), new int[]{NUM_PARENT}, new Set[]{expected_grounding_set2});
        assertTrue(rule2.getCounterexamples().isEmpty());
    }

    void checkEvidence(EvidenceBatch actualEvidence, int[] expectedRelationsInRule, Set<ComparableArray<Record>>[] expectedGroundingSets) {
        assertArrayEquals(expectedRelationsInRule, actualEvidence.predicateSymbolsInRule);
        Set<ComparableArray<Record>> actual_grounding_set = new HashSet<>();
        for (int[][] grounding: actualEvidence.evidenceList) {
            Record[] record_list = new Record[grounding.length];
            for (int i = 0; i < grounding.length; i++) {
                record_list[i] = new Record(grounding[i]);
            }
            actual_grounding_set.add(new ComparableArray<>(record_list));
        }
        boolean match_found = false;
        for (Set<ComparableArray<Record>> expected_grounding_set: expectedGroundingSets) {
            if (expected_grounding_set.equals(actual_grounding_set)) {
                match_found = true;
                break;
            }
        }
        assertTrue(match_found);
    }

    @Test
    void testValidity1()  {
        final SimpleKb kb = kbFamily();
        final Set<Fingerprint> fp_cache = new HashSet<>();
        final Map<MultiSet<Integer>, Set<Fingerprint>> tabu_map = new HashMap<>();

        /* father(?,?):- */
        final FragmentedCachedRule rule = new FragmentedCachedRule(NUM_FATHER, 2, fp_cache, tabu_map, kb);
        assertEquals("father(?,?):-", rule.toDumpString(KB));

        /* #1: father(X,?) :- father(?,X) */
        final FragmentedCachedRule rule1 = new FragmentedCachedRule(rule);
        rule1.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule1.cvt2Uvs2NewLv(NUM_FATHER, 2, 1, 0, 0));
        assertEquals("father(X0,?):-father(?,X0)", rule1.toDumpString(KB));

        /* #1: father(X,Y) :- father(Y,X) */
        rule1.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule1.cvt2Uvs2NewLv(0, 1, 1, 0));
        assertEquals("father(X0,X1):-father(X1,X0)", rule1.toDumpString(KB));

        /* #2: father(X,?) :- father(X,?) [now should NOT be invalid] */
        final FragmentedCachedRule rule2 = new FragmentedCachedRule(rule);
        rule2.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule2.cvt2Uvs2NewLv(NUM_FATHER, 2, 0, 0, 0));
    }

    @Test
    void testValidity2()  {
        final SimpleKb kb = kbFamily();
        final Set<Fingerprint> fp_cache = new HashSet<>();
        final Map<MultiSet<Integer>, Set<Fingerprint>> tabu_map = new HashMap<>();

        /* father(?,?):- */
        final FragmentedCachedRule rule = new FragmentedCachedRule(NUM_FATHER, 2, fp_cache, tabu_map, kb);
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(NUM_FATHER, 2, 1, 0, 0));
        assertEquals("father(X0,?):-father(?,X0)", rule.toDumpString(KB));

        /* father(X,?) :- father(?,X), father(?,X) */
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt1Uv2ExtLv(NUM_FATHER, 2, 1, 0));
        assertEquals("father(X0,?):-father(?,X0),father(?,X0)", rule.toDumpString(KB));

        /* father(X,?) :- father(Y,X), father(Y,X) [invalid] */
        assertEquals(UpdateStatus.INVALID, rule.cvt2Uvs2NewLv(1, 0, 2, 0));
    }

    @Test
    void testRcPruning1()  {
        Rule.MIN_FACT_COVERAGE = 0.44;

        final SimpleKb kb = kbFamily();
        final Set<Fingerprint> fp_cache = new HashSet<>();
        final Map<MultiSet<Integer>, Set<Fingerprint>> tabu_map = new HashMap<>();

        /* parent(X, ?) :- father(X, ?) */
        final FragmentedCachedRule rule = new FragmentedCachedRule(NUM_PARENT, 2, fp_cache, tabu_map, kb);
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(NUM_FATHER, 2, 0, 0, 0));
        assertEquals("parent(X0,?):-father(X0,?)", rule.toDumpString(KB));
        assertEquals(
                new Eval(4, 4 * 16, 1),
                rule.getEval()
        );
    }

    @Test
    void testRcPruning2()  {
        Rule.MIN_FACT_COVERAGE = 0.45;

        final SimpleKb kb = kbFamily();
        final Set<Fingerprint> fp_cache = new HashSet<>();
        final Map<MultiSet<Integer>, Set<Fingerprint>> tabu_map = new HashMap<>();

        /* parent(X, ?) :- father(X, ?) */
        final FragmentedCachedRule rule = new FragmentedCachedRule(NUM_PARENT, 2, fp_cache, tabu_map, kb);
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.INSUFFICIENT_COVERAGE, rule.cvt2Uvs2NewLv(NUM_FATHER, 2, 0, 0, 0));
        assertEquals(4.0 / 9.0, rule.recordCoverage());
        assertEquals(1, tabu_map.size());
        assertTrue(tabu_map.get(new MultiSet<>(new Integer[]{NUM_FATHER})).contains(rule.getFingerprint()));
    }

    @Test
    void testAnyRule1()  {
        int a = 1;
        int A = 2;
        int plus = 3;
        int b = 4;
        int B = 5;
        int c = 6;
        int C = 7;
        int minus = 8;
        int[] p1 = new int[]{a, A, plus};
        int[] p2 = new int[]{b, B, plus};
        int[] p3 = new int[]{c, C, minus};
        int[] h1 = new int[]{a, a, A, A};
        int[] h2 = new int[]{b, b, B, B};
        SimpleKb kb = new SimpleKb("test", new int[][][] {
                new int[][] {p1, p2, p3},
                new int[][] {h1, h2}
        }, new String[]{"p", "h"});
        SimpleRelation rel_p = kb.getRelation("p");
        SimpleRelation rel_h = kb.getRelation("h");

        /* h(X, X, Y, Y) :- p(X, Y, +) */
        FragmentedCachedRule rule = new FragmentedCachedRule(rel_h.id, 4, new HashSet<>(), new HashMap<>(), kb);
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(rel_p.id, 3, 0, 0, 0));
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(0, 2, 1, 1));
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt1Uv2ExtLv(0, 1, 0));
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt1Uv2ExtLv(0, 3, 1));
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt1Uv2Const(1, 2, plus));
        assertEquals("h(X0,X0,X1,X1):-p(X0,X1,3)", rule.toDumpString(kb));
        assertEquals(new Eval(2, 2, 5), rule.getEval());
        assertEquals(2, rule.usedLimitedVars());
        assertEquals(5, rule.length());
        final Set<ComparableArray<Record>> expected_grounding_set = new HashSet<>();
        expected_grounding_set.add(new ComparableArray<>(new Record[]{new Record(h1), new Record(p1)}));
        expected_grounding_set.add(new ComparableArray<>(new Record[]{new Record(h2), new Record(p2)}));
        checkEvidence(rule.getEvidenceAndMarkEntailment(), new int[]{rel_h.id, rel_p.id}, new Set[]{expected_grounding_set});
        assertTrue(rule.getCounterexamples().isEmpty());
    }

    @Test
    void testAnyRule2()  {
        int a = 1;
        int A = 2;
        int b = 3;
        int[] p1 = new int[]{A, a};
        int[] p2 = new int[]{a, a};
        int[] p3 = new int[]{A, A};
        int[] p4 = new int[]{b, a};
        int[] q1 = new int[]{b};
        int[] q2 = new int[]{a};
        int[] h1 = new int[]{a};
        SimpleKb kb = new SimpleKb("test", new int[][][] {
                new int[][] {p1, p2, p3, p4},
                new int[][] {q1, q2},
                new int[][] {h1}
        }, new String[]{"p", "q", "h"});
        SimpleRelation rel_p = kb.getRelation("p");
        SimpleRelation rel_q = kb.getRelation("q");
        SimpleRelation rel_h = kb.getRelation("h");

        /* h(X) :- p(X, X), q(X) */
        FragmentedCachedRule rule = new FragmentedCachedRule(rel_h.id, 1, new HashSet<>(), new HashMap<>(), kb);
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(rel_p.id, 2, 0, 0, 0));
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt1Uv2ExtLv(rel_q.id, 1, 0, 0));
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt1Uv2ExtLv(1, 1, 0));
        assertEquals("h(X0):-p(X0,X0),q(X0)", rule.toDumpString(kb));
        assertEquals(new Eval(1, 1, 3), rule.getEval());
        assertEquals(1, rule.usedLimitedVars());
        assertEquals(3, rule.length());
        final Set<ComparableArray<Record>> expected_grounding_set = new HashSet<>();
        expected_grounding_set.add(new ComparableArray<>(new Record[]{new Record(h1), new Record(p2), new Record(q2)}));
        checkEvidence(rule.getEvidenceAndMarkEntailment(), new int[]{rel_h.id, rel_p.id, rel_q.id}, new Set[]{expected_grounding_set});
        assertTrue(rule.getCounterexamples().isEmpty());
    }

    @Test
    void testAnyRule3()  {
        int a = 1;
        int b = 2;
        int c = 3;
        int[] h1 = new int[]{a, a};
        int[] h2 = new int[]{b, b};
        int[] h3 = new int[]{a, c};
        SimpleKb kb = new SimpleKb("test", new int[][][] {
                new int[][] {h1, h2, h3}
        }, new String[]{"h"});
        SimpleRelation rel_h = kb.getRelation("h");

        /* h(X, X) :- */
        FragmentedCachedRule rule = new FragmentedCachedRule(rel_h.id, 2, new HashSet<>(), new HashMap<>(), kb);
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(0, 0, 0, 1));
        assertEquals("h(X0,X0):-", rule.toDumpString(kb));
        assertEquals(new Eval(2, 3, 1), rule.getEval());
        assertEquals(1, rule.usedLimitedVars());
        assertEquals(1, rule.length());
        final Set<ComparableArray<Record>> expected_grounding_set = new HashSet<>();
        expected_grounding_set.add(new ComparableArray<>(new Record[]{new Record(h1)}));
        expected_grounding_set.add(new ComparableArray<>(new Record[]{new Record(h2)}));
        checkEvidence(rule.getEvidenceAndMarkEntailment(), new int[]{rel_h.id}, new Set[]{expected_grounding_set});
        assertEquals(new HashSet<>(List.of(new Record(new int[]{c, c}))), rule.getCounterexamples());

        assertTrue(rel_h.isEntailed(h1));
        assertTrue(rel_h.isEntailed(h2));
        assertFalse(rel_h.isEntailed(h3));
    }

    @Test
    void testAnyRule4()  {
        int a = 1;
        int b = 2;
        int[] h1 = new int[]{a, a, b};
        int[] h2 = new int[]{b, b, a};
        int[] h3 = new int[]{a, b, b};
        SimpleKb kb = new SimpleKb("test", new int[][][] {
                new int[][] {h1, h2, h3}
        }, new String[]{"h"});
        SimpleRelation rel_h = kb.getRelation("h");

        /* h(X, X, ?) :- */
        FragmentedCachedRule rule = new FragmentedCachedRule(rel_h.id, 3, new HashSet<>(), new HashMap<>(), kb);
        rule.updateCacheIndices();
        assertEquals(UpdateStatus.NORMAL, rule.cvt2Uvs2NewLv(0, 0, 0, 1));
        assertEquals("h(X0,X0,?):-", rule.toDumpString(kb));
        assertEquals(new Eval(2, 4, 1), rule.getEval());
        assertEquals(1, rule.usedLimitedVars());
        assertEquals(1, rule.length());
        final Set<ComparableArray<Record>> expected_grounding_set = new HashSet<>();
        expected_grounding_set.add(new ComparableArray<>(new Record[]{new Record(h1)}));
        expected_grounding_set.add(new ComparableArray<>(new Record[]{new Record(h2)}));
        checkEvidence(rule.getEvidenceAndMarkEntailment(), new int[]{rel_h.id}, new Set[]{expected_grounding_set});
        assertEquals(new HashSet<>(List.of(new Record(new int[]{a, a, a}), new Record(new int[]{b, b, b}))), rule.getCounterexamples());

        assertTrue(rel_h.isEntailed(h1));
        assertTrue(rel_h.isEntailed(h2));
        assertFalse(rel_h.isEntailed(h3));
    }

    @Test
    void testStructureConstructor1()  {
        /* h(X, Y, X, ?, a, ?) :- p(Y, Z, Z) */
        int a = 1;
        int b = 2;
        int c = 3;
        int d = 4;
        int e = 5;
        int[] h1 = new int[]{a, a, b, a, a, a};
        int[] h2 = new int[]{b, b, b, c, a, e};//
        int[] h3 = new int[]{c, d, c, c, a, d};//
        int[] h4 = new int[]{d, d, d, d, d, d};
        int[] h5 = new int[]{e, a, a, d, d, d};
        int[] p1 = new int[]{a, b, b};
        int[] p2 = new int[]{a, c, d};
        int[] p3 = new int[]{c, c, c};
        int[] p4 = new int[]{b, d, d};//
        int[] p5 = new int[]{e, a, a};
        int[] p6 = new int[]{e, b, c};
        SimpleKb kb = new SimpleKb("test", new int[][][] {
                new int[][] {h1, h2, h3, h4, h5},
                new int[][] {p1, p2, p3, p4, p5, p6}
        }, new String[]{"h", "p"});
        SimpleRelation rel_h = kb.getRelation("h");
        SimpleRelation rel_p = kb.getRelation("p");

        /* Construct */
        FragmentedCachedRule rule = new FragmentedCachedRule(new ArrayList<>(List.of(
                new Predicate(rel_h.id, new int[]{
                        Argument.variable(0), Argument.variable(1), Argument.variable(0),
                        Argument.EMPTY_VALUE, Argument.constant(a), Argument.EMPTY_VALUE
                }),
                new Predicate(rel_p.id, new int[]{
                        Argument.variable(1), Argument.variable(2), Argument.variable(2)
                }))),
                new HashSet<>(), new HashMap<>(), kb
        );
        assertEquals("h(X0,X1,X0,?,1,?):-p(X1,X2,X2)", rule.toDumpString(kb));
        assertEquals(new Eval(1, 4 * 5 * 5 * 5, 4), rule.getEval());
        assertEquals(3, rule.usedLimitedVars());
        assertEquals(4, rule.length());
        final Set<ComparableArray<Record>> expected_grounding_set = new HashSet<>();
        expected_grounding_set.add(new ComparableArray<>(new Record[]{new Record(h2), new Record(p4)}));
        checkEvidence(rule.getEvidenceAndMarkEntailment(), new int[]{rel_h.id, rel_p.id}, new Set[]{expected_grounding_set});
        Set<Record> expected_counter_example_set = new HashSet<>();
        for (int arg1 = 1; arg1 <= kb.totalConstants(); arg1++) {
            for (int arg2: new int[]{a, b, c, e}) {
                for (int arg4 = 1; arg4 <= kb.totalConstants(); arg4++) {
                    for (int arg6 = 1; arg6 <= kb.totalConstants(); arg6++) {
                        expected_counter_example_set.add(new Record(new int[]{arg1, arg2, arg1, arg4, a, arg6}));
                    }
                }
            }
        }
        expected_counter_example_set.remove(new Record(h2));
        assertEquals(expected_counter_example_set, rule.getCounterexamples());
        assertTrue(rel_h.isEntailed(h2));
    }

    @Test
    void testStructureConstructor2()  {
        /* h(?, X, X, ?) :- p(a, X, ?), q(X, ?, X), s(a), r(X) */
        int a = 1;
        int b = 2;
        int c = 3;
        int d = 4;
        int e = 5;
        int[] h1 = new int[]{a, b, c, d};
        int[] h2 = new int[]{a, c, c, d};//c
        int[] h3 = new int[]{d, e, e, a};//e
        int[] h4 = new int[]{b, b, b, b};//b
        int[] p1 = new int[]{a, b, b};//b
        int[] p2 = new int[]{a, c, d};//c
        int[] p3 = new int[]{c, c, c};
        int[] p4 = new int[]{b, d, d};
        int[] p5 = new int[]{e, a, a};
        int[] p6 = new int[]{a, b, c};//b
        int[] q1 = new int[]{b, a, b};//b
        int[] q2 = new int[]{b, c, d};//b
        int[] q3 = new int[]{c, e, c};//c
        int[] q4 = new int[]{a, b, c};
        int[] q5 = new int[]{c, d, e};
        int[] r1 = new int[]{a};
        int[] r2 = new int[]{b};
        int[] r3 = new int[]{d};
        int[] s1 = new int[]{a};
        int[] s2 = new int[]{b};
        SimpleKb kb = new SimpleKb("test", new int[][][] {
                new int[][] {h1, h2, h3, h4},
                new int[][] {p1, p2, p3, p4, p5, p6},
                new int[][] {q1, q2, q3, q4, q5},
                new int[][] {r1, r2, r3},
                new int[][] {s1, s2}
        }, new String[]{"h", "p", "q", "r", "s"});
        SimpleRelation rel_h = kb.getRelation("h");
        SimpleRelation rel_p = kb.getRelation("p");
        SimpleRelation rel_q = kb.getRelation("q");
        SimpleRelation rel_r = kb.getRelation("r");
        SimpleRelation rel_s = kb.getRelation("s");

        /* Construct */
        FragmentedCachedRule rule = new FragmentedCachedRule(new ArrayList<>(List.of(
                new Predicate(rel_h.id, new int[]{
                        Argument.EMPTY_VALUE, Argument.variable(0), Argument.variable(0), Argument.EMPTY_VALUE
                }),
                new Predicate(rel_p.id, new int[]{
                        Argument.constant(a), Argument.variable(0), Argument.EMPTY_VALUE
                }),
                new Predicate(rel_q.id, new int[]{
                        Argument.variable(0), Argument.EMPTY_VALUE, Argument.variable(0)
                }),
                new Predicate(rel_s.id, new int[]{Argument.constant(a)}),
                new Predicate(rel_r.id, new int[]{Argument.variable(0)}))),
                new HashSet<>(), new HashMap<>(), kb
        );
        assertEquals("h(?,X0,X0,?):-p(1,X0,?),q(X0,?,X0),s(1),r(X0)", rule.toDumpString(kb));
        assertEquals(new Eval(1, 1 * 5 * 5, 7), rule.getEval());
        assertEquals(1, rule.usedLimitedVars());
        assertEquals(7, rule.length());
        final Set<ComparableArray<Record>> expected_grounding_set1 = new HashSet<>();
        expected_grounding_set1.add(new ComparableArray<>(new Record[]{new Record(h4), new Record(p1), new Record(q1), new Record(s1), new Record(r2)}));
        final Set<ComparableArray<Record>> expected_grounding_set2 = new HashSet<>();
        expected_grounding_set2.add(new ComparableArray<>(new Record[]{new Record(h4), new Record(p1), new Record(q2), new Record(s1), new Record(r2)}));
        checkEvidence(rule.getEvidenceAndMarkEntailment(), new int[]{rel_h.id, rel_p.id, rel_q.id, rel_s.id, rel_r.id}, new Set[]{expected_grounding_set1, expected_grounding_set2});
        Set<Record> expected_counter_example_set = new HashSet<>();
        for (int arg1 = 1; arg1 <= kb.totalConstants(); arg1++) {
            for (int arg2: new int[]{b}) {
                for (int arg4 = 1; arg4 <= kb.totalConstants(); arg4++) {
                    expected_counter_example_set.add(new Record(new int[]{arg1, arg2, arg2, arg4}));
                }
            }
        }
        expected_counter_example_set.remove(new Record(h4));
        assertEquals(expected_counter_example_set, rule.getCounterexamples());
        assertTrue(rel_h.isEntailed(h4));
    }
}