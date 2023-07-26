package sinc2.impl.negsamp;

import sinc2.RelationMiner;
import sinc2.SincConfig;
import sinc2.common.SincException;
import sinc2.kb.NegSampleKb;
import sinc2.kb.SimpleKb;
import sinc2.rule.EvalMetric;

public class SincWithAdvNegSampling extends SincWithNegSampling {

    protected long samplingTime = 0;

    public SincWithAdvNegSampling(SincConfig config) throws SincException {
        super(config);
    }

    public SincWithAdvNegSampling(SincConfig config, SimpleKb kb, NegSampleKb negKb) throws SincException {
        super(config, kb, negKb);
    }

    @Override
    protected RelationMiner createRelationMiner(int targetRelationNum) {
        return new AdvNegRelationMiner(
                kb, targetRelationNum, config.evalMetric, config.beamwidth, config.stopCompressionRatio,
                predicate2NodeMap, dependencyGraph, logger,
                config.weightedNegSamples ? negKb.getSampleWeight(targetRelationNum) : null,
                negKb.getNegSamples(targetRelationNum), config.budgetFactor
        );
    }

    @Override
    protected void finalizeRelationMiner(RelationMiner miner) {
        super.finalizeRelationMiner(miner);
        samplingTime += ((AdvNegRelationMiner) miner).samplingTime;
    }

    @Override
    protected void showMonitor() {
        super.showMonitor();

        logger.println("\n### Adversarial Negative Sampling SInC Performance Info ###\n");
        logger.println("--- Time Cost ---");
        logger.printf("(ms) %10s\n", "Sampling");
        logger.printf("     %10d\n\n", samplingTime / NegSamplingMonitor.NANO_PER_MILL);
        logger.flush();
    }

    public static void main(String[] args) throws SincException {
        SincWithAdvNegSampling sinc = new SincWithAdvNegSampling(new SincConfig(
                "datasets/SimpleFormat", "Fm", ".", "SincWithAdvNegTest-Fm",
                1, false, 5, EvalMetric.CompressionRatio, 0.05,
                0.25, 1, 0.0,
                "datasets/Neg", "Fm_neg_pos_rel_1.0", 1.0f, true
        ));
        sinc.run();
    }
}
