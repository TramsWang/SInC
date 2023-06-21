package sinc2.impl.negsamp;

import sinc2.common.ArgLocation;
import sinc2.common.Argument;
import sinc2.common.Predicate;
import sinc2.common.Record;
import sinc2.kb.IntTable;
import sinc2.util.ArrayOperation;

import java.util.*;

/**
 * A cache fragment is a structure where multiple relations are joined together and are linked by LVs. There is no relation
 * in a fragment that shares no LV with the remaining part.
 *
 * The update of a fragment can be divided into the following cases:
 *   1a: Convert a UV to existing LV in current fragment
 *   1b: Append a new relation and convert a UV to existing LV
 *   1c: Convert a UV to existing LV and merge with another fragment
 *   2a: Convert two UVs to a new LV
 *   2b: Append a new relation and convert two UVs to a new LV (one in the original part, one in the new relation)
 *   2c: Merge two fragments by converting two UVs to a new LV (one UV in one fragment, the other UV in the other fragment)
 *   3: Convert a UV to a constant
 */
public class CacheFragment implements Iterable<List<CB>> {

    /**
     * Used for quickly locating an LV (PLV)
     */
    static protected class VarInfo {
        final int tabIdx;
        final int colIdx;
        boolean isPlv;

        public VarInfo(int tabIdx, int colIdx, boolean isPlv) {
            this.tabIdx = tabIdx;
            this.colIdx = colIdx;
            this.isPlv = isPlv;
        }

        public VarInfo(VarInfo another) {
            this.tabIdx = another.tabIdx;
            this.colIdx = another.colIdx;
            this.isPlv = another.isPlv;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;
            VarInfo varInfo = (VarInfo) o;
            return tabIdx == varInfo.tabIdx && colIdx == varInfo.colIdx && isPlv == varInfo.isPlv;
        }

        @Override
        public int hashCode() {
            return Objects.hash(tabIdx, colIdx, isPlv);
        }
    }

    /** Partially assigned rule structure for this fragment. Predicate symbols are unnecessary here, but useful for debugging */
    protected List<Predicate> partAssignedRule;
    /** Compact cache entries, each entry is a list of CB */
    protected List<List<CB>> entries;   // Todo: If CB can be fully copy-on-write, that is, no two CBs in the memory contains the same compliance set, the specialization and counting can be faster
    /** A list of LV info. Each index is the ID of an LV */
    protected List<VarInfo> varInfoList;

    public CacheFragment(IntTable firstRelation, int relationSymbol) {
        partAssignedRule = new ArrayList<>();
        entries = new ArrayList<>();
        varInfoList = new ArrayList<>();

        partAssignedRule.add(new Predicate(relationSymbol, firstRelation.totalCols()));
        List<CB> first_entry = new ArrayList<>();
        first_entry.add(new CB(firstRelation.getAllRows(), firstRelation));
        entries.add(first_entry);
    }

    /**
     * This constructor is used to construct an empty fragment
     */
    public CacheFragment(int relationSymbol, int arity) {
        partAssignedRule = new ArrayList<>();
        entries = new ArrayList<>();
        varInfoList = new ArrayList<>();
        partAssignedRule.add(new Predicate(relationSymbol, arity));
    }

