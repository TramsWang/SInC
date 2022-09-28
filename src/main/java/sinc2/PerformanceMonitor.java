package sinc2;

import java.io.PrintWriter;

/**
 * Monitor class that records monitoring information.
 *
 * @since 1.0
 */
public class PerformanceMonitor {
    /* Time Monitors */
    public long kbLoadTime = 0;
    public long hypothesisMiningTime = 0;
    public long dependencyAnalysisTime = 0;
    public long dumpTime = 0;
    public long validationTime = 0;
    public long neo4jTime = 0;
    public long totalTime = 0;

    /* Mining Statics Monitors */
    public int kbFunctors = 0;
    public int kbConstants = 0;
    public int kbSize = 0;
    public int hypothesisRuleNumber = 0;
    public int hypothesisSize = 0;
    public int necessaryFacts = 0;
    public int counterexamples = 0;
    public int supplementaryConstants = 0;
    public int sccNumber = 0;
    public int sccVertices = 0;
    public int fvsVertices = 0;


    public void show(PrintWriter writer) {
        writer.println("\n### Monitored Performance Info ###\n");
        writer.println("--- Time Cost ---");
        writer.printf(
                "(ms) %10s %10s %10s %10s %10s %10s %10s\n",
                "Load", "Hypo", "Dep", "Dump", "Validate", "Neo4j", "Total"
        );
        writer.printf(
                "     %10d %10d %10d %10d %10d %10d %10d\n\n",
                kbLoadTime, hypothesisMiningTime, dependencyAnalysisTime, dumpTime, validationTime, neo4jTime, totalTime
        );

        writer.println("--- Statistics ---");
        writer.printf(
                "# %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s\n",
                "|P|", "|Σ|", "|B|", "|H|", "||H||", "|N|", "|A|", "|ΔΣ|", "#SCC", "|SCC|", "|FVS|", "Comp(%)"
        );
        writer.printf(
                "  %10d %10d %10d %10d %10d %10d %10d %10d %10d %10d %10d %10.2f\n\n",
                kbFunctors,
                kbConstants,
                kbSize,
                hypothesisRuleNumber,
                hypothesisSize,
                necessaryFacts,
                counterexamples,
                supplementaryConstants,
                sccNumber,
                sccVertices,
                fvsVertices,
                (necessaryFacts + counterexamples + hypothesisSize) * 100.0 / kbSize
        );
    }
}
