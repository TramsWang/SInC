package sinc2.exp.hint;

import sinc2.common.Predicate;
import sinc2.kb.KbException;
import sinc2.kb.SimpleKb;
import sinc2.kb.SimpleRelation;
import sinc2.rule.*;
import sinc2.util.ArrayOperation;
import sinc2.util.MultiSet;
import sinc2.util.kb.NumerationMap;

import java.io.*;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ForkJoinPool;
import java.util.concurrent.TimeUnit;

/**
 * This class reads a hint template and a KB to find evaluation of instantiated rules.
 *
 * The structure of the template file contains n+2 lines:
 *   - The first two lines are the settings of the thresholds of "Fact Coverage" and "τ". The output rules must satisfy
 *     the restrictions (evaluations no smaller than the thresholds);
 *   - Each of the following lines is a template of rules, and the structure is:
 *     <Horn Rule Template>;[<Restrictions>]
 *     - Horn rule template complies the grammar of the following context-free-grammar (similar to Prolog):
 *
 *       line := template_rule;[restrictions]
 *       template_rule := predicate:-body
 *       body := ε | predicate | predicate,body
 *       predicate := pred_symbol(args)
 *       args := ε | variable | constant | variable,args | constant,args
 *       restrictions := ε | sym_tuple | sym_tuple,restrictions
 *       sym_tuple := (sym_list)
 *       sym_list := pred_symbol | pred_symbol,sym_list
 *
 *       A "variable" is defined by the following regular expression: [A-Z][a-zA-z0-9]*
 *       A "pred_symbol" and a "constant" are defined by the following regex: [a-z][a-zA-z0-9]*
 *       Note: in the current implementation, there should *NOT* be constants in the template (for performance
 *       consideration).
 *     - The restrictions are a list of predicate symbol tuples. Each tuple of predicate symbols mean they cannot be all
 *       instantiated to the same predicate symbol in the same rule.
 *       For example, let "(p, q, r)" be a restriction tuple, then the following instantiation of the three predicates
 *       are valid:
 *         - p = father, q = mother, r = gender
 *         - p = parent, q = parent, r = grandparent
 *       But the following is invalid:
 *         - p = father, q = father, r = father
 *       If we need to represent that p, q, and r are mutually different from each other, then we can use three pairs
 *       instead: [(p, q), (p, r), (q, r)].
 *
 * The output is a "rules_<KB>.tsv" file containing n+1 rows and 7 columns:
 *   - The first is the title row. Other rows are in the descent order of τ(r)
 *   - The columns are:
 *     1. Rule: An instance of the template, written as r.
 *     2. |r|
 *     3. E^+_r
 *     4. E^-_r
 *     5. Fact Coverage of r
 *     6. τ(r)
 *     7. δ(r)
 *
 * The following is an example "template.hint" file:
 *
 *   0.2
 *   0.8
 *   p(X,Y):-q(Y,X);[]
 *   p(X,Y):-q(X,Z),r(Z,Y);[(p,q),(p,r)]
 *
 * The following is an example output "rules_someKB.tsv" file:
 *
 *   rule	|r|	E+	E-	FC	τ	δ
 *   friend(X,Y):-friend(Y,X)	2	10	2	0.5	0.71	6
 *   grandparent(X,Y):-parent(X,Z),parent(Z,Y)	3	150	0	0.9375	0.98	147
 *
 * @since 2.0
 */
public class HinterMultiThread {
    /** A special value for the instantiation of the predicate symbols meaning it has not yet been instantiated */
    protected static final int UNDETERMINED = -1;

    /**
     * A structure that records a successfully instantiated rule and its score.
     */
    protected static class CollectedRuleInfo {
        final String rule;
        final double score;

        public CollectedRuleInfo(String rule, double score) {
            this.rule = rule;
            this.score = score;
        }
    }

    protected final String kbPath;
    protected final String kbName;
    protected final String hintFilePath;
    protected final Path outputFilePath;