    /**
     * This constructor is used for constructing a cache fragment given a certain fragment of a rule and a list of tables
     * corresponding to the rule fragment.
     *
     * @param structure A fragment of a rule
     * @param tables    A list of tables
     */
    public CacheFragment(List<Predicate> structure, IntTable[] tables) {
        class ConstRestriction {        // This class is used for representing constant restrictions in the rule
            public final int argIdx;    // The argument index of the constant in a predicate
            public final int constant;  // The constant

            public ConstRestriction(int argIdx, int constant) {
                this.argIdx = argIdx;
                this.constant = constant;
            }
        }

        /* Construct structure */
        entries = new ArrayList<>();
        partAssignedRule = new ArrayList<>(structure.size());
        int max_vid = -1;
        List<ConstRestriction>[] const_restrictions_in_tabs = new List[structure.size()];   // Table index as the array index
        for (Predicate predicate : structure) {
            partAssignedRule.add(new Predicate(predicate));
            for (int argument : predicate.args) {
                if (Argument.isVariable(argument)) {
                    max_vid = Math.max(max_vid, Argument.decode(argument));
                }
            }
        }
        varInfoList = new ArrayList<>(max_vid + 1);
        List<ArgLocation>[] var_id_locs = new List[max_vid + 1];    // Var ID as the index of the array
        for (int vid = 0; vid <= max_vid; vid++) {
            varInfoList.add(null);
        }

        /* Find variable locations and constant restrictions */
        for (int pred_idx = 0; pred_idx < partAssignedRule.size(); pred_idx++) {
            Predicate predicate = partAssignedRule.get(pred_idx);
            for (int arg_idx = 0; arg_idx < predicate.arity(); arg_idx++) {
                int argument = predicate.args[arg_idx];
                if (Argument.isVariable(argument)) {
                    int vid = Argument.decode(argument);
                    if (null == var_id_locs[vid]) {
                        var_id_locs[vid] = new ArrayList<>();
                    }
                    var_id_locs[vid].add(new ArgLocation(pred_idx, arg_idx));
                } else if (Argument.isConstant(argument)) {
                    if (null == const_restrictions_in_tabs[pred_idx]) {
                        const_restrictions_in_tabs[pred_idx] = new ArrayList<>();
                    }
                    const_restrictions_in_tabs[pred_idx].add(new ConstRestriction(arg_idx, Argument.decode(argument)));
                }   // no need to record empty argument
            }
        }
        for (int vid = 0; vid < var_id_locs.length; vid++) {
            List<ArgLocation> var_locs = var_id_locs[vid];
            if (null != var_locs && !var_locs.isEmpty()) {
                /* Record an LV or PLV */
                ArgLocation var_loc = var_locs.get(0);
                varInfoList.set(vid, new VarInfo(var_loc.predIdx, var_loc.argIdx, var_locs.size() == 1));
            }
        }

        /* Filter records in tables by constant restrictions */
        IntTable[] filtered_tables = new IntTable[tables.length];
        boolean is_empty = false;
        for (int tab_idx = 0; tab_idx < filtered_tables.length; tab_idx++) {
            IntTable original_table = tables[tab_idx];
            if (0 == original_table.totalRows()) {
                is_empty = true;
                break;
            }
            List<ConstRestriction> const_restrictions = const_restrictions_in_tabs[tab_idx];
            if (null == const_restrictions) {
                filtered_tables[tab_idx] = original_table;
            } else {
                /* Filter by constant restrictions */
                List<int[]> records_complied_to_constants = new ArrayList<>(original_table.totalRows());
                for (int[] record : original_table.getAllRows()) {
                    boolean match_all = true;
                    for (ConstRestriction restriction: const_restrictions) {
                        if (restriction.constant != record[restriction.argIdx]) {
                            match_all = false;
                            break;
                        }
                    }
                    if (match_all) {
                        records_complied_to_constants.add(record);
                    }
                }

                if (records_complied_to_constants.size() == original_table.totalRows()) {
                    filtered_tables[tab_idx] = original_table;
                } else if (records_complied_to_constants.isEmpty()) {
                    is_empty = true;
                    break;
                } else {
                    filtered_tables[tab_idx] = new IntTable(records_complied_to_constants.toArray(new int[0][]));
                }
            }
        }

        /* Construct cache entries */
        if (is_empty) {
            return;
        }
        List<CB> first_entry = new ArrayList<>();
        for (IntTable table: filtered_tables) {
            first_entry.add(new CB(table.getAllRows(), table));
        }
        entries.add(first_entry);
        for (List<ArgLocation> var_locs: var_id_locs) {
            if (null != var_locs && var_locs.size() >= 2) {
                splitCacheEntriesByLvs(var_locs);
            }
        }
    }

    /**
     * This method split cache entries by a given LV. The LV may occur more than twice in the fragment structure.
     *
     * @param lvLocations Locations of the LV in the structure
     */
    protected void splitCacheEntriesByLvs(List<ArgLocation> lvLocations) {
        /* Group arguments indices in predicates */
        List<Integer>[] lv_locs_in_tabs = new List[partAssignedRule.size()];  // Table index as the array index
        int preds_with_lvs = 0;
        for (ArgLocation lv_loc: lvLocations) {
            if (null == lv_locs_in_tabs[lv_loc.predIdx]) {
                lv_locs_in_tabs[lv_loc.predIdx] = new ArrayList<>();
                preds_with_lvs++;
            }
            lv_locs_in_tabs[lv_loc.predIdx].add(lv_loc.argIdx);
        }

        /* Filter tables with more than 1 LVs */
        int[] tab_idxs_with_lvs = new int[preds_with_lvs];
        int[] var_arg_idx_in_preds = new int[preds_with_lvs];
        int pred_idx_with_lv = 0;
        for (int tab_idx = 0; tab_idx < partAssignedRule.size(); tab_idx++) {
            List<Integer> lv_arg_idxs = lv_locs_in_tabs[tab_idx];
            if (null != lv_arg_idxs) {
                tab_idxs_with_lvs[pred_idx_with_lv] = tab_idx;
                var_arg_idx_in_preds[pred_idx_with_lv] = lv_arg_idxs.get(0);
                pred_idx_with_lv++;
                if (1 < lv_arg_idxs.size()) {
                    /* Filter the CBs in entries */
                    Iterator<List<CB>> entry_itr = entries.listIterator();
                    while (entry_itr.hasNext()) {
                        List<CB> entry = entry_itr.next();
                        CB cb = entry.get(tab_idx);
                        List<int[]> new_comp_set = new ArrayList<>();
                        for (int[] record : cb.complianceSet) {
                            boolean all_matched = true;
                            Iterator<Integer> arg_idx_itr = lv_arg_idxs.iterator();
                            final int val = record[arg_idx_itr.next()];
                            while (arg_idx_itr.hasNext()) {
                                if (val != record[arg_idx_itr.next()]) {
                                    all_matched = false;
                                    break;
                                }
                            }
                            if (all_matched) {
                                new_comp_set.add(record);
                            }
                        }
                        if (cb.complianceSet.length != new_comp_set.size()) {
                            if (new_comp_set.isEmpty()) {
                                entry_itr.remove();
                            } else {
                                /* Change CB */
                                int[][] new_comp_set_rows = new_comp_set.toArray(new int[0][]);
                                entry.set(tab_idx, new CB(new_comp_set_rows, new IntTable(new_comp_set_rows)));
                            }
                        }
                    }
                }
            }
        }

        /* Update cache by splitting entries */
        List<List<CB>> new_entries = new ArrayList<>();
        IntTable[] tables = new IntTable[preds_with_lvs];
        for (List<CB> cache_entry: entries) {
            /* Match values */
            for (int i = 0; i < tab_idxs_with_lvs.length; i++) {
                tables[i] = cache_entry.get(tab_idxs_with_lvs[i]).indices;
            }
            List<int[][]>[] slices = IntTable.matchSlices(tables, var_arg_idx_in_preds);
            int slices_cnt = slices[0].size();
            for (int slice_idx = 0; slice_idx < slices_cnt; slice_idx++) {
                List<CB> new_entry = new ArrayList<>(cache_entry);
                for (int i = 0; i < tab_idxs_with_lvs.length; i++) {
                    int[][] slice = slices[i].get(slice_idx);
                    if (slice.length != tables[i].totalRows()) {
                        new_entry.set(tab_idxs_with_lvs[i], new CB(slice, new IntTable(slice)));
                    }
                }
                new_entries.add(new_entry);
            }
        }
        entries = new_entries;
    }

