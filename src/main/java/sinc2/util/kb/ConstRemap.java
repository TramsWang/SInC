package sinc2.util.kb;

import sinc2.common.Record;
import sinc2.kb.KbException;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;

public class ConstRemap {

    public static void main(String[] args) throws KbException, IOException {
        String input_path = args[0];
        String output_path = args[1];
        File input_path_file = new File(input_path);
        for (File f: input_path_file.listFiles()) {
            if (f.isDirectory()) {
                System.out.printf("Remapping: %s ...\n", f.getName());
                NumeratedKb kb = new NumeratedKb(f.getName(), input_path);
                System.out.println("- Alphabetical");
                remapByAlphabeticalOrder(kb, output_path);
                System.out.println("- Frequency");
                remapByFrequencyOrder(kb, output_path);
                System.out.println("- Random");
                remapByRandomOrder(kb, output_path);
            }
        }
    }

    static void remapByAlphabeticalOrder(NumeratedKb kb, String dumpPath) throws IOException {
        ArrayList<String> constants = new ArrayList<>(kb.getNumerationMap().numMap.keySet());
        constants.sort(String::compareTo);
        int[] re_mapping = new int[constants.size() + 1];
        for (int i = 0; i < constants.size(); i++) {
            String constant = constants.get(i);
            int old_num = kb.name2Num(constant);
            re_mapping[old_num] = i + 1;
        }
        NumeratedKb new_kb = new NumeratedKb(kb, kb.name + "_alpha");
        remap(new_kb, re_mapping);
        new_kb.dump(dumpPath);
    }

    static void remapByFrequencyOrder(NumeratedKb kb, String dumpPath) throws IOException {
        class Counter {
            final int num;
            int cnt = 0;

            public Counter(int num) {
                this.num = num;
            }
        }

        Counter[] counters = new Counter[kb.totalMappings() + 1];
        for (int i = 0; i < counters.length; i++) {
            counters[i] = new Counter(i);
        }
        counters[0].cnt = Integer.MAX_VALUE;
        for (KbRelation relation: kb.relations) {
            for (Record record: relation) {
                for (int arg: record.args) {
                    counters[arg].cnt++;
                }
            }
        }
        Arrays.sort(counters, Comparator.comparingInt((Counter counter) -> counter.cnt).reversed());
        int[] re_mapping = new int[counters.length];
        for (int i = 1; i < re_mapping.length; i++) {
            Counter counter = counters[i];
            re_mapping[counter.num] = i;
        }
        NumeratedKb new_kb = new NumeratedKb(kb, kb.name + "_freq");
        remap(new_kb, re_mapping);
        new_kb.dump(dumpPath);
    }

    static void remapByRandomOrder(NumeratedKb kb, String dumpPath) throws IOException {
        ArrayList<Integer> nums = new ArrayList<>(kb.totalMappings());
        for (int i = 1; i <= kb.totalMappings(); i++) {
            nums.add(i);
        }
        Collections.shuffle(nums);
        int[] re_mapping = new int[kb.totalMappings() + 1];
        for (int i = 0; i < nums.size(); i++) {
            re_mapping[i + 1] = nums.get(i);
        }
        NumeratedKb new_kb = new NumeratedKb(kb, kb.name + "_rand");
        remap(new_kb, re_mapping);
        new_kb.dump(dumpPath);
    }

    static void remap(NumeratedKb kb, int[] remapping) {
        for (KbRelation relation: kb.relations) {
            for (Record record: relation) {
                for (int arg_idx = 0; arg_idx < record.args.length; arg_idx++) {
                    record.args[arg_idx] = remapping[record.args[arg_idx]];
                }
            }
        }
    }
}
