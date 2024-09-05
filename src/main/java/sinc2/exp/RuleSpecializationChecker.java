package sinc2.exp;

import sinc2.common.ParsedArg;
import sinc2.common.ParsedPred;
import sinc2.rule.Rule;
import sinc2.rule.RuleParseException;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.*;

public class RuleSpecializationChecker {

    /**
     * Split rule pairs in the input file to two files. The input file contains pairs of rules that will be tested for
     * specialization. Each line contains a string of a single rule. Rules in line 1 & 2 are a pair, so do 3 & 4, 5 & 6, ...
     * If the first in the pair can be specialized to the second, the two rules will be collected in one file (positive)
     * orders reserved, and those are not will be in another (negative).
     */
    public static void main(String[] args) throws IOException, RuleParseException {
        if (1 > args.length) {
            System.err.println("Usage: <Path to file that contains pairs of rules>");
            return;
        }
        BufferedReader reader = new BufferedReader(new FileReader(args[0]));
        PrintWriter positive_writer = new PrintWriter("spec_rules_positive.txt");
        PrintWriter negative_writer = new PrintWriter("spec_rules_negative.txt");
        int tp = 0;
        int fp = 0;
        while (true) {
            String rule1_str = reader.readLine();
            String rule2_str = reader.readLine();
            if (null == rule1_str) {
                break;
            }
            List<ParsedPred> rule1 = Rule.parseStructure(rule1_str);
            List<ParsedPred> rule2 = Rule.parseStructure(rule2_str);
            if (specializationOf(rule1, rule2)) {
                /* True positive */
                positive_writer.println(rule1_str);
                positive_writer.println(rule2_str);
                tp++;
            } else {
                /* False positive */
                negative_writer.println(rule1_str);
                negative_writer.println(rule2_str);
                fp++;
            }
        }
        reader.close();
        positive_writer.close();
        negative_writer.close();
        System.out.printf("True Pos: %d\nFalse Pos: %d\n", tp, fp);
    }

    /**
     * Check whether "specialization" can be specialized from "original". Variable IDs in the rules should range from 0
     * to n-1, where n is the number of LVs in the rule.
     */
    static boolean specializationOf(List<ParsedPred> original, List<ParsedPred> specialization) {
        int original_length = ruleLength(original);
        int spec_length = ruleLength(specialization);
        if (original_length > spec_length) {
            return false;
        }
        
        Map<String, Integer> functor2ArityMap = new HashMap<>();
        Set<String> constants = new HashSet<>();
        for (ParsedPred pred: specialization) {
            functor2ArityMap.put(pred.functor, pred.args.length);
            for (ParsedArg arg: pred.args) {
                if (null != arg && null != arg.name) {
                    constants.add(arg.name);
                }
            }
        }
        return specializationOfHandler(original, original_length, specialization, spec_length, functor2ArityMap, constants);
    }
    
    static boolean specializationOfHandler(
            List<ParsedPred> original, int originalLength, List<ParsedPred> specialization, int specializationLength,
            Map<String, Integer> functor2ArityMap, Set<String> constants
    ) {
        if (originalLength == specializationLength) {
            return RuleDuplicationChecker.matchRules(original, specialization);
        }
        List<List<ParsedPred>> extensions = findExtensions(original, functor2ArityMap, constants);
        for (List<ParsedPred> extension : extensions) {
            if (specializationOfHandler(
                    extension, originalLength + 1, specialization, specializationLength, functor2ArityMap, constants
            )) {
                return true;
            }
        }
        return false;
    }

    static int ruleLength(List<ParsedPred> rule) {
        int constants = 0;
        int var_args = 0;
        int lvs = 0;
        for (ParsedPred predicate: rule) {
            for (ParsedArg argument: predicate.args) {
                if (null != argument) {
                    if (null == argument.name) {
                        var_args++;
                        lvs = Math.max(lvs, argument.id);
                    } else {
                        constants++;
                    }
                }
            }
        }
        return constants + var_args - lvs;
    }

