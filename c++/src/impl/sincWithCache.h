#pragma once

#include "../base/sinc.h"
#include "../kb/intTable.h"
#include <unordered_map>

namespace sinc {
    /**
     * A simplified complied block.
     * 
     * @since 2.0
     */
    class CompliedBlock {
    public:
        /**
         * Create a new CB and register it to the pool
         * 
         * NOTE: The pointer `complianceSet` WILL be maintained by this CB object
         */
        static CompliedBlock* create(int** const complianceSet, int const totalRows, int const totalCols, bool maintainComplianceSet);

        /**
         * Create a new CB and register it to the pool
         * 
         * NOTE: The pointers `complianceSet` WILL be maintained by this CB object
         */
        static CompliedBlock* create(
            int** const complianceSet, int const totalRows, int const totalCols, IntTable* indices,
            bool maintainComplianceSet, bool maintainIndices
        );

        /**
         * Release all pointers in the pool and clear the pool
         */
        static void clearPool();

        /**
         * Count the total number of CBs in the pool
         */
        static size_t totalNumCbs();

        /**
         * Count the total size of CBs in the pool
         */
        static size_t totalCbMemoryCost();

        ~CompliedBlock();

        /**
         * Build the indices if it is null
         */
        void buildIndices();

        int* const* getComplianceSet() const;
        const IntTable& getIndices() const;
        int getTotalRows() const;
        int getTotalCols() const;
        size_t memoryCost() const;

        void showComplianceSet() const;

    protected:
        static std::vector<CompliedBlock*> pool;

        int** const complianceSet;
        IntTable* indices;
        int const totalRows;
        int const totalCols;
        bool mainTainComplianceSet;
        bool maintainIndices;

        /**
         * Register a pointer to a CB in the static pool
         */
        static void registerCb(CompliedBlock* cb);

        /**
         * NOTE: The pointer `complianceSet` WILL be maintained by this CB object
         */
        CompliedBlock(int** const complianceSet, int const totalRows, int const totalCols, bool maintainComplianceSet);

        /**
         * NOTE: The pointers `complianceSet` and `indices` WILL be maintained by this CB object
         */
        CompliedBlock(
            int** const complianceSet, int const totalRows, int const totalCols, IntTable* indices, bool maintainComplianceSet,
            bool maintainIndices
        );
    };

    /**
     * Monitoring information for cache in SInC. Time is measured in nanoseconds.
     *
     * @since 2.0
     */
    class CachedSincPerfMonitor : public PerformanceMonitor {
    public:
        uint64_t posCacheUpdateTime = 0;
        uint64_t prunedPosCacheUpdateTime = 0;
        uint64_t entCacheUpdateTime = 0;
        uint64_t allCacheUpdateTime = 0;
        uint64_t posCacheIndexingTime = 0;
        uint64_t entCacheIndexingTime = 0;
        uint64_t allCacheIndexingTime = 0;
        uint64_t copyTime = 0;

        int posCacheEntriesTotal = 0;
        int entCacheEntriesTotal = 0;
        int allCacheEntriesTotal = 0;
        int posCacheEntriesMax = 0;
        int entCacheEntriesMax = 0;
        int allCacheEntriesMax = 0;
        int totalGeneratedRules = 0;

        void show(std::ostream& os) override;
    };

    /**
     * Used for quickly locating an LV (PLV)
     */
    class VarInfo {
    public:
        int tabIdx; // -1 means this information is empty
        int colIdx;
        bool isPlv;

        /**
         * Create an empty info
         */
        VarInfo();

        VarInfo(int const tabIdx, int const colIdx, bool isPlv);
        VarInfo(const VarInfo& another);
        bool isEmpty() const;
        VarInfo& operator=(VarInfo&& another) noexcept;
        VarInfo& operator=(const VarInfo& another) noexcept;
        bool operator==(const VarInfo &another) const;
        size_t hash() const;
    };
}

