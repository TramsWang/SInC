package sinc2.util.datagen;

import sinc2.common.Record;
import sinc2.kb.SimpleKb;
import sinc2.util.ArrayOperation;

import java.io.IOException;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.ThreadLocalRandom;

public class RandomGenerator {
    public static void main(String[] args) throws IOException {
        final int TOTAL_CONST = 10;
        final int TOTAL_RELATIONS = 10;
        final int DUPLICATIONS = 10;
        final int RECORDS_IN_RELATION = 10;

        String[] mapped_names = new String[TOTAL_CONST + 1];
        for (int c = 1; c <= TOTAL_CONST; c++) {
            mapped_names[c] = Integer.toString(c);
        }

        String[] relation_names = new String[TOTAL_RELATIONS];
        for (int rel = 0; rel < TOTAL_RELATIONS; rel++) {
            relation_names[rel] = String.valueOf(Character.toChars('a' + rel));
        }

        for (int dup = 0; dup < DUPLICATIONS; dup++) {
            System.out.printf("Randomizing KB %d ...\n", dup);
            int[][][] relations = new int[TOTAL_RELATIONS][][];
            for (int rel = 0; rel < TOTAL_RELATIONS; rel++) {
                int arity = rel + 1;
                Set<Record> record_set = new HashSet<>();
                for (int rec = 0; rec < RECORDS_IN_RELATION; rec++) {
                    int[] record = new int[arity];
                    for (int i = 0; i < arity; i++) {
                        record[i] = ThreadLocalRandom.current().nextInt(1, TOTAL_CONST + 1);
                    }
                    record_set.add(new Record(record));
                }
                int[][] relation = new int[record_set.size()][];
                int i = 0;
                for (Record record: record_set) {
                    relation[i] = record.args;
                    i++;
                }
                relations[rel] = relation;
            }
            SimpleKb kb = new SimpleKb("Rand" + dup, relations, relation_names);
            kb.dump(".", mapped_names);
        }
    }
}
