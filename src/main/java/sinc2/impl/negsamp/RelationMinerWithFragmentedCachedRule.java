package sinc2.impl.negsamp;

import sinc2.RelationMiner;
import sinc2.SInC;
import sinc2.common.InterruptedSignal;
import sinc2.common.Predicate;
import sinc2.kb.KbException;
import sinc2.kb.SimpleKb;
import sinc2.rule.EvalMetric;
import sinc2.rule.Rule;
import sinc2.rule.UpdateStatus;
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

    public final CachedSincPerfMonitor monitor = new CachedSincPerfMonitor();

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
        FragmentedCachedRule rule = (FragmentedCachedRule) r;
        rule.updateCacheIndices();
        monitor.posCacheIndexingTime += rule.posCacheIndexingTime;
        monitor.entCacheIndexingTime += rule.entCacheIndexingTime;
        monitor.allCacheIndexingTime += rule.allCacheIndexingTime;
    }

    /**
     * Record monitoring information compared to the super implementation
     */
    @Override
    protected int checkThenAddRule(UpdateStatus updateStatus, Rule updatedRule, Rule originalRule, Rule[] candidates) throws InterruptedSignal {
        FragmentedCachedRule rule = (FragmentedCachedRule) updatedRule;
        if (UpdateStatus.NORMAL == updateStatus) {
            monitor.posCacheEntriesTotal += rule.posCache.totalEntries();
            monitor.entCacheEntriesTotal += rule.entCache.totalEntries();
            int all_cache_entries = 0;
            if (!rule.allCache.isEmpty()) {
                for (CacheFragment fragment : rule.allCache) {
                    all_cache_entries += fragment.totalEntries();
                }
                all_cache_entries /= rule.allCache.size();
            }
            monitor.allCacheEntriesTotal += all_cache_entries;
            monitor.posCacheEntriesMax = Math.max(monitor.posCacheEntriesMax, rule.posCache.totalEntries());
            monitor.entCacheEntriesMax = Math.max(monitor.entCacheEntriesMax, rule.entCache.totalEntries());
            monitor.allCacheEntriesMax = Math.max(monitor.allCacheEntriesMax, all_cache_entries);
            monitor.totalGeneratedRules++;
            monitor.posCacheUpdateTime += rule.posCacheUpdateTime;
            monitor.entCacheUpdateTime += rule.entCacheUpdateTime;
            monitor.allCacheUpdateTime += rule.allCacheUpdateTime;
            monitor.evalTime += rule.evalTime;
        } else {
            monitor.prunedPosCacheUpdateTime += rule.posCacheUpdateTime;
        }
        monitor.copyTime += rule.copyTime;
        monitor.pruningTime += rule.getPruningTime();

        return super.checkThenAddRule(updateStatus, updatedRule, originalRule, candidates);
    }

    public void run() throws KbException {
        Rule rule;
        int covered_facts = 0;
        final int total_facts = kb.getRelation(targetRelation).totalRows();
        while (!SInC.interrupted && (covered_facts < total_facts) && (null != (rule = findRule()))) {
            hypothesis.add(rule);
            covered_facts += updateKbAndDependencyGraph(rule);
            rule.releaseMemory();
            monitor.kbUpdateTime += ((FragmentedCachedRule) rule).kbUpdateTime;
            logger.printf(
                    "Found (Coverage: %.2f%%, %d/%d): %s\n", covered_facts * 100.0 / total_facts, covered_facts, total_facts,
                    rule.toDumpString(kb)
            );
            logger.flush();
        }
        logger.println("Done");
    }
}