    static List<List<ParsedPred>> findExtensions(final List<ParsedPred> rule, Map<String, Integer> functor2ArityMap, Set<String> constants) {
        /* Find all empty arguments */
        class ArgPos {
            public final int predIdx;
            public final int argIdx;

            public ArgPos(int predIdx, int argIdx) {
                this.predIdx = predIdx;
                this.argIdx = argIdx;
            }
        }
        List<ArgPos> vacant_list = new ArrayList<>();    // List of empty arguments
        Set<Integer> used_LVs = new HashSet<>();
        int max_var_id = 0;
        for (int pred_idx = 0; pred_idx < rule.size(); pred_idx++) {
            final ParsedPred pred_info = rule.get(pred_idx);
            for (int arg_idx = 0; arg_idx < pred_info.args.length; arg_idx++) {
                ParsedArg argument = pred_info.args[arg_idx];
                if (null == argument) {
                    vacant_list.add(new ArgPos(pred_idx, arg_idx));
                } else if (null == argument.name) {
                    used_LVs.add(argument.id);
                    max_var_id = Math.max(max_var_id, argument.id);
                }
            }
        }

        /* Case 1 and 2 */
        List<List<ParsedPred>> extensions = new ArrayList<>();
        for (int var_id: used_LVs) {
            /* Case 1 */
            for (ArgPos vacant: vacant_list) {
                List<ParsedPred> new_rule = copyRule(rule);
                new_rule.get(vacant.predIdx).args[vacant.argIdx] = ParsedArg.variable(var_id);
                extensions.add(new_rule);
            }

            for (Map.Entry<String, Integer> entry: functor2ArityMap.entrySet()) {
                /* Case 2 */
                final String functor = entry.getKey();
                final int arity = entry.getValue();
                for (int arg_idx = 0; arg_idx < arity; arg_idx++) {
                    List<ParsedPred> new_rule = copyRule(rule);
                    ParsedPred predicate = new ParsedPred(functor, arity);
                    predicate.args[arg_idx] = ParsedArg.variable(var_id);
                    new_rule.add(predicate);
                    extensions.add(new_rule);
                }
            }
        }

        int new_var_id = max_var_id+1;
        for (int i = 0; i < vacant_list.size(); i++) {
            /* Find the first empty arguments */
            final ArgPos first_vacant = vacant_list.get(i);

            /* Case 5 */
            for (String const_symbol: constants) {
                List<ParsedPred> new_rule = copyRule(rule);
                new_rule.get(first_vacant.predIdx).args[first_vacant.argIdx] = ParsedArg.constant(const_symbol);
                extensions.add(new_rule);
            }

            /* Case 3 and 4 */
            for (int j = i + 1; j < vacant_list.size(); j++) {
                /* Case 3 */
                final ArgPos second_vacant = vacant_list.get(j);
                List<ParsedPred> new_rule = copyRule(rule);
                new_rule.get(first_vacant.predIdx).args[first_vacant.argIdx] = ParsedArg.variable(new_var_id);
                new_rule.get(second_vacant.predIdx).args[second_vacant.argIdx] = ParsedArg.variable(new_var_id);
                extensions.add(new_rule);
            }
            for (Map.Entry<String, Integer> entry: functor2ArityMap.entrySet()) {
                /* Case 4 */
                final String functor = entry.getKey();
                final int arity = entry.getValue();
                for (int arg_idx = 0; arg_idx < arity; arg_idx++) {
                    List<ParsedPred> new_rule = copyRule(rule);
                    new_rule.get(first_vacant.predIdx).args[first_vacant.argIdx] = ParsedArg.variable(new_var_id);
                    ParsedPred predicate = new ParsedPred(functor, arity);
                    predicate.args[arg_idx] = ParsedArg.variable(new_var_id);
                    new_rule.add(predicate);
                    extensions.add(new_rule);
                }
            }
        }
        return extensions;
    }

    static List<ParsedPred> copyRule(List<ParsedPred> rule) {
        List<ParsedPred> copied = new ArrayList<>(rule.size());
        for (ParsedPred predicate: rule) {
            copied.add(new ParsedPred(predicate));
        }
        return copied;
    }

}
