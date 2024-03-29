package sinc2.util.kb;

import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import sinc2.util.Pair;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.nio.file.Paths;
import java.util.*;

import static org.junit.jupiter.api.Assertions.*;

class NumerationMapTest {


    static TestKbManager testKbManager;

    @BeforeAll
    static void setupKb() throws IOException {
        testKbManager = new TestKbManager();
    }

    @AfterAll
    static void removeKb() {
        testKbManager.cleanUpKb();
    }

    @Test
    void testGetMapFilePath() {
        String kb_path = "/root/home/dir";
        int num = 201;
        assertEquals("/root/home/dir/map201.tsv", NumerationMap.getMapFilePath(kb_path, num).toFile().getAbsolutePath());
        assertEquals("/root/home/dir/map201.tsv", NumerationMap.getMapFilePath(kb_path, num).toString());

        kb_path = "/root/home/dir/";
        assertEquals("/root/home/dir/map201.tsv", NumerationMap.getMapFilePath(kb_path, num).toFile().getAbsolutePath());
        assertEquals("/root/home/dir/map201.tsv", NumerationMap.getMapFilePath(kb_path, num).toString());

        kb_path = "./dir";
        assertEquals("./dir/map201.tsv", NumerationMap.getMapFilePath(kb_path, num).toString());

        kb_path = "dir";
        assertEquals("dir/map201.tsv", NumerationMap.getMapFilePath(kb_path, num).toString());
    }

    @Test
    void testCreateEmpty() {
        NumerationMap num_map = new NumerationMap();

        assertEquals(0, num_map.totalMappings());
        assertEquals(0, num_map.numMap.size());
        assertEquals(1, num_map.numArray.size());
        assertEquals(0, num_map.freeNums.size());

        assertEquals(NumerationMap.NUM_NULL, num_map.unmapName("a"));
        assertNull(num_map.unmapNumeration(0));
        assertNull(num_map.unmapNumeration(1));

        assertEquals(1, num_map.mapName("a"));
        assertEquals(2, num_map.mapName("b"));
        assertEquals(3, num_map.mapName("c"));

        assertEquals(3, num_map.totalMappings());
        assertEquals(3, num_map.numMap.size());
        assertEquals(4, num_map.numArray.size());
        assertEquals(0, num_map.freeNums.size());
    }

    @Test
    void testRead() {
        NumerationMap map = new NumerationMap(testKbManager.getKbPath());

        assertEquals(14, map.totalMappings());
        assertEquals(14, map.numMap.size());
        assertEquals(18, map.numArray.size());
        assertEquals(3, map.freeNums.size());

        assertNull(map.num2Name(0x1));
        assertNull(map.num2Name(0x2));
        assertNull(map.num2Name(0x3));
        assertEquals("alice", map.num2Name(0x4));
        assertEquals("bob", map.num2Name(0x5));
        assertEquals("catherine", map.num2Name(0x6));
        assertEquals("diana", map.num2Name(0x7));
        assertEquals("erick", map.num2Name(0x8));
        assertEquals("frederick", map.num2Name(0x9));
        assertEquals("gabby", map.num2Name(0xa));
        assertEquals("harry", map.num2Name(0xb));
        assertEquals("isaac", map.num2Name(0xc));
        assertEquals("jena", map.num2Name(0xd));
        assertEquals("kyle", map.num2Name(0xe));
        assertEquals("lily", map.num2Name(0xf));
        assertEquals("marvin", map.num2Name(0x10));
        assertEquals("nataly", map.num2Name(0x11));

        assertEquals(NumerationMap.NUM_NULL, map.name2Num("family"));
        assertEquals(NumerationMap.NUM_NULL, map.name2Num("mother"));
        assertEquals(NumerationMap.NUM_NULL, map.name2Num("father"));
        assertEquals(0x4, map.name2Num("alice"));
        assertEquals(0x5, map.name2Num("bob"));
        assertEquals(0x6, map.name2Num("catherine"));
        assertEquals(0x7, map.name2Num("diana"));
        assertEquals(0x8, map.name2Num("erick"));
        assertEquals(0x9, map.name2Num("frederick"));
        assertEquals(0xa, map.name2Num("gabby"));
        assertEquals(0xb, map.name2Num("harry"));
        assertEquals(0xc, map.name2Num("isaac"));
        assertEquals(0xd, map.name2Num("jena"));
        assertEquals(0xe, map.name2Num("kyle"));
        assertEquals(0xf, map.name2Num("lily"));
        assertEquals(0x10, map.name2Num("marvin"));
        assertEquals(0x11, map.name2Num("nataly"));
    }

