#pragma once

#include "sincWithCache.h"

namespace sinc {
    /**
     * This class is to denote the correspondence of two LVs in a certain predicate. E.g., the variables X and Y in the 2nd
     * predicate of the following rule:
     *
     *   p(X, Y) :- q(X, Z), r(Z, Y)
     *
     * Such links compose link paths between LVs, such as "X->Z->Y" in the above rule.
     *
     * @since 2.1
     */
    class VarLink {
    public:
        int predIdx;
        int fromVid;
        int fromArgIdx;
        int toVid;
        int toArgIdx;

        VarLink(int const predIdx, int const fromVid, int const fromArgidx, int const toVid, int const toArgIdx);
        VarLink(const VarLink& another);
        VarLink(const VarLink&& another);

        VarLink& operator=(const VarLink& another);
        bool operator==(const VarLink& another) const;
        size_t hash() const;
    };
}

/**
 * This is for hashing "VarLink" in unordered containers.
 */
template <>
struct std::hash<sinc::VarLink> {
    size_t operator()(const sinc::VarLink& r) const;
};

template <>
struct std::hash<const sinc::VarLink> {
    size_t operator()(const sinc::VarLink& r) const;
};

/**
 * This is for hashing "VarLink*" in unordered containers.
 */
template <>
struct std::hash<sinc::VarLink*> {
    size_t operator()(const sinc::VarLink *r) const;
};

template <>
struct std::hash<const sinc::VarLink*> {
    size_t operator()(const sinc::VarLink *r) const;
};

/**
 * This is for checking equivalence "VarLink*" in unordered containers.
 */
template <>
struct std::equal_to<sinc::VarLink*> {
    bool operator()(const sinc::VarLink *r1, const sinc::VarLink *r2) const;
};

template <>
struct std::equal_to<const sinc::VarLink*> {
    bool operator()(const sinc::VarLink *r1, const sinc::VarLink *r2) const;
};

namespace sinc {
    /**
     * A pair of LV IDs.
     *
     * @since 2.1
     */
    class VarPair {
    public:
        int vid1;
        int vid2;

        VarPair(int const vid1, int const vid2);
        VarPair(const VarPair& another);
        VarPair(const VarPair&& another);

        VarPair& operator=(const VarPair& another);
        bool operator==(const VarPair& another) const;
        size_t hash() const;
    };
}

/**
 * This is for hashing "VarPair" in unordered containers.
 */
template <>
struct std::hash<sinc::VarPair> {
    size_t operator()(const sinc::VarPair& r) const;
};

template <>
struct std::hash<const sinc::VarPair> {
    size_t operator()(const sinc::VarPair& r) const;
};

/**
 * This is for hashing "VarPair*" in unordered containers.
 */
template <>
struct std::hash<sinc::VarPair*> {
    size_t operator()(const sinc::VarPair *r) const;
};

template <>
struct std::hash<const sinc::VarPair*> {
    size_t operator()(const sinc::VarPair *r) const;
};

/**
 * This is for checking equivalence "VarPair*" in unordered containers.
 */
template <>
struct std::equal_to<sinc::VarPair*> {
    bool operator()(const sinc::VarPair *r1, const sinc::VarPair *r2) const;
};

template <>
struct std::equal_to<const sinc::VarPair*> {
    bool operator()(const sinc::VarPair *r1, const sinc::VarPair *r2) const;
};

namespace sinc {
    /**
     * This class is used for quickly determine whether two LVs in a rule is linked in the body. It also searches for the
     * shortest link path of these vars.
     *
     * @since 2.1
     */
    class BodyVarLinkManager {
    public:
        typedef std::vector<std::unordered_set<VarLink>> varLinkGraphType;

        /**
         * Construct an instance
         *
         * @param rule        The current structure of the rule
         * @param currentVars The number of LVs used in the rule
         */
        BodyVarLinkManager(std::vector<Predicate>* const rule, int const currentVars);

        /**
         * Copy constructor.
         *
         * NOTE: the var link manager instance should be linked to the copied rule structure, instead of the original one
         *
         * @param another Another instance
         * @param newRule The copied rule structure in the copied rule
         */
        BodyVarLinkManager(const BodyVarLinkManager& another, std::vector<Predicate>* const newRule);

