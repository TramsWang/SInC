package sinc2.kb;

import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import sinc2.common.Argument;
import sinc2.common.Predicate;
import sinc2.common.Record;
import sinc2.rule.BareRule;
import sinc2.rule.Rule;
import sinc2.util.kb.TestKbManager;

import java.io.IOException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

class SimpleCompressedKbTest {

    static TestKbManager testKbManager;

    @BeforeAll
    static void setupKb() throws IOException {
        testKbManager = new TestKbManager();
    }

    @AfterAll
    static void removeKb() {
        testKbManager.cleanUpKb();
    }

    @Test
    void testCreateAndDump() throws IOException, KbException {
        SimpleKb kb = new SimpleKb(testKbManager.getKbName(), TestKbManager.MEM_DIR);
        kb.setAsEntailed("family", new int[]{4, 5, 6});
        kb.setAsEntailed("family", new int[]{7, 8, 9});
        kb.setAsEntailed("family", new int[]{10, 11, 12});
        kb.setAsEntailed("father", new int[]{5, 6});
        kb.setAsEntailed("father", new int[]{8, 9});
        kb.setAsEntailed("father", new int[]{11, 12});
        kb.setAsEntailed("mother", new int[]{4, 6});
        kb.setAsEntailed("mother", new int[]{7, 9});
        kb.setAsEntailed("mother", new int[]{10, 12});
        SimpleRelation rel_family = kb.getRelation("family");
        SimpleRelation rel_mother = kb.getRelation("mother");
        SimpleRelation rel_father = kb.getRelation("father");

        SimpleCompressedKb ckb = new SimpleCompressedKb(testKbManager.getCkbName(), kb);
        ckb.addFvsRecord(rel_family.id, new int[]{10, 11, 12});

        List<Predicate> rule_father_structure = List.of(
                new Predicate(rel_father.id, new int[]{
                        Argument.variable(0), Argument.variable(1)
                }),
                new Predicate(rel_family.id, new int[]{
                        Argument.EMPTY_VALUE, Argument.variable(0), Argument.variable(1)
                })
        );
        Rule rule_father = new BareRule(rule_father_structure, new HashSet<>(), new HashMap<>());
        List<Predicate> rule_mother_structure = List.of(
                new Predicate(rel_mother.id, new int[]{
                        Argument.variable(0), Argument.constant(6)
                }),
                new Predicate(rel_family.id, new int[]{
                        Argument.variable(0), Argument.EMPTY_VALUE, Argument.constant(6)
                })
        );
        Rule rule_mother = new BareRule(rule_mother_structure, new HashSet<>(), new HashMap<>());

        assertEquals("father(X0,X1):-family(?,X0,X1)", rule_father.toDumpString(kb));
        assertEquals("mother(X0,6):-family(X0,?,6)", rule_mother.toDumpString(kb));
        ckb.addHypothesisRules(List.of(rule_father, rule_mother));

        ckb.addCounterexamples(rel_mother.id, List.of(new Record(new int[]{5, 5})));
        ckb.addCounterexamples(rel_father.id, List.of(
                new Record(new int[]{16, 17}),
                new Record(new int[]{14, 15})
        ));
        ckb.updateSupplementaryConstants();

        assertEquals(testKbManager.getCkbName(), ckb.getName());
        assertEquals(4, ckb.totalNecessaryRecords());
        assertEquals(1, ckb.totalFvsRecords());
        assertEquals(3, ckb.totalCounterexamples());
        assertEquals(5, ckb.totalHypothesisSize());
        assertEquals(7, ckb.totalSupplementaryConstants()); // In this test, numbers 1, 2, and 3 should be taken into consideration
        assertEquals(new HashSet<>(List.of(1, 2, 3, 4, 7, 8, 9)), ckb.supplementaryConstants);

        /* Dump */
        String tmp_dir_path = testKbManager.createTmpDir();
        ckb.dump(tmp_dir_path);
        SimpleKb kb2 = new SimpleKb(ckb.getName(), tmp_dir_path);
        assertEquals(4, kb2.totalRecords());
        assertTrue(kb2.hasRecord("family", new int[]{10, 11, 12}));
        assertTrue(kb2.hasRecord("family", new int[]{13, 14, 15}));
        assertTrue(kb2.hasRecord("father", new int[]{16, 17}));
        assertTrue(kb2.hasRecord("mother", new int[]{13, 15}));
    }
}