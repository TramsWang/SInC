package sinc2.impl.negsamp;

import sinc2.SInC;
import sinc2.common.Predicate;
import sinc2.common.Record;
import sinc2.kb.IntTable;
import sinc2.kb.KbException;
import sinc2.kb.SimpleKb;
import sinc2.rule.Eval;
import sinc2.rule.EvalMetric;
import sinc2.rule.Rule;
import sinc2.util.graph.GraphNode;

import java.io.PrintWriter;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/**
 * This relation miner employs "NegSampleCachedRule" to induce logic rules from a KB and given negative samples.
 *
 * @since 2.3
 */
public class NegSampleRelationMiner extends RelationMinerWithFragmentedCachedRule {

    /** The map from negative examples to their weight. If NULL, all negative examples are weighted as 1. */
    protected final Map<Record, Float> negSampleWeightMap;
    /** All negative samples, organized as an integer table */
    protected final IntTable negSamples;

    /**
     * @param negSampleWeightMap Weight for negative samples. If NULL, all samples are weighted as 1
     * @param negSamples         All negative samples, organized as an integer table
     */
    public NegSampleRelationMiner(
            SimpleKb kb, int targetRelation, EvalMetric evalMetric, int beamwidth, double stopCompressionRatio,
            Map<Predicate, GraphNode<Predicate>> predicate2NodeMap,
            Map<GraphNode<Predicate>, Set<GraphNode<Predicate>>> dependencyGraph, PrintWriter logger,
            Map<Record, Float> negSampleWeightMap, IntTable negSamples
    ) {
        super(kb, targetRelation, evalMetric, beamwidth, stopCompressionRatio, predicate2NodeMap, dependencyGraph, logger);
        this.negSampleWeightMap = negSampleWeightMap;
        this.negSamples = negSamples;
    }

    @Override
    protected Rule getStartRule() {
        return new NegSampleCachedRule(
                targetRelation, kb.getRelation(targetRelation).totalCols(), new HashSet<>(), tabuSet, kb,
                negSampleWeightMap, negSamples
        );
    }

    @Override
    protected void selectAsBeam(Rule r) {
        ((NegSampleCachedRule) r).updateCacheIndices();
    }

    /**
     * The difference between this method and its super is that it calculates the real evaluation score after a rule
     * has been found. Therefore, the calculation of compression ratio and rule score will not be misleading.
     */
    @Override
    public void run() throws KbException {
        Rule rule;
        int covered_facts = 0;
        final int total_facts = kb.getRelation(targetRelation).totalRows();
        while (!SInC.interrupted && (covered_facts < total_facts) && (null != (rule = findRule()))) {
            Eval est_eval = rule.getEval();
            ((NegSampleCachedRule) rule).calcRealEval();    // Replace the estimated evaluation score with the real one
            hypothesis.add(rule);
            covered_facts += updateKbAndDependencyGraph(rule);
            rule.releaseMemory();
            logger.printf(
                    "Found (Coverage: %.2f%%, %d/%d): %s\n", covered_facts * 100.0 / total_facts, covered_facts, total_facts,
                    rule.toDumpString(kb)
            );
            logger.printf("Est Eval: %s\n", est_eval);
            logger.flush();
        }
        logger.println("Done");
    }
}