/**
 * This is for hashing "VarInfo" in unordered containers.
 */
template <>
struct std::hash<sinc::VarInfo> {
    size_t operator()(const sinc::VarInfo& r) const;
};

template <>
struct std::hash<const sinc::VarInfo> {
    size_t operator()(const sinc::VarInfo& r) const;
};

/**
 * This is for hashing "VarInfo*" in unordered containers.
 */
template <>
struct std::hash<sinc::VarInfo*> {
    size_t operator()(const sinc::VarInfo *r) const;
};

template <>
struct std::hash<const sinc::VarInfo*> {
    size_t operator()(const sinc::VarInfo *r) const;
};

/**
 * This is for checking equivalence "VarInfo*" in unordered containers.
 */
template <>
struct std::equal_to<sinc::VarInfo*> {
    bool operator()(const sinc::VarInfo *r1, const sinc::VarInfo *r2) const;
};

template <>
struct std::equal_to<const sinc::VarInfo*> {
    bool operator()(const sinc::VarInfo *r1, const sinc::VarInfo *r2) const;
};

namespace sinc {
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
     * 
     * NOTE: It is hard to manage "copy-on-write" on the level of `CacheFragment`. Therefore, `CacheFragment`s are simply copied
     * in the copy constructor.
     * 
     * @since 2.2
     */
    class CacheFragment {
    public:
        typedef std::vector<CompliedBlock*> entryType;
        typedef std::vector<entryType*> entriesType;

        CacheFragment(IntTable* const firstRelation, int const relationSymbol); // Todo: Refine `const` modifier for all parameters

        // CacheFragment(std::vector<int*> const& rows, int const relationSymbol, int const arity);

        /**
         * Construct an object by an existing CB.
        */
        CacheFragment(CompliedBlock* const firstCb, int const relationSymbol);

        /**
         * This constructor is used to construct an empty fragment
         */
        CacheFragment(int const relationSymbol, int const arity);

        CacheFragment(const CacheFragment& another);

        ~CacheFragment();

        /**
         * Update case 1a. If the LV has not been assigned in the fragment yet, record as a PLV.
         */
        void updateCase1a(int const tabIdx, int const colIdx, int const vid);

        /**
         * Update case 1b.
         *
         * NOTE: In this case, there must be an argument, in the PAR of this fragment, that has already been assigned to the LV.
         */
        void updateCase1b(IntTable* const newRelation, int const relationSymbol, int const colIdx, int const vid);

        /**
         * Update case 1c. The argument "fragment" will be merged into this fragment. The two arguments "tabIdx" and "colIdx"
         * denote the position of the UV that will be converted to "vId" in "fragment".
         *
         * NOTE: In this case, there must be an argument, in the PAR of this fragment, that has already been assigned to the LV.
         */
        void updateCase1c(CacheFragment const& fragment, int const tabIdx, int const colIdx, int const vid);

        /**
         * Update case 2a.
         */
        void updateCase2a(int const tabIdx1, int const colIdx1, int const tabIdx2, int const colIdx2, int const newVid);

        /**
         * Update case 2b.
         */
        void updateCase2b(
            IntTable* const newRelation, int const relationSymbol, int const colIdx1, int const tabIdx2, int const colIdx2,
            int const newVid
        );

        /**
         * Update case 2c. The argument "fragment" will be merged into this fragment. The two arguments "tabIdx2" and "colIdx2"
         * denote the position of the UV in "fragment". "tabIdx" and "colIdx" denote the position of the UV in this fragment.
         */
        void updateCase2c(
            int const tabIdx, int const colIdx, CacheFragment const& fragment, int const tabIdx2, int const colIdx2, int const newVid
        );

        /**
         * Update case 3.
         */
        void updateCase3(int const tabIdx, int const colIdx, int const constant);

        /**
         * Build indices of each CB in the entries.
         *
         * NOTE: this should be called before update is made
         */
        void buildIndices();

