package sinc2.impl.negsamp;

import sinc2.common.Argument;
import sinc2.common.Predicate;
import sinc2.common.Record;
import sinc2.kb.IntTable;
import sinc2.kb.SimpleKb;
import sinc2.kb.SimpleRelation;
import sinc2.kb.SplitRecords;
import sinc2.rule.*;
import sinc2.util.MultiSet;

import java.util.*;

/**
 * First-order Horn rule with the compact grounding cache (CGC). The CGC is implemented by "CacheFragment".
 *
 * @since 2.3
 */
public class FragmentedCachedRule extends Rule {

    /**
     * This class is for mapping predicates in the rule to tables in cache fragments in the E-cache.
     */
    static class TabInfo {
        int fragmentIdx;
        int tabIdx;

        public TabInfo(int fragmentIdx, int tabIdx) {
            this.fragmentIdx = fragmentIdx;
            this.tabIdx = tabIdx;
        }

        public TabInfo(TabInfo another) {
            this.fragmentIdx = another.fragmentIdx;
            this.tabIdx = another.tabIdx;
        }
    }

    /** The original KB */
    protected final SimpleKb kb;
    /** The cache for the positive entailments (E+-cache) (not entailed). One cache fragment is sufficient as all
     *  predicates are linked to the head. */
    protected CacheFragment posCache;
    /** This cache is used to monitor the already-entailed records (T-cache). One cache fragment is sufficient as all
     *  predicates are linked to the head. */
    protected CacheFragment entCache;
    /** The cache for all the entailments (E-cache). The cache is composed of multiple fragments, each of which maintains
     *  a linked component of the rule body. The first relation should not be included, as the head should be removed
     *  from E-cache */
    protected List<CacheFragment> allCache;
    /** This list is a mapping from predicate indices in the rule structure to fragment and table indices in the E-cache.
     *  The array indices are predicate indices. */
    protected List<TabInfo> predIdx2AllCacheTableInfo;

    /* Monitoring info. The time (in nanoseconds) refers to the corresponding time consumption in the last update of the rule */
    protected long posCacheUpdateTime = 0;
    protected long entCacheUpdateTime = 0;
    protected long allCacheUpdateTime = 0;
    protected long posCacheIndexingTime = 0;
    protected long entCacheIndexingTime = 0;
    protected long allCacheIndexingTime = 0;
    protected long evalTime = 0;
    protected long kbUpdateTime = 0;
    protected long counterexampleTime = 0;
    protected long copyTime = 0;

    /**
     * Initialize the most general rule.
     *
     * @param headPredSymbol      The functor of the head predicate, i.e., the target relation.
     * @param arity               The arity of the functor
     * @param fingerprintCache    The cache of the used fingerprints
     * @param category2TabuSetMap The tabu set of pruned fingerprints
     * @param kb                  The original KB
     */
    public FragmentedCachedRule(
            int headPredSymbol, int arity, Set<Fingerprint> fingerprintCache,
            Map<MultiSet<Integer>, Set<Fingerprint>> category2TabuSetMap, SimpleKb kb
    ) {
        super(headPredSymbol, arity, fingerprintCache, category2TabuSetMap);
        this.kb = kb;

        /* Initialize the E+-cache & T-cache */
        SimpleRelation head_relation = kb.getRelation(headPredSymbol);
        SplitRecords split_records = head_relation.splitByEntailment();
        if (0 == split_records.entailedRecords.length) {
            posCache = new CacheFragment(head_relation, headPredSymbol);
            entCache = new CacheFragment(headPredSymbol, arity);
        } else if (0 == split_records.nonEntailedRecords.length) {
            /* No record to entail, E+-cache and T-cache are both empty */
            posCache = new CacheFragment(headPredSymbol, arity);
            entCache = new CacheFragment(headPredSymbol, arity);
        } else {
            posCache = new CacheFragment(new IntTable(split_records.nonEntailedRecords), headPredSymbol);
            entCache = new CacheFragment(new IntTable(split_records.entailedRecords), headPredSymbol);
        }

        /* Initialize the E-cache */
        allCache = new ArrayList<>();
        predIdx2AllCacheTableInfo = new ArrayList<>();
        predIdx2AllCacheTableInfo.add(null);    // head is not mapped to any fragment

        /* Initial evaluation */
        int pos_ent = split_records.nonEntailedRecords.length;
        int already_ent = split_records.entailedRecords.length;
        double all_ent = Math.pow(kb.totalConstants(), arity);
        this.eval = new Eval(pos_ent, all_ent - already_ent, length);
    }