    public CacheFragment(CacheFragment another) {
        this.partAssignedRule = new ArrayList<>(another.partAssignedRule.size());
        for (Predicate predicate: another.partAssignedRule) {
            this.partAssignedRule.add(new Predicate(predicate));
        }
        this.entries = another.entries; // The caches can be simply copied, as the list should not be modified, but directly replaced (Copy-on-write)
        this.varInfoList = new ArrayList<>(another.varInfoList.size());
        for (VarInfo var_info: another.varInfoList) {
            this.varInfoList.add((null == var_info) ? null : new VarInfo(var_info));
        }
    }

    /**
     * Split cache entries according to two columns in the fragment.
     */
    protected void splitCacheEntries(int tabIdx1, int colIdx1, int tabIdx2, int colIdx2) {
        List<List<CB>> new_entries = new ArrayList<>();
        if (tabIdx1 == tabIdx2) {
            for (List<CB> cache_entry: entries) {
                CB cb = cache_entry.get(tabIdx1);
                List<int[][]> slices = cb.indices.matchSlices(colIdx1, colIdx2);
                for (int[][] slice: slices) {
                    CB new_cb = new CB(slice);
                    List<CB> new_entry = new ArrayList<>(cache_entry);
                    new_entry.set(tabIdx1, new_cb);
                    new_entries.add(new_entry);
                }
            }
        } else {
            for (List<CB> cache_entry : entries) {
                CB cb1 = cache_entry.get(tabIdx1);
                CB cb2 = cache_entry.get(tabIdx2);
                IntTable.MatchedSubTables slices = IntTable.matchSlices(cb1.indices, colIdx1, cb2.indices, colIdx2);
                for (int i = 0; i < slices.slices1.size(); i++) {
                    CB new_cb1 = new CB(slices.slices1.get(i));
                    CB new_cb2 = new CB(slices.slices2.get(i));
                    List<CB> new_entry = new ArrayList<>(cache_entry);
                    new_entry.set(tabIdx1, new_cb1);
                    new_entry.set(tabIdx2, new_cb2);
                    new_entries.add(new_entry);
                }
            }
        }
        entries = new_entries;
    }

    /**
     * Append a new relation and split cache entries according to two columns in the fragment. One column is in one of
     * the original relations, and the other is in the appended relation.
     */
    protected void splitCacheEntries(int tabIdx1, int colIdx1, IntTable newRelation, int colIdx2) {
        List<List<CB>> new_entries = new ArrayList<>();
        for (List<CB> cache_entry : entries) {
            CB cb1 = cache_entry.get(tabIdx1);
            IntTable.MatchedSubTables slices = IntTable.matchSlices(cb1.indices, colIdx1, newRelation, colIdx2);
            for (int i = 0; i < slices.slices1.size(); i++) {
                CB new_cb1 = new CB(slices.slices1.get(i));
                CB new_cb2 = new CB(slices.slices2.get(i));
                List<CB> new_entry = new ArrayList<>(cache_entry);
                new_entry.set(tabIdx1, new_cb1);
                new_entry.add(new_cb2);
                new_entries.add(new_entry);
            }
        }
        entries = new_entries;
    }

