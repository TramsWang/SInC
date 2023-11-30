#pragma once

#include <filesystem>
#include <iostream>
#include "../kb/simpleKb.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../util/common.h"
#include "../util/graphAlg.h"
#include "../rule/rule.h"

#define NANO_TO_MILL(T) ((T) / 1000000)
#define LOG_FILE_NAME "log.meta"
#define STD_OUTPUT_FILE_NAME "stdout.meta"
#define STD_ERROR_FILE_NAME "stderr.meta"
#define INTERRUPT_CMD "stop"

/* Followings are macros for parsing command line options and arguments */
#define DEFAULT_PATH "."
#define DEFAULT_THREADS 1
#define DEFAULT_VALIDATION false
#define DEFAULT_BEAMWIDTH 5
#define DEFAULT_EVAL_METRIC "τ"
#define DEFAULT_MIN_FACT_COVERAGE 0.05
#define DEFAULT_MIN_COSTANT_COVERAGE 0.25
#define DEFAULT_STOP_COMPRESSION_RATIO 1.0
#define DEFAULT_OBSERVATION_RATIO 2.0
#define DEFAULT_BUDGET_FACTOR 5
#define DEFAULT_WEIGHTED_NEG_SAMPLES true

namespace fs = std::filesystem;

namespace sinc {
    /**
     * The configurations used in SInC.
     *
     * @since 1.0
     */
    class SincConfig {
    public:
        /* I/O configurations */
        /** The path to the directory where the kb is located */
        fs::path basePath;
        /** The name of the KB */
        const char* kbName;
        /** The path where the compressed KB should be stored */
        fs::path dumpPath;
        /** The name of the dumped KB */
        const char* dumpName;
        /** The path where the compressed KB should be stored */

        /* Runtime Config */
        /** The number of threads used to run SInC Todo: Implement multi-thread strategy */
        int threads;
        /** Whether the compressed KB is recovered to check the correctness */
        bool validation;
        /** Take first `maxRelations` relations as targets if this is larger than 0 */
        int maxRelations;

        /* Algorithm Strategy Config */
        /** The beamwidth */
        int beamwidth;
        // bool searchGeneralizations; Todo: Is it possible to efficiently update the cache for the generalizations? If so, implement the option here
        /** The rule evaluation metric */
        EvalMetric::Value evalMetric;
        /** The threshold for fact coverage */
        double minFactCoverage;
        /** The threshold for constant coverage */
        double minConstantCoverage;
        /** The threshold for maximum compression ratio of a single rule */
        double stopCompressionRatio;
        /** The ratio (>=1) that extends the number of rules that are actually specialized according to the estimations */
        double observationRatio;
        /** The base path to the negative sample KB on file system. If non-NULL, negative sampling will be employed in SInC.
         *  If "negKbName" is NULL, adversarial negative sampling will be used and this argument is ineffective. Otherwise,
         *  negative samples are from the loaded negative sample KB. */
        fs::path negKbBasePath;
        /** The name of the negative sample KB. */
        const char* negKbName;
        /** The factor of the number of expected negative samples over the number of positive ones. If < 1, normal sampling
         *  strategy is used (depend on the strategy that generates the negative KB). otherwise, adversarial sampling is used */
        double budgetFactor;
        /** Whether negative samples are weighted differently */
        bool weightedNegSamples;

        SincConfig(
            const char* basePath, const char* kbName, const char* dumpPath, const char* dumpName,
            int const threads, bool const validation, int const maxRelations, int const beamwidth, EvalMetric::Value evalMetric,
            double const minFactCoverage, double const minConstantCoverage, double const stopCompressionRatio,
            double const observationRatio, const char* negKbBasePath, const char* negKbName, double const budgetFactor, bool const weightedNegSamples
        );
        ~SincConfig();
    };

    /**
     * Monitor class that records monitoring information.
     *
     * @since 1.0
     */
    class BaseMonitor : public PerformanceMonitor {
    public:
        /* Time Monitors */
        uint64_t kbLoadTime = 0;
        uint64_t hypothesisMiningTime = 0;
        uint64_t dependencyAnalysisTime = 0;
        uint64_t dumpTime = 0;
        uint64_t validationTime = 0;
        uint64_t neo4jTime = 0;
        uint64_t totalTime = 0;

        /* Basic Rule Mining Time Statistics (measured in nanoseconds) */
        uint64_t fingerprintCreationTime = 0;
        uint64_t pruningTime = 0;
        uint64_t evalTime = 0;
        uint64_t kbUpdateTime = 0;

        /* Mining Statistics Monitors */
        int kbFunctors = 0;
        int kbConstants = 0;
        int kbSize = 0;
        int hypothesisRuleNumber = 0;
        int hypothesisSize = 0;
        int necessaryFacts = 0;
        int counterexamples = 0;
        int supplementaryConstants = 0;
        int sccNumber = 0;
        int sccVertices = 0;
        int fvsVertices = 0;
        /** This member keeps track of the number of evaluated SQL queries */
        int evaluatedSqls = 0;

