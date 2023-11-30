#include "sinc.h"

#include <cstring>
#include <stdarg.h>
#include "../util/util.h"
#include <sys/resource.h>
#include <csignal>

/**
 * SincConfig
 */
using sinc::SincConfig;
SincConfig::SincConfig(
    const char* _basePath, const char* _kbName, const char* _dumpPath, const char* _dumpName,
    int const _threads, bool const _validation, int const _maxRelations, int const _beamwidth, EvalMetric::Value _evalMetric,
    double const _minFactCoverage, double const _minConstantCoverage, double const _stopCompressionRatio,
    double const _observationRatio, const char* _negKbBasePath, const char* _negKbName, double const _budgetFactor, bool const _weightedNegSamples
) : basePath(_basePath), kbName(strdup(_kbName)), dumpPath(_dumpPath), dumpName(strdup(_dumpName)),
    threads(_threads), validation(_validation), maxRelations(_maxRelations), beamwidth(_beamwidth), evalMetric(_evalMetric),
    minFactCoverage(_minFactCoverage), minConstantCoverage(_minConstantCoverage), stopCompressionRatio(_stopCompressionRatio),
    observationRatio(_observationRatio), negKbBasePath(_negKbBasePath), negKbName(strdup(_negKbName)), budgetFactor(_budgetFactor), weightedNegSamples(_weightedNegSamples)
{}

SincConfig::~SincConfig() {
    free((void*)kbName);
    free((void*)dumpName);
    free((void*)negKbName);
}

/**
 * Performance Monitor
 */
using sinc::BaseMonitor;
void BaseMonitor::show(std::ostream& os) {
    os << "\n### Monitored Performance Info ###\n\n";
    os << "--- Time Cost ---\n";
    printf(os, "(ms) %10s %10s %10s %10s %10s %10s %10s\n", "Load", "Hypo", "Dep", "Dump", "Validate", "Neo4j", "Total");
    printf(
        os, "     %10d %10d %10d %10d %10d %10d %10d\n\n",
        NANO_TO_MILL(kbLoadTime), NANO_TO_MILL(hypothesisMiningTime), NANO_TO_MILL(dependencyAnalysisTime), NANO_TO_MILL(dumpTime),
        NANO_TO_MILL(validationTime), NANO_TO_MILL(neo4jTime), NANO_TO_MILL(totalTime)
    );

    os << "--- Basic Rule Mining Cost ---\n";
    printf(os, "(ms) %10s %10s %10s %10s %10s\n", "Fp", "Prune", "Eval", "KB Upd", "Total");
    printf(
        os, "     %10d %10d %10d %10d %10d\n\n",
        NANO_TO_MILL(fingerprintCreationTime), NANO_TO_MILL(pruningTime), NANO_TO_MILL(evalTime), NANO_TO_MILL(kbUpdateTime),
        NANO_TO_MILL(fingerprintCreationTime + pruningTime + evalTime + kbUpdateTime)
    );

    rusage usage;
    if (0 != getrusage(RUSAGE_SELF, &usage)) {
        std::cerr << "Failed to get `rusage`" << std::endl;
        usage.ru_maxrss = 1024 * 1024 * 1024;   // set to 1T
    }
    os << "--- Basic Memory Cost ---\n";
    printf(os, "%10s %10s %10s %10s %10s %10s %10s\n", "KB", "KB(%)", "Dep.G", "Dep.G(%)", "CKB", "CKB(%)", "Peak");
    printf(
        os, "%10s %10.2f %10s %10.2f %10s %10.2f %10s\n\n",
        formatMemorySize(kbMemCost).c_str(), ((double) kbMemCost) / usage.ru_maxrss * 100.0,
        formatMemorySize(dependencyGraphMemCost).c_str(), ((double) dependencyGraphMemCost) / usage.ru_maxrss * 100.0,
        formatMemorySize(ckbMemCost).c_str(), ((double) ckbMemCost) / usage.ru_maxrss * 100.0,
        formatMemorySize(usage.ru_maxrss).c_str()
    );

    os << "--- Statistics ---\n";
    printf(
        os, "# %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s\n",
        "|P|", "|Σ|", "|B|", "|H|", "||H||", "|N|", "|A|", "|ΔΣ|", "#SCC", "|SCC|", "|FVS|", "Comp(%)", "#SQL", "#SQL/|H|"
    );
    printf(
        os, "  %10d %10d %10d %10d %10d %10d %10d %10d %10d %10d %10d %10.2f %10d %10.2f\n\n",
        kbFunctors, kbConstants, kbSize, hypothesisRuleNumber, hypothesisSize, necessaryFacts, counterexamples, supplementaryConstants,
        sccNumber, sccVertices, fvsVertices, (necessaryFacts + counterexamples + hypothesisSize) * 100.0 / kbSize,
        evaluatedSqls, evaluatedSqls * 1.0 / hypothesisRuleNumber
    );
}

