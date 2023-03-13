package sinc2.util.kb;

import sinc2.kb.KbException;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

public class FormatConverter {
    public static void main(String[] args) throws IOException, KbException {
        if (2 != args.length) {
            System.err.println("Usage: <Input base dir> <Output dir>");
            return;
        }

        String base_dir = args[0];
        String target_dir = args[1];
        File base_dir_file = new File(base_dir);
        for (File f: base_dir_file.listFiles()) {
            if (f.isFile() && f.getName().endsWith(".tsv")) {
                System.out.printf("Converting %s ...\n", f.getName());

                /* Load TSV as Numerated KB */
                BufferedReader reader = new BufferedReader(new FileReader(f));
                String line;
                NumeratedKb kb = new NumeratedKb(f.getName().replace(".tsv", ""));
                while (null != (line = reader.readLine())) {
                    String[] components = line.split("\t"); // p, s, o
                    String[] record = new String[components.length - 1];
                    System.arraycopy(components, 1, record, 0, record.length);
                    kb.addRecord(components[0], record);
                }
                reader.close();

                /* Dump */
                kb.dump(target_dir);
            }
        }
    }
}
