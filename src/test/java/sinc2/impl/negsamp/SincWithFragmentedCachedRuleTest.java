package sinc2.impl.negsamp;

import org.junit.jupiter.api.Test;
import sinc2.SincConfig;
import sinc2.common.Argument;
import sinc2.common.Predicate;
import sinc2.common.SincException;
import sinc2.kb.KbException;
import sinc2.kb.SimpleCompressedKb;
import sinc2.kb.SimpleKb;
import sinc2.rule.BareRule;
import sinc2.rule.EvalMetric;
import sinc2.rule.Fingerprint;
import sinc2.rule.Rule;
import sinc2.util.datagen.FamilyRelationGenerator;
import sinc2.util.kb.NumerationMap;

import java.io.File;
import java.io.IOException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;

import static org.junit.jupiter.api.Assertions.*;

class SincWithFragmentedCachedRuleTest {

    static final String TMP_DIR = "/dev/shm/";

    @Test
    void testTinyHypothesis() throws KbException, IOException, SincException {
        /*
         * Hypothesis:
         *      gender(X, male) <- father(X, ?)
         *      gender(X, female) <- mother(X, ?)
         */
        String kb_name = "family.tiny." + UUID.randomUUID();
//        String compressed_kb_name = kb_name + ".comp";
        FamilyRelationGenerator.generateTiny(TMP_DIR, kb_name, 10, 0);
        SimpleKb kb = new SimpleKb(kb_name, TMP_DIR);
        Path kb_dir_path = Paths.get(TMP_DIR, kb_name);
        NumerationMap map = new NumerationMap(kb_dir_path.toString());
        int male = map.name2Num(FamilyRelationGenerator.Gender.MALE.getName());
        int female = map.name2Num(FamilyRelationGenerator.Gender.FEMALE.getName());

        final Predicate head1 = new Predicate(
                kb.getRelation(FamilyRelationGenerator.OtherRelation.GENDER.getName()).id,
                new int[]{Argument.variable(0), Argument.constant(male)}
        );
        final Predicate body1 = new Predicate(
                kb.getRelation(FamilyRelationGenerator.FamilyRelations.FATHER.getName()).id,
                new int[]{Argument.variable(0), Argument.EMPTY_VALUE}
        );
        BareRule r1 = new BareRule(new ArrayList<>(Arrays.asList(head1, body1)), new HashSet<>(), new HashMap<>());
        assertEquals(String.format("gender(X0,%d):-father(X0,?)", male), r1.toDumpString(kb));

        final Predicate head2 = new Predicate(
                kb.getRelation(FamilyRelationGenerator.OtherRelation.GENDER.getName()).id,
                new int[]{Argument.variable(0), Argument.constant(female)}
        );
        final Predicate body2 = new Predicate(
                kb.getRelation(FamilyRelationGenerator.FamilyRelations.MOTHER.getName()).id,
                new int[]{Argument.variable(0), Argument.EMPTY_VALUE}
        );
        BareRule r2 = new BareRule(new ArrayList<>(Arrays.asList(head2, body2)), new HashSet<>(), new HashMap<>());
        assertEquals(String.format("gender(X0,%d):-mother(X0,?)", female), r2.toDumpString(kb));

        final Set<Fingerprint> expected_rules = new HashSet<>();
        expected_rules.add(r1.getFingerprint());
        expected_rules.add(r2.getFingerprint());

        for (EvalMetric eval_type: new EvalMetric[]{
                EvalMetric.CompressionCapacity,
                EvalMetric.CompressionRatio
        }) {
            String compressed_kb_name = "family.tiny.comp." + UUID.randomUUID();
            System.out.println("CompKB: " + compressed_kb_name);
            final SincConfig config = new SincConfig(
                    TMP_DIR, kb_name, TMP_DIR, compressed_kb_name, 1, true, 5,
                    eval_type, 0.05, 0.25, 1, 1.0
            );
            Set<Fingerprint> rule_set_sinc = new HashSet<>();
            SincWithFragmentedCachedRule sinc = new SincWithFragmentedCachedRule(config);
            sinc.run();
            SimpleCompressedKb compressed_kb = sinc.getCompressedKb();
            for (Rule r: compressed_kb.getHypothesis()) {
                rule_set_sinc.add(r.getFingerprint());
            }

//            assertTrue(sinc.recover());  // Todo: uncomment here
            System.out.println("Actual Rules:");
            for (Rule rule: compressed_kb.getHypothesis()) {
                System.out.println(rule.toString(kb));
            }
            assertEquals(expected_rules, rule_set_sinc);
            deleteDir(Paths.get(TMP_DIR, compressed_kb_name).toFile());
        }
        deleteDir(kb_dir_path.toFile());
    }

