package sinc2.impl.base;

import sinc2.RelationMiner;
import sinc2.SInC;
import sinc2.SincConfig;
import sinc2.SincRecovery;
import sinc2.common.SincException;
import sinc2.kb.SimpleKb;

/**
 * A basic implementation of SInC. Rule mining are with compact caching and tabu pruning.
 *
 * @since 2.0
 */
public class SincBasic extends SInC {
    /**
     * Create a SInC object with configurations.
     *
     * @param config The configurations
     */
    public SincBasic(SincConfig config) throws SincException {
        super(config);
    }

    public SincBasic(SincConfig config, SimpleKb kb) throws SincException {
        super(config, kb);
    }

    @Override
    protected SincRecovery createRecovery() {
        return new SincRecoveryBasic();
    }

    @Override
    protected RelationMiner createRelationMiner(int targetRelationNum) {
        return new RelationMinerBasic(
                kb, targetRelationNum, config.evalMetric, config.beamwidth, config.stopCompressionRatio,
                predicate2NodeMap, dependencyGraph, logger
        );
    }
}
