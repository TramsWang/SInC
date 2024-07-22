package sinc2.util.datagen;

import sinc2.common.Record;
import sinc2.kb.KbException;
import sinc2.util.kb.KbRelation;
import sinc2.util.kb.NumeratedKb;

import java.io.IOException;
import java.io.PrintWriter;

public class TsvDumper {
    public static void main(String[] args) throws KbException, IOException {
        dump(args[0], args[1], args[2]);
    }

    static void dump(String kbName, String kbPath, String outputPath) throws IOException, KbException {
        NumeratedKb kb = new NumeratedKb(kbName, kbPath);
        PrintWriter writer = new PrintWriter(outputPath);
        for (KbRelation relation: kb.getRelations()) {
            String rel_name = relation.getName();
            for (Record record: relation) {
                writer.write(rel_name);
                for (int arg: record.args) {
                    writer.write('\t');
                    writer.write(kb.num2Name(arg));
                }
                writer.write('\n');
            }
        }
        writer.close();
    }
}
