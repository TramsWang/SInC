package sinc2.exp.negsample;

import com.google.gson.Gson;
import sinc2.common.Predicate;
import sinc2.common.Record;
import sinc2.impl.negsamp.CB;
import sinc2.impl.negsamp.NegSampleCachedRule;
import sinc2.impl.negsamp.NegSampleRelationMiner;
import sinc2.kb.*;
import sinc2.rule.EvalMetric;
import sinc2.rule.Fingerprint;
import sinc2.rule.Rule;
import sinc2.util.ArrayOperation;
import sinc2.util.MultiSet;
import sinc2.util.graph.GraphNode;
import sinc2.util.io.IntWriter;

import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.*;

/**
 * Observe how negative samples distributions in corresponding negative intervals.
 */
public class TestSampleDistribution {

    static final int BEAMWIDTH = 5;
    static final EvalMetric EVAL_METRIC = EvalMetric.CompressionRatio;

    public static void main(String[] args) throws IOException, KbException {
        if (5 != args.length) {
            System.err.println("Usage: <KB Base Path> <KB Name> <NegKB Base Path> <NegKB Name> <Output File Name>");
            return;
        }

        String pos_base_path = args[0];
        String pos_kb_name = args[1];
        String neg_base_path = args[2];
        String neg_kb_name = args[3];
        String out_file_name = args[4];

//        testDistInInterval(pos_base_path, pos_kb_name, neg_base_path, neg_kb_name, out_file_name);
//        testDistOverAll(pos_base_path, pos_kb_name, neg_base_path, neg_kb_name, out_file_name);
        dumpRemainingNegSamples(pos_base_path, pos_kb_name, neg_base_path, neg_kb_name, out_file_name);
//        testOrder(pos_base_path, pos_kb_name, neg_base_path, neg_kb_name, out_file_name);
    }

    ///////////////////////////////////////////////////////////////
    //////////////// Test Distribution in Interval ////////////////
    ///////////////////////////////////////////////////////////////

    static class Result4DistInInterval {
        public final float[][] startDist;
        public final List<Float>[] progressDist;

        public Result4DistInInterval(float[][] startDist, List<Float>[] progressDist) {
            this.startDist = startDist;
            this.progressDist = progressDist;
        }
    }

    static void testDistInInterval(String posBasePath, String posKbName, String negBasePath, String negKbName, String outFileName) throws IOException, KbException {
        SimpleKb kb = new SimpleKb(posKbName, posBasePath);
        kb.updatePromisingConstants();
        NegSampleKb neg_kb = new NegSampleKb(negKbName, negBasePath);

        Result4DistInInterval result = new Result4DistInInterval(
                observeStartingDistributionInInterval(kb, neg_kb),
                observeProgressingDistributionInInterval(kb, neg_kb)
        );
        FileWriter writer = new FileWriter(outFileName);
        Gson gson = new Gson();
        gson.toJson(result, writer);
        writer.close();
    }

    static float[][] observeStartingDistributionInInterval(SimpleKb kb, NegSampleKb negKb) {
        float[][] result = new float[kb.totalRelations()][];
        for (int rel_idx = 0; rel_idx < kb.totalRelations(); rel_idx++) {
            result[rel_idx] = observeStartingDistributionInInterval(kb.getRelation(rel_idx), negKb.getNegSamples(rel_idx), kb.totalConstants());
        }
        return result;
    }

    static float[] observeStartingDistributionInInterval(IntTable posRelation, IntTable negRelation, int totalConstants) {
        int[] insert_indices = posRelation.insertIndices(negRelation);
        float[] percentiles = new float[insert_indices.length];
        int[][] pos_records = posRelation.getAllRows();
        int[][] neg_records = negRelation.getAllRows();
        for (int i = 0; i < neg_records.length; i++) {
            percentiles[i] = getPercentileInInterval(pos_records, neg_records[i], insert_indices[i], totalConstants);
        }
        return percentiles;
    }

    static class TestNegRule extends NegSampleCachedRule {