    /**
     * Match a column to another that has already been assigned an LV.
     *
     * @param matchedTabIdx  The index of the table containing the assigned LV
     * @param matchedColIdx  The index of the column of the assigned LV
     * @param matchingTabIdx The index of the table containing the matching column
     * @param matchingColIdx The index of the matching column
     */
    protected void matchCacheEntries(int matchedTabIdx, int matchedColIdx, int matchingTabIdx, int matchingColIdx) {
        List<List<CB>> new_entries = new ArrayList<>();
        if (matchedTabIdx == matchingTabIdx) {
            for (List<CB> cache_entry: entries) {
                CB cb = cache_entry.get(matchedTabIdx);
                final int matched_constant = cb.complianceSet[0][matchedColIdx];
                int[][] slice = cb.indices.getSlice(matchingColIdx, matched_constant);
                if (0 < slice.length) {
                    CB new_cb = new CB(slice);
                    List<CB> new_entry = new ArrayList<>(cache_entry);
                    new_entry.set(matchedTabIdx, new_cb);
                    new_entries.add(new_entry);
                }
            }
        } else {
            for (List<CB> cache_entry : entries) {
                CB matched_cb = cache_entry.get(matchedTabIdx);
                CB matching_cb = cache_entry.get(matchingTabIdx);
                final int matched_constant = matched_cb.complianceSet[0][matchedColIdx];
                int[][] slice = matching_cb.indices.getSlice(matchingColIdx, matched_constant);
                if (0 < slice.length) {
                    CB new_cb = new CB(slice);
                    List<CB> new_entry = new ArrayList<>(cache_entry);
                    new_entry.set(matchingTabIdx, new_cb);
                    new_entries.add(new_entry);
                }
            }
        }
        entries = new_entries;
    }

    /**
     * Append a new relation and match a column to another that has already been assigned an LV. The matching column is
     * in the appended relation.
     */
    protected void matchCacheEntries(int matchedTabIdx, int matchedColIdx, IntTable newRelation, int matchingColIdx) {
        List<List<CB>> new_entries = new ArrayList<>();
        for (List<CB> cache_entry : entries) {
            CB matched_cb = cache_entry.get(matchedTabIdx);
            final int matched_constant = matched_cb.complianceSet[0][matchedColIdx];
            int[][] slice = newRelation.getSlice(matchingColIdx, matched_constant);
            if (0 < slice.length) {
                CB new_cb = new CB(slice);
                List<CB> new_entry = new ArrayList<>(cache_entry);
                new_entry.add(new_cb);
                new_entries.add(new_entry);
            }
        }
        entries = new_entries;
    }

    /**
     * Filter a constant symbol at a certain column.
     */
    protected void assignCacheEntries(int tabIdx, int colIdx, int constant) {
        List<List<CB>> new_entries = new ArrayList<>();
        for (List<CB> cache_entry : entries) {
            CB cb = cache_entry.get(tabIdx);
            int[][] slice = cb.indices.getSlice(colIdx, constant);
            if (0 < slice.length) {
                CB new_cb = new CB(slice);
                List<CB> new_entry = new ArrayList<>(cache_entry);
                new_entry.set(tabIdx, new_cb);
                new_entries.add(new_entry);
            }
        }
        entries = new_entries;
    }

    /**
     * Add a used LV info to the fragment.
     *
     * @param vid     The ID of the LV
     * @param varInfo The Info structure of the LV
     */
    protected void addVarInfo(int vid, VarInfo varInfo) {
        if (vid < varInfoList.size()) {
            varInfoList.set(vid, varInfo);
        } else {
            for (int i = varInfoList.size(); i < vid; i++) {
                varInfoList.add(null);
            }
            varInfoList.add(varInfo);
        }
    }

    /**
     * Update case 1a. If the LV has not been assigned in the fragment yet, record as a PLV.
     */
    public void updateCase1a(int tabIdx, int colIdx, int vid) {
        partAssignedRule.get(tabIdx).args[colIdx] = Argument.variable(vid);
        VarInfo var_info = (vid < varInfoList.size())? varInfoList.get(vid) : null;
        if (null == var_info) {
            /* Record as a PLV */
            addVarInfo(vid, new VarInfo(tabIdx, colIdx, true));
        } else {
            /* Filter the two columns */
            if (var_info.isPlv) {
                /* Split by the two columns */
                var_info.isPlv = false;
                splitCacheEntries(var_info.tabIdx, var_info.colIdx, tabIdx, colIdx);
            } else {
                /* Match the new column to the original */
                matchCacheEntries(var_info.tabIdx, var_info.colIdx, tabIdx, colIdx);
            }
        }
    }

    /**
     * Update case 1b.
     *
     * NOTE: In this case, there must be an argument, in the PAR of this fragment, that has already been assigned to the LV.
     */
    public void updateCase1b(IntTable newRelation, int relationSymbol, int colIdx, int vid) {
        Predicate new_pred = new Predicate(relationSymbol, newRelation.totalCols());
        new_pred.args[colIdx] = Argument.variable(vid);
        partAssignedRule.add(new_pred);
        VarInfo var_info = varInfoList.get(vid);    // Assertion: this shall NOT be NULL

        /* Filter the two columns */
        if (var_info.isPlv) {
            /* Split by the two columns */
            var_info.isPlv = false;
            splitCacheEntries(var_info.tabIdx, var_info.colIdx, newRelation, colIdx);
        } else {
            /* Match the new column to the original */
            matchCacheEntries(var_info.tabIdx, var_info.colIdx, newRelation, colIdx);
        }
    }

