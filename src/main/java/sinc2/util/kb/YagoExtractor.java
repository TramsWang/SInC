package sinc2.util.kb;

import sinc2.kb.KbException;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

public class YagoExtractor {
    public static void main(String[] args) throws IOException, KbException {
        NumeratedKb kb = new NumeratedKb("Yago");
        BufferedReader reader = new BufferedReader(new FileReader("yagoFacts.tsv"));
        String line;
        int cnt = 0;
        while (null != (line = reader.readLine())) {
            String[] components = line.split("\t");
            if (4 > components.length) {
                continue;
            }
            kb.addRecord(components[2], new String[]{components[1], components[3]});
            cnt++;
            if (cnt % 1000 == 0) {
                System.out.printf("%d K\n", cnt / 1000);
            }
        }
        kb.dump(".");
    }
}
