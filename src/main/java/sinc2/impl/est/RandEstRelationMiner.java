package sinc2.impl.est;

import sinc2.common.*;
import sinc2.impl.base.CachedRule;
import sinc2.impl.base.RelationMinerBasic;
import sinc2.kb.SimpleKb;
import sinc2.kb.SimpleRelation;
import sinc2.rule.*;
import sinc2.util.graph.GraphNode;

import java.io.PrintWriter;
import java.util.*;
import java.util.concurrent.ThreadLocalRandom;

public class RandEstRelationMiner extends RelationMinerBasic {
    /** The ratio (>=1) that extends the number of rules that are actually specialized according to the estimations */
    protected final double observationRatio;

    public RandEstRelationMiner(
            SimpleKb kb, int targetRelation, EvalMetric evalMetric, int beamwidth, double observationRatio,
            double stopCompressionRatio, Map<Predicate, GraphNode<Predicate>> predicate2NodeMap,
            Map<GraphNode<Predicate>, Set<GraphNode<Predicate>>> dependencyGraph, PrintWriter logger
    ) {
        super(kb, targetRelation, evalMetric, beamwidth, stopCompressionRatio, predicate2NodeMap, dependencyGraph, logger);
        this.observationRatio = observationRatio;
    }

    @Override
    protected Rule getStartRule() {
        return new RandEstRule(targetRelation, kb.getRelation(targetRelation).totalCols(), new HashSet<>(), tabuSet, kb);
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
                List<SpecOpr>[] spec_lists = new List[beamwidth];
                for (int i = 0; i < beamwidth && null != beams[i]; i++) {
                    Rule r = beams[i];
                    selectAsBeam(r);
                    logger.printf("Extend: %s\n", r.toString(kb));
                    logger.flush();
                    spec_lists[i] = ((RandEstRule) r).listSpecializations();
                }
                findRandomSpecializations(beams, spec_lists, top_candidates);

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

    protected void findRandomSpecializations(
            Rule[] beams, List<SpecOpr>[] specLists, Rule[] topCandidates
    ) throws InterruptedSignal {
        int observations = (int) Math.round(beamwidth * observationRatio);
        int num_beams = 1;  // at least 1 beam in the "beams" array
        for (; num_beams < beamwidth && null != beams[num_beams]; num_beams++); // find the exact number of beams in the array
        int[] idxs = new int[num_beams];
        class SpecInfo {
            public final int beamIdx;
            public final int specOprIdx;

            public SpecInfo(int beamIdx, int specOprIdx) {
                this.beamIdx = beamIdx;
                this.specOprIdx = specOprIdx;
            }

            @Override
            public boolean equals(Object o) {
                if (this == o) return true;
                if (o == null || getClass() != o.getClass()) return false;
                SpecInfo specInfo = (SpecInfo) o;
                return beamIdx == specInfo.beamIdx && specOprIdx == specInfo.specOprIdx;
            }

            @Override
            public int hashCode() {
                return Objects.hash(beamIdx, specOprIdx);
            }
        }
        Set<SpecInfo> rand_spec_infos = new HashSet<>();
        while (rand_spec_infos.size() < observations) {
            int beam_idx = ThreadLocalRandom.current().nextInt(num_beams);
            rand_spec_infos.add(new SpecInfo(
                    beam_idx,
                    ThreadLocalRandom.current().nextInt(specLists[beam_idx].size())
            ));
        }
        for (SpecInfo spec_info: rand_spec_infos) {
            Rule copy = beams[spec_info.beamIdx].clone();
            UpdateStatus status = specLists[spec_info.beamIdx].get(spec_info.specOprIdx).specialize(copy);
            checkThenAddRule(status, copy, beams[spec_info.beamIdx], topCandidates);
        }
    }
}