/**
 * RelationMiner
 */
using sinc::RelationMiner;

RelationMiner::AxiomNodeType::AxiomNodeType() : nodeType(new sinc::Predicate(-1, 1)) {}

RelationMiner::AxiomNodeType::~AxiomNodeType() {
    delete content;
}

RelationMiner::AxiomNodeType RelationMiner::AxiomNode;

RelationMiner::RelationMiner(
    SimpleKb& _kb, int const _targetRelation, EvalMetric::Value _evalMetric, int const _beamwidth, double const _stopCompressionRatio,
    nodeMapType& _predicate2NodeMap, depGraphType& _dependencyGraph, std::vector<Rule*>& _hypothesis,
    std::unordered_set<Record>& _counterexamples, std::ostream& _logger
) : kb(_kb), targetRelation(_targetRelation), evalMetric(_evalMetric), beamwidth(_beamwidth), stopCompressionRatio(_stopCompressionRatio),
    predicate2NodeMap(_predicate2NodeMap), dependencyGraph(_dependencyGraph), hypothesis(_hypothesis), counterexamples(_counterexamples),
    logger(_logger), logFormatter(_logger) {}

RelationMiner::~RelationMiner() {
    /* Release resources in tabu map */
    for (std::pair<const sinc::MultiSet<int> *, sinc::Rule::fingerprintCacheType*> const& kv: tabuMap) {
        delete kv.first;
        delete kv.second;
    }
}

void RelationMiner::run() {
    Rule* rule;
    int covered_facts = 0;
    int const total_facts = kb.getRelation(targetRelation)->getTotalRows();
    BaseMonitor monitor; // use its formatter method
    while (shouldContinue && (covered_facts < total_facts) && (nullptr != (rule = findRule()))) {
        hypothesis.push_back(rule);
        covered_facts += updateKbAndDependencyGraph(*rule);
        rule->releaseMemory();
        logFormatter.printf(
                "Found (Coverage: %.2f%%, %d/%d): %s\n", covered_facts * 100.0 / total_facts, covered_facts, total_facts,
                rule->toDumpString(kb.getRelationNames()).c_str()
        );
        logger.flush();

        /* Log memory usage */
        rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        logger << "Max Mem:" << monitor.formatMemorySize(usage.ru_maxrss) << std::endl;
    }
    logger << "Done" << std::endl;
}

void RelationMiner::discontinue() {
    shouldContinue = false;
}

using sinc::Record;
std::unordered_set<Record>& RelationMiner::getCounterexamples() const {
    return counterexamples;
}

using sinc::Rule;
std::vector<Rule*>& RelationMiner::getHypothesis() const {
    return hypothesis;
}