        public TestNegRule(int headPredSymbol, int arity, Set<Fingerprint> fingerprintCache, Map<MultiSet<Integer>, Set<Fingerprint>> category2TabuSetMap, SimpleKb kb, Map<Record, Float> negSampleWeightMap, IntTable negSamples) {
            super(headPredSymbol, arity, fingerprintCache, category2TabuSetMap, kb, negSampleWeightMap, negSamples);
        }

        public TestNegRule(TestNegRule another) {
            super(another);
        }

        @Override
        public TestNegRule clone() {
            return new TestNegRule(this);
        }

        public IntTable getRemainingNegSamples() {
            Set<Record> remaining_neg_samples = new HashSet<>();
            for (List<CB> entry: negCache) {
                for (int[] record: entry.get(HEAD_PRED_IDX).complianceSet) {
                    remaining_neg_samples.add(new Record(record));
                }
            }
            if (remaining_neg_samples.isEmpty()) {
                return null;
            }
            int[][] records = new int[remaining_neg_samples.size()][];
            int idx = 0;
            for (Record record: remaining_neg_samples) {
                records[idx] = record.args;
                idx++;
            }
            return new IntTable(records);
        }
    }

    static class RelationMiner4DistInInterval extends NegSampleRelationMiner {
        List<Float> percentiles = new ArrayList<>();

        public RelationMiner4DistInInterval(
                SimpleKb kb, int targetRelation, EvalMetric evalMetric, int beamwidth, double stopCompressionRatio,
                Map<Predicate, GraphNode<Predicate>> predicate2NodeMap, Map<GraphNode<Predicate>, Set<GraphNode<Predicate>>> dependencyGraph,
                PrintWriter logger, Map<Record, Float> negSampleWeightMap, IntTable negSamples
        ) {
            super(kb, targetRelation, evalMetric, beamwidth, stopCompressionRatio, predicate2NodeMap, dependencyGraph, logger, negSampleWeightMap, negSamples);
            float[] _percentiles = observeStartingDistributionInInterval(kb.getRelation(targetRelation), negSamples, kb.totalConstants());
            for (float percentile: _percentiles) {
                percentiles.add(percentile);
            }
        }

        @Override
        protected Rule getStartRule() {
            return new TestNegRule(
                    targetRelation, kb.getRelation(targetRelation).totalCols(), new HashSet<>(), tabuSet, kb,
                    negSampleWeightMap, negSamples
            );
        }

        @Override
        protected void selectAsBeam(Rule r) {
            super.selectAsBeam(r);
            TestNegRule rule = (TestNegRule) r;
            IntTable remaining_neg_samples = rule.getRemainingNegSamples();
            if (null != remaining_neg_samples) {
                float[] _percentiles = observeStartingDistributionInInterval(kb.getRelation(targetRelation), remaining_neg_samples, kb.totalConstants());
                for (float percentile : _percentiles) {
                    percentiles.add(percentile);
                }
            }
        }

        @Override
        public void run() throws KbException {
            Rule rule = findRule();
            if (null == rule) {
                return;
            }
            rule.releaseMemory();
            logger.printf("Found: %s\n", rule.toDumpString(kb));
            logger.println("Done");
            logger.flush();
        }
    }

    static List<Float>[] observeProgressingDistributionInInterval(SimpleKb kb, NegSampleKb negKb) throws KbException {
        SimpleRelation[] relations = kb.getRelations();
        List<Float>[] lists = new List[relations.length];
        for (int rel_idx = 0; rel_idx < relations.length; rel_idx++) {
            lists[rel_idx] = observeProgressingDistributionInInterval(
                    kb, rel_idx, negKb.getNegSamples(rel_idx), negKb.getSampleWeight(rel_idx)
            );
        }
        return lists;
    }

    static List<Float> observeProgressingDistributionInInterval(
            SimpleKb kb, int relIdx, IntTable negSamples, Map<Record, Float> weightMap
    ) throws KbException {
        PrintWriter writer = new PrintWriter(System.out);
        RelationMiner4DistInInterval miner = new RelationMiner4DistInInterval(
                kb, relIdx, EVAL_METRIC, BEAMWIDTH, 1.0, null, null,
                writer, weightMap, negSamples
        );
        miner.run();
//        writer.close();
        return miner.percentiles;
    }

