package sinc2.impl.negsamp;

import sinc2.common.ArgLocation;
import sinc2.common.Argument;
import sinc2.common.Predicate;
import sinc2.common.Record;
import sinc2.kb.*;
import sinc2.rule.Eval;
import sinc2.rule.Fingerprint;
import sinc2.util.MultiSet;

import java.util.List;
import java.util.Map;
import java.util.Set;

public class AdvNegCachedRule extends NegSampleCachedRule {

    protected final int budget;

    public AdvNegCachedRule(
            int headPredSymbol, int arity, Set<Fingerprint> fingerprintCache,
            Map<MultiSet<Integer>, Set<Fingerprint>> category2TabuSetMap, SimpleKb kb,
            Map<Record, Float> negSampleWeightMap, IntTable negSamples, int budget
    ) {
        super(headPredSymbol, arity, fingerprintCache, category2TabuSetMap, kb, negSampleWeightMap, negSamples);
        this.budget = budget;
    }

    public AdvNegCachedRule(AdvNegCachedRule another) {
        super(another);
        this.budget = another.budget;
    }

    @Override
    public AdvNegCachedRule clone() {
        long time_start = System.nanoTime();
        AdvNegCachedRule rule = new AdvNegCachedRule(this);
        rule.copyTime = System.nanoTime() - time_start;
        return rule;
    }

    protected void sample() {
        /* Prepare parameters */
        Predicate head_pred = structure.get(HEAD_PRED_IDX);
        SimpleRelation head_relation = kb.getRelation(head_pred.predSymbol);
        CacheFragArgLoc[] arg_locs = new CacheFragArgLoc[head_relation.totalCols()];
        int uvid = usedLimitedVars();
        for (int arg_idx = 0; arg_idx < arg_locs.length; arg_idx++) {
            int argument = head_pred.args[arg_idx];
            if (Argument.isVariable(argument)) {
                final int vid = Argument.decode(argument);
                List<ArgLocation> locs = limitedVarArgs.get(vid);
                CacheFragArgLoc arg_loc = null;
                for (ArgLocation loc: locs) {
                    if (HEAD_PRED_IDX != loc.predIdx) {
                        TabInfo tab_info = predIdx2AllCacheTableInfo.get(loc.predIdx);
                        arg_loc = CacheFragArgLoc.createLoc(tab_info.fragmentIdx, tab_info.tabIdx, loc.argIdx);
                        break;
                    }
                }
                if (null == arg_loc) {
                    /* A head only variable */
                    arg_loc = CacheFragArgLoc.createVid(vid);
                }
                arg_locs[arg_idx] = arg_loc;
            } else if (Argument.isConstant(argument)) {
                arg_locs[arg_idx] = CacheFragArgLoc.createConstant(Argument.decode(argument));
            } else {
                arg_locs[arg_idx] = CacheFragArgLoc.createVid(uvid);
                uvid++;
            }
        }

        /* Sample */
        AdversarialSampledResult sample_result = NegSampler.adversarialSampling(
                head_relation, kb.totalConstants(), budget, allCache, arg_locs, structure, kb
        );
        if (null == sample_result) {
            negCache.clear();
            eval = new Eval(eval.getPosEtls(), eval.getPosEtls(), length);
            return;
        }

        negCache = sample_result.negCache;
        IntTable neg_table = sample_result.negTable;
        if (null != negSampleWeightMap) {
            negSampleWeightMap = NegSampler.calcNegSampleWeight(head_relation, neg_table, kb.totalConstants());
        }

        /* Update evaluation score. The new score will be used for comparison with specializations */
        double all_ent;
        if (null == negSampleWeightMap) {
            all_ent = ((double) neg_table.totalRows()) + eval.getPosEtls();
        } else {
            all_ent = eval.getPosEtls();
            for (float weight: negSampleWeightMap.values()) {
                all_ent += weight;
            }
        }
        eval = new Eval(eval.getPosEtls(), all_ent, length);
    }
}
