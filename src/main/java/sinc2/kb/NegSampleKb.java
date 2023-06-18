package sinc2.kb;

import sinc2.common.Record;
import sinc2.util.io.IntReader;
import sinc2.util.io.IntWriter;
import sinc2.util.kb.NumeratedKb;

import java.io.File;
import java.io.IOException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.HashMap;
import java.util.Map;

/**
 * This class maintains negative samples from local file system. The storage protocol for negative samples of a KB is
 * similar to that of the "NumeratedKb". Files for a complete negative sampling are dumped to a directory named by the
 * name of the negative KB and are listed as follows:
 *   - A relation information file "Relations.dat": This binary file is a list of integers, defined as follows:
 *     > The first integer denotes the total number of relations in this negative sample KB
 *     > For each relation ID from 0 to n-1, there are two integers denoting the arity and the total number of samples.
 *       Each of the following pairs are of the ascending order of the ID's.
 *     > Example (as a list of integers): [n, arity_0, num_rec_0, arity_1, num_rec_1, ... arity_{n-1}, num_rec_{n-1}]
 *   - The relations of negative samples: The names of the relation files are "<ID>.neg". The file format is the same as
 *     the dump of "KBRelation".
 *   - The files of sample weights: The names of the weight files are "<ID>.wt". The file is in binary format and is a
 *     list of floats. The number of floats is the number of negative samples in the corresponding negative relation.
 *     The i-th float is the weight of the i-th negative sample.
 *
 * @since 2.3
 * @see sinc2.util.kb.NumeratedKb
 * @see sinc2.util.kb.KbRelation
 */
public class NegSampleKb {
    /** The name of the relation information file */
    public static final String REL_INFO_FILE_NAME = "Relations.dat";

    /** Return the name of the negative relation file for the "relId"-th relation */
    public static String getNegRelFileName(int relId) {
        return String.format("%d.neg", relId);
    }

    /** Return the name of the sample weight file for the "relId"-th relation */
    public static String getWeightFileName(int relId) {
        return String.format("%d.wt", relId);
    }

    /** The name of the negative sample KB */
    protected final String name;
    /** The list of negative relations */
    protected final IntTable[] negSampleTabs;
    /** The list of negative sample weight */
    protected final Map<Record, Float>[] negSampleWeightMaps;

    /**
     * Construct a negative sample KB from in-memory data
     * @param name                The name of the negative sample KB
     * @param negSampleTabs       The list of negative relations
     * @param negSampleWeightMaps The list of negative sample weight
     */
    public NegSampleKb(String name, IntTable[] negSampleTabs, Map<Record, Float>[] negSampleWeightMaps) {
        this.name = name;
        this.negSampleTabs = negSampleTabs;
        this.negSampleWeightMaps = negSampleWeightMaps;
    }

    /**
     * Load negative samples KB from local file system.
     *
     * @param name     The name of the negative sample KB
     * @param basePath The path to where the KB is located
     */
    public NegSampleKb(String name, String basePath) throws IOException {
        /* Load relation information */
        this.name = name;
        String dir_path = NumeratedKb.getKbPath(name, basePath).toString();
        File rel_info_file = Paths.get(dir_path, REL_INFO_FILE_NAME).toFile();
        IntReader rel_info_reader = new IntReader(rel_info_file);
        int total_relations = rel_info_reader.next();

        negSampleTabs = new IntTable[total_relations];
        negSampleWeightMaps = new Map[total_relations];
        for (int rel_id = 0; rel_id < total_relations; rel_id++) {
            /* Load negative samples and weight */
            int arity = rel_info_reader.next();
            int total_records = rel_info_reader.next();
            int[][] neg_samples = SimpleRelation.loadFile(
                    Paths.get(dir_path, getNegRelFileName(rel_id)).toFile(), arity, total_records
            );
            Map<Record, Float> weight_map = new HashMap<>();
            IntReader weight_reader = new IntReader(Paths.get(dir_path, getWeightFileName(rel_id)).toFile());
            for (int[] neg_sample: neg_samples) {
                weight_map.put(new Record(neg_sample), Float.intBitsToFloat(weight_reader.next()));
            }
            weight_reader.close();
            negSampleWeightMaps[rel_id] = weight_map;
            negSampleTabs[rel_id] = new IntTable(neg_samples);
        }

        rel_info_reader.close();
    }

    /**
     * Dump the negative sample KB to local file system.
     *
     * @param basePath The path to where the KB will be dumped
     */
    public void dump(String basePath) throws IOException {
        /* Check & create dir */
        Path kb_dir_path = NumeratedKb.getKbPath(name, basePath);
        File kb_dir_file = kb_dir_path.toFile();
        if (!kb_dir_file.exists() && !kb_dir_file.mkdirs()) {
            throw new IOException("KB directory creation failed: " + kb_dir_file.getAbsolutePath());
        }

        /* Dump relation info file */
        String kb_dir_str = kb_dir_path.toString();
        IntWriter rel_info_writer = new IntWriter(Paths.get(kb_dir_str, REL_INFO_FILE_NAME).toFile());
        rel_info_writer.write(negSampleTabs.length);
        for (IntTable neg_table: negSampleTabs) {
            rel_info_writer.write(neg_table.totalCols());
            rel_info_writer.write(neg_table.totalRows());
        }
        rel_info_writer.close();

        /* Dump negative samples and weight */
        for (int rel_id = 0; rel_id < negSampleTabs.length; rel_id++) {
            IntTable neg_table = negSampleTabs[rel_id];
            Map<Record, Float> weight_map = negSampleWeightMaps[rel_id];
            IntWriter neg_writer = new IntWriter(Paths.get(kb_dir_str, getNegRelFileName(rel_id)).toFile());
            IntWriter weight_writer = new IntWriter(Paths.get(kb_dir_str, getWeightFileName(rel_id)).toFile());
            for (int[] neg_record: neg_table) {
                for (int arg: neg_record) {
                    neg_writer.write(arg);
                }
                weight_writer.write(Float.floatToRawIntBits(weight_map.get(new Record(neg_record))));
            }
            neg_writer.close();
            weight_writer.close();
        }
    }
}