        bool hasLv(int const vid) const;

        /**
         * This method returns the number of unique combinations of all listed variables.
         *
         * NOTE: the listed variables must NOT contain duplications and LVs that are not presented in this fragment.
         */
        int countCombinations(std::vector<int> const& vids) const;

        /**
         * This method returns the set of combinations of all listed variables.
         *
         * NOTE: the listed variables must NOT contain duplications and LVs that are not presented in this fragment.
         * 
         * NOTE: The returned pointer to the set and the pointers returned by `getArgs()` of each record SHOULD be
         * maintained by USER
         */
        std::unordered_set<Record>* enumerateCombinations(std::vector<int> const& vids) const;

        bool isEmpty() const;

        void clear();

        const entriesType& getEntries() const;

        entryType* getEntry(int const idx) const;

        /**
         * Count the number of unique records in a separate table.
         */
        int countTableSize(int const tabIdx) const;

        int totalTables() const;

        std::vector<Predicate> const& getPartAssignedRule() const;

        std::vector<VarInfo> const& getVarInfoList() const;

        static void showEntry(entryType const& entry);

        static void showEntries(entriesType const& entries);

    protected:
        typedef std::unordered_map<int, entriesType*> const2EntriesMapType;

        /** Partially assigned rule structure for this fragment. Predicate symbols are unnecessary here, but useful for debugging */
        std::vector<Predicate> partAssignedRule;
        /** Compact cache entries, each entry is a list of CB */
        entriesType* entries; // Todo: If CB can be fully copy-on-write, that is, no two CBs in the memory contains the same compliance set, the specialization and counting can be faster
        /** A list of LV info. Each index is the ID of an LV */
        std::vector<VarInfo> varInfoList;

        /**
         * Split cache entries according to two columns in the fragment.
         */
        void splitCacheEntries(int const tabIdx1, int const colIdx1, int const tabIdx2, int const colIdx2);

        /**
         * Append a new relation and split cache entries according to two columns in the fragment. One column is in one of
         * the original relations, and the other is in the appended relation.
         */
        void splitCacheEntries(int const tabIdx1, int const colIdx1, IntTable* const newRelation, int const colIdx2);

        /**
         * Match a column to another that has already been assigned an LV.
         *
         * @param matchedTabIdx  The index of the table containing the assigned LV
         * @param matchedColIdx  The index of the column of the assigned LV
         * @param matchingTabIdx The index of the table containing the matching column
         * @param matchingColIdx The index of the matching column
         */
        void matchCacheEntries(int const matchedTabIdx, int const matchedColIdx, int const matchingTabIdx, int const matchingColIdx);

        /**
         * Append a new relation and match a column to another that has already been assigned an LV. The matching column is
         * in the appended relation.
         */
        void matchCacheEntries(int const matchedTabIdx, int const matchedColIdx, IntTable* const newRelation, int matchingColIdx);

        /**
         * Filter a constant symbol at a certain column.
         */
        void assignCacheEntries(int const tabIdx, int const colIdx, int const constant);

        /**
         * Add a used LV info to the fragment.
         *
         * @param vid     The ID of the LV
         * @param varInfo The Info structure of the LV
         */
        void addVarInfo(int const vid, int const tabIdx, int const colIdx, bool const isPlv);

        /**
         * This helper function splits and gathers entries with same value at a certain column.
         */
        static const2EntriesMapType* calcConst2EntriesMap(entriesType const& entries, int const tabIdx, int const colIdx, int const arity);

        /**
         * This helper function merges two batches entries that have already been gathered by the targeting columns.
         * 
         * NOTE: This method updates `entries`
         */
        void mergeFragmentEntries(
            const2EntriesMapType const& baseConst2EntriesMap, const2EntriesMapType const& mergingConst2EntriesMap
        );