    @Test
    void testCreateFromMap() {
        Map<String, Integer> existing_mapping = new HashMap<>();
        existing_mapping.put("a", 1);
        existing_mapping.put("b", 2);
        existing_mapping.put("d", 4);
        existing_mapping.put("g", 7);
        existing_mapping.put("z", 26);
        NumerationMap map = new NumerationMap(existing_mapping);

        assertEquals(5, map.totalMappings());
        assertEquals(5, map.numMap.size());
        assertEquals(27, map.numArray.size());
        assertEquals(21, map.freeNums.size());

        assertEquals(1, map.name2Num("a"));
        assertEquals(2, map.name2Num("b"));
        assertEquals(4, map.name2Num("d"));
        assertEquals(7, map.name2Num("g"));
        assertEquals(26, map.name2Num("z"));

        assertEquals("a", map.num2Name(1));
        assertEquals("b", map.num2Name(2));
        assertEquals("d", map.num2Name(4));
        assertEquals("g", map.num2Name(7));
        assertEquals("z", map.num2Name(26));
    }

    @Test
    void testWrite1() throws IOException {
        NumerationMap map = new NumerationMap(testKbManager.getKbPath());
        String tmp_dir_path = testKbManager.createTmpDir();
        map.dump(tmp_dir_path, 2);
        NumerationMap map2 = new NumerationMap(tmp_dir_path);

        assertEquals(map.totalMappings(), map2.totalMappings());
        assertEquals(map.numMap, map2.numMap);
        assertEquals(map.numArray, map2.numArray);

        Set<String> map_files = new HashSet<>(List.of(
                "map1.tsv", "map2.tsv", "map3.tsv", "map4.tsv", "map5.tsv", "map6.tsv", "map7.tsv", "map8.tsv", "map9.tsv"
        ));
        assertEquals(map_files, new HashSet<>(Arrays.asList(Objects.requireNonNull(new File(tmp_dir_path).list()))));
        for (int i = 1; i < 9; i++) {
            BufferedReader reader = new BufferedReader(new FileReader(
                    Paths.get(tmp_dir_path, String.format("map%d.tsv", i)).toFile()
            ));
            int lines = 0;
            while (null != reader.readLine()) {
                lines++;
            }
            reader.close();
            assertEquals(2, lines);
        }
        BufferedReader reader = new BufferedReader(new FileReader(Paths.get(tmp_dir_path, "map9.tsv").toFile()));
        int lines = 0;
        while (null != reader.readLine()) {
            lines++;
        }
        reader.close();
        assertEquals(1, lines);
    }

    @Test
    void testWrite2() throws IOException {
        NumerationMap map = new NumerationMap(testKbManager.getKbPath());
        map.dump(testKbManager.getKbPath(), "testmap.tsv");
        NumerationMap map2 = new NumerationMap(testKbManager.getKbPath(), "testmap.tsv");

        assertEquals(map.totalMappings(), map2.totalMappings());
        assertEquals(map.numMap, map2.numMap);
        assertEquals(map.numArray, map2.numArray);
        for (Map.Entry<String, Integer> entry: map.numMap.entrySet()) {
            assertEquals(entry.getValue(), map2.name2Num(entry.getKey()));
            assertEquals(entry.getKey(), map2.num2Name(entry.getValue()));
        }
        for (Map.Entry<String, Integer> entry: map2.numMap.entrySet()) {
            assertEquals(entry.getValue(), map.name2Num(entry.getKey()));
            assertEquals(entry.getKey(), map.num2Name(entry.getValue()));
        }
    }

    @Test
    void testMappingName() {
        NumerationMap map = new NumerationMap(testKbManager.getKbPath());

        assertEquals(1, map.mapName("family"));
        assertEquals(4, map.mapName("alice"));
        assertEquals(17, map.mapName("nataly"));

        assertEquals(2, map.mapName("a"));
        assertEquals(3, map.mapName("b"));
        assertEquals(18, map.mapName("c"));
        assertEquals(19, map.mapName("d"));
        assertEquals(20, map.mapName("e"));

        assertEquals(20, map.totalMappings());
        assertEquals(20, map.numMap.size());
        assertEquals(21, map.numArray.size());
        assertEquals(0, map.freeNums.size());
    }

