package sinc2.util.kb;

import sinc2.kb.KbException;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

public class DBpediaExtractor {
    public static void main(String[] args) throws IOException, KbException {
        NumeratedKb kb = new NumeratedKb("DBpedia");
//        BufferedReader reader = new BufferedReader(new FileReader("infobox_properties_en.ttl"));
        BufferedReader reader = new BufferedReader(new FileReader("simplified.tsv"));
        String line = reader.readLine();    // ignor first line
        int cnt = 0;
        String[] pair = new String[2];
        while (null != (line = reader.readLine())) {
            String[] components = line.strip().split("\\s+");
            if (3 > components.length) {
                continue;
            }
            pair[0] = components[0];
            pair[1] = components[2];
            kb.addRecord(components[1], pair);
            cnt++;
            if (cnt % 1000 == 0) {
                System.out.printf("%d K\n", cnt / 1000);
            }
//            if (27132194 == cnt) {
//                break;
//            }
        }
        kb.dump(".");
    }
}