    /**
     * This helper function splits and gathers entries with same value at a certain column.
     */
    protected Map<Integer, List<List<CB>>> calcConst2EntriesMap(List<List<CB>> entries, int tabIdx, int colIdx) {
        Map<Integer, List<List<CB>>> const_2_entries_map = new HashMap<>();
        for (List<CB> cache_entry: entries) {
            CB cb = cache_entry.get(tabIdx);
            int[][][] slices = cb.indices.splitSlices(colIdx);
            for (int[][] slice: slices) {
                List<List<CB>> entries_of_the_value = const_2_entries_map.computeIfAbsent(
                        slice[0][colIdx], k -> new ArrayList<>()
                );
                List<CB> new_entry = new ArrayList<>(cache_entry);
                new_entry.set(tabIdx, new CB(slice));
                entries_of_the_value.add(new_entry);
            }
        }
        return const_2_entries_map;
    }

    /**
     * This helper function merges two batches entries that have already been gathered by the targeting columns.
     */
    protected List<List<CB>> mergeFragmentEntries(
            Map<Integer, List<List<CB>>> baseConst2EntriesMap, Map<Integer, List<List<CB>>> mergingConst2EntriesMap
    ) {
        List<List<CB>> new_entries = new ArrayList<>();
        for (Map.Entry<Integer, List<List<CB>>> base_map_entry: baseConst2EntriesMap.entrySet()) {
            List<List<CB>> merging_entries = mergingConst2EntriesMap.get(base_map_entry.getKey());
            if (null != merging_entries) {
                for (List<CB> base_entry: base_map_entry.getValue()) {
                    for (List<CB> merging_entry: merging_entries) {
                        List<CB> new_entry = new ArrayList<>(base_entry);
                        new_entry.addAll(merging_entry);
                        new_entries.add(new_entry);
                    }
                }
            }
        }
        return new_entries;
    }

    /**
     * This helper function merges a batch of entries gathered by mering value to a list of base entries
     */
    protected List<List<CB>> mergeFragmentEntries(
            List<List<CB>> baseEntries, int tabIdx, int colIdx, Map<Integer, List<List<CB>>> mergingConst2EntriesMap
    ) {
        List<List<CB>> new_entries = new ArrayList<>();
        for (List<CB> base_entry: baseEntries) {
            int constant = base_entry.get(tabIdx).complianceSet[0][colIdx];
            List<List<CB>> merging_entries = mergingConst2EntriesMap.get(constant);
            if (null != merging_entries) {
                for (List<CB> merging_entry: merging_entries) {
                    List<CB> new_entry = new ArrayList<>(base_entry);
                    new_entry.addAll(merging_entry);
                    new_entries.add(new_entry);
                }
            }
        }
        return new_entries;
    }

    /**
     * Update case 1c. The argument "fragment" will be merged into this fragment. The two arguments "tabIdx" and "colIdx"
     * denote the position of the UV that will be converted to "vId" in "fragment".
     *
     * NOTE: In this case, there must be an argument, in the PAR of this fragment, that has already been assigned to the LV.
     */
    public void updateCase1c(CacheFragment fragment, int tabIdx, int colIdx, int vid) {
        /* Merge PAR */
        int original_tabs = partAssignedRule.size();
        for (Predicate predicate: fragment.partAssignedRule) {
            partAssignedRule.add(new Predicate(predicate));
        }
        partAssignedRule.get(original_tabs + tabIdx).args[colIdx] = Argument.variable(vid);

        /* Merge LV info */
        for (int i = fragment.varInfoList.size() - 1; i >= 0; i--) {
            VarInfo var_info = fragment.varInfoList.get(i);
            if (null != var_info && (i >= this.varInfoList.size() || this.varInfoList.get(i) == null)) {
                addVarInfo(i, new VarInfo(original_tabs + var_info.tabIdx, var_info.colIdx, var_info.isPlv));
            }
        }

        /* Merge entries */
        Map<Integer, List<List<CB>>> merging_frag_const_2_entries_map = calcConst2EntriesMap(
                fragment.entries, tabIdx, colIdx
        );
        VarInfo var_info = varInfoList.get(vid);    // Assertion: this shall NOT be NULL
        if (var_info.isPlv) {
            var_info.isPlv = false;
            Map<Integer, List<List<CB>>> base_frag_const_2_entries_map = calcConst2EntriesMap(
                    entries, var_info.tabIdx, var_info.colIdx
            );
            entries = mergeFragmentEntries(base_frag_const_2_entries_map, merging_frag_const_2_entries_map);
        } else {
            entries = mergeFragmentEntries(entries, var_info.tabIdx, var_info.colIdx, merging_frag_const_2_entries_map);
        }
    }