    /**
     * Initialize a cached rule from a list of predicate.
     *
     * @param structure The structure of the rule.
     * @param category2TabuSetMap The tabu set of pruned fingerprints
     * @param kb The original KB
     */
    public FragmentedCachedRule(
            List<Predicate> structure, Set<Fingerprint> fingerprintCache,
            Map<MultiSet<Integer>, Set<Fingerprint>> category2TabuSetMap, SimpleKb kb
    ) {
        super(structure, fingerprintCache, category2TabuSetMap);
        throw new Error("Not Implemented!");    // Todo: Implement here
    }

    /**
     * Copy constructor
     */
    public FragmentedCachedRule(FragmentedCachedRule another) {
        super(another);
        this.kb = another.kb;

        this.posCache = new CacheFragment(another.posCache);
        this.entCache = new CacheFragment(another.entCache);
        this.allCache = new ArrayList<>(another.allCache.size());
        for (CacheFragment fragment: another.allCache) {
            this.allCache.add(new CacheFragment(fragment));
        }
        this.predIdx2AllCacheTableInfo = new ArrayList<>(another.predIdx2AllCacheTableInfo.size());
        this.predIdx2AllCacheTableInfo.add(null);
        Iterator<TabInfo> itr = another.predIdx2AllCacheTableInfo.listIterator();
        itr.next(); // skip head, which is NULL
        while (itr.hasNext()) {
            this.predIdx2AllCacheTableInfo.add(new TabInfo(itr.next()));
        }
    }

    @Override
    public FragmentedCachedRule clone() {
        long time_start = System.nanoTime();
        FragmentedCachedRule rule =  new FragmentedCachedRule(this);
        long time_done = System.nanoTime();
        rule.copyTime = time_done - time_start;
        return rule;
    }

    /**
     * Calculate the record coverage of the rule.
     */
    @Override
    protected double recordCoverage() {
        return ((double) posCache.countTableSize(HEAD_PRED_IDX)) / kb.getRelation(getHead().predSymbol).totalRows();
    }

    /**
     * Calculate the evaluation of the rule.
     */
    @Override
    protected Eval calculateEval() {
        /* Find all variables in the head */
        long time_start = System.nanoTime();
        final Set<Integer> head_only_lvs = new HashSet<>();  // For the head only LVs
        int head_uv_cnt = 0;
        final Predicate head_pred = getHead();
        for (int argument: head_pred.args) {
            if (Argument.isEmpty(argument)) {
                head_uv_cnt++;
            } else if (Argument.isVariable(argument)) {
                head_only_lvs.add(Argument.decode(argument));    // The GVs will be removed later
            }
        }

        /* Locate and remove GVs */
        List<List<Integer>> gvs_in_all_cache_fragments = new ArrayList<>();
        for (int i = 0; i < allCache.size(); i++) {
            gvs_in_all_cache_fragments.add(new ArrayList<>());
        }
        for (Iterator<Integer> itr = head_only_lvs.iterator(); itr.hasNext(); ) {
            int head_lv = itr.next();
            for (int frag_idx = 0; frag_idx < allCache.size(); frag_idx++) {
                if (allCache.get(frag_idx).hasLv(head_lv)) {
                    gvs_in_all_cache_fragments.get(frag_idx).add(head_lv);
                    itr.remove();
                    break;
                }
            }
        }

        /* Count the number of entailments */
        double all_ent = Math.pow(kb.totalConstants(), head_uv_cnt + head_only_lvs.size());
        for (int i = 0; i < allCache.size(); i++) {
            CacheFragment fragment = allCache.get(i);
            List<Integer> vids = gvs_in_all_cache_fragments.get(i);
            all_ent *= fragment.countCombinations(vids);
        }
        int new_pos_ent = posCache.countTableSize(HEAD_PRED_IDX);
        int already_ent = entCache.countTableSize(HEAD_PRED_IDX);
        long time_done = System.nanoTime();
        evalTime = time_done - time_start;

        /* Update evaluation score */
        /* Those already proved should be excluded from the entire entailment set. Otherwise, they are counted as negative ones */
        return new Eval(new_pos_ent, all_ent - already_ent, length);
    }

