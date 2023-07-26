package sinc2.impl.negsamp;

import org.junit.jupiter.api.Test;
import sinc2.SincConfig;
import sinc2.common.SincException;
import sinc2.kb.*;
import sinc2.rule.EvalMetric;
import sinc2.rule.Fingerprint;
import sinc2.rule.Rule;
import sinc2.util.datagen.FamilyRelationGenerator;

import java.io.File;
import java.io.IOException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;

import static org.junit.jupiter.api.Assertions.*;

class SincWithNegSamplingTest {

    static final String TMP_DIR = "/dev/shm/";

    @Test
    void testTinyHypothesis() throws KbException, IOException, SincException {
        /*
         * Hypothesis:
         *      gender(X, male) <- father(X, ?)
         *      gender(X, female) <- mother(X, ?)
         */
        String kb_name = "family.tiny." + UUID.randomUUID();
        FamilyRelationGenerator.generateTiny(TMP_DIR, kb_name, 10, 0);
        SimpleKb kb = new SimpleKb(kb_name, TMP_DIR);
        Path kb_dir_path = Paths.get(TMP_DIR, kb_name);
        NegSampleKb neg_kb = buildNegKb(kb);

        /* Test equality */
        for (EvalMetric eval_type: new EvalMetric[]{
                EvalMetric.CompressionCapacity,
                EvalMetric.CompressionRatio
        }) {
            String compressed_kb_name = "family.tiny.comp." + UUID.randomUUID();
            System.out.println("CompKB: " + compressed_kb_name);
            final SincConfig config = new SincConfig(
                    TMP_DIR, kb_name, TMP_DIR, compressed_kb_name, 1, true, 5,
                    eval_type, 0.05, 0.25, 1, 1.0,
                    null, null, 0, false
            );
            SincWithFragmentedCachedRule sinc = new SincWithFragmentedCachedRule(config);
            sinc.run();
            Set<Fingerprint> expected_rules = new HashSet<>();
            for (Rule r: sinc.getCompressedKb().getHypothesis()) {
                expected_rules.add(r.getFingerprint());
            }

            /* Test neg-sampling */
            SincWithNegSampling neg_sinc = new SincWithNegSampling(config, null, neg_kb);
            neg_sinc.run();
            Set<Fingerprint> actual_rules = new HashSet<>();
            for (Rule r: neg_sinc.getCompressedKb().getHypothesis()) {
                actual_rules.add(r.getFingerprint());
            }
            assertEquals(expected_rules, actual_rules);

            System.out.println("Test Passed with: " + eval_type);
            System.out.flush();
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
        FamilyRelationGenerator.generateSimple(TMP_DIR, kb_name, 10, 0);
        SimpleKb kb = new SimpleKb(kb_name, TMP_DIR);
        Path kb_dir_path = Paths.get(TMP_DIR, kb_name);
        NegSampleKb neg_kb = buildNegKb(kb);

        for (EvalMetric eval_type: new EvalMetric[]{
                EvalMetric.CompressionCapacity,
                EvalMetric.CompressionRatio
        }) {
            String compressed_kb_name = "family.tiny.comp." + UUID.randomUUID();
            System.out.println("CompKB: " + compressed_kb_name);
            final SincConfig config = new SincConfig(
                    TMP_DIR, kb_name, TMP_DIR, compressed_kb_name, 1, true, 5,
                    eval_type, 0.05, 0.25, 1, 1.0,
                    TMP_DIR, neg_kb.getName(), 0, false
            );
            SincWithFragmentedCachedRule sinc = new SincWithFragmentedCachedRule(config);
            sinc.run();
            Set<Fingerprint> expected_rules = new HashSet<>();
            for (Rule r: sinc.getCompressedKb().getHypothesis()) {
                expected_rules.add(r.getFingerprint());
            }

            /* Test neg-sampling */
            SincWithNegSampling neg_sinc = new SincWithNegSampling(config, null, neg_kb);
            neg_sinc.run();
            Set<Fingerprint> actual_rules = new HashSet<>();
            for (Rule r: neg_sinc.getCompressedKb().getHypothesis()) {
                actual_rules.add(r.getFingerprint());
            }
            assertEquals(expected_rules, actual_rules);

            System.out.println("Test Passed with: " + eval_type);
            System.out.flush();
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

    private NegSampleKb buildNegKb(SimpleKb kb) {
        SimpleRelation[] relations = kb.getRelations();
        IntTable[] neg_tables = new IntTable[relations.length];
        for (int rel_idx = 0; rel_idx < relations.length; rel_idx++) {
            SimpleRelation relation = relations[rel_idx];
            List<int[]> neg_records = new ArrayList<>();
            for (int arg1 = 1; arg1 < kb.totalConstants(); arg1++) {
                for (int arg2 = 1; arg2 < kb.totalConstants(); arg2++) {
                    int[] record = new int[]{arg1, arg2};
                    if (!relation.hasRow(record)) {
                        neg_records.add(record);
                    }
                }
            }
            neg_tables[rel_idx] = new IntTable(neg_records.toArray(new int[0][]));
        }
        return new NegSampleKb(kb.getName() + "neg", neg_tables, new Map[relations.length]);
    }
}