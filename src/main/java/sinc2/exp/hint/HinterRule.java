package sinc2.exp.hint;

import sinc2.common.Predicate;
import sinc2.common.Record;
import sinc2.impl.base.CachedRule;
import sinc2.impl.base.CompliedBlock;
import sinc2.kb.KbException;
import sinc2.kb.SimpleRelation;
import sinc2.rule.Fingerprint;
import sinc2.rule.Rule;
import sinc2.util.MultiSet;

import java.util.*;

/**
 * A subclass of "CachedRule" that can explicitly extract entailments to other data structures.
 *
 * @since 2.0
 */
public class HinterRule extends CachedRule {

    public HinterRule(
            int headRelNum, int arity, Set<Fingerprint> fingerprintCache,
            Map<MultiSet<Integer>, Set<Fingerprint>> category2TabuSetMap, HinterKb kb
    ) {
        super(headRelNum, arity, fingerprintCache, category2TabuSetMap, kb);
    }

    public HinterRule(
            List<Predicate> structure, Set<Fingerprint> fingerprintCache,
            Map<MultiSet<Integer>, Set<Fingerprint>> category2TabuSetMap, HinterKb kb
    ) {
        super(structure, fingerprintCache, category2TabuSetMap, kb);
    }

    public HinterRule(CachedRule another) {
        super(another);
    }

    @Override
    public HinterRule clone() {
        return new HinterRule(this);
    }

    /**
     * Extract positive entailments to a relation.
     */
    public void extractPositiveEntailments(SimpleRelation relation) throws KbException {
        Set<Record> pos_entails = new HashSet<>();
        for (List<CompliedBlock> entry: posCache) {
            for (int[] record: entry.get(0).complSet) {
                pos_entails.add(new Record(record));
            }
        }
        int[][] entailed_records = new int[pos_entails.size()][];
        int i = 0;
        for (Record record: pos_entails) {
            entailed_records[i] = record.args;
            i++;
        }
        relation.setAllAsEntailed(entailed_records);
    }

    /**
     * Override the original version to thread safe.
     */
    @Override
    protected void add2TabuSet() {
        final MultiSet<Integer> functor_mset = new MultiSet<>();
        for (int pred_idx = Rule.FIRST_BODY_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
            functor_mset.add(structure.get(pred_idx).predSymbol);
        }
        final Set<Fingerprint> tabu_set = category2TabuSetMap.computeIfAbsent(
                functor_mset, k -> Collections.synchronizedSet(new HashSet<>())
        );
        tabu_set.add(fingerprint);
    }
}
