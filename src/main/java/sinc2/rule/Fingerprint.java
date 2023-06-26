package sinc2.rule;

import sinc2.common.ArgIndicator;
import sinc2.common.Argument;
import sinc2.common.Predicate;
import sinc2.util.MultiSet;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;

/**
 * The fingerprint class that quickly tells whether two rules are equivalent.
 *
 * Note: The fingerprint structure may incorrectly tell two rules are identical. For instance:
 *
 *   1.1 p(X,Y) :- f(X,X), f(?,Y)
 *   1.2 p(X,Y) :- f(X,Y), f(?,X)
 *
 *   2.1 p(X,Y) :- f(X,?), f(Z,Y), f(?,Z)
 *   2.2 p(X,Y) :- f(X,Z), f(?,Y), f(Z,?)
 *
 * The fingerprint structure may also incorrectly tell one rule is the specialization of another. For more examples,
 * please refer to the test class.
 *
 * @since 1.0
 */
public class Fingerprint {

    /**
     * Arguments in this class are equivalent classes. Each equivalent class is corresponding to some argument in the
     * original rule.
     */
    static class PredicateWithClass {
        final int functor;
        final MultiSet<ArgIndicator>[] classArgs;

        public PredicateWithClass(int functor, int arity) {
            this.functor = functor;
            this.classArgs = new MultiSet[arity];
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;
            PredicateWithClass that = (PredicateWithClass) o;
            return functor == that.functor && Arrays.equals(classArgs, that.classArgs);
        }

        @Override
        public int hashCode() {
            int result = Objects.hash(functor);
            result = 31 * result + Arrays.hashCode(classArgs);
            return result;
        }
    }

    /** The equivalence classes in this fingerprint */
    protected final MultiSet<MultiSet<ArgIndicator>> equivalenceClasses = new MultiSet<>();
    /** In this rule structure, each argument is the corresponding equivalence class */
    protected final List<PredicateWithClass> classedStructure = new ArrayList<>();
    /** The rule that generates the fingerprint */
    protected final List<Predicate> rule;

    /**
     * Construct the fingerprint with the structure of the rule.
     *
     * @param rule The rule structure (ordered list of predicates, where the head is the first)
     */
    public Fingerprint(List<Predicate> rule) {
        this.rule = rule;

        /* Count the number of LVs */
        /* Assumption: The IDs for the variables start from 0 and are continuous */
        int max_lv_id = -1;
        for (Predicate predicate: rule) {
            for (int argument: predicate.args) {
                if (Argument.isVariable(argument)) {
                    max_lv_id = Math.max(max_lv_id, Argument.decode(argument));
                }
            }
        }
        MultiSet<ArgIndicator>[] lv_equiv_classes = new MultiSet[max_lv_id+1];
        for (int i = 0; i < lv_equiv_classes.length; i++) {
            lv_equiv_classes[i] = new MultiSet<>();
        }

        /* Construct equivalence classes */
        for (Predicate predicate : rule) {
            PredicateWithClass pred_with_class = new PredicateWithClass(predicate.predSymbol, predicate.arity());
            for (int arg_idx = 0; arg_idx < predicate.arity(); arg_idx++) {
                int argument = predicate.args[arg_idx];
                if (Argument.isEmpty(argument)) {
                    MultiSet<ArgIndicator> eqc = new MultiSet<>();
                    eqc.add(ArgIndicator.getVariableIndicator(predicate.predSymbol, arg_idx));
                    equivalenceClasses.add(eqc);
                    pred_with_class.classArgs[arg_idx] = eqc;
                } else if (Argument.isVariable(argument)) {
                    int var_id = Argument.decode(argument);
                    MultiSet<ArgIndicator> eqc = lv_equiv_classes[var_id];
                    eqc.add(ArgIndicator.getVariableIndicator(predicate.predSymbol, arg_idx));
                    pred_with_class.classArgs[arg_idx] = eqc;
                } else {
                    int constant = Argument.decode(argument);
                    MultiSet<ArgIndicator> eqc = new MultiSet<>();
                    eqc.add(ArgIndicator.getVariableIndicator(predicate.predSymbol, arg_idx));
                    eqc.add(ArgIndicator.getConstantIndicator(constant));
                    equivalenceClasses.add(eqc);
                    pred_with_class.classArgs[arg_idx] = eqc;
                }
            }
            classedStructure.add(pred_with_class);
        }

        /* Add the equivalent classes for the LVs to the fingerprint */
        equivalenceClasses.addAll(lv_equiv_classes);
    }

    public MultiSet<MultiSet<ArgIndicator>> getEquivalenceClasses() {
        return equivalenceClasses;
    }

    public List<PredicateWithClass> getClassedStructure() {
        return classedStructure;
    }

    /**
     * Check for generalizations.
     *
     * @param another The fingerprint of another rule
     * @return Ture if the rule of this fingerprint is the generalization of the other.
     */
    public boolean generalizationOf(Fingerprint another) {
        PredicateWithClass head = classedStructure.get(0);
        PredicateWithClass another_head = another.classedStructure.get(0);
        if (!generalizationOf(head, another_head)) {
            return false;
        }
        if (2 == head.classArgs.length && (
                (head.classArgs[0] == head.classArgs[1] && another_head.classArgs[0] != another_head.classArgs[1]) ||
                        (head.classArgs[0] != head.classArgs[1] && another_head.classArgs[0] == another_head.classArgs[1])
        )) {
            return false;
        }
        for (int pred_idx = Rule.FIRST_BODY_PRED_IDX; pred_idx < classedStructure.size(); pred_idx++) {
            PredicateWithClass predicate = classedStructure.get(pred_idx);
            boolean found_specialization = false;
            for (int pred_idx_another = Rule.FIRST_BODY_PRED_IDX; pred_idx_another < another.classedStructure.size(); pred_idx_another++) {
                if (generalizationOf(predicate, another.classedStructure.get(pred_idx_another))) {
                    found_specialization = true;
                    break;
                }
            }
            if (!found_specialization) {
                return false;
            }
        }
        return true;
    }

    /**
     * Check whether "predicate" is the generalization of "specializedPredicate".
     */
    protected boolean generalizationOf(PredicateWithClass predicate, PredicateWithClass specializedPredicate) {
        if (predicate.functor != specializedPredicate.functor) {
            return false;
        }
        for (int arg_idx = 0; arg_idx < predicate.classArgs.length; arg_idx++) {
            if (!predicate.classArgs[arg_idx].subsetOf(specializedPredicate.classArgs[arg_idx])) {
                return false;
            }
        }
        return true;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        Fingerprint that = (Fingerprint) o;
        return Objects.equals(classedStructure.get(0), that.classedStructure.get(0)) &&
                Objects.equals(equivalenceClasses, that.equivalenceClasses);
    }

    @Override
    public int hashCode() {
        return Objects.hash(classedStructure.get(0), equivalenceClasses);
    }
}
