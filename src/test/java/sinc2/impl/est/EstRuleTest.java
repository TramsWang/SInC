package sinc2.impl.est;

import org.junit.jupiter.api.Test;
import sinc2.common.Argument;
import sinc2.common.Predicate;
import sinc2.kb.SimpleKb;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

class EstRuleTest {

    static SimpleKb KB_SIMPLE = new SimpleKb(
            "test", new int[][][]{
                    new int[][] {new int[]{1, 1}},
                    new int[][] {new int[]{1, 1}},
                    new int[][] {new int[]{1, 1}}
            }, new String[]{"p" ,"q", "r"}
    );

}