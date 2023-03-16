package sinc2.rule;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import sinc2.common.*;
import sinc2.util.MultiSet;
import sinc2.util.kb.NumerationMap;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;

class FingerprintTest {

    static final String FUNCTOR_H = "h";
    static final String FUNCTOR_P = "p";
    static final String FUNCTOR_Q = "q";
    static final String FUNCTOR_FATHER = "father";
    static final String FUNCTOR_PARENT = "parent";
    static final String FUNCTOR_GRANDPARENT = "grandparent";
    static final String CONST_C = "c";
    static final String CONST_E = "e";
    
    static final int NUM_H = 1;
    static final int NUM_P = 2;
    static final int NUM_Q = 3;
    static final int NUM_FATHER = 4;
    static final int NUM_PARENT = 5;
    static final int NUM_GRANDPARENT = 6;
    static final int NUM_C = 7;
    static final int NUM_E = 8;
    
    static final int ARITY_FATHER = 2;
    static final int ARITY_PARENT = 2;
    static final int ARITY_GRANDPARENT = 2;
    static final NumerationMap map = new NumerationMap();

    static List<List<Predicate>[]> rule_pairs_dup_positive = new ArrayList<>();
    static List<List<Predicate>[]> rule_pairs_dup_negative = new ArrayList<>();
    static List<List<Predicate>[]> rule_pairs_spec_positive = new ArrayList<>();
    static List<List<Predicate>[]> rule_pairs_spec_negative = new ArrayList<>();

    @BeforeAll
    static void prepareTestCases() throws IOException, RuleParseException {
        /* Create numeration map*/
        assertEquals(NUM_H, map.mapName(FUNCTOR_H));
        assertEquals(NUM_P, map.mapName(FUNCTOR_P));
        assertEquals(NUM_Q, map.mapName(FUNCTOR_Q));
        assertEquals(NUM_FATHER, map.mapName(FUNCTOR_FATHER));
        assertEquals(NUM_PARENT, map.mapName(FUNCTOR_PARENT));
        assertEquals(NUM_GRANDPARENT, map.mapName(FUNCTOR_GRANDPARENT));
        assertEquals(NUM_C, map.mapName(CONST_C));
        assertEquals(NUM_E, map.mapName(CONST_E));

        /* Load test cases */
        String[] file_names = new String[]{
                "test/dup_rules_positive.txt",
                "test/dup_rules_negative.txt",
                "test/spec_rules_positive.txt",
                "test/spec_rules_negative.txt"
        };
        List<List<Predicate>[]>[] test_cases_lists = new List[]{
                rule_pairs_dup_positive,
                rule_pairs_dup_negative,
                rule_pairs_spec_positive,
                rule_pairs_spec_negative
        };
        for (int i = 0; i < file_names.length; i++) {
//            BufferedReader reader = new BufferedReader(new FileReader(file_names[i]));
            BufferedReader reader = new BufferedReader(new InputStreamReader(FingerprintTest.class.getClassLoader().getResourceAsStream(file_names[i])));
            List<List<Predicate>[]> test_cases = test_cases_lists[i];
            while (true) {
                String rule_str1 = reader.readLine();
                String rule_str2 = reader.readLine();
                if (null == rule_str1) break;
                List<ParsedPred> parsed_rule_1 = Rule.parseStructure(rule_str1);
                List<ParsedPred> parsed_rule_2 = Rule.parseStructure(rule_str2);
                test_cases.add(new List[]{
                        parsedStructure2RuleStructure(parsed_rule_1),
                        parsedStructure2RuleStructure(parsed_rule_2)
                });
            }
            reader.close();
        }
    }