    /**
     * Update case 2a.
     */
    public void updateCase2a(int tabIdx1, int colIdx1, int tabIdx2, int colIdx2, int newVid) {
        /* Modify PAR */
        addVarInfo(newVid, new VarInfo(tabIdx1, colIdx1, false));
        int var_arg = Argument.variable(newVid);
        partAssignedRule.get(tabIdx1).args[colIdx1] = var_arg;
        partAssignedRule.get(tabIdx2).args[colIdx2] = var_arg;

        /* Modify cache entries */
        splitCacheEntries(tabIdx1, colIdx1, tabIdx2, colIdx2);
    }

    /**
     * Update case 2b.
     */
    public void updateCase2b(IntTable newRelation, int relationSymbol, int colIdx1, int tabIdx2, int colIdx2, int newVid) {
        /* Modify PAR */
        addVarInfo(newVid, new VarInfo(tabIdx2, colIdx2, false));
        int var_arg = Argument.variable(newVid);
        partAssignedRule.get(tabIdx2).args[colIdx2] = var_arg;
        Predicate new_pred = new Predicate(relationSymbol, newRelation.totalCols());
        new_pred.args[colIdx1] = var_arg;
        partAssignedRule.add(new_pred);

        /* Modify cache entries */
        splitCacheEntries(tabIdx2, colIdx2, newRelation, colIdx1);
    }

    /**
     * Update case 2c. The argument "fragment" will be merged into this fragment. The two arguments "tabIdx2" and "colIdx2"
     * denote the position of the UV in "fragment". "tabIdx" and "colIdx" denote the position of the UV in this fragment.
     */
    public void updateCase2c(int tabIdx, int colIdx, CacheFragment fragment, int tabIdx2, int colIdx2, int newVid) {
        /* Merge PAR */
        int original_tabs = partAssignedRule.size();
        for (Predicate predicate: fragment.partAssignedRule) {
            partAssignedRule.add(new Predicate(predicate));
        }
        int var_arg = Argument.variable(newVid);
        partAssignedRule.get(tabIdx).args[colIdx] = var_arg;
        partAssignedRule.get(original_tabs + tabIdx2).args[colIdx2] = var_arg;

        /* Merge LV info */
        addVarInfo(newVid, new VarInfo(tabIdx, colIdx, false));
        for (int i = 0; i < fragment.varInfoList.size(); i++) {
            VarInfo var_info = fragment.varInfoList.get(i);
            if (null != var_info && (i >= this.varInfoList.size() || this.varInfoList.get(i) == null)) {
                addVarInfo(i, new VarInfo(original_tabs + var_info.tabIdx, var_info.colIdx, var_info.isPlv));
            }
        }

        /* Merge entries */
        Map<Integer, List<List<CB>>> merging_frag_const_2_entries_map = calcConst2EntriesMap(
                fragment.entries, tabIdx2, colIdx2
        );
        Map<Integer, List<List<CB>>> base_frag_const_2_entries_map = calcConst2EntriesMap(
                entries, tabIdx, colIdx
        );
        entries = mergeFragmentEntries(base_frag_const_2_entries_map, merging_frag_const_2_entries_map);
    }

    /**
     * Update case 3.
     */
    public void updateCase3(int tabIdx, int colIdx, int constant) {
        /* Modify PAR */
        partAssignedRule.get(tabIdx).args[colIdx] = Argument.constant(constant);

        /* Modify cache entries */
        assignCacheEntries(tabIdx, colIdx, constant);
    }

    /**
     * Build indices of each CB in the entries.
     *
     * NOTE: this should be called before update is made
     */
    public void buildIndices() {
        for (List<CB> entry: entries) {
            for (CB cb: entry) {
                cb.buildIndices();
            }
        }
    }

    public boolean hasLv(int vid) {
        return (vid < varInfoList.size()) && null != varInfoList.get(vid);
    }

    /**
     * Recursively compute the cartesian product of binding values of grouped PLVs and add each combination in the product
     * to the binding set.
     *
     * @param completeBindings the binding set
     * @param plvBindingSets the binding values of the grouped PLVs
     * @param template the template array to hold the binding combination
     * @param bindingSetIdx the index of the binding set in current recursion
     * @param templateStartIdx the starting index of the template for the PLV bindings
     */
    protected void addCompletePlvBindings(
            Set<Record> completeBindings, Set<Record>[] plvBindingSets, int[] template, int bindingSetIdx, int templateStartIdx
    ) {
        final Set<Record> plv_bindings = plvBindingSets[bindingSetIdx];
        Iterator<Record> itr = plv_bindings.iterator();
        Record plv_binding = itr.next();
        int binding_length = plv_binding.args.length;
        if (bindingSetIdx == plvBindingSets.length - 1) {
            /* Complete each template and add to the set */
            while (true) {
                System.arraycopy(plv_binding.args, 0, template, templateStartIdx, binding_length);
                completeBindings.add(new Record(template.clone()));
                if (!itr.hasNext()) break;
                plv_binding = itr.next();
            }
        } else {
            /* Complete part of the template and move to next recursion */
            while (true) {
                System.arraycopy(plv_binding.args, 0, template, templateStartIdx, binding_length);
                addCompletePlvBindings(
                        completeBindings, plvBindingSets, template, bindingSetIdx+1, templateStartIdx+binding_length
                );
                if (!itr.hasNext()) break;
                plv_binding = itr.next();
            }
        }
    }

