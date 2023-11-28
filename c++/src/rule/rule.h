#pragma once

#include <unordered_set>
#include "components.h"

/** The index of the head predicate */
#define HEAD_PRED_IDX 0
/** The beginning index of the body predicate */
#define FIRST_BODY_PRED_IDX 1
/** Minimum rule length */
#define MIN_LENGTH 0
#define DEFAULT_MIN_FACT_COVERAGE 0.05

namespace sinc {
    /**
     * The class of the basic rule structure. The class defines the basic structure of a rule and the basic operations that
     * manipulates the structure.
     * 
     * NOTE: Rule objects will insert fingerprint pointers to the `fingerprintCache`. The pointers in the set SHOULD be released
     * by USER, before destruction of the set.
     *
     * @since 1.0
     */
    class Rule {
    public:
        typedef std::unordered_set<const Fingerprint*> fingerprintCacheType;
        typedef std::unordered_map<MultiSet<int>*, fingerprintCacheType*> tabuMapType;

        /** The threshold of the coverage value for pruning */
        static double MinFactCoverage;

        /**
         * Parse a plain-text string into a rule structure. The allowed input can be defined by the following context-free
         * grammar (which is similar to Prolog):
         *
         * rule := predicate:-body
         * body := ε | predicate | predicate,body
         * predicate := pred_symbol(args)
         * args := ε | variable | constant | variable,args | constant,args
         *
         * A "variable" is defined by the following regular expression: [A-Z][a-zA-z0-9]*
         * A "pred_symbol" and a "constant" are defined by the following regex: [a-z][a-zA-z0-9]*
         *
         * If there are `n` LVs, the IDs of the variables range from `0` to `n-1`. Possible UVs are replaced as empty arguments.
         * 
         * NOTE: The returned vector pointer and pointers in the vectors SHOULD be maintained by USER.
         *
         * @return The rule structure is represented by a list of ParsedPred because there is no mapping information for the
         * numerations of the names.
         */
        static std::vector<ParsedPred*>* parseStructure(const std::string& ruleStr);

        /**
         * Parse a plain-text string into an argument.
         *
         * @return The argument is represented by an instance of ParsedArg because there is no mapping information for the
         * numerations of the names.
         */
        static ParsedArg* parseArg(const std::string& str, std::unordered_map<std::string, int>& variable2IdMap);

        static void releaseParsedStructure(std::vector<ParsedPred *>* parsedStructure);

        /**
         * Stringify a parsed rule structure.
         */
        static std::string toString(const std::vector<ParsedPred*>& parsedStructure);

        /**
         * Initialize the most general rule of a certain target head relation.
         *
         * @param headPredSymbol The functor of the head predicate, i.e., the target relation.
         * @param arity The arity of the functor
         * @param fingerprintCache The cache of the used fingerprints
         * @param category2TabuSetMap The tabu set of pruned fingerprints
         */
        Rule(int const headPredSymbol, int const arity, fingerprintCacheType& fingerprintCache, tabuMapType& category2TabuSetMap);

        Rule(const Rule& another);

        virtual ~Rule();

        /**
         * A clone method that correctly uses the copy constructor of any sub-class. The copy constructor may not be useful,
         * as a super class copy constructor generates only the super class object even on the sub-class object.
         *
         * @return A copy of the rule instance
         */
        virtual Rule* clone() const = 0;

        /**
         * Specialization Case 1: Convert a UV to an existing LV.
         *
         * @param predIdx The index of the predicate
         * @param argIdx The index of the argument
         * @param varId The id of the LV
         * @return The update status
         */
        UpdateStatus specializeCase1(int const predIdx, int const argIdx, int const varId);

        /**
         * Specialization Case 2: Add a new predicate and convert a UV in the new predicate to an existing LV.
         *
         * @param functor The functor of the new predicate
         * @param arity The arity of the new predicate
         * @param argIdx The index of the argument
         * @param varId The id of the LV
         * @return The update status
         */
        UpdateStatus specializeCase2(int const predSymbol, int const arity, int const argIdx, int const varId);

        /**
         * Specialization Case 3: Convert 2 UVs in the rule to a new LV.
         *
         * @param predIdx1 The index of the first predicate
         * @param argIdx1 The argument index in the first predicate
         * @param predIdx2 The index of the second predicate
         * @param argIdx2 The argument index in the second predicate
         * @return The update status
         */
        UpdateStatus specializeCase3(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2);