Rule* RelationMiner::findRule() {
    /* Create the beams */
    Rule** beams = new Rule*[beamwidth]{};
    beams[0] = getStartRule();
    Rule* best_local_optimum = nullptr;

    /* Find a local optimum (there is certainly a local optimum in the search routine) */
    while (true) {
        /* Find the candidates in the next round according to current beams */
        Rule** top_candidates = new Rule*[beamwidth]{};
        for (int i = 0; i < beamwidth && nullptr != beams[i]; i++) {
            Rule* const r = beams[i];
            selectAsBeam(r);
            logFormatter.printf("Extend: %s\n", r->toString(kb.getRelationNames()).c_str());
            logger.flush();

            /* Find the specializations and generalizations of rule 'r' */
            int specializations_cnt = findSpecializations(*r, top_candidates);
            int generalizations_cnt = findGeneralizations(*r, top_candidates);
            if (0 == specializations_cnt && 0 == generalizations_cnt) {
                /* If no better specialized and generalized rules, 'r' is a local optimum */
                /* Keep track of only the best local optimum */
                if (nullptr == best_local_optimum ||
                        best_local_optimum->getEval().value(evalMetric) < r->getEval().value(evalMetric)) {
                    best_local_optimum = r;
                }
            }
        }
        if (!shouldContinue) {
            /* Stop the finding procedure at the current stage and return the best rule */
            Rule* best_rule = beams[0];
            for (int i = 1; i < beamwidth && nullptr != beams[i]; i++) {
                if (best_rule->getEval().value(evalMetric) < beams[i]->getEval().value(evalMetric)) {
                    delete best_rule;
                    best_rule = beams[i];
                } else {
                    delete beams[i];
                }
            }
            for (int i = 0; i < beamwidth && nullptr != top_candidates[i]; i++) {
                if (best_rule->getEval().value(evalMetric) < top_candidates[i]->getEval().value(evalMetric)) {
                    delete best_rule;
                    best_rule = top_candidates[i];
                } else {
                    delete top_candidates[i];
                }
            }
            if (nullptr != best_rule) {
                if (!best_rule->getEval().useful()) {
                    delete best_rule;
                    best_rule = nullptr;
                }
            }
            return best_rule;
        }

        /* Find the best candidate */
        Rule* best_candidate = nullptr;
        if (nullptr != top_candidates[0]) {
            best_candidate = top_candidates[0];
            for (int i = 1; i < beamwidth && nullptr != top_candidates[i]; i++) {
                if (best_candidate->getEval().value(evalMetric) < top_candidates[i]->getEval().value(evalMetric)) {
                    best_candidate = top_candidates[i];
                }
            }
        }

        /* If there is a local optimum and it is the best among all, return the rule */
        if (nullptr != best_local_optimum &&
                (nullptr == best_candidate ||
                        best_local_optimum->getEval().value(evalMetric) > best_candidate->getEval().value(evalMetric))
        ) {
            /* If the best is not useful, return NULL */
            Rule* const ret = best_local_optimum->getEval().useful() ? best_local_optimum : nullptr;
            for (int i = 0; i < beamwidth && nullptr != beams[i]; i++) {
                if (ret != beams[i]) {
                    delete beams[i];
                }
            }
            delete[] beams;
            for (int i = 0; i < beamwidth && nullptr != top_candidates[i]; i++) {
                delete top_candidates[i];
            }
            delete[] top_candidates;
            return ret;
        }

        /* If the best rule reaches the stopping threshold, return the rule */
        /* The "best_candidate" is certainly not NULL if the workflow goes here */
        /* Assumption: the stopping threshold is no less than the threshold of usefulness */
        const Eval& best_eval = best_candidate->getEval();
        if (stopCompressionRatio <= best_eval.value(EvalMetric::Value::CompressionRatio) || 0 == best_eval.getNegEtls()) {
            Rule* const ret = best_eval.useful() ? best_candidate : nullptr;
            for (int i = 0; i < beamwidth && nullptr != beams[i]; i++) {
                delete beams[i];
            }
            delete[] beams;
            for (int i = 0; i < beamwidth && nullptr != top_candidates[i]; i++) {
                if (ret != top_candidates[i]) {
                    delete top_candidates[i];
                }
            }
            delete[] top_candidates;
            return ret;
        }

        /* Update the beams */
        for (int i = 0; i < beamwidth && nullptr != beams[i]; i++) {
            delete beams[i];
        }
        delete[] beams;
        best_local_optimum = nullptr;
        beams = top_candidates;
    }
}

