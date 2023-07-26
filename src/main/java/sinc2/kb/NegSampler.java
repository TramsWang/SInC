package sinc2.kb;

import sinc2.common.Predicate;
import sinc2.common.Record;
import sinc2.exp.negsample.TestConstantRelevance;
import sinc2.impl.negsamp.CB;
import sinc2.impl.negsamp.CacheFragment;
import sinc2.rule.Rule;
import sinc2.util.ArrayOperation;
import sinc2.util.MultiSet;
import sinc2.util.io.IntReader;
import sinc2.util.kb.NumeratedKb;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.file.Paths;
import java.util.*;
import java.util.concurrent.ThreadLocalRandom;

/**
 * This class is used for negative sampling from KBs under closed-word assumption (CWA). Five strategies are implemented
 * currently:
 *   1. Uniform Sampling
 *   2. Positive-relative Sampling
 *   3. Negative Interval Sampling
 *   4. Constant Relevance Sampling
 *   5. Adversarial Sampling
 *
 * This class also calculates the weight of each negative sample w.r.t. the given KB.
 *
 * NOTE: Constant symbols are labeled by integers from 1 to n (n is the number of constants involved in the KB). 0 is not
 * a valid label for denoting constant symbols. Therefore, a record can be recognized as a (n-1)-based integer.
 *
 * @since 2.3
 */
public class NegSampler {

    /**
     * This method samples negative records, w.r.t. a set of positive records, under uniform probability, i.e., each
     * negative sample is selected with equal probability.
     *
     * @param posRelation    The set of positive records
     * @param totalConstants The number of constants involved in the KB
     * @param budget         The number of negative samples
     * @return A set of negative samples organized as an IntTable
     */
    public static IntTable uniformSampling(IntTable posRelation, int totalConstants, int budget) {
        final int[][] neg_samples = new int[budget][];
        final int rand_upper_bound = totalConstants + 1;
        final int arity = posRelation.totalCols();
        Set<Record> neg_sample_set = new HashSet<>();
        for (int i = 0; i < budget; ) {
            int[] neg_sample = new int[arity];
            for (int arg_idx = 0; arg_idx < arity; arg_idx++) {
                neg_sample[arg_idx] = ThreadLocalRandom.current().nextInt(1, rand_upper_bound);
            }
            if (!posRelation.hasRow(neg_sample) && neg_sample_set.add(new Record(neg_sample))) {
                neg_samples[i] = neg_sample;
                i++;
            }
        }
        return new IntTable(neg_samples);
    }

    /**
     * This method samples negative records by randomly modifying positive records.
     *
     * @param posRelation    The set of positive records
     * @param totalConstants The number of constants involved in the KB
     * @param budget         The number of negative samples
     * @return A set of negative samples organized as an IntTable
     */
    public static IntTable posRelativeSampling(IntTable posRelation, int totalConstants, int budget) {
        final int[][] neg_samples = new int[budget][];
        final int rand_upper_bound = totalConstants + 1;
        final int arity = posRelation.totalCols();
        final int pos_rows = posRelation.totalRows();
        final int[][] pos_records = posRelation.getAllRows();
        Set<Record> neg_sample_set = new HashSet<>();
        for (int i = 0; i < budget; ) {
            int pos_row_idx = ThreadLocalRandom.current().nextInt(pos_rows);
            int pos_col_idx = ThreadLocalRandom.current().nextInt(arity);
            int another_const = ThreadLocalRandom.current().nextInt(1, rand_upper_bound);
            int[] neg_sample = pos_records[pos_row_idx].clone();
            neg_sample[pos_col_idx] = another_const;
            if (!posRelation.hasRow(neg_sample) && neg_sample_set.add(new Record(neg_sample))) {
                neg_samples[i] = neg_sample;
                i++;
            }
        }
        return new IntTable(neg_samples);
    }

    /**
     * This method samples negative records from negative intervals. The lower and upper bounds of each negative interval
     * are selected. Therefore, at most 2n+2 (n is the number of positive records) negative samples will be selected under
     * this strategy.
     *
     * @param posRelation    The set of positive records
     * @param totalConstants The number of constants involved in the KB
     * @param enhance        If > 0, split the alphabetically ordered list of all possible records into many intervals,
     *                       the number of which is "enhance", and sample the beginning record of each interval to enhance
     *                       the sample set
     * @return A set of negative samples organized as an IntTable
     */
    public static IntTable negIntervalSampling(IntTable posRelation, int totalConstants, int enhance) {
        Set<Record> neg_samples = new HashSet<>();
        int[][] pos_records = posRelation.getAllRows();

        /* Neg interval beginnings */
        int[] first_rec = ArrayOperation.initArrayWithValue(posRelation.totalCols(), 1);
        if (0 != IntTable.rowComparator.compare(first_rec, pos_records[0])) {
            neg_samples.add(new Record(first_rec));
        }

        int lim = pos_records.length - 1;
        for (int i = 0; i < lim; i++) {
            int[] next_record = nextRecord(pos_records[i], totalConstants);
            if (0 != IntTable.rowComparator.compare(next_record, pos_records[i+1])) {
                neg_samples.add(new Record(next_record));
            }
        }
        int[] next_record = nextRecord(pos_records[lim], totalConstants);
        if (0 != IntTable.rowComparator.compare(next_record, pos_records[lim])) {
            neg_samples.add(new Record(next_record));
        }

        /* Neg interval endings */
        int[] last_rec = ArrayOperation.initArrayWithValue(posRelation.totalCols(), totalConstants);
        if (0 != IntTable.rowComparator.compare(last_rec, pos_records[lim])) {
            neg_samples.add(new Record(last_rec));
        }
        for (int i = 1; i < pos_records.length; i++) {
            int[] prev_record = previousRecord(pos_records[i], totalConstants);
            if (0 != IntTable.rowComparator.compare(prev_record, pos_records[i-1])) {
                neg_samples.add(new Record(prev_record));
            }
        }
        int[] prev_record = previousRecord(pos_records[0], totalConstants);
        if (0 != IntTable.rowComparator.compare(prev_record, pos_records[0])) {
            neg_samples.add(new Record(prev_record));
        }

        return new IntTable(collectionToArray(neg_samples));
    }

