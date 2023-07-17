package sinc2.exp.negsample;

import sinc2.kb.KbException;
import sinc2.kb.SimpleKb;
import sinc2.kb.SimpleRelation;
import sinc2.util.MultiSet;
import sinc2.util.io.IntWriter;

import java.io.IOException;
import java.util.*;

public class TestConstantRelevance {

    public static void main(String[] args) throws IOException, KbException {
        if (3 != args.length) {
            System.err.println("Usage: <KB Base Path> <KB Name> <Output File/Dir Name>");
            return;
        }

        String pos_base_path = args[0];
        String pos_kb_name = args[1];
        String out_file_name = args[2];

        dumpCoOccurrenceRelevance(pos_base_path, pos_kb_name, out_file_name);
    }

    //////////////////////////////////////////////////////////////
    //////////////// Test Co-occurrence Relevance ////////////////
    //////////////////////////////////////////////////////////////

    /**
     * Output format:
     *   - Total number of constants
     *   - For each constant of 1, 2, ...:
     *     - Total number of relevant constants
     *     - For each relevant constant c:
     *       - c
     *       - normalized relevance
     */
    static void dumpCoOccurrenceRelevance(String posBasePath, String posKbName, String outFileName) throws IOException {
        SimpleKb kb = new SimpleKb(posKbName, posBasePath);
        List<ConstantRelevance>[] direct_relevance_lists = calcDirectCoOccurrenceRelevance(kb);
        IntWriter writer = new IntWriter(outFileName);
        writer.write(direct_relevance_lists.length);
        for (List<ConstantRelevance> relevance_list: direct_relevance_lists) {
            writer.write(relevance_list.size());
            for (ConstantRelevance cr: relevance_list) {
                writer.write(cr.constant);
                writer.write(Float.floatToRawIntBits(cr.normalizedRelevance));
            }
        }
        writer.close();
    }

    static class ConstantRelevance {
        final int constant;
        final float normalizedRelevance;

        public ConstantRelevance(int constant, float normalizedRelevance) {
            this.constant = constant;
            this.normalizedRelevance = normalizedRelevance;
        }
    }

    static List<ConstantRelevance>[] calcDirectCoOccurrenceRelevance(SimpleKb kb) {
        MultiSet<Integer>[] co_occurrence_sets = new MultiSet[kb.totalConstants() + 1];
        for (int c = 1; c <= kb.totalConstants(); c++) {
            co_occurrence_sets[c] = new MultiSet<>();
        }

        for (SimpleRelation relation: kb.getRelations()) {
            for (int[] record: relation) {
                /* Here I made an assumption all relations in tested KBs are binary */
                final int c1 = record[0];
                final int c2 = record[1];
                co_occurrence_sets[c1].add(c2);
                co_occurrence_sets[c2].add(c1);
            }
        }

        List<ConstantRelevance>[] direct_relevance = new ArrayList[kb.totalConstants()];
        for (int c = 1; c <= direct_relevance.length; c++) {
            MultiSet<Integer> co_occurrences = co_occurrence_sets[c];
            float total_occurrences = co_occurrences.size();
            List<ConstantRelevance> relevances = new ArrayList<>();
            for (Map.Entry<Integer, Integer> entry: co_occurrences) {
                int constant = entry.getKey();
                int occurrence = entry.getValue();
                relevances.add(new ConstantRelevance(constant, occurrence / total_occurrences));
            }
            relevances.sort(Comparator.comparing((ConstantRelevance r) -> r.normalizedRelevance, Float::compare).reversed());
            direct_relevance[c-1] = relevances;
        }

        return direct_relevance;
    }
}
