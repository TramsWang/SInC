package sinc2.impl.negsamp;

import sinc2.common.Predicate;
import sinc2.common.Record;
import sinc2.kb.IntTable;
import sinc2.kb.SimpleKb;
import sinc2.rule.EvalMetric;
import sinc2.rule.Rule;
import sinc2.util.graph.GraphNode;

import java.io.PrintWriter;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class AdvNegRelationMiner extends NegSampleRelationMiner {

    protected final int budget;

    protected long samplingTime = 0;

    public AdvNegRelationMiner(
            SimpleKb kb, int targetRelation, EvalMetric evalMetric, int beamwidth, double stopCompressionRatio,
            Map<Predicate, GraphNode<Predicate>> predicate2NodeMap, Map<GraphNode<Predicate>, Set<GraphNode<Predicate>>> dependencyGraph,
            PrintWriter logger, Map<Record, Float> negSampleWeightMap, IntTable negSamples, float budgetFactor
    ) {
        super(
                kb, targetRelation, evalMetric, beamwidth, stopCompressionRatio, predicate2NodeMap, dependencyGraph,
                logger, negSampleWeightMap, negSamples
        );
        this.budget = (int) (kb.getRelation(targetRelation).totalRows() * budgetFactor);
    }

    @Override
    protected Rule getStartRule() {
        return new AdvNegCachedRule(
                targetRelation, kb.getRelation(targetRelation).totalCols(), new HashSet<>(), tabuSet, kb,
                negSampleWeightMap, negSamples, budget
        );
    }

    @Override
    protected void selectAsBeam(Rule r) {
        long time_start = System.nanoTime();
        ((AdvNegCachedRule) r).sample();
        samplingTime += System.nanoTime() - time_start;
        super.selectAsBeam(r);
    }
}