    /**
     * This method samples negative records from negative intervals. The lower bound of each negative interval is selected.
     * Therefore, at most 2n+2 (n is the number of positive records) negative samples will be selected under this strategy.
     *
     * @param posRelation    The set of positive records
     * @param totalConstants The number of constants involved in the KB
     * @param enhance        If > 0, split the alphabetically ordered list of all possible records into many intervals,
     *                       the number of which is "enhance", and sample the beginning record of each interval to enhance
     *                       the sample set
     * @return A set of negative samples organized as an IntTable
     */
    public static IntTable negIntervalBeginningSampling(IntTable posRelation, int totalConstants, int enhance) {
        Set<Record> neg_samples = new HashSet<>();
        int[][] pos_records = posRelation.getAllRows();

        /* Neg interval beginnings */
        int[] first_rec = ArrayOperation.initArrayWithValue(posRelation.totalCols(), 1);
        if (0 != IntTable.rowComparator.compare(first_rec, pos_records[0])) {
            neg_samples.add(new Record(first_rec));
        }

        int lim = pos_records.length - 1;
        for (int i = 0; i < lim; i++) {
            int[] next_record = nextRecord(pos_records[i], totalConstants);
            if (0 != IntTable.rowComparator.compare(next_record, pos_records[i+1])) {
                neg_samples.add(new Record(next_record));
            }
        }
        int[] next_record = nextRecord(pos_records[lim], totalConstants);
        if (0 != IntTable.rowComparator.compare(next_record, pos_records[lim])) {
            neg_samples.add(new Record(next_record));
        }

        enhanceViaEvenlySampling(posRelation, totalConstants, neg_samples, enhance);

        return new IntTable(collectionToArray(neg_samples));
    }

    /**
     * This method samples negative records from negative intervals. The upper bounds of each negative interval is selected.
     * Therefore, at most 2n+2 (n is the number of positive records) negative samples will be selected under this strategy.
     *
     * @param posRelation    The set of positive records
     * @param totalConstants The number of constants involved in the KB
     * @param enhance        If > 0, split the alphabetically ordered list of all possible records into many intervals,
     *                       the number of which is "enhance", and sample the beginning record of each interval to enhance
     *                       the sample set
     * @return A set of negative samples organized as an IntTable
     */
    public static IntTable negIntervalEndingSampling(IntTable posRelation, int totalConstants, int enhance) {
        Set<Record> neg_samples = new HashSet<>();
        int[][] pos_records = posRelation.getAllRows();
        int lim = pos_records.length - 1;

        /* Neg interval endings */
        int[] last_rec = ArrayOperation.initArrayWithValue(posRelation.totalCols(), totalConstants);
        if (0 != IntTable.rowComparator.compare(last_rec, pos_records[lim])) {
            neg_samples.add(new Record(last_rec));
        }
        for (int i = 1; i < pos_records.length; i++) {
            int[] prev_record = previousRecord(pos_records[i], totalConstants);
            if (0 != IntTable.rowComparator.compare(prev_record, pos_records[i-1])) {
                neg_samples.add(new Record(prev_record));
            }
        }
        int[] prev_record = previousRecord(pos_records[0], totalConstants);
        if (0 != IntTable.rowComparator.compare(prev_record, pos_records[0])) {
            neg_samples.add(new Record(prev_record));
        }

        enhanceViaEvenlySampling(posRelation, totalConstants, neg_samples, enhance);

        return new IntTable(collectionToArray(neg_samples));
    }

    /**
     * This method samples negative records from alphabetical neighbors of positive records. For each positive record r,
     * generate at most d relevant negative samples, where d is the arity of r:
     *   - For each dimension i = 1, 2, ..., d, change r.i to r.i + 1
     *   - If the new record is not in the KB, add the record to the negative sample set
     *
     * @param posRelation    The set of positive records
     * @param totalConstants The number of constants involved in the KB
     * @param enhance        If > 0, split the alphabetically ordered list of all possible records into many intervals,
     *                       the number of which is "enhance", and sample the beginning record of each interval to enhance
     *                       the sample set
     * @return A set of negative samples organized as an IntTable
     */
    public static IntTable alphaNeighborUpperSampling(IntTable posRelation, int totalConstants, int enhance) {
        Set<Record> neg_samples = new HashSet<>();

        for (int[] pos_record: posRelation.getAllRows()) {
            for (int arg_idx = 0; arg_idx < pos_record.length; arg_idx++) {
                /* r.i + 1 */
                if (pos_record[arg_idx] < totalConstants) {
                    int[] new_rec = pos_record.clone();
                    new_rec[arg_idx]++;
                    if (!posRelation.hasRow(new_rec)) {
                        neg_samples.add(new Record(new_rec));
                    }
                }
            }
        }

        enhanceViaEvenlySampling(posRelation, totalConstants, neg_samples, enhance);

        return new IntTable(collectionToArray(neg_samples));
    }

    /**
     * Enhance negative sampling sets by evenly selecting records from the alphabetically sorted list of all possible
     * records.
     *
     * @param enhance The number of samples selected from the overall sorted list
     */
    protected static void enhanceViaEvenlySampling(
            IntTable posRelation, int totalConstants, Set<Record> negSamples, int enhance
    ) {
        if (enhance <= 0) {
            return;
        }

        final int step_size = posRelation.totalRows() / enhance + ((0 == posRelation.totalRows() % enhance) ? 0 : 1);
        int[] record_template = ArrayOperation.initArrayWithValue(posRelation.totalCols(), 1);
        for (int i = 0; i < enhance; i++) {
            if (!posRelation.hasRow(record_template)) {
                negSamples.add(new Record(record_template));
            }
            record_template = nextRecord(record_template, totalConstants, step_size);
        }
    }