    @Test
    void testUnmappingName() {
        NumerationMap map = new NumerationMap(testKbManager.getKbPath());

        assertEquals(NumerationMap.NUM_NULL, map.unmapName("a"));
        assertEquals(NumerationMap.NUM_NULL, map.unmapName("family"));
        assertEquals(4, map.unmapName("alice"));
        assertEquals(17, map.unmapName("nataly"));
        assertEquals(NumerationMap.NUM_NULL, map.unmapName("family"));
        assertEquals(NumerationMap.NUM_NULL, map.unmapName("alice"));
        assertEquals(NumerationMap.NUM_NULL, map.unmapName("nataly"));

        assertEquals(12, map.totalMappings());
        assertEquals(12, map.numMap.size());
        assertEquals(18, map.numArray.size());
        assertEquals(5, map.freeNums.size());
    }

    @Test
    void testUnmappingNum() {
        NumerationMap map = new NumerationMap(testKbManager.getKbPath());

        assertNull(map.unmapNumeration(0));
        assertNull(map.unmapNumeration(18));
        assertNull(map.unmapNumeration(1));
        assertEquals("alice", map.unmapNumeration(4));
        assertEquals("nataly", map.unmapNumeration(17));
        assertNull(map.unmapNumeration(1));
        assertNull(map.unmapNumeration(4));
        assertNull(map.unmapNumeration(17));

        assertEquals(12, map.totalMappings());
        assertEquals(12, map.numMap.size());
        assertEquals(18, map.numArray.size());
        assertEquals(5, map.freeNums.size());
    }

    @Test
    void testMappingWithUnmapping() {
        NumerationMap map = new NumerationMap(testKbManager.getKbPath());

        assertEquals(NumerationMap.NUM_NULL, map.unmapName("family"));
        assertEquals(4, map.unmapName("alice"));
        assertEquals(17, map.unmapName("nataly"));
        assertEquals(1, map.mapName("FAMILY"));
        assertEquals(2, map.mapName("ALICE"));
        assertEquals(3, map.mapName("NATALY"));

        assertEquals(4, map.mapName("a"));
        assertEquals(17, map.mapName("b"));
        assertEquals(18, map.mapName("c"));
        assertEquals("a", map.unmapNumeration(4));
        assertEquals("b", map.unmapNumeration(17));
        assertEquals("c", map.unmapNumeration(18));

        assertEquals(15, map.totalMappings());
        assertEquals(15, map.numMap.size());
        assertEquals(19, map.numArray.size());
        assertEquals(3, map.freeNums.size());
    }

    @Test
    void testIteration() {
        NumerationMap map = new NumerationMap(testKbManager.getKbPath());

        Set<Pair<String, Integer>> expected_str2int_entry_set = new HashSet<>(List.of(
                new Pair<>("alice", 0x4),
                new Pair<>("bob", 0x5),
                new Pair<>("catherine", 0x6),
                new Pair<>("diana", 0x7),
                new Pair<>("erick", 0x8),
                new Pair<>("frederick", 0x9),
                new Pair<>("gabby", 0xa),
                new Pair<>("harry", 0xb),
                new Pair<>("isaac", 0xc),
                new Pair<>("jena", 0xd),
                new Pair<>("kyle", 0xe),
                new Pair<>("lily", 0xf),
                new Pair<>("marvin", 0x10),
                new Pair<>("nataly", 0x11)
        ));
        Set<Pair<Integer, String>> expected_int2str_pair_set = new HashSet<>(List.of(
                new Pair<>(0x4, "alice"),
                new Pair<>(0x5, "bob"),
                new Pair<>(0x6, "catherine"),
                new Pair<>(0x7, "diana"),
                new Pair<>(0x8, "erick"),
                new Pair<>(0x9, "frederick"),
                new Pair<>(0xa, "gabby"),
                new Pair<>(0xb, "harry"),
                new Pair<>(0xc, "isaac"),
                new Pair<>(0xd, "jena"),
                new Pair<>(0xe, "kyle"),
                new Pair<>(0xf, "lily"),
                new Pair<>(0x10, "marvin"),
                new Pair<>(0x11, "nataly")
        ));

        Set<Pair<String, Integer>> str2int_entry_set = new HashSet<>();
        Iterator<Map.Entry<String, Integer>> str2int_itr = map.iterName2Num();
        while (str2int_itr.hasNext()) {
            Map.Entry<String, Integer> entry = str2int_itr.next();
            str2int_entry_set.add(new Pair<>(entry.getKey(), entry.getValue()));
        }
        assertEquals(expected_str2int_entry_set, str2int_entry_set);

        Set<Pair<Integer, String>> int2str_pair_set = new HashSet<>();
        Iterator<Pair<Integer, String>> int2str_itr = map.iterNum2Name();
        while (int2str_itr.hasNext()) {
            int2str_pair_set.add(int2str_itr.next());
        }
        assertEquals(expected_int2str_pair_set, int2str_pair_set);
    }

