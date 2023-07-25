package sinc2.util.datagen;

import com.google.gson.Gson;
import sinc2.kb.KbException;
import sinc2.util.kb.NumeratedKb;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;

public class CodexExtractor {
    static final String[] TRIPLE_FILES = new String[] {"train.txt", "valid.txt", "test.txt"};
    static final String TYPE_FILE = "entity2types.json";

    public static void main(String[] args) throws KbException, IOException {
//        extractWithoutType("Cs", "/home/Ruoyu/codex/data/triples/codex-s", "datasets/SimpleFormat");
//        extractWithoutType("Cm", "/home/Ruoyu/codex/data/triples/codex-m", "datasets/SimpleFormat");
//        extractWithoutType("Cl", "/home/Ruoyu/codex/data/triples/codex-l", "datasets/SimpleFormat");
        extractWithType("Cts", "/home/Ruoyu/codex/data/triples/codex-s", "/home/Ruoyu/codex/data/types", "datasets/SimpleFormat");
        extractWithType("Ctm", "/home/Ruoyu/codex/data/triples/codex-m", "/home/Ruoyu/codex/data/types", "datasets/SimpleFormat");
        extractWithType("Ctl", "/home/Ruoyu/codex/data/triples/codex-l", "/home/Ruoyu/codex/data/types", "datasets/SimpleFormat");
    }

    static void extractWithoutType(String kbName, String inputPath, String outputPath) throws IOException, KbException {
        NumeratedKb kb = new NumeratedKb(kbName);
        for (String tripe_file_name: TRIPLE_FILES) {
            BufferedReader reader = new BufferedReader(new FileReader(Paths.get(inputPath, tripe_file_name).toFile()));
            String line;
            while (null != (line = reader.readLine())) {
                String[] components = line.split("\t");
                kb.addRecord(components[1], new String[]{components[0], components[2]});
            }
            reader.close();
        }
        kb.dump(outputPath);

        /* Show statistics */
        System.out.printf("KB: %s\n", kbName);
        System.out.printf("- Total Entities: %d\n", kb.totalMappings());
        System.out.printf("- Total Relations: %d\n", kb.totalRelations());
        System.out.printf("- Total Records: %d\n", kb.totalRecords());
    }

    static void extractWithType(String kbName, String tripeFilesPath, String typeFilesPath, String outputPath) throws IOException, KbException {
        NumeratedKb kb = new NumeratedKb(kbName);
        for (String tripe_file_name: TRIPLE_FILES) {
            BufferedReader reader = new BufferedReader(new FileReader(Paths.get(tripeFilesPath, tripe_file_name).toFile()));
            String line;
            while (null != (line = reader.readLine())) {
                String[] components = line.split("\t");
                kb.addRecord(components[1], new String[]{components[0], components[2]});
            }
            reader.close();
        }
//        Gson gson = new Gson();
//        Map<String, String[]> _map = new HashMap<>();
//        Map<String, String[]> type_map = gson.fromJson(new FileReader(Paths.get(typeFilesPath, TYPE_FILE).toFile()), _map.getClass());
//        for (Map.Entry<String, String[]> entry: type_map.entrySet()) {
//            String entity = entry.getKey();
//            if (0 != kb.name2Num(entity)) {
//                for (String type : entry.getValue()) {
//                    kb.addRecord(type, new String[]{entity});
//                }
//            }
//        }
        List<TypeInfo> type_infos = readTypes(Paths.get(typeFilesPath, TYPE_FILE));
        for (TypeInfo type_info: type_infos) {
            if (0 != kb.name2Num(type_info.entity)) {
                for (String type : type_info.types) {
                    kb.addRecord(type, new String[]{type_info.entity});
                }
            }
        }
        kb.dump(outputPath);

        /* Show statistics */
        System.out.printf("KB: %s\n", kbName);
        System.out.printf("- Total Entities: %d\n", kb.totalMappings());
        System.out.printf("- Total Relations: %d\n", kb.totalRelations());
        System.out.printf("- Total Records: %d\n", kb.totalRecords());
    }

    static class TypeInfo {
        String entity;
        List<String> types = new ArrayList<>();

        public TypeInfo(String entity) {
            this.entity = entity;
        }

        @Override
        public String toString() {
            return "TypeInfo{" +
                    "entity='" + entity + '\'' +
                    ", types=" + Arrays.toString(types.toArray(new String[0])) +
                    '}';
        }
    }

    static List<TypeInfo> readTypes(Path path) throws IOException {
        BufferedReader reader = Files.newBufferedReader(path, StandardCharsets.UTF_8);
        int i;
        boolean on_key = true;
        TypeInfo type_info = null;
        List<TypeInfo> type_infos = new ArrayList<>();
        StringBuilder builder = null;
        while (-1 != (i = reader.read())) {
            char c = (char) i;
            switch (c) {
                case '{':
                    // start
                    on_key = true;
                    break;
                case ']':
                    // end type list
                    on_key = true;
                    type_infos.add(type_info);
                    type_info = null;
                    break;
                case '}':
                    // end
                    break;
                case '"':
                    // begin/end a name
                    if (null == builder) {
                        builder = new StringBuilder();
                    } else {
                        String name = builder.toString();
                        builder = null;
                        if (on_key) {
                            type_info = new TypeInfo(name);
                        } else {
                            type_info.types.add(name);
                        }
                    }
                    break;
                case ':':
                    // switch from key to value
                    on_key = false;
                    break;
                case '[':
                    // begin type list
                    break;
                case ',':
                    // end types/pairs
                    break;
                default:
                    if (null != builder) {
                        builder.append(c);
                    }
            }
        }
        reader.close();
        return type_infos;
    }

}