    /**
     * This method samples negative records from alphabetical neighbors of positive records. For each positive record r,
     * generate at most 2d relevant negative samples, where d is the arity of r:
     *   - For each dimension i = 1, 2, ..., d, change r.i to r.i + 1 and r.i - 1
     *   - If the new record is not in the KB, add the record to the negative sample set
     *
     * @param posRelation    The set of positive records
     * @param totalConstants The number of constants involved in the KB
     * @param enhance        If > 0, split the alphabetically ordered list of all possible records into many intervals,
     *                       the number of which is "enhance", and sample the beginning record of each interval to enhance
     *                       the sample set
     * @return A set of negative samples organized as an IntTable
     */
    public static IntTable alphaNeighborLowerSampling(IntTable posRelation, int totalConstants, int enhance) {
        Set<Record> neg_samples = new HashSet<>();

        for (int[] pos_record: posRelation.getAllRows()) {
            for (int arg_idx = 0; arg_idx < pos_record.length; arg_idx++) {
                /* r.i - 1*/
                if (pos_record[arg_idx] > 1) {
                    int[] new_rec = pos_record.clone();
                    new_rec[arg_idx]--;
                    if (!posRelation.hasRow(new_rec)) {
                        neg_samples.add(new Record(new_rec));
                    }
                }
            }
        }

        enhanceViaEvenlySampling(posRelation, totalConstants, neg_samples, enhance);

        return new IntTable(collectionToArray(neg_samples));
    }

    /**
     * This method samples negative records from alphabetical neighbors of positive records. For each positive record r,
     * generate at most 2d relevant negative samples, where d is the arity of r:
     *   - For each dimension i = 1, 2, ..., d, change r.i to r.i + 1 and r.i - 1
     *   - If the new record is not in the KB, add the record to the negative sample set
     *
     * @param posRelation    The set of positive records
     * @param totalConstants The number of constants involved in the KB
     * @param enhance        If > 0, split the alphabetically ordered list of all possible records into many intervals,
     *                       the number of which is "enhance", and sample the beginning record of each interval to enhance
     *                       the sample set
     * @return A set of negative samples organized as an IntTable
     */
    public static IntTable alphaNeighborSampling(IntTable posRelation, int totalConstants, int enhance) {
        Set<Record> neg_samples = new HashSet<>();

        for (int[] pos_record: posRelation.getAllRows()) {
            for (int arg_idx = 0; arg_idx < pos_record.length; arg_idx++) {
                /* r.i + 1 */
                if (pos_record[arg_idx] < totalConstants) {
                    int[] new_rec = pos_record.clone();
                    new_rec[arg_idx]++;
                    if (!posRelation.hasRow(new_rec)) {
                        neg_samples.add(new Record(new_rec));
                    }
                }

                /* r.i - 1*/
                if (pos_record[arg_idx] > 1) {
                    int[] new_rec = pos_record.clone();
                    new_rec[arg_idx]--;
                    if (!posRelation.hasRow(new_rec)) {
                        neg_samples.add(new Record(new_rec));
                    }
                }
            }
        }

        enhanceViaEvenlySampling(posRelation, totalConstants, neg_samples, enhance);

        return new IntTable(collectionToArray(neg_samples));
    }

//    /**
//     * This method generates a negative KB by constant relevance. The method reads a partially completed KB data, where
//     * negative samples are given without weight, from the dump written by a Python script "gen_neg_con_rel.py" in this
//     * package. The parameters of this method are used to find the intermediate dump. This method will generate kdn
//     * negative samples, where k is the parameter "TopK", d is the arity of the relation, and n is the number of records
//     * in the positive relation.
//     *
//     * @param kb   The positive KB
//     * @param hop  The number of hops used to calculate the relevance matrix
//     * @param topK Top k relevant constants used to generate neg samples
//     */
//    public static void constantRelevanceSampling(SimpleKb kb, int hop, int topK, String basePath) throws IOException {
//        String neg_kb_name = String.format("%s_neg_con_rel_h%dk%di", kb.name, hop, topK);
//        completeWeight(neg_kb_name, basePath, kb);
//    }

//    /**
//     * This method generates a negative KB by constant relevance. The method reads a partially completed KB data, where
//     * negative samples are given without weight, from the dump written by a Python script "gen_neg_con_rel.py" in this
//     * package. The parameters of this method are used to find the intermediate dump. This method will generate rdn
//     * negative samples, where r is the parameter "round", d is the arity of the relation, and n is the number of records
//     * in the positive relation.
//     *
//     * @param kb         The positive KB
//     * @param hop        The number of hops used to calculate the relevance matrix
//     * @param percentage Top percentages of relevant constants used to generate neg samples
//     * @param rounds     The number of alternated constants generated from one argument in a positive record
//     */
//    public static void constantRelevanceSampling(SimpleKb kb, int hop, int percentage, int rounds, String basePath) throws IOException {
//        String neg_kb_name = String.format("%s_neg_con_rel_h%dp%dr%d", kb.name, hop, percentage, rounds);
//        completeWeight(neg_kb_name, basePath, kb);
//    }

//    /**
//     * This method generates a negative KB by constant relevance. The method reads a partially completed KB data, where
//     * negative samples are given without weight, from the dump written by a Python script "gen_neg_con_rel.py" in this
//     * package. The parameters of this method are used to find the intermediate dump. This method will generate kdn
//     * negative samples, where k is the parameter "TopK", d is the arity of the relation, and n is the number of records
//     * in the positive relation.
//     *
//     * @param kb     The positive KB
//     * @param hop    The number of hops used to calculate the relevance matrix
//     * @param rounds The number of alternated constants generated from one argument in a positive record
//     */
//    public static void constantRelevanceSampling(SimpleKb kb, int hop, int rounds, String basePath) throws IOException {
//        String neg_kb_name = String.format("%s_neg_con_rel_h%dr%d", kb.name, hop, rounds);
//        completeWeight(neg_kb_name, basePath, kb);
//    }

//    /**
//     * This method is only used to complete the weight of negative samples in a partially completed neg KB and dump the
//     * completed KB to the original location.
//     */
//    protected static void completeWeight(String negKbName, String basePath, SimpleKb kb) throws IOException {
//        /* Load relation information */
//        String dir_path = NumeratedKb.getKbPath(negKbName, basePath).toString();
//        File rel_info_file = Paths.get(dir_path, NegSampleKb.REL_INFO_FILE_NAME).toFile();
//        IntReader rel_info_reader = new IntReader(rel_info_file);
//        int total_relations = rel_info_reader.next();
//
//        IntTable[] negSampleTabs = new IntTable[total_relations];
//        Map<Record, Float>[] negSampleWeightMaps = new Map[total_relations];
//        for (int rel_id = 0; rel_id < total_relations; rel_id++) {
//            /* Load negative samples and weight */
//            int arity = rel_info_reader.next();
//            int total_records = rel_info_reader.next();
//            int[][] neg_samples = SimpleRelation.loadFile(
//                    Paths.get(dir_path, NegSampleKb.getNegRelFileName(rel_id)).toFile(), arity, total_records
//            );
//            IntTable neg_table = new IntTable(neg_samples);
//            Map<Record, Float> weight_map = calcNegSampleWeight(kb.getRelation(rel_id), neg_table, kb.totalConstants());
//            negSampleWeightMaps[rel_id] = weight_map;
//            negSampleTabs[rel_id] = neg_table;
//        }
//        rel_info_reader.close();
//
//        /* Dump new neg KB */
//        NegSampleKb neg_kb = new NegSampleKb(negKbName, negSampleTabs, negSampleWeightMaps);
//        neg_kb.dump(basePath);
//    }