        /**
         * Specialization Case 4: Add a new predicate to the rule and convert 2 UVs in the rule to a new LV. Note: Exactly one of the
         * selected arguments are from the newly added predicate. 
         *
         * @param functor The functor of the new predicate
         * @param arity The arity of the new predicate
         * @param argIdx1 The argument index in the first predicate
         * @param predIdx2 The index of the second predicate
         * @param argIdx2 The argument index in the second predicate
         * @return The update status
         */
        UpdateStatus specializeCase4(int const predSymbol, int const arity, int const argIdx1, int const predIdx2, int const argIdx2);

        /**
         * Specialization Case 5: Convert a UV to a constant.
         *
         * @param predIdx The index of the predicate
         * @param argIdx The index of the argument
         * @param constant The numeration of the constant symbol
         * @return The update status
         */
        UpdateStatus specializeCase5(int const predIdx, int const argIdx, int const constant);

        /**
         * Generalization: Remove an assignment to the argument.
         *
         * @param predIdx The index of the predicate
         * @param argIdx The index of the argument
         * @return The update status
         */
        UpdateStatus generalize(int const predIdx, int const argIdx);

        /**
         * Calculate the evidence of positively entailed facts in the KB. Each piece of evidence is an ordered array of
         * predicates, where the head is the first.
         *
         * @return The evidence generated by the rule.
         */
        virtual EvidenceBatch* getEvidenceAndMarkEntailment() = 0;

        /**
         * Release unnecessary memory space after the rule is included in the hypothesis.
         */
        virtual void releaseMemory() = 0;

        /**
         * Calculate the counterexamples generated by the rule.
         *
         * @return The set of counterexamples.
         */
        virtual std::unordered_set<Record>* getCounterexamples() const = 0;

        Predicate const& getPredicate(int const idx) const;
        Predicate const& getHead() const;
        int getLength() const;
        int usedLimitedVars() const;
        int numPredicates() const;
        virtual Eval const& getEval() const;
        Fingerprint const& getFingerprint() const;

        /**
         * Convert the rule to a string under the following structure: (<Eval>)<Structure>.
         *
         * @param names An array of names where the indices are the relation indices.
         */
        std::string toString(const char* const names[]) const;

        /**
         * Convert the rule to a string under the following structure: (<Eval>)<Structure>.
         * Without a numeration map, the integers will not be translated.
         */
        std::string toString() const;

        /**
         * Convert only the rule structure to a string.
         *
         * @param names An array of names where the indices are the relation indices.
         */
        std::string toDumpString(const char* const names[]) const;

        /**
         * Convert only the rule structure to a string. Without a numeration map, the integers will not be translated.
         */
        std::string toDumpString() const;

        uint64_t getFingerprintCreationTime() const;
        uint64_t getPruningTime() const;
        uint64_t getEvalTime() const;
        bool operator==(const Rule &another) const;
        size_t hash() const;

        virtual size_t memoryCost() const;

    protected:
        /** The cache of all used fingerprints */
        fingerprintCacheType& fingerprintCache;
        /** The fingerprint tabu set of all rules that are pruned due to insufficient coverage */
        tabuMapType& category2TabuSetMap;
        /** The structure of the rule */
        std::vector<Predicate> structure;
        /** The arguments that are assigned to limited variables of certain IDs */
        std::vector<std::vector<ArgLocation>*> limitedVarArgs;
        /** The fingerprint of the rule */
        Fingerprint* fingerprint;
        /** 
         * Whether the pointer of the fingerprint should be released in destructor.
         * 
         * This is `true` when a newly created fingerprint has failed to be added to the fingerprint cache set.
         */
        bool releaseFingerprint;
        /** The rule length */
        int length;
        Eval eval;

        /* Performance monitoring members (measured in nanoseconds) */
        uint64_t fingerprintCreationTime = 0;
        uint64_t pruningTime = 0;
        uint64_t evalTime = 0;

        void updateFingerprint();

        /**
         * Check if the rule structure is invalid. The structure is invalid if:
         *   1. It contains duplicated predicates
         *   2. It contains independent Fragment
         */
        bool isInvalid();

        /**
         * Check if the rule structure has already been verified and added to the cache. If not, add the rule fingerprint to
         * the cache.
         */
        bool cacheHit();

        /**
         * Check if the rule structure should be pruned by the tabu set.
         */
        bool tabuHit();

