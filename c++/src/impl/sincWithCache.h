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
     * This class is used for representing the link from a body GV to all GVs in the head.
     *
     * @since 1.0
     */
    class BodyGvLinkInfo {
    public:
        /** The predicate index of the GV in the body */
        int const bodyPredIdx;
        /** The argument index of the GV in the body */
        int const bodyArgIdx;
        /** The argument indices of the GVs in the head */
        std::vector<int>* const headVarLocs;

        BodyGvLinkInfo(int const bodyPredIdx, int const bodyArgIdx, std::vector<int>* const headVarLocs);
        ~BodyGvLinkInfo();
    };

    /**
     * This class is used for marking the location of a pre-LV (PLV) in the body and one of the corresponding LV in the head.
     *
     * @since 2.0
     */
    class PlvLoc {
    public:
        /** The index of the body predicate */
        int bodyPredIdx;
        /** The index of the argument in the predicate */
        int bodyArgIdx;
        /** The index of the argument in the head */
        int headArgIdx;

        /**
         * Create empty instance
         */
        PlvLoc();

        PlvLoc(int const bodyPredIdx, int const bodyArgIdx, int const headArgIdx);
        PlvLoc(const PlvLoc& another);
        bool isEmpty() const;
        void setEmpty();
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
        typedef std::vector<CompliedBlock*> entryType;
        typedef std::vector<entryType*> entriesType;

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
        const entriesType& getPosCache() const;
        const entriesType& getEntCache() const;
        const entriesType& getAllCache() const;

    protected:
        /** The original KB */
        SimpleKb& kb;
        /** The cache for the positive entailments (E+-cache) (not entailed). One cache fragment is sufficient as all
         *  predicates are linked to the head. */
        entriesType* posCache;
        /** This cache is used to monitor the already-entailed records (T-cache). One cache fragment is sufficient as all
         *  predicates are linked to the head. */
        entriesType* entCache;
        /** The cache for all the entailments (E-cache). The cache is composed of multiple fragments, each of which maintains
         *  a linked component of the rule body. The first relation should not be included, as the head should be removed
         *  from E-cache */
        entriesType* allCache;
        /** The list of a PLV in the body. This list should always be of the same length as "limitedVarCnts" */
        std::vector<PlvLoc> plvList;
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
         * Append a raw complied block to each entry of the cache.
         *
         * Note: all indices should be up-to-date in the entries
         *
         * @param cache The original cache
         * @param predSymbol The numeration of the appended relation
         * @return A new cache containing all updated cache entries
         */
        entriesType* appendCacheEntries(entriesType* cache, IntTable* const newRelation);

        /**
         * Split entries in a cache according to two arguments in the rule.
         *
         * Note: all indices should be up-to-date in the entries
         *
         * @param cache The original cache
         * @param predIdx1 The 1st predicate index
         * @param argIdx1 The argument index in the 1st predicate
         * @param predIdx2 The 2nd predicate index
         * @param argIdx2 The argument index in the 2nd predicate
         * @return A new cache containing all updated cache entries
         */
        entriesType* splitCacheEntries(entriesType* cache, int predIdx1, int argIdx1, int predIdx2, int argIdx2);

        /**
         * Assign a constant to an argument in each cache entry.
         *
         * Note: all indices should be up-to-date in the entries
         *
         * @param cache The original cache
         * @param predIdx The index of the modified predicate
         * @param argIdx The index of the argument in the predicate
         * @param constant The numeration of the constant
         * @return A new cache containing all updated cache entries
         */
        entriesType* assignCacheEntries(entriesType* cache, int predIdx, int argIdx, int constant);

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
        void addCompleteBodyPlvBindings(
                std::unordered_set<Record>& completeBindings, std::unordered_set<Record>* const plvBindingSets, int* const argTemplate,
                int const bindingSetIdx, int const templateStartIdx, int const numSets, int const arity
        ) const ;

        /**
         * Recursively add PLV bindings to the linked head arguments.
         *
         * @param headTemplates The set of head templates
         * @param plvBindingSets The bindings of PLVs grouped by predicate
         * @param plvLinkLists The linked arguments in the head for each PLV
         * @param template An argument list template
         * @param linkIdx The index of the PLV group
         */
        void addBodyPlvBindings2HeadTemplates(
                std::unordered_set<Record>& headTemplates, std::unordered_set<Record>* const plvBindingSets,
                std::vector<BodyGvLinkInfo>** const plvLinkLists, int* const argTemplate, int const linkIdx, int const numSets,
                int const arity
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