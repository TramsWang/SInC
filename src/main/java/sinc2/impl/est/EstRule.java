package sinc2.impl.est;

import sinc2.common.Argument;
import sinc2.common.Predicate;
import sinc2.impl.base.CachedRule;
import sinc2.kb.SimpleKb;
import sinc2.rule.Eval;
import sinc2.rule.Fingerprint;
import sinc2.rule.SpecOpr;
import sinc2.util.MultiSet;

import java.util.*;

/**
 * This rule is a specialization of cached rule that estimates the evaluation of each possible specialization.
 *
 * @since 2.1
 */
public class EstRule extends CachedRule {
    /** Link tree of this rule */
    protected final List<LinkTreeNode> linkTree = new ArrayList<>();

    public EstRule(int headPredSymbol, int arity, Set<Fingerprint> fingerprintCache, Map<MultiSet<Integer>, Set<Fingerprint>> category2TabuSetMap, SimpleKb kb) {
        super(headPredSymbol, arity, fingerprintCache, category2TabuSetMap, kb);

        /* Build link tree */
        linkTree.add(new LinkTreeNode(arity, 0, 0, 0));
    }

    public EstRule(List<Predicate> structure, Set<Fingerprint> fingerprintCache, Map<MultiSet<Integer>, Set<Fingerprint>> category2TabuSetMap, SimpleKb kb) {
        super(structure, fingerprintCache, category2TabuSetMap, kb);

        /* Build link tree */
        Iterator<Predicate> pred_itr = structure.listIterator();
        Predicate pred_head = pred_itr.next();
        linkTree.add(new LinkTreeNode(pred_head.arity(), 0, 0, 0));
        while (pred_itr.hasNext()) {
            Predicate predicate = pred_itr.next();
            LinkTreeNode node = new LinkTreeNode(predicate.arity(), 0, 0, 0);
            node.level = structure.size() + 1;
            linkTree.add(node);
        }
        List<Integer> waiting_pred_idx = new ArrayList<>();
        waiting_pred_idx.add(HEAD_PRED_IDX);
        for (int i = 0; i < waiting_pred_idx.size(); i++) {
            int pred_idx = waiting_pred_idx.get(i);
            Predicate predicate = structure.get(pred_idx);
            int level = linkTree.get(pred_idx).level;
            Map<Integer, Integer> var_2_cols = new HashMap<>();
            for (int arg_idx = 0; arg_idx < predicate.arity(); arg_idx++) {
                int argument = predicate.args[arg_idx];
                if (Argument.isVariable(argument)) {
                    var_2_cols.put(argument, arg_idx);
                }
            }
            for (int pred_idx2 = FIRST_BODY_PRED_IDX; pred_idx2 < structure.size(); pred_idx2++) {
                LinkTreeNode successor_node = linkTree.get(pred_idx2);
                if (level < successor_node.level - 1) {
                    Predicate successor_pred = structure.get(pred_idx2);
                    for (int arg_idx2 = 0; arg_idx2 < successor_pred.arity(); arg_idx2++) {
                        Integer col = var_2_cols.get(successor_pred.args[arg_idx2]);
                        if (null != col) {
                            /* There is a linked var with 'predicate' */
                            successor_node.level = level + 1;
                            successor_node.predNodeIdx = pred_idx;
                            successor_node.predKeyArgIdx = col;
                            successor_node.keyColIdx = arg_idx2;
                            waiting_pred_idx.add(pred_idx2);
                            break;
                        }
                    }
                }
            }
        }
    }

    public EstRule(EstRule another) {
        super(another);
        for (LinkTreeNode node: another.linkTree) {
            this.linkTree.add(new LinkTreeNode(node));
        }
    }