        /**
         * Update the instance via case-1 specialization
         */
        void specOprCase1(int const predIdx, int const argIdx, int const varId);

        /**
         * Update the instance via case-3 specialization operation
         */
        void specOprCase3(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2);

        /**
         * Update the instance via case-4 specialization operation. Here only the index of the predicate that was NOT newly
         * added should be passed here, as well as the corresponding argument index.
         *
         * @param predIdx The index of the predicate that was NOT newly added
         * @param argIdx  Corresponding index of the argument in the predicate
         */
        void specOprCase4(int const predIdx, int const argIdx);

        /**
         * Return the newly linked LV pairs if the rule is updated by a case-1 specialization.
         *
         * @param predIdx NOTE: this parameter should not be 0 (HEAD_PRED_IDX) here
         * 
         * @return `nullptr` if no such pair.
         * 
         * NOTE: The returned vector SHOULD be maintained by USER
         */
        std::unordered_set<VarPair>* assumeSpecOprCase1(int const predIdx, int const argIdx, int const varId) const;

        /**
         * Return the newly linked LV pairs if the rule is updated by a case-3 specialization.
         *
         * NOTE: the new LV is not included in the returned set
         *
         * @param predIdx1 NOTE: this parameter should not be 0 (HEAD_PRED_IDX) here, and predIdx1 != predIdx2
         * @param predIdx2 NOTE: this parameter should not be 0 (HEAD_PRED_IDX) here, and predIdx1 != predIdx2
         * 
         * @return `nullptr` if no such pair.
         * 
         * NOTE: The returned vector SHOULD be maintained by USER
         */
        std::unordered_set<VarPair>* assumeSpecOprCase3(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) const;

        /**
         * Find the shortest (directed) link path between two LVs.
         *
         * @return `nullptr` if the two LVs are not linked.
         */
        std::vector<VarLink>* shortestPath(int fromVid, int toVid) const;

        /**
         * Find the shortest path between two LVs if a certain UV is turned to an existing LV via case-1 specialization.
         *
         * @param predIdx NOTE: this parameter should not be 0 (HEAD_PRED_IDX) here
         * @return `nullptr` if no such path
         * 
         * NOTE: The returned vector SHOULD be maintained by USER
         */
        std::vector<VarLink>* assumeShortestPathCase1(
            int const predIdx, int const argIdx, int const varId, int const fromVid, int const toVid
        ) const;

        /**
         * Find the shortest path between two LVs if 2 certain UVs are turned to an existing LV via case-3 specialization.
         *
         * @param predIdx1 NOTE: this parameter should not be 0 (Rule.HEAD_PRED_IDX) here
         * @param predIdx2 NOTE: this parameter should not be 0 (Rule.HEAD_PRED_IDX) here
         * @return `nullptr` if no such path
         * 
         * NOTE: The returned vector SHOULD be maintained by USER
         */
        std::vector<VarLink>* assumeShortestPathCase3(
            int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2, int const fromVid, int const toVid
        ) const;

        /**
         * Find the shortest path between two LVs if 2 certain UVs are turned to an existing LV via case-3 specialization.
         *
         * NOTE: this method assumes another UV is in the head, and the end of the path is the newly converted LV. Therefore,
         * there is no parameter specifying the target LV.
         *
         * @param predIdx NOTE: this parameter should not be 0 (Rule.HEAD_PRED_IDX) here
         * @return `nullptr` if no such path
         * 
         * NOTE: The returned vector SHOULD be maintained by USER
         */
        std::vector<VarLink>* assumeShortestPathCase3(int const predIdx, int const argIdx, int const fromVid) const;

    protected:
        /** The structure of the rule */
        std::vector<Predicate> const* const rule;
        /** The labels of LVs, denoting the linked component they belong to */
        std::vector<int> varLabels;
        /** The link graph of LVs. There is an edge between two vars if they appear in a same predicate */
        varLinkGraphType varLinkGraph;