    @Test
    public void testConstruction() {
        /* h(X, c) <- p(X, Y), q(Y, Z, e), h(Z, ?), h(X, ?) */
        /* Equivalence Classes:
         *    X: {h[0], p[0], h[0]}
         *    c: {h[1], c}
         *    Y: {p[1], q[0]}
         *    Z: {q[1], h[0]}
         *    e: {q[2], e}
         *    ?: {h[1]}
         *    ?: {h[1]}
         */
        final Predicate head = new Predicate(NUM_H, 2);
        head.args[0] = Argument.variable(0);
        head.args[1] = Argument.constant(NUM_C);
        final Predicate body1 = new Predicate(NUM_P, 2);
        body1.args[0] = Argument.variable(0);
        body1.args[1] = Argument.variable(1);
        final Predicate body2 = new Predicate(NUM_Q, 3);
        body2.args[0] = Argument.variable(1);
        body2.args[1] = Argument.variable(2);
        body2.args[2] = Argument.constant(NUM_E);
        final Predicate body3 = new Predicate(NUM_H, 2);
        body3.args[0] = Argument.variable(2);
        body3.args[1] = Argument.EMPTY_VALUE;
        final Predicate body4 = new Predicate(NUM_H, 2);
        body4.args[0] = Argument.variable(0);
        body4.args[1] = Argument.EMPTY_VALUE;
        final List<Predicate> rule = List.of(head, body1, body2, body3, body4);
        assertEquals("h(X0,c):-p(X0,X1),q(X1,X2,e),h(X2,?),h(X0,?)", rule2String(rule));

        MultiSet<ArgIndicator> eqc_x = new MultiSet<>();
        eqc_x.add(ArgIndicator.getVariableIndicator(NUM_H, 0));
        eqc_x.add(ArgIndicator.getVariableIndicator(NUM_P, 0));
        eqc_x.add(ArgIndicator.getVariableIndicator(NUM_H, 0));
        MultiSet<ArgIndicator> eqc_c = new MultiSet<>();
        eqc_c.add(ArgIndicator.getVariableIndicator(NUM_H, 1));
        eqc_c.add(ArgIndicator.getConstantIndicator(NUM_C));
        MultiSet<ArgIndicator> eqc_y = new MultiSet<>();
        eqc_y.add(ArgIndicator.getVariableIndicator(NUM_P, 1));
        eqc_y.add(ArgIndicator.getVariableIndicator(NUM_Q, 0));
        MultiSet<ArgIndicator> eqc_z = new MultiSet<>();
        eqc_z.add(ArgIndicator.getVariableIndicator(NUM_Q, 1));
        eqc_z.add(ArgIndicator.getVariableIndicator(NUM_H, 0));
        MultiSet<ArgIndicator> eqc_e = new MultiSet<>();
        eqc_e.add(ArgIndicator.getVariableIndicator(NUM_Q, 2));
        eqc_e.add(ArgIndicator.getConstantIndicator(NUM_E));
        MultiSet<ArgIndicator> eqc_uv1 = new MultiSet<>();
        eqc_uv1.add(ArgIndicator.getVariableIndicator(NUM_H, 1));
        MultiSet<ArgIndicator> eqc_uv2 = new MultiSet<>();
        eqc_uv2.add(ArgIndicator.getVariableIndicator(NUM_H, 1));
        MultiSet<MultiSet<ArgIndicator>> expected_eqc_set = new MultiSet<>(
                new MultiSet[]{eqc_x, eqc_y, eqc_z, eqc_c, eqc_e, eqc_uv1, eqc_uv2}
        );
        Fingerprint.PredicateWithClass pwc_head = new Fingerprint.PredicateWithClass(NUM_H, 2);
        pwc_head.classArgs[0] = eqc_x;
        pwc_head.classArgs[1] = eqc_c;
        Fingerprint.PredicateWithClass pwc_body1 = new Fingerprint.PredicateWithClass(NUM_P, 2);
        pwc_body1.classArgs[0] = eqc_x;
        pwc_body1.classArgs[1] = eqc_y;
        Fingerprint.PredicateWithClass pwc_body2 = new Fingerprint.PredicateWithClass(NUM_Q, 3);
        pwc_body2.classArgs[0] = eqc_y;
        pwc_body2.classArgs[1] = eqc_z;
        pwc_body2.classArgs[2] = eqc_e;
        Fingerprint.PredicateWithClass pwc_body3 = new Fingerprint.PredicateWithClass(NUM_H, 2);
        pwc_body3.classArgs[0] = eqc_z;
        pwc_body3.classArgs[1] = eqc_uv1;
        Fingerprint.PredicateWithClass pwc_body4 = new Fingerprint.PredicateWithClass(NUM_H, 2);
        pwc_body4.classArgs[0] = eqc_x;
        pwc_body4.classArgs[1] = eqc_uv2;
        List<Fingerprint.PredicateWithClass> expected_classed_structure = new ArrayList<>(List.of(
                new Fingerprint.PredicateWithClass[]{pwc_head, pwc_body1, pwc_body2, pwc_body3, pwc_body4}
        ));

        Fingerprint fingerprint = new Fingerprint(rule);
        assertEquals(expected_eqc_set, fingerprint.getEquivalenceClasses());
        assertEquals(expected_classed_structure, fingerprint.getClassedStructure());
    }