    @Test
    void testNoName() {
        NumerationMap map = new NumerationMap();
        assertEquals(NumerationMap.NUM_NULL, map.name2Num("name"));
        assertNull(map.num2Name(1));
    }

    @Test
    void testReplaceEmpty() {
        Map<String, Integer> existing_mapping = new HashMap<>();
        existing_mapping.put("a", 1);
        existing_mapping.put("b", 2);
        existing_mapping.put("d", 4);
        existing_mapping.put("g", 7);
        existing_mapping.put("z", 26);
        existing_mapping.put("last", 27);
        NumerationMap map = new NumerationMap(existing_mapping);
        assertEquals(27, map.unmapName("last"));

        Map<Integer, Integer> expected_remap = new HashMap<>();
        expected_remap.put(26, 3);
        expected_remap.put(7, 5);
        List<String> expected_num_array = new ArrayList<>();
        expected_num_array.add(null);
        expected_num_array.add("a");
        expected_num_array.add("b");
        expected_num_array.add("z");
        expected_num_array.add("d");
        expected_num_array.add("g");
        Map<String, Integer> expected_num_map = new HashMap<>();
        expected_num_map.put("a", 1);
        expected_num_map.put("b", 2);
        expected_num_map.put("d", 4);
        expected_num_map.put("g", 5);
        expected_num_map.put("z", 3);

        assertEquals(expected_remap, map.replaceEmpty());
        assertEquals(expected_num_array, map.numArray);
        assertEquals(expected_num_map, map.numMap);
        assertTrue(map.freeNums.isEmpty());
    }

    @Test
    void testRearrange1() {
        NumerationMap map = new NumerationMap();
        assertEquals(1, map.mapName("a"));
        assertEquals(2, map.mapName("b"));
        assertEquals(3, map.mapName("c"));
        assertEquals(4, map.mapName("d"));
        assertEquals(5, map.mapName("e"));
        map.rearrange(new int[]{0, 5, 4, 3, 2, 1});
        assertEquals(5, map.name2Num("a"));
        assertEquals(4, map.name2Num("b"));
        assertEquals(3, map.name2Num("c"));
        assertEquals(2, map.name2Num("d"));
        assertEquals(1, map.name2Num("e"));
        assertEquals("a", map.num2Name(5));
        assertEquals("b", map.num2Name(4));
        assertEquals("c", map.num2Name(3));
        assertEquals("d", map.num2Name(2));
        assertEquals("e", map.num2Name(1));
    }

    @Test
    void testRearrange2() {
        Map<String, Integer> existing_mapping = new HashMap<>();
        existing_mapping.put("a", 1);
        existing_mapping.put("b", 2);
        existing_mapping.put("d", 4);
        existing_mapping.put("g", 7);
        existing_mapping.put("z", 26);
        NumerationMap map = new NumerationMap(existing_mapping);
        int[] old_2_new = new int[27];
        for (int old = 1; old < 27; old++) {
            old_2_new[old] = 27-old;
        }
        map.rearrange(old_2_new);
        assertEquals(26, map.name2Num("a"));
        assertEquals(25, map.name2Num("b"));
        assertEquals(23, map.name2Num("d"));
        assertEquals(20, map.name2Num("g"));
        assertEquals(1, map.name2Num("z"));
        assertEquals("a", map.num2Name(26));
        assertEquals("b", map.num2Name(25));
        assertEquals("d", map.num2Name(23));
        assertEquals("g", map.num2Name(20));
        assertEquals("z", map.num2Name(1));
        for (int i = 1; i < 27; i++) {
            switch (i) {
                case 1: case 20: case 23: case 25: case 26:
                    break;
                default:
                    assertNull(map.num2Name(i));
            }
        }
    }
}