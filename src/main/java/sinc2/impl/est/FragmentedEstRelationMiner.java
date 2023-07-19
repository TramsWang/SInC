package sinc2.impl.est;

import sinc2.common.DebugLevel;
import sinc2.common.InterruptedSignal;
import sinc2.common.Predicate;
import sinc2.impl.base.CachedRule;
import sinc2.impl.base.RelationMinerBasic;
import sinc2.impl.negsamp.RelationMinerWithFragmentedCachedRule;
import sinc2.kb.SimpleKb;
import sinc2.rule.Eval;
import sinc2.rule.EvalMetric;
import sinc2.rule.Rule;
import sinc2.rule.UpdateStatus;
import sinc2.util.graph.GraphNode;

import java.io.PrintWriter;
import java.util.*;

public class FragmentedEstRelationMiner extends RelationMinerWithFragmentedCachedRule {

    /** The ratio (>=1) that extends the number of rules that are actually specialized according to the estimations */
    protected final double observationRatio;

    /**
     * Construct by passing parameters from the compressor that loads the data.
     *
     * @param kb                   The input KB
     * @param targetRelation       The target relation in the KB
     * @param evalMetric           The rule evaluation metric
     * @param beamwidth            The beamwidth used in the rule mining procedure
     * @param observationRatio     The ratio (>=1) that extends the number of rules that are actually specialized according to the estimations
     * @param stopCompressionRatio The stopping compression ratio for inducing a single rule
     * @param predicate2NodeMap    The mapping from predicates to the nodes in the dependency graph
     * @param dependencyGraph      The dependency graph
     * @param logger               A logger
     */
    public FragmentedEstRelationMiner(
            SimpleKb kb, int targetRelation, EvalMetric evalMetric, int beamwidth, double observationRatio,
            double stopCompressionRatio, Map<Predicate, GraphNode<Predicate>> predicate2NodeMap,
            Map<GraphNode<Predicate>, Set<GraphNode<Predicate>>> dependencyGraph, PrintWriter logger
    ) {
        super(kb, targetRelation, evalMetric, beamwidth, stopCompressionRatio, predicate2NodeMap, dependencyGraph, logger);
        this.observationRatio = observationRatio;
    }

    @Override
    protected Rule getStartRule() {
        return new FragmentedEstRule(targetRelation, kb.getRelation(targetRelation).totalCols(), new HashSet<>(), tabuSet, kb);
    }

    @Override
    protected Rule findRule() {
        /* Create the beams */
        Rule[] beams = new Rule[beamwidth];
        beams[0] = getStartRule();

        /* Find a local optimum (there is certainly a local optimum in the search routine) */
        while (true) {
            /* Find the candidates in the next round according to current beams */
            Rule[] top_candidates = new Rule[beamwidth];
            try {
                List<SpecOprWithScore>[] estimated_spec_lists = new List[beamwidth];
                for (int i = 0; i < beamwidth && null != beams[i]; i++) {
                    Rule r = beams[i];
                    selectAsBeam(r);
                    if (DebugLevel.VERBOSE <= DebugLevel.LEVEL) {
                        logger.printf("Extend: %s\n", r.toString(kb));
                        logger.flush();
                    }
                    List<SpecOprWithScore> estimated_specs = ((FragmentedEstRule) r).estimateSpecializations();
                    estimated_specs.sort(Comparator.comparingDouble((SpecOprWithScore e) -> e.estEval.value(evalMetric)).reversed());
                    estimated_spec_lists[i] = estimated_specs;
                }
                findEstimatedSpecializations(beams, estimated_spec_lists, top_candidates);

                /* Find the best in beams and candidates */
                Rule best_beam = beams[0];
                for (int i = 1; i < beamwidth && null != beams[i]; i++) {
                    if (best_beam.getEval().value(evalMetric) < beams[i].getEval().value(evalMetric)) {
                        best_beam = beams[i];
                    }
                }
                Rule best_candidate = top_candidates[0];
                for (int i = 0; i < beamwidth && null != top_candidates[i]; i++) {
                    if (best_candidate.getEval().value(evalMetric) < top_candidates[i].getEval().value(evalMetric)) {
                        best_candidate = top_candidates[i];
                    }
                }
                if (null == best_candidate || best_beam.getEval().value(evalMetric) >= best_candidate.getEval().value(evalMetric)) {
                    /* The best in the beams is a local optimum among all tested rules */
                    return best_beam.getEval().useful() ? best_beam : null;
                }

                /* If the best rule reaches the stopping threshold, return the rule */
                /* The "best_candidate" is certainly not NULL if the workflow goes here */
                /* There is no need to check "best_beam", because it has been checked by the following code in the last iteration */
                /* Assumption: the stopping threshold is no less than the threshold of usefulness */
                Eval best_eval = best_candidate.getEval();
                if (stopCompressionRatio <= best_eval.value(EvalMetric.CompressionRatio) || 0 == best_eval.getNegEtls()) {
                    return best_eval.useful() ? best_candidate : null;
                }

                /* Update the beams */
                beams = top_candidates;
            } catch (InterruptedSignal e) {
                /* Stop the finding procedure at the current stage and return the best rule */
                Rule best_rule = beams[0];
                for (int i = 1; i < beamwidth && null != beams[i]; i++) {
                    if (best_rule.getEval().value(evalMetric) < beams[i].getEval().value(evalMetric)) {
                        best_rule = beams[i];
                    }
                }
                for (int i = 0; i < beamwidth && null != top_candidates[i]; i++) {
                    if (best_rule.getEval().value(evalMetric) < top_candidates[i].getEval().value(evalMetric)) {
                        best_rule = top_candidates[i];
                    }
                }
                return (null != best_rule && best_rule.getEval().useful()) ? best_rule : null;
            }
        }
    }

    protected void findEstimatedSpecializations(
            Rule[] beams, List<SpecOprWithScore>[] estimatedSpecLists, Rule[] topCandidates
    ) throws InterruptedSignal {
        int observations = (int) Math.round(beamwidth * observationRatio);
        int num_beams = 1;  // at least 1 beam in the "beams" array
        for (; num_beams < beamwidth && null != beams[num_beams]; num_beams++); // find the exact number of beams in the array
        int[] idxs = new int[num_beams];
        for (int i = 0; i < observations; i++) {
            int best_rule_idx = -1;
            double best_score = Eval.MIN.value(evalMetric);
            for (int rule_idx = 0; rule_idx < num_beams; rule_idx++) {
                List<SpecOprWithScore> est_spec_list = estimatedSpecLists[rule_idx];
                int idx = idxs[rule_idx];
                if (idx < est_spec_list.size()) {
                    double score = est_spec_list.get(idx).estEval.value(evalMetric);
                    if (best_score < score) {
                        best_score = score;
                        best_rule_idx = rule_idx;
                    }
                }
            }
            if (-1 == best_rule_idx) {
                /* No more option to compare */
                break;
            }
            SpecOprWithScore best_spec = estimatedSpecLists[best_rule_idx].get(idxs[best_rule_idx]);
            idxs[best_rule_idx]++;
            Rule copy = beams[best_rule_idx].clone();
            UpdateStatus status = best_spec.opr.specialize(copy);
            checkThenAddRule(status, copy, beams[best_rule_idx], topCandidates);
        }
    }
}
