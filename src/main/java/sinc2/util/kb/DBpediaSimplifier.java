package sinc2.util.kb;

import sinc2.common.Record;
import sinc2.kb.KbException;

import java.io.IOException;
import java.io.PrintWriter;

public class DBpediaSimplifier {
    static int THRESHOLD = 1000;
    public static void main(String[] args) throws KbException, IOException {
        NumeratedKb kb = new NumeratedKb("DBpedia", ".");
        PrintWriter writer = new PrintWriter("simplified.tsv");
        int cnt_rels = 0;
        System.out.println("KB Loaded");
        for (KbRelation relation: kb.relations) {
            if (relation.totalRecords() >= THRESHOLD) {
                cnt_rels++;
                for (Record record: relation) {
                    writer.printf("%s\t%s\t%s\n", kb.num2Name(record.args[0]), relation.name, kb.num2Name(record.args[1]));
                }
                System.out.printf("Dump Rel #%d\n", relation.id);
            }
        }
        System.out.printf("Total Dumped: %d\n", cnt_rels);
    }
}
