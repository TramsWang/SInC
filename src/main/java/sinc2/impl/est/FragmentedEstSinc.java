package sinc2.impl.est;

import sinc2.RelationMiner;
import sinc2.SincConfig;
import sinc2.common.SincException;
import sinc2.impl.negsamp.SincWithFragmentedCachedRule;
import sinc2.kb.SimpleKb;
import sinc2.rule.EvalMetric;

public class FragmentedEstSinc extends SincWithFragmentedCachedRule {
    public FragmentedEstSinc(SincConfig config) throws SincException {
        super(config);
    }

    public FragmentedEstSinc(SincConfig config, SimpleKb kb) throws SincException {
        super(config, kb);
    }

    @Override
    protected RelationMiner createRelationMiner(int targetRelationNum) {
        return new FragmentedEstRelationMiner(
                kb, targetRelationNum, config.evalMetric, config.beamwidth, config.observationRatio,
                config.stopCompressionRatio, predicate2NodeMap, dependencyGraph, logger
        );
    }

    public static void main(String[] args) throws SincException {
        final SincConfig config = new SincConfig(
                "./datasets/SimpleFormat", "NELL-", ".", "TestFragEstSinc-NELL-", 1, false, 5,
                EvalMetric.CompressionRatio, 0.05, 0.25, 1, 5.0,
                null, null, false
        );
        FragmentedEstSinc sinc = new FragmentedEstSinc(config);
        sinc.run();
    }
}
