package sinc2.exp.est;

import com.google.gson.Gson;
import sinc2.common.Predicate;
import sinc2.impl.base.CompliedBlock;
import sinc2.impl.est.EstRule;
import sinc2.impl.est.SpecOprWithScore;
import sinc2.kb.SimpleKb;
import sinc2.kb.SimpleRelation;
import sinc2.rule.*;
import sinc2.util.MultiSet;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.*;

public class AssumptionValidation {
    static final int BEAMWIDTH = 5;
    static final EvalMetric EVAL_METRIC = EvalMetric.CompressionRatio;
    static final double OBSERVATION_RATIO = 3;

    static class TestCachedRule extends EstRule {

        public TestCachedRule(int headPredSymbol, int arity, Set<Fingerprint> fingerprintCache, Map<MultiSet<Integer>, Set<Fingerprint>> category2TabuSetMap, SimpleKb kb) {
            super(headPredSymbol, arity, fingerprintCache, category2TabuSetMap, kb);
        }

        public TestCachedRule(List<Predicate> structure, Set<Fingerprint> fingerprintCache, Map<MultiSet<Integer>, Set<Fingerprint>> category2TabuSetMap, SimpleKb kb) {
            super(structure, fingerprintCache, category2TabuSetMap, kb);
        }

        public TestCachedRule(TestCachedRule another) {
            super(another);
        }

        @Override
        public TestCachedRule clone() {
            return new TestCachedRule(this);
        }

        public int[][] getPosCbSizes() {
            int[][] cb_sizes = new int[structure.size()][];
            for (int pred_idx = HEAD_PRED_IDX; pred_idx < cb_sizes.length; pred_idx++) {
                int[] cb_sizes_in_pred = new int[posCache.size()];
                cb_sizes[pred_idx] = cb_sizes_in_pred;
                for (int cache_idx = 0; cache_idx < cb_sizes_in_pred.length; cache_idx++) {
                    cb_sizes_in_pred[cache_idx] = posCache.get(cache_idx).get(pred_idx).complSet.length;
                }
            }
            return cb_sizes;
        }

        public int[][] getAllCbSizes() {
            int[][] cb_sizes = new int[structure.size()][];
            for (int pred_idx = FIRST_BODY_PRED_IDX; pred_idx < cb_sizes.length; pred_idx++) {
                int[] cb_sizes_in_pred = new int[allCache.size()];
                cb_sizes[pred_idx] = cb_sizes_in_pred;
                for (int cache_idx = 0; cache_idx < cb_sizes_in_pred.length; cache_idx++) {
                    cb_sizes_in_pred[cache_idx] = allCache.get(cache_idx).get(pred_idx).complSet.length;
                }
            }
            return cb_sizes;
        }
    }

    static class CbSizes {
        final int[][] posCbSizes;
        final int[][] allCbSizes;

        public CbSizes(int[][] posCbSizes, int[][] allCbSizes) {
            this.posCbSizes = posCbSizes;
            this.allCbSizes = allCbSizes;
        }
    }

    public static void main(String[] args) throws IOException {
        if (3 != args.length) {
            System.err.println("Usage: <KB dir> <KB name> <result file path>");
            return;
        }
        AssumptionValidation test = new AssumptionValidation();
        test.validateAssumption(args[0], args[1], args[2]);
    }

    void validateAssumption(String kbDir, String dataset, String outputFilePath) throws IOException {
        SimpleKb kb = new SimpleKb(dataset, kbDir);
        kb.updatePromisingConstants();
        CbSizes[][][] cb_sizes = new CbSizes[kb.totalRelations()][][];    // int[relation idx][rule length][beams][rank]
        for (SimpleRelation relation : kb.getRelations()) {
            cb_sizes[relation.id] = getCbSizeInRelation(kb, relation);
        }
        Gson gson = new Gson();
        PrintWriter writer = new PrintWriter(outputFilePath);
        gson.toJson(cb_sizes, writer);
        writer.close();
    }