        /**
         * This method is to continue a BFS search based on a certain starting status.
         *
         * @param visitedVid     An array denoting whether an LV has been visited
         * @param bfs            The BFS array of edges
         * @param predecessorIdx The array where each element denoting the index of the predecessor edge of the corresponding edge in 'bfs'
         * @param toVid          The target LV
         * @return The shortest path to the target LV, or `nullptr` if no such path.
         * 
         * NOTE: The returned vector SHOULD be maintained by USER
         */
        std::vector<VarLink>* shortestPathBfsHandler(
            bool* const visitedVid, std::vector<VarLink>& bfs, std::vector<int>& predecessorIdx, int const toVid
        ) const;


        /**
         * This method is for finding the shortest path from the given argument to an LV.
         *
         * @param predIdx        The index of the predicate of the starting argument
         * @param argIdx         The corresponding argument index
         * @param assumedFromVid An assumed ID of the argument. NOTE: this ID should be different from existing LVs
         * @param toVid          The target LV
         * @return The shortest path, or `nullptr` of none.
         * 
         * NOTE: The returned vector SHOULD be maintained by USER
         */
        std::vector<VarLink>* assumeShortestPathHandler(int const predIdx, int const argIdx, int const assumedFromVid, int const toVid) const;
    };

    class SpecOprWithScore {
    public:
        /** NOTE: This pointer SHOULD be maintained by the object */
        SpecOpr const* opr;
        Eval estEval;

        /**
         * @param opr NOTE: This pointer SHOULD be maintained by the object
         */
        SpecOprWithScore(SpecOpr const* opr, const Eval& eval);
        SpecOprWithScore(SpecOprWithScore&& another);
        ~SpecOprWithScore();

        SpecOprWithScore& operator=(SpecOprWithScore&& another);
    };

    /**
     * Monitoring information for cache and estimation in SInC. Time is measured in nanoseconds.
     *
     * @since 2.6
     */
    class EstSincPerfMonitor : public PerformanceMonitor {
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

        /* Memory cost (KB) */
        size_t cbMemCost = 0;
        size_t maxEstIdxCost = 0;

        void show(std::ostream& os) override;
    };

    /**
     * This rule is a sub-class of cached rule that estimates the evaluation of each possible specialization.
     * 
     * NOTE: The estimation of this rule involves E+-cache, T-cache, and E-cache. Therefore, it directly inherits `Rule` and re-implements
     * most methods of `CachedRule`.
     *
     * @since 2.6
     */
    using sinc::CacheFragment;
    class EstRule : public Rule {
    public:
        EstRule(
            int const headPredSymbol, int const arity, fingerprintCacheType& fingerprintCache, tabuMapType& category2TabuSetMap, SimpleKb& kb
        );

        EstRule(const EstRule& another);
        ~EstRule();
        EstRule* clone() const override;

        /**
         * Enumerate specializations of this rule and calculate corresponding estimation scores
         */
        std::vector<SpecOprWithScore*>* estimateSpecializations();

        /* The followings are methods in `CachedRule` */
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
        size_t getEstIdxMemCost() const;

    protected:
        BodyVarLinkManager bodyVarLinkManager;
        size_t estIdxMemCost = 0;

        /* Followings are members in `CachedRule` */
        SimpleKb& kb;
        CacheFragment* posCache;
        CacheFragment* entCache;
        std::vector<CacheFragment*>* allCache;
        std::vector<TabInfo> predIdx2AllCacheTableInfo;
        bool maintainPosCache;
        bool maintainEntCache;
        bool maintainAllCache;
        uint64_t copyTime = 0;
        uint64_t posCacheUpdateTime = 0;
        uint64_t entCacheUpdateTime = 0;
        uint64_t allCacheUpdateTime = 0;
        uint64_t posCacheIndexingTime = 0;
        uint64_t entCacheIndexingTime = 0;
        uint64_t allCacheIndexingTime = 0;

        void specCase1UpdateStructure(int const predIdx, int const argIdx, int const varId) override;
        void specCase3UpdateStructure(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) override;
        void specCase4UpdateStructure(
            int const predSymbol, int const arity, int const argIdx1, int const predIdx2, int const argIdx2
        ) override;