int RelationMiner::findSpecializations(Rule& rule, Rule** const candidates) {
    int added_candidate_cnt = 0;

    /* Find all empty arguments */
    std::vector<ArgLocation> empty_args;
    for (int pred_idx = HEAD_PRED_IDX; pred_idx < rule.numPredicates(); pred_idx++) {
        const Predicate& predicate = rule.getPredicate(pred_idx);
        for (int arg_idx = 0; arg_idx < predicate.getArity(); arg_idx++) {
            if (ARG_IS_EMPTY(predicate.getArg(arg_idx))) {
                empty_args.emplace_back(pred_idx, arg_idx);
            }
        }
    }

    /* Add existing LVs (case 1 and 2) */
    std::vector<SimpleRelation*>* const relations = kb.getRelations();
    for (int var_id = 0; var_id < rule.usedLimitedVars(); var_id++) {
        /* Case 1 */
        for (ArgLocation const& vacant: empty_args) {
            Rule* const new_rule = rule.clone();
            UpdateStatus const update_status = new_rule->specializeCase1(vacant.predIdx, vacant.argIdx, var_id);
            added_candidate_cnt += checkThenAddRule(update_status, new_rule, rule, candidates);
        }

        /* Case 2 */
        for (SimpleRelation* const& relation: *relations) {
            for (int arg_idx = 0; arg_idx < relation->getTotalCols(); arg_idx++) {
                Rule* const new_rule = rule.clone();
                UpdateStatus const update_status = new_rule->specializeCase2(
                        relation->id, relation->getTotalCols(), arg_idx, var_id
                );
                added_candidate_cnt += checkThenAddRule(update_status, new_rule, rule, candidates);
            }
        }
    }

    /* Case 3, 4, and 5 */
    for (int i = 0; i < empty_args.size(); i++) {
        /* Find the first empty argument */
        ArgLocation const& empty_arg_loc_1 = empty_args[i];
        Predicate const& predicate1 = rule.getPredicate(empty_arg_loc_1.predIdx);

        /* Case 5 */
        std::vector<int>* const_list = kb.getPromisingConstants(predicate1.getPredSymbol())[empty_arg_loc_1.argIdx];
        for (int const& constant: *const_list) {
            Rule* const new_rule = rule.clone();
            UpdateStatus const update_status = new_rule->specializeCase5(
                    empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx, constant
            );
            added_candidate_cnt += checkThenAddRule(update_status, new_rule, rule, candidates);
        }

        /* Case 3 */
        for (int j = i + 1; j < empty_args.size(); j++) {
            /* Find another empty argument */
            ArgLocation const& empty_arg_loc_2 = empty_args[j];
            Rule* const new_rule = rule.clone();
            UpdateStatus update_status = new_rule->specializeCase3(
                    empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx, empty_arg_loc_2.predIdx, empty_arg_loc_2.argIdx
            );
            added_candidate_cnt += checkThenAddRule(update_status, new_rule, rule, candidates);
        }

        /* Case 4 */
        for (SimpleRelation* const& relation: *relations) {
            for (int arg_idx = 0; arg_idx < relation->getTotalCols(); arg_idx++) {
                Rule* const new_rule = rule.clone();
                UpdateStatus const update_status = new_rule->specializeCase4(
                        relation->id, relation->getTotalCols(), arg_idx, empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx
                );
                added_candidate_cnt += checkThenAddRule(update_status, new_rule, rule, candidates);
            }
        }
    }
    return added_candidate_cnt;
}

int RelationMiner::findGeneralizations(Rule& rule, Rule** const candidates) {
    int added_candidate_cnt = 0;
    for (int pred_idx = HEAD_PRED_IDX; pred_idx < rule.numPredicates(); pred_idx++) {
        /* Independent fragment may appear in a generalized rule, but this will be found by checking rule validness */
        Predicate const& predicate = rule.getPredicate(pred_idx);
        for (int arg_idx = 0; arg_idx < predicate.getArity(); arg_idx++) {
            if (ARG_IS_NON_EMPTY(predicate.getArg(arg_idx))) {
                Rule* const new_rule = rule.clone();
                UpdateStatus const update_status = new_rule->generalize(pred_idx, arg_idx);
                added_candidate_cnt += checkThenAddRule(update_status, new_rule, rule, candidates);
            }
        }
    }
    return added_candidate_cnt;
}

