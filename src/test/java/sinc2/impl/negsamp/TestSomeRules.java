package sinc2.impl.negsamp;

import sinc2.kb.NegSampleKb;
import sinc2.kb.SimpleKb;
import sinc2.rule.EvalMetric;

import java.io.IOException;
import java.util.HashMap;
import java.util.HashSet;

public class TestSomeRules {
    public static void main(String[] args) throws IOException {
        SimpleKb kb = new SimpleKb("Fm", "datasets/SimpleFormat");
        NegSampleKb negKb = new NegSampleKb("Fm_neg_uni_2.0", "Exp/NegEst/casestudy");
//        NegSampleKb negKb = new NegSampleKb("Fm_neg_det_h3r1", "Exp/NegEst/casestudy");
        boolean use_weight = true;

        NegSampleCachedRule rule = new NegSampleCachedRule(
                6, 2, new HashSet<>(), new HashMap<>(),
                kb, use_weight ? negKb.getSampleWeight(6) : null, negKb.getNegSamples(6)
        );
        rule.cvt2Uvs2NewLv(3, 2, 0, 0, 0);
        rule.updateCacheIndices();
        rule.cvt2Uvs2NewLv(2, 2, 0, 1, 1);
        System.out.println(rule.getEval().value(EvalMetric.CompressionRatio));
        System.out.println(rule.getEval().toString());

        rule = new NegSampleCachedRule(
                6, 2, new HashSet<>(), new HashMap<>(),
                kb, use_weight ? negKb.getSampleWeight(6) : null, negKb.getNegSamples(6)
        );
        rule.cvt2Uvs2NewLv(3, 2, 0, 0, 0);
        rule.updateCacheIndices();
        rule.cvt2Uvs2NewLv(2, 2, 1, 0, 1);
        System.out.println(rule.getEval().value(EvalMetric.CompressionRatio));
        System.out.println(rule.getEval().toString());

        rule = new NegSampleCachedRule(
                6, 2, new HashSet<>(), new HashMap<>(),
                kb, use_weight ? negKb.getSampleWeight(6) : null, negKb.getNegSamples(6)
        );
        rule.cvt2Uvs2NewLv(3, 2, 0, 0, 0);
        rule.updateCacheIndices();
        rule.cvt2Uvs2NewLv(0, 2, 1, 0, 1);
        System.out.println(rule.getEval().value(EvalMetric.CompressionRatio));
        System.out.println(rule.getEval().toString());
    }
}
