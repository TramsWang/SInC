package sinc2.exp;

import sinc2.impl.est.EstRule;
import sinc2.kb.SimpleKb;

import java.io.IOException;
import java.util.HashMap;
import java.util.HashSet;

public class TestSpecWn {
    public static void main(String[] args) throws IOException {
        SimpleKb kb = new SimpleKb("WN18", "datasets/numerated/SimpleFormat");
        EstRule rule = new EstRule(0, 2, new HashSet<>(), new HashMap<>(), kb);
        System.out.println(rule);
        System.out.flush();
        rule.updateCacheIndices();
        rule.cvt2Uvs2NewLv(0, 2, 0, 0, 0);
        System.out.println(rule);
        System.out.flush();
        rule.updateCacheIndices();
        rule.cvt2Uvs2NewLv(0, 2, 1, 0, 1);
        System.out.println(rule);
        System.out.flush();
    }
}