    /**
     * This method is used for sampling according to constant relevance. The total number of negative samples generated
     * from a relation R by this method is drn, where d is the arity of R, r is the parameter "rounds", and n is the number
     * of records in R.
     *
     * NOTE: Here the KB is assumed to be a KG, where all relations are binary.
     *
     * @param steps >= 1
     */
    public static NegSampleKb constantRelevanceSampling(SimpleKb kb, int steps, int rounds) {
        System.out.printf("Constant Relevance Sampling: s=%d, r=%d\n", steps, rounds);
        long mem_start = Runtime.getRuntime().totalMemory();

        /* Calculate Constant Relevance */
        long time_start = System.currentTimeMillis();
        int[][] relevant_const_lists = calcRelevantConstants(kb);   // const numeration is the index of the array

        /* Generate Neg KB */
        int total_constants = kb.totalConstants();
        SimpleRelation[] relations = kb.getRelations();
        IntTable[] neg_tables = new IntTable[relations.length];
        Map<Record, Float>[] weight_maps = new Map[relations.length];
        Set<Integer> hard_constants = new HashSet<>();
        for (int rel_idx = 0; rel_idx < relations.length; rel_idx++) {
//            System.out.printf("Relation #%d\n", rel_idx);
            SimpleRelation relation = relations[rel_idx];
            IntTable neg_table = NegSampler.constantRelevanceSampling(
                    relation, relevant_const_lists, steps, rounds, total_constants, hard_constants
            );
            neg_tables[rel_idx] = neg_table;
            weight_maps[rel_idx] = NegSampler.calcNegSampleWeight(relation, neg_table, total_constants);
        }
        long time_done = System.currentTimeMillis();
        long mem_done = Runtime.getRuntime().totalMemory();
        System.out.printf("Hard constants: %d\n", hard_constants.size());
        System.out.printf("Time Cost: %d(ms)\n", time_done - time_start);
        System.out.printf("Memory Cost: %d(B)\n", mem_done - mem_start);
        System.out.println();
        return new NegSampleKb(String.format("%s_neg_con_rel_h%dr%d", kb.getName(), steps - 1, rounds), neg_tables, weight_maps);
    }

    protected static int[][] calcRelevantConstants(SimpleKb kb) {
        Set<Integer>[] relevant_constant_sets = new Set[kb.totalConstants() + 1];
        for (int c = 1; c < relevant_constant_sets.length; c++) {
            relevant_constant_sets[c] = new HashSet<>();
        }

        for (SimpleRelation relation: kb.getRelations()) {
            for (int[] record: relation) {
                /* Here I made an assumption all relations in tested KBs are binary */
                final int c1 = record[0];
                final int c2 = record[1];
                relevant_constant_sets[c1].add(c2);
                relevant_constant_sets[c2].add(c1);
            }
        }

        int[][] relevant_constant_lists = new int[kb.totalConstants() + 1][];
        for (int constant = 1; constant < relevant_constant_lists.length; constant++) {
            relevant_constant_lists[constant] = ArrayOperation.toArray(relevant_constant_sets[constant]);
        }
        return relevant_constant_lists;
    }

