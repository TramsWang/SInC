package sinc2.impl.est;

import sinc2.RelationMiner;
import sinc2.SincConfig;
import sinc2.common.SincException;
import sinc2.impl.base.SincBasic;
import sinc2.kb.SimpleKb;
import sinc2.rule.EvalMetric;

public class RandEstSinc extends SincBasic {
    public RandEstSinc(SincConfig config) throws SincException {
        super(config);
    }

    public RandEstSinc(SincConfig config, SimpleKb kb) throws SincException {
        super(config, kb);
    }

    @Override
    protected RelationMiner createRelationMiner(int targetRelationNum) {
        return new RandEstRelationMiner(
                kb, targetRelationNum, config.evalMetric, config.beamwidth, config.observationRatio,
                config.stopCompressionRatio, predicate2NodeMap, dependencyGraph, logger
        );
    }

    public static void main(String[] args) throws SincException {
        final SincConfig config = new SincConfig(
                "./datasets/SimpleFormat", "WN18", ".", "WN18-comp", 1, false, 5,
                EvalMetric.CompressionRatio, 0.05, 0.25, 1, 2.0,
                null, null, 0, false
        );
        RandEstSinc sinc = new RandEstSinc(config);
        sinc.run();
    }
}
