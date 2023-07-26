package sinc2.impl.negsamp;

import sinc2.RelationMiner;
import sinc2.SInC;
import sinc2.SincConfig;
import sinc2.SincRecovery;
import sinc2.common.SincException;
import sinc2.impl.base.SincRecoveryBasic;
import sinc2.kb.SimpleKb;
import sinc2.rule.EvalMetric;

public class SincWithFragmentedCachedRule extends SInC {

    protected CachedSincPerfMonitor monitor = new CachedSincPerfMonitor();

    /**
     * Create a SInC object with configurations.
     *
     * @param config The configurations
     */
    public SincWithFragmentedCachedRule(SincConfig config) throws SincException {
        super(config);
    }

    public SincWithFragmentedCachedRule(SincConfig config, SimpleKb kb) throws SincException {
        super(config, kb);
    }

    @Override
    protected SincRecovery createRecovery() {
        return new SincRecoveryBasic();
    }

    @Override
    protected RelationMiner createRelationMiner(int targetRelationNum) {
        return new RelationMinerWithFragmentedCachedRule(
                kb, targetRelationNum, config.evalMetric, config.beamwidth, config.stopCompressionRatio,
                predicate2NodeMap, dependencyGraph, logger
        );
    }

    @Override
    protected void finalizeRelationMiner(RelationMiner miner) {
        super.finalizeRelationMiner(miner);
        RelationMinerWithFragmentedCachedRule rel_miner = (RelationMinerWithFragmentedCachedRule) miner;
        monitor.posCacheUpdateTime += rel_miner.monitor.posCacheUpdateTime;
        monitor.prunedPosCacheUpdateTime += rel_miner.monitor.prunedPosCacheUpdateTime;
        monitor.entCacheUpdateTime += rel_miner.monitor.entCacheUpdateTime;
        monitor.allCacheUpdateTime += rel_miner.monitor.allCacheUpdateTime;
        monitor.posCacheIndexingTime += rel_miner.monitor.posCacheIndexingTime;
        monitor.entCacheIndexingTime += rel_miner.monitor.entCacheIndexingTime;
        monitor.allCacheIndexingTime += rel_miner.monitor.allCacheIndexingTime;
        monitor.posCacheEntriesTotal += rel_miner.monitor.posCacheEntriesTotal;
        monitor.entCacheEntriesTotal += rel_miner.monitor.entCacheEntriesTotal;
        monitor.allCacheEntriesTotal += rel_miner.monitor.allCacheEntriesTotal;
        monitor.posCacheEntriesMax = Math.max(monitor.posCacheEntriesMax, rel_miner.monitor.posCacheEntriesMax);
        monitor.entCacheEntriesMax = Math.max(monitor.entCacheEntriesMax, rel_miner.monitor.entCacheEntriesMax);
        monitor.allCacheEntriesMax = Math.max(monitor.allCacheEntriesMax, rel_miner.monitor.allCacheEntriesMax);
        monitor.totalGeneratedRules += rel_miner.monitor.totalGeneratedRules;
        monitor.copyTime += rel_miner.monitor.copyTime;
    }

    @Override
    protected void showMonitor() {
        super.showMonitor();
        monitor.show(logger);
    }

    public static void main(String[] args) throws SincException {
        SincWithFragmentedCachedRule sinc = new SincWithFragmentedCachedRule(new SincConfig(
                "datasets/SimpleFormat", "Fm", ".", "SincWithFrgCacheTest",
                1, false, 5, EvalMetric.CompressionRatio, 0.05,
                0.25, 1, 0.0,
                null, null, 0, false
        ));
        sinc.run();
    }
}
