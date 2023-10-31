package sinc2.impl.est;

import sinc2.common.ArgLocation;
import sinc2.common.Argument;
import sinc2.common.Predicate;
import sinc2.common.Record;
import sinc2.impl.base.CachedRule;
import sinc2.impl.base.CompliedBlock;
import sinc2.kb.SimpleKb;
import sinc2.kb.SimpleRelation;
import sinc2.rule.*;
import sinc2.util.ArrayOperation;
import sinc2.util.MultiSet;

import java.util.*;

public class RandEstRule extends CachedRule {
    public RandEstRule(int headPredSymbol, int arity, Set<Fingerprint> fingerprintCache, Map<MultiSet<Integer>, Set<Fingerprint>> category2TabuSetMap, SimpleKb kb) {
        super(headPredSymbol, arity, fingerprintCache, category2TabuSetMap, kb);
    }

    public RandEstRule(CachedRule another) {
        super(another);
    }

    @Override
    public RandEstRule clone() {
        return new RandEstRule(this);
    }

    public List<SpecOpr> listSpecializations() {
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

        /* Estimate case 1 & 2 */
        final SimpleRelation[] relations = kb.getRelations();
        List<SpecOpr> results = new ArrayList<>();
        for (int var_id = 0; var_id < usedLimitedVars(); var_id++) {
            List<ArgLocation> var_arg_locs = limitedVarArgs.get(var_id);
            final int var_arg = Argument.variable(var_id);

            /* Case 1 */
            for (ArgLocation vacant: empty_args) {
                results.add(new SpecOprCase1(vacant.predIdx, vacant.argIdx, var_id));
            }

            /* Case 2 */
            for (SimpleRelation relation: relations) {
                for (int arg_idx = 0; arg_idx < relation.totalCols(); arg_idx++) {
                    results.add(new SpecOprCase2(relation.id, relation.totalCols(), arg_idx, var_id));
                }
            }
        }

        /* Case 3, 4, and 5 */
        for (int i = 0; i < empty_args.size(); i++) {
            /* Find the first empty argument */
            final ArgLocation empty_arg_loc_1 = empty_args.get(i);
            final Predicate predicate1 = getPredicate(empty_arg_loc_1.predIdx);

            /* Case 5 */
            final int[] const_list = kb.getPromisingConstants(predicate1.predSymbol)[empty_arg_loc_1.argIdx];
            for (int constant: const_list) {
                results.add(new SpecOprCase5(empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx, constant));
            }

            /* Case 3 */
            for (int j = i + 1; j < empty_args.size(); j++) {
                /* Find another empty argument */
                final ArgLocation empty_arg_loc_2 = empty_args.get(j);
                results.add(new SpecOprCase3(
                        empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx, empty_arg_loc_2.predIdx, empty_arg_loc_2.argIdx
                ));
            }

            /* Case 4 */
            for (SimpleRelation relation: relations) {
                for (int arg_idx = 0; arg_idx < relation.totalCols(); arg_idx++) {
                    results.add(new SpecOprCase4(
                            relation.id, relation.totalCols(), arg_idx, empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx
                    ));
                }
            }
        }

        return results;
    }

}