int RelationMiner::checkThenAddRule(UpdateStatus updateStatus, Rule* const updatedRule, Rule& originalRule, Rule** candidates) {
    fingerprintCreationTime += updatedRule->getFingerprintCreationTime();
    pruningTime += updatedRule->getPruningTime();

    bool updated_is_better = false;
    switch (updateStatus) {
        case Normal:
            #if DEBUG_LEVEL >= DEBUG_DEBUG
                logFormatter.printf("Spec. %s\n", updatedRule->toString(kb.getRelationNames()).c_str());
                logger.flush();
            #endif
            evalTime += updatedRule->getEvalTime();
            evaluatedSqls += 2;
            if (updatedRule->getEval().value(evalMetric) > originalRule.getEval().value(evalMetric)) {
                updated_is_better = true;
                int replace_idx = -1;
                double replaced_score = updatedRule->getEval().value(evalMetric);
                for (int i = 0; i < beamwidth; i++) {
                    if (nullptr == candidates[i]) {
                        replace_idx = i;
                        break;
                    }
                    double candidate_socre = candidates[i]->getEval().value(evalMetric);
                    if (replaced_score > candidate_socre) {
                        replace_idx = i;
                        replaced_score = candidate_socre;
                    }
                }
                if (0 <= replace_idx) {
                    if (nullptr != candidates[replace_idx]) {
                        delete candidates[replace_idx];
                    }
                    candidates[replace_idx] = updatedRule;
                } else {
                    delete updatedRule;
                }
            // } else {
            //     delete updatedRule;
            }
            break;
        case Invalid:
        case Duplicated:
        case InsufficientCoverage:
            // delete updatedRule;
            break;
        case TabuPruned:
            #if DEBUG_LEVEL >= DEBUG_DEBUG
                logger.printf("TABU: %s\n", updatedRule->toString(kb.getRelationNames()));
                logger.flush();
            #endif
            evaluatedSqls++;
            // delete updatedRule;
            break;
        default:
            // delete updatedRule;
            std::ostringstream os;
            os << "Unknown Update Status of Rule: " << updateStatus;
            throw SincException(os.str());
    }
    if (updated_is_better) {
        return 1;
    } else {
        delete updatedRule;
        return 0;
    }
    // return updated_is_better ? 1 : 0;
}

int RelationMiner::updateKbAndDependencyGraph(Rule& rule) {
    uint64_t time_start = currentTimeInNano();
    std::unordered_set<Record>* counterexample_by_rule = rule.getCounterexamples();
    for (Record const& r: *counterexample_by_rule) {
        counterexamples.emplace(r.getArgs(), r.getArity());
    }
    delete counterexample_by_rule;
    EvidenceBatch* evidence_batch = rule.getEvidenceAndMarkEntailment();
    for (int** const& grounding: evidence_batch->evidenceList) {
        Predicate const& head_pred = rule.getHead();
        Predicate* head_ground = new Predicate(
                evidence_batch->predicateSymbolsInRule[HEAD_PRED_IDX], grounding[HEAD_PRED_IDX], evidence_batch->aritiesInRule[HEAD_PRED_IDX]
        );
        // head_ground->maintainArgs();

        nodeType* head_node = new nodeType(head_ground);
        std::pair<nodeMapType::iterator, bool> ret = predicate2NodeMap.emplace(head_ground, head_node);
        if (!ret.second) {
            delete head_ground;
            delete head_node;
            head_ground = ret.first->first;
            head_node = ret.first->second;
        }

        depGraphType::iterator itr = dependencyGraph.find(head_node);
        std::unordered_set<nodeType*>* dependencies;
        if (dependencyGraph.end() == itr) {
            dependencies = new std::unordered_set<nodeType*>();
            dependencyGraph.emplace(head_node, dependencies);
        } else {
            dependencies = itr->second;
        }
        if (1 >= rule.numPredicates()) {
            /* dependency is the "⊥" node */
            dependencies->insert(&RelationMiner::AxiomNode);
        } else {
            for (int pred_idx = FIRST_BODY_PRED_IDX; pred_idx < rule.numPredicates(); pred_idx++) {
                Predicate* body_ground = new Predicate(
                        evidence_batch->predicateSymbolsInRule[pred_idx], grounding[pred_idx], evidence_batch->aritiesInRule[pred_idx]
                );
                nodeType* body_node = new nodeType(body_ground);
                ret = predicate2NodeMap.emplace(body_ground, body_node);
                if (!ret.second) {
                    delete body_ground;
                    delete body_node;
                    body_ground = ret.first->first;
                    body_node = ret.first->second;
                }
                dependencies->insert(body_node);
            }
        }
    }
    kbUpdateTime += currentTimeInNano() - time_start;
    int num_entailed = evidence_batch->evidenceList.size();
    delete evidence_batch;
    return num_entailed;
}

/**
 * SInC
 */