        /* Memory cost (KB) */
        size_t kbMemCost = 0;
        size_t dependencyGraphMemCost = 0;
        size_t ckbMemCost = 0;

        void show(std::ostream& os) override;
    };

    /**
     * A relation miner is used to induce logic rules that compress a single relation in a KB.
     *
     * @since 2.0
     */
    class RelationMiner {
    public:
        typedef GraphNode<Predicate> nodeType;
        typedef std::unordered_map<Predicate*, nodeType*> nodeMapType;
        typedef std::unordered_map<nodeType*, std::unordered_set<nodeType*>*> depGraphType;

        /**
         * This class is for the axiom node, which automatically releases its content pointer.
         */
        class AxiomNodeType : public nodeType {
        public:
            AxiomNodeType();
            ~AxiomNodeType();
        };

        static AxiomNodeType AxiomNode;

        /* Basic Rule Mining Time Statistics (measured in nanoseconds) */
        uint64_t fingerprintCreationTime = 0;
        uint64_t pruningTime = 0;
        uint64_t evalTime = 0;
        uint64_t kbUpdateTime = 0;
        /** This member keeps track of the number of evaluated SQL queries */
        int evaluatedSqls = 0;

        /**
         * Construct by passing parameters from the compressor that loads the data.
         *
         * @param kb The input KB
         * @param targetRelation The target relation in the KB
         * @param evalMetric The rule evaluation metric
         * @param beamwidth The beamwidth used in the rule mining procedure
         * @param stopCompressionRatio The stopping compression ratio for inducing a single rule
         * @param predicate2NodeMap The mapping from predicates to the nodes in the dependency graph
         * @param dependencyGraph The dependency graph
         * @param hypothesis The hypothesis set
         * @param counterexamples The counterexample set
         * @param logger A logger
         */
        RelationMiner(
            SimpleKb& kb, int const targetRelation, EvalMetric::Value evalMetric, int const beamwidth, double const stopCompressionRatio,
            nodeMapType& predicate2NodeMap, depGraphType& dependencyGraph, std::vector<Rule*>& hypothesis,
            std::unordered_set<Record>& counterexamples, std::ostream& logger
        );

        virtual ~RelationMiner();

        /**
         * Find rules and compress the target relation.
         *
         * @throws KbException When KB operation fails
         */
        virtual void run();

        /**
         * Interrupt mining and discontinue the iteration in `run()`
         */
        void discontinue();

        std::unordered_set<Record>& getCounterexamples() const;
        std::vector<Rule*>& getHypothesis() const;

    protected:
        /** The input KB */
        SimpleKb& kb;
        /** The target relation numeration */
        int const targetRelation;
        /** The rule evaluation metric */
        EvalMetric::Value const evalMetric;
        /** The beamwidth used in the rule mining procedure */
        int const beamwidth;
        /** The stopping compression ratio for inducing a single rule */
        double const stopCompressionRatio;
        /** A mapping from predicates to the nodes in the dependency graph */
        nodeMapType& predicate2NodeMap;
        /** The dependency graph, in the form of an adjacent list */
        depGraphType& dependencyGraph;
        /** The hypothesis set, i.e., a list of rules */
        std::vector<Rule*>& hypothesis;
        /** The set of counterexamples when mining this relation */
        std::unordered_set<Record>& counterexamples;
        /** The tabu set */
        Rule::tabuMapType tabuMap;
        /** Mark whether the mining iteration in `run()` should continue */
        bool shouldContinue = true;

        /** Logger */
        std::ostream& logger;
        StreamFormatter logFormatter;

        /**
         * Create the starting rule at the beginning of a rule mining procedure.
         */
        virtual Rule* getStartRule() = 0;

        /**
         * The rule mining procedure that finds a single rule in the target relation.
         *
         * @return The rule that can be used to compress the target relation. NULL if no proper rule can be found.
         */
        virtual Rule* findRule();

        /**
         * Find the specializations of a base rule. Only the specializations that have a better quality score is added to the
         * candidate list. The candidate list always keeps the best rules.
         *
         * @param rule The basic rule
         * @param candidates The candidate list
         * @return The number of added candidates
         * @throws InterruptedSignal Thrown when the workflow should be interrupted
         */
        int findSpecializations(Rule& rule, Rule** const candidates);

        /**
         * Find the generalizations of a basic rule. Only the specializations that have a better quality score is added to the
         * candidate list. The candidate list always keeps the best rules.
         *
         * @param rule The original rule
         * @param candidates The candidate list
         * @return The number of added candidates
         * @throws InterruptedSignal Thrown when the workflow should be interrupted
         */
        int findGeneralizations(Rule& rule, Rule** const candidates);

        /**
         * Check the status of updated rule and add to a candidate list if the update is successful and the evaluation score
         * of the updated rule is higher than the original one. The candidate list always keeps the best rules.
         *
         * @param updateStatus The update status
         * @param updatedRule The updated rule
         * @param originalRule The original rule
         * @param candidates The candidate list
         * @return 1 if the update is successful and the updated rule is better than the original one; 0 otherwise.
         * @throws InterruptedSignal Thrown when the workflow should be interrupted
         */
        virtual int checkThenAddRule(UpdateStatus updateStatus, Rule* const updatedRule, Rule& originalRule, Rule** candidates);