    /** The target KB */
    protected HinterKb kb;
    /** The numeration map */
    protected NumerationMap numMap;
    /** The numerations of the relations in the KB */
    protected int[] kbRelationNums;
    /** The arities of the relations in the KB (correspond to the relation numeration) */
    protected int[] kbRelationArities;
    /** "Fact Coverage" and "τ" */
    protected double factCoverageThreshold, compRatioThreshold;
    /** The list of collected rules and the evaluation details */
    protected List<CollectedRuleInfo> collectedRuleInfos = new ArrayList<>();
    /** Threads */
    protected ForkJoinPool threadPool;

    public static Path getRulesFilePath(String hintFilePath, String kbName) {
        return Paths.get(
                new File(hintFilePath).toPath().toAbsolutePath().getParent().toString(),
                String.format("rules_%s.tsv", kbName)
        );
    }

    public static Path getLogFilePath(String hintFilePath, String kbName) {
        return Paths.get(
                new File(hintFilePath).toPath().toAbsolutePath().getParent().toString(),
                String.format("rules_%s.log", kbName)
        );
    }

    /**
     * Create a Hinter object.
     *
     * @param kbPath         The path to the numerated KB
     * @param kbName         The name of the KB
     * @param hintFilePath   The path to the hint file
     * @param threads        The number of threads that runs the hinter
     */
    public HinterMultiThread(String kbPath, String kbName, String hintFilePath, int threads) throws FileNotFoundException {
        this.kbPath = kbPath;
        this.kbName = kbName;
        this.hintFilePath = hintFilePath;
        this.outputFilePath = getRulesFilePath(hintFilePath, kbName);
        this.threadPool = new ForkJoinPool(threads, ForkJoinPool.defaultForkJoinWorkerThreadFactory, null, true);
        System.setOut(new PrintStream(getLogFilePath(hintFilePath, kbName).toFile()));
    }

