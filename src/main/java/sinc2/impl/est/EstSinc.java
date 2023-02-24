package sinc2.impl.est;

import sinc2.RelationMiner;
import sinc2.SincConfig;
import sinc2.common.SincException;
import sinc2.impl.base.SincBasic;
import sinc2.kb.SimpleKb;

public class EstSinc extends SincBasic {
    public EstSinc(SincConfig config) throws SincException {
        super(config);
    }

    public EstSinc(SincConfig config, SimpleKb kb) throws SincException {
        super(config, kb);
    }

    @Override
    protected RelationMiner createRelationMiner(int targetRelationNum) {
        return new EstRelationMiner(
                kb, targetRelationNum, config.evalMetric, config.beamwidth, config.observationRatio,
                config.stopCompressionRatio, predicate2NodeMap, dependencyGraph, logger
        );
    }
}
