package sinc2.exp.hint.predefined;

import sinc2.rule.Eval;
import sinc2.rule.EvalMetric;
import sinc2.util.ArrayOperation;
import sinc2.util.io.IntReader;
import sinc2.util.kb.NumeratedKb;
import sinc2.util.kb.NumerationMap;

import java.io.*;
import java.nio.file.Paths;
import java.util.*;

public class TypeRulesMiner {

    static class TypedEntities {
        protected static final int BITS_PER_INT = Integer.BYTES * 8;
        final int type;
        final int[] entities;
        final int[] entailmentFlags;

        public TypedEntities(int type, int[] entities) {
            this.type = type;
            this.entities = entities;
            entailmentFlags = new int[entities.length / BITS_PER_INT + ((0 == entities.length % BITS_PER_INT) ? 0 : 1)];
        }

        public void entailAll(int[] entities) {
            int idx = 0;
            for (int entity: entities) {
                idx = Arrays.binarySearch(entities, idx, entities.length, entity);
                entailmentFlags[idx / BITS_PER_INT] |= 0x1 << (idx % BITS_PER_INT);
                idx++;
            }
        }

        public int entailedEntities() {
            int cnt = 0;
            for (int i: entailmentFlags) {
                cnt += Integer.bitCount(i);
            }
            return cnt;
        }
    }

    public static final String LOGGER_FILE_NAME = "rules.log";
    public static final String SUB_TYPE_FILE_NAME = "Subtypes.tsv";
    public static final String TYPE_INFERENCE_FILE_NAME = "TypeInferences.tsv";
    public static final String RULE_FILE_TITLE_LINE = "rule\t|r|\tE+\tE-\tFC\tτ\tδ";

    public static String getOutputDir(String kbName) {
        return "TypingRules_" + kbName;
    }

    public static boolean isTypeRelation(String relName) {
        return "rdf:type".equals(relName) || "type".equals(relName);
    }

    public static void main(String[] args) throws IOException {
        if (2 != args.length) {
            System.err.println("Usage: <input path> <KB name>");
            return;
        }
        run(args[0], args[1]);
    }

