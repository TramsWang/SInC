#pragma once

#include <string>
#include "../util/common.h"
#include "../util/util.h"

#define _SYMBOL_COMPRESSION_RATIO "τ"
#define _SYMBOL_COMPRESSION_CAPACITY "δ"
#define _SYMBOL_INFO_GAIN "h"
#define _DESC_COMPRESSION_RATIO "Compression Rate"
#define _DESC_COMPRESSION_CAPACITY "Compression Capacity"
#define _DESC_INFO_GAIN "Information Gain"
#define COMP_RATIO_USEFUL_THRESHOLD 0.5

namespace sinc {
    /**
     * The status of rule update.
     *
     * @since 1.0
     */
    enum UpdateStatus {
        /** The update is successful */
        Normal,
        /** The updated rule is duplicated and pruned */
        Duplicated,
        /** The updated rule structure is invalid (only happens when generalization is on) */
        Invalid,
        /** The updated rule is pruned due to insufficient fact coverage */
        InsufficientCoverage,
        /** The updated rule is pruned by the tabu set */
        TabuPruned
    };

    /**
     * Enumeration of quality evaluation metrics.
     *
     * @since 1.0
     */
    class EvalMetric {
    public:
        /**
         * Enumeration value of types of evaluation metrics
         */
        enum Value {
            /** Compression Rate */
            CompressionRatio,
            /** Compression Capacity */
            CompressionCapacity,
            /** Information Gain (proposed in FOIL) */
            InfoGain
        };

        static const std::string SYMBOL_COMPRESSION_RATIO;
        static const std::string SYMBOL_COMPRESSION_CAPACITY;
        static const std::string SYMBOL_INFO_GAIN;
        static const std::string DESC_COMPRESSION_RATIO;
        static const std::string DESC_COMPRESSION_CAPACITY;
        static const std::string DESC_INFO_GAIN;

        static const std::string& getSymbol(Value v);
        static const std::string& getDescription(Value v);
        static Value getBySymbol(const std::string& symbol);
    };

    /**
     * The quality evaluation of rules for compressing knowledge bases.
     * 
     * @since 1.0
     */
    class Eval {
    public:
        /**
         * Initialize a evaluation score.
         *
         * @param posEtls The total number of positive entailments
         * @param allEtls The total number of entailments
         * @param ruleLength The length of the rule
         */
        Eval(double const posEtls, double const allEtls, int const ruleLength);

        Eval(const Eval& another);

        /**
         * Get the score value of certain metric type.
         *
         * @param type The type of the evaluation metric
         * @return The evaluation score
         */
        double value(EvalMetric::Value type) const;

        /**
         * Check whether the evaluation score indicates a useful rule for compression.
         */
        bool useful() const;

        double getAllEtls() const;
        double getPosEtls() const;
        double getNegEtls() const;
        double getRuleLength() const;
        std::string toString() const;

        bool operator==(const Eval& another) const;
        Eval& operator=(const Eval& another);
        Eval& operator=(const Eval&& another);

    protected:
        /** The number of positive entailments (double type to be the same as 'negEtls') */
        double posEtls;
        /** The number of negative entailments (double type because values may be extremely large) */
        double negEtls;
        /** The number of total entailments (double type because values may be extremely large) */
        double allEtls;
        /** The length of a rule */
        int ruleLength;
        /** The score of compression ratio */
        double compRatio;
        /** The score of compression capacity */
        double compCapacity;
        /** The score of information gain */
        double infoGain;
    };

    /**
     * This class is for a batch of evidence generated by a single rule. The structure of evidence looks like this:
     *
     *          Rel1    Rel2    ...     Reln
     *  Ev1     rec11   rec12   ...     rec1n
     *  Ev2     rec21   rec22   ...     rec2n
     *  ...
     *  Evk     reck1   reck2   ...     reckn
     * 
     * NOTE: Each `int*` in each grounding SHOULD be maintained by USER.
     *
     * @since 2.0
     */
    class EvidenceBatch {
    public:
        /** The number of predicates in rule */
        int const numPredicates;
        /** The relation of each predicate in the rule */
        int* const predicateSymbolsInRule;
        /** The arity of each predicate in the rule */
        int* const aritiesInRule;
        /** The list of evidence. Each list is a piece of evidence */
        std::vector<int**> evidenceList;

        EvidenceBatch(int const numPredicates);
        ~EvidenceBatch();
        void showGroundings() const;
        static void showGroundings(std::unordered_set<std::vector<Record>> const& groundings);
    };

    /**
     * Arguments in this class are equivalent classes. Each equivalent class is corresponding to some argument in the
     * original rule.
     * 
     * @since 1.0
     */
    class PredicateWithClass {
    public:
        typedef MultiSet<ArgIndicator*> equivalenceClassType;

        int const functor;
        int const arity;
        /** An array of pointers to equivalent classes. Pointers of equivalent classes are maintained by USER. */
        equivalenceClassType** const classArgs;

        PredicateWithClass(int const functor, int const arity);
        ~PredicateWithClass();

        bool operator==(const PredicateWithClass &another) const;
        size_t hash() const;
    };
}

/**
 * This is for hashing "PredicateWithClass" in unordered containers.
 */