    protected static IntTable constantRelevanceSampling(
            SimpleRelation posRelation, int[][] relevantContLists, int steps, int rounds, int totalConstants, Set<Integer> hardConstants
    ) {
        final int MAX_ATTEMPTS = 2 * steps * rounds;
        int idx_base = 0;
        int steps_base = 0;
        Set<Record> neg_sample_set = new HashSet<>();
        final int[][] pos_records = posRelation.getAllRows();
        for (int r = 0; r < rounds; r++) {
            for (int[] pos_record : pos_records) {
                /* Change the 2nd arg */
                int[] neg_sample = pos_record.clone();
                for (int attempts = 0; attempts < MAX_ATTEMPTS; attempts++) {
                    if (hardConstants.contains(neg_sample[0])) {
                        /* Initiate hard mode */
                        attempts = MAX_ATTEMPTS - 1;
                    } else {
                        int _steps = (steps_base % steps) + 1;
                        steps_base++;
                        int current_const = neg_sample[0];
                        for (int s = 0; s < _steps; s++) {
                            int[] rel_consts = relevantContLists[current_const];
                            current_const = rel_consts[idx_base % rel_consts.length];
                            idx_base++;
                        }
                        neg_sample[1] = current_const;
                        if (!posRelation.hasRow(neg_sample) && neg_sample_set.add(new Record(neg_sample))) {
                            break;
                        }
                    }
                    if (MAX_ATTEMPTS - 1 == attempts) {
                        /* If reaches here then it is hard to find a negative sample for the record, try arbitrary const */
                        hardConstants.add(neg_sample[0]);
                        for (int attempts2 = 0; attempts2 < MAX_ATTEMPTS; attempts2++) {
                            neg_sample[1] = idx_base % totalConstants + 1;
                            idx_base++;
                            if (!posRelation.hasRow(neg_sample) && neg_sample_set.add(new Record(neg_sample))) {
                                break;
                            }
                        }
                        /* If reaches here and negative sample is still not found, it means it is really hard to find a
                           negative sample. Give up for this record. */
                    }
                }

                /* Change the 1st arg */
                neg_sample = pos_record.clone();
                for (int attempts = 0; attempts < MAX_ATTEMPTS; attempts++) {
                    if (hardConstants.contains(neg_sample[1])) {
                        /* Initiate hard mode */
                        attempts = MAX_ATTEMPTS - 1;
                    } else {
                        int _steps = (steps_base % steps) + 1;
                        steps_base++;
                        int current_const = neg_sample[1];
                        for (int s = 0; s < _steps; s++) {
                            int[] rel_consts = relevantContLists[current_const];
                            current_const = rel_consts[idx_base % rel_consts.length];
                            idx_base++;
                        }
                        neg_sample[0] = current_const;
                        if (!posRelation.hasRow(neg_sample) && neg_sample_set.add(new Record(neg_sample))) {
                            break;
                        }
                    }
                    if (MAX_ATTEMPTS - 1 == attempts) {
                        /* If reaches here then it is hard to find a negative sample for the record, try arbitrary const */
                        hardConstants.add(neg_sample[1]);
                        for (int attempts2 = 0; attempts2 < MAX_ATTEMPTS; attempts2++) {
                            neg_sample[0] = idx_base % totalConstants + 1;
                            idx_base++;
                            if (!posRelation.hasRow(neg_sample) && neg_sample_set.add(new Record(neg_sample))) {
                                break;
                            }
                        }
                        /* If reaches here and negative sample is still not found, it means it is really hard to find a
                           negative sample. Give up for this record. */
                    }
                }
            }
        }
        final int[][] neg_samples = new int[neg_sample_set.size()][];  // Assume the relation to be binary
        int idx = 0;
        for (Record record: neg_sample_set) {
            neg_samples[idx] = record.args;
            idx++;
        }
        return new IntTable(neg_samples);
    }

    /**
     * This method samples negative records from current entailment. Harder negative records will be sampled with higher
     * probability. The hardness of samples is measured by the number of possible evidence that entails the sample. Given
     * that the negative samples will be harder to sample when the quality of rule is improving, the budget here stands
     * for the number of sampling operations performed to generate samples. At most one negative sample will be generated
     * by a single operation.
     *
     * @param posRelation    The set of positive records
     * @param totalConstants The number of constants involved in the KB
     * @param budget         The number of sample operations
     * @param allCache       Current cache for finding entailments
     * @param argLocs        Locations of arguments that are used to assign constants to the head
     * @param ruleStructure  Current structure of rule for sampling
     * @param kb             KB used for sampling
     * @return The returned structure contains a set of negative samples organized as an IntTable and a cache fragment
     * that entails the negative samples and can be operated to filter negative samples. If no negative samples are
     * generated, a NULL will be returned
     */
    public static AdversarialSampledResult adversarialSampling(
            IntTable posRelation, int totalConstants, int budget, List<CacheFragment> allCache,
            CacheFragArgLoc[] argLocs, List<Predicate> ruleStructure, SimpleKb kb
    ) {
        /* Gather head only variables */
        Map<Integer, Integer> vid_2_new_idx_map = new HashMap<>();
        for (CacheFragArgLoc loc_info: argLocs) {
            if (CacheFragArgLoc.TYPE_VAR == loc_info.getType()) {
                loc_info.vid = vid_2_new_idx_map.computeIfAbsent(loc_info.vid, k -> vid_2_new_idx_map.size());
            }
        }
        int[] head_only_values = new int[vid_2_new_idx_map.size()];

        /* Sampling */
        Set<Record> neg_sample_set = new HashSet<>();
        int arity = posRelation.totalCols();
        final int rand_upper_bound = totalConstants + 1;
        int[][][] rand_entry_in_frags = new int[allCache.size()][][];
        for (int frag_idx = 0; frag_idx < rand_entry_in_frags.length; frag_idx++) {
            rand_entry_in_frags[frag_idx] = new int[allCache.get(frag_idx).totalTables()][];
        }
        for (int i = 0; i < budget; i++) {
            /* Randomize head only var values */
            for (int arg_idx = 0; arg_idx < head_only_values.length; arg_idx++) {
                head_only_values[arg_idx] = ThreadLocalRandom.current().nextInt(1, rand_upper_bound);
            }

            /* Randomize cache entry */
            for (int frag_idx = 0; frag_idx < rand_entry_in_frags.length; frag_idx++) {
                CacheFragment fragment = allCache.get(frag_idx);
                List<CB> _rand_entry_in_frag = fragment.getEntry(
                        ThreadLocalRandom.current().nextInt(fragment.totalEntries())
                );
                int[][] rand_entry_in_frag = rand_entry_in_frags[frag_idx];
                for (int tab_idx = 0; tab_idx < rand_entry_in_frag.length; tab_idx++) {
                    CB cb = _rand_entry_in_frag.get(tab_idx);
                    if (1 < cb.complianceSet.length) {
                        rand_entry_in_frag[tab_idx] = cb.complianceSet[
                                ThreadLocalRandom.current().nextInt(cb.complianceSet.length)
                        ];
                    } else {
                        rand_entry_in_frag[tab_idx] = cb.complianceSet[0];
                    }
                }
            }

            /* Generate sample */
            int[] sample = new int[arity];
            for (int arg_idx = 0; arg_idx < arity; arg_idx++) {
                CacheFragArgLoc loc = argLocs[arg_idx];
                switch (loc.type) {
                    case CacheFragArgLoc.TYPE_VAR:
                        sample[arg_idx] = head_only_values[loc.vid];
                        break;
                    case CacheFragArgLoc.TYPE_LOC:
                        sample[arg_idx] = rand_entry_in_frags[loc.fragIdx][loc.tabIdx][loc.colIdx];
                        break;
                    case CacheFragArgLoc.TYPE_CONST:
                        sample[arg_idx] = loc.constant;
                }
            }
            if (!posRelation.hasRow(sample)) {
                neg_sample_set.add(new Record(sample));
            }
        }

        /* Construct cache structure */
        if (neg_sample_set.isEmpty()) {
            return null;
        }

        int[][] neg_samples = new int[neg_sample_set.size()][];
        int idx = 0;
        for (Record neg_record: neg_sample_set) {
            neg_samples[idx] = neg_record.args;
            idx++;
        }

        IntTable neg_table = new IntTable(neg_samples);
        IntTable[] tables = new IntTable[ruleStructure.size()];
        tables[0] = neg_table;
        for (int tab_idx = Rule.FIRST_BODY_PRED_IDX; tab_idx < tables.length; tab_idx++) {
            tables[tab_idx] = kb.getRelation(ruleStructure.get(tab_idx).predSymbol);
        }
        return new AdversarialSampledResult(neg_table, new CacheFragment(ruleStructure, tables));
    }