    @Test
    void testSimpleHypothesis() throws KbException, IOException, SincException {
        /*
         * Hypothesis:
         *      gender(X,male):-father(X,?).
         *      gender(X,female):-mother(X,?).
         *      parent(X,Y):-father(X,Y).
         *      parent(X,Y):-mother(X,Y).
         */
        String kb_name = "family.simple." + UUID.randomUUID();
        String compressed_kb_name = kb_name + ".comp";
        FamilyRelationGenerator.generateSimple(TMP_DIR, kb_name, 10, 0);
        SimpleKb kb = new SimpleKb(kb_name, TMP_DIR);
        Path kb_dir_path = Paths.get(TMP_DIR, kb_name);
        NumerationMap map = new NumerationMap(kb_dir_path.toString());
        int male = map.name2Num(FamilyRelationGenerator.Gender.MALE.getName());
        int female = map.name2Num(FamilyRelationGenerator.Gender.FEMALE.getName());

        final Predicate head1 = new Predicate(
                kb.getRelation(FamilyRelationGenerator.OtherRelation.GENDER.getName()).id,
                new int[]{Argument.variable(0), Argument.constant(male)}
        );
        final Predicate body1 = new Predicate(
                kb.getRelation(FamilyRelationGenerator.FamilyRelations.FATHER.getName()).id,
                new int[]{Argument.variable(0), Argument.EMPTY_VALUE}
        );
        BareRule r1 = new BareRule(new ArrayList<>(Arrays.asList(head1, body1)), new HashSet<>(), new HashMap<>());
        assertEquals(String.format("gender(X0,%d):-father(X0,?)", male), r1.toDumpString(kb));

        final Predicate head2 = new Predicate(
                kb.getRelation(FamilyRelationGenerator.OtherRelation.GENDER.getName()).id,
                new int[]{Argument.variable(0), Argument.constant(female)}
        );
        final Predicate body2 = new Predicate(
                kb.getRelation(FamilyRelationGenerator.FamilyRelations.MOTHER.getName()).id,
                new int[]{Argument.variable(0), Argument.EMPTY_VALUE}
        );
        BareRule r2 = new BareRule(new ArrayList<>(Arrays.asList(head2, body2)), new HashSet<>(), new HashMap<>());
        assertEquals(String.format("gender(X0,%d):-mother(X0,?)", female), r2.toDumpString(kb));

        final Predicate head3 = new Predicate(
                kb.getRelation(FamilyRelationGenerator.FamilyRelations.PARENT.getName()).id,
                new int[]{
                        Argument.variable(0),
                        Argument.variable(1)
                }
        );
        final Predicate body3 = new Predicate(
                kb.getRelation(FamilyRelationGenerator.FamilyRelations.FATHER.getName()).id,
                new int[]{
                        Argument.variable(0),
                        Argument.variable(1)
                }
        );
        BareRule r3 = new BareRule(new ArrayList<>(Arrays.asList(head3, body3)), new HashSet<>(), new HashMap<>());
        assertEquals("parent(X0,X1):-father(X0,X1)", r3.toDumpString(kb));

        final Predicate head4 = new Predicate(
                kb.getRelation(FamilyRelationGenerator.FamilyRelations.PARENT.getName()).id,
                new int[]{
                        Argument.variable(0),
                        Argument.variable(1)
                }
        );
        final Predicate body4 = new Predicate(
                kb.getRelation(FamilyRelationGenerator.FamilyRelations.MOTHER.getName()).id,
                new int[]{
                        Argument.variable(0),
                        Argument.variable(1)
                }
        );
        BareRule r4 = new BareRule(new ArrayList<>(Arrays.asList(head4, body4)), new HashSet<>(), new HashMap<>());
        assertEquals("parent(X0,X1):-mother(X0,X1)", r4.toDumpString(kb));

        final Set<Fingerprint> expected_rules = new HashSet<>();
        expected_rules.add(r1.getFingerprint());
        expected_rules.add(r2.getFingerprint());
        expected_rules.add(r3.getFingerprint());
        expected_rules.add(r4.getFingerprint());

        for (EvalMetric eval_type: new EvalMetric[]{
                EvalMetric.CompressionCapacity,
                EvalMetric.CompressionRatio
        }) {
            final SincConfig config = new SincConfig(
                    TMP_DIR, kb_name, TMP_DIR, compressed_kb_name, 1, true, 5,
                    eval_type, 0.05, 0.25, 1, 1.0
            );
            Set<Fingerprint> rule_set_sinc = new HashSet<>();
            SincWithFragmentedCachedRule sinc = new SincWithFragmentedCachedRule(config);
            sinc.run();
            SimpleCompressedKb compressed_kb = sinc.getCompressedKb();
            for (Rule r: compressed_kb.getHypothesis()) {
                rule_set_sinc.add(r.getFingerprint());
            }

//            assertTrue(sinc.recover());  // Todo: uncomment here
            System.out.println("Actual Rules:");
            for (Rule rule: compressed_kb.getHypothesis()) {
                System.out.println(rule.toString(kb));
            }
            assertEquals(expected_rules, rule_set_sinc);
            deleteDir(Paths.get(TMP_DIR, compressed_kb_name).toFile());
        }
        deleteDir(kb_dir_path.toFile());
    }

    private void deleteDir(File file) {
        File[] contents = file.listFiles();
        if (contents != null) {
            for (File f : contents) {
                deleteDir(f);
            }
        }
        assertTrue(file.delete());
    }
}