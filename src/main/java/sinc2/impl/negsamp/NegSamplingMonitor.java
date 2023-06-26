package sinc2.impl.negsamp;

import java.io.PrintWriter;

public class NegSamplingMonitor {
    /** 1ms = 1000000ns */
    public static final int NANO_PER_MILL = 1000000;

    public long negCacheUpdateTime = 0;
    public long negCacheIndexingTime = 0;
    public long realEvalCalTime = 0;
    public int negCacheEntriesTotal = 0;
    public int negCacheEntriesMax = 0;
    public int totalGeneratedRules = 0;

    public void show(PrintWriter writer) {
        writer.println("\n### Uniform/Pos-relative Negative Sampling SInC Performance Info ###\n");
        writer.println("--- Time Cost ---");
        writer.printf(
                "(ms) %10s %10s %10s %10s\n",
                "N.Upd", "N.Idx", "Real-Eval", "Total"
        );
        writer.printf(
                "     %10d %10d %10d %10d\n\n",
                negCacheUpdateTime / NANO_PER_MILL, negCacheIndexingTime / NANO_PER_MILL, realEvalCalTime / NANO_PER_MILL,
                (negCacheUpdateTime + negCacheIndexingTime + realEvalCalTime) / NANO_PER_MILL
        );

        writer.println("--- Statistics ---");
        writer.printf(
                "# %10s %10s\n",
                "N.Avg", "N.Max"
        );
        writer.printf(
                "  %10.2f %10d\n\n",
                ((double) negCacheEntriesTotal) / totalGeneratedRules, negCacheEntriesMax
        );
    }
}
