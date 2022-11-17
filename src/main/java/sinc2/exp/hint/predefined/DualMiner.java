package sinc2.exp.hint.predefined;

import sinc2.exp.hint.HinterKb;
import sinc2.kb.SimpleRelation;
import sinc2.util.ArrayOperation;

import java.util.*;

/**
 * Template miner for:
 * 4. Dual:
 *   h(X, Y) :- p(Y, X)
 *
 * @since 2.0
 */
public class DualMiner extends TemplateMiner {
    @Override
    public List<MatchedRule> matchTemplate(HinterKb kb) {
        SimpleRelation[] relations = kb.getRelations();
        List<MatchedRule> matched_rules = new ArrayList<>();
        for (int p = 0; p < relations.length; p++) {
            SimpleRelation relation_p = relations[p];
            if (2 != relation_p.totalCols()) {
                continue;
            }

            /* Match head & check validness */
            List<HinterKb.ColInfo> similar_cols = kb.inverseSimilarCols(p, 0);
            Set<Integer> rel_idxs = new HashSet<>();
            for (int i = 1; i < similar_cols.size(); i++) { // Skip the column itself
                HinterKb.ColInfo col_info = similar_cols.get(i);
                if (1 == col_info.colIdx && 2 == relations[col_info.relIdx].totalCols()) {
                    rel_idxs.add(col_info.relIdx);
                }
            }
            similar_cols = kb.inverseSimilarCols(p, 1);
            List<Integer> _rel_idxs = new ArrayList<>(similar_cols.size());
            for (int i = 1; i < similar_cols.size(); i++) { // Skip the column itself
                HinterKb.ColInfo col_info = similar_cols.get(i);
                if (0 == col_info.colIdx) {
                    _rel_idxs.add(col_info.relIdx);
                }
            }
            rel_idxs.removeIf(e -> !_rel_idxs.contains(e));
            if (!rel_idxs.isEmpty()) {
                /* Find entailments */
                int[][] records = relation_p.getAllRows();
                int[][] entailments = new int[records.length][];
                for (int i = 0; i < records.length; i++) {
                    int[] record = records[i];
                    entailments[i] = new int[]{record[1], record[0]};
                }

                /* Check validness */
                for (int h: rel_idxs) {
                    SimpleRelation head = relations[h];
                    if (2 == head.totalCols()) {
                        checkThenAdd(head, entailments, matched_rules, dualRuleString(head.name, relation_p.name));
                    }
                }
            }
        }
        return matched_rules;
    }

    protected String dualRuleString(String h, String p) {
        return String.format("%s(X,Y):-%s(Y,X)", h, p);
    }

    @Override
    public int templateLength() {
        return 2;
    }

    @Override
    public String templateName() {
        return "Dual";
    }
}