    // Todo: Currently, the structure-update procedures does not necessarily guarantee the generated link tree to be minimum
    @Override
    protected void cvt1Uv2ExtLvUpdStrc(final int predIdx, final int argIdx, final int varId) {
        super.cvt1Uv2ExtLvUpdStrc(predIdx, argIdx, varId);
        LinkTreeNode node = linkTree.get(predIdx);
        final int var_arg = Argument.variable(varId);
        for (int pred_idx = HEAD_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
            LinkTreeNode predecessor_node = linkTree.get(pred_idx);
            if (predecessor_node.level < node.level - 1) {
                Predicate predecessor_predicate = structure.get(pred_idx);
                for (int arg_idx = 0; arg_idx < predecessor_predicate.arity(); arg_idx++) {
                    if (var_arg == predecessor_predicate.args[arg_idx]) {
                        node.level = predecessor_node.level + 1;
                        node.predNodeIdx = pred_idx;
                        node.predKeyArgIdx = arg_idx;
                        node.keyColIdx = argIdx;
                        break;
                    }
                }
            }
        }
    }

    @Override
    protected void cvt1Uv2ExtLvUpdStrc(final int functor, final int arity, final int argIdx, final int varId) {
        super.cvt1Uv2ExtLvUpdStrc(functor, arity, argIdx, varId);
        LinkTreeNode node = new LinkTreeNode(arity, 0, 0, 0);
        node.level = structure.size();
        linkTree.add(node);
        final int var_arg = Argument.variable(varId);
        for (int pred_idx = HEAD_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
            LinkTreeNode predecessor_node = linkTree.get(pred_idx);
            if (predecessor_node.level < node.level - 1) {
                Predicate predicate = structure.get(pred_idx);
                for (int arg_idx = 0; arg_idx < predicate.arity(); arg_idx++) {
                    if (var_arg == predicate.args[arg_idx]) {
                        node.level = predecessor_node.level + 1;
                        node.predNodeIdx = pred_idx;
                        node.predKeyArgIdx = arg_idx;
                        node.keyColIdx = argIdx;
                        break;
                    }
                }
            }
        }
    }

    @Override
    protected void cvt2Uvs2NewLvUpdStrc(final int predIdx1, final int argIdx1, final int predIdx2, final int argIdx2) {
        super.cvt2Uvs2NewLvUpdStrc(predIdx1, argIdx1, predIdx2, argIdx2);
        LinkTreeNode node1 = linkTree.get(predIdx1);
        LinkTreeNode node2 = linkTree.get(predIdx2);
        if (node1.level < node2.level - 1) {
            node2.level = node1.level + 1;
            node2.predNodeIdx = predIdx1;
            node2.predKeyArgIdx = argIdx1;
            node2.keyColIdx = argIdx2;
        } else if (node2.level < node1.level - 1) {
            node1.level = node2.level + 1;
            node1.predNodeIdx = predIdx2;
            node1.predKeyArgIdx = argIdx2;
            node1.keyColIdx = argIdx1;
        }
    }

    @Override
    protected void cvt2Uvs2NewLvUpdStrc(
            final int functor, final int arity, final int argIdx1, final int predIdx2, final int argIdx2
    ) {
        super.cvt2Uvs2NewLvUpdStrc(functor, arity, argIdx1, predIdx2, argIdx2);
        LinkTreeNode node = new LinkTreeNode(arity, argIdx1, predIdx2, argIdx2);
        node.level = linkTree.get(predIdx2).level + 1;
        linkTree.add(node);
    }

//    Case 5 does not change the level of the node in the link tree
//    @Override
//    protected void cvt1Uv2ConstUpdStrc(final int predIdx, final int argIdx, final int constant) {
//        super.cvt1Uv2ConstUpdStrc(predIdx, argIdx, constant);
//    }

    static public class SpecOprWithScore {
        public final SpecOpr opr;
        public final Eval estEval;

        public SpecOprWithScore(SpecOpr opr, Eval estEval) {
            this.opr = opr;
            this.estEval = estEval;
        }
    }

//    public List<SpecOprWithScore> estimateSpecializations() {
//        ?
//        /* Gather values in columns */
//
//        /* Update factors in link tree */
//
//        /* Estimate case 1 */
//        /* Estimate case 2 */
//        /* Estimate case 3 */
//        /* Estimate case 4 */
//        /* Estimate case 5 */
//    }

}
