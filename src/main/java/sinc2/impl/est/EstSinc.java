package sinc2.impl.est;

import sinc2.RelationMiner;
import sinc2.SincConfig;
import sinc2.common.SincException;
import sinc2.impl.base.SincBasic;
import sinc2.kb.SimpleKb;
import sinc2.rule.EvalMetric;

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

    public static void main(String[] args) throws SincException {
        final SincConfig config = new SincConfig(
                "./datasets/new", "UMLS", ".", "UMLS.comp", 1, false, 5,
                EvalMetric.CompressionRatio, 0.05, 0.25, 1, 2.0,
                null, null, false
        );
        EstSinc sinc = new EstSinc(config);
        sinc.run();
    }
}