        /**
         * This helper function merges a batch of entries gathered by mering value to a list of base entries
         * 
         * NOTE: This method updates `entries`
         */
        void mergeFragmentEntries(
            entriesType const& baseEntries, int const tabIdx, int const colIdx, const2EntriesMapType const& mergingConst2EntriesMap
        );

        static void releaseConst2EntryMap(const2EntriesMapType* map);

        /**
         * Recursively compute the cartesian product of binding values of grouped PLVs and add each combination in the product
         * to the binding set.
         *
         * @param completeBindings the binding set
         * @param plvBindingSets the binding values of the grouped PLVs
         * @param template the template array to hold the binding combination
         * @param bindingSetIdx the index of the binding set in current recursion
         * @param templateStartIdx the starting index of the template for the PLV bindings
         * @param numSets              The number of elements in `plvBindingSets[]`
         */
        void addCompletePlvBindings(
            std::unordered_set<Record>& completeBindings, std::unordered_set<Record>* const plvBindingSets, int* const argTemplate,
            int const bindingSetIdx, int const templateStartIdx, int const numSets
        ) const;

        /**
         * Recursively add PLV bindings to a given template
         *
         * @param templateSet          The set of finished templates
         * @param plvBindingSets       The bindings of PLVs grouped by predicate
         * @param plv2TemplateIdxLists The linked arguments in the head for each PLV
         * @param template             An argument list template
         * @param setIdx               The index of the PLV group
         * @param numSets              The number of elements in `plvBindingSets[]`
         * @param templateLength       The length of `argTemplate[]`
         */
        void addPlvBindings2Templates(
            std::unordered_set<Record>& templateSet, std::unordered_set<Record>* const plvBindingSets,
            std::vector<int>** const plv2TemplateIdxLists, int* const argTemplate, int const setIdx, int const numSets,
            int const templateLength
        ) const;

        void releaseEntries();
    };

    /**
     * This class is for mapping predicates in the rule to tables in cache fragments in the E-cache.
     * 
     * @since 2.3
     */
    class TabInfo {
    public:
        int fragmentIdx;
        int tabIdx;

        TabInfo(int const fragmentIdx, int const tabIdx);
        TabInfo(const TabInfo& another);
        bool isEmpty() const;
        TabInfo& operator=(TabInfo&& another) noexcept;
        TabInfo& operator=(const TabInfo& another) noexcept;
        bool operator==(const TabInfo &another) const;
    };

    /**
     * First-order Horn rule with the compact grounding cache (CGC). The CGC is implemented by "CacheFragment".
     *
     * NOTE: The caches (i.e., the lists of cache entries) shall not be modified. Any modification of the cache should follow
     * the copy-on-write strategy, i.e., replace the lists directly with new ones.
     *
     * NOTE: This class is translated from `sinc2.impl.negsamp.FragmentedCachedRule`.
     * 
     * @since 2.3
     */
    class CachedRule : public Rule {
    public:
        /**
         * Initialize the most general rule.
         *
         * @param headPredSymbol      The functor of the head predicate, i.e., the target relation.
         * @param arity               The arity of the functor
         * @param fingerprintCache    The cache of the used fingerprints
         * @param category2TabuSetMap The tabu set of pruned fingerprints
         * @param kb                  The original KB
         */
        CachedRule(
            int const headPredSymbol, int const arity, fingerprintCacheType& fingerprintCache, tabuMapType& category2TabuSetMap,
            SimpleKb& kb
        );

        CachedRule(const CachedRule& another);
        ~CachedRule();
        CachedRule* clone() const override;
        void updateCacheIndices();
        EvidenceBatch* getEvidenceAndMarkEntailment() override;
        std::unordered_set<Record>* getCounterexamples() const override;
        void releaseMemory() override;