    /**
     * This method returns the number of unique combinations of all listed variables.
     *
     * NOTE: the listed variables must NOT contain duplications and LVs that are not presented in this fragment.
     */
    public int countCombinations(List<Integer> vids) {
        List<VarInfo> lvs = new ArrayList<>();
        List<Integer>[] plv_col_index_lists = new List[partAssignedRule.size()];    // Table index is the index of the array
        List<Integer> tab_idxs_with_plvs = new ArrayList<>();
        int total_plvs = 0;
        for (int vid: vids) {
            VarInfo var_info = varInfoList.get(vid);    // This shall NOT be NULL
            if (var_info.isPlv) {
                total_plvs++;
                if (null == plv_col_index_lists[var_info.tabIdx]) {
                    plv_col_index_lists[var_info.tabIdx] = new ArrayList<>();
                    tab_idxs_with_plvs.add(var_info.tabIdx);
                }
                plv_col_index_lists[var_info.tabIdx].add(var_info.colIdx);
            } else {
                lvs.add(var_info);
            }
        }

        int total_unique_bindings = 0;
        if (0 == total_plvs) {
            /* No PLV. Just count all combinations of LVs */
            final Set<Record> lv_bindings = new HashSet<>();
            for (List<CB> cache_entry: entries) {
                int[] binding = new int[lvs.size()];
                for (int i = 0; i < binding.length; i++) {
                    VarInfo lv_info = lvs.get(i);
                    binding[i] = cache_entry.get(lv_info.tabIdx).complianceSet[0][lv_info.colIdx];
                }
                lv_bindings.add(new Record(binding));
            }
            total_unique_bindings = lv_bindings.size();
        } else  {
            Map<Record, Set<Record>> lv_bindings_2_plv_bindings = new HashMap<>();
            for (List<CB> cache_entry: entries) {
                /* Find LV binding first */
                final int[] lv_binding = new int[lvs.size()];
                for (int i = 0; i < lv_binding.length; i++) {
                    VarInfo lv_info = lvs.get(i);
                    lv_binding[i] = cache_entry.get(lv_info.tabIdx).complianceSet[0][lv_info.colIdx];
                }

                /* Cartesian product all the PLVs */
                Set<Record>[] plv_bindings_within_tab_sets = new Set[tab_idxs_with_plvs.size()];
                for (int i = 0; i < plv_bindings_within_tab_sets.length; i++) {
                    int tab_idx = tab_idxs_with_plvs.get(i);
                    final List<Integer> plv_col_idxs = plv_col_index_lists[tab_idx];
                    final Set<Record> plv_bindings = new HashSet<>();
                    for (int[] cs_record : cache_entry.get(tab_idx).complianceSet) {
                        final int[] plv_binding_within_tab = new int[plv_col_idxs.size()];
                        for (int j = 0; j < plv_binding_within_tab.length; j++) {
                            plv_binding_within_tab[j] = cs_record[plv_col_idxs.get(j)];
                        }
                        plv_bindings.add(new Record(plv_binding_within_tab));
                    }
                    plv_bindings_within_tab_sets[i] = plv_bindings;
                }
                final Set<Record> complete_plv_bindings = lv_bindings_2_plv_bindings.computeIfAbsent(
                        new Record(lv_binding), k -> new HashSet<>()
                );
                if (1 == plv_bindings_within_tab_sets.length) {
                    /* No need to perform Cartesian product */
                    complete_plv_bindings.addAll(plv_bindings_within_tab_sets[0]);
                } else {
                    /* Cartesian product required */
                    addCompletePlvBindings(
                            complete_plv_bindings, plv_bindings_within_tab_sets, new int[total_plvs], 0, 0
                    );
                }
            }
            for (Set<Record> plv_bindings: lv_bindings_2_plv_bindings.values()) {
                total_unique_bindings += plv_bindings.size();
            }
        }
        return total_unique_bindings;
    }

    /**
     * Recursively add PLV bindings to a given template
     *
     * @param templateSet          The set of finished templates
     * @param plvBindingSets       The bindings of PLVs grouped by predicate
     * @param plv2TemplateIdxLists The linked arguments in the head for each PLV
     * @param template             An argument list template
     * @param setIdx               The index of the PLV group
     */
    protected void addPlvBindings2Templates(
            final Set<Record> templateSet, final Set<Record>[] plvBindingSets, final List<Integer>[] plv2TemplateIdxLists,
            final int[] template, final int setIdx
    ) {
        final Set<Record> plv_bindings = plvBindingSets[setIdx];
        final List<Integer> plv_2_template_idxs = plv2TemplateIdxLists[setIdx];
        if (setIdx == plvBindingSets.length - 1) {
            /* Finish the last group of PLVs, add to the template set */
            for (Record plv_binding: plv_bindings) {
                for (int i = 0; i < plv_binding.args.length; i++) {
                    template[plv_2_template_idxs.get(i)] = plv_binding.args[i];
                }
                templateSet.add(new Record(template.clone()));
            }
        } else {
            /* Add current binding to the template and move to the next recursion */
            for (Record plv_binding: plv_bindings) {
                for (int i = 0; i < plv_binding.args.length; i++) {
                    template[plv_2_template_idxs.get(i)] = plv_binding.args[i];
                }
                addPlvBindings2Templates(templateSet, plvBindingSets, plv2TemplateIdxLists, template, setIdx + 1);
            }
        }
    }