    /**
     * This method calculates the weight of negative samples given all positive records.
     *
     * @param posRelation    The relation of all positive records
     * @param negRelation    The relation of all negative samples
     * @param totalConstants The number of constants involved in the KB
     * @return The mapping from negative samples to their weight
     */
    public static Map<Record, Float> calcNegSampleWeight(
            IntTable posRelation, IntTable negRelation, int totalConstants
    ) {
        int[][] pos_records = posRelation.getAllRows();
        int[][] neg_records = negRelation.getAllRows();
        int[] neg_sample_insert_indices = posRelation.insertIndices(negRelation);
        Map<Record, Float> weight_map = new HashMap<>();
        for (int i = 0; i < neg_sample_insert_indices.length; ) {
            final int insert_index = neg_sample_insert_indices[i];
            /* Determine the number of total negative samples within the same negative span */
            for (int j = i + 1; j <= neg_sample_insert_indices.length; j++) {
                if (neg_sample_insert_indices.length == j || insert_index != neg_sample_insert_indices[j]) {
                    float weight = recordsInInterval(pos_records, insert_index, totalConstants) / (j - i);
                    for (int k = i; k < j; k++) {
                        weight_map.put(new Record(neg_records[k]), weight);
                    }
                    i = j;
                    break;
                }
            }
        }
        return weight_map;
    }

    /**
     * Calculate the number of records within the span between two records at indices "idx" and "idx-1". If the end index
     * is out of the range of the records. The number returned is counted from the last records to the maximum one.
     *
     * NOTE: The constants in the KB format starts from 1. Thus, the smallest record should be [1, 1, ..., 1] and the
     * largest should be [n, n, ..., n], where n is the number of total constants.
     *
     * @param records        List of records, sorted in alphabetical order
     * @param idx            The end index of the span (should be >= 0).
     * @param totalConstants The number of constants involved in the KB
     */
    public static float recordsInInterval(int[][] records, int idx, int totalConstants) {
        final int[] start_record, end_record;
        final int delta;
        if (0 == idx) {
            end_record = records[0];
            start_record = ArrayOperation.initArrayWithValue(end_record.length, 1);
            delta = 0;
        } else if (records.length <= idx) {
            start_record = records[records.length - 1];
            end_record = ArrayOperation.initArrayWithValue(start_record.length, totalConstants);
            delta = 0;
        } else {
            start_record = records[idx - 1];
            end_record = records[idx];
            delta = 1;
        }
        return recordsInInterval(start_record, end_record, totalConstants) - delta;
    }

    /**
     * The number of records in the span: [startRecord, endRecord)
     */
    public static int recordsInInterval(int[] startRecord, int[] endRecord, int totalConstants) {
        int diff = endRecord[0] - startRecord[0];
        for (int i = 1; i < startRecord.length; i++) {
            diff = diff * totalConstants + endRecord[i] - startRecord[i];
        }
        return diff;
    }

    /**
     * Return the next record of the given one in alphabetical order.
     *
     * NOTE: The next of the largest record is the one itself.
     */
    public static int[] nextRecord(int[] record, int totalConstants) {
        int[] next_record = record.clone();
        boolean is_max = true;
        for (int i = next_record.length - 1; i >= 0; i--) {
            if (next_record[i] < totalConstants) {
                next_record[i]++;
                is_max = false;
                break;
            } else {
                next_record[i] = 1;
            }
        }
        return is_max ? record.clone() : next_record;
    }

    /**
     * Return the record that is "stepSize" records behind the given one in the alphabetical order.
     *
     * NOTE: If the next record is "larger" than the maximum record, the max will be returned.
     */
    public static int[] nextRecord(int[] record, int totalConstants, int stepSize) {
        int[] next_record = record.clone();
        int carry = 0;
        for (int i = next_record.length - 1; i >= 0 && (stepSize > 0 || carry > 0); i--) {
            int addition = stepSize % totalConstants;
            stepSize = stepSize / totalConstants;
            next_record[i] += addition + carry;
            carry = (next_record[i] - 1) / totalConstants;
            next_record[i] = (next_record[i] - 1) % totalConstants + 1;
        }
        return (0 == carry) ? next_record : ArrayOperation.initArrayWithValue(record.length, totalConstants);
    }

    /**
     * Return the previous record of the given one in alphabetical order.
     *
     * NOTE: The previous of the smallest record is the one itself.
     */
    public static int[] previousRecord(int[] record, int totalConstants) {
        int[] prev_record = record.clone();
        boolean is_min = true;
        for (int i = prev_record.length - 1; i >= 0; i--) {
            if (prev_record[i] > 1) {
                prev_record[i]--;
                is_min = false;
                break;
            } else {
                prev_record[i] = totalConstants;
            }
        }
        return is_min ? record.clone() : prev_record;
    }

    protected static int[][] collectionToArray(Collection<Record> records) {
        int[][] _records = new int[records.size()][];
        int idx = 0;
        for (Record record: records) {
            _records[idx] = record.args;
            idx++;
        }
        return _records;
    }