        uint64_t getCopyTime() const;
        uint64_t getPosCacheUpdateTime() const;
        uint64_t getEntCacheUpdateTime() const;
        uint64_t getAllCacheUpdateTime() const;
        uint64_t getPosCacheIndexingTime() const;
        uint64_t getEntCacheIndexingTime() const;
        uint64_t getAllCacheIndexingTime() const;
        const CacheFragment& getPosCache() const;
        const CacheFragment& getEntCache() const;
        const std::vector<CacheFragment*>& getAllCache() const;

    protected:
        /** The original KB */
        SimpleKb& kb;
        /** The cache for the positive entailments (E+-cache) (not entailed). One cache fragment is sufficient as all
         *  predicates are linked to the head. */
        CacheFragment* posCache;
        /** This cache is used to monitor the already-entailed records (T-cache). One cache fragment is sufficient as all
         *  predicates are linked to the head. */
        CacheFragment* entCache;
        /** The cache for all the entailments (E-cache). The cache is composed of multiple fragments, each of which maintains
         *  a linked component of the rule body. The first relation should not be included, as the head should be removed
         *  from E-cache */
        std::vector<CacheFragment*>* allCache;
        /** This list is a mapping from predicate indices in the rule structure to fragment and table indices in the E-cache.
         *  The array indices are predicate indices. */
        std::vector<TabInfo> predIdx2AllCacheTableInfo;
        /** Whether the pointer `posCache` should be maintained by this object */
        bool maintainPosCache;
        /** Whether the pointer `entCache` should be maintained by this object */
        bool maintainEntCache;
        /** Whether the pointer `allCache` and pointers within the vector should be maintained by this object */
        bool maintainAllCache;

        /* Monitoring info. The time (in nanoseconds) refers to the corresponding time consumption in the last update of the rule */
        uint64_t copyTime = 0;
        uint64_t posCacheUpdateTime = 0;
        uint64_t entCacheUpdateTime = 0;
        uint64_t allCacheUpdateTime = 0;
        uint64_t posCacheIndexingTime = 0;
        uint64_t entCacheIndexingTime = 0;
        uint64_t allCacheIndexingTime = 0;

        /** If this object does not maintain the E+-cache, get a copy of the cache */
        void obtainPosCache();

        /** If this object does not maintain the T-cache, get a copy of the cache */
        void obtainEntCache();

        /** If this object does not maintain the E-cache, get a copy of the cache */
        void obtainAllCache();

        double recordCoverage() override;

        Eval calculateEval() const override;
        UpdateStatus specCase1HandlerPrePruning(int const predIdx, int const argIdx, int const varId) override;
        UpdateStatus specCase1HandlerPostPruning(int const predIdx, int const argIdx, int const varId) override;
        UpdateStatus specCase2HandlerPrePruning(int const predSymbol, int const arity, int const argIdx, int const varId) override;
        UpdateStatus specCase2HandlerPostPruning(int const predSymbol, int const arity, int const argIdx, int const varId) override;
        UpdateStatus specCase3HandlerPrePruning(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) override;
        UpdateStatus specCase3HandlerPostPruning(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) override;
        UpdateStatus specCase4HandlerPrePruning(
            int const predSymbol, int const arity, int const argIdx1, int const predIdx2, int const argIdx2
        ) override;
        UpdateStatus specCase4HandlerPostPruning(
            int const predSymbol, int const arity, int const argIdx1, int const predIdx2, int const argIdx2
        ) override;
        UpdateStatus specCase5HandlerPrePruning(int const predIdx, int const argIdx, int const constant) override;
        UpdateStatus specCase5HandlerPostPruning(int const predIdx, int const argIdx, int const constant) override;
        UpdateStatus generalizeHandlerPrePruning(int const predIdx, int const argIdx) override;
        UpdateStatus generalizeHandlerPostPruning(int const predIdx, int const argIdx) override;