    /**
     * Use the hints to instantiate rules in the target KB
     *
     */
    public void run() throws ExperimentException {
        try {
            /* Load hint file */
            /* Read "Fact Coverage" and "τ" */
            BufferedReader reader = new BufferedReader(new FileReader(hintFilePath));
            try {
                factCoverageThreshold = Double.parseDouble(reader.readLine());
                Rule.MIN_FACT_COVERAGE = factCoverageThreshold;
            } catch (Exception e) {
                throw new ExperimentException("Missing fact coverage setting", e);
            }
            try {
                compRatioThreshold = Double.parseDouble(reader.readLine());
            } catch (Exception e) {
                throw new ExperimentException("Missing compression ratio setting", e);
            }

            /* Read templates and restrictions */
            String line;
            List<Hint> hints = new ArrayList<>();
            while (null != (line = reader.readLine())) {
                hints.add(new Hint(line, numMap));
            }

            /* Load KB */
            long time_start = System.currentTimeMillis();
            kb = new HinterKb(kbName, kbPath, factCoverageThreshold);
            numMap = new NumerationMap(Paths.get(kbPath, kbName).toString());
            kbRelationNums = new int[kb.totalRelations()];
            kbRelationArities = new int[kb.totalRelations()];
            int idx = 0;
            for (SimpleRelation relation: kb.getRelations()) {
                kbRelationNums[idx] = relation.id;
                kbRelationArities[idx] = relation.totalCols();
                idx++;
            }

            /* Instantiate templates */
            int total_covered_records = 0;
            int rules_idx = 0;
            for (int i = 0; i < kbRelationNums.length; i++) {
                /* Create the initial rule */
                int head_functor = kbRelationNums[i];
                int head_arity = kbRelationArities[i];

                /* Create a relation for all positive entailments */
                SimpleRelation pos_entails = new SimpleRelation(
                        "entailments", head_functor, kb.getRelation(head_functor).getAllRows().clone()
                );

                /* Try each template */
                Map<MultiSet<Integer>, Set<Fingerprint>> tabu_set = new ConcurrentHashMap<>();
                for (int j = 0; j < hints.size(); j++) {
                    System.out.printf("Try hint (%d/%d)\n", j, hints.size());
                    Hint hint = hints.get(j);
                    if (head_arity != hint.functorArities[0]) {
                        continue;
                    }
                    Set<Fingerprint> fingerprint_cache = Collections.synchronizedSet(new HashSet<>());
                    HinterRule rule = new HinterRule(head_functor, head_arity, fingerprint_cache, tabu_set, kb);
                    int totalFunctors = hint.functorRestrictionCounterLink.length;
                    int[] template_functor_instantiation = ArrayOperation.initArrayWithValue(totalFunctors, UNDETERMINED);
                    int[] restriction_counters = ArrayOperation.initArrayWithValue(hint.restrictions.size(), 1);
                    int[] restriction_targets = ArrayOperation.initArrayWithValue(hint.restrictions.size(), -1);

                    /* Set head functor */
                    template_functor_instantiation[0] = head_functor;

                    /* Set restrictions */
                    threadPool.submit(() -> specializeByOperations(
                            rule, hint.operations, 0, template_functor_instantiation, hint.functorArities,
                            hint.functorRestrictionCounterLink, hint.restrictionCounterBounds, restriction_counters, restriction_targets,
                            pos_entails
                    ));
                    while (!threadPool.awaitQuiescence(1, TimeUnit.SECONDS));   // wait for all DFS tasks to finish
                    System.out.println("Rules from the template:");
                    for (int k = rules_idx; k < collectedRuleInfos.size(); k++) {
                        System.out.println(collectedRuleInfos.get(k).rule);
                    }
                    rules_idx = collectedRuleInfos.size();
                }

                System.out.printf("Relation Done (%d/%d): %s\n", i+1, kbRelationNums.length, kb.getRelation(head_functor).name);
                int covered_records = pos_entails.totalEntailedRecords();
                int total_records = kb.getRelation(head_functor).totalRows();
                total_covered_records += covered_records;
                System.out.printf("Coverage: %.2f%% (%d/%d)\n", covered_records * 100.0 / total_records, covered_records, total_records);
                System.out.flush();
            }
            int total_records = kb.totalRecords();
            long time_done = System.currentTimeMillis();
            System.out.printf("Total Coverage: %.2f%% (%d/%d)\n", total_covered_records * 100.0 / total_records, total_covered_records, total_records);
            System.out.printf("Total Time: %d (ms)\n", time_done - time_start);

            /* Dump the results */
            PrintWriter writer = new PrintWriter(outputFilePath.toFile());
            writer.printf("rule\t|r|\tE+\tE-\tFC\tτ\tδ\n");
            collectedRuleInfos.sort(Comparator.comparingDouble((CollectedRuleInfo e) -> e.score).reversed());
            for (CollectedRuleInfo rule_info: collectedRuleInfos) {
                writer.println(rule_info.rule);
            }
            writer.close();
        } catch (IOException | RuleParseException e) {
            throw new ExperimentException(e);
        }
    }

