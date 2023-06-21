package sinc2.kb;

import sinc2.common.Predicate;
import sinc2.common.Record;
import sinc2.impl.negsamp.CB;
import sinc2.impl.negsamp.CacheFragment;
import sinc2.rule.Rule;
import sinc2.util.ArrayOperation;

import java.util.*;
import java.util.concurrent.ThreadLocalRandom;

/**
 * This class is used for negative sampling from KBs under closed-word assumption (CWA). Four strategies are implemented
 * currently:
 *   1. Uniform Sampling
 *   2. Positive-relative Sampling
 *   3. Negative Interval Sampling
 *   4. Adversarial Sampling
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
     * This method samples negative records from negative intervals. The lower bound of each negative interval is selected.
     * Therefore, at most n+1 (n is the number of positive records) negative samples will be selected under this strategy.
     *
     * @param posRelation    The set of positive records
     * @param totalConstants The number of constants involved in the KB
     * @return A set of negative samples organized as an IntTable
     */
    public static IntTable negIntervalSampling(IntTable posRelation, int totalConstants) {
        List<int[]> neg_samples = new ArrayList<>();
        int[][] pos_records = posRelation.getAllRows();

        /* Check the first negative interval */
        boolean bottom_interval_exists = false;
        int[] pos_record = pos_records[0];
        for (int arg: pos_record) {
            if (1 < arg) {  // [1, 1, ..., 1] is the smallest record
                bottom_interval_exists = true;
                break;
            }
        }
        if (bottom_interval_exists) {
            neg_samples.add(ArrayOperation.initArrayWithValue(pos_record.length, 1));
        }

        /* Check the last negative interval */
        boolean top_interval_exists = false;
        pos_record = pos_records[pos_records.length - 1];
        for (int arg: pos_record) {
            if (arg < totalConstants) { // [n, n, ..., n] is the largest record
                top_interval_exists = true;
                break;
            }
        }
        if (top_interval_exists) {
            neg_samples.add(nextRecord(pos_record, totalConstants));
        }

        /* Select the lower bound of each negative interval */
        int lim = pos_records.length - 1;
        for (int i = 0; i < lim; i++) {
            int[] next_record = nextRecord(pos_records[i], totalConstants);
            if (0 != IntTable.rowComparator.compare(next_record, pos_records[i+1])) {
                neg_samples.add(next_record);
            }
        }

        return new IntTable(neg_samples.toArray(new int[0][]));
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
     * that entails the negative samples and can be operated to filter negative samples
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
        int[][] neg_samples = new int[neg_sample_set.size()][];
        int idx = 0;
        for (Record neg_record: neg_sample_set) {
            neg_samples[idx] = neg_record.args;
            idx++;
        }

        /* Construct cache structure */
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
                    float weight = recordsInSpan(pos_records, insert_index, totalConstants) / (j - i);
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
    protected static float recordsInSpan(int[][] records, int idx, int totalConstants) {
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
        int diff = end_record[0] - start_record[0];
        for (int i = 1; i < start_record.length; i++) {
            diff = diff * totalConstants + end_record[i] - start_record[i];
        }
        return diff - delta;
    }

    /**
     * Return the next record of the given one in alphabetical order.
     *
     * NOTE: The next of the largest record is the one itself.
     */
    protected static int[] nextRecord(int[] record, int totalConstants) {
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
}
