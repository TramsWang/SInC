package sinc2.impl.est;

import org.junit.jupiter.api.Test;
import sinc2.common.Argument;
import sinc2.common.Predicate;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import static org.junit.jupiter.api.Assertions.*;

class BodyVarLinkManagerTest {

    static final int p = 0;
    static final int q = 1;
    static final int r = 2;
    static final int s = 3;
    static final int t = 4;
    static final int u = 5;

    void assertGraphEqual(List<Set<VarLink>> expected, List<Set<VarLink>> actual) {
        assertEquals(expected.size(), actual.size());
        for (int i = 0; i < expected.size(); i++) {
            Set<VarLink> expected_links = expected.get(i);
            Set<VarLink> actual_links = actual.get(i);
            assertEquals(expected_links.size(), actual_links.size(), String.format("Different links @ %d", i));
            for (VarLink expected_link: expected_links) {
                boolean found = false;
                for (VarLink actual_link: actual_links) {
                    if (expected_link.equals(actual_link)) {
                        assertEquals(expected_link.predIdx, actual_link.predIdx);
                        assertEquals(expected_link.fromVid, actual_link.fromVid);
                        assertEquals(expected_link.fromArgIdx, actual_link.fromArgIdx);
                        assertEquals(expected_link.toVid, actual_link.toVid);
                        assertEquals(expected_link.toArgIdx, actual_link.toArgIdx);
                        found = true;
                        break;
                    }
                }
                assertTrue(found);
            }
        }
    }

