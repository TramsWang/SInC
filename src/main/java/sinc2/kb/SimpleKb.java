package sinc2.kb;

import sinc2.util.kb.NumeratedKb;
import sinc2.util.kb.NumerationMap;

import java.io.*;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A simple in-memory KB. The values in the KB are converted to integers so each relation in the KB is a 2D table of
 * integers. It is the same as "NumeratedKb", except that the simple KB is read-only. Due to its simplicity, the memory
 * cost is much lower than NumeratedKb. The estimated size of the memory cost, at the worst case, is about 3 times the
 * size of the disk space taken by all relation files. For more information, please refer to "NumeratedKB".
 *
 * Given that the constant numerations are in a continuous integer span [1, n], only one integer, n, denoting the total
 * number of constants are needed to refer to all constants.
 *
 * @see NumeratedKb
 * @since 2.1
 */
public class SimpleKb {

    /** The name of the KB */
    protected final String name;
    /** The list of relations. The ID of each relation is its index in the list */
    protected final SimpleRelation[] relations;
    /** The map from relation names to IDs */
    protected final Map<String, SimpleRelation> relationNameMap;
    /** The list of promising constants in the corresponding relations */
    protected int[][][] promisingConstants;
    /** The total number of constants in the KB */
    protected final int totalConstants;

    /**
     * Load a KB from local file system.
     *
     * @param name The name of the KB
     * @param basePath The base path to the dir of the KB
     * @throws IOException
     */
    public SimpleKb(String name, String basePath) throws IOException {
        this.name = name;
        this.relationNameMap = new HashMap<>();
        this.relations = loadRelations(basePath);
        int max_value = 0;
        for (SimpleRelation relation: relations) {
            max_value = Math.max(max_value, relation.maxValue());
        }
        totalConstants = max_value;
    }

    /**
     * Create a KB from a list of relations (a relation here is a list of int arrays).
     *
     * NOTE: Suppose the integers in the relations lie in the integer span: [1, n], each integer "i" in [1, n] should
     * appear in at least one argument. Otherwise, the number of constants will NOT be correct.
     *
     * @param kbName             The name of the KB
     * @param relations          The list of relations
     * @param relNames           The list of names of the corresponding relations
     */
    public SimpleKb(String kbName, int[][][] relations, String[] relNames) {
        this.name = kbName;
        this.relations = new SimpleRelation[relations.length];
        this.relationNameMap = new HashMap<>();
        int max_value = 0;
        for (int i = 0; i < relations.length; i++) {
            this.relations[i] = new SimpleRelation(relNames[i], i, relations[i]);
            relationNameMap.put(relNames[i], this.relations[i]);
            max_value = Math.max(max_value, this.relations[i].maxValue());
        }
        totalConstants = max_value;
    }

    public SimpleKb(SimpleKb another) {
        this.name = another.name;
        this.relations = another.relations;
        this.relationNameMap = another.relationNameMap;
        this.totalConstants = another.totalConstants;
        this.promisingConstants = another.promisingConstants;
    }

    protected SimpleRelation[] loadRelations(String basePath) throws IOException {
        BufferedReader reader = new BufferedReader(new FileReader(NumeratedKb.getRelInfoFilePath(name, basePath).toFile()));
        String line;
        int line_num = 0;
        String kb_dir_path = NumeratedKb.getKbPath(name, basePath).toString();
        List<SimpleRelation> relations = new ArrayList<>();
        while (null != (line = reader.readLine())) {
            String[] components = line.split("\t"); // relation name, arity, total records
            File rel_file = Paths.get(kb_dir_path, NumeratedKb.getRelDataFileName(line_num)).toFile();
            line_num++;
            if (rel_file.exists()) {    // If no file found, the relation is empty, and thus should not be included in the SimpleKb
                /* Load from file */
                int rel_id = relations.size();
                SimpleRelation relation = new SimpleRelation(
                        components[0], rel_id, Integer.parseInt(components[1]), Integer.parseInt(components[2]),
                        rel_file.getName(), kb_dir_path
                );
                relations.add(relation);
                relationNameMap.put(relation.name, relation);
            }
        }
        reader.close();
        return relations.toArray(new SimpleRelation[0]);
    }