    static float getPercentileInInterval(int[][] posRecords, int[] negRecord, int insertIdx, int totalConstants) {
        final int[] start_record, end_record;
        final int delta;
        final int delta2;
        if (0 == insertIdx) {
            end_record = posRecords[0];
            start_record = ArrayOperation.initArrayWithValue(end_record.length, 1);
            delta = 0;
            delta2 = 1;
        } else if (posRecords.length <= insertIdx) {
            start_record = posRecords[posRecords.length - 1];
            end_record = ArrayOperation.initArrayWithValue(start_record.length, totalConstants);
            delta = 0;
            delta2 = 0;
        } else {
            start_record = posRecords[insertIdx - 1];
            end_record = posRecords[insertIdx];
            delta = 1;
            delta2 = 0;
        }
        float records_in_interval = NegSampler.recordsInInterval(start_record, end_record, totalConstants) - delta;
        float smaller_neg_records_in_interval = NegSampler.recordsInInterval(start_record, negRecord, totalConstants) + delta2;
        return  100 * smaller_neg_records_in_interval / records_in_interval;
    }


    ///////////////////////////////////////////////////////////
    //////////////// Test Overall Distribution ////////////////
    ///////////////////////////////////////////////////////////

    static class RelationMiner4DistOverAll extends NegSampleRelationMiner {
        List<Double> percentiles = new ArrayList<>();

        public RelationMiner4DistOverAll(
                SimpleKb kb, int targetRelation, EvalMetric evalMetric, int beamwidth, double stopCompressionRatio,
                Map<Predicate, GraphNode<Predicate>> predicate2NodeMap, Map<GraphNode<Predicate>, Set<GraphNode<Predicate>>> dependencyGraph,
                PrintWriter logger, Map<Record, Float> negSampleWeightMap, IntTable negSamples
        ) {
            super(kb, targetRelation, evalMetric, beamwidth, stopCompressionRatio, predicate2NodeMap, dependencyGraph, logger, negSampleWeightMap, negSamples);
            double[] _percentiles = getOverallPercentiles(negSamples.getAllRows(), kb.totalConstants());
            for (double percentile: _percentiles) {
                percentiles.add(percentile);
            }
        }

        @Override
        protected Rule getStartRule() {
            return new TestNegRule(
                    targetRelation, kb.getRelation(targetRelation).totalCols(), new HashSet<>(), tabuSet, kb,
                    negSampleWeightMap, negSamples
            );
        }

        @Override
        protected void selectAsBeam(Rule r) {
            super.selectAsBeam(r);
            TestNegRule rule = (TestNegRule) r;
            IntTable remaining_neg_samples = rule.getRemainingNegSamples();
            if (null != remaining_neg_samples) {
                double[] _percentiles = getOverallPercentiles(remaining_neg_samples.getAllRows(), kb.totalConstants());
                for (double percentile : _percentiles) {
                    percentiles.add(percentile);
                }
            }
        }

        @Override
        public void run() throws KbException {
            Rule rule = findRule();
            if (null == rule) {
                return;
            }
            rule.releaseMemory();
            logger.printf("Found: %s\n", rule.toDumpString(kb));
            logger.println("Done");
            logger.flush();
        }
    }

    static class Result4DistOverall {
        final double[][] posDist;
        final double[][] negStartDist;
        final List<Double>[] negProgressDist;

        public Result4DistOverall(double[][] posDist, double[][] negStartDist, List<Double>[] negProgressDist) {
            this.posDist = posDist;
            this.negStartDist = negStartDist;
            this.negProgressDist = negProgressDist;
        }
    }