    CbSizes[][] getCbSizeInRelation(SimpleKb kb, SimpleRelation relation) {   // int[rule length][beams]
        List<CbSizes[]> cb_sizes_of_rule_length = new ArrayList<>();

        /* Create the beams */
        TestCachedRule[] beams = new TestCachedRule[BEAMWIDTH];
        beams[0] = new TestCachedRule(relation.id, relation.totalCols(), new HashSet<>(), new HashMap<>(), kb);

        /* Find a local optimum (there is certainly a local optimum in the search routine) */
        while (true) {
            /* Find the candidates in the next round according to current beams */
            TestCachedRule[] top_candidates = new TestCachedRule[BEAMWIDTH];
            List<SpecOprWithScore>[] estimated_spec_lists = new List[BEAMWIDTH];
            List<CbSizes> cb_sizes_of_beams = new ArrayList<>();
            for (int i = 0; i < BEAMWIDTH && null != beams[i]; i++) {
                TestCachedRule r = beams[i];
                r.updateCacheIndices();
                cb_sizes_of_beams.add(new CbSizes(r.getPosCbSizes(), r.getAllCbSizes()));
                List<SpecOprWithScore> estimated_specs = r.estimateSpecializations();
                estimated_specs.sort(Comparator.comparingDouble((SpecOprWithScore e) -> e.estEval.value(EVAL_METRIC)).reversed());
                estimated_spec_lists[i] = estimated_specs;
            }
            cb_sizes_of_rule_length.add(cb_sizes_of_beams.toArray(new CbSizes[0]));
            findEstimatedSpecializations(beams, estimated_spec_lists, top_candidates);

            /* Find the best in beams and candidates */
            Rule best_beam = beams[0];
            for (int i = 1; i < BEAMWIDTH && null != beams[i]; i++) {
                if (best_beam.getEval().value(EVAL_METRIC) < beams[i].getEval().value(EVAL_METRIC)) {
                    best_beam = beams[i];
                }
            }
            Rule best_candidate = top_candidates[0];
            for (int i = 0; i < BEAMWIDTH && null != top_candidates[i]; i++) {
                if (best_candidate.getEval().value(EVAL_METRIC) < top_candidates[i].getEval().value(EVAL_METRIC)) {
                    best_candidate = top_candidates[i];
                }
            }
            if (null == best_candidate || best_beam.getEval().value(EVAL_METRIC) >= best_candidate.getEval().value(EVAL_METRIC)) {
                /* The best in the beams is a local optimum among all tested rules */
                break;
            }

            /* If the best rule reaches the stopping threshold, return the rule */
            /* The "best_candidate" is certainly not NULL if the workflow goes here */
            /* There is no need to check "best_beam", because it has been checked by the following code in the last iteration */
            /* Assumption: the stopping threshold is no less than the threshold of usefulness */
            Eval best_eval = best_candidate.getEval();
            if (0 == best_eval.getNegEtls()) {
                break;
            }

            /* Update the beams */
            beams = top_candidates;
        }
        return cb_sizes_of_rule_length.toArray(new CbSizes[0][]);
    }

    protected void findEstimatedSpecializations(
            Rule[] beams, List<SpecOprWithScore>[] estimatedSpecLists, Rule[] topCandidates
    ) {
        int observations = (int) Math.round(BEAMWIDTH * OBSERVATION_RATIO);
        int num_beams = 1;  // at least 1 beam in the "beams" array
        for (; num_beams < BEAMWIDTH && null != beams[num_beams]; num_beams++); // find the exact number of beams in the array
        int[] idxs = new int[num_beams];
        for (int i = 0; i < observations; i++) {
            int best_rule_idx = -1;
            double best_score = Eval.MIN.value(EVAL_METRIC);
            for (int rule_idx = 0; rule_idx < num_beams; rule_idx++) {
                List<SpecOprWithScore> est_spec_list = estimatedSpecLists[rule_idx];
                int idx = idxs[rule_idx];
                if (idx < est_spec_list.size()) {
                    double score = est_spec_list.get(idx).estEval.value(EVAL_METRIC);
                    if (best_score < score) {
                        best_score = score;
                        best_rule_idx = rule_idx;
                    }
                }
            }
            SpecOprWithScore best_spec = estimatedSpecLists[best_rule_idx].get(idxs[best_rule_idx]);
            idxs[best_rule_idx]++;
            Rule copy = beams[best_rule_idx].clone();
            UpdateStatus status = best_spec.opr.specialize(copy);
            checkThenAddRule(status, copy, beams[best_rule_idx], topCandidates);
        }
    }

    protected int checkThenAddRule(
            UpdateStatus updateStatus, Rule updatedRule, Rule originalRule, Rule[] candidates
    ) {
        boolean updated_is_better = false;
        switch (updateStatus) {
            case NORMAL:
                if (updatedRule.getEval().value(EVAL_METRIC) > originalRule.getEval().value(EVAL_METRIC)) {
                    updated_is_better = true;
                    int replace_idx = -1;
                    double replaced_score = updatedRule.getEval().value(EVAL_METRIC);
                    for (int i = 0; i < candidates.length; i++) {
                        if (null == candidates[i]) {
                            replace_idx = i;
                            break;
                        }
                        double candidate_socre = candidates[i].getEval().value(EVAL_METRIC);
                        if (replaced_score > candidate_socre) {
                            replace_idx = i;
                            replaced_score = candidate_socre;
                        }
                    }
                    if (0 <= replace_idx) {
                        candidates[replace_idx] = updatedRule;
                    }
                }
                break;
            case INVALID:
            case DUPLICATED:
            case INSUFFICIENT_COVERAGE:
            case TABU_PRUNED:
                break;
            default:
                throw new Error("Unknown Update Status of Rule: " + updateStatus.name());
        }
        return updated_is_better ? 1 : 0;
    }
}