    /**
     * @param mappedNames   The length of this array is `c + 1` where `c` is the number of constants in the KB. The names
     *                      in this array starts from `mappedNames[1]`.
     */
    public void dump(String basePath, String[] mappedNames) throws IOException {
        /* Check & create dir */
        Path kb_dir = NumeratedKb.getKbPath(name, basePath);
        File kb_dir_file = kb_dir.toFile();
        if (!kb_dir_file.exists() && !kb_dir_file.mkdirs()) {
            throw new IOException("KB directory creation failed: " + kb_dir_file.getAbsolutePath());
        }

        /* Dump mappings */
        String kb_dir_path = kb_dir_file.getAbsolutePath();
        int map_num = NumerationMap.MAP_FILE_NUMERATION_START;
        PrintWriter writer = new PrintWriter(NumerationMap.getMapFilePath(kb_dir_path, map_num).toFile());
        int records_cnt = 0;
        for (int i = 1; i < mappedNames.length; i++) {
            String name = mappedNames[i];
            if (NumerationMap.MAX_MAP_ENTRIES <= records_cnt) {
                writer.close();
                map_num++;
                records_cnt = 0;
                writer = new PrintWriter(NumerationMap.getMapFilePath(kb_dir_path, map_num).toFile());
            }
            writer.println((null == name) ? "" : name);
            records_cnt++;
        }
        writer.close();

        /* Dump relations */
        writer = new PrintWriter(NumeratedKb.getRelInfoFilePath(name, basePath).toFile());
        for (int rel_id = 0; rel_id < relations.length; rel_id++) {
            SimpleRelation relation = relations[rel_id];
            writer.printf("%s\t%d\t%d\n", relation.name, relation.totalCols(), relation.totalRows());
            if (0 < relation.totalRows()) {
                /* Dump only non-empty relations */
                relation.dump(kb_dir_path, NumeratedKb.getRelDataFileName(rel_id));
            }
        }
        writer.close();
    }

    public SimpleRelation getRelation(String name) {
        return relationNameMap.get(name);
    }

    public SimpleRelation getRelation(int id) {
        return (id >=0 && id < relations.length) ? relations[id] : null;
    }

    public boolean hasRecord(String relationName, int[] record) {
        SimpleRelation relation = relationNameMap.get(relationName);
        return (null != relation) && relation.hasRow(record);
    }

    public boolean hasRecord(int relationId, int[] record) {
        return relationId >= 0 && relationId < relations.length && relations[relationId].hasRow(record);
    }

    public void setAsEntailed(String relationName, int[] record) {
        relationNameMap.get(relationName).setAsEntailed(record);
    }

    public void setAsEntailed(int relationId, int[] record) {
        relations[relationId].setAsEntailed(record);
    }

    public void setAsNotEntailed(String relationName, int[] record) {
        relationNameMap.get(relationName).setAsNotEntailed(record);
    }

    public void setAsNotEntailed(int relationId, int[] record) {
        relations[relationId].setAsNotEntailed(record);
    }

    public void updatePromisingConstants() {
        promisingConstants = new int[relations.length][][];
        for (int i = 0; i < relations.length; i++) {
            promisingConstants[i] = relations[i].getPromisingConstants();
        }
    }

    public int[][] getPromisingConstants(int relId) {
        return promisingConstants[relId];
    }

    public String getName() {
        return name;
    }

    public SimpleRelation[] getRelations() {
        return relations.clone();
    }

    public int totalRelations() {
        return relations.length;
    }

    public int totalRecords() {
        int cnt = 0;
        for (SimpleRelation relation: relations) {
            cnt += relation.totalRows();
        }
        return cnt;
    }

    public int totalConstants() {
        return totalConstants;
    }
}