template <>
struct std::hash<sinc::PredicateWithClass> {
    size_t operator()(const sinc::PredicateWithClass& r) const;
};

template <>
struct std::hash<const sinc::PredicateWithClass> {
    size_t operator()(const sinc::PredicateWithClass& r) const;
};

/**
 * This is for hashing "PredicateWithClass*" in unordered containers.
 */
template <>
struct std::hash<sinc::PredicateWithClass*> {
    size_t operator()(const sinc::PredicateWithClass *r) const;
};

template <>
struct std::hash<const sinc::PredicateWithClass*> {
    size_t operator()(const sinc::PredicateWithClass *r) const;
};

/**
 * This is for checking equivalence "PredicateWithClass*" in unordered containers.
 */
template <>
struct std::equal_to<sinc::PredicateWithClass*> {
    bool operator()(const sinc::PredicateWithClass *r1, const sinc::PredicateWithClass *r2) const;
};

template <>
struct std::equal_to<const sinc::PredicateWithClass*> {
    bool operator()(const sinc::PredicateWithClass *r1, const sinc::PredicateWithClass *r2) const;
};

namespace sinc {
    /**
     * The fingerprint class that quickly tells whether two rules are equivalent.
     *
     * NOTE: The fingerprint structure may incorrectly tell two rules are identical. For instance:
     *
     *   1.1 p(X,Y) :- f(X,X), f(?,Y)
     *   1.2 p(X,Y) :- f(X,Y), f(?,X)
     *
     *   2.1 p(X,Y) :- f(X,?), f(Z,Y), f(?,Z)
     *   2.2 p(X,Y) :- f(X,Z), f(?,Y), f(Z,?)
     *
     * The fingerprint structure may also incorrectly tell one rule is the specialization of another. For more examples,
     * please refer to the test class.
     *
     * @since 1.0
     */
    class Fingerprint {
    public:
        typedef MultiSet<ArgIndicator*> equivalenceClassType;
        /**
         * Construct the fingerprint with the structure of the rule.
         *
         * @param rule The rule structure (ordered list of predicates, where the head is the first)
         */
        Fingerprint(const std::vector<Predicate>& rule);
        ~Fingerprint();

        /** NOTE: The returned pointer should NOT be released */
        const MultiSet<equivalenceClassType*>& getEquivalenceClasses() const;

        /** NOTE: The returned pointer should NOT be released */
        const std::vector<PredicateWithClass>& getClassedStructure() const;

        /**
         * Check for generalizations.
         *
         * @param another The fingerprint of another rule
         * @return Ture if the rule of this fingerprint is the generalization of the other.
         */
        bool generalizationOf(const Fingerprint& another) const;

        bool operator==(const Fingerprint &another) const;
        size_t hash() const;
        static void releaseEquivalenceClass(equivalenceClassType* equivalenceClass);

    protected:
        /** The equivalence classes in this fingerprint */
        MultiSet<equivalenceClassType*> equivalenceClasses;
        /** Stores pointers to all equivalence class objects */
        std::vector<equivalenceClassType*> equivalenceClassPtrs;
        /** In this rule structure, each argument is the corresponding equivalence class */
        std::vector<PredicateWithClass> classedStructure;
        /** The rule that generates the fingerprint */
        const std::vector<Predicate>& rule;
        /** Fingerprint object are not modifiable. Thus hashCode can be calculated during construction and stored. */
        size_t hashCode;

        static bool generalizationOf(const PredicateWithClass& predicate, const PredicateWithClass& specializedPredicate);
    };
}

/**
 * This is for hashing "Fingerprint" in unordered containers.
 */
template <>
struct std::hash<sinc::Fingerprint> {
    size_t operator()(const sinc::Fingerprint& r) const;
};

/**
 * This is for hashing "Fingerprint*" in unordered containers.
 */
template <>
struct std::hash<sinc::Fingerprint*> {
    size_t operator()(const sinc::Fingerprint *r) const;
};

/**
 * This is for checking equivalence "Fingerprint*" in unordered containers.
 */
template <>
struct std::equal_to<sinc::Fingerprint*> {
    bool operator()(const sinc::Fingerprint *r1, const sinc::Fingerprint *r2) const;
};

/**
 * This is for hashing "Fingerprint" in unordered containers.
 */
template <>
struct std::hash<const sinc::Fingerprint> {
    size_t operator()(const sinc::Fingerprint& r) const;
};

/**
 * This is for hashing "Fingerprint*" in unordered containers.
 */
template <>
struct std::hash<const sinc::Fingerprint*> {
    size_t operator()(const sinc::Fingerprint *r) const;
};

/**
 * This is for checking equivalence "Fingerprint*" in unordered containers.
 */
template <>
struct std::equal_to<const sinc::Fingerprint*> {
    bool operator()(const sinc::Fingerprint *r1, const sinc::Fingerprint *r2) const;
};

namespace sinc {
    /**
     * Exception thrown when errors occur during Horn rule parsing
     *
     * @since 2.0
     */
    class RuleParseException : public std::exception {
    public:
        RuleParseException();
        RuleParseException(const std::string& msg);
        virtual const char* what() const throw();

    private:
        std::string message;
    };
}