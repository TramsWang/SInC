package sinc2.impl.negsamp;

import sinc2.RelationMiner;
import sinc2.SInC;
import sinc2.SincConfig;
import sinc2.common.SincException;
import sinc2.kb.KbException;
import sinc2.kb.NegSampleKb;
import sinc2.kb.SimpleKb;
import sinc2.rule.EvalMetric;

import java.io.IOException;

/**
 * This version of SInC loads both positive and negative samples from file system and evaluate rules according to these
 * samples instead of accurate counting of negative records.
 *
 * @since 2.3
 */
public class SincWithNegSampling extends SincWithFragmentedCachedRule {

    /** The KB of negative samples */
    protected NegSampleKb negKb;

    protected NegSamplingMonitor monitor = new NegSamplingMonitor();

    public SincWithNegSampling(SincConfig config) throws SincException {
        super(config);
    }

    public SincWithNegSampling(SincConfig config, SimpleKb kb, NegSampleKb negKb) throws SincException {
        super(config, kb);
        this.negKb = negKb;
    }

    protected void loadKb() throws IOException, KbException {
        super.loadKb();
        if (null == negKb) {
            negKb = new NegSampleKb(config.negKbName, config.negKbBasePath);
        }
    }

    @Override
    protected RelationMiner createRelationMiner(int targetRelationNum) {
        return new NegSampleRelationMiner(
                kb, targetRelationNum, config.evalMetric, config.beamwidth, config.stopCompressionRatio,
                predicate2NodeMap, dependencyGraph, logger,
                config.weightedNegSamples ? negKb.getSampleWeight(targetRelationNum) : null,
                negKb.getNegSamples(targetRelationNum)
        );
    }

    @Override
    protected void finalizeRelationMiner(RelationMiner miner) {
        super.finalizeRelationMiner(miner);
        NegSampleRelationMiner rel_miner = (NegSampleRelationMiner) miner;
        monitor.negCacheUpdateTime += rel_miner.monitor.negCacheUpdateTime;
        monitor.negCacheIndexingTime += rel_miner.monitor.negCacheIndexingTime;
        monitor.realEvalCalTime += rel_miner.monitor.realEvalCalTime;
        monitor.negCacheEntriesTotal += rel_miner.monitor.negCacheEntriesTotal;
        monitor.negCacheEntriesMax = Math.max(monitor.negCacheEntriesMax, rel_miner.monitor.negCacheEntriesMax);
    }

    @Override
    protected void showMonitor() {
        super.showMonitor();
        monitor.totalGeneratedRules = super.monitor.totalGeneratedRules;
        monitor.show(logger);
    }

    public static void main(String[] args) throws SincException {
        SincWithNegSampling sinc = new SincWithNegSampling(new SincConfig(
                "datasets/SimpleFormat", "Fm", ".", "SincWithNegSamplingTest",
                1, false, 5, EvalMetric.CompressionRatio, 0.05,
                0.25, 1, 0.0,
                "datasets/Neg", "Fm_neg_pos_rel_5.0", 0, true
        ));
        sinc.run();
    }
}