    @Test
    public void testDupFromStructure() {
        String[][] rule_pair_strs = new String[][]{
                new String[]{
                        "h(X0,X1):-h(X1,X0)",
                        "h(X1,X0):-h(X0,X1)"
                },
                new String[]{
                        "h(X0,X2):-p(X0,X1),q(X1,X2)",
                        "h(X1,X0):-q(X2,X0),p(X1,X2)"
                }
        };
        List<Predicate>[][] rule_pairs = new List[][]{
                new List[] {
                        new ArrayList<>(List.of(
                                new Predicate(NUM_H, new int[]{Argument.variable(0), Argument.variable(1)}),
                                new Predicate(NUM_H, new int[]{Argument.variable(1), Argument.variable(0)})
                        )),
                        new ArrayList<>(List.of(
                                new Predicate(NUM_H, new int[]{Argument.variable(1), Argument.variable(0)}),
                                new Predicate(NUM_H, new int[]{Argument.variable(0), Argument.variable(1)})
                        ))
                },
                new List[]{
                        new ArrayList<>(List.of(
                                new Predicate(NUM_H, new int[]{Argument.variable(0), Argument.variable(2)}),
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.variable(1)}),
                                new Predicate(NUM_Q, new int[]{Argument.variable(1), Argument.variable(2)})
                        )),
                        new ArrayList<>(List.of(
                                new Predicate(NUM_H, new int[]{Argument.variable(1), Argument.variable(0)}),
                                new Predicate(NUM_Q, new int[]{Argument.variable(2), Argument.variable(0)}),
                                new Predicate(NUM_P, new int[]{Argument.variable(1), Argument.variable(2)})
                        ))
                }
        };
        int failed = 0;
        for (int i = 0; i < rule_pairs.length; i++) {
            List<Predicate> rule1 = rule_pairs[i][0];
            List<Predicate> rule2 = rule_pairs[i][1];
            assertEquals(rule_pair_strs[i][0], rule2String(rule1));
            assertEquals(rule_pair_strs[i][1], rule2String(rule2));
            Fingerprint fp1 = new Fingerprint(rule1);
            Fingerprint fp2 = new Fingerprint(rule2);
            boolean fp1_eq_fp2 = fp1.equals(fp2);
            boolean fp2_eq_fp1 = fp2.equals(fp1);
            if (!fp1_eq_fp2 || !fp2_eq_fp1) {
                System.out.printf("Should match but not (@%d, %s, %s):\n", i, fp1_eq_fp2?"":"-x->", fp2_eq_fp1?"":"<-x-");
                System.out.println(rule2String(rule1));
                System.out.println(rule2String(rule2));
                failed++;
            }
        }
        int passed = rule_pairs.length - failed;
        System.out.printf("Passed: %.2f%%(%d/%d)\n", passed * 100.0 / rule_pairs.length, passed, rule_pairs.length);
        assertEquals(0, failed);
    }

    /* It is normal if this test failed */
    @Test
    public void testNotDupFromStructure() {
        String[][] rule_pair_strs = new String[][]{
                new String[]{   // If no independent fragment is introduced in the search, this will not happen in real world cases
                        "h(X0):-h(X1)",
                        "h(X0):-h(X1),h(X2)"
                },
                new String[]{
                        "h(X0):-p(X0,X1),q(X1,c)",
                        "h(X0):-p(X0,X1),q(c,X1)"
                },
                new String[]{
                        "p(X0,X1):-q(X0,X0),q(?,X1)",
                        "p(X0,X1):-q(X0,X1),q(?,X0)",
                },
                new String[]{
                        "p(X0,X1):-q(X0,?),q(X2,X1),q(?,X2)",
                        "p(X0,X1):-q(X0,X2),q(?,X1),q(X2,?)"
                },
                new String[]{
                        "h(X0,X2):-p(X0,X1),q(X1,X2)",
                        "h(X1,X0):-q(X2,X0),p(X2,X1)"
                }
        };
        List<Predicate>[][] rule_pairs = new List[][]{
                new List[]{
                        new ArrayList<>(List.of(
                                new Predicate(NUM_H, new int[]{Argument.variable(0)}),
                                new Predicate(NUM_H, new int[]{Argument.variable(1)})
                        )),
                        new ArrayList<>(List.of(
                                new Predicate(NUM_H, new int[]{Argument.variable(0)}),
                                new Predicate(NUM_H, new int[]{Argument.variable(1)}),
                                new Predicate(NUM_H, new int[]{Argument.variable(2)})
                        ))
                },
                new List[] {
                        new ArrayList<>(List.of(
                                new Predicate(NUM_H, new int[]{Argument.variable(0)}),
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.variable(1)}),
                                new Predicate(NUM_Q, new int[]{Argument.variable(1), Argument.constant(NUM_C)})
                        )),
                        new ArrayList<>(List.of(
                                new Predicate(NUM_H, new int[]{Argument.variable(0)}),
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.variable(1)}),
                                new Predicate(NUM_Q, new int[]{Argument.constant(NUM_C), Argument.variable(1)})
                        ))
                },
                new List[]{
                        new ArrayList<>(List.of(
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.variable(1)}),
                                new Predicate(NUM_Q, new int[]{Argument.variable(0), Argument.variable(0)}),
                                new Predicate(NUM_Q, new int[]{Argument.EMPTY_VALUE, Argument.variable(1)})
                        )),
                        new ArrayList<>(List.of(
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.variable(1)}),
                                new Predicate(NUM_Q, new int[]{Argument.variable(0), Argument.variable(1)}),
                                new Predicate(NUM_Q, new int[]{Argument.EMPTY_VALUE, Argument.variable(0)})
                        ))
                },
                new List[]{
                        new ArrayList<>(List.of(
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.variable(1)}),
                                new Predicate(NUM_Q, new int[]{Argument.variable(0), Argument.EMPTY_VALUE}),
                                new Predicate(NUM_Q, new int[]{Argument.variable(2), Argument.variable(1)}),
                                new Predicate(NUM_Q, new int[]{Argument.EMPTY_VALUE, Argument.variable(2)})
                        )),
                        new ArrayList<>(List.of(
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.variable(1)}),
                                new Predicate(NUM_Q, new int[]{Argument.variable(0), Argument.variable(2)}),
                                new Predicate(NUM_Q, new int[]{Argument.EMPTY_VALUE, Argument.variable(1)}),
                                new Predicate(NUM_Q, new int[]{Argument.variable(2), Argument.EMPTY_VALUE})
                        ))
                },
                new List[]{
                        new ArrayList<>(List.of(
                                new Predicate(NUM_H, new int[]{Argument.variable(0), Argument.variable(2)}),
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.variable(1)}),
                                new Predicate(NUM_Q, new int[]{Argument.variable(1), Argument.variable(2)})
                        )),
                        new ArrayList<>(List.of(
                                new Predicate(NUM_H, new int[]{Argument.variable(1), Argument.variable(0)}),
                                new Predicate(NUM_Q, new int[]{Argument.variable(2), Argument.variable(0)}),
                                new Predicate(NUM_P, new int[]{Argument.variable(2), Argument.variable(1)})
                        ))
                }
        };
        int failed = 0;
        for (int i = 0; i < rule_pairs.length; i++) {
            List<Predicate> rule1 = rule_pairs[i][0];
            List<Predicate> rule2 = rule_pairs[i][1];
            assertEquals(rule_pair_strs[i][0], rule2String(rule1));
            assertEquals(rule_pair_strs[i][1], rule2String(rule2));
            Fingerprint fp1 = new Fingerprint(rule1);
            Fingerprint fp2 = new Fingerprint(rule2);
            boolean fp1_eq_fp2 = fp1.equals(fp2);
            boolean fp2_eq_fp1 = fp2.equals(fp1);
            if (fp1_eq_fp2 || fp2_eq_fp1) {
                System.out.printf("Should not but match (@%d, %s, %s):\n", i, fp1_eq_fp2?"->":"", fp2_eq_fp1?"<-":"");
                System.out.println(rule2String(rule1));
                System.out.println(rule2String(rule2));
                failed++;
            }
        }
        int passed = rule_pairs.length - failed;
        System.out.printf("Passed: %.2f%%(%d/%d)\n", passed * 100.0 / rule_pairs.length, passed, rule_pairs.length);
        assertEquals(0, failed);
    }

    @Test
    void testSpecOfFromStructure() {
        String[][] rule_pair_strs = new String[][]{
                new String[]{
                        "p(X0,?):-q(?,X0)",
                        "p(X0,X1):-q(X1,X0)"
                },
                new String[]{
                        "p(X0,?):-q(?,X0)",
                        "p(X0,?):-q(X1,X0),p(X1,?)"
                },
                new String[]{
                        "p(X0,?):-q(?,X0)",
                        "p(X0,?):-q(c,X0)"
                }
        };
        List<Predicate>[][] rule_pairs = new List[][]{
                new List[] {
                        new ArrayList<>(List.of(
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.EMPTY_VALUE}),
                                new Predicate(NUM_Q, new int[]{Argument.EMPTY_VALUE, Argument.variable(0)})
                        )),
                        new ArrayList<>(List.of(
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.variable(1)}),
                                new Predicate(NUM_Q, new int[]{Argument.variable(1), Argument.variable(0)})
                        ))
                },
                new List[] {
                        new ArrayList<>(List.of(
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.EMPTY_VALUE}),
                                new Predicate(NUM_Q, new int[]{Argument.EMPTY_VALUE, Argument.variable(0)})
                        )),
                        new ArrayList<>(List.of(
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.EMPTY_VALUE}),
                                new Predicate(NUM_Q, new int[]{Argument.variable(1), Argument.variable(0)}),
                                new Predicate(NUM_P, new int[]{Argument.variable(1), Argument.EMPTY_VALUE})
                        ))
                },
                new List[] {
                        new ArrayList<>(List.of(
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.EMPTY_VALUE}),
                                new Predicate(NUM_Q, new int[]{Argument.EMPTY_VALUE, Argument.variable(0)})
                        )),
                        new ArrayList<>(List.of(
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.EMPTY_VALUE}),
                                new Predicate(NUM_Q, new int[]{Argument.constant(NUM_C), Argument.variable(0)})
                        )),
                },
        };
        int failed = 0;
        for (int i = 0; i < rule_pairs.length; i++) {
            List<Predicate> rule1 = rule_pairs[i][0];
            List<Predicate> rule2 = rule_pairs[i][1];
            assertEquals(rule_pair_strs[i][0], rule2String(rule1));
            assertEquals(rule_pair_strs[i][1], rule2String(rule2));
            Fingerprint fp1 = new Fingerprint(rule1);
            Fingerprint fp2 = new Fingerprint(rule2);
            if (!fp1.generalizationOf(fp2)) {
                System.out.printf("Should be specialization but not (@%d):\n", i);
                System.out.println(rule2String(rule1));
                System.out.println(rule2String(rule2));
                failed++;
            }
        }
        int passed = rule_pairs.length - failed;
        System.out.printf("Passed: %.2f%%(%d/%d)\n", passed * 100.0 / rule_pairs.length, passed, rule_pairs.length);
        assertEquals(0, failed);
    }

    @Test
    void testNotSpecOfFormStructure() {
        String[][] rule_pair_strs = new String[][]{
                new String[]{
                        "p(X0,X1):-q(X0,X1)",
                        "p(X0,X1):-q(X0,?),q(?,X1)"
                },
                new String[] {
                        "p(X0,?):-p(X0,X0)",
                        "p(X0,X1):-p(X1,X0)"
                },
                new String[] {
                        "p(X0,X0):-",
                        "p(X0,X1):-p(X1,X0)"
                }
        };
        List<Predicate>[][] rule_pairs = new List[][]{
                new List[]{
                        new ArrayList<>(List.of(
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.variable(1)}),
                                new Predicate(NUM_Q, new int[]{Argument.variable(0), Argument.variable(1)})
                        )),
                        new ArrayList<>(List.of(
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.variable(1)}),
                                new Predicate(NUM_Q, new int[]{Argument.variable(0), Argument.EMPTY_VALUE}),
                                new Predicate(NUM_Q, new int[]{Argument.EMPTY_VALUE, Argument.variable(1)})
                        ))
                },
                new List[]{
                        new ArrayList<>(List.of(
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.EMPTY_VALUE}),
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.variable(0)})
                        )),
                        new ArrayList<>(List.of(
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.variable(1)}),
                                new Predicate(NUM_P, new int[]{Argument.variable(1), Argument.variable(0)})
                        ))
                },
                new List[]{
                        new ArrayList<>(List.of(
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.variable(0)})
                        )),
                        new ArrayList<>(List.of(
                                new Predicate(NUM_P, new int[]{Argument.variable(0), Argument.variable(1)}),
                                new Predicate(NUM_P, new int[]{Argument.variable(1), Argument.variable(0)})
                        ))
                },
        };
        int failed = 0;
        for (int i = 0; i < rule_pairs.length; i++) {
            List<Predicate> rule1 = rule_pairs[i][0];
            List<Predicate> rule2 = rule_pairs[i][1];
            assertEquals(rule_pair_strs[i][0], rule2String(rule1));
            assertEquals(rule_pair_strs[i][1], rule2String(rule2));
            Fingerprint fp1 = new Fingerprint(rule1);
            Fingerprint fp2 = new Fingerprint(rule2);
            if (fp1.generalizationOf(fp2)) {
                System.out.printf("Should not be specialization but is (@%d):\n", i);
                System.out.println(rule2String(rule1));
                System.out.println(rule2String(rule2));
                failed++;
            }
        }
        int passed = rule_pairs.length - failed;
        System.out.printf("Passed: %.2f%%(%d/%d)\n", passed * 100.0 / rule_pairs.length, passed, rule_pairs.length);
        assertEquals(0, failed);
    }

    @Test
    void testDupFromRuleString() {
        int failed = 0;
        int total_cases = rule_pairs_dup_positive.size();
        for (int i = 0; i < total_cases; i++) {
            List<Predicate> rule1 = rule_pairs_dup_positive.get(i)[0];
            List<Predicate> rule2 = rule_pairs_dup_positive.get(i)[1];
            Fingerprint fp1 = new Fingerprint(rule1);
            Fingerprint fp2 = new Fingerprint(rule2);
            boolean fp1_eq_fp2 = fp1.equals(fp2);
            boolean fp2_eq_fp1 = fp2.equals(fp1);
            if (!fp1_eq_fp2 || !fp2_eq_fp1) {
                System.out.printf("Should match but not (@%d, %s, %s):\n", i, fp1_eq_fp2?"":"-x->", fp2_eq_fp1?"":"<-x-");
                System.out.println(rule2String(rule1));
                System.out.println(rule2String(rule2));
                failed++;
            }
        }
        int passed = total_cases - failed;
        System.out.printf("Passed: %.2f%%(%d/%d)\n", passed * 100.0 / total_cases, passed, total_cases);
        assertEquals(0, failed);
    }

    /* It is normal if this test failed */
    @Test
    void testNotDupFromRuleString() {
        int failed = 0;
        int total_cases = rule_pairs_dup_negative.size();
        for (int i = 0; i < total_cases; i++) {
            List<Predicate> rule1 = rule_pairs_dup_negative.get(i)[0];
            List<Predicate> rule2 = rule_pairs_dup_negative.get(i)[1];
            Fingerprint fp1 = new Fingerprint(rule1);
            Fingerprint fp2 = new Fingerprint(rule2);
            boolean fp1_eq_fp2 = fp1.equals(fp2);
            boolean fp2_eq_fp1 = fp2.equals(fp1);
            if (fp1_eq_fp2 || fp2_eq_fp1) {
                System.out.printf("Should not but match (@%d, %s, %s):\n", i, fp1_eq_fp2?"->":"", fp2_eq_fp1?"<-":"");
                System.out.println(rule2String(rule1));
                System.out.println(rule2String(rule2));
                failed++;
            }
        }
        int passed = total_cases - failed;
        System.out.printf("Passed: %.2f%%(%d/%d)\n", passed * 100.0 / total_cases, passed, total_cases);
        assertEquals(0, failed);
    }

    @Test
    void testSpecOfFromRuleString() {
        int failed = 0;
        int total_cases = rule_pairs_spec_positive.size();
        for (int i = 0; i < total_cases; i++) {
            List<Predicate> rule1 = rule_pairs_spec_positive.get(i)[0];
            List<Predicate> rule2 = rule_pairs_spec_positive.get(i)[1];
            Fingerprint fp1 = new Fingerprint(rule1);
            Fingerprint fp2 = new Fingerprint(rule2);
            if (!fp1.generalizationOf(fp2)) {
                System.out.printf("Should be specialization but not (@%d):\n", i);
                System.out.println(rule2String(rule1));
                System.out.println(rule2String(rule2));
                failed++;
            }
        }
        int passed = total_cases - failed;
        System.out.printf("Passed: %.2f%%(%d/%d)\n", passed * 100.0 / total_cases, passed, total_cases);
        assertEquals(0, failed);
    }

    /* It is normal if this test failed */
    @Test
    void testNotSpecOfFromRuleString() {
        int failed = 0;
        int total_cases = rule_pairs_spec_negative.size();
        for (int i = 0; i < total_cases; i++) {
            List<Predicate> rule1 = rule_pairs_spec_negative.get(i)[0];
            List<Predicate> rule2 = rule_pairs_spec_negative.get(i)[1];
            Fingerprint fp1 = new Fingerprint(rule1);
            Fingerprint fp2 = new Fingerprint(rule2);
            if (fp1.generalizationOf(fp2)) {
                System.out.printf("Should not be specialization but is (@%d):\n", i);
                System.out.println(rule2String(rule1));
                System.out.println(rule2String(rule2));
                failed++;
            }
        }
        int passed = total_cases - failed;
        System.out.printf("Passed: %.2f%%(%d/%d)\n", passed * 100.0 / total_cases, passed, total_cases);
        assertEquals(0, failed);
    }

    @Test
    void testRules() {
        String[] rule_strs = new String[]{
                "family(?,X0,X1):-father(X0,X1),isMale(X1)",
                "family(X1,?,X0):-father(?,X0),mother(X1,?),isMale(X0)",
                "family(X1,X0,?):-father(X0,X2),mother(X1,?),isMale(X2)",
                "family(X1,X0,X2):-father(X0,?),mother(X1,?),isMale(X2)",
                "family(X1,X0,X2):-father(X0,X2),mother(X1,?)"
        };
        NumerationMap map = new NumerationMap();
        map.mapName("family");
        map.mapName("father");
        map.mapName("mother");
        map.mapName("isMale");

        Rule rule0 = new BareRule(map.name2Num("family"), 3, new HashSet<>(), new HashMap<>());
        rule0.cvt2Uvs2NewLv(map.name2Num("father"), 2, 0, 0, 1);
        rule0.cvt2Uvs2NewLv(map.name2Num("isMale"), 1, 0, 0, 2);
        rule0.cvt1Uv2ExtLv(1, 1, 1);
        assertEquals(rule_strs[0], rule0.toDumpString(map));

        Rule rule1 = new BareRule(map.name2Num("family"), 3, new HashSet<>(), new HashMap<>());
        rule1.cvt2Uvs2NewLv(map.name2Num("father"), 2, 1, 0, 2);
        rule1.cvt2Uvs2NewLv(map.name2Num("mother"), 2, 0, 0, 0);
        rule1.cvt1Uv2ExtLv(map.name2Num("isMale"), 1, 0, 0);
        assertEquals(rule_strs[1], rule1.toDumpString(map));

        Rule rule2 = new BareRule(map.name2Num("family"), 3, new HashSet<>(), new HashMap<>());
        rule2.cvt2Uvs2NewLv(map.name2Num("father"), 2, 0, 0, 1);
        rule2.cvt2Uvs2NewLv(map.name2Num("mother"), 2, 0, 0, 0);
        rule2.cvt2Uvs2NewLv(map.name2Num("isMale"), 1, 0, 1, 1);
        assertEquals(rule_strs[2], rule2.toDumpString(map));

        Rule rule3 = new BareRule(map.name2Num("family"), 3, new HashSet<>(), new HashMap<>());
        rule3.cvt2Uvs2NewLv(map.name2Num("father"), 2, 0, 0, 1);
        rule3.cvt2Uvs2NewLv(map.name2Num("mother"), 2, 0, 0, 0);
        rule3.cvt2Uvs2NewLv(map.name2Num("isMale"), 1, 0, 0, 2);
        assertEquals(rule_strs[3], rule3.toDumpString(map));

        Rule rule4 = new BareRule(map.name2Num("family"), 3, new HashSet<>(), new HashMap<>());
        rule4.cvt2Uvs2NewLv(map.name2Num("father"), 2, 0, 0, 1);
        rule4.cvt2Uvs2NewLv(map.name2Num("mother"), 2, 0, 0, 0);
        rule4.cvt2Uvs2NewLv(0, 2, 1, 1);
        assertEquals(rule_strs[4], rule4.toDumpString(map));

        Rule[] rules = new Rule[]{rule0, rule1, rule2, rule3, rule4};
        for (int i = 0; i < rules.length; i++) {
            for (int j = i + 1; j < rules.length; j++) {
                assertNotEquals(rules[i], rules[j]);
            }
        }
    }

    private static List<Predicate> parsedStructure2RuleStructure(List<ParsedPred> parsedStructure) {
        List<Predicate> structure = new ArrayList<>(parsedStructure.size());
        for (ParsedPred parsed_predicate: parsedStructure) {
            Predicate predicate = new Predicate(map.mapName(parsed_predicate.functor), parsed_predicate.args.length);
            for (int arg_idx = 0; arg_idx < predicate.arity(); arg_idx++) {
                ParsedArg parsed_arg = parsed_predicate.args[arg_idx];
                predicate.args[arg_idx] = (null == parsed_arg) ? Argument.EMPTY_VALUE : (
                        (null == parsed_arg.name) ? Argument.variable(parsed_arg.id) : Argument.constant(map.mapName(parsed_arg.name))
                );
            }
            structure.add(predicate);
        }
        return structure;
    }

    private static String rule2String(List<Predicate> rule) {
        StringBuilder builder = new StringBuilder();
        builder.append(rule.get(0).toString(map)).append(":-");
        if (1 < rule.size()) {
            builder.append(rule.get(1).toString(map));
            for (int i = 2; i < rule.size(); i++) {
                builder.append(',');
                builder.append(rule.get(i).toString(map));
            }
        }
        return builder.toString();
    }
}