using sinc::SInC;
namespace fs = std::filesystem;

fs::path SInC::getLogFilePath(fs::path& dumpPath, const char* dumpName) {
    return dumpPath / dumpName / LOG_FILE_NAME;
}

fs::path SInC::getStdOutFilePath(fs::path& dumpPath, const char* dumpName) {
    return dumpPath / dumpName / STD_OUTPUT_FILE_NAME;
}

fs::path SInC::getStdErrFilePath(fs::path& dumpPath, const char* dumpName) {
    return dumpPath / dumpName / STD_ERROR_FILE_NAME;
}

SInC::SInC(SincConfig* const _config) : SInC(_config, nullptr) {}

SInC::SInC(SincConfig* const _config, SimpleKb* const _kb) : 
    config(_config),
    kb(_kb),
    compressedKb(nullptr) 
{
    /* Create writer objects to log and std output files */
    fs::path dump_kb_dir = config->dumpPath / config->dumpName;
    if (!fs::exists(dump_kb_dir) && !fs::create_directories(dump_kb_dir)) {
        throw SincException("Dump path creation failed.");
    }
    try {
        logger = new std::ofstream(getLogFilePath(config->dumpPath, config->dumpName), std::ios::out);
        freeLogger = true;
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        std::cout << "Logger use `std::cout` instead" << std::endl;
        logger = &std::cout;
        freeLogger = false;
    }

    Rule::MinFactCoverage = config->minFactCoverage;
    SimpleRelation::minConstantCoverage = config->minConstantCoverage;
}

SInC::~SInC() {
    if (freeLogger) {
        delete logger;
    }
    if (nullptr != newStdOut) {
        delete newStdOut;
    }
    std::cout.rdbuf(coutBuf);
    if (nullptr != newStdErr) {
        delete newStdErr;
    }
    std::cerr.rdbuf(cerrBuf);
    if (nullptr != compressedKb) {
        delete compressedKb;
    }
    if (nullptr != kb) {
        delete kb;
    }
    delete config;
    for (std::pair<const Predicate*, GraphNode<Predicate>*> const& kv: predicate2NodeMap) {
        delete kv.first;
        delete kv.second;
    }
    for (std::pair<const RelationMiner::nodeType*, std::unordered_set<RelationMiner::nodeType*>*> const& kv: dependencyGraph) {
        delete kv.second;
    }
}

bool SInC::recover() const {
    return false;  // Todo: Re-implement here
}

sinc::SimpleCompressedKb& SInC::getCompressedKb() const {
    return *compressedKb;
}

void SInC::run() {
    /* Set out/err stream to output files */
    coutBuf = std::cout.rdbuf();
    try {
        newStdOut = new std::ofstream(getStdOutFilePath(config->dumpPath, config->dumpName), std::ios::out);
        std::cout.rdbuf(newStdOut->rdbuf());
    } catch (const std::exception& e) {
        std::cout.rdbuf(coutBuf);
        std::cout << "`std:out` redirection failed" << std::endl;
    }

    cerrBuf = std::cerr.rdbuf();
    try {
        newStdErr = new std::ofstream(getStdErrFilePath(config->dumpPath, config->dumpName), std::ios::out);
        std::cerr.rdbuf(newStdErr->rdbuf());
    } catch (const std::exception& e) {
        std::cerr.rdbuf(cerrBuf);
        std::cerr << "`std:err` redirection failed" << std::endl;
    }

    compress(); // Todo: listen to interrupt signal in another thread
}

void SInC::loadKb() {
    if (nullptr == kb) {
        kb = new SimpleKb(config->kbName, config->basePath);
    }
    kb->updatePromisingConstants();
}

void SInC::getTargetRelations(int* & targetRelationIds, int& numTargets) {
    /* Relation IDs in SimpleKb are from 0 to n-1, where n is the number of relations */
    if (0 < config->maxRelations && kb->totalRelations() >= config->maxRelations) {
        numTargets = config->maxRelations;
    } else {
        numTargets = kb->totalRelations();
    }
    targetRelationIds = new int[numTargets];
    for (int i = 0; i < numTargets; i++) {
        targetRelationIds[i] = i;
    }
}