    /**
     * Use this method to generate negative samples in each dataset. There are two arguments in the array:
     *   1. The path to the KBs
     *   2. The path where the negative KB will be dumped
     */
    public static void main(String[] args) throws IOException {
        String base_path = args[0];
        String neg_base_path = args[1];
//        float[] budget_factors = new float[] {0.5f, 1, 1.5f, 2, 2.5f, 3, 3.5f, 4, 4.5f, 5};
//        float[] budget_factors = new float[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        float[] budget_factors = new float[] {2, 4, 6, 8, 10, 12, 14, 16, 18, 20};
//        int[] rounds_list = new int[] {1, 2, 3, 4, 5};
//        int[] steps_list = new int[] {1, 2, 3, 4};
        int[] rounds_list = new int[] {5};
        int[] steps_list = new int[] {4};
        File base_path_file = new File(base_path);
        for (File kb_dir_file: base_path_file.listFiles()) {
            if (kb_dir_file.isDirectory()) {
                String kb_name = kb_dir_file.getName();
                System.out.println("Sampling from: " + kb_name);
                SimpleKb kb = new SimpleKb(kb_name, base_path);

//                for (float budget_factor: budget_factors) {
//                    System.out.printf("Uniform (%.2f) ...\n", budget_factor);
//                    NegSampleKb uniform_neg_kb = uniformGenerator(kb, budget_factor);
//                    uniform_neg_kb.dump(neg_base_path);
//
//                    System.out.printf("Pos Relative (%.2f) ...\n", budget_factor);
//                    NegSampleKb pos_rel_neg_kb = posRelativeGenerator(kb, budget_factor);
//                    pos_rel_neg_kb.dump(neg_base_path);
//                }

//                for (boolean enhance: new boolean[] {false, true}) {
//                    System.out.println("Neg Interval ...");
//                    NegSampleKb neg_intv_neg_kb = negIntervalGenerator(kb, enhance);
//                    neg_intv_neg_kb.dump(neg_base_path);
//                    NegSampleKb nib_neg_kb = negIntervalBeginningGenerator(kb, enhance);
//                    nib_neg_kb.dump(neg_base_path);
//                    NegSampleKb nie_neg_kb = negIntervalEndingGenerator(kb, enhance);
//                    nie_neg_kb.dump(neg_base_path);
//
//                    System.out.println("Alphabetical Neighbor ...");
//                    NegSampleKb an_neg_kb = alphaNeighborGenerator(kb, enhance);
//                    an_neg_kb.dump(neg_base_path);
//                    NegSampleKb anl_neg_kb = alphaNeighborLowerGenerator(kb, enhance);
//                    anl_neg_kb.dump(neg_base_path);
//                    NegSampleKb anu_neg_kb = alphaNeighborUpperGenerator(kb, enhance);
//                    anu_neg_kb.dump(neg_base_path);
//                }

//                constRelevanceGenerator(kb, neg_base_path);

                for (int steps: steps_list) {
                    for (int rounds : rounds_list) {
                        NegSampleKb con_rel_neg_kb = constantRelevanceSampling(kb, steps, rounds);
                        con_rel_neg_kb.dump(neg_base_path);
                    }
                }
            }
        }
    }

    protected static NegSampleKb uniformGenerator(SimpleKb kb, float budget_factor) {
        SimpleRelation[] relations = kb.getRelations();
        int total_constants = kb.totalConstants();
        IntTable[] neg_tables = new IntTable[relations.length];
        Map<Record, Float>[] weight_maps = new Map[relations.length];
        for (int rel_idx = 0; rel_idx < relations.length; rel_idx++) {
            SimpleRelation relation = relations[rel_idx];
            int budget = (int) Math.round(Math.min(
                    relation.totalRows() * budget_factor,
                    Math.pow(total_constants, relation.totalCols()) - relation.totalRows()
            ));
            IntTable neg_table = NegSampler.uniformSampling(relation, total_constants, budget);
            neg_tables[rel_idx] = neg_table;
            weight_maps[rel_idx] = NegSampler.calcNegSampleWeight(relation, neg_table, total_constants);
        }
        return new NegSampleKb(String.format("%s_neg_uni_%.1f", kb.getName(), budget_factor), neg_tables, weight_maps);
    }

    protected static NegSampleKb posRelativeGenerator(SimpleKb kb, float budget_factor) {
        SimpleRelation[] relations = kb.getRelations();
        int total_constants = kb.totalConstants();
        IntTable[] neg_tables = new IntTable[relations.length];
        Map<Record, Float>[] weight_maps = new Map[relations.length];
        for (int rel_idx = 0; rel_idx < relations.length; rel_idx++) {
            SimpleRelation relation = relations[rel_idx];
            int budget = (int) Math.round(Math.min(
                    relation.totalRows() * budget_factor,
                    Math.pow(total_constants, relation.totalCols()) - relation.totalRows()
            ));
            IntTable neg_table = NegSampler.posRelativeSampling(relation, total_constants, budget);
            neg_tables[rel_idx] = neg_table;
            weight_maps[rel_idx] = NegSampler.calcNegSampleWeight(relation, neg_table, total_constants);
        }
        return new NegSampleKb(String.format("%s_neg_pos_rel_%.1f", kb.getName(), budget_factor), neg_tables, weight_maps);
    }

    protected static NegSampleKb negIntervalGenerator(SimpleKb kb, boolean enhance) {
        SimpleRelation[] relations = kb.getRelations();
        int total_constants = kb.totalConstants();
        IntTable[] neg_tables = new IntTable[relations.length];
        Map<Record, Float>[] weight_maps = new Map[relations.length];
        for (int rel_idx = 0; rel_idx < relations.length; rel_idx++) {
            SimpleRelation relation = relations[rel_idx];
            IntTable neg_table = NegSampler.negIntervalSampling(relation, total_constants, enhance ? relation.totalRows() : 0);
            neg_tables[rel_idx] = neg_table;
            weight_maps[rel_idx] = NegSampler.calcNegSampleWeight(relation, neg_table, total_constants);
        }
        return new NegSampleKb(String.format(enhance ? "%s_neg_NI+" : "%s_neg_NI", kb.getName()), neg_tables, weight_maps);
    }

