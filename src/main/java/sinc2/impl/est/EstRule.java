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
    protected final BodyVarLinkManager bodyVarLinkManager;

    public EstRule(int headPredSymbol, int arity, Set<Fingerprint> fingerprintCache, Map<MultiSet<Integer>, Set<Fingerprint>> category2TabuSetMap, SimpleKb kb) {
        super(headPredSymbol, arity, fingerprintCache, category2TabuSetMap, kb);
        bodyVarLinkManager = new BodyVarLinkManager(structure, 0);
    }

    public EstRule(List<Predicate> structure, Set<Fingerprint> fingerprintCache, Map<MultiSet<Integer>, Set<Fingerprint>> category2TabuSetMap, SimpleKb kb) {
        super(structure, fingerprintCache, category2TabuSetMap, kb);
        bodyVarLinkManager = new BodyVarLinkManager(structure, usedLimitedVars());
    }

    public EstRule(EstRule another) {
        super(another);
        this.bodyVarLinkManager = new BodyVarLinkManager(another.bodyVarLinkManager, structure);
    }

    @Override
    public EstRule clone() {
        return new EstRule(this);
    }

    @Override
    protected void cvt1Uv2ExtLvUpdStrc(int predIdx, int argIdx, int varId) {
        super.cvt1Uv2ExtLvUpdStrc(predIdx, argIdx, varId);
        bodyVarLinkManager.specOprCase1(predIdx, argIdx, varId);
    }

    @Override
    protected void cvt2Uvs2NewLvUpdStrc(int predIdx1, int argIdx1, int predIdx2, int argIdx2) {
        super.cvt2Uvs2NewLvUpdStrc(predIdx1, argIdx1, predIdx2, argIdx2);
        bodyVarLinkManager.specOprCase3(predIdx1, argIdx1, predIdx2, argIdx2);
    }

    @Override
    protected void cvt2Uvs2NewLvUpdStrc(int functor, int arity, int argIdx1, int predIdx2, int argIdx2) {
        super.cvt2Uvs2NewLvUpdStrc(functor, arity, argIdx1, predIdx2, argIdx2);
        bodyVarLinkManager.specOprCase4(predIdx2, argIdx2);
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

        /* Estimate case 1 & 2 */   // Todo: How to estimate the number of records that have already been entailed?
        List<SpecOprWithScore> results = new ArrayList<>();
        for (int var_id = 0; var_id < usedLimitedVars(); var_id++) {
            List<ArgLocation> var_arg_locs = limitedVarArgs.get(var_id);
            final int var_arg = Argument.variable(var_id);

            /* Case 1 */
            for (ArgLocation vacant: empty_args) {
                int another_arg_idx = -1;   // another arg in the same predicate with 'vacant' that shares the same var
                {
                    Predicate vacant_pred = structure.get(vacant.predIdx);
                    for (int arg_idx = 0; arg_idx < vacant_pred.arity(); arg_idx++) {
                        if (var_arg == vacant_pred.args[arg_idx]) {
                            another_arg_idx = arg_idx;
                            break;
                        }
                    }
                }

                double est_pos_ent, est_all_ent;
                if (-1 != another_arg_idx) {
                    /* There is another occurrence of var in the same predicate */
                    // Todo: Random sampling may be applied here
                    int remaining_rows = 0;
                    for (List<CompliedBlock> cache_entry: posCache) {
                        CompliedBlock cb = cache_entry.get(vacant.predIdx);
                        for (int[] record: cb.complSet) {
                            remaining_rows += record[another_arg_idx] == record[vacant.argIdx] ? 1 : 0;
                        }
                    }
                    est_pos_ent = eval.getPosEtls() / column_values_in_pos_cache.get(vacant.predIdx)[vacant.argIdx].size() * remaining_rows;

                    if (HEAD_PRED_IDX == vacant.predIdx) {
                        est_all_ent = eval.getAllEtls() / kb.totalConstants();
                    } else {
                        // Todo: Random sampling may be applied here
                        remaining_rows = 0;
                        for (List<CompliedBlock> cache_entry: allCache) {
                            CompliedBlock cb = cache_entry.get(vacant.predIdx);
                            for (int[] record: cb.complSet) {
                                remaining_rows += record[another_arg_idx] == record[vacant.argIdx] ? 1 : 0;
                            }
                        }
                        est_all_ent = eval.getAllEtls() / column_values_in_all_cache.get(vacant.predIdx)[vacant.argIdx].size() * remaining_rows;
                    }
                } else {
                    /* Other occurrences of the var are in different predicates */
                    double[] est_pos_ratios = new double[var_arg_locs.size() + 1];
                    {
                        MultiSet<Integer> vacant_column_values = column_values_in_pos_cache.get(vacant.predIdx)[vacant.argIdx];
                        for (int i = 0; i < var_arg_locs.size(); i++) {
                            ArgLocation var_arg_loc = var_arg_locs.get(i);
                            MultiSet<Integer> var_column_values = column_values_in_pos_cache.get(var_arg_loc.predIdx)[var_arg_loc.argIdx];
                            est_pos_ratios[i] = ((double) var_column_values.itemCount(vacant_column_values.distinctValues())) / var_column_values.size();
                        }
                        ArgLocation _arg_loc_of_var = var_arg_locs.get(0);
                        est_pos_ratios[est_pos_ratios.length - 1] = ((double) vacant_column_values.itemCount(
                                column_values_in_pos_cache.get(_arg_loc_of_var.predIdx)[_arg_loc_of_var.argIdx].distinctValues()
                        )) / vacant_column_values.size();
                    }
                    est_pos_ent = estimateRatiosInPosCache(est_pos_ratios) * eval.getPosEtls();

                    boolean[] vars_in_head = vars_in_preds[HEAD_PRED_IDX];
                    if (HEAD_PRED_IDX == vacant.predIdx) {
                        /* Find a linked var with this one both in head and body (with the shortest path) */
                        VarLink[] var_link_path = null;
                        for (int head_vid = 0; head_vid < vars_in_head.length; head_vid++) {
                            if (vars_in_head[head_vid]) {
                                VarLink[] _var_link_path = bodyVarLinkManager.shortestPath(head_vid, var_id);
                                if (null != _var_link_path && (null == var_link_path || _var_link_path.length < var_link_path.length)) {
                                    var_link_path = _var_link_path;
                                }
                            }
                        }
                        /* Theoretically, var_link_path != null */
                        /* The new var is body-linked with some vars in the head, estimate according to the shortest path */
                        est_all_ent = eval.getAllEtls() / kb.totalConstants() * estimateLinkVarRatio(var_link_path, column_values_in_all_cache);
//                        if (null == var_link_path) {
//                            /* The new var is not body-linked with any of the vars in the head */
//                            ArgLocation arg_loc_of_var = args_of_var.get(0);
//                            est_all_ent = ((double) column_values_in_all_cache.get(arg_loc_of_var.predIdx)[arg_loc_of_var.argIdx].differentValues())
//                                    / kb.totalConstants() * eval.getAllEtls();
//                        }
                    } else {
                        Set<VarPair> new_linked_var_pairs = bodyVarLinkManager.assumeSpecOprCase1(vacant.predIdx, vacant.argIdx, var_id);
                        est_all_ent = eval.getAllEtls();
                        for (VarPair new_linked_var_pair: new_linked_var_pairs) {
                            if (vars_in_head[new_linked_var_pair.vid1] && vars_in_head[new_linked_var_pair.vid2]) {
                                for (ArgLocation arg_loc_of_v2: limitedVarArgs.get(new_linked_var_pair.vid2)) {
                                    if (HEAD_PRED_IDX != arg_loc_of_v2.predIdx) {
                                        /* Find an occurrence of vid2 in the body to get the number of values assigned to that var */
                                        VarLink[] var_link_path = bodyVarLinkManager.assumeShortestPathCase1(
                                                vacant.predIdx, vacant.argIdx, var_id, new_linked_var_pair.vid1, new_linked_var_pair.vid2
                                        );
                                        est_all_ent = est_all_ent / column_values_in_all_cache.get(arg_loc_of_v2.predIdx)[arg_loc_of_v2.argIdx].differentValues()
                                                * estimateLinkVarRatio(var_link_path, column_values_in_all_cache);
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                        /* Find other occurrences of this var in the body */
                        List<ArgLocation> var_arg_locs_in_body = new ArrayList<>();
                        for (ArgLocation arg_loc: var_arg_locs) {
                            if (HEAD_PRED_IDX != arg_loc.predIdx) {
                                var_arg_locs_in_body.add(arg_loc);
                            }
                        }
                        MultiSet<Integer> vacant_column_values = column_values_in_all_cache.get(vacant.predIdx)[vacant.argIdx];
                        if (!var_arg_locs_in_body.isEmpty()) {
                            double[] est_all_ratios = new double[var_arg_locs_in_body.size() + 1];
                            for (int i = 0; i < var_arg_locs_in_body.size(); i++) {
                                ArgLocation arg_loc_of_var = var_arg_locs_in_body.get(i);
                                MultiSet<Integer> var_column_values = column_values_in_all_cache.get(arg_loc_of_var.predIdx)[arg_loc_of_var.argIdx];
                                est_all_ratios[i] = ((double) var_column_values.itemCount(vacant_column_values.distinctValues())) / var_column_values.size();
                            }
                            ArgLocation _arg_loc_of_var = var_arg_locs_in_body.get(0);
                            est_all_ratios[est_all_ratios.length - 1] = ((double) vacant_column_values.itemCount(
                                    column_values_in_all_cache.get(_arg_loc_of_var.predIdx)[_arg_loc_of_var.argIdx].distinctValues()
                            )) / vacant_column_values.size();
                            est_all_ent *= estimateRatiosInAllCache(est_all_ratios);
                        }   // Else: the newly added LV must be included in some newly linked pairs and handled by the previous loop
                    }
                }
                results.add(new SpecOprWithScore(
                        new SpecOprCase1(vacant.predIdx, vacant.argIdx, var_id),
                        new Eval(est_pos_ent, est_all_ent, length + 1)
                ));
            }

            /* Case 2 */
            List<ArgLocation> var_arg_locs_in_body = new ArrayList<>();
            for (ArgLocation arg_loc: var_arg_locs) {
                if (HEAD_PRED_IDX != arg_loc.predIdx) {
                    var_arg_locs_in_body.add(arg_loc);
                }
            }
            for (SimpleRelation relation: kb.getRelations()) {
                for (int arg_idx = 0; arg_idx < relation.totalCols(); arg_idx++) {
                    double[] est_pos_ratios = new double[var_arg_locs.size()];
                    Collection<Integer> relation_column_values = ArrayOperation.toCollection(relation.valuesInColumn(arg_idx));
                    for (int i = 0; i < est_pos_ratios.length; i++) {
                        ArgLocation arg_loc_of_var = var_arg_locs.get(i);
                        MultiSet<Integer> var_column_values = column_values_in_pos_cache.get(arg_loc_of_var.predIdx)[arg_loc_of_var.argIdx];
                        est_pos_ratios[i] = ((double) var_column_values.itemCount(relation_column_values)) / var_column_values.size();
                    }
                    double est_pos_ent = estimateRatiosInPosCache(est_pos_ratios) * eval.getPosEtls();

                    double est_all_ent;
                    if (var_arg_locs_in_body.isEmpty()) {
                        /* All occurrences of the var are in the head */
                        est_all_ent = eval.getAllEtls() / kb.totalConstants() * relation.valuesInColumn(arg_idx).length;
                    } else {
                        double[] est_all_ratios = new double[var_arg_locs_in_body.size()];
                        for (int i = 0; i < est_all_ratios.length; i++) {
                            ArgLocation arg_loc_of_var = var_arg_locs_in_body.get(i);
                            MultiSet<Integer> var_column_values = column_values_in_all_cache.get(arg_loc_of_var.predIdx)[arg_loc_of_var.argIdx];
                            est_all_ratios[i] = ((double) var_column_values.itemCount(relation_column_values)) / var_column_values.size();
                        }
                        est_all_ent = estimateRatiosInAllCache(est_all_ratios) * eval.getAllEtls();
                    }
                    results.add(new SpecOprWithScore(
                            new SpecOprCase2(relation.id, relation.totalCols(), arg_idx, var_id),
                            new Eval(est_pos_ent, est_all_ent, length + 1)
                    ));
                }
            }
        }

        /* Estimate case 3 & 4 & 5 */
        for (int i = 0; i < empty_args.size(); i++) {
            /* Find the first empty argument */
            final ArgLocation empty_arg_loc_1 = empty_args.get(i);
            MultiSet<Integer> empty_arg1_pos_column_values = column_values_in_pos_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx];
            MultiSet<Integer> empty_arg1_all_column_values = column_values_in_all_cache.get(empty_arg_loc_1.predIdx)[empty_arg_loc_1.argIdx];

            /* Case 3 */
            for (int j = i + 1; j < empty_args.size(); j++) {
                /* Find another empty argument */
                final ArgLocation empty_arg_loc_2 = empty_args.get(j);
                MultiSet<Integer> empty_arg2_pos_column_values = column_values_in_pos_cache.get(empty_arg_loc_2.predIdx)[empty_arg_loc_2.argIdx];
                MultiSet<Integer> empty_arg2_all_column_values = column_values_in_all_cache.get(empty_arg_loc_2.predIdx)[empty_arg_loc_2.argIdx];

                double est_pos_ent, est_all_ent;
                if (empty_arg_loc_1.predIdx == empty_arg_loc_2.predIdx) {
                    /* The new args are in the same predicate */
                    // Todo: Random sampling may be applied here
                    int remaining_rows = 0;
                    for (List<CompliedBlock> cache_entry: posCache) {
                        CompliedBlock cb = cache_entry.get(empty_arg_loc_1.predIdx);
                        for (int[] record: cb.complSet) {
                            remaining_rows += record[empty_arg_loc_1.argIdx] == record[empty_arg_loc_2.argIdx] ? 1 : 0;
                        }
                    }
                    est_pos_ent = eval.getPosEtls() / empty_arg1_pos_column_values.size() * remaining_rows;

                    if (HEAD_PRED_IDX == empty_arg_loc_1.predIdx) {
                        est_all_ent = eval.getAllEtls() / kb.totalConstants();
                    } else {
                        // Todo: Random sampling may be applied here
                        remaining_rows = 0;
                        for (List<CompliedBlock> cache_entry: allCache) {
                            CompliedBlock cb = cache_entry.get(empty_arg_loc_1.predIdx);
                            for (int[] record: cb.complSet) {
                                remaining_rows += record[empty_arg_loc_1.argIdx] == record[empty_arg_loc_2.argIdx] ? 1 : 0;
                            }
                        }
                        est_all_ent = eval.getAllEtls() / empty_arg1_all_column_values.size() * remaining_rows;
                    }
                } else {
                    /* The new args are in different predicates */
                    double est_pos_ratio1 = ((double) empty_arg1_pos_column_values.itemCount(empty_arg2_pos_column_values.distinctValues())) / empty_arg1_pos_column_values.size();
                    double est_pos_ratio2 = ((double) empty_arg2_pos_column_values.itemCount(empty_arg1_pos_column_values.distinctValues())) / empty_arg2_pos_column_values.size();
                    est_pos_ent = estimateRatiosInPosCache(new double[]{est_pos_ratio1, est_pos_ratio2}) * eval.getPosEtls();

                    boolean[] vars_in_head = vars_in_preds[HEAD_PRED_IDX];
                    if (HEAD_PRED_IDX == empty_arg_loc_1.predIdx) {
                        /* HEAD_PRED_IDX != empty_arg_loc_2.predIdx */
                        est_all_ent = eval.getAllEtls() / kb.totalConstants();
                        for (int head_vid = 0; head_vid < vars_in_head.length; head_vid++) {
                            if (vars_in_head[head_vid]) {
                                VarLink[] var_link_path = bodyVarLinkManager.assumeShortestPathCase3(empty_arg_loc_2.predIdx, empty_arg_loc_2.argIdx, head_vid);
                                if (null != var_link_path) {
                                    /* Estimate for linked vars */
                                    est_all_ent *= estimateLinkVarRatio(var_link_path, column_values_in_all_cache);
                                    break;
                                }
                            }
                        }
                        /* Theoretically, 'est_all_ent' will always be estimated in the above loop */
                    } else  {
                        /* Here, both the arguments are in the body and in different predicates */
                        Set<VarPair> new_linked_var_pairs = bodyVarLinkManager.assumeSpecOprCase3(
                                empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx, empty_arg_loc_2.predIdx, empty_arg_loc_2.argIdx
                        );
                        est_all_ent = eval.getAllEtls();
                        for (VarPair new_linked_var_pair: new_linked_var_pairs) {
                            if (vars_in_head[new_linked_var_pair.vid1] && vars_in_head[new_linked_var_pair.vid2]) {
                                for (ArgLocation arg_loc_of_v2_in_body: limitedVarArgs.get(new_linked_var_pair.vid2)) {
                                    if (HEAD_PRED_IDX != arg_loc_of_v2_in_body.predIdx) {
                                        VarLink[] var_link_path = bodyVarLinkManager.assumeShortestPathCase3(
                                                empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx, empty_arg_loc_2.predIdx, empty_arg_loc_2.argIdx,
                                                new_linked_var_pair.vid1, new_linked_var_pair.vid2
                                        );
                                        est_all_ent = est_all_ent
                                                / column_values_in_all_cache.get(arg_loc_of_v2_in_body.predIdx)[arg_loc_of_v2_in_body.argIdx].differentValues()
                                                * estimateLinkVarRatio(var_link_path, column_values_in_all_cache);
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                        double est_all_ratio1 = ((double) empty_arg1_all_column_values.itemCount(empty_arg2_all_column_values.distinctValues())) / empty_arg1_all_column_values.size();
                        double est_all_ratio2 = ((double) empty_arg2_all_column_values.itemCount(empty_arg1_all_column_values.distinctValues())) / empty_arg2_all_column_values.size();
                        est_all_ent *= estimateRatiosInAllCache(new double[]{est_all_ratio1, est_all_ratio2});
                    }
                }
                results.add(new SpecOprWithScore(
                        new SpecOprCase3(empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx, empty_arg_loc_2.predIdx, empty_arg_loc_2.argIdx),
                        new Eval(est_pos_ent, est_all_ent, length + 1)
                ));
            }

            /* Case 4 */
            if (HEAD_PRED_IDX == empty_arg_loc_1.predIdx) {
                for (SimpleRelation relation: kb.getRelations()) {
                    for (int arg_idx = 0; arg_idx < relation.totalCols(); arg_idx++) {
                        double est_pos_ent = eval.getPosEtls() / empty_arg1_pos_column_values.size() * empty_arg1_pos_column_values.itemCount(
                                ArrayOperation.toCollection(relation.valuesInColumn(arg_idx))
                        );
                        double est_all_ent = eval.getAllEtls() / kb.totalConstants() * relation.valuesInColumn(arg_idx).length;
                        results.add(new SpecOprWithScore(
                                new SpecOprCase4(relation.id, relation.totalCols(), arg_idx, empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx),
                                new Eval(est_pos_ent, est_all_ent, length + 1)
                        ));
                    }
                }
            } else {
                for (SimpleRelation relation: kb.getRelations()) {
                    for (int arg_idx = 0; arg_idx < relation.totalCols(); arg_idx++) {
                        Collection<Integer> relation_column_values = ArrayOperation.toCollection(relation.valuesInColumn(arg_idx));
                        double est_pos_ent = eval.getPosEtls() / empty_arg1_pos_column_values.size() * empty_arg1_pos_column_values.itemCount(relation_column_values);
                        double est_all_ent = eval.getAllEtls() / empty_arg1_all_column_values.size() * empty_arg1_all_column_values.itemCount(relation_column_values);
                        results.add(new SpecOprWithScore(
                                new SpecOprCase4(relation.id, relation.totalCols(), arg_idx, empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx),
                                new Eval(est_pos_ent, est_all_ent, length + 1)
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
                    double est_pos_ent = eval.getPosEtls() / empty_arg1_pos_column_values.size() * empty_arg1_pos_column_values.itemCount(constant);
                    results.add(new SpecOprWithScore(
                            new SpecOprCase5(empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx, constant),
                            new Eval(est_pos_ent, est_all_ent, length + 1)
                    ));
                }
            } else {
                for (int constant: const_list) {
                    double est_pos_ent = eval.getPosEtls() / empty_arg1_pos_column_values.size() * empty_arg1_pos_column_values.itemCount(constant);
                    double est_all_ent = eval.getAllEtls() / empty_arg1_all_column_values.size() * empty_arg1_all_column_values.itemCount(constant);
                    results.add(new SpecOprWithScore(
                            new SpecOprCase5(empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx, constant),
                            new Eval(est_pos_ent, est_all_ent, length + 1)
                    ));
                }
            }
        }
        return results;
    }

    protected double estimateRatiosInPosCache(double[] ratios) {
        double min = ratios[0];
        for (double ratio: ratios) {
            min = Math.min(min, ratio);
        }
        return min;
    }

    protected double estimateRatiosInAllCache(double[] ratios) {
        double min = ratios[0];
        for (double ratio: ratios) {
            min = Math.min(min, ratio);
        }
        return min;
    }

    protected double estimateLinkVarRatio(VarLink[] varLinkPath, List<MultiSet<Integer>[]> columnValuesInCache) {
        double ratio = 1.0;
        for (VarLink link: varLinkPath) {
            MultiSet<Integer>[] column_values_in_pred = columnValuesInCache.get(link.predIdx);
            ratio = ratio * column_values_in_pred[link.fromArgIdx].size() /
                    column_values_in_pred[link.fromArgIdx].differentValues();
        }
        return ratio;
    }
}
