package sinc2.impl.negsamp;

import sinc2.common.Predicate;
import sinc2.common.Record;
import sinc2.kb.IntTable;
import sinc2.kb.SimpleKb;
import sinc2.kb.SimpleRelation;
import sinc2.rule.Eval;
import sinc2.rule.Fingerprint;
import sinc2.rule.UpdateStatus;
import sinc2.util.MultiSet;

import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * This rule employs a negative cache (N-cache) which caches entailment evidence of negative samples. The score of this
 * type of rule is given by E+-cache, T-cache, and N-cache. Only when enumerating counter examples will the E-cache be
 * used.
 *
 * @since 2.3
 */
public class NegSampleCachedRule extends FragmentedCachedRule {

    /** This cache is used to monitor sampled negative records (N-cache). One cache fragment is sufficient as all
     *  predicates are linked to the head. */
    protected CacheFragment negCache;
    /** The map from negative examples to their weight. If NULL, all negative examples are weighted as 1. */
    protected Map<Record, Float> negSampleWeightMap;

    /* Monitoring info. The time (in nanoseconds) refers to the corresponding time consumption in the last update of the rule */
    protected long negCacheUpdateTime = 0;
    protected long negCacheIndexingTime = 0;

    /**
     * @param negSampleWeightMap Weight for negative samples. If NULL, all samples are weighted as 1
     * @param negSamples         All negative samples, organized as an integer table
     */
    public NegSampleCachedRule(
            int headPredSymbol, int arity, Set<Fingerprint> fingerprintCache,
            Map<MultiSet<Integer>, Set<Fingerprint>> category2TabuSetMap, SimpleKb kb,
            Map<Record, Float> negSampleWeightMap, IntTable negSamples
    ) {
        super(headPredSymbol, arity, fingerprintCache, category2TabuSetMap, kb);
        negCache = new CacheFragment(negSamples, headPredSymbol);
        this.negSampleWeightMap = negSampleWeightMap;
        /* Evaluation has already been calculated in super() */
    }

    public NegSampleCachedRule(
            List<Predicate> structure, Set<Fingerprint> fingerprintCache,
            Map<MultiSet<Integer>, Set<Fingerprint>> category2TabuSetMap, SimpleKb kb,
            Map<Record, Float> negSampleWeightMap, IntTable negSamples) {
        super(structure, fingerprintCache, category2TabuSetMap, kb);
        throw new Error("Not Implemented!");    // Todo: Implement here
    }

    public NegSampleCachedRule(NegSampleCachedRule another) {
        super(another);
        this.negCache = new CacheFragment(another.negCache);
        this.negSampleWeightMap = another.negSampleWeightMap;
    }

    @Override
    public NegSampleCachedRule clone() {
        long time_start = System.nanoTime();
        NegSampleCachedRule rule =  new NegSampleCachedRule(this);
        rule.copyTime = System.nanoTime() - time_start;
        return rule;
    }

    /**
     * Calculate the evaluation of the rule.
     */
    @Override
    protected Eval calculateEval() {
        int new_pos_ent = posCache.countTableSize(HEAD_PRED_IDX);
        float neg_ent;
        if (null == negSampleWeightMap) {
            /* Equal weight. Only count the number of negative samples */
            neg_ent = negCache.countTableSize(HEAD_PRED_IDX);
        } else {
            /* Count the total weight of negative entailments */
            Set<int[]> neg_ent_set = new HashSet<>();   // object reference is sufficient for identifying a record in a (neg-) KB
            neg_ent = 0;
            for (List<CB> cache_entry: negCache.entries) {
                for (int[] neg_record: cache_entry.get(HEAD_PRED_IDX).complianceSet) {
                    if (neg_ent_set.add(neg_record)) {
                        neg_ent += negSampleWeightMap.get(new Record(neg_record));
                    }
                }
            }
        }
        return new Eval(new_pos_ent, new_pos_ent + neg_ent, length);
    }

    /**
     * This method calculates real evaluation based on the accurate number of positive and negative entailments and
     * replaces the rule evaluation with it.
     */
    public void calcRealEval() {
        eval = super.calculateEval();
    }

    @Override
    public void updateCacheIndices() {
        long time_start = System.nanoTime();
        posCache.buildIndices();
        long time_pos_done = System.nanoTime();
        entCache.buildIndices();
        long time_ent_done = System.nanoTime();
        for (CacheFragment fragment: allCache) {
            fragment.buildIndices();
        }
        long time_all_done = System.nanoTime();
        negCache.buildIndices();
        long time_neg_done = System.nanoTime();
        posCacheIndexingTime = time_pos_done - time_start;
        entCacheIndexingTime = time_ent_done - time_pos_done;
        allCacheIndexingTime = time_all_done - time_ent_done;
        negCacheIndexingTime = time_neg_done - time_all_done;
    }

    @Override
    protected UpdateStatus cvt1Uv2ExtLvHandlerPostCvg(int predIdx, int argIdx, int varId) {
        long time_start = System.nanoTime();
        negCache.updateCase1a(predIdx, argIdx, varId);
        negCacheUpdateTime = System.nanoTime() - time_start;
        return super.cvt1Uv2ExtLvHandlerPostCvg(predIdx, argIdx, varId);
    }

    @Override
    protected UpdateStatus cvt1Uv2ExtLvHandlerPostCvg(Predicate newPredicate, int argIdx, int varId) {
        long time_start = System.nanoTime();
        SimpleRelation new_relation = kb.getRelation(newPredicate.predSymbol);
        negCache.updateCase1b(new_relation, new_relation.id, argIdx, varId);
        negCacheUpdateTime = System.nanoTime() - time_start;
        return super.cvt1Uv2ExtLvHandlerPostCvg(newPredicate, argIdx, varId);
    }

    @Override
    protected UpdateStatus cvt2Uvs2NewLvHandlerPostCvg(int predIdx1, int argIdx1, int predIdx2, int argIdx2) {
        long time_start = System.nanoTime();
        negCache.updateCase2a(predIdx1, argIdx1, predIdx2, argIdx2, usedLimitedVars() - 1);
        negCacheUpdateTime = System.nanoTime() - time_start;
        return super.cvt2Uvs2NewLvHandlerPostCvg(predIdx1, argIdx1, predIdx2, argIdx2);
    }

    @Override
    protected UpdateStatus cvt2Uvs2NewLvHandlerPostCvg(Predicate newPredicate, int argIdx1, int predIdx2, int argIdx2) {
        long time_start = System.nanoTime();
        SimpleRelation new_relation = kb.getRelation(newPredicate.predSymbol);
        negCache.updateCase2b(new_relation, new_relation.id, argIdx1, predIdx2, argIdx2, usedLimitedVars() - 1);
        negCacheUpdateTime = System.nanoTime() - time_start;
        return super.cvt2Uvs2NewLvHandlerPostCvg(newPredicate, argIdx1, predIdx2, argIdx2);
    }

    @Override
    protected UpdateStatus cvt1Uv2ConstHandlerPostCvg(int predIdx, int argIdx, int constant) {
        long time_start = System.nanoTime();
        negCache.updateCase3(predIdx, argIdx, constant);
        negCacheUpdateTime = System.nanoTime() - time_start;
        return super.cvt1Uv2ConstHandlerPostCvg(predIdx, argIdx, constant);
    }

    @Override
    public void releaseMemory() {
        super.releaseMemory();
        negCache = null;
    }
}