        /**
         * Update fragment indices and predicate index mappings when two fragments merge together.
         *
         * NOTE: This should be called BEFORE the two fragments are merged
         * 
         * NOTE: This method will remove the reference to the merging fragment
         *
         * @param baseFragmentIdx    The index of the base fragment. This fragment will not be changed.
         * @param mergingFragmentIdx The index of the fragment that will be merged into the base. This fragment will be removed after the merge
         */
        void mergeFragmentIndices(int const baseFragmentIdx, int const mergingFragmentIdx);

        /**
         * Recursively add PLV bindings to the linked head arguments.
         *
         * NOTE: This should only be called when body is not empty
         *
         * @param headTemplates       The set of head templates
         * @param bindingsInFragments The bindings of LVs grouped by cache fragment
         * @param headArgIdxLists     The linked head argument indices for LV groups
         * @param template            A template of the head templates
         * @param validFragmentIndices The indices of fragments that contains LVs to be substituted
         * @param headArity            The arity of the head predicate
         * @param idx             The index of `validFragmentIndices`
         */
        void generateHeadTemplates(
            std::unordered_set<Record>& headTempaltes, std::unordered_set<Record>** const bindingsInFragments,
            std::vector<std::vector<int>>* const headArgIdxLists, int* const argTemplate,
            std::vector<int> const& validFragmentIndices, int const headArity, int const fragIdx
        ) const;

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
         * @param numVectors      The number of vectors in array `varLocs`
         */
        void expandHeadUvs4CounterExamples(
            SimpleRelation const& targetRelation, std::unordered_set<Record>& counterexamples, int* const recTemplate,
            std::vector<int>** varLocs, int const idx, int const numVectors
        ) const;
    };

    /**
     * The relation miner class that uses "CachedRule".
     *
     * @since 2.3
     */
    class RelationMinerWithCachedRule : public RelationMiner {
    public:
        CachedSincPerfMonitor monitor;

        /**
         * Construct by passing parameters from the compressor that loads the data.
         *
         * @param kb                   The input KB
         * @param targetRelation       The target relation in the KB
         * @param evalMetric           The rule evaluation metric
         * @param beamwidth            The beamwidth used in the rule mining procedure
         * @param stopCompressionRatio The stopping compression ratio for inducing a single rule
         * @param predicate2NodeMap    The mapping from predicates to the nodes in the dependency graph
         * @param dependencyGraph      The dependency graph
         * @param logger               A logger
         */
        RelationMinerWithCachedRule(
            SimpleKb& kb, int const targetRelation, EvalMetric::Value evalMetric, int const beamwidth, double const stopCompressionRatio,
            nodeMapType& predicate2NodeMap, depGraphType& dependencyGraph, std::vector<Rule*>& hypothesis,
            std::unordered_set<Record>& counterexamples, std::ostream& logger
        );

        ~RelationMinerWithCachedRule();

    protected:
        std::vector<Rule::fingerprintCacheType*> fingerprintCaches;

        /**
         * Create a rule with compact caching and tabu set.
         */
        Rule* getStartRule() override;

        /**
         * When a rule r is selected as beam, update its cache indices. The rule r here is a "CachedRule".
         */
        void selectAsBeam(Rule* r) override;

        /**
         * Record monitoring information compared to the super implementation
         */
        int checkThenAddRule(UpdateStatus updateStatus, Rule* const updatedRule, Rule& originalRule, Rule** candidates) override;
    };

    /**
     * A basic implementation of SInC. Rule mining are with compact caching and tabu pruning.
     *
     * @since 2.0
     */
    class SincWithCache : public SInC {
    public:
        SincWithCache(SincConfig* const config);
        SincWithCache(SincConfig* const config, SimpleKb* const kb);

    protected:
        CachedSincPerfMonitor monitor;

        SincRecovery* createRecovery() override;
        RelationMiner* createRelationMiner(int const targetRelationNum) override;
        void finalizeRelationMiner(RelationMiner* miner) override;
        void showMonitor() override;
        void finish() override;
    };
}