        /**
         * Enumerate the subcategories of the rule of a certain size.
         * 
         * NOTE: The returned pointer and pointers in the set SHOULD all be maintained by USER.
         *
         * @param subsetSize The size of the subcategories
         * @return A set of subcategories
         */
        std::unordered_set<MultiSet<int>*>* categorySubsets(int const subsetSize) const;

        /**
         * Handler for enumerating the subcategories.
         */
        void templateSubsetsHandler(
            std::unordered_set<MultiSet<int>*>* const subsets, int* const templateArr, int const arrLength, int const depth,
            int const startIdx
        ) const;

        /**
         * Check if the coverage of the rule is below the threshold. If so, add the fingerprint to the tabu set.
         */
        bool insufficientCoverage();

        /**
         * Calculate the record coverage of the rule.
         */
        virtual double recordCoverage() = 0;

        /**
         * Add this rule to the tabu map
         */
        void add2TabuMap();

        /**
         * Calculate the evaluation of the rule.
         */
        virtual Eval calculateEval() = 0;

        void updateEval();

        /**
         * Update rule structure for case 1.
         */
        virtual void specCase1UpdateStructure(int const predIdx, int const argIdx, int const varId);

        /**
         * Handler for case 1, before the coverage check. This function is for the implementation of more actions during
         * the rule update in the sub-classes.
         */
        virtual UpdateStatus specCase1HandlerPrePruning(int const predIdx, int const argIdx, int const varId) = 0;

        /**
         * Handler for case 1, after the coverage check. This function is for the implementation of more actions during
         * the rule update in the sub-classes.
         */
        virtual UpdateStatus specCase1HandlerPostPruning(int const predIdx, int const argIdx, int const varId) = 0;

        /**
         * Update rule structure for case 2.
         */
        virtual void specCase2UpdateStructure(int const predSymbol, int const arity, int const argIdx, int const varId);

        /**
         * Handler for case 2, before the coverage check. This function is for the implementation of more actions during
         * the rule update in the sub-classes.
         *
         * @param newPredicate The newly created predicate in the updated structure
         */
        virtual UpdateStatus specCase2HandlerPrePruning(int const predSymbol, int const arity, int const argIdx, int const varId) = 0;

        /**
         * Handler for case 2, after the coverage check. This function is for the implementation of more actions during
         * the rule update in the sub-classes.
         *
         * @param newPredicate The newly created predicate in the updated structure
         */
        virtual UpdateStatus specCase2HandlerPostPruning(int const predSymbol, int const arity, int const argIdx, int const varId) = 0;

        /**
         * Update rule structure for case 3.
         */
        virtual void specCase3UpdateStructure(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2);

        /**
         * Handler for case 3, before the coverage check. This function is for the implementation of more actions during
         * the rule update in the sub-classes.
         */
        virtual UpdateStatus specCase3HandlerPrePruning(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) = 0;

        /**
         * Handler for case 3, after the coverage check. This function is for the implementation of more actions during
         * the rule update in the sub-classes.
         */
        virtual UpdateStatus specCase3HandlerPostPruning(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) = 0;

        /**
         * Update rule structure for case 4.
         */
        virtual void specCase4UpdateStructure(
            int const predSymbol, int const arity, int const argIdx1, int const predIdx2, int const argIdx2
        );

        /**
         * Handler for case 4, before the coverage check. This function is for the implementation of more actions during
         * the rule update in the sub-classes.
         *
         * @param newPredicate The newly created predicate in the updated structure
         */
        virtual UpdateStatus specCase4HandlerPrePruning(
            int const predSymbol, int const arity, int const argIdx1, int const predIdx2, int const argIdx2
        ) = 0;

        /**
         * Handler for case 4, after the coverage check. This function is for the implementation of more actions during
         * the rule update in the sub-classes.
         *
         * @param newPredicate The newly created predicate in the updated structure
         */
        virtual UpdateStatus specCase4HandlerPostPruning(
            int const predSymbol, int const arity, int const argIdx1, int const predIdx2, int const argIdx2
        ) = 0;

        /**
         * Update rule structure for case 5.
         */
        virtual void specCase5UpdateStructure(int const predIdx, int const argIdx, int const constant);

        /**
         * Handler for case 5, before the coverage check. This function is for the implementation of more actions during
         * the rule update in the sub-classes.
         */
        virtual UpdateStatus specCase5HandlerPrePruning(int const predIdx, int const argIdx, int const constant) = 0;

