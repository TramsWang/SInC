package sinc2.impl.negsamp;

import sinc2.RelationMiner;
import sinc2.SincConfig;
import sinc2.common.SincException;
import sinc2.kb.KbException;
import sinc2.kb.NegSampleKb;
import sinc2.kb.SimpleKb;

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
}