    static void testDistOverAll(String posBasePath, String posKbName, String negBasePath, String negKbName, String outFileName) throws IOException, KbException {
        SimpleKb kb = new SimpleKb(posKbName, posBasePath);
        kb.updatePromisingConstants();
        NegSampleKb neg_kb = new NegSampleKb(negKbName, negBasePath);

        SimpleRelation[] relations = kb.getRelations();
        double[][] pos_percentiles = new double[relations.length][];
        for (int rel_idx = 0; rel_idx < relations.length; rel_idx++) {
            pos_percentiles[rel_idx] = getOverallPercentiles(relations[rel_idx].getAllRows(), kb.totalConstants());
        }

        Result4DistOverall result = new Result4DistOverall(
                pos_percentiles,
                observeStartingDistributionOverAll(kb, neg_kb),
                observeProgressingDistributionOverAll(kb, neg_kb)
        );
        FileWriter writer = new FileWriter(outFileName);
        Gson gson = new Gson();
        gson.toJson(result, writer);
        writer.close();
    }

    static double[][] observeStartingDistributionOverAll(SimpleKb kb, NegSampleKb negKb) {
        double[][] result = new double[kb.totalRelations()][];
        for (int rel_idx = 0; rel_idx < result.length; rel_idx++) {
            result[rel_idx] = getOverallPercentiles(negKb.getNegSamples(rel_idx).getAllRows(), kb.totalConstants());
        }
        return result;
    }

    static List<Double>[] observeProgressingDistributionOverAll(SimpleKb kb, NegSampleKb negKb) throws KbException {
        PrintWriter writer = new PrintWriter(System.out);
        SimpleRelation[] relations = kb.getRelations();
        List<Double>[] result = new List[relations.length];
        for (int rel_idx = 0; rel_idx < relations.length; rel_idx++) {
            RelationMiner4DistOverAll miner = new RelationMiner4DistOverAll(
                    kb, rel_idx, EVAL_METRIC, BEAMWIDTH, 1.0, null, null,
                    writer, negKb.getSampleWeight(rel_idx), negKb.getNegSamples(rel_idx)
            );
            miner.run();
            result[rel_idx] = miner.percentiles;
        }
        return result;
    }

    static double[] getOverallPercentiles(int[][] records, int totalConstants) {
        final double all_numbers = Math.pow(totalConstants, records[0].length);
        double[] percentiles = new double[records.length];
        for (int i = 0; i < records.length; i++) {
            percentiles[i] = 100 * toNumber(records[i], totalConstants) / all_numbers;
        }
        return percentiles;
    }

    static double toNumber(int[] record, int totalConstants) {
        double num = record[0] - 1;
        for (int i = 1; i < record.length; i++) {
            num = num * totalConstants + record[i] - 1;
        }
        return num;
    }

    //////////////////////////////////////////////////////////////////////////////
    //////////////// Get Remaining Neg Records in Mining Progress ////////////////
    //////////////////////////////////////////////////////////////////////////////

    static class RelationMiner4RemainingNegSamples extends NegSampleRelationMiner {
        List<int[][]> remainingNegSamples = new ArrayList<>();

        public RelationMiner4RemainingNegSamples(
                SimpleKb kb, int targetRelation, EvalMetric evalMetric, int beamwidth, double stopCompressionRatio,
                Map<Predicate, GraphNode<Predicate>> predicate2NodeMap, Map<GraphNode<Predicate>, Set<GraphNode<Predicate>>> dependencyGraph,
                PrintWriter logger, Map<Record, Float> negSampleWeightMap, IntTable negSamples
        ) {
            super(kb, targetRelation, evalMetric, beamwidth, stopCompressionRatio, predicate2NodeMap, dependencyGraph, logger, negSampleWeightMap, negSamples);
        }

        @Override
        protected Rule getStartRule() {
            return new TestNegRule(
                    targetRelation, kb.getRelation(targetRelation).totalCols(), new HashSet<>(), tabuSet, kb,
                    negSampleWeightMap, negSamples
            );
        }

        @Override
        protected void selectAsBeam(Rule r) {
            super.selectAsBeam(r);
            TestNegRule rule = (TestNegRule) r;
            IntTable remaining_neg_samples = rule.getRemainingNegSamples();
            if (null != remaining_neg_samples) {
                remainingNegSamples.add(remaining_neg_samples.getAllRows());
            }
        }

        @Override
        public void run() throws KbException {
            Rule rule = findRule();
            if (null == rule) {
                return;
            }
            rule.releaseMemory();
            logger.printf("Found: %s\n", rule.toDumpString(kb));
            logger.println("Done");
            logger.flush();
        }
    }

