package sinc2.util.datagen;

import sinc2.kb.KbException;
import sinc2.util.MultiSet;
import sinc2.util.kb.NumeratedKb;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

public class TsvExtractor {
    public static void main(String[] args) throws KbException, IOException {
        extract("FB-500", "datasets/SimpleFormat/FB15K.tsv", 500, "datasets/SimpleFormat");
//        extract("FB-", "datasets/SimpleFormat/FB15K.tsv", "datasets/SimpleFormat");
    }
    static void extract(String kbName, String tsvFileName, int threshold, String outputPath) throws IOException, KbException {
//        Map<String, RelationInfo> rel_info_map = loadRelationInfo(relationInfoFile);

        MultiSet<String> relation_set = new MultiSet<>();
        BufferedReader reader = new BufferedReader(new FileReader(tsvFileName));
        String line;
        while (null != (line = reader.readLine())) {
            String[] components = line.split("\t");
            relation_set.add(components[0]);
        }
        reader.close();

        NumeratedKb kb = new NumeratedKb(kbName);
        reader = new BufferedReader(new FileReader(tsvFileName));
        while (null != (line = reader.readLine())) {
            String[] components = line.split("\t");
            if (threshold <= relation_set.itemCount(components[0]) && 3 == components.length) {
                kb.addRecord(components[0], new String[]{components[1], components[2]});
            }
        }
        reader.close();
        kb.dump(outputPath);

        /* Show statistics */
        System.out.printf("KB: %s\n", kbName);
        System.out.printf("- Total Entities: %d\n", kb.totalMappings());
        System.out.printf("- Total Relations: %d\n", kb.totalRelations());
        System.out.printf("- Total Records: %d\n", kb.totalRecords());
    }

    static class RelationInfo {
        final String name;
        final int arity;
        final int totalRecords;

        public RelationInfo(String name, int arity, int totalRecords) {
            this.name = name;
            this.arity = arity;
            this.totalRecords = totalRecords;
        }
    }

    static Map<String, RelationInfo> loadRelationInfo(String fname) throws IOException {
        BufferedReader reader = new BufferedReader(new FileReader(fname));
        Map<String, RelationInfo> map = new HashMap<>();
        String line;
        while (null != (line = reader.readLine())) {
            String[] components = line.split("\t");
            RelationInfo info = new RelationInfo(components[0], Integer.parseInt(components[1]), Integer.parseInt(components[2]));
            if (100 <= info.totalRecords) {
                map.put(info.name, info);
            }
        }
        return map;
    }
}