    static void run(String inputPath, String kbName) throws IOException {
        /* Create dir */
        String output_dir_path = getOutputDir(kbName);
        File output_dir_file = new File(output_dir_path);
        if (!output_dir_file.exists() && !output_dir_file.mkdirs()) {
            throw new IOException("Output dir creation failed: " + output_dir_path);
        }

        /* Load mappings */
        long time_start = System.currentTimeMillis();
        PrintWriter logger = new PrintWriter(Paths.get(output_dir_path, LOGGER_FILE_NAME).toFile());
        String kb_dir_path = NumeratedKb.getKbPath(kbName, inputPath).toAbsolutePath().toString();
        logger.println("Loading mappings ...");
        NumerationMap num_map = new NumerationMap(kb_dir_path);
        long time_map_loaded = System.currentTimeMillis();
        logger.printf("Done (%d ms)\n", time_map_loaded - time_start);
        logger.flush();

        /* Load type & other normal relations */
        logger.println("Loading relations ...");
        BufferedReader reader = new BufferedReader(new FileReader(NumeratedKb.getRelInfoFilePath(kbName, inputPath).toFile()));
        String line;
        List<int[][]> relations = new ArrayList<>();
        List<String> relation_names = new ArrayList<>();
        Map<Integer, Set<Integer>> _typed_entities = new HashMap<>();
        int rel_id = 0;
        while (null != (line = reader.readLine())) {
            String[] components = line.split("\t"); // relation name, arity, total records
            int arity = Integer.parseInt(components[1]);
            int total_records = Integer.parseInt(components[2]);
            File rel_file = Paths.get(kb_dir_path, NumeratedKb.getRelDataFileName(rel_id)).toFile();
            if (rel_file.exists()) {
                /* Load from file */
                IntReader int_reader = new IntReader(rel_file);
                if (isTypeRelation(components[0])) {    // type relation
                    for (int i = 0; i < total_records; i++) {
                        int entity = int_reader.next();
                        int type = int_reader.next();
                        _typed_entities.compute(type, (t, set) -> {
                            if (null == set) {
                                set = new HashSet<>();
                            }
                            set.add(entity);
                            return set;
                        });
                    }
                } else {    // normal relations
                    Set<Integer>[] values = new Set[arity];
                    for (int arg_idx = 0; arg_idx < arity; arg_idx++) {
                        values[arg_idx] = new HashSet<>();
                    }
                    for (int i = 0; i < total_records; i++) {
                        for (int arg_idx = 0; arg_idx < arity; arg_idx++) {
                            values[arg_idx].add(int_reader.next());
                        }
                    }
                    int[][] relation = new int[arity][];
                    for (int arg_idx = 0; arg_idx < arity; arg_idx++) {
                        relation[arg_idx] = ArrayOperation.toArray(values[arg_idx]);
                        Arrays.sort(relation[arg_idx]);
                    }
                    relations.add(relation);
                    relation_names.add(components[0]);
                }
                int_reader.close();
            }
            rel_id++;
        }
        TypedEntities[] typed_entities = new TypedEntities[_typed_entities.size()];
        int type_idx = 0;
        for (Map.Entry<Integer, Set<Integer>> entry: _typed_entities.entrySet()) {
            int[] entities = ArrayOperation.toArray(entry.getValue());
            Arrays.sort(entities);
            typed_entities[type_idx] = new TypedEntities(entry.getKey(), entities);
            type_idx++;
        }
        Arrays.sort(typed_entities, (et1, et2) -> {
            int diff = et1.entities[0] - et2.entities[0];
            return (diff != 0) ? diff : (et1.entities[et1.entities.length-1] - et2.entities[et2.entities.length-1]);
        });
        long time_relation_loaded = System.currentTimeMillis();
        logger.printf("Done (%d s)\n", (time_relation_loaded - time_map_loaded) / 1000);
        logger.flush();

        /* Compare sub-types */
        logger.println("Comparing subtypes ...");
        PrintWriter sub_type_writer = new PrintWriter(Paths.get(output_dir_path, SUB_TYPE_FILE_NAME).toFile());
        sub_type_writer.println(RULE_FILE_TITLE_LINE);
        for (int type_idx1 = 0; type_idx1 < typed_entities.length; type_idx1++) {
            TypedEntities typed_entities1 = typed_entities[type_idx1];
            for (int type_idx2 = type_idx1+1; type_idx2 < typed_entities.length; type_idx2++) {
                TypedEntities typed_entities2 = typed_entities[type_idx2];
                if (typed_entities1.entities[typed_entities1.entities.length-1] < typed_entities2.entities[0]) {
                    break;
                }
                int[] intersection = intersection(typed_entities1.entities, typed_entities2.entities);
                if (intersection.length + intersection.length > typed_entities1.entities.length) {
                    /* type2 <- type1 */
                    Eval eval = new Eval(intersection.length, typed_entities1.entities.length, 1);
                    sub_type_writer.printf(
                            "%s(X0):-%s(X0)\t%d\t%d\t%d\t%.2f\t%.2f\t%d\n",
                            num_map.num2Name(typed_entities2.type), num_map.num2Name(typed_entities1.type),
                            1, (int) eval.getPosEtls(), (int) eval.getNegEtls(),
                            eval.getPosEtls() / typed_entities2.entities.length, eval.value(EvalMetric.CompressionRatio),
                            (int) eval.value(EvalMetric.CompressionCapacity)
                    );
                    typed_entities2.entailAll(intersection);
                }
                if (intersection.length + intersection.length > typed_entities2.entities.length) {
                    /* type1 <- type2 */
                    Eval eval = new Eval(intersection.length, typed_entities2.entities.length, 1);
                    sub_type_writer.printf(
                            "%s(X0):-%s(X0)\t%d\t%d\t%d\t%.2f\t%.2f\t%d\n",
                            num_map.num2Name(typed_entities1.type), num_map.num2Name(typed_entities2.type),
                            1, (int) eval.getPosEtls(), (int) eval.getNegEtls(),
                            eval.getPosEtls() / typed_entities1.entities.length, eval.value(EvalMetric.CompressionRatio),
                            (int) eval.value(EvalMetric.CompressionCapacity)
                    );
                    typed_entities1.entailAll(intersection);
                }
            }
        }
        sub_type_writer.close();
        long time_sub_types_found = System.currentTimeMillis();
        logger.printf("Done (%d s)\n", (time_sub_types_found - time_relation_loaded) / 1000);
        logger.flush();

        /* Find type inferences */
        logger.println("Finding type inferences ...");
        PrintWriter type_inference_writer = new PrintWriter(Paths.get(output_dir_path, TYPE_INFERENCE_FILE_NAME).toFile());
        type_inference_writer.println(RULE_FILE_TITLE_LINE);
        for (int rel_idx = 0; rel_idx < relations.size(); rel_idx++) {
            int[][] relation = relations.get(rel_idx);
            for (int arg_idx = 0; arg_idx < relation.length; arg_idx++) {
                int[] arg_values = relation[arg_idx];
                for (type_idx = 0; type_idx < typed_entities.length; type_idx++) {
                    TypedEntities type = typed_entities[type_idx];
                    if (arg_values[0] > type.entities[type.entities.length-1]) {
                        continue;
                    }
                    if (arg_values[arg_values.length-1] < type.entities[0]) {
                        break;
                    }
                    int[] intersection = intersection(arg_values, type.entities);
                    if (intersection.length + intersection.length > arg_values.length) {
                        /* type(X) <- relation(..., X, ...) */
                        Eval eval = new Eval(intersection.length, arg_values.length, 1);
                        type_inference_writer.printf("%s(X0):-%s(", num_map.num2Name(type.type), relation_names.get(rel_idx));
                        if (0 == arg_idx) {
                            type_inference_writer.print("X0,?)");
                        } else {
                            type_inference_writer.print("?,X0)");
                        }
                        type_inference_writer.printf("\t%d\t%d\t%d\t%.2f\t%.2f\t%d\n",
                                1, (int) eval.getPosEtls(), (int) eval.getNegEtls(),
                                eval.getPosEtls() / type.entities.length, eval.value(EvalMetric.CompressionRatio),
                                (int) eval.value(EvalMetric.CompressionCapacity)
                        );
                        type.entailAll(intersection);
                    }
                }
            }
        }
        type_inference_writer.close();
        long time_type_inference_found = System.currentTimeMillis();
        logger.printf("Done (%d s)\n", (time_type_inference_found - time_sub_types_found) / 1000);
        logger.flush();

        /* Calculate total coverage of types */
        int total_covered = 0;
        int total = 0;
        for (TypedEntities type: typed_entities) {
            int covered = type.entailedEntities();
            logger.printf(
                    "Type Coverage of '%s': %.2f%% (%d/%d)\n", num_map.num2Name(type.type),
                    covered * 100.0 / type.entities.length, covered, type.entities.length
            );
            total += type.entities.length;
            total_covered += covered;
        }
        logger.printf("Total Coverage: %.2f%% (%d/%d)\n", total_covered * 100.0 / total, total_covered, total);
        logger.printf("Total Time: %d s\n", (System.currentTimeMillis() - time_start) / 1000);
        logger.flush();
        logger.close();
    }

    static int[] intersection(int[] a, int[] b) {
        List<Integer> results = new ArrayList<>();
        int idxa = 0;
        int idxb = 0;
        while (idxa < a.length && idxb < b.length) {
            int inta = a[idxa];
            int intb = b[idxb];
            if (inta < intb) {
                idxa = Arrays.binarySearch(a, idxa + 1, a.length, intb);
                idxa = (0 > idxa) ? (-idxa-1) : idxa;
            } else if (inta > intb) {
                idxb = Arrays.binarySearch(b, idxb + 1, b.length, inta);
                idxb = (0 > idxb) ? (-idxb-1) : idxb;
            } else {
                results.add(inta);
                idxa++;
                idxb++;
            }
        }
        return ArrayOperation.toArray(results);
    }
}