    /**
     * Update the cache indices before specialization. E.g., right after the rule is selected as one of the beams.
     */
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
        posCacheIndexingTime = time_pos_done - time_start;
        entCacheIndexingTime = time_ent_done - time_pos_done;
        allCacheIndexingTime = time_all_done - time_ent_done;
    }

    /**
     * Update the E+-cache for case 1 specialization.
     *
     * Note: all indices should be up-to-date
     */
    @Override
    protected UpdateStatus cvt1Uv2ExtLvHandlerPreCvg(int predIdx, int argIdx, int varId) {
        long time_start = System.nanoTime();
        posCache.updateCase1a(predIdx, argIdx, varId);
        long time_done = System.nanoTime();
        posCacheUpdateTime = time_done - time_start;
        return UpdateStatus.NORMAL;
    }

    /**
     * Update fragment indices and predicate index mappings when two fragments merge together.
     *
     * NOTE: This should be called BEFORE the two fragments are merged
     * NOTE: This method will remove the reference to the merging fragment
     *
     * @param baseFragmentIdx    The index of the base fragment. This fragment will not be changed.
     * @param mergingFragmentIdx The index of the fragment that will be merged into the base. This fragment will be removed after the merge
     */
    protected void mergeFragmentIndices(final int baseFragmentIdx, final int mergingFragmentIdx) {
        /* Replace the merging fragment with the last */
        final int last_frag_idx = allCache.size() - 1;
        final int tabs_in_base = allCache.get(baseFragmentIdx).partAssignedRule.size();
        allCache.set(mergingFragmentIdx, allCache.get(last_frag_idx));

        /* Update predicate-fragment mapping */
        Iterator<TabInfo> itr = predIdx2AllCacheTableInfo.listIterator();
        itr.next(); // skip head
        while (itr.hasNext()) {
            TabInfo tab_info = itr.next();
            if (mergingFragmentIdx == tab_info.fragmentIdx) {
                tab_info.fragmentIdx = baseFragmentIdx;
                tab_info.tabIdx += tabs_in_base;
            }
            tab_info.fragmentIdx = (last_frag_idx == tab_info.fragmentIdx) ? mergingFragmentIdx : tab_info.fragmentIdx;
        }

        /* Remove the last fragment */
        allCache.remove(last_frag_idx);
    }

    /**
     * Update the T-cache and E-cache for case 1 specialization.
     *
     * Note: all indices should be up-to-date
     */
    @Override
    protected UpdateStatus cvt1Uv2ExtLvHandlerPostCvg(int predIdx, int argIdx, int varId) {
        long time_start = System.nanoTime();
        entCache.updateCase1a(predIdx, argIdx, varId);
        long time_ent_done = System.nanoTime();

        if (HEAD_PRED_IDX != predIdx) { // No need to update the E-cache if the update is in the head
            final TabInfo tab_info = predIdx2AllCacheTableInfo.get(predIdx);
            final CacheFragment fragment = allCache.get(tab_info.fragmentIdx);
            boolean cache_empty = false;
            if (fragment.hasLv(varId)) {
                /* Update within the fragment */
                fragment.updateCase1a(tab_info.tabIdx, argIdx, varId);
                cache_empty = fragment.isEmpty();
            } else {
                /* Find the fragment that has the LV */
                boolean not_found = true;
                for (int frag_idx2 = 0; frag_idx2 < allCache.size(); frag_idx2++) {
                    CacheFragment fragment2 = allCache.get(frag_idx2);
                    if (fragment2.hasLv(varId)) {
                        /* Find the LV in another fragment, merge the two fragments */
                        not_found = false;
                        final int fragment_tab_idx = tab_info.tabIdx;
                        mergeFragmentIndices(frag_idx2, tab_info.fragmentIdx);
                        fragment2.updateCase1c(fragment, fragment_tab_idx, argIdx, varId);
                        cache_empty = fragment2.isEmpty();
                        break;
                    }
                }
                if (not_found) {
                    /* Update within the target fragment */
                    /* The var will be a PLV, so this fragment will not be empty */
                    fragment.updateCase1a(tab_info.tabIdx, argIdx, varId);
                }
            }
            if (cache_empty) {
                for (CacheFragment _fragment: allCache) {
                    _fragment.clear();
                }
            }
        }
        long time_all_done = System.nanoTime();
        entCacheUpdateTime = time_ent_done - time_start;
        allCacheUpdateTime = time_all_done - time_ent_done;

        return UpdateStatus.NORMAL;
    }

    /**
     * Update the E+-cache for case 2 specialization.
     *
     * Note: all indices should be up-to-date
     */
    @Override
    protected UpdateStatus cvt1Uv2ExtLvHandlerPreCvg(Predicate newPredicate, int argIdx, int varId) {
        long time_start = System.nanoTime();
        SimpleRelation new_relation = kb.getRelation(newPredicate.predSymbol);
        posCache.updateCase1b(new_relation, new_relation.id, argIdx, varId);
        long time_done = System.nanoTime();
        posCacheUpdateTime = time_done - time_start;
        return UpdateStatus.NORMAL;
    }

    /**
     * Update the T-cache and the E-cache for case 2 specialization.
     *
     * Note: all indices should be up-to-date
     */
    @Override
    protected UpdateStatus cvt1Uv2ExtLvHandlerPostCvg(Predicate newPredicate, int argIdx, int varId) {
        long time_start = System.nanoTime();
        SimpleRelation new_relation = kb.getRelation(newPredicate.predSymbol);
        entCache.updateCase1b(new_relation, new_relation.id, argIdx, varId);
        long time_ent_done = System.nanoTime();

        CacheFragment updated_fragment = null;
        for (int frag_idx = 0; frag_idx < allCache.size(); frag_idx++) {
            CacheFragment fragment = allCache.get(frag_idx);
            if (fragment.hasLv(varId)) {
                /* Append the new relation to the fragment that contains the LV */
                updated_fragment = fragment;
                predIdx2AllCacheTableInfo.add(new TabInfo(frag_idx, fragment.partAssignedRule.size()));
                fragment.updateCase1b(new_relation, new_relation.id, argIdx, varId);
                break;
            }
        }
        if (null == updated_fragment) {
            /* The LV has not been included in body yet. Create a new fragment */
            updated_fragment = new CacheFragment(new_relation, new_relation.id);
            updated_fragment.updateCase1a(0, argIdx, varId);
            predIdx2AllCacheTableInfo.add(new TabInfo(allCache.size(), 0));
            allCache.add(updated_fragment);
        }
        if (updated_fragment.isEmpty()) {
            for (CacheFragment _fragment: allCache) {
                _fragment.clear();
            }
        }
        long time_all_done = System.nanoTime();
        entCacheUpdateTime = time_ent_done - time_start;
        allCacheUpdateTime = time_all_done - time_ent_done;

        return UpdateStatus.NORMAL;
    }

    /**
     * Update the E+-cache for case 3 specialization.
     *
     * Note: all indices should be up-to-date
     */
    @Override
    protected UpdateStatus cvt2Uvs2NewLvHandlerPreCvg(int predIdx1, int argIdx1, int predIdx2, int argIdx2) {
        long time_start = System.nanoTime();
        posCache.updateCase2a(predIdx1, argIdx1, predIdx2, argIdx2, usedLimitedVars() - 1);
        long time_done = System.nanoTime();
        posCacheUpdateTime = time_done - time_start;
        return UpdateStatus.NORMAL;
    }

    /**
     * Update the T-cache and the E-cache for case 3 specialization.
     *
     * Note: all indices should be up-to-date
     */
    @Override
    protected UpdateStatus cvt2Uvs2NewLvHandlerPostCvg(int predIdx1, int argIdx1, int predIdx2, int argIdx2) {
        long time_start = System.nanoTime();
        final int new_vid = usedLimitedVars() - 1;
        entCache.updateCase2a(predIdx1, argIdx1, predIdx2, argIdx2, new_vid);
        long time_ent_done = System.nanoTime();

        TabInfo tab_info1 = predIdx2AllCacheTableInfo.get(predIdx1);
        TabInfo tab_info2 = predIdx2AllCacheTableInfo.get(predIdx2);
        CacheFragment fragment1 = null;
        CacheFragment fragment2 = null;
        if (HEAD_PRED_IDX == predIdx1) {
            if (HEAD_PRED_IDX != predIdx2) {
                /* Update the fragment of predIdx2 only */
                fragment2 = allCache.get(tab_info2.fragmentIdx);
                fragment2.updateCase1a(tab_info2.tabIdx, argIdx2, new_vid);
            }   //  Otherwise, update is in the head only, no need to change E-cache
        } else {
            fragment1 = allCache.get(tab_info1.fragmentIdx);
            if (HEAD_PRED_IDX != predIdx2) {
                /* Update two predicates together */
                if (tab_info1.fragmentIdx == tab_info2.fragmentIdx) {
                    /* Update within one fragment */
                    fragment1.updateCase2a(tab_info1.tabIdx, argIdx1, tab_info2.tabIdx, argIdx2, new_vid);
                } else {
                    /* Merge two fragments */
                    fragment2 = allCache.get(tab_info2.fragmentIdx);
                    final int fragment2_tab_idx = tab_info2.tabIdx;
                    mergeFragmentIndices(tab_info1.fragmentIdx, tab_info2.fragmentIdx);
                    fragment1.updateCase2c(tab_info1.tabIdx, argIdx1, fragment2, fragment2_tab_idx, argIdx2, new_vid);
                    fragment2 = null;
                }
            } else {
                /* Update the fragment of predIdx1 only */
                fragment1.updateCase1a(tab_info1.tabIdx, argIdx1, new_vid);
            }
        }
        if ((null != fragment1 && fragment1.isEmpty()) || (null != fragment2 && fragment2.isEmpty())) {
            for (CacheFragment _fragment: allCache) {
                _fragment.clear();
            }
        }
        long time_all_done = System.nanoTime();
        entCacheUpdateTime = time_ent_done - time_start;
        allCacheUpdateTime = time_all_done - time_ent_done;

        return UpdateStatus.NORMAL;
    }

    /**
     * Update the E+-cache for case 4 specialization.
     *
     * Note: all indices should be up-to-date
     */
    @Override
    protected UpdateStatus cvt2Uvs2NewLvHandlerPreCvg(Predicate newPredicate, int argIdx1, int predIdx2, int argIdx2) {
        long time_start = System.nanoTime();
        SimpleRelation new_relation = kb.getRelation(newPredicate.predSymbol);
        posCache.updateCase2b(new_relation, new_relation.id, argIdx1, predIdx2, argIdx2, usedLimitedVars() - 1);
        long time_done = System.nanoTime();
        posCacheUpdateTime = time_done - time_start;
        return UpdateStatus.NORMAL;
    }

    /**
     * Update the T-cache and the E-cache for case 4 specialization.
     *
     * Note: all indices should be up-to-date
     */
    @Override
    protected UpdateStatus cvt2Uvs2NewLvHandlerPostCvg(Predicate newPredicate, int argIdx1, int predIdx2, int argIdx2) {
        long time_start = System.nanoTime();
        SimpleRelation new_relation = kb.getRelation(newPredicate.predSymbol);
        final int new_vid = usedLimitedVars() - 1;
        entCache.updateCase2b(new_relation, new_relation.id, argIdx1, predIdx2, argIdx2, new_vid);
        long time_ent_done = System.nanoTime();

        if (HEAD_PRED_IDX == predIdx2) {   // One is the head and the other is not
            /* Create a new fragment for the new predicate */
            predIdx2AllCacheTableInfo.add(new TabInfo(allCache.size(), 0));
            CacheFragment fragment = new CacheFragment(new_relation, new_relation.id);
            fragment.updateCase1a(0, argIdx1, new_vid);
            allCache.add(fragment);
        } else {    // Both are in the body
            TabInfo tab_info = predIdx2AllCacheTableInfo.get(predIdx2);
            CacheFragment fragment = allCache.get(tab_info.fragmentIdx);
            predIdx2AllCacheTableInfo.add(new TabInfo(tab_info.fragmentIdx, fragment.partAssignedRule.size()));
            fragment.updateCase2b(new_relation, new_relation.id, argIdx1, tab_info.tabIdx, argIdx2, new_vid);
            if (fragment.isEmpty()) {
                for (CacheFragment _fragment: allCache) {
                    _fragment.clear();
                }
            }
        }
        long time_all_done = System.nanoTime();
        entCacheUpdateTime = time_ent_done - time_start;
        allCacheUpdateTime = time_all_done - time_ent_done;

        return UpdateStatus.NORMAL;
    }

    /**
     * Update the E+-cache for case 5 specialization.
     *
     * Note: all indices should be up-to-date
     */
    @Override
    protected UpdateStatus cvt1Uv2ConstHandlerPreCvg(int predIdx, int argIdx, int constant) {
        long time_start = System.nanoTime();
        posCache.updateCase3(predIdx, argIdx, constant);
        long time_done = System.nanoTime();
        posCacheUpdateTime = time_done - time_start;
        return UpdateStatus.NORMAL;
    }

    /**
     * Update the T-cache and the E-cache for case 5 specialization.
     *
     * Note: all indices should be up-to-date
     */
    @Override
    protected UpdateStatus cvt1Uv2ConstHandlerPostCvg(int predIdx, int argIdx, int constant) {
        long time_start = System.nanoTime();
        entCache.updateCase3(predIdx, argIdx, constant);
        long time_ent_done = System.nanoTime();

        if (HEAD_PRED_IDX != predIdx) { // No need to update the E-cache if the update is in the head
            TabInfo tab_info = predIdx2AllCacheTableInfo.get(predIdx);
            CacheFragment fragment = allCache.get(tab_info.fragmentIdx);
            fragment.updateCase3(tab_info.tabIdx, argIdx, constant);
            if (fragment.isEmpty()) {
                for (CacheFragment _fragment: allCache) {
                    _fragment.clear();
                }
            }
        }
        long time_all_done = System.nanoTime();
        entCacheUpdateTime = time_ent_done - time_start;
        allCacheUpdateTime = time_all_done - time_ent_done;

        return UpdateStatus.NORMAL;
    }

    /**
     * Generalization is not applicable in cached rules. This function always returns "UpdateStatus.INVALID"
     *
     * @return UpdateStatus.INVALID
     */
    @Override
    @Deprecated
    protected UpdateStatus rmAssignedArgHandlerPreCvg(int predIdx, int argIdx) {
        return UpdateStatus.INVALID;
    }

    /**
     * Generalization is not applicable in cached rules. This function always returns "UpdateStatus.INVALID"
     *
     * @return UpdateStatus.INVALID
     */
    @Override
    @Deprecated
    protected UpdateStatus rmAssignedArgHandlerPostCvg(int predIdx, int argIdx) {
        return UpdateStatus.INVALID;
    }

    /**
     * Find one piece of evidence for each positively entailed record and mark the positive entailments in the KB.
     *
     * @return Batched evidence
     */
    @Override
    public EvidenceBatch getEvidenceAndMarkEntailment() {
        long time_start = System.nanoTime();
        final int[] pred_symbols_in_rule = new int[structure.size()];
        for (int i = 0; i < pred_symbols_in_rule.length; i++) {
            pred_symbols_in_rule[i] = structure.get(i).predSymbol;
        }
        EvidenceBatch evidence_batch = new EvidenceBatch(pred_symbols_in_rule);

        SimpleRelation target_relation = kb.getRelation(getHead().predSymbol);
        for (final List<CB> cache_entry: posCache.entries) {
            /* Find the grounding body */
            final int[][] grounding_template = new int[pred_symbols_in_rule.length][];
            for (int pred_idx = FIRST_BODY_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
                grounding_template[pred_idx] = cache_entry.get(pred_idx).complianceSet[0];
            }

            /* Find all entailed records */
            int[][] head_records = cache_entry.get(HEAD_PRED_IDX).complianceSet;
            if (1 < head_records.length) {
                for (int[] head_record :head_records){
                    if (target_relation.entailIfNot(head_record)) {
                        final int[][] grounding = grounding_template.clone();
                        grounding[HEAD_PRED_IDX] = head_record;
                        evidence_batch.evidenceList.add(grounding);
                    }
                }
            } else {
                int[] head_record = head_records[0];
                if (target_relation.entailIfNot(head_record)) {
                    grounding_template[HEAD_PRED_IDX] = head_record;
                    evidence_batch.evidenceList.add(grounding_template);
                }
            }
        }
        long time_done = System.nanoTime();
        kbUpdateTime = time_done - time_start;

        return evidence_batch;
    }

    /**
     * Find the counterexamples generated by the rule.
     */
    @Override
    public Set<Record> getCounterexamples() {
        long time_start = System.nanoTime();

        /* Find all variables in the head */
        final Map<Integer, List<Integer>> head_only_vid_2_loc_map = new HashMap<>();  // GVs will be removed later
        int uv_id = usedLimitedVars();
        final Predicate head_pred = getHead();
        for (int arg_idx = 0; arg_idx < head_pred.arity(); arg_idx++) {
            final int argument = head_pred.args[arg_idx];
            if (Argument.isEmpty(argument)) {
                List<Integer> list = new ArrayList<>();
                list.add(arg_idx);
                head_only_vid_2_loc_map.put(uv_id, list);
                uv_id++;
            } else if (Argument.isVariable(argument)) {
                head_only_vid_2_loc_map.computeIfAbsent(Argument.decode(argument), k -> new ArrayList<>()).add(arg_idx);
            }
        }

        Set<Record> head_templates = new HashSet<>();
        if (allCache.isEmpty()) {
            /* If body is empty, create a single empty head template. The execution flow will go to line [a] */
            head_templates.add(new Record(head_pred.args.clone())); // copy constant in head if any
        } else {
            /* Locate and remove GVs in head */
            List<Integer>[] gvids_in_all_cache_fragments = new List[allCache.size()];
            List<List<Integer>>[] head_arg_idx_lists = new List[allCache.size()];
            for (int frag_idx = 0; frag_idx < allCache.size(); frag_idx++) {
                gvids_in_all_cache_fragments[frag_idx] = new ArrayList<>();
                head_arg_idx_lists[frag_idx] = new ArrayList<>();
            }
            for (Iterator<Map.Entry<Integer, List<Integer>>> itr = head_only_vid_2_loc_map.entrySet().iterator(); itr.hasNext(); ) {
                Map.Entry<Integer, List<Integer>> entry = itr.next();
                int head_vid = entry.getKey();
                List<Integer> head_var_arg_idxs = entry.getValue();
                for (int frag_idx = 0; frag_idx < allCache.size(); frag_idx++) {
                    if (allCache.get(frag_idx).hasLv(head_vid)) {
                        gvids_in_all_cache_fragments[frag_idx].add(head_vid);
                        head_arg_idx_lists[frag_idx].add(head_var_arg_idxs);
                        itr.remove();
                        break;
                    }
                }
            }

            /* Enumerate GV bindings in fragments and generate head templates */
            Set<Record>[] bindings_in_fragments = new Set[allCache.size()];
            for (int frag_idx = 0; frag_idx < allCache.size(); frag_idx++) {
                bindings_in_fragments[frag_idx] = allCache.get(frag_idx).enumerateCombinations(
                        gvids_in_all_cache_fragments[frag_idx]
                );
            }
            generateHeadTemplates(head_templates, bindings_in_fragments, head_arg_idx_lists, head_pred.args.clone(), 0);
        }

        /* Extend head templates */
        SimpleRelation target_relation = kb.getRelation(head_pred.predSymbol);
        if (head_only_vid_2_loc_map.isEmpty()) {
            /* No need to extend UVs */
            head_templates.removeIf(r -> target_relation.hasRow(r.args));
            long time_done = System.nanoTime();
            counterexampleTime = time_done - time_start;

            return head_templates;
        } else {
            /* [a] Extend UVs in the templates */
            final Set<Record> counter_example_set = new HashSet<>();
            List<Integer>[] head_only_var_loc_lists = new List[head_only_vid_2_loc_map.size()];
            int i = 0;
            for (List<Integer> head_only_var_loc_list: head_only_vid_2_loc_map.values()) {
                head_only_var_loc_lists[i] = head_only_var_loc_list;
                i++;
            }
            for (Record head_template: head_templates) {
                expandHeadUvs4CounterExamples(
                        target_relation, counter_example_set, head_template, head_only_var_loc_lists, 0
                );
            }
            long time_done = System.nanoTime();
            counterexampleTime = time_done - time_start;

            return counter_example_set;
        }
    }

    /**
     * Recursively add PLV bindings to the linked head arguments.
     *
     * NOTE: This should only be called when body is not empty
     *
     * @param headTemplates       The set of head templates
     * @param bindingsInFragments The bindings of LVs grouped by cache fragment
     * @param headArgIdxLists     The linked head argument indices for LV groups
     * @param template            A template of the head templates
     * @param fragIdx             The index of the fragment
     */
    protected void generateHeadTemplates(
            final Set<Record> headTemplates, final Set<Record>[] bindingsInFragments,
            final List<List<Integer>>[] headArgIdxLists, int[] template, final int fragIdx
    ) {
        final Set<Record> bindings_in_fragment = bindingsInFragments[fragIdx];
        final List<List<Integer>> gv_links = headArgIdxLists[fragIdx];
        if (fragIdx == bindingsInFragments.length - 1) {
            /* Finish the last group of bindings, add to the template set */
            for (Record binding: bindings_in_fragment) {
                for (int i = 0; i < binding.args.length; i++) {
                    List<Integer> head_arg_idxs = gv_links.get(i);
                    for (int head_arg_idx: head_arg_idxs) {
                        template[head_arg_idx] = binding.args[i];
                    }
                }
                headTemplates.add(new Record(template.clone()));
            }
        } else {
            /* Add current binding to the template and move to the next recursion */
            for (Record binding: bindings_in_fragment) {
                for (int i = 0; i < binding.args.length; i++) {
                    List<Integer> head_arg_idxs = gv_links.get(i);
                    for (int head_arg_idx: head_arg_idxs) {
                        template[head_arg_idx] = binding.args[i];
                    }
                }
                generateHeadTemplates(headTemplates, bindingsInFragments, headArgIdxLists, template, fragIdx + 1);
            }
        }
    }

    /**
     * Recursively expand UVs in the template and add to counterexample set
     *
     * NOTE: This should only be called when there is at least one head-only var in the head
     *
     * @param targetRelation  The target relation
     * @param counterexamples The counterexample set
     * @param template        The template record
     * @param idx             The index of UVs
     * @param varLocs         The locations of UVs
     */
    protected void expandHeadUvs4CounterExamples(
            final SimpleRelation targetRelation, final Set<Record> counterexamples, final Record template,
            final List<Integer>[] varLocs, final int idx
    ) {
        final List<Integer> locations = varLocs[idx];
        if (idx < varLocs.length - 1) {
            /* Expand current UV and move to the next recursion */
            for (int constant_symbol = 1; constant_symbol <= kb.totalConstants(); constant_symbol++) {
                final int argument = Argument.constant(constant_symbol);
                for (int loc: locations) {
                    template.args[loc] = argument;
                }
                expandHeadUvs4CounterExamples(targetRelation, counterexamples, template, varLocs, idx + 1);
            }
        } else {
            /* Expand the last UV and add to counterexample set if it is */
            for (int constant_symbol = 1; constant_symbol <= kb.totalConstants(); constant_symbol++) {
                final int argument = Argument.constant(constant_symbol);
                for (int loc: locations) {
                    template.args[loc] = argument;
                }
                if (!targetRelation.hasRow(template.args)) {
                    counterexamples.add(new Record(template.args.clone()));
                }
            }
        }
    }

    @Override
    public void releaseMemory() {
        posCache = null;
        entCache = null;
        allCache = null;
    }
}
