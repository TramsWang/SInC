package sinc2.kb;

import sinc2.util.kb.KbRelation;
import sinc2.util.kb.NumeratedKb;

import java.io.File;
import java.io.IOException;
import java.util.*;

/**
 * A simple in-memory KB. The values in the KB are converted to integers so each relation in the KB is a 2D table of
 * integers. It is the same as "NumeratedKb", except that the simple KB is read-only. Due to its simplicity, the memory
 * cost is much lower than NumeratedKb. The estimated size of the memory cost, at the worst case, is about 3 times the
 * size of the disk space taken by all relation files. For more information, please refer to "NumeratedKB".
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
    protected final Map<String, Integer> relationNameMap;
    /** The list of promising constants in the corresponding relations */
    protected int[][][] promisingConstants;
    protected Set<Integer> constants;

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
        constants = null;
    }

    /**
     * Create a KB from a list of relations (a relation here is a list of int arrays)
     *
     * @param kbName             The name of the KB
     * @param relations          The list of relations
     * @param relNames           The list of names of the corresponding relations
     * @param calculateConstants Whether all constants are enumerated during construction
     */
    public SimpleKb(String kbName, int[][][] relations, String[] relNames, boolean calculateConstants) {
        this.name = kbName;
        this.relations = new SimpleRelation[relations.length];
        this.relationNameMap = new HashMap<>();
        for (int i = 0; i < relations.length; i++) {
            this.relations[i] = new SimpleRelation(relNames[i], i, relations[i]);
            relationNameMap.put(relNames[i], i);
        }
        if (calculateConstants) {
            constants = new HashSet<>();
            for (SimpleRelation relation: this.relations) {
                relation.collectConstants(constants);
            }
        } else {
            constants = null;
        }
    }

    /**
     * Load a KB from local file system.
     *
     * @param name The name of the KB
     * @param basePath The base path to the dir of the KB
     * @param calculateConstants Whether all constants are enumerated during construction
     * @throws IOException
     */
    public SimpleKb(String name, String basePath, boolean calculateConstants) throws IOException {
        this.name = name;
        this.relationNameMap = new HashMap<>();
        this.relations = loadRelations(basePath);
        if (calculateConstants) {
            constants = new HashSet<>();
            for (SimpleRelation relation: relations) {
                relation.collectConstants(constants);
            }
        } else {
            constants = null;
        }
    }

    protected SimpleRelation[] loadRelations(String basePath) throws IOException {
        File kb_dir = NumeratedKb.getKbPath(name, basePath).toFile();
        String kb_dir_path = kb_dir.getAbsolutePath();
        File[] files = kb_dir.listFiles();
        List<SimpleRelation> relations = new ArrayList<>();
        if (null != files) {
            for (File f: files) {
                KbRelation.RelationInfo rel_info = KbRelation.parseRelFilePath(f.getName());
                if (null != rel_info) {
                    SimpleRelation relation = new SimpleRelation(
                            rel_info.name, relations.size(), rel_info.arity, rel_info.totalRecords, kb_dir_path
                    );
                    relations.add(relation);
                    relationNameMap.put(relation.name, relation.id);
                }
            }
        }
        return relations.toArray(new SimpleRelation[0]);
    }

    public SimpleRelation getRelation(String name) {
        Integer idx = relationNameMap.get(name);
        return (null == idx) ? null : relations[idx];
    }

    public SimpleRelation getRelation(int id) {
        return (id >=0 && id < relations.length) ? relations[id] : null;
    }

    public boolean hasRecord(String relationName, int[] record) {
        Integer idx = relationNameMap.get(relationName);
        return (null != idx) && relations[idx].hasRow(record);
    }

    public boolean hasRecord(int relationId, int[] record) {
        return relationId >= 0 && relationId < relations.length && relations[relationId].hasRow(record);
    }

    public void setAsEntailed(String relationName, int[] record) {
        Integer idx = relationNameMap.get(relationName);
        if (null != idx) {
            relations[idx].setAsEntailed(record);
        }
    }

    public void setAsEntailed(int relationId, int[] record) {
        relations[relationId].setAsEntailed(record);
    }

    public void setAsNotEntailed(String relationName, int[] record) {
        relations[relationNameMap.get(relationName)].setAsNotEntailed(record);
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

    public Set<Integer> allConstants() {
        if (null == constants) {
            constants = new HashSet<>();
            for (SimpleRelation relation: relations) {
                relation.collectConstants(constants);
            }
        }
        return constants;
    }

    public int totalConstants() {
        if (null == constants) {
            constants = new HashSet<>();
            for (SimpleRelation relation: relations) {
                relation.collectConstants(constants);
            }
        }
        return constants.size();
    }

    public int getRelationArity(int relId) {
        return relations[relId].totalCols();
    }
}
