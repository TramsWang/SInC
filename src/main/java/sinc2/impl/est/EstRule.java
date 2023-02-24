package sinc2.impl.est;

import sinc2.common.ArgLocation;
import sinc2.common.Argument;
import sinc2.common.Predicate;
import sinc2.impl.base.CachedRule;
import sinc2.impl.base.CompliedBlock;
import sinc2.kb.SimpleKb;
import sinc2.kb.SimpleRelation;
import sinc2.rule.*;
import sinc2.util.ArrayOperation;
import sinc2.util.MultiSet;

import java.util.*;

/**
 * This rule is a specialization of cached rule that estimates the evaluation of each possible specialization.
 *
 * @since 2.1
 */
public class EstRule extends CachedRule {
    public EstRule(int headPredSymbol, int arity, Set<Fingerprint> fingerprintCache, Map<MultiSet<Integer>, Set<Fingerprint>> category2TabuSetMap, SimpleKb kb) {
        super(headPredSymbol, arity, fingerprintCache, category2TabuSetMap, kb);
    }

    public EstRule(List<Predicate> structure, Set<Fingerprint> fingerprintCache, Map<MultiSet<Integer>, Set<Fingerprint>> category2TabuSetMap, SimpleKb kb) {
        super(structure, fingerprintCache, category2TabuSetMap, kb);
    }

    public EstRule(EstRule another) {
        super(another);
    }

    @Override
    public EstRule clone() {
        return new EstRule(this);
    }

