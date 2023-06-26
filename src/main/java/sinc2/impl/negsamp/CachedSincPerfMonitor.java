package sinc2.impl.negsamp;

import sinc2.rule.Fingerprint;

import java.io.PrintWriter;

/**
 * Monitoring information for cache in SInC. Time is measured in nanoseconds.
 *
 * @since 2.3
 */
public class CachedSincPerfMonitor {
    /** 1ms = 1000000ns */
    public static final int NANO_PER_MILL = 1000000;

    public long posCacheUpdateTime = 0;
    public long prunedPosCacheUpdateTime = 0;
    public long entCacheUpdateTime = 0;
    public long allCacheUpdateTime = 0;
    public long posCacheIndexingTime = 0;
    public long entCacheIndexingTime = 0;
    public long allCacheIndexingTime = 0;
    public long evalTime = 0;
    public long kbUpdateTime = 0;
    public long counterexampleTime = 0;
    public long copyTime = 0;
    public long pruningTime = 0;

    public int posCacheEntriesTotal = 0;
    public int entCacheEntriesTotal = 0;
    public int allCacheEntriesTotal = 0;
    public int posCacheEntriesMax = 0;
    public int entCacheEntriesMax = 0;
    public int allCacheEntriesMax = 0;
    public int totalGeneratedRules = 0;

    public void show(PrintWriter writer) {
        writer.println("\n### Cached SInC Performance Info ###\n");
        writer.println("--- Time Cost ---");
        writer.printf(
                "(ms) %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s\n",
                "E+.Upd", "E+.Prune", "T.Upd", "E.Upd", "E+.Idx", "T.Idx", "E.Idx", "Eval", "KB Upd", "Ctr.Exp", "Copy", "Prune", "Fp", "Total"
        );
        writer.printf(
                "     %10d %10d %10d %10d %10d %10d %10d %10d %10d %10d %10d %10d %10d %10d\n\n",
                posCacheUpdateTime / NANO_PER_MILL, prunedPosCacheUpdateTime / NANO_PER_MILL, entCacheUpdateTime / NANO_PER_MILL, allCacheUpdateTime / NANO_PER_MILL,
                posCacheIndexingTime / NANO_PER_MILL, entCacheIndexingTime / NANO_PER_MILL, allCacheIndexingTime / NANO_PER_MILL,
                evalTime / NANO_PER_MILL, kbUpdateTime / NANO_PER_MILL, counterexampleTime / NANO_PER_MILL, copyTime / NANO_PER_MILL,
                pruningTime / NANO_PER_MILL, Fingerprint.getConstructTime() / NANO_PER_MILL,
                (posCacheUpdateTime + prunedPosCacheUpdateTime + entCacheUpdateTime + allCacheUpdateTime +
                        posCacheIndexingTime + entCacheIndexingTime + allCacheIndexingTime +
                        evalTime + kbUpdateTime + counterexampleTime + copyTime + pruningTime + Fingerprint.getConstructTime()
                ) / NANO_PER_MILL
        );

        writer.println("--- Statistics ---");
        writer.printf(
                "# %10s %10s %10s %10s %10s %10s %10s\n",
                "E+.Avg", "T.Avg", "E.Avg", "E+.Max", "T.Max", "E.Max", "Rules"
        );
        writer.printf(
                "  %10.2f %10.2f %10.2f %10d %10d %10d %10d\n\n",
                ((double) posCacheEntriesTotal) / totalGeneratedRules,
                ((double) entCacheEntriesTotal) / totalGeneratedRules,
                ((double) allCacheEntriesTotal) / totalGeneratedRules,
                posCacheEntriesMax, entCacheEntriesMax, allCacheEntriesMax, totalGeneratedRules
        );
    }
}