        /**
         * Handler for case 5, after the coverage check. This function is for the implementation of more actions during
         * the rule update in the sub-classes.
         */
        virtual UpdateStatus specCase5HandlerPostPruning(int const predIdx, int const argIdx, int const constant) = 0;

        /**
         * Update rule structure for generalization.
         */
        virtual void generalizeUpdateStructure(int const predIdx, int const argIdx);

        /**
         * Handler for generalization, before the coverage check. This function is for the implementation of more actions during
         * the rule update in the sub-classes.
         */
        virtual UpdateStatus generalizeHandlerPrePruning(int const predIdx, int const argIdx) = 0;

        /**
         * Handler for generalization, after the coverage check. This function is for the implementation of more actions during
         * the rule update in the sub-classes.
         */
        virtual UpdateStatus generalizeHandlerPostPruning(int const predIdx, int const argIdx) = 0;
    };

    /**
     * A rule implementation with barely nothing but the structure and fingerprint operations. This class is used for loading
     * Horn rules from strings. It can also be used for testing basic operations in the abstract class 'Rule' or for
     * manipulating the rule structure only.
     *
     * @since 1.0
     */
    class BareRule : public Rule {
    public:
        Eval returningEval;
        double coverage = 1.0;
        UpdateStatus case1PrePruningStatus = UpdateStatus::Normal;
        UpdateStatus case1PostPruningStatus = UpdateStatus::Normal;
        UpdateStatus case2PrePruningStatus = UpdateStatus::Normal;
        UpdateStatus case2PostPruningStatus = UpdateStatus::Normal;
        UpdateStatus case3PrePruningStatus = UpdateStatus::Normal;
        UpdateStatus case3PostPruningStatus = UpdateStatus::Normal;
        UpdateStatus case4PrePruningStatus = UpdateStatus::Normal;
        UpdateStatus case4PostPruningStatus = UpdateStatus::Normal;
        UpdateStatus case5PrePruningStatus = UpdateStatus::Normal;
        UpdateStatus case5PostPruningStatus = UpdateStatus::Normal;
        UpdateStatus generalizationPrePruningStatus = UpdateStatus::Normal;
        UpdateStatus generalizationPostPruningStatus = UpdateStatus::Normal;
        /* This pointer SHOULD be maintained by USER */
        EvidenceBatch* returningEvidence = nullptr;
        /* This pointer SHOULD be maintained by USER */
        std::unordered_set<Record>* returningCounterexamples = nullptr;

        BareRule(int const headPredSymbol, int arity, fingerprintCacheType& fingerprintCacheType, tabuMapType& category2TabuSetMap);
        BareRule(const BareRule& another);
        ~BareRule();
        BareRule* clone() const override;
        EvidenceBatch* getEvidenceAndMarkEntailment() override;
        void releaseMemory() override;
        std::unordered_set<Record>* getCounterexamples() const override;
        Eval const& getEval() const override;

    protected:
        double recordCoverage() override;
        Eval calculateEval() override;
        UpdateStatus specCase1HandlerPrePruning(int const predIdx, int const argIdx, int const varId) override;
        UpdateStatus specCase1HandlerPostPruning(int const predIdx, int const argIdx, int const varId) override;
        UpdateStatus specCase2HandlerPrePruning(int const predSymbol, int const arity, int const argIdx, int const varId) override;
        UpdateStatus specCase2HandlerPostPruning(int const predSymbol, int const arity, int const argIdx, int const varId) override;
        UpdateStatus specCase3HandlerPrePruning(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) override;
        UpdateStatus specCase3HandlerPostPruning(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) override;
        UpdateStatus specCase4HandlerPrePruning(int const predSymbol, int const arity, int const argIdx1, int const predIdx2, int const argIdx2) override;
        UpdateStatus specCase4HandlerPostPruning(int const predSymbol, int const arity, int const argIdx1, int const predIdx2, int const argIdx2) override;
        UpdateStatus specCase5HandlerPrePruning(int const predIdx, int const argIdx, int const constant) override;
        UpdateStatus specCase5HandlerPostPruning(int const predIdx, int const argIdx, int const constant) override;
        UpdateStatus generalizeHandlerPrePruning(int const predIdx, int const argIdx) override;
        UpdateStatus generalizeHandlerPostPruning(int const predIdx, int const argIdx) override;
    };
}