        double itemsCountOfKeys(sinc::MultiSet<int> const& counterSet, sinc::MultiSet<int> const& setOfKeys) const;
        double estimateRatiosInPosCache(double const* const ratios, int const length) const;
        double estimateRatiosInAllCache(double const* const ratios, int const length) const;
        double estimateLinkVarRatio(std::vector<VarLink> const& varLinkPath, std::vector<MultiSet<int>*> columnValuesInCache) const;

        /* Followings are methods in `CachedRule` */
        void obtainPosCache();
        void obtainEntCache();
        void obtainAllCache();
        double recordCoverage() override;
        Eval calculateEval() override;
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
        void mergeFragmentIndices(int const baseFragmentIdx, int const mergingFragmentIdx);
        void generateHeadTemplates(
            std::unordered_set<Record>& headTemplates, std::unordered_set<Record>** const bindingsInFragments,
            std::vector<std::vector<int>>* const headArgIdxLists, int* const argTemplate,
            std::vector<int> const& validFragmentIndices, int const headArity, int const fragIdx
        ) const;
        void expandHeadUvs4CounterExamples(
            SimpleRelation const& targetRelation, std::unordered_set<Record>& counterexamples, int* const recTemplate,
            std::vector<int>** varLocs, int const idx, int const numVectors
        ) const;
    };

    /**
     * Mines a relation with `EstRule`.
     * 
     * NOTE: This class copies and modifies the implementation of `RelationMinerWithCachedRule`.
     * 
     * @since 2.6
     */
    class EstRelationMiner : public RelationMiner {
    public:
        EstSincPerfMonitor monitor;

        /**
         * Construct by passing parameters from the compressor that loads the data.
         *
         * @param kb                   The input KB
         * @param targetRelation       The target relation in the KB
         * @param evalMetric           The rule evaluation metric
         * @param beamwidth            The beamwidth used in the rule mining procedure
         * @param observationRatio     The ratio (>=1) that extends the number of rules that are actually specialized according to the estimations
         * @param stopCompressionRatio The stopping compression ratio for inducing a single rule
         * @param predicate2NodeMap    The mapping from predicates to the nodes in the dependency graph
         * @param dependencyGraph      The dependency graph
         * @param logger               A logger
         */
        EstRelationMiner(
            SimpleKb& kb, int const targetRelation, EvalMetric::Value evalMetric, int const beamwidth, double const observationRatio,
            double const stopCompressionRatio, nodeMapType& predicate2NodeMap, depGraphType& dependencyGraph,
            std::vector<Rule*>& hypothesis, std::unordered_set<Record>& counterexamples, std::ostream& logger
        );

        ~EstRelationMiner();

    protected:
        std::vector<Rule::fingerprintCacheType*> fingerprintCaches;
        /** The ratio (>=1) that extends the number of rules that are actually specialized according to the estimations */
        double const observationRatio;

        /**
         * Create a rule with compact caching and estimation.
         */
        Rule* getStartRule() override;

        /**
         * When a rule r is selected as beam, update its cache indices. The rule r here is a "CachedRule".
         */
        void selectAsBeam(Rule* r) override;

        Rule* findRule() override;

        void findEstimatedSpecializations(Rule** beams, std::vector<SpecOprWithScore*>** estimatedSpecLists, Rule** topCandidates);

        /**
         * Record monitoring information compared to the super implementation
         */
        int checkThenAddRule(UpdateStatus updateStatus, Rule* const updatedRule, Rule& originalRule, Rule** candidates) override;
    };

    /**
     * Sinc with estimation pruning.
     * 
     * NOTE: This class copies and modifies the implementation of `SincWithCache`.
     * 
     * @since 2.6
     */
    class SincWithEstimation : public SInC {
    public:
        SincWithEstimation(SincConfig* const config);
        SincWithEstimation(SincConfig* const config, SimpleKb* const kb);

        /**
         * This overridden function added a reservation of memory space for the CB pool and indices after the conventional process.
         * 
         * @since 2.5
         */
        void getTargetRelations(int* & targetRelationIds, int& numTargets) override;

    protected:
        EstSincPerfMonitor monitor;

        SincRecovery* createRecovery() override;
        RelationMiner* createRelationMiner(int const targetRelationNum) override;
        void finalizeRelationMiner(RelationMiner* miner) override;
        void showMonitor() override;
        void finish() override;
    };
}