    /**
     * Structure of dump KB:
     *   - Relations.dat: Meta information, elements (in order) are:
     *     - total number of relations
     *     - arity and iterations in the relation
     *     - number of records in each relation
     *     - E.g. [#rel, arity_0, itr_0, #rec_0_0, ..., #rec_0_(itr_0-1), ...]
     *   - <ID>.neg: relation file
     */
    static void dumpRemainingNegSamples(String posBasePath, String posKbName, String negBasePath, String negKbName, String outDirName) throws IOException, KbException {
        SimpleKb kb = new SimpleKb(posKbName, posBasePath);
        kb.updatePromisingConstants();
        NegSampleKb neg_kb = new NegSampleKb(negKbName, negBasePath);

        PrintWriter writer = new PrintWriter(System.out);
        SimpleRelation[] relations = kb.getRelations();
        IntWriter meta_writer = new IntWriter(outDirName + "/Relations.dat");
        meta_writer.write(relations.length);    // total relations
        for (int rel_idx = 0; rel_idx < relations.length; rel_idx++) {
            RelationMiner4RemainingNegSamples miner = new RelationMiner4RemainingNegSamples(
                    kb, rel_idx, EVAL_METRIC, BEAMWIDTH, 1.0, null, null,
                    writer, neg_kb.getSampleWeight(rel_idx), neg_kb.getNegSamples(rel_idx)
            );
            miner.run();

            /* Dump records */
            meta_writer.write(relations[rel_idx].totalCols());  // arity
            meta_writer.write(miner.remainingNegSamples.size());         // total iterations
            IntWriter int_writer = new IntWriter(outDirName + '/' + rel_idx + ".neg");
            for (int[][] neg_samples: miner.remainingNegSamples) {
                meta_writer.write(neg_samples.length);  // total records in iteration
                for (int[] neg_sample: neg_samples) {
                    for (int arg : neg_sample) {
                        int_writer.write(arg);
                    }
                }
            }
            int_writer.close();
        }
        meta_writer.close();
    }

    static double[] calcMinAlphabeticalDist(IntTable posRecords, IntTable negRecords, int totalConstants) {
        int[] insert_indices = posRecords.insertIndices(negRecords);
        int[][] pos_records = posRecords.getAllRows();
        int[][] neg_records = negRecords.getAllRows();
        double[] min_distances = new double[insert_indices.length];
        for (int i = 0; i < insert_indices.length; i++) {
            int insert_index = insert_indices[i];
            final double dist;
            if (0 == insert_index) {
                dist = NegSampler.recordsInInterval(neg_records[i], pos_records[0], totalConstants);
            } else if (pos_records.length <= insert_index) {
                dist = NegSampler.recordsInInterval(pos_records[pos_records.length - 1], neg_records[i], totalConstants);
            } else {
                dist = Math.min(
                        NegSampler.recordsInInterval(pos_records[insert_index - 1], neg_records[i], totalConstants),
                        NegSampler.recordsInInterval(neg_records[i], pos_records[insert_index], totalConstants)
                );
            }
            min_distances[i] = dist;
        }
        return min_distances;
    }

    static void testOrder(String posBasePath, String posKbName, String negBasePath, String negKbName, String outDirName) throws IOException, KbException {
        SimpleKb kb = new SimpleKb(posKbName, posBasePath);
        kb.updatePromisingConstants();
        for (SimpleRelation relation: kb.getRelations()) {
            int[][] rows = relation.getAllRows();
            for (int i = 1; i < rows.length; i++) {
                if (recordGreaterThanOrEqual(rows[i-1], rows[i])) {
                    System.out.printf("%s > %s", Arrays.toString(rows[i - 1]), Arrays.toString(rows[i]));
                }
            }
        }
    }

    static boolean recordGreaterThanOrEqual(int[] row1, int[] row2) {
        for (int i = 0; i < row1.length; i++) {
            if (row1[i] < row2[i]) {
                return false;
            } else if (row1[i] > row2[i]) {
                return true;
            }
        }
        return true;
    }
}