    public List<SpecOprWithScore> estimateSpecializations() {
        /* Gather values in columns */
        List<MultiSet<Integer>[]> column_values_in_pos_cache = new ArrayList<>();
        List<MultiSet<Integer>[]> column_values_in_all_cache = new ArrayList<>();
        boolean[][] vars_in_preds = new boolean[structure.size()][];
        for (int pred_idx = HEAD_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
            Predicate predicate = structure.get(pred_idx);
            MultiSet<Integer>[] arg_sets_pos = new MultiSet[predicate.arity()];
            MultiSet<Integer>[] arg_sets_all = new MultiSet[predicate.arity()];
            for (int i = 0; i < arg_sets_pos.length; i++) {
                arg_sets_pos[i] = new MultiSet<>();
                arg_sets_all[i] = new MultiSet<>();
            }
            column_values_in_pos_cache.add(arg_sets_pos);
            column_values_in_all_cache.add(arg_sets_all);
            boolean[] vars_in_pred = new boolean[usedLimitedVars()];
            for (int arg: predicate.args) {
                if (Argument.isVariable(arg)) {
                    vars_in_pred[Argument.decode(arg)] = true;
                }
            }
            vars_in_preds[pred_idx] = vars_in_pred;
        }
        for (List<CompliedBlock> cache_entry: posCache) {
            for (int pred_idx = HEAD_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
                CompliedBlock cb = cache_entry.get(pred_idx);
                MultiSet<Integer>[] arg_sets = column_values_in_pos_cache.get(pred_idx);
                for (int arg_idx = 0; arg_idx < cb.partAsgnRecord.length; arg_idx++) {
                    MultiSet<Integer> arg_set = arg_sets[arg_idx];
                    for (int[] record: cb.complSet) {
                        arg_set.add(record[arg_idx]);
                    }
                }
            }
        }
        for (List<CompliedBlock> cache_entry: allCache) {
            for (int pred_idx = FIRST_BODY_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
                CompliedBlock cb = cache_entry.get(pred_idx);
                MultiSet<Integer>[] arg_sets = column_values_in_all_cache.get(pred_idx);
                for (int arg_idx = 0; arg_idx < cb.partAsgnRecord.length; arg_idx++) {
                    MultiSet<Integer> arg_set = arg_sets[arg_idx];
                    for (int[] record: cb.complSet) {
                        arg_set.add(record[arg_idx]);
                    }
                }
            }
        }

        /* Find all empty arguments */
        List<ArgLocation> empty_args = new ArrayList<>();
        for (int pred_idx = Rule.HEAD_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
            final Predicate predicate = structure.get(pred_idx);
            for (int arg_idx = 0; arg_idx < predicate.arity(); arg_idx++) {
                if (Argument.isEmpty(predicate.args[arg_idx])) {
                    empty_args.add(new ArgLocation(pred_idx, arg_idx));
                }
            }
        }

        /* Find the arg that appears the last for each var */
        ArgLocation[] last_arg_of_vars = new ArgLocation[usedLimitedVars()];
        for (int var_id = 0; var_id < last_arg_of_vars.length; var_id++) {
            final int var_arg = Argument.variable(var_id);
            for (int pred_idx = structure.size() - 1; pred_idx >= HEAD_PRED_IDX; pred_idx--) {
                Predicate predicate = structure.get(pred_idx);
                for (int arg_idx = 0; arg_idx < predicate.arity(); arg_idx++) {
                    if (var_arg == predicate.args[arg_idx]) {
                        last_arg_of_vars[var_id] = new ArgLocation(pred_idx, arg_idx);
                        break;
                    }
                }
            }
        }

        /* Estimate case 1 & 2 */   // Todo: How to estimate the number of records that have already been entailed?
        List<SpecOprWithScore> results = new ArrayList<>();
        for (int var_id = 0; var_id < usedLimitedVars(); var_id++) {
            ArgLocation last_arg_of_var = last_arg_of_vars[var_id];
            final int var_arg = Argument.variable(var_id);

            /* Case 1 */
            for (ArgLocation vacant: empty_args) {
                int another_arg_idx = -1;   // another arg in the same predicate with 'vacant' that shares the same var
                Predicate vacant_pred = structure.get(vacant.predIdx);
                for (int arg_idx = 0; arg_idx < vacant_pred.arity(); arg_idx++) {
                    if (vacant_pred.args[arg_idx] == var_arg) {
                        another_arg_idx = arg_idx;
                        break;
                    }
                }

                double est_pos_ent, est_all_ent;
                if (-1 != another_arg_idx) {
                    /* There is another occurrence of var in the same predicate */
                    int remaining_rows = 0;
                    for (List<CompliedBlock> cache_entry: posCache) {
                        CompliedBlock cb = cache_entry.get(vacant.predIdx);
                        for (int[] record: cb.complSet) {
                            remaining_rows += record[another_arg_idx] == record[vacant.argIdx] ? 1 : 0;
                        }
                    }
                    est_pos_ent = ((double) remaining_rows) / column_values_in_pos_cache.get(vacant.predIdx)[vacant.argIdx].size()
                            * eval.getPosEtls();

                    if (HEAD_PRED_IDX == vacant.predIdx) {
                        est_all_ent = eval.getAllEtls() / kb.totalConstants();
                    } else {
                        remaining_rows = 0;
                        for (List<CompliedBlock> cache_entry: allCache) {
                            CompliedBlock cb = cache_entry.get(vacant.predIdx);
                            for (int[] record: cb.complSet) {
                                remaining_rows += record[another_arg_idx] == record[vacant.argIdx] ? 1 : 0;
                            }
                        }
                        est_all_ent = ((double) remaining_rows) / column_values_in_all_cache.get(vacant.predIdx)[vacant.argIdx].size()
                                * eval.getAllEtls();
                    }
                } else {
                    /* Other occurrences of the var are in different predicates */
                    double est_pos_ratio1 = ((double) column_values_in_pos_cache.get(last_arg_of_var.predIdx)[last_arg_of_var.argIdx].itemCount(
                            column_values_in_pos_cache.get(vacant.predIdx)[vacant.argIdx].distinctValues()
                    )) / column_values_in_pos_cache.get(last_arg_of_var.predIdx)[last_arg_of_var.argIdx].size();
                    double est_pos_ratio2 = ((double) column_values_in_pos_cache.get(vacant.predIdx)[vacant.argIdx].itemCount(
                            column_values_in_pos_cache.get(last_arg_of_var.predIdx)[last_arg_of_var.argIdx].distinctValues()
                    )) / column_values_in_pos_cache.get(vacant.predIdx)[vacant.argIdx].size();
                    est_pos_ent = estimateRatiosInPosCache(est_pos_ratio1, est_pos_ratio2) * eval.getPosEtls();

                    if (HEAD_PRED_IDX == vacant.predIdx) {
                        boolean exist_combined_var = false;
                        boolean[] vars_in_head = vars_in_preds[HEAD_PRED_IDX];
                        for (int pred_idx = FIRST_BODY_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
                            boolean[] vars_in_body_pred = vars_in_preds[pred_idx];
                            if (vars_in_preds[pred_idx][var_id]) {
                                int vid = 0;
                                for (; vid < usedLimitedVars() && !(vars_in_head[vid] && vars_in_body_pred[vid]); vid++);
                                if (vid < usedLimitedVars()) {
                                    exist_combined_var = true;
                                    break;
                                }
                            }
                        }
                        if (exist_combined_var) {
                            /* There are variable combinations in head and another body predicate */
                            est_all_ent = eval.getAllEtls() / kb.totalConstants();  // Todo: Not very accurate here
                        } else {
                            est_all_ent = ((double) column_values_in_all_cache.get(last_arg_of_var.predIdx)[last_arg_of_var.argIdx].differentValues())
                                    / kb.totalConstants() * eval.getAllEtls();
                        }
                    } else {
                        double est_all_ratio1 = ((double) column_values_in_all_cache.get(last_arg_of_var.predIdx)[last_arg_of_var.argIdx].itemCount(
                                column_values_in_all_cache.get(vacant.predIdx)[vacant.argIdx].distinctValues()
                        )) / column_values_in_all_cache.get(last_arg_of_var.predIdx)[last_arg_of_var.argIdx].size();
                        double est_all_ratio2 = ((double) column_values_in_all_cache.get(vacant.predIdx)[vacant.argIdx].itemCount(
                                column_values_in_all_cache.get(last_arg_of_var.predIdx)[last_arg_of_var.argIdx].distinctValues()
                        )) / column_values_in_all_cache.get(vacant.predIdx)[vacant.argIdx].size();
                        est_all_ent = estimateRatiosInAllCache(est_all_ratio1, est_all_ratio2) * eval.getAllEtls();
                    }
                }
                results.add(new SpecOprWithScore(
                        new SpecOprCase1(vacant.predIdx, vacant.argIdx, var_id),
                        new Eval(null, est_pos_ent, est_all_ent, length + 1)
                ));
            }

            /* Case 2 */
            for (SimpleRelation relation: kb.getRelations()) {
                for (int arg_idx = 0; arg_idx < relation.totalCols(); arg_idx++) {
                    double est_pos_ent = ((double) column_values_in_pos_cache.get(last_arg_of_var.predIdx)[last_arg_of_var.argIdx].itemCount(
                            ArrayOperation.toCollection(relation.valuesInColumn(arg_idx))
                    )) / column_values_in_pos_cache.get(last_arg_of_var.predIdx)[last_arg_of_var.argIdx].size() * eval.getPosEtls();
                    double est_all_ent;
                    if (HEAD_PRED_IDX == last_arg_of_var.predIdx) {
                        est_all_ent = ((double) relation.valuesInColumn(arg_idx).length / kb.totalConstants()) * eval.getAllEtls();
                    } else {
                        est_all_ent = ((double) column_values_in_all_cache.get(last_arg_of_var.predIdx)[last_arg_of_var.argIdx].itemCount(
                                ArrayOperation.toCollection(relation.valuesInColumn(arg_idx))
                        )) / column_values_in_all_cache.get(last_arg_of_var.predIdx)[last_arg_of_var.argIdx].size() * eval.getAllEtls();
                    }
                    results.add(new SpecOprWithScore(
                            new SpecOprCase2(relation.id, relation.totalCols(), arg_idx, var_id),
                            new Eval(null, est_pos_ent, est_all_ent, length + 1)
                    ));
                }
            }
        }

        /* Estimate case 3 & 4 */
        for (int i = 0; i < empty_args.size(); i++) {
            /* Find the first empty argument */
            final ArgLocation empty_arg_loc_1 = empty_args.get(i);

            /* Case 3 */
            for (int j = i + 1; j < empty_args.size(); j++) {
                /* Find another empty argument */
                final ArgLocation empty_arg_loc_2 = empty_args.get(j);
                double est_pos_ratio1 = ((double) column_values_in_pos_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx].itemCount(
                        column_values_in_pos_cache.get(empty_arg_loc_2.predIdx)[empty_arg_loc_2.argIdx].distinctValues()
                )) / column_values_in_pos_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx].size();
                double est_pos_ratio2 = ((double) column_values_in_pos_cache.get(empty_arg_loc_2.predIdx)[empty_arg_loc_2.argIdx].itemCount(
                        column_values_in_pos_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx].distinctValues()
                )) / column_values_in_pos_cache.get(empty_arg_loc_2.predIdx)[empty_arg_loc_2.argIdx].size();
                double est_pos_ent = estimateRatiosInPosCache(est_pos_ratio1, est_pos_ratio2) * eval.getPosEtls();
                double est_all_ent;
                if (HEAD_PRED_IDX == empty_arg_loc_1.predIdx) {
                    if (HEAD_PRED_IDX == empty_arg_loc_2.predIdx) {
                        est_all_ent = eval.getNegEtls() / kb.totalConstants();
                    } else {
                        boolean[] vars_in_head = vars_in_preds[HEAD_PRED_IDX];
                        boolean[] vars_in_body_pred = vars_in_preds[empty_arg_loc_2.predIdx];
                        int vid = 0;
                        for (; vid < usedLimitedVars() && !(vars_in_head[vid] && vars_in_body_pred[vid]); vid++);
                        if (vid < usedLimitedVars()) {
                            /* There are variable combinations in head and another body predicate */
                            est_all_ent = eval.getAllEtls() / kb.totalConstants();  // Todo: Not very accurate here
                        } else {
                            est_all_ent = eval.getNegEtls() / kb.totalConstants() *
                                    column_values_in_all_cache.get(empty_arg_loc_2.predIdx)[empty_arg_loc_2.argIdx].differentValues();
                        }
                    }
                } else  {   // here, "empty_arg_loc_2" cannot be in the head
                    double est_neg_ratio1 = ((double) column_values_in_all_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx].itemCount(
                            column_values_in_all_cache.get(empty_arg_loc_2.predIdx)[empty_arg_loc_2.argIdx].distinctValues()
                    )) / column_values_in_all_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx].size();
                    double est_neg_ratio2 = ((double) column_values_in_all_cache.get(empty_arg_loc_2.predIdx)[empty_arg_loc_2.argIdx].itemCount(
                            column_values_in_all_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx].distinctValues()
                    )) / column_values_in_all_cache.get(empty_arg_loc_2.predIdx)[empty_arg_loc_2.argIdx].size();
                    est_all_ent = Math.min(est_neg_ratio1, est_neg_ratio2) * eval.getAllEtls();
                }
                results.add(new SpecOprWithScore(
                        new SpecOprCase3(empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx, empty_arg_loc_2.predIdx, empty_arg_loc_2.argIdx),
                        new Eval(null, est_pos_ent, est_all_ent, length + 1)
                ));
            }

            /* Case 4 */
            if (HEAD_PRED_IDX == empty_arg_loc_1.predIdx) {
                for (SimpleRelation relation: kb.getRelations()) {
                    for (int arg_idx = 0; arg_idx < relation.totalCols(); arg_idx++) {
                        double est_pos_ent = ((double) column_values_in_pos_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx].itemCount(
                                ArrayOperation.toCollection(relation.valuesInColumn(arg_idx))
                        )) / column_values_in_pos_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx].size() * eval.getPosEtls();
                        double est_all_ent = eval.getAllEtls() / kb.totalConstants() * relation.valuesInColumn(arg_idx).length;
                        results.add(new SpecOprWithScore(
                                new SpecOprCase4(relation.id, relation.totalCols(), arg_idx, empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx),
                                new Eval(null, est_pos_ent, est_all_ent, length + 1)
                        ));
                    }
                }
            } else {
                for (SimpleRelation relation: kb.getRelations()) {
                    for (int arg_idx = 0; arg_idx < relation.totalCols(); arg_idx++) {
                        double est_pos_ent = ((double) column_values_in_pos_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx].itemCount(
                                ArrayOperation.toCollection(relation.valuesInColumn(arg_idx))
                        )) / column_values_in_pos_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx].size() * eval.getPosEtls();
                        double est_all_ent = ((double) column_values_in_all_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx].itemCount(
                                ArrayOperation.toCollection(relation.valuesInColumn(arg_idx))
                        )) / column_values_in_all_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx].size() * eval.getAllEtls();
                        results.add(new SpecOprWithScore(
                                new SpecOprCase4(relation.id, relation.totalCols(), arg_idx, empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx),
                                new Eval(null, est_pos_ent, est_all_ent, length + 1)
                        ));
                    }
                }
            }

            /* Case 5 */
            final Predicate predicate1 = structure.get(empty_arg_loc_1.predIdx);
            final int[] const_list = kb.getPromisingConstants(predicate1.predSymbol)[empty_arg_loc_1.argIdx];
            if (HEAD_PRED_IDX == empty_arg_loc_1.predIdx) {
                double est_all_ent = eval.getAllEtls() / kb.totalConstants();
                for (int constant: const_list) {
                    double est_pos_ent = ((double) column_values_in_pos_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx].itemCount(constant))
                            / column_values_in_pos_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx].size() * eval.getPosEtls();
                    results.add(new SpecOprWithScore(
                            new SpecOprCase5(empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx, constant),
                            new Eval(null, est_pos_ent, est_all_ent, length + 1)
                    ));
                }
            } else {
                for (int constant: const_list) {
                    double est_pos_ent = ((double) column_values_in_pos_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx].itemCount(constant))
                            / column_values_in_pos_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx].size() * eval.getPosEtls();
                    double est_all_ent = ((double) column_values_in_all_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx].itemCount(constant))
                            / column_values_in_all_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx].size() * eval.getAllEtls();
                    results.add(new SpecOprWithScore(
                            new SpecOprCase5(empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx, constant),
                            new Eval(null, est_pos_ent, est_all_ent, length + 1)
                    ));
                }
            }
        }
        return results;
    }

    protected double estimateRatiosInPosCache(double ratio1, double ratio2) {
        return Math.min(ratio1, ratio2);
    }

    protected double estimateRatiosInAllCache(double ratio1, double ratio2) {
        return Math.min(ratio1, ratio2);
    }
}
