package sinc2.exp.est;

import com.google.gson.Gson;
import sinc2.impl.est.EstRule;
import sinc2.impl.est.SpecOprWithScore;
import sinc2.kb.SimpleKb;
import sinc2.kb.SimpleRelation;
import sinc2.rule.Eval;
import sinc2.rule.EvalMetric;
import sinc2.rule.Rule;
import sinc2.rule.UpdateStatus;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.*;

public class EstimationRanking {

    static final int BEAMWIDTH = 5;
    static final EvalMetric EVAL_METRIC = EvalMetric.CompressionRatio;

    public static void main(String[] args) throws IOException {
        if (3 != args.length) {
            System.err.println("Usage: <KB dir> <KB name> <result file path>");
            return;
        }
        EstimationRanking test = new EstimationRanking();
        test.observeEstimationOrder(args[0], args[1], args[2]);
    }

    void observeEstimationOrder(String kbDir, String dataset, String outputFilePath) throws IOException {
        SimpleKb kb = new SimpleKb(dataset, kbDir);
        kb.updatePromisingConstants();
        int[][][][] rankings_when_mining_relations = new int[kb.totalRelations()][][][];    // int[relation idx][rule length][beams][rank]
        for (SimpleRelation relation: kb.getRelations()) {
            rankings_when_mining_relations[relation.id] = getRankingsInRelation(kb, relation);
        }
        Gson gson = new Gson();
        PrintWriter writer = new PrintWriter(outputFilePath);
        gson.toJson(rankings_when_mining_relations, writer);
        writer.close();
    }


    static class EstRanking {
        public final Eval estEval;
        public final int estRank;
        public final EstRule specRule;

        public EstRanking(Eval estEval, int estRank, EstRule specRule) {
            this.estEval = estEval;
            this.estRank = estRank;
            this.specRule = specRule;
        }
    }

    int[][][] getRankingsInRelation(SimpleKb kb, SimpleRelation relation) {
        List<int[][]> rankings_of_lengths = new ArrayList<>();

        /* Create the beams */
        EstRule[] beams = new EstRule[BEAMWIDTH];
        beams[0] = new EstRule(relation.id, relation.totalCols(), new HashSet<>(), new HashMap<>(), kb);

        /* Find a local optimum (there is certainly a local optimum in the search routine) */
        while (true) {
            /* Find the candidates in the next round according to current beams */
            EstRule[] top_candidates = new EstRule[BEAMWIDTH];
            List<EstRanking>[] rankings_of_length = new List[BEAMWIDTH];
            for (int i = 0; i < BEAMWIDTH && null != beams[i]; i++) {
                EstRule r = beams[i];
                r.updateCacheIndices();
                List<SpecOprWithScore> estimated_specs = r.estimateSpecializations();
                rankings_of_length[i] = rankEstOprs(r, estimated_specs);
            }
            findEstimatedSpecializations(beams, rankings_of_length, top_candidates);
            rankings_of_lengths.add(convertFormat(rankings_of_length));

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
                return rankings_of_lengths.toArray(new int[0][][]);
            }

            /* Update the beams */
            beams = top_candidates;
        }
    }

    List<EstRanking> rankEstOprs(EstRule rule, List<SpecOprWithScore> estimatedSpecs) {
        List<EstRanking> ranked_specs = new ArrayList<>();
        estimatedSpecs.sort(Comparator.comparingDouble((SpecOprWithScore e) -> e.estEval.value(EVAL_METRIC)).reversed());
        for (SpecOprWithScore est_spec_opr: estimatedSpecs) {
            EstRule new_rule = rule.clone();
            UpdateStatus update_status = est_spec_opr.opr.specialize(new_rule);
            if (UpdateStatus.NORMAL == update_status) {
                ranked_specs.add(new EstRanking(est_spec_opr.estEval, ranked_specs.size(), new_rule));
            }
        }
        ranked_specs.sort(Comparator.comparingDouble((EstRanking e) -> e.specRule.getEval().value(EVAL_METRIC)).reversed());
        return ranked_specs;
    }
    void findEstimatedSpecializations(
            EstRule[] beams, List<EstRanking>[] specLists, EstRule[] topCandidates
    ) {
        int num_beams = 1;  // at least 1 beam in the "beams" array
        for (; num_beams < BEAMWIDTH && null != beams[num_beams]; num_beams++); // find the exact number of beams in the array
        int[] idxs = new int[num_beams];
        while (null == topCandidates[topCandidates.length - 1]) {
            int best_rule_idx = -1;
            double best_score = Eval.MIN.value(EVAL_METRIC);
            for (int rule_idx = 0; rule_idx < num_beams; rule_idx++) {
                List<EstRanking> est_spec_list = specLists[rule_idx];
                int idx = idxs[rule_idx];
                if (idx < est_spec_list.size()) {
                    double score = est_spec_list.get(idx).specRule.getEval().value(EVAL_METRIC);
                    if (best_score < score) {
                        best_score = score;
                        best_rule_idx = rule_idx;
                    }
                }
            }
            if (-1 == best_rule_idx) {
                break;
            }
            EstRanking best_spec = specLists[best_rule_idx].get(idxs[best_rule_idx]);
            EstRule original_rule = beams[best_rule_idx];
            EstRule updated_rule = best_spec.specRule;
            idxs[best_rule_idx]++;

            if (updated_rule.getEval().value(EVAL_METRIC) > original_rule.getEval().value(EVAL_METRIC)) {
                int replace_idx = -1;
                double replaced_score = updated_rule.getEval().value(EVAL_METRIC);
                for (int i = 0; i < topCandidates.length; i++) {
                    if (null == topCandidates[i]) {
                        replace_idx = i;
                        break;
                    }
                    double candidate_socre = topCandidates[i].getEval().value(EVAL_METRIC);
                    if (replaced_score > candidate_socre) {
                        replace_idx = i;
                        replaced_score = candidate_socre;
                    }
                }
                if (0 <= replace_idx) {
                    topCandidates[replace_idx] = updated_rule;
                }
            }
        }
    }

    int[][] convertFormat(List<EstRanking>[] rankings) {
        List<int[]> converted_rankings = new ArrayList<>();
        for (int i = 0; i < rankings.length && null != rankings[i]; i++) {
            List<EstRanking> ranking = rankings[i];
            int[] converted_ranking = new int[ranking.size()];
            for (int j = 0; j < converted_ranking.length; j++) {
                converted_ranking[j] = ranking.get(j).estRank;
            }
            converted_rankings.add(converted_ranking);
        }
        return converted_rankings.toArray(new int[0][]);
    }
}
