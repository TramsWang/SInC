package sinc2.exp.hint.predefined;

import sinc2.exp.hint.HinterKb;
import sinc2.kb.SimpleRelation;
import sinc2.rule.Eval;
import sinc2.rule.EvalMetric;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

/**
 * The multi-thread version of TemplateMiner.
 *
 * @since 2.2
 */
abstract public class TemplateMinerMT extends TemplateMiner {
    /** The instance of comparator used for sorting entailed list */
    public static final EntailedRecordsComparator ENTAILED_RECORDS_COMPARATOR = new EntailedRecordsComparator();

    /**
     * This class is used for comparing entailed records in the entailment list. All entailed records contains exactly
     * 2 values.
     */
    static class EntailedRecordsComparator implements Comparator<int[]> {
        @Override
        public int compare(int[] arr1, int[] arr2) {
            int diff0 = arr1[0] - arr2[0];
            int diff1 = arr1[1] - arr2[1];
            return (0 == diff0) ? diff1 : diff0;
        }
    }

    /** The number of threads for new constructed template miners. This parameter can be changed. */
    public static int THREADS = 10;
    /** The maximum number of threads waiting in the thread pool. If the number of threads in the waiting queue reached
     *  this number, the new task will be blocked  */
    public static int MAX_THREADS = 10000;

    /** The thread pool */
    protected final ThreadPoolExecutor threadPool;

    /** Construct a miner and the number of threads is according to the static member value */
    public TemplateMinerMT() {
        threadPool = (ThreadPoolExecutor) Executors.newFixedThreadPool(THREADS);
    }

    /** Construct a miner and the number of threads is according to the parameter */
    public TemplateMinerMT(int threads) {
        threadPool = (ThreadPoolExecutor) Executors.newFixedThreadPool(threads);
    }

    @Override
    public List<MatchedRule> matchTemplate(HinterKb kb) {
        List<MatchedRule> matched_rules = new ArrayList<>();
        matchTemplateHandler(kb, matched_rules);
        threadPool.shutdown();
        try {
            if (!threadPool.awaitTermination(1, TimeUnit.HOURS)) {
                System.err.printf("TemplateMiner '%s' time out.\n", templateName());
            }
        } catch (InterruptedException e) {
            System.err.println("TemplateMinerMT thread pool interrupted");
            e.printStackTrace();
        }
        return matched_rules;
    }

    abstract protected void matchTemplateHandler(HinterKb kb, List<MatchedRule> matchedRules);

    @Override
    protected void checkThenAdd(
            SimpleRelation headRelation, int[][] entailments, List<MatchedRule> matchedRules, String ruleString
    ) {
        while (MAX_THREADS <= threadPool.getQueue().size()) {
            /* Block until some threads are consumed */
            try {
                Thread.sleep(1000);
            } catch (InterruptedException ignore) {}
        }
        threadPool.submit(() -> checkThenAddHandler(headRelation, entailments, matchedRules, ruleString));
    }

    protected void checkThenAddHandler(
            SimpleRelation headRelation, int[][] entailments, List<MatchedRule> matchedRules, String ruleString
    ) {
        synchronized (this) {
            checkCnt++;
        }
        if (checkCnt % 1000 == 0) {
            System.out.printf("Checked Rule: %dK (time used: %d ms)\n", checkCnt/1000, System.currentTimeMillis() - timeStart);
        }
        if (COVERAGE_THRESHOLD > ((double) entailments.length) / headRelation.totalRows()) {
            return;
        }
        int[][] positive_entailments = headRelation.intersectionWithSortedRows(entailments);
        double coverage = ((double) positive_entailments.length) / headRelation.totalRows();
        Eval eval = new Eval(positive_entailments.length, entailments.length, templateLength());
        if (COVERAGE_THRESHOLD <= coverage && TAU_THRESHOLD <= eval.value(EvalMetric.CompressionRatio)) {
            /* Add result */
            synchronized (matchedRules) {
                matchedRules.add(new MatchedRule(ruleString, eval, coverage));
            }
            synchronized (headRelation) {
                headRelation.setAllAsEntailed(positive_entailments);
            }
        }
    }
}