        /**
         * Select rule r as one of the beams in the next iteration of rule mining. Shared operations for beams may be added
         * in this method, e.g., updating the cache indices.
         */
        virtual void selectAsBeam(Rule* r) = 0;

        /**
         * Find the positive and negative entailments of the rule. Label the positive entailments and add the negative ones
         * to the counterexample set. Evidence for the positive entailments are also needed to update the dependency graph.
         *
         * @return The number of newly entailed records
         */
        int updateKbAndDependencyGraph(Rule& rule);
    };

    /**
     * The recovery class, retuning a compressed KB to the original version.
     * 
     * @since 1.0
     */
    class SincRecovery {
    public:
        /**
         * Decompress the KB to the original form.
         *
         * @param decompressedName The name of the decompressed KB.
         */
        virtual SimpleKb* recover(SimpleCompressedKb& compressedKb, const char* decompressedName) = 0;
    };

    /**
     * The abstract class for SInC. The overall compression procedure is implemented here.
     *
     * @since 1.0
     */
    class SInC {
    public:
        static fs::path getLogFilePath(fs::path& dumpPath, const char* dumpName);
        static fs::path getStdOutFilePath(fs::path& dumpPath, const char* dumpName);
        static fs::path getStdErrFilePath(fs::path& dumpPath, const char* dumpName);

        /**
         * Create a SInC object with configurations.
         *
         * @param config The configurations
         * @throws SincException Dump path creation failure
         */
        SInC(SincConfig* const config);

        /**
         * Create a SInC object with configurations and a KB in memory. If the KB is not NULL, the input KB will be the one
         * in the memory instead of loading from file system.
         * 
         * NOTE: The kb WILL be released by the `SInC` object
         *
         * @param config The configurations
         * @param kb     The KB object in memory
         * @throws SincException Dump path creation failure
         */
        SInC(SincConfig* const config, SimpleKb* const kb);

        virtual ~SInC();

        /**
         * Recover from the compressed KB to verify the correctness of the compression.
         *
         * @return Whether the compressed KB can be recovered to the original one.
         */
        bool recover() const;

        SimpleCompressedKb& getCompressedKb() const;

        /**
         * Run the compression and an interruption daemon.
         */
        void run();

    protected:
        /* Runtime configurations */
        /** SInC configuration */
        SincConfig* config;
        /** The logger */
        std::ostream* logger;
        /** Whether the pointer `logger` should be freed in destructor */
        bool freeLogger;
        /** A redirect stream for `std::cout` */
        std::ofstream* newStdOut = nullptr;
        /** A redirect stream for `std::cerr` */
        std::ofstream* newStdErr = nullptr;
        /** Buffer pointer of `std::cout` */
        std::streambuf* coutBuf;
        /** Buffer pointer of `std::cerr` */
        std::streambuf* cerrBuf;

        /* Compression related data */
        /** The input KB */
        SimpleKb* kb;
        /** The compressed KB */
        SimpleCompressedKb* compressedKb;
        /** A mapping from predicates to the nodes in the dependency graph */
        RelationMiner::nodeMapType predicate2NodeMap;
        /**
         * The dependency graph, in the form of an adjacent list.
         * Note: In the implementation, the set of neighbours in the adjacent list refers to the in-neighbours instead of
         * out-neighbours.
         */
        RelationMiner::depGraphType dependencyGraph;
        /** The performance monitor */
        BaseMonitor monitor;

        /**
         * Load a KB (in the format of Numerated KB)
         */
        virtual void loadKb();

        /**
         * The relations that will be the targets of rule mining procedures. By default, all relations are the targets.
         * Results will be written into the argument references.
         * 
         * This function can be overridden to customize the target list.
         * 
         * NOTE: The pointer `targetRelationIds` SHOULD be maintained by USER
         */
        virtual void getTargetRelations(int* & targetRelationIds, int& numTargets);

        /**
         * Determine the necessary set.
         */
        void dependencyAnalysis();

        virtual SincRecovery* createRecovery() = 0;

        /**
         * Dump the compressed KB
         */
        void dumpCompressedKb();

        virtual void showMonitor();
        virtual RelationMiner* createRelationMiner(int const targetRelationId) = 0;
        void showConfig() const ;
        void showHypothesis() const;
        virtual void finish() = 0;

        /**
         * The compress procedure.
         */
        void compress();

        virtual void finalizeRelationMiner(RelationMiner* miner);
        void logInfo(const char* msg) const;
        void logInfo(std::string const& msg) const;
        void logError(const char* msg) const;
        void logError(std::string const& msg) const;

    private:
        /**
         * This method captures SIGINT and throws an `InterruptionSignal`
         */
        static void sigIntHandler(int signum);
    };
}