    /**
     * This method returns the set of combinations of all listed variables.
     *
     * NOTE: the listed variables must NOT contain duplications and LVs that are not presented in this fragment.
     */
    public Set<Record> enumerateCombinations(List<Integer> vids) {
        /* Split LVs and PLVs and find the locations of the vars */
        /* Also, group PLVs within tabs */
        List<VarInfo> lvs = new ArrayList<>();
        List<Integer> lv_template_idxs = new ArrayList<>();
        List<Integer>[] plv_col_idx_lists = new List[partAssignedRule.size()];    // Table index is the index of the array
        List<Integer>[] plv_2_template_idx_lists = new List[partAssignedRule.size()];
        List<Integer> tab_idxs_with_plvs = new ArrayList<>();
        for (int template_idx = 0; template_idx < vids.size(); template_idx++) {
            VarInfo var_info = varInfoList.get(vids.get(template_idx));    // This shall NOT be NULL
            if (var_info.isPlv) {
                if (null == plv_col_idx_lists[var_info.tabIdx]) {
                    plv_col_idx_lists[var_info.tabIdx] = new ArrayList<>();
                    plv_2_template_idx_lists[var_info.tabIdx] = new ArrayList<>();
                    tab_idxs_with_plvs.add(var_info.tabIdx);
                }
                plv_col_idx_lists[var_info.tabIdx].add(var_info.colIdx);
                plv_2_template_idx_lists[var_info.tabIdx].add(template_idx);
            } else {
                lv_template_idxs.add(template_idx);
                lvs.add(var_info);
            }
        }

        final Set<Record> bindings = new HashSet<>();
        if (tab_idxs_with_plvs.isEmpty()) {
            /* No PLV. Just enumerate all combinations of LVs */
            for (List<CB> cache_entry: entries) {
                int[] binding = new int[vids.size()];
                for (int template_idx = 0; template_idx < binding.length; template_idx++) {
                    VarInfo lv_info = lvs.get(template_idx);
                    binding[template_idx] = cache_entry.get(lv_info.tabIdx).complianceSet[0][lv_info.colIdx];
                }
                bindings.add(new Record(binding));
            }
        } else  {
            for (List<CB> cache_entry: entries) {
                /* Find LV binding first */
                final int[] binding_with_only_lvs = new int[vids.size()];
                for (int i = 0; i < lvs.size(); i++) {
                    VarInfo lv_info = lvs.get(i);
                    binding_with_only_lvs[lv_template_idxs.get(i)] = cache_entry.get(lv_info.tabIdx).complianceSet[0][lv_info.colIdx];
                }

                /* Cartesian product all the PLVs */
                Set<Record>[] plv_bindings_within_tab_sets = new Set[tab_idxs_with_plvs.size()];
                List<Integer>[] plv_2_template_idx_within_tab_lists = new List[tab_idxs_with_plvs.size()];
                for (int i = 0; i < plv_bindings_within_tab_sets.length; i++) {
                    final int tab_idx = tab_idxs_with_plvs.get(i);
                    final List<Integer> plv_col_idxs = plv_col_idx_lists[tab_idx];
                    final Set<Record> plv_bindings = new HashSet<>();
                    plv_bindings_within_tab_sets[i] = plv_bindings;
                    plv_2_template_idx_within_tab_lists[i] = plv_2_template_idx_lists[tab_idx];
                    for (int[] cs_record : cache_entry.get(tab_idx).complianceSet) {
                        final int[] plv_binding_within_tab = new int[plv_col_idxs.size()];
                        for (int j = 0; j < plv_binding_within_tab.length; j++) {
                            plv_binding_within_tab[j] = cs_record[plv_col_idxs.get(j)];
                        }
                        plv_bindings.add(new Record(plv_binding_within_tab));
                    }
                }
                addPlvBindings2Templates(
                        bindings, plv_bindings_within_tab_sets, plv_2_template_idx_within_tab_lists, binding_with_only_lvs, 0
                );
            }
        }
        return bindings;
    }

    /**
     * Count the number of unique records in a separate table.
     */
    public int countTableSize(int tabIdx) {
        Set<int[]> records = new HashSet<>();
        for (List<CB> entry: entries) {
            records.addAll(Arrays.asList(entry.get(tabIdx).complianceSet));
        }
        return records.size();
    }

    public boolean isEmpty() {
        return entries.isEmpty();
    }

    public void clear() {
        if (!entries.isEmpty()) {
            entries = new ArrayList<>();
        }
    }

    @Override
    public Iterator<List<CB>> iterator() {
        return entries.iterator();
    }

    public int totalEntries() {
        return entries.size();
    }

    public List<CB> getEntry(int idx) {
        return entries.get(idx);
    }

    public int totalTables() {
        return partAssignedRule.size();
    }
}