void SInC::dependencyAnalysis() {
    /* The KB has already been updated by the relation miners. All records that are not entailed have already been
        * flagged in the relations. Here we only need to find the nodes in the MFVS solution */
    /* Find all SCCs */
    Tarjan<Predicate> tarjan(&dependencyGraph, false);
    Tarjan<Predicate>::resultType const& sccs = tarjan.run();
    for (Tarjan<Predicate>::nodeSetType* const& scc: sccs) {
        /* Find a solution of MFVS and add to "necessaries" */
        FeedbackVertexSetSolver<Predicate> fvs_solver(dependencyGraph, *scc);
        FeedbackVertexSetSolver<Predicate>::nodeSetType* fvs = fvs_solver.run();
        for (GraphNode<Predicate>* const& node: *fvs) {
            compressedKb->addFvsRecord(node->content->getPredSymbol(), node->content->getArgs());
        }
        monitor.sccVertices += scc->size();
        monitor.fvsVertices += fvs->size();
        delete fvs;
    }
    monitor.sccNumber = sccs.size();
}

void SInC::dumpCompressedKb() {
    compressedKb->dump(config->dumpPath);
}

void SInC::showMonitor() {
    /* Calculate memory cost */
    monitor.kbMemCost = kb->memoryCost() / 1024;
    monitor.ckbMemCost = compressedKb->memoryCost() / 1024;
    monitor.dependencyGraphMemCost += sizeOfUnorderedMap(
        predicate2NodeMap.bucket_count(), predicate2NodeMap.max_load_factor(), sizeof(std::pair<Predicate*, RelationMiner::nodeType*>),
        sizeof(predicate2NodeMap)
    ) + (sizeof(Predicate) + sizeof(RelationMiner::nodeType)) * predicate2NodeMap.size();   // `predicate2NodeMap`
    monitor.dependencyGraphMemCost += sizeOfUnorderedMap(
        dependencyGraph.bucket_count(), dependencyGraph.max_load_factor(),
        sizeof(std::pair<RelationMiner::nodeType*, std::unordered_set<RelationMiner::nodeType*>*>), sizeof(dependencyGraph)
    );
    for (std::pair<RelationMiner::nodeType*, std::unordered_set<RelationMiner::nodeType*>*> const& kv: dependencyGraph) {
        std::unordered_set<RelationMiner::nodeType*> const& set = *(kv.second);
        monitor.dependencyGraphMemCost += sizeOfUnorderedSet(
            set.bucket_count(), set.max_load_factor(), sizeof(RelationMiner::nodeType*), sizeof(set)
        );
    }
    monitor.dependencyGraphMemCost /= 1024;

    monitor.show(*logger);
}

void SInC::showConfig() const {
    (*logger) << "Base Path:\t" << config->basePath << '\n';
    (*logger) << "KB Name:\t" << config->kbName << '\n';
    (*logger) << "Dump Path:\t" << config->dumpPath << '\n';
    (*logger) << "Dump Name:\t" << config->dumpName << '\n';
    (*logger) << "Beamwidth:\t" << config->beamwidth << '\n';
    (*logger) << "Threads:\t" << config->threads << '\n';
    (*logger) << "Eval Metric:\t" << config->evalMetric << '\n';
    (*logger) << "Min Fact Coverage:\t" << config->minFactCoverage << '\n';
    (*logger) << "Min Constant Coverage:\t" << config->minConstantCoverage << '\n';
    (*logger) << "Stop Compression Ratio:\t" << config->stopCompressionRatio << '\n';
    (*logger) << "Observation Ratio:\t" << config->observationRatio << '\n';
    (*logger) << "Validation:\t" << config->validation << "\n\n'";
}

void SInC::showHypothesis() const {
    logInfo("\n### Hypothesis Found ###");
    for (Rule* const& rule : compressedKb->getHypothesis()) {
        logInfo(rule->toString(kb->getRelationNames()));
    }
    (*logger) << '\n';
}

void SInC::sigIntHandler(int signum) {
    std::cout << "\n<<< Interrupted >>>" << std::endl;
    throw InterruptionSignal("Interrupted by signal");
}

