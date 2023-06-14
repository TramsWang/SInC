package sinc2.impl.negsamp;

import sinc2.RelationMiner;
import sinc2.common.Predicate;
import sinc2.kb.SimpleKb;
import sinc2.rule.EvalMetric;
import sinc2.rule.Rule;
import sinc2.util.graph.GraphNode;

import java.io.PrintWriter;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/**
 * The relation miner class that uses "FragmentedCachedRule".
 *
 * @since 2.3
 */
public class RelationMinerWithFragmentedCachedRule extends RelationMiner {
    /**
     * Construct by passing parameters from the compressor that loads the data.
     *
     * @param kb                   The input KB
     * @param targetRelation       The target relation in the KB
     * @param evalMetric           The rule evaluation metric
     * @param beamwidth            The beamwidth used in the rule mining procedure
     * @param stopCompressionRatio The stopping compression ratio for inducing a single rule
     * @param predicate2NodeMap    The mapping from predicates to the nodes in the dependency graph
     * @param dependencyGraph      The dependency graph
     * @param logger               A logger
     */
    public RelationMinerWithFragmentedCachedRule(
            SimpleKb kb, int targetRelation, EvalMetric evalMetric, int beamwidth, double stopCompressionRatio,
            Map<Predicate, GraphNode<Predicate>> predicate2NodeMap,
            Map<GraphNode<Predicate>, Set<GraphNode<Predicate>>> dependencyGraph, PrintWriter logger
    ) {
        super(kb, targetRelation, evalMetric, beamwidth, stopCompressionRatio, predicate2NodeMap, dependencyGraph, logger);
    }

    /**
     * Create a rule with compact caching and tabu set.
     */
    @Override
    protected Rule getStartRule() {
        return new FragmentedCachedRule(targetRelation, kb.getRelation(targetRelation).totalCols(), new HashSet<>(), tabuSet, kb);
    }

    /**
     * When a rule r is selected as beam, update its cache indices. The rule r here is a "CachedRule".
     */
    @Override
    protected void selectAsBeam(Rule r) {
        ((FragmentedCachedRule) r).updateCacheIndices();
    }
}
