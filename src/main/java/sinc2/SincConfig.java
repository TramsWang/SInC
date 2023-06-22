package sinc2;

import sinc2.rule.Eval;
import sinc2.rule.EvalMetric;

/**
 * The configurations used in SInC.
 *
 * @since 1.0
 */
public class SincConfig {
    /* I/O configurations */
    /** The path to the directory where the kb is located */
    public final String basePath;
    /** The name of the KB */
    public final String kbName;
    /** The path where the compressed KB should be stored */
    public final String dumpPath;
    /** The name of the dumped KB */
    public final String dumpName;

    /* Runtime Config */
    /** The number of threads used to run SInC Todo: Implement multi-thread strategy */
    public int threads;
    /** Whether the compressed KB is recovered to check the correctness */
    public boolean validation;

    /* Algorithm Strategy Config */
    /** The beamwidth */
    public int beamwidth;
    // public boolean searchGeneralizations; Todo: Is it possible to efficiently update the cache for the generalizations? If so, implement the option here
    /** The rule evaluation metric */
    public EvalMetric evalMetric;
    /** The threshold for fact coverage */
    public double minFactCoverage;
    /** The threshold for constant coverage */
    public double minConstantCoverage;
    /** The threshold for maximum compression ratio of a single rule */
    public double stopCompressionRatio;
    /** The ratio (>=1) that extends the number of rules that are actually specialized according to the estimations */
    public double observationRatio;
    /** The base path to the negative sample KB on file system. If non-NULL, negative sampling will be employed in SInC.
     *  If "negKbName" is NULL, adversarial negative sampling will be used and this argument is ineffective. Otherwise,
     *  negative samples are from the loaded negative sample KB. */
    public String negKbBasePath;
    /** The name of the negative sample KB. */
    public String negKbName;
    /** Whether negative samples are weighted differently */
    public boolean weightedNegSamples;

    public SincConfig(
            String basePath, String kbName, String dumpPath, String dumpName, int threads, boolean validation,
            int beamwidth, EvalMetric evalMetric, double minFactCoverage, double minConstantCoverage,
            double stopCompressionRatio, double observationRatio,
            String negKbBasePath, String negKbName, boolean weightedNegSamples
    ) {
        this.basePath = basePath;
        this.kbName = kbName;
        this.dumpPath = dumpPath;
        this.dumpName = dumpName;
        this.threads = Math.max(1, threads);
        this.validation = validation;
        this.beamwidth = Math.max(1, beamwidth);
        this.evalMetric = evalMetric;
        this.minFactCoverage = minFactCoverage;
        this.minConstantCoverage = minConstantCoverage;
        this.stopCompressionRatio = Math.max(Eval.COMP_RATIO_USEFUL_THRESHOLD, stopCompressionRatio);  // make sure the stopping compression ratio threshold is useful
        this.observationRatio = Math.max(1.0, observationRatio);
        this.negKbBasePath = negKbBasePath;
        this.negKbName = negKbName;
        this.weightedNegSamples = weightedNegSamples;
    }
}