    protected static NegSampleKb negIntervalBeginningGenerator(SimpleKb kb, boolean enhance) {
        SimpleRelation[] relations = kb.getRelations();
        int total_constants = kb.totalConstants();
        IntTable[] neg_tables = new IntTable[relations.length];
        Map<Record, Float>[] weight_maps = new Map[relations.length];
        for (int rel_idx = 0; rel_idx < relations.length; rel_idx++) {
            SimpleRelation relation = relations[rel_idx];
            IntTable neg_table = NegSampler.negIntervalBeginningSampling(relation, total_constants, enhance ? relation.totalRows() : 0);
            neg_tables[rel_idx] = neg_table;
            weight_maps[rel_idx] = NegSampler.calcNegSampleWeight(relation, neg_table, total_constants);
        }
        return new NegSampleKb(String.format(enhance ? "%s_neg_NIB+" : "%s_neg_NIB", kb.getName()), neg_tables, weight_maps);
    }

    protected static NegSampleKb negIntervalEndingGenerator(SimpleKb kb, boolean enhance) {
        SimpleRelation[] relations = kb.getRelations();
        int total_constants = kb.totalConstants();
        IntTable[] neg_tables = new IntTable[relations.length];
        Map<Record, Float>[] weight_maps = new Map[relations.length];
        for (int rel_idx = 0; rel_idx < relations.length; rel_idx++) {
            SimpleRelation relation = relations[rel_idx];
            IntTable neg_table = NegSampler.negIntervalEndingSampling(relation, total_constants, enhance ? relation.totalRows() : 0);
            neg_tables[rel_idx] = neg_table;
            weight_maps[rel_idx] = NegSampler.calcNegSampleWeight(relation, neg_table, total_constants);
        }
        return new NegSampleKb(String.format(enhance ? "%s_neg_NIE+" : "%s_neg_NIE", kb.getName()), neg_tables, weight_maps);
    }

    protected static NegSampleKb alphaNeighborGenerator(SimpleKb kb, boolean enhance) {
        SimpleRelation[] relations = kb.getRelations();
        int total_constants = kb.totalConstants();
        IntTable[] neg_tables = new IntTable[relations.length];
        Map<Record, Float>[] weight_maps = new Map[relations.length];
        for (int rel_idx = 0; rel_idx < relations.length; rel_idx++) {
            SimpleRelation relation = relations[rel_idx];
            IntTable neg_table = NegSampler.alphaNeighborSampling(relation, total_constants, enhance ? relation.totalRows() : 0);
            neg_tables[rel_idx] = neg_table;
            weight_maps[rel_idx] = NegSampler.calcNegSampleWeight(relation, neg_table, total_constants);
        }
        return new NegSampleKb(String.format(enhance ? "%s_neg_AN+" : "%s_neg_AN", kb.getName()), neg_tables, weight_maps);
    }

    protected static NegSampleKb alphaNeighborLowerGenerator(SimpleKb kb, boolean enhance) {
        SimpleRelation[] relations = kb.getRelations();
        int total_constants = kb.totalConstants();
        IntTable[] neg_tables = new IntTable[relations.length];
        Map<Record, Float>[] weight_maps = new Map[relations.length];
        for (int rel_idx = 0; rel_idx < relations.length; rel_idx++) {
            SimpleRelation relation = relations[rel_idx];
            IntTable neg_table = NegSampler.alphaNeighborLowerSampling(relation, total_constants, enhance ? relation.totalRows() : 0);
            neg_tables[rel_idx] = neg_table;
            weight_maps[rel_idx] = NegSampler.calcNegSampleWeight(relation, neg_table, total_constants);
        }
        return new NegSampleKb(String.format(enhance ? "%s_neg_ANL+" : "%s_neg_ANL", kb.getName()), neg_tables, weight_maps);
    }

    protected static NegSampleKb alphaNeighborUpperGenerator(SimpleKb kb, boolean enhance) {
        SimpleRelation[] relations = kb.getRelations();
        int total_constants = kb.totalConstants();
        IntTable[] neg_tables = new IntTable[relations.length];
        Map<Record, Float>[] weight_maps = new Map[relations.length];
        for (int rel_idx = 0; rel_idx < relations.length; rel_idx++) {
            SimpleRelation relation = relations[rel_idx];
            IntTable neg_table = NegSampler.alphaNeighborUpperSampling(relation, total_constants, enhance ? relation.totalRows() : 0);
            neg_tables[rel_idx] = neg_table;
            weight_maps[rel_idx] = NegSampler.calcNegSampleWeight(relation, neg_table, total_constants);
        }
        return new NegSampleKb(String.format(enhance ? "%s_neg_ANU+" : "%s_neg_ANU", kb.getName()), neg_tables, weight_maps);
    }

//    protected static void constRelevanceGenerator(SimpleKb kb, String negBasePath) throws IOException {
////        for (int hop : new int[]{0, 1, 2, 3}) {
////            for (int top_k: new int[]{1, 2, 3, 4, 5}) {
////                System.out.printf("Hop = %d, K = %d\n", hop, top_k);
////                constantRelevanceSampling(kb, hop, top_k, negBasePath);
////            }
////        }
//
////        for (int hop : new int[]{0, 1, 2, 3}) {
////            for (int percentage: new int[]{10, 20, 30, 40, 50}) {
////                for (int rounds: new int[]{1, 2, 3, 4, 5}) {
////                    System.out.printf("Hop = %d, Percentage = %d, R = %d\n", hop, percentage, rounds);
////                    constantRelevanceSampling(kb, hop, percentage, rounds, negBasePath);
////                }
////            }
////        }
//
////        for (int hop : new int[]{0, 1, 2, 3}) {
////            for (int rounds: new int[]{1, 2, 3, 4, 5}) {
////                System.out.printf("Hop = %d, Rounds = %d\n", hop, rounds);
////                constantRelevanceSampling(kb, hop, rounds, negBasePath);
////            }
////        }
//    }
}