void SInC::compress() {
    /* Register signal handler */
    signal(SIGINT, SInC::sigIntHandler);

    showConfig();

    /* Load KB */
    uint64_t time_start = currentTimeInNano();
    try {
        loadKb();
    } catch (std::exception const& e) {
        std::cerr << e.what() << std::endl;
        logError("KB load failed, abort.");
        finish();
        return;
    }
    monitor.kbSize = kb->totalRecords();
    monitor.kbFunctors = kb->totalRelations();
    monitor.kbConstants = kb->totalConstants();
    compressedKb = new SimpleCompressedKb(config->dumpName, kb);
    uint64_t time_kb_loaded = currentTimeInNano();
    monitor.kbLoadTime = time_kb_loaded - time_start;

    /* Run relation miners on each relation */
    int* target_relations;
    int num_targets;
    RelationMiner* relation_miner;
    try {
        getTargetRelations(target_relations, num_targets);
        for (int i = 0; i < num_targets; i++) {
            int relation_num = target_relations[i];
            relation_miner = createRelationMiner(relation_num);
            relation_miner->run();  // counterexamples and hypothesis should be added to `compressedKb` in this procedure
            // compressedKb->addCounterexamples(relation_num, relation_miner->getCounterexamples());
            // compressedKb->addHypothesisRules(relation_miner.getHypothesis());
            (*logger) << "Relation mining done (" << i+1 << '/' << num_targets << "): " << kb->getRelation(relation_num)->name << '\n';
            finalizeRelationMiner(relation_miner);
            delete relation_miner;
            relation_miner = nullptr;
        }
    } catch (std::exception const& e) {
        logError("Relation Miner failed. Interrupt");
        (*logger) << e.what() << std::endl;
        if (nullptr != relation_miner) {
            relation_miner->discontinue();
        }
    }
    delete[] target_relations;
    uint64_t time_hypothesis_found = currentTimeInNano();
    monitor.hypothesisMiningTime = time_hypothesis_found - time_kb_loaded;

    /* Dependency analysis */
    bool abort = false;
    try {
        dependencyAnalysis();
    } catch (std::exception e) {
        std::cerr << e.what() << std::endl;
        logError("Dependency Analysis failed. Abort.");
        abort = true;
    }

    monitor.necessaryFacts = compressedKb->totalNecessaryRecords();
    monitor.counterexamples = compressedKb->totalCounterexamples();
    monitor.supplementaryConstants = compressedKb->totalSupplementaryConstants();
    uint64_t time_dependency_resolved = currentTimeInNano();
    monitor.dependencyAnalysisTime = time_dependency_resolved - time_hypothesis_found;
    showHypothesis();

    if (abort) {
        monitor.totalTime = time_dependency_resolved - time_start;
        showMonitor();
        finish();
        return;
    }

    /* Dump the compressed KB */
    try {
        dumpCompressedKb();
    } catch (std::exception e) {
        std::cerr << e.what() << std::endl;
        logError("Compressed KB dump failed. Abort.");
        showMonitor();
        finish();
        return;
    }

    uint64_t time_kb_dumped = currentTimeInNano();
    monitor.dumpTime = time_kb_dumped - time_dependency_resolved;

    /* 检查结果 */
    if (config->validation) {
        if (!recover()) {
            logError("Validation Failed");
        }
    }
    uint64_t time_validated = currentTimeInNano();
    monitor.validationTime = time_validated - time_kb_dumped;

    monitor.totalTime = time_validated - time_start;
    showMonitor();
    finish();
}

void SInC::finalizeRelationMiner(RelationMiner* miner) {
    monitor.hypothesisSize = 0;
    for (Rule* const& r: miner->getHypothesis()) {
        monitor.hypothesisSize += r->getLength();
    }
    monitor.hypothesisRuleNumber = miner->getHypothesis().size();
    monitor.evaluatedSqls += miner->evaluatedSqls;
    monitor.fingerprintCreationTime += miner->fingerprintCreationTime;
    monitor.pruningTime += miner->pruningTime;
    monitor.evalTime += miner->evalTime;
    monitor.kbUpdateTime += miner->kbUpdateTime;
}

void SInC::logInfo(const char* msg) const {
    (*logger) << msg << '\n';
}

void SInC::logInfo(std::string const& msg) const {
    (*logger) << msg << '\n';
}

void SInC::logError(const char* msg) const {
    (*logger) << "[ERROR]" << msg << '\n';
}

void SInC::logError(std::string const& msg) const {
    (*logger) << "[ERROR]" << msg << '\n';
}
