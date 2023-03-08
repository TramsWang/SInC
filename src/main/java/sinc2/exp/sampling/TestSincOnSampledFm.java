package sinc2.exp.sampling;

import sinc2.SInC;
import sinc2.SincConfig;
import sinc2.common.SincException;
import sinc2.impl.base.SincBasic;
import sinc2.kb.KbException;
import sinc2.kb.SimpleKb;
import sinc2.rule.EvalMetric;
import sinc2.rule.Rule;
import sinc2.sampling.EdgeSampler;
import sinc2.sampling.MDRWSampler;
import sinc2.sampling.SamplingInfo;
import sinc2.util.datagen.FamilyRelationGenerator;

import java.io.IOException;

/**
 * Test SInC on KBs sampled from "family.medium".
 *
 * @since 2.1
 */
public class TestSincOnSampledFm {
    static final int DUP = 3;

    static void runSinc(int families, double errorRate, int dimension, int budget) throws KbException, IOException, SincException {
        System.out.println("Generating KB ...");
        SimpleKb original_kb = FamilyRelationGenerator.generateMedium("original", families, errorRate);

//        System.out.println("Compressing original ...");
//        SincConfig config = new SincConfig(
//                null, null, "/dev/shm/TestSampling", "original_comp",
//                1, false, 5, EvalMetric.CompressionRatio, Rule.MIN_FACT_COVERAGE,
//                0.25, 1
//        );
//        SInC sinc = new SincBasic(config, original_kb);
//        sinc.run();

        EdgeSampler edge_sampler = new EdgeSampler();
        MDRWSampler mdrw_sampler = new MDRWSampler(dimension);
        for (int i = 0; i < DUP; i++) {
            System.out.printf("Compressing sampled #%d ...\n", i + 1);
            System.out.print("Try Edge Sampler ... ");
            long time_start = System.currentTimeMillis();
            SamplingInfo sampling_info = edge_sampler.sample(original_kb, budget, "edge_sampled");
            long time_done = System.currentTimeMillis();
            System.out.printf("(sampling time: %d ms) ...\n", time_done - time_start);
            SincConfig config = new SincConfig(
                    null, null, "/dev/shm/TestSampling", "edge_sampled_comp_" + (i + 1),
                    1, false, 5, EvalMetric.CompressionRatio, Rule.MIN_FACT_COVERAGE,
                    0.25, 1, 1.0
            );
            SInC sinc = new SincBasic(config, sampling_info.sampledKb);
            sinc.run();

            System.out.print("Try MDRW Sampler ... ");
            time_start = System.currentTimeMillis();
            sampling_info = mdrw_sampler.sample(original_kb, budget, "mdrw_sampled");
            time_done = System.currentTimeMillis();
            System.out.printf("(sampling time: %d ms) ...\n", time_done - time_start);
            config = new SincConfig(
                    null, null, "/dev/shm/TestSampling", "mdrw_sampled_comp_" + (i + 1),
                    1, false, 5, EvalMetric.CompressionRatio, Rule.MIN_FACT_COVERAGE,
                    0.25, 1, 1.0
            );
            sinc = new SincBasic(config, sampling_info.sampledKb);
            sinc.run();
        }
    }

    public static void main(String[] args) throws SincException, KbException, IOException {
        if (4 != args.length) {
            System.out.println("Usage: <families> <error rate> <MDRW sampler dimension> <budget>");
        }
        int families = Integer.parseInt(args[0]);
        double error_rate = Double.parseDouble(args[1]);
        int dimension = Integer.parseInt(args[2]);
        int budget = Integer.parseInt(args[3]);
        runSinc(families, error_rate, dimension, budget);
    }
}
