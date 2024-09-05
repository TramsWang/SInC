package sinc2.exp.negsample;

import sinc2.common.Record;
import sinc2.kb.*;
import sinc2.util.kb.NumerationMap;

import java.io.IOException;
import java.nio.file.Paths;
import java.util.*;

public class GenDiffIntMapping {
    /**
     * @param remapping remapping[old_num] = new_num
     */
    public static int[][] changeRows(int[][] rows, int[] remapping) {
        int[][] new_rows = new int[rows.length][];
        final int COLS = rows[0].length;
        for (int i = 0; i < rows.length; i++) {
            int[] row = rows[i];
            int[] new_row = new int[COLS];
            for (int j = 0; j < COLS; j++) {
                new_row[j] = remapping[row[j]];
            }
            new_rows[i] = new_row;
        }
        return new_rows;
    }

    /**
     * @param remapping remapping[old_num] = new_num
     */
    public static IntTable changeTable(IntTable table, int[] remapping) {
        return new IntTable(changeRows(table.getAllRows(), remapping));
    }

    /**
     * @param remapping remapping[old_num] = new_num
     */
    public static void changeMapping(NegSampleKb nkb, SimpleKb kb, int[] remapping, String newNegKbName, String dumpPath) throws IOException {
        IntTable[] new_neg_tables = new IntTable[kb.totalRelations()];
        Map<Record, Float>[] new_neg_weights = new Map[kb.totalRelations()];
        int[][][] new_kb_relations = new int[kb.totalRelations()][][];
        String[] rel_names = new String[kb.totalRelations()];
        for (int i = 0; i < kb.totalRelations(); i++) {
            IntTable kb_relation = kb.getRelation(i);
            new_kb_relations[i] = changeRows(kb_relation.getAllRows(), remapping);
            rel_names[i] = Integer.toString(i);

            IntTable neg_table = nkb.getNegSamples(i);
            new_neg_tables[i] = changeTable(neg_table, remapping);
        }
        SimpleKb new_kb = new SimpleKb("tmp", new_kb_relations, rel_names);

        for (int i = 0; i < kb.totalRelations(); i++) {
            new_neg_weights[i] = NegSampler.calcNegSampleWeight(new_kb.getRelation(i), new_neg_tables[i], kb.totalConstants());
        }
        NegSampleKb new_nkb = new NegSampleKb(newNegKbName, new_neg_tables, new_neg_weights);
        new_nkb.dump(dumpPath);
    }

    public static void remapNegKB(String basePath, String kbName, String negKbName) throws IOException {
        SimpleKb kb = new SimpleKb(kbName, basePath);
        NegSampleKb nkb = new NegSampleKb(negKbName, basePath);
        NumerationMap num_map = new NumerationMap(Paths.get(basePath, kbName).toString());

        /* Alphabetical Order */
        String[] constant_names = new String[num_map.totalMappings() + 1];
        Arrays.sort(constant_names, 1, constant_names.length);
        int[] remapping_by_alphabetical_order = new int[constant_names.length];
        for (int i = 1; i < constant_names.length; i++) {
            String const_name = constant_names[i];
            remapping_by_alphabetical_order[num_map.name2Num(const_name)] = i;
        }
        changeMapping(nkb, kb, remapping_by_alphabetical_order, negKbName + "_alpha", basePath);

        /* Frequency Order */
        class FreqPair {
            final int num;
            int cnt = 0;

            public FreqPair(int num) {
                this.num = num;
            }
        }
        FreqPair[] freq_pairs = new FreqPair[constant_names.length];
        for (int i = 1; i < freq_pairs.length; i++) {
            freq_pairs[i] = new FreqPair(i);
        }
        for (SimpleRelation relation: kb.getRelations()) {
            for (int[] row: relation) {
                for (int arg: row) {
                    freq_pairs[arg].cnt++;
                }
            }
        }
        Arrays.sort(freq_pairs, 1, freq_pairs.length, Comparator.comparingInt((FreqPair p) -> p.cnt).reversed());
        int[] remapping_by_freq = new int[constant_names.length];
        for (int i = 1; i < remapping_by_freq.length; i++) {
            int old_num = freq_pairs[i].num;
            remapping_by_freq[old_num] = i;
        }
        changeMapping(nkb, kb, remapping_by_freq, negKbName + "_freq", basePath);

        /* Random Order */
        List<Integer> permutation = new ArrayList<>(constant_names.length);
        for (int i = 1; i < constant_names.length; i++) {
            permutation.add(i);
        }
        Collections.shuffle(permutation);
        int[] remapping_by_rand = new int[constant_names.length];
        for (int i = 1; i < constant_names.length; i++) {
            remapping_by_rand[i] = permutation.get(i - 1);
        }
        changeMapping(nkb, kb, remapping_by_rand, negKbName + "_rand", basePath);
    }

    public static void main(String[] args) throws IOException {
        if (3 != args.length) {
            System.err.println("Args: <Base Path> <KB Name> <Neg KB Name>");
        }
        remapNegKB(args[0], args[1], args[2]);
    }
}
