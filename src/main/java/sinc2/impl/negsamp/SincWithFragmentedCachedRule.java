package sinc2.impl.negsamp;

import sinc2.RelationMiner;
import sinc2.SInC;
import sinc2.SincConfig;
import sinc2.SincRecovery;
import sinc2.common.SincException;
import sinc2.impl.base.SincRecoveryBasic;
import sinc2.kb.SimpleKb;

public class SincWithFragmentedCachedRule extends SInC {
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
}