    /**
     * DFS search for specializing a rule according to a list of specialization operations. Functors in the templates
     * should be instantiated by real functors in the KB. Record all template instances that satisfies the requirements
     * of the hint file.
     *
     * @param rule                           The rule to be specialized
     * @param operations                     The list of specialization operations
     * @param oprStartIdx                    The start index of the operation list
     * @param templateFunctorInstantiation   The instantiation of the template functors
     * @param templateFunctorArities         The arities of the template functors
     * @param functorRestrictionCounterLinks Template functor to restriction counter index
     * @param restrictionCounterBounds       The bounds for the restriction counters
     * @param restrictionCounters            The restriction counters
     * @param restrictionTargets             The comparing target functors that determines the validation of restrictions
     * @param positiveEntailments            The relation object that holds all the entailed records in one relation
     */
    protected void specializeByOperations(
            HinterRule rule, List<SpecOpr> operations, int oprStartIdx,
            int[] templateFunctorInstantiation, int[] templateFunctorArities,
            int[][] functorRestrictionCounterLinks, int[] restrictionCounterBounds, int[] restrictionCounters,
            int[] restrictionTargets, SimpleRelation positiveEntailments
    ) {
        for (int opr_idx = oprStartIdx; opr_idx < operations.size(); opr_idx++) {
            SpecOpr opr = operations.get(opr_idx);
            rule.updateCacheIndices();
            switch (opr.getSpecCase()) {
                case CASE1:
                case CASE3:
                case CASE5:
                    if (UpdateStatus.NORMAL != opr.specialize(rule)) {
                        return;
                    }
                    break;
                case CASE2:
                    SpecOprCase2 opr_case2 = (SpecOprCase2) opr;
                    if (UNDETERMINED == templateFunctorInstantiation[opr_case2.functor]) {
                        for (int i = 0; i < kbRelationNums.length; i++) {
                            /* Try every possible functor with the same arity of the template one */
                            if (templateFunctorArities[opr_case2.functor] != kbRelationArities[i]) {  // Check whether arity matches
                                continue;
                            }
                            final int new_functor = kbRelationNums[i];
                            boolean valid = true;
                            int[] new_restriction_counters = restrictionCounters.clone();
                            int[] new_restriction_targets = restrictionTargets.clone();
                            for (int counter_idx: functorRestrictionCounterLinks[opr_case2.functor]) {  // Add counters
                                if (new_restriction_targets[counter_idx] == new_functor) {
                                    new_restriction_counters[counter_idx]++;
                                } else {
                                    new_restriction_targets[counter_idx] = new_functor;
                                }
                                if (restrictionCounterBounds[counter_idx] == new_restriction_counters[counter_idx]) {
                                    valid = false;
                                    break;
                                }
                            }
                            if (valid) {
                                /* Run the branch in a new thread */
                                final int new_opr_idx = opr_idx + 1;
                                threadPool.submit(() -> {
                                    /* Copy arguments */
                                    HinterRule specialized_rule = rule.clone();
                                    int[] new_template_functor_instantiation = templateFunctorInstantiation.clone();
                                    new_template_functor_instantiation[opr_case2.functor] = new_functor;

                                    /* DFS to the next step */
                                    if (UpdateStatus.NORMAL == specialized_rule.cvt1Uv2ExtLv(   // Can't use the method "specialize" of "SpecOpr",
                                            new_functor,                                        // because the functor in the objects denotes the
                                            templateFunctorArities[opr_case2.functor],          // template index instead of the real numeration of the functors
                                            opr_case2.argIdx, opr_case2.varId
                                    )) {
                                        specializeByOperations(
                                                specialized_rule, operations, new_opr_idx,
                                                new_template_functor_instantiation, templateFunctorArities,
                                                functorRestrictionCounterLinks, restrictionCounterBounds,
                                                new_restriction_counters, new_restriction_targets, positiveEntailments
                                        );
                                    }
                                });
                            }
                        }

                        /* If it goes here, all following operations are done in the recursive call above, just return */
                        return;
                    } else {
                        if (UpdateStatus.NORMAL != rule.cvt1Uv2ExtLv(
                                templateFunctorInstantiation[opr_case2.functor], templateFunctorArities[opr_case2.functor],
                                opr_case2.argIdx, opr_case2.varId
                        )) {
                            return;
                        }
                    }
                    break;
                case CASE4:
                    SpecOprCase4 opr_case4 = (SpecOprCase4) opr;
                    if (UNDETERMINED == templateFunctorInstantiation[opr_case4.functor]) {
                        for (int i = 0; i < kbRelationNums.length; i++) {
                            /* Try every possible functor with the same arity of the template one */
                            if (templateFunctorArities[opr_case4.functor] != kbRelationArities[i]) {  // Check whether arity matches
                                continue;
                            }
                            final int new_functor = kbRelationNums[i];
                            boolean valid = true;
                            int[] new_restriction_counters = restrictionCounters.clone();
                            int[] new_restriction_targets = restrictionTargets.clone();
                            for (int counter_idx: functorRestrictionCounterLinks[opr_case4.functor]) {  // Add counters
                                if (new_restriction_targets[counter_idx] == new_functor) {
                                    new_restriction_counters[counter_idx]++;
                                } else {
                                    new_restriction_targets[counter_idx] = new_functor;
                                }
                                if (restrictionCounterBounds[counter_idx] == new_restriction_counters[counter_idx]) {
                                    valid = false;
                                    break;
                                }
                            }
                            if (valid) {
                                /* Run the branch in a new thread */
                                final int new_opr_idx = opr_idx + 1;
                                threadPool.submit(() -> {
                                    /* Copy arguments */
                                    HinterRule specialized_rule = rule.clone();
                                    int[] new_template_functor_instantiation = templateFunctorInstantiation.clone();
                                    new_template_functor_instantiation[opr_case4.functor] = new_functor;

                                    /* DFS to the next step */
                                    if (UpdateStatus.NORMAL == specialized_rule.cvt2Uvs2NewLv(  // Can't use the method "specialize" of "SpecOpr",
                                            new_functor,                                        // because the functor in the objects denotes the
                                            templateFunctorArities[opr_case4.functor],          // template index instead of the real numeration of the functors
                                            opr_case4.argIdx1, opr_case4.predIdx2, opr_case4.argIdx2)
                                    ) {
                                        specializeByOperations(
                                                specialized_rule, operations, new_opr_idx,
                                                new_template_functor_instantiation, templateFunctorArities,
                                                functorRestrictionCounterLinks, restrictionCounterBounds,
                                                new_restriction_counters, new_restriction_targets, positiveEntailments
                                        );
                                    }
                                });
                            }
                        }

                        /* If it goes here, all following operations are done in the recursive call above, just return */
                        return;
                    } else {
                        if (UpdateStatus.NORMAL != rule.cvt2Uvs2NewLv(
                                templateFunctorInstantiation[opr_case4.functor], templateFunctorArities[opr_case4.functor],
                                opr_case4.argIdx1, opr_case4.predIdx2, opr_case4.argIdx2
                        )) {
                            return;
                        }
                    }
                    break;
            }
        }

        /* Specialization finished, check rule quality */
        Eval eval = rule.getEval();
        if (compRatioThreshold <= eval.value(EvalMetric.CompressionRatio)) {
            String rule_info = String.format("%s\t%d\t%d\t%d\t%.2f\t%.2f\t%d",
                    rule.toDumpString(numMap), rule.length(), (int) eval.getPosEtls(), (int) eval.getNegEtls(),
                    eval.getPosEtls()/kb.getRelation(templateFunctorInstantiation[0]).totalRows()*100,
                    eval.value(EvalMetric.CompressionRatio), (int) eval.value(EvalMetric.CompressionCapacity)
            );
            synchronized (collectedRuleInfos) {
                collectedRuleInfos.add(new CollectedRuleInfo(rule_info, eval.value(EvalMetric.CompressionRatio)));
            }
            try {
                synchronized (positiveEntailments) {
                    rule.extractPositiveEntailments(positiveEntailments);
                }
            } catch (KbException e) {
                System.err.println("Error occurs during extracting positive entailments");
                e.printStackTrace();
            }
        }
    }

    protected String rule2String(List<Predicate> structure) {
        StringBuilder builder = new StringBuilder();
        builder.append(structure.get(0).toString()).append(":-");
        if (1 < structure.size()) {
            builder.append(structure.get(1).toString());
            for (int i = 2; i < structure.size(); i++) {
                builder.append(',');
                builder.append(structure.get(i).toString());
            }
        }
        return builder.toString();
    }

    /**
     * Usage: <Path to the KB> <KB name> <Path to the hint file>
     * Example: datasets/ UMLS template.hint
     */
    public static void main(String[] args) throws FileNotFoundException, ExperimentException {
        if (4 != args.length) {
            System.out.println("Usage: <Path to the KB> <KB name> <Path to the hint file> <threads>");
        }
        HinterMultiThread hinter = new HinterMultiThread(args[0], args[1], args[2], Integer.parseInt(args[3]));
        hinter.run();
    }
}
