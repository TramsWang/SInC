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

    @Test
    void testBuildLinkTree1() {
        /* p(X, Y) :- q(X, Y) */
        List<LinkTreeNode> expected_link_tree = new ArrayList<>();
        LinkTreeNode node_head = new LinkTreeNode(2, 0, 0, 0);
        node_head.level = 0;
        LinkTreeNode node_body = new LinkTreeNode(2, 0, 0, 0);
        node_body.level = 1;
        expected_link_tree.add(node_head);
        expected_link_tree.add(node_body);
        
        EstRule rule1 = new EstRule(0, 2, new HashSet<>(), new HashMap<>(), KB_SIMPLE);
        rule1.cvt2Uvs2NewLv(1, 2, 0, 0, 0);
        rule1.updateCacheIndices();
        rule1.cvt2Uvs2NewLv(0, 1, 1, 1);
        assertEquals("p(X0,X1):-q(X0,X1)", rule1.toDumpString(KB_SIMPLE));
        assertIterableEquals(expected_link_tree, rule1.linkTree);

        Predicate head = new Predicate(0, 2);
        head.args[0] = Argument.variable(0);
        head.args[1] = Argument.variable(1);
        Predicate body = new Predicate(1, 2);
        body.args[0] = Argument.variable(0);
        body.args[1] = Argument.variable(1);
        EstRule rule2 = new EstRule(List.of(head, body), new HashSet<>(), new HashMap<>(), KB_SIMPLE);
        assertEquals("p(X0,X1):-q(X0,X1)", rule2.toDumpString(KB_SIMPLE));
        assertIterableEquals(expected_link_tree, rule2.linkTree);

        EstRule rule3 = new EstRule(rule1);
        rule1.updateCacheIndices();
        rule1.cvt1Uv2ExtLv(1, 2, 0, 0);
        assertEquals("p(X0,X1):-q(X0,X1)", rule3.toDumpString(KB_SIMPLE));
        assertIterableEquals(expected_link_tree, rule3.linkTree);
    }

    @Test
    void testBuildLinkTree2() {
        /* p(Y, X) :- q(X, Z), r(Z, Y) */
        List<LinkTreeNode> expected_link_tree = new ArrayList<>();
        LinkTreeNode node_head = new LinkTreeNode(2, 0, 0, 0);
        node_head.level = 0;
        LinkTreeNode node_body1 = new LinkTreeNode(2, 0, 0, 1);
        node_body1.level = 1;
        LinkTreeNode node_body2 = new LinkTreeNode(2, 1, 0, 0);
        node_body2.level = 1;
        expected_link_tree.add(node_head);
        expected_link_tree.add(node_body1);
        expected_link_tree.add(node_body2);

        EstRule rule1 = new EstRule(0, 2, new HashSet<>(), new HashMap<>(), KB_SIMPLE);
        rule1.cvt2Uvs2NewLv(1, 2, 0, 0, 1);
        rule1.updateCacheIndices();
        rule1.cvt2Uvs2NewLv(2, 2, 1, 0, 0);
        rule1.updateCacheIndices();
        rule1.cvt2Uvs2NewLv(1, 1, 2, 0);
        assertEquals("p(X1,X0):-q(X0,X2),r(X2,X1)", rule1.toDumpString(KB_SIMPLE));
        assertIterableEquals(expected_link_tree, rule1.linkTree);

        Predicate head = new Predicate(0, 2);
        head.args[0] = Argument.variable(1);
        head.args[1] = Argument.variable(0);
        Predicate body1 = new Predicate(1, 2);
        body1.args[0] = Argument.variable(0);
        body1.args[1] = Argument.variable(2);
        Predicate body2 = new Predicate(2, 2);
        body2.args[0] = Argument.variable(2);
        body2.args[1] = Argument.variable(1);
        EstRule rule2 = new EstRule(List.of(head, body1, body2), new HashSet<>(), new HashMap<>(), KB_SIMPLE);
        assertEquals("p(X0,X1):-q(X1,X2),r(X2,X0)", rule2.toDumpString(KB_SIMPLE));
        assertIterableEquals(expected_link_tree, rule2.linkTree);

        EstRule rule3 = new EstRule(rule1);
        rule1.updateCacheIndices();
        rule1.cvt1Uv2ExtLv(1, 2, 0, 0);
        assertEquals("p(X1,X0):-q(X0,X2),r(X2,X1)", rule3.toDumpString(KB_SIMPLE));
        assertIterableEquals(expected_link_tree, rule3.linkTree);
    }

    @Test
    void testBuildLinkTree3() {
        /* p(?, X) :- q(X, Y), r(Y, X), r(?, X) */
        List<LinkTreeNode> expected_link_tree = new ArrayList<>();
        LinkTreeNode node_head = new LinkTreeNode(2, 0, 0, 0);
        node_head.level = 0;
        LinkTreeNode node_body1 = new LinkTreeNode(2, 0, 0, 1);
        node_body1.level = 1;
        LinkTreeNode node_body2 = new LinkTreeNode(2, 1, 0, 1);
        node_body2.level = 1;
        LinkTreeNode node_body3 = new LinkTreeNode(2, 1, 0, 1);
        node_body3.level = 1;
        expected_link_tree.add(node_head);
        expected_link_tree.add(node_body1);
        expected_link_tree.add(node_body2);
        expected_link_tree.add(node_body3);

        EstRule rule1 = new EstRule(0, 2, new HashSet<>(), new HashMap<>(), KB_SIMPLE);
        rule1.cvt2Uvs2NewLv(1, 2, 0, 0, 1);
        rule1.updateCacheIndices();
        rule1.cvt2Uvs2NewLv(2, 2, 0, 1, 1);
        rule1.updateCacheIndices();
        rule1.cvt1Uv2ExtLv(2, 1, 0);
        rule1.updateCacheIndices();
        rule1.cvt1Uv2ExtLv(2, 2, 1, 0);
        assertEquals("p(?,X0):-q(X0,X1),r(X1,X0),r(?,X0)", rule1.toDumpString(KB_SIMPLE));
        assertIterableEquals(expected_link_tree, rule1.linkTree);

        Predicate head = new Predicate(0, 2);
        head.args[0] = Argument.EMPTY_VALUE;
        head.args[1] = Argument.variable(0);
        Predicate body1 = new Predicate(1, 2);
        body1.args[0] = Argument.variable(0);
        body1.args[1] = Argument.variable(1);
        Predicate body2 = new Predicate(2, 2);
        body2.args[0] = Argument.variable(1);
        body2.args[1] = Argument.variable(0);
        Predicate body3 = new Predicate(2, 2);
        body3.args[0] = Argument.EMPTY_VALUE;
        body3.args[1] = Argument.variable(0);
        EstRule rule2 = new EstRule(List.of(head, body1, body2, body3), new HashSet<>(), new HashMap<>(), KB_SIMPLE);
        assertEquals("p(?,X0):-q(X0,X1),r(X1,X0),r(?,X0)", rule2.toDumpString(KB_SIMPLE));
        assertIterableEquals(expected_link_tree, rule2.linkTree);

        EstRule rule3 = new EstRule(rule1);
        rule1.updateCacheIndices();
        rule1.cvt1Uv2ExtLv(1, 2, 0, 0);
        assertEquals("p(?,X0):-q(X0,X1),r(X1,X0),r(?,X0)", rule3.toDumpString(KB_SIMPLE));
        assertIterableEquals(expected_link_tree, rule3.linkTree);
    }
}