    @Test
    void testConstruction1() {
        /* p(X, Y) :- q(X, ?), r(?, Y) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.EMPTY_VALUE}),
                new Predicate(r, new int[]{Argument.EMPTY_VALUE, Argument.variable(1)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 2);
        assertEquals(new ArrayList<>(List.of(0, 1)), bvm.varLabels);
        assertGraphEqual(new ArrayList<>(List.of(new HashSet<VarLink>(), new HashSet<VarLink>())), bvm.varLinkGraph);
    }

    @Test
    void testConstruction2() {
        /* p(X, Y) :- q(X, Z), r(Z, Y) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(2)}),
                new Predicate(r, new int[]{Argument.variable(2), Argument.variable(1)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 3);
        assertEquals(new ArrayList<>(List.of(0, 0, 0)), bvm.varLabels);
        List<Set<VarLink>> expected_graph = new ArrayList<>();
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 0, 0, 2, 1)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(2, 1, 1, 2, 0)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 2, 1, 0, 0),
                new VarLink(2, 2, 0, 1, 1)
        )));
        assertGraphEqual(expected_graph, bvm.varLinkGraph);
    }

    @Test
    void testConstruction3() {
        /* p(X, ?, Z) :- q(?, X, Y, Y), r(Z, W), s(W) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.EMPTY_VALUE, Argument.variable(2)}),
                new Predicate(q, new int[]{Argument.EMPTY_VALUE, Argument.variable(0), Argument.variable(1), Argument.variable(1)}),
                new Predicate(r, new int[]{Argument.variable(2), Argument.variable(3)}),
                new Predicate(s, new int[]{Argument.variable(3)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 4);
        assertEquals(new ArrayList<>(List.of(0, 0, 2, 2)), bvm.varLabels);
        List<Set<VarLink>> expected_graph = new ArrayList<>();
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 0, 1, 1, 2)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 1, 2, 0, 1)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(2, 2, 0, 3, 1)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(2, 3, 1, 2, 0)
        )));
        assertGraphEqual(expected_graph, bvm.varLinkGraph);
    }

    @Test
    void testConstruction4() {
        /* p(X, ?, Z) :- q(W, X, Y, Y), r(Z, W), s(W) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.EMPTY_VALUE, Argument.variable(2)}),
                new Predicate(q, new int[]{Argument.variable(3), Argument.variable(0), Argument.variable(1), Argument.variable(1)}),
                new Predicate(r, new int[]{Argument.variable(2), Argument.variable(3)}),
                new Predicate(s, new int[]{Argument.variable(3)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 4);
        assertEquals(new ArrayList<>(List.of(2, 2, 2, 2)), bvm.varLabels);
        List<Set<VarLink>> expected_graph = new ArrayList<>();
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 0, 1, 1, 2),
                new VarLink(1, 0, 1, 3, 0)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 1, 2, 0, 1),
                new VarLink(1, 1, 2, 3, 0)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(2, 2, 0, 3, 1)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 3, 0, 0, 1),
                new VarLink(1, 3, 0, 1, 2),
                new VarLink(2, 3, 1, 2, 0)
        )));
        assertGraphEqual(expected_graph, bvm.varLinkGraph);
    }

    @Test
    void testCopyConstruction() {
        /* p(X, Y) :- q(X, ?), r(?, Y) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.EMPTY_VALUE}),
                new Predicate(r, new int[]{Argument.EMPTY_VALUE, Argument.variable(1)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 2);
        BodyVarLinkManager bvm2 = new BodyVarLinkManager(bvm, copyRule(rule));
        assertEquals(new ArrayList<>(List.of(0, 1)), bvm2.varLabels);
        assertGraphEqual(new ArrayList<>(List.of(new HashSet<>(), new HashSet<>())), bvm2.varLinkGraph);

        /* p(X, Y) :- q(X, Z), r(Z, Y) */
        bvm2.specOprCase3(1, 1, 2, 0);
        assertEquals(new ArrayList<>(List.of(0, 0, 0)), bvm2.varLabels);
        List<Set<VarLink>> expected_graph = new ArrayList<>();
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 0, 0, 2, 1)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(2, 1, 1, 2, 0)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 2, 1, 0, 0),
                new VarLink(2, 2, 0, 1, 1)
        )));
        assertEquals(expected_graph, bvm2.varLinkGraph);

        assertEquals(new ArrayList<>(List.of(0, 1)), bvm.varLabels);
        assertGraphEqual(new ArrayList<>(List.of(new HashSet<>(), new HashSet<>())), bvm.varLinkGraph);
    }

    protected List<Predicate> copyRule(List<Predicate> rule) {
        List<Predicate> rule2 = new ArrayList<>();
        for (Predicate p: rule) {
            rule2.add(new Predicate(p));
        }
        return rule2;
    }

    @Test
    void specCase1test1() {
        /* p(X, Y) :- q(X, ?), r(?, Y) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.EMPTY_VALUE}),
                new Predicate(r, new int[]{Argument.EMPTY_VALUE, Argument.variable(1)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 2);

        /* p(X, Y) :- q(X, X), r(?, Y) */
        rule.get(1).args[1] = Argument.variable(0);
        bvm.specOprCase1(1, 1, 0);
        assertEquals(new ArrayList<>(List.of(0, 1)), bvm.varLabels);
        assertGraphEqual(new ArrayList<>(List.of(new HashSet<>(), new HashSet<>())), bvm.varLinkGraph);
    }

    @Test
    void specCase1test2() {
        /* p(X, Y) :- q(?, X), r(?, Y) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(q, new int[]{Argument.EMPTY_VALUE, Argument.variable(0)}),
                new Predicate(r, new int[]{Argument.EMPTY_VALUE, Argument.variable(1)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 2);

        /* p(X, Y) :- q(Y, X), r(?, Y) */
        rule.get(1).args[0] = Argument.variable(1);
        bvm.specOprCase1(1, 0, 1);
        assertEquals(new ArrayList<>(List.of(1, 1)), bvm.varLabels);
        List<Set<VarLink>> expected_graph = new ArrayList<>();
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 0, 1, 1, 0)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 1, 0, 0, 1)
        )));
        assertGraphEqual(expected_graph, bvm.varLinkGraph);
    }

    @Test
    void specCase1test3() {
        /* p(X, ?) :- q(X, Y), r(Y, Z), s(Z) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.EMPTY_VALUE}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(r, new int[]{Argument.variable(1), Argument.variable(2)}),
                new Predicate(s, new int[]{Argument.variable(2)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 3);

        /* p(X, Z) :- q(X, Y), r(Y, Z), s(Z) */
        rule.get(0).args[1] = Argument.variable(2);
        bvm.specOprCase1(0, 1, 2);
        assertEquals(new ArrayList<>(List.of(0, 0, 0)), bvm.varLabels);
        List<Set<VarLink>> expected_graph = new ArrayList<>();
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 0, 0, 1, 1)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 1, 1, 0, 0),
                new VarLink(2, 1, 0, 2, 1)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(2, 2, 1, 1, 0)
        )));
        assertGraphEqual(expected_graph, bvm.varLinkGraph);
    }

    @Test
    void specCase3test1() {
        /* p(X, Y) :- q(X, ?), r(?, Y) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.EMPTY_VALUE}),
                new Predicate(r, new int[]{Argument.EMPTY_VALUE, Argument.variable(1)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 2);

        /* p(X, Y) :- q(X, Z), r(Z, Y) */
        rule.get(1).args[1] = Argument.variable(2);
        rule.get(2).args[0] = Argument.variable(2);
        bvm.specOprCase3(1, 1, 2, 0);
        assertEquals(new ArrayList<>(List.of(0, 0, 0)), bvm.varLabels);
        List<Set<VarLink>> expected_graph = new ArrayList<>();
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 0, 0, 2, 1)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(2, 1, 1, 2, 0)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 2, 1, 0, 0),
                new VarLink(2, 2, 0, 1, 1)
        )));
        assertGraphEqual(expected_graph, bvm.varLinkGraph);
    }

    @Test
    void specCase3test2() {
        /* p(X, ?) :- q(X, Y), r(Y, ?) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.EMPTY_VALUE}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(r, new int[]{Argument.variable(1), Argument.EMPTY_VALUE})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 2);

        /* p(X, Z) :- q(X, Y), r(Y, Z) */
        rule.get(0).args[1] = Argument.variable(2);
        rule.get(2).args[1] = Argument.variable(2);
        bvm.specOprCase3(0, 1, 2, 1);
        assertEquals(new ArrayList<>(List.of(0, 0, 0)), bvm.varLabels);
        List<Set<VarLink>> expected_graph = new ArrayList<>();
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 0, 0, 1, 1)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 1, 1, 0, 0),
                new VarLink(2, 1, 0, 2, 1)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(2, 2, 1, 1, 0)
        )));
        assertGraphEqual(expected_graph, bvm.varLinkGraph);
    }

    @Test
    void specCase3test3() {
        /* p(X, ?, ?) :- q(X, Y), r(Y, ?) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.EMPTY_VALUE, Argument.EMPTY_VALUE}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(r, new int[]{Argument.variable(1), Argument.EMPTY_VALUE})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 2);

        /* p(X, Z, Z) :- q(X, Y), r(Y, ?) */
        rule.get(0).args[1] = Argument.variable(2);
        rule.get(0).args[2] = Argument.variable(2);
        bvm.specOprCase3(0, 1, 0, 2);
        assertEquals(new ArrayList<>(List.of(0, 0, 2)), bvm.varLabels);
        List<Set<VarLink>> expected_graph = new ArrayList<>();
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 0, 0, 1, 1)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 1, 1, 0, 0)
        )));
        expected_graph.add(new HashSet<>());
        assertGraphEqual(expected_graph, bvm.varLinkGraph);
    }

    @Test
    void specCase3test4() {
        /* p(X, ?, ?) :- q(X, Y), r(Y, ?, ?) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.EMPTY_VALUE, Argument.EMPTY_VALUE}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(r, new int[]{Argument.variable(1), Argument.EMPTY_VALUE, Argument.EMPTY_VALUE})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 2);

        /* p(X, ?, ?) :- q(X, Y), r(Y, Z, Z) */
        rule.get(2).args[1] = Argument.variable(2);
        rule.get(2).args[2] = Argument.variable(2);
        bvm.specOprCase3(2, 1, 2, 2);
        assertEquals(new ArrayList<>(List.of(0, 0, 0)), bvm.varLabels);
        List<Set<VarLink>> expected_graph = new ArrayList<>();
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 0, 0, 1, 1)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 1, 1, 0, 0),
                new VarLink(2, 1, 0, 2, 1)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(2, 2, 1, 1, 0)
        )));
        assertGraphEqual(expected_graph, bvm.varLinkGraph);
    }

    @Test
    void specCase4test1() {
        /* p(X, ?, ?) :- q(X, Y), r(Y, ?, ?) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.EMPTY_VALUE, Argument.EMPTY_VALUE}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(r, new int[]{Argument.variable(1), Argument.EMPTY_VALUE, Argument.EMPTY_VALUE})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 2);

        /* p(X, ?, ?) :- q(X, Y), r(Y, Z, ?), s(?, Z, ?) */
        rule.get(2).args[1] = Argument.variable(2);
        rule.add(new Predicate(s, new int[]{Argument.EMPTY_VALUE, Argument.variable(2), Argument.EMPTY_VALUE}));
        bvm.specOprCase4(2, 1);
        assertEquals(new ArrayList<>(List.of(0, 0, 0)), bvm.varLabels);
        List<Set<VarLink>> expected_graph = new ArrayList<>();
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 0, 0, 1, 1)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 1, 1, 0, 0),
                new VarLink(2, 1, 0, 2, 1)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(2, 2, 1, 1, 0)
        )));
        assertGraphEqual(expected_graph, bvm.varLinkGraph);
    }

    @Test
    void specCase4test2() {
        /* p(X, ?, ?) :- q(X, Y), r(Y, ?, ?) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.EMPTY_VALUE, Argument.EMPTY_VALUE}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(r, new int[]{Argument.variable(1), Argument.EMPTY_VALUE, Argument.EMPTY_VALUE})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 2);

        /* p(X, ?, Z) :- q(X, Y), r(Y, ?, ?), s(?, Z, ?) */
        rule.get(0).args[2] = Argument.variable(2);
        rule.add(new Predicate(s, new int[]{Argument.EMPTY_VALUE, Argument.variable(2), Argument.EMPTY_VALUE}));
        bvm.specOprCase4(0, 2);
        assertEquals(new ArrayList<>(List.of(0, 0, 2)), bvm.varLabels);
        List<Set<VarLink>> expected_graph = new ArrayList<>();
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 0, 0, 1, 1)
        )));
        expected_graph.add(new HashSet<>(List.of(
                new VarLink(1, 1, 1, 0, 0)
        )));
        expected_graph.add(new HashSet<>());
        assertGraphEqual(expected_graph, bvm.varLinkGraph);
    }

    void assertPathEquals(VarLink[] expected, VarLink[] actual) {
        assertEquals(expected.length, actual.length);
        for (int i = 0; i < expected.length; i++) {
            VarLink expected_link = expected[i];
            VarLink actual_link = actual[i];
            assertEquals(expected_link.predIdx, actual_link.predIdx);
            assertEquals(expected_link.fromVid, actual_link.fromVid);
            assertEquals(expected_link.fromArgIdx, actual_link.fromArgIdx);
            assertEquals(expected_link.toVid, actual_link.toVid);
            assertEquals(expected_link.toArgIdx, actual_link.toArgIdx);
        }
    }

    @Test
    void testFindShortestPath1() {
        /* p(X, Z, W) :- q(X, Y, ?), r(Y, ?, Z), s(W) [X, Y] */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(2), Argument.variable(3)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(1), Argument.EMPTY_VALUE}),
                new Predicate(r, new int[]{Argument.variable(1), Argument.EMPTY_VALUE, Argument.variable(2)}),
                new Predicate(s, new int[]{Argument.variable(3)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 4);
        assertPathEquals(new VarLink[]{
                new VarLink(1, 0, 0, 1, 1)
        }, bvm.shortestPath(0, 1));
        assertPathEquals(new VarLink[]{
                new VarLink(1, 1, 1, 0, 0)
        }, bvm.shortestPath(1, 0));
    }

    @Test
    void testFindShortestPath2() {
        /* p(X, Z, W) :- q(X, Y, ?), r(Y, ?, Z), s(W) [X, Z] */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(2), Argument.variable(3)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(1), Argument.EMPTY_VALUE}),
                new Predicate(r, new int[]{Argument.variable(1), Argument.EMPTY_VALUE, Argument.variable(2)}),
                new Predicate(s, new int[]{Argument.variable(3)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 4);
        assertPathEquals(new VarLink[]{
                new VarLink(1, 0, 0, 1, 1),
                new VarLink(2, 1, 0, 2, 2)
        }, bvm.shortestPath(0, 2));
        assertPathEquals(new VarLink[]{
                new VarLink(2, 2, 2, 1, 0),
                new VarLink(1, 1, 1, 0, 0),
        }, bvm.shortestPath(2, 0));
    }

    @Test
    void testFindShortestPath3() {
        /* p(X, Z, W) :- q(X, Y, ?), r(Y, ?, Z), s(W) [X, W] */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(2), Argument.variable(3)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(1), Argument.EMPTY_VALUE}),
                new Predicate(r, new int[]{Argument.variable(1), Argument.EMPTY_VALUE, Argument.variable(2)}),
                new Predicate(s, new int[]{Argument.variable(3)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 4);
        assertNull(bvm.shortestPath(0, 3));
        assertNull(bvm.shortestPath(1, 3));
        assertNull(bvm.shortestPath(2, 3));
        assertNull(bvm.shortestPath(3, 0));
        assertNull(bvm.shortestPath(3, 1));
        assertNull(bvm.shortestPath(3, 2));
    }

    @Test
    void testFindShortestPath4() {
        /* p(X, Z, W) :- q(X, Y, W), r(Y, ?, Z), s(W, R), s(R, Z) [X, Z] */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(2), Argument.variable(3)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(1), Argument.variable(3)}),
                new Predicate(r, new int[]{Argument.variable(1), Argument.EMPTY_VALUE, Argument.variable(2)}),
                new Predicate(s, new int[]{Argument.variable(3), Argument.variable(4)}),
                new Predicate(s, new int[]{Argument.variable(4), Argument.variable(2)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 5);
        assertPathEquals(new VarLink[]{
                new VarLink(1, 0, 0, 1, 1),
                new VarLink(2, 1, 0, 2, 2)
        }, bvm.shortestPath(0, 2));
        assertPathEquals(new VarLink[]{
                new VarLink(1, 3, 2, 0, 0),
        }, bvm.shortestPath(3, 0));
    }

    void assertEqualVarPairs(Set<VarPair> expected, Set<VarPair> actual) {
        assertEquals(expected.size(), actual.size());
        for (VarPair expected_pair: expected) {
            boolean found = false;
            for (VarPair actual_pair: actual) {
                if ((expected_pair.vid1 == actual_pair.vid1 && expected_pair.vid2 == actual_pair.vid2) ||
                        (expected_pair.vid1 == actual_pair.vid2 && expected_pair.vid2 == actual_pair.vid1)) {
                    found = true;
                    break;
                }
            }
            assertTrue(found);
        }
    }

    @Test
    void assumeSpecCase1test1() {
        /* p(X, Y) :- q(X, Z), r(?, Y), s(Z) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(2)}),
                new Predicate(r, new int[]{Argument.EMPTY_VALUE, Argument.variable(1)}),
                new Predicate(s, new int[]{Argument.variable(2)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 3);

        /* p(X, Y) :- q(X, Z), r(Z, Y), s(Z) */
        Set<VarPair> expected_pairs = new HashSet<>(List.of(new VarPair(0, 1), new VarPair(2, 1)));
        assertEqualVarPairs(expected_pairs, bvm.assumeSpecOprCase1(2, 0, 2));
    }

    @Test
    void assumeSpecCase1test2() {
        /* p(X, Y) :- q(X, Z), r(?, Y), s(Z) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(2)}),
                new Predicate(r, new int[]{Argument.EMPTY_VALUE, Argument.variable(1)}),
                new Predicate(s, new int[]{Argument.variable(2)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 3);

        /* p(X, Y) :- q(X, Z), r(X, Y), s(Z) */
        Set<VarPair> expected_pairs = new HashSet<>(List.of(new VarPair(0, 1), new VarPair(2, 1)));
        assertEqualVarPairs(expected_pairs, bvm.assumeSpecOprCase1(2, 0, 0));
    }

    @Test
    void assumeSpecCase1test3() {
        /* p(X, Y) :- q(X, Z), r(?, Y), s(Z) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(2)}),
                new Predicate(r, new int[]{Argument.EMPTY_VALUE, Argument.variable(1)}),
                new Predicate(s, new int[]{Argument.variable(2)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 3);

        /* p(X, Y) :- q(X, Z), r(Y, Y), s(Z) */
        assertTrue(bvm.assumeSpecOprCase1(2, 0, 1).isEmpty());
    }

    @Test
    void assumeSpecCase1test4() {
        /* p(X, Y) :- q(X, W, Z), r(?, Y, ?), s(Z), t(W, R), u(W, R), u(Y, S) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(3), Argument.variable(2)}),
                new Predicate(r, new int[]{Argument.EMPTY_VALUE, Argument.variable(1), Argument.EMPTY_VALUE}),
                new Predicate(s, new int[]{Argument.variable(2)}),
                new Predicate(t, new int[]{Argument.variable(3), Argument.variable(4)}),
                new Predicate(u, new int[]{Argument.variable(3), Argument.variable(4)}),
                new Predicate(u, new int[]{Argument.variable(1), Argument.variable(5)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 6);

        /* p(X, Y) :- q(X, W, Z), r(Z, Y, R), s(Z), t(W, R), u(W, R) */
        Set<VarPair> expected_pairs = new HashSet<>(List.of(
                new VarPair(0, 1), new VarPair(2, 1), new VarPair(3, 1), new VarPair(4, 1),
                new VarPair(0, 5), new VarPair(2, 5), new VarPair(3, 5), new VarPair(4, 5)
        ));
        assertEqualVarPairs(expected_pairs, bvm.assumeSpecOprCase1(2, 0, 2));
    }

    @Test
    void assumeSpecCase3test1() {
        /* p(X, Y) :- q(X, Z), r(Z, Y), s(X, ?), t(?, Y)*/
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(2)}),
                new Predicate(r, new int[]{Argument.variable(2), Argument.variable(1)}),
                new Predicate(s, new int[]{Argument.variable(0), Argument.EMPTY_VALUE}),
                new Predicate(t, new int[]{Argument.EMPTY_VALUE, Argument.variable(1)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 3);

        /* p(X, Y) :- q(X, Z), r(Z, Y), s(X, W), t(W, Y)*/
        assertTrue(bvm.assumeSpecOprCase3(3, 1, 4, 0).isEmpty());
    }

    @Test
    void assumeSpecCase3test2() {
        /* p(X, Y, Z) :- q(X, W), r(W, Y), s(Y, ?), t(?, Z)*/
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(1), Argument.variable(2)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(3)}),
                new Predicate(r, new int[]{Argument.variable(3), Argument.variable(1)}),
                new Predicate(s, new int[]{Argument.variable(1), Argument.EMPTY_VALUE}),
                new Predicate(t, new int[]{Argument.EMPTY_VALUE, Argument.variable(2)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 4);

        /* p(X, Y, Z) :- q(X, W), r(W, Y), s(Y, R), t(R, Z)*/
        Set<VarPair> expected_pairs = new HashSet<>(List.of(
                new VarPair(0, 2), new VarPair(1, 2), new VarPair(3, 2)
        ));
        assertEqualVarPairs(expected_pairs, bvm.assumeSpecOprCase3(3, 1, 4, 0));
    }

    @Test
    void assumeShortestPathCase1test1() {
        /* p(X, Y) :- q(X, Z), r(?, Y), s(Z) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(2)}),
                new Predicate(r, new int[]{Argument.EMPTY_VALUE, Argument.variable(1)}),
                new Predicate(s, new int[]{Argument.variable(2)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 3);

        /* p(X, Y) :- q(X, Z), r(Z, Y), s(Z) */
        assertPathEquals(
                new VarLink[]{
                        new VarLink(1, 0, 0, 2, 1),
                        new VarLink(2, 2, 0, 1, 1),
                },
                bvm.assumeShortestPathCase1(2, 0, 2, 0, 1)
        );
        assertPathEquals(
                new VarLink[]{
                        new VarLink(2, 1, 1, 2, 0),
                        new VarLink(1, 2, 1, 0, 0),
                },
                bvm.assumeShortestPathCase1(2, 0, 2, 1, 0)
        );
        assertPathEquals(
                new VarLink[]{
                        new VarLink(2, 2, 0, 1, 1),
                },
                bvm.assumeShortestPathCase1(2, 0, 2, 2, 1)
        );
        assertPathEquals(
                new VarLink[]{
                        new VarLink(2, 1, 1, 2, 0),
                },
                bvm.assumeShortestPathCase1(2, 0, 2, 1, 2)
        );
    }

    @Test
    void assumeShortestPathCase1test2() {
        /* p(X, Y) :- q(X, Z), r(?, Y), s(Z) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(2)}),
                new Predicate(r, new int[]{Argument.EMPTY_VALUE, Argument.variable(1)}),
                new Predicate(s, new int[]{Argument.variable(2)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 3);

        /* p(X, Y) :- q(X, Z), r(X, Y), s(Z) */
        assertPathEquals(
                new VarLink[]{
                        new VarLink(2, 1, 1, 0, 0),
                },
                bvm.assumeShortestPathCase1(2, 0, 0, 1, 0)
        );
        assertPathEquals(
                new VarLink[]{
                        new VarLink(2, 1, 1, 0, 0),
                        new VarLink(1, 0, 0, 2, 1),
                },
                bvm.assumeShortestPathCase1(2, 0, 0, 1, 2)
        );
        assertPathEquals(
                new VarLink[]{
                        new VarLink(1, 2, 1, 0, 0),
                        new VarLink(2, 0, 0, 1, 1),
                },
                bvm.assumeShortestPathCase1(2, 0, 0, 2, 1)
        );
    }

    @Test
    void assumeShortestPathCase1test3() {
        /* p(X, Y) :- q(X, Z), r(?, Y), s(Z) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(2)}),
                new Predicate(r, new int[]{Argument.EMPTY_VALUE, Argument.variable(1)}),
                new Predicate(s, new int[]{Argument.variable(2)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 3);

        /* p(X, Y) :- q(X, Z), r(Y, Y), s(Z) */
        assertNull(bvm.assumeShortestPathCase1(2, 0, 1, 1, 0));
        assertNull(bvm.assumeShortestPathCase1(2, 0, 1, 0, 1));
        assertNull(bvm.assumeShortestPathCase1(2, 0, 1, 1, 2));
        assertNull(bvm.assumeShortestPathCase1(2, 0, 1, 2, 1));
    }

    @Test
    void assumeShortestPathCase1test4() {
        /* p(X, Y) :- q(X, W, Z), r(?, Y, ?), s(Z), t(W, R), u(W, R) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(3), Argument.variable(2)}),
                new Predicate(r, new int[]{Argument.EMPTY_VALUE, Argument.variable(1), Argument.EMPTY_VALUE}),
                new Predicate(s, new int[]{Argument.variable(2)}),
                new Predicate(t, new int[]{Argument.variable(3), Argument.variable(4)}),
                new Predicate(u, new int[]{Argument.variable(3), Argument.variable(4)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 5);

        /* p(X, Y) :- q(X, W, Z), r(Z, Y, ?), s(Z), t(W, R), u(W, R) */
        assertPathEquals(
                new VarLink[]{
                        new VarLink(2, 1, 1, 2, 0),
                        new VarLink(1, 2, 2, 0, 0),
                },
                bvm.assumeShortestPathCase1(2, 0, 2, 1, 0)
        );
        assertPathEquals(
                new VarLink[]{
                        new VarLink(2, 2, 0, 1, 1),
                },
                bvm.assumeShortestPathCase1(2, 0, 2, 2, 1)
        );
        assertPathEquals(
                new VarLink[]{
                        new VarLink(1, 3, 1, 2, 2),
                        new VarLink(2, 2, 0, 1, 1),
                },
                bvm.assumeShortestPathCase1(2, 0, 2, 3, 1)
        );
        assertPathEquals(
                new VarLink[]{
                        new VarLink(4, 4, 1, 3, 0),
                        new VarLink(1, 3, 1, 2, 2),
                        new VarLink(2, 2, 0, 1, 1),
                },
                bvm.assumeShortestPathCase1(2, 0, 2, 4, 1)
        );
    }

    @Test
    void assumeShortestPathCase3test1() {
        /* p(X, Y) :- q(X, ?), r(?, Y) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(1)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.EMPTY_VALUE}),
                new Predicate(r, new int[]{Argument.EMPTY_VALUE, Argument.variable(1)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 2);

        /* p(X, Y) :- q(X, Z), r(Z, Y) */
        assertPathEquals(
                new VarLink[]{
                        new VarLink(2, 1, 1, -1, 0),
                        new VarLink(1, -1, 1, 0, 0),
                },
                bvm.assumeShortestPathCase3(1, 1, 2, 0, 1, 0)
        );
    }

    @Test
    void assumeShortestPathCase3test2() {
        /* p(X, Y, Z) :- q(X, W), r(W, Y), s(Y, ?), t(?, Z)*/
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(1), Argument.variable(2)}),
                new Predicate(q, new int[]{Argument.variable(0), Argument.variable(3)}),
                new Predicate(r, new int[]{Argument.variable(3), Argument.variable(1)}),
                new Predicate(s, new int[]{Argument.variable(1), Argument.EMPTY_VALUE}),
                new Predicate(t, new int[]{Argument.EMPTY_VALUE, Argument.variable(2)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 4);

        /* p(X, Y, Z) :- q(X, W), r(W, Y), s(Y, R), t(R, Z)*/
        assertPathEquals(
                new VarLink[]{
                        new VarLink(1, 0, 0, 3, 1),
                        new VarLink(2, 3, 0, 1, 1),
                        new VarLink(3, 1, 0, -1, 1),
                        new VarLink(4, -1, 0, 2, 1),
                },
                bvm.assumeShortestPathCase3(3, 1, 4, 0, 0, 2)
        );
    }

    @Test
    void assumeShortestPathCase3test3() {
        /* p(X, Y, ?) :- q(?, Y), r(X, Y) */
        List<Predicate> rule = new ArrayList<>(List.of(
                new Predicate(p, new int[]{Argument.variable(0), Argument.variable(1), Argument.EMPTY_VALUE}),
                new Predicate(q, new int[]{Argument.EMPTY_VALUE, Argument.variable(1)}),
                new Predicate(r, new int[]{Argument.variable(0), Argument.variable(1)})
        ));
        BodyVarLinkManager bvm = new BodyVarLinkManager(rule, 2);

        /* p(X, Y, Z) :- q(Z, Y), r(X, Y) */
        assertPathEquals(
                new VarLink[]{
                        new VarLink(2, 0, 0, 1, 1),
                        new VarLink(1, 1, 1, -1, 0),
                },
                bvm.assumeShortestPathCase3(1, 0, 0)
        );
        assertPathEquals(
                new VarLink[]{
                        new VarLink(1, 1, 1, -1, 0),
                },
                bvm.assumeShortestPathCase3(1, 0, 1)
        );
    }
}