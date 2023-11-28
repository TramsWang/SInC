#include "rule.h"
#include "../util/common.h"
#include <sstream>
#include "../util/util.h"

/**
 * Rule
 */
using sinc::Rule;
using sinc::ParsedPred;
using sinc::ParsedArg;

double Rule::MinFactCoverage = DEFAULT_MIN_FACT_COVERAGE;

std::vector<ParsedPred*>* Rule::parseStructure(const std::string& ruleStr) {
    std::vector<ParsedPred*>* const structure = new std::vector<ParsedPred*>();
    std::unordered_map<std::string, int> variable_2_id_map;
    std::ostringstream os;
    std::string predSymbol;
    bool has_pred_symbol = false;
    std::vector<ParsedArg*> arguments;
    bool has_argument = false;
    for (size_t char_idx = 0; char_idx < ruleStr.length(); char_idx++) {
        const char& c = ruleStr.at(char_idx);
        switch (c) {
            case '(':
                /* Buffer as functor */
                predSymbol = os.str();
                has_pred_symbol = true;
                os.str("");
                break;
            case ')': {
                /* Buffer as argument, finish a predicate */
                arguments.push_back(parseArg(os.str(), variable_2_id_map));
                ParsedPred* predicate = new ParsedPred(predSymbol, arguments.size());
                for (int arg_idx = 0; arg_idx < predicate->getArity(); arg_idx++) {
                    predicate->setArg(arg_idx, arguments[arg_idx]);
                }
                structure->push_back(predicate);
                os.str("");
                has_pred_symbol = false;
                arguments.clear();
                break;
            }
            case ',':
                /* In Predicate: Buffer as argument; Out of predicate: nothing */
                if (has_pred_symbol) {
                    arguments.push_back(parseArg(os.str(), variable_2_id_map));
                    os.str("");
                }
                break;
            case ':':
            case '-':
            case ' ': case '\n': case '\t':
                /* Nothing */
                break;
            default:
                /* Append buffer */
                os << c;
        }
    }

    /* Remove possible named UVs */
    int const total_num_vars = variable_2_id_map.size();
    int lv_cnts[total_num_vars]{0};
    for (ParsedPred* const& predicate: *structure) {
        for (int arg_idx = 0; arg_idx < predicate->getArity(); arg_idx++) {
            ParsedArg* argument = predicate->getArg(arg_idx);
            if (nullptr != argument && argument->isVariable()) {
                lv_cnts[argument->getId()]++;
            }
        }
    }
    int named_uv_cnt = 0;
    for (int vid = 0; vid < total_num_vars; vid++) {
        if (1 == lv_cnts[vid]) {
            /* Remove the UV argument */
            named_uv_cnt++;
            bool found = false;
            for (int pred_idx = 0; pred_idx < structure->size() && !found; pred_idx++) {
                ParsedPred* predicate = (*structure)[pred_idx];
                for (int arg_idx = 0; arg_idx < predicate->getArity(); arg_idx++) {
                    ParsedArg* argument = predicate->getArg(arg_idx);
                    if (nullptr != argument && argument->isVariable() && argument->getId() == vid) {
                        found = true;
                        delete predicate->getArg(arg_idx);
                        predicate->setArg(arg_idx, nullptr);
                        break;
                    }
                }
            }
        }
    }
    for (int named_uv_id = 0; named_uv_id < (total_num_vars - named_uv_cnt); named_uv_id++) {
        /* Use LV ID to replace used UV IDs */
        if (1 == lv_cnts[named_uv_id]) {
            for (int lv_id = total_num_vars - 1; lv_id > named_uv_id; lv_id--) {
                if (1 < lv_cnts[lv_id]) {
                    lv_cnts[lv_id] = 0;
                    for (ParsedPred* const& predicate: *structure) {
                        for (int arg_idx = 0; arg_idx < predicate->getArity(); arg_idx++) {
                            ParsedArg* argument = predicate->getArg(arg_idx);
                            if (nullptr != argument && argument->isVariable() && argument->getId() == lv_id) {
                                delete predicate->getArg(arg_idx);
                                predicate->setArg(arg_idx, ParsedArg::variable(named_uv_id));
                            }
                        }
                    }
                    break;
                }
            }
        }
    }

    return structure;
}

ParsedArg* Rule::parseArg(const std::string& str, std::unordered_map<std::string, int>& variable2IdMap) {
    char const& first_char = str.at(0);
    switch (first_char) {
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': case 'K':
        case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V':
        case 'W': case 'X': case 'Y': case 'Z': {
            /* Parse LV */
            std::pair<std::unordered_map<std::string, int>::iterator, bool> ret = variable2IdMap.emplace(
                str, (int)variable2IdMap.size()
            );
            return ParsedArg::variable(ret.first->second);
        }
        case '?':
            /* Parse UV */
            return nullptr;
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
        case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v':
        case 'w': case 'x': case 'y': case 'z':
            /* Parse Constant */
            return ParsedArg::constant(str);
        default:
            throw RuleParseException("Character not allowed at the beginning of the argument: '" + first_char + '\'');
    }
}

void Rule::releaseParsedStructure(std::vector<ParsedPred*>* parsedStructure) {
    for (ParsedPred* const& pp: *parsedStructure) {
        delete pp;
    }
    delete parsedStructure;
}

std::string Rule::toString(const std::vector<ParsedPred*>& parsedStructure) {
    std::ostringstream os;
    os << parsedStructure[0]->toString() << ":-";
    if (1 < parsedStructure.size()) {
        os << parsedStructure[1]->toString();
        for (int i = 2; i < parsedStructure.size(); i++) {
            os << ',' << parsedStructure[i]->toString();
        }
    }
    return os.str();
}


Rule::Rule(int const headPredSymbol, int const arity, fingerprintCacheType& _fingerprintCache, tabuMapType& _category2TabuSetMap) :
    fingerprintCache(_fingerprintCache), category2TabuSetMap(_category2TabuSetMap), fingerprint(nullptr), releaseFingerprint(false),
    length(MIN_LENGTH), eval(0, 0, 0) // initial `eval` will not be used, this is just for compilation.
{
    structure.emplace_back(headPredSymbol, arity);
    updateFingerprint();
    std::pair<fingerprintCacheType::iterator, bool> ret = fingerprintCache.insert(fingerprint);
    releaseFingerprint = !ret.second;
}

Rule::Rule(const Rule& another) : fingerprintCache(another.fingerprintCache), category2TabuSetMap(another.category2TabuSetMap),
    fingerprint(another.fingerprint), releaseFingerprint(false), length(another.length), eval(another.eval),
    structure(another.structure)
{
    limitedVarArgs.reserve(another.limitedVarArgs.size());
    for (std::vector<ArgLocation>* const& vp: another.limitedVarArgs) {
        limitedVarArgs.push_back(new std::vector<ArgLocation>(*vp));
    }
}

Rule::~Rule() {
    for (std::vector<ArgLocation>* const& vp: limitedVarArgs) {
        delete vp;
    }
    if (releaseFingerprint) {
        delete fingerprint;
    }
}

using sinc::UpdateStatus;
UpdateStatus Rule::specializeCase1(int const predIdx, int const argIdx, int const varId) {
    specCase1UpdateStructure(predIdx, argIdx, varId);

    if (cacheHit()) {
        return UpdateStatus::Duplicated;
    }

    if (isInvalid()) {
        return UpdateStatus::Invalid;
    }

    if (tabuHit()) {
        return UpdateStatus::TabuPruned;
    }

    /* Invoke handler and check coverage */
    UpdateStatus status = specCase1HandlerPrePruning(predIdx, argIdx, varId);
    if (UpdateStatus::Normal == status) {
        if (insufficientCoverage()) {
            return UpdateStatus::InsufficientCoverage;
        }
        status = specCase1HandlerPostPruning(predIdx, argIdx, varId);
        if (UpdateStatus::Normal == status) {
            updateEval();
        }
    }
    return status;
}

UpdateStatus Rule::specializeCase2(int const predSymbol, int const arity, int const argIdx, int const varId) {
    specCase2UpdateStructure(predSymbol, arity, argIdx, varId);

    if (cacheHit()) {
        return UpdateStatus::Duplicated;
    }

    if (isInvalid()) {
        return UpdateStatus::Invalid;
    }

    if (tabuHit()) {
        return UpdateStatus::TabuPruned;
    }

    /* Invoke handler and check coverage */
    UpdateStatus status = specCase2HandlerPrePruning(predSymbol, arity, argIdx, varId);
    if (UpdateStatus::Normal == status) {
        if (insufficientCoverage()) {
            return UpdateStatus::InsufficientCoverage;
        }
        status = specCase2HandlerPostPruning(predSymbol, arity, argIdx, varId);
        if (UpdateStatus::Normal == status) {
            updateEval();
        }
    }
    return status;
}

UpdateStatus Rule::specializeCase3(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) {
    specCase3UpdateStructure(predIdx1, argIdx1, predIdx2, argIdx2);

    if (cacheHit()) {
        return UpdateStatus::Duplicated;
    }

    if (isInvalid()) {
        return UpdateStatus::Invalid;
    }

    if (tabuHit()) {
        return UpdateStatus::TabuPruned;
    }

    /* Invoke handler and check coverage */
    UpdateStatus status = specCase3HandlerPrePruning(predIdx1, argIdx1, predIdx2, argIdx2);
    if (UpdateStatus::Normal == status) {
        if (insufficientCoverage()) {
            return UpdateStatus::InsufficientCoverage;
        }
        status = specCase3HandlerPostPruning(predIdx1, argIdx1, predIdx2, argIdx2);
        if (UpdateStatus::Normal == status) {
            updateEval();
        }
    }
    return status;
}

UpdateStatus Rule::specializeCase4(int const predSymbol, int const arity, int const argIdx1, int const predIdx2, int const argIdx2) {
    specCase4UpdateStructure(predSymbol, arity, argIdx1, predIdx2, argIdx2);

    if (cacheHit()) {
        return UpdateStatus::Duplicated;
    }

    if (isInvalid()) {
        return UpdateStatus::Invalid;
    }

    if (tabuHit()) {
        return UpdateStatus::TabuPruned;
    }

    /* Invoke handler and check coverage */
    UpdateStatus status = specCase4HandlerPrePruning(predSymbol, arity, argIdx1, predIdx2, argIdx2);
    if (UpdateStatus::Normal == status) {
        if (insufficientCoverage()) {
            return UpdateStatus::InsufficientCoverage;
        }
        status = specCase4HandlerPostPruning(predSymbol, arity, argIdx1, predIdx2, argIdx2);
        if (UpdateStatus::Normal == status) {
            updateEval();
        }
    }
    return status;
}

UpdateStatus Rule::specializeCase5(int const predIdx, int const argIdx, int const constant) {
    specCase5UpdateStructure(predIdx, argIdx, constant);

    if (cacheHit()) {
        return UpdateStatus::Duplicated;
    }

    if (isInvalid()) {
        return UpdateStatus::Invalid;
    }

    if (tabuHit()) {
        return UpdateStatus::TabuPruned;
    }

    /* Invoke handler and check coverage */
    UpdateStatus status = specCase5HandlerPrePruning(predIdx, argIdx, constant);
    if (UpdateStatus::Normal == status) {
        if (insufficientCoverage()) {
            return UpdateStatus::InsufficientCoverage;
        }
        status = specCase5HandlerPostPruning(predIdx, argIdx, constant);
        if (UpdateStatus::Normal == status) {
            updateEval();
        }
    }
    return status;
}

UpdateStatus Rule::generalize(int const predIdx, int const argIdx) {
    generalizeUpdateStructure(predIdx, argIdx);

    if (cacheHit()) {
        return UpdateStatus::Duplicated;
    }

    if (isInvalid()) {
        return UpdateStatus::Invalid;
    }

    if (tabuHit()) {
        return UpdateStatus::TabuPruned;
    }

    /* Invoke handler and check coverage */
    UpdateStatus status = generalizeHandlerPrePruning(predIdx, argIdx);
    if (UpdateStatus::Normal == status) {
        if (insufficientCoverage()) {
            return UpdateStatus::InsufficientCoverage;
        }
        status = generalizeHandlerPostPruning(predIdx, argIdx);
        if (UpdateStatus::Normal == status) {
            updateEval();
        }
    }
    return status;
}

using sinc::Predicate;
Predicate const& Rule::getPredicate(int const idx) const {
    return structure[idx];
}

Predicate const& Rule::getHead() const {
    return structure[HEAD_PRED_IDX];
}

int Rule::getLength() const {
    return length;
}

int Rule::usedLimitedVars() const {
    return limitedVarArgs.size();
}

int Rule::numPredicates() const {
    return structure.size();
}

using sinc::Eval;
Eval const& Rule::getEval() const {
    return eval;
}

using sinc::Fingerprint;
Fingerprint const& Rule::getFingerprint() const {
    return *fingerprint;
}

std::string Rule::toString(const char* const names[]) const {
    std::ostringstream os;
    os << '(' << eval.toString() << ')';
    os << getHead().toString(names) << ":-";
    if (1 < numPredicates()) {
        os << structure[1].toString(names);
        for (int pred_idx = 2; pred_idx < numPredicates(); pred_idx++) {
            os << ',' << structure[pred_idx].toString(names);
        }
    }
    return os.str();
}

std::string Rule::toString() const {
    std::ostringstream os;
    os << '(' << eval.toString() << ')';
    os << getHead().toString() << ":-";
    if (1 < numPredicates()) {
        os << structure[1].toString();
        for (int pred_idx = 2; pred_idx < numPredicates(); pred_idx++) {
            os << ',' << structure[pred_idx].toString();
        }
    }
    return os.str();
}

std::string Rule::toDumpString(const char* const names[]) const {
    std::ostringstream os;
    os << getHead().toString(names) << ":-";
    if (1 < numPredicates()) {
        os << structure[1].toString(names);
        for (int pred_idx = 2; pred_idx < numPredicates(); pred_idx++) {
            os << ',' << structure[pred_idx].toString(names);
        }
    }
    return os.str();
}

std::string Rule::toDumpString() const {
    std::ostringstream os;
    os << getHead().toString() << ":-";
    if (1 < numPredicates()) {
        os << structure[1].toString();
        for (int pred_idx = 2; pred_idx < numPredicates(); pred_idx++) {
            os << ',' << structure[pred_idx].toString();
        }
    }
    return os.str();
}

uint64_t Rule::getFingerprintCreationTime() const {
    return fingerprintCreationTime;
}

uint64_t Rule::getPruningTime() const {
    return pruningTime;
}

uint64_t Rule::getEvalTime() const {
    return evalTime;
}

bool Rule::operator==(const Rule &another) const {
    return *fingerprint == another.getFingerprint();
}

size_t Rule::hash() const {
    return fingerprint->hash();
}

size_t Rule::memoryCost() const {
    size_t size = sizeof(Rule) + sizeof(Predicate) * structure.capacity() + sizeof(std::vector<ArgLocation>*) * limitedVarArgs.capacity();
    for (std::vector<ArgLocation>* const& arg_locs: limitedVarArgs) {
        size += sizeof(ArgLocation) * arg_locs->capacity();
    }
    return size;
}

void Rule::updateFingerprint() {
    uint64_t time_start = sinc::currentTimeInNano();
    if (releaseFingerprint) {
        delete fingerprint;
    }
    fingerprint = new Fingerprint(structure);
    fingerprintCreationTime += sinc::currentTimeInNano() - time_start;
}

/* The following version prevent partial duplication with the head */
bool Rule::isInvalid() {
    /* Check independent fragment (may happen when looking for generalizations) */
    /* Use the disjoint set: join limited variables if they are in the same predicate */
    /* Assumption: no body predicate contains only empty or constant argument */
    uint64_t time_start = sinc::currentTimeInNano();
    DisjointSet disjoint_set(usedLimitedVars());
    std::unordered_set<const Predicate*> predicate_set;

    /* Check Head */
    const Predicate& head_pred = getHead();
    std::vector<int> lv_ids;
    lv_ids.reserve(usedLimitedVars());
    bool args_complete = true;
    for (int arg_idx = 0; arg_idx < head_pred.getArity(); arg_idx++) {
        int argument = head_pred.getArg(arg_idx);
        if (ARG_IS_VARIABLE(argument)) {
            lv_ids.push_back(ARG_DECODE(argument));
        }
    }
    if (lv_ids.empty()) {
        if (structure.size() >= 2) {
            /* The body is an independent fragment if the head contains no LV while body is not empty */
            pruningTime += sinc::currentTimeInNano() - time_start;
            return true;
        }
    } else {
        /* Must check here because there may be no LV in the head */
        int first_id = lv_ids[0];
        for (int i = 1; i < lv_ids.size(); i++) {
            disjoint_set.unionSets(first_id, lv_ids[i]);
        }
    }

    /* Check body */
    for (int pred_idx = FIRST_BODY_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
        const Predicate& body_pred = structure[pred_idx];

        /* Check for partial duplication with head */
        if (head_pred.getPredSymbol() == body_pred.getPredSymbol()) {
            for (int arg_idx = 0; arg_idx < head_pred.getArity(); arg_idx++) {
                int head_arg = head_pred.getArg(arg_idx);
                int body_arg = body_pred.getArg(arg_idx);
                if (ARG_IS_NON_EMPTY(head_arg) && head_arg == body_arg) {
                    pruningTime += sinc::currentTimeInNano() - time_start;
                    return true;
                }
            }
        }

        args_complete = true;
        lv_ids.clear();
        for (int arg_idx = 0; arg_idx < body_pred.getArity(); arg_idx++) {
            int argument = body_pred.getArg(arg_idx);
            if (ARG_IS_EMPTY(argument)) {
                args_complete = false;
            } else if (ARG_IS_VARIABLE(argument)) {
                lv_ids.push_back(ARG_DECODE(argument));
            }
        }

        if (args_complete) {
            if (!predicate_set.insert(&body_pred).second) {
                /* Predicate duplicated */
                pruningTime += sinc::currentTimeInNano() - time_start;
                return true;
            }
        }

        /* Join the LVs in the same predicate */
        if (lv_ids.empty()) {
            /* If no LV in body predicate, the predicate is certainly part of the fragment */
            pruningTime += sinc::currentTimeInNano() - time_start;
            return true;
        } else {
            int first_id = lv_ids[0];
            for (int i = 1; i < lv_ids.size(); i++) {
                disjoint_set.unionSets(first_id, lv_ids[i]);
            }
        }
    }

    /* Check for independent fragment */
    pruningTime += sinc::currentTimeInNano() - time_start;
    return 2 <= disjoint_set.totalSets();
}

// /* The following version does not prevent partial duplication with the head */
// bool Rule::isInvalid() {
//     /* Check independent fragment (may happen when looking for generalizations) */
//     /* Use the disjoint set: join limited variables if they are in the same predicate */
//     /* Assumption: no body predicate contains only empty or constant argument */
//     uint64_t time_start = sinc::currentTimeInNano();
//     DisjointSet disjoint_set(usedLimitedVars());
//     std::unordered_set<const Predicate*> predicate_set;

//     /* Check Head */
//     const Predicate& head_pred = getHead();
//     std::vector<int> lv_ids;
//     lv_ids.reserve(usedLimitedVars());
//     bool args_complete = true;
//     for (int arg_idx = 0; arg_idx < head_pred.getArity(); arg_idx++) {
//         int argument = head_pred.getArg(arg_idx);
//         if (ARG_IS_EMPTY(argument)) {
//             args_complete = false;
//         } else if (ARG_IS_VARIABLE(argument)) {
//             lv_ids.push_back(ARG_DECODE(argument));
//         }
//     }
//     if (args_complete) {
//         predicate_set.insert(&head_pred);
//     }
//     if (lv_ids.empty()) {
//         if (structure.size() >= 2) {
//             /* The body is an independent fragment if the head contains no LV while body is not empty */
//             pruningTime += sinc::currentTimeInNano() - time_start;
//             return true;
//         }
//     } else {
//         /* Must check here because there may be no LV in the head */
//         int first_id = lv_ids[0];
//         for (int i = 1; i < lv_ids.size(); i++) {
//             disjoint_set.unionSets(first_id, lv_ids[i]);
//         }
//     }

//     /* Check body */
//     for (int pred_idx = FIRST_BODY_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
//         const Predicate& body_pred = structure[pred_idx];

//         args_complete = true;
//         lv_ids.clear();
//         for (int arg_idx = 0; arg_idx < body_pred.getArity(); arg_idx++) {
//             int argument = body_pred.getArg(arg_idx);
//             if (ARG_IS_EMPTY(argument)) {
//                 args_complete = false;
//             } else if (ARG_IS_VARIABLE(argument)) {
//                 lv_ids.push_back(ARG_DECODE(argument));
//             }
//         }

//         if (args_complete) {
//             if (!predicate_set.insert(&body_pred).second) {
//                 /* Predicate duplicated */
//                 pruningTime += sinc::currentTimeInNano() - time_start;
//                 return true;
//             }
//         }

//         /* Join the LVs in the same predicate */
//         if (lv_ids.empty()) {
//             /* If no LV in body predicate, the predicate is certainly part of the fragment */
//             pruningTime += sinc::currentTimeInNano() - time_start;
//             return true;
//         } else {
//             int first_id = lv_ids[0];
//             for (int i = 1; i < lv_ids.size(); i++) {
//                 disjoint_set.unionSets(first_id, lv_ids[i]);
//             }
//         }
//     }

//     /* Check for independent fragment */
//     pruningTime += sinc::currentTimeInNano() - time_start;
//     return 2 <= disjoint_set.totalSets();
// }

bool Rule::cacheHit() {
    uint64_t time_start = sinc::currentTimeInNano();
    releaseFingerprint = !fingerprintCache.insert(fingerprint).second;
    pruningTime += sinc::currentTimeInNano() - time_start;
    return releaseFingerprint;
}

bool Rule::tabuHit() {
    uint64_t time_start = sinc::currentTimeInNano();
    for (int subset_size = 0; subset_size < structure.size(); subset_size++) {
        std::unordered_set<MultiSet<int>*>* category_subsets = categorySubsets(subset_size);
        for (MultiSet<int>* const& category_subset : *category_subsets) {
            tabuMapType::iterator itr = category2TabuSetMap.find(category_subset);
            if (category2TabuSetMap.end() == itr) {
                continue;
            }
            fingerprintCacheType* tabu_set = itr->second;
            for (const Fingerprint* const& rfp : *tabu_set) {
                if (rfp->generalizationOf(*fingerprint)) {
                    for (MultiSet<int>* const& category_subset : *category_subsets) {
                        delete category_subset;
                    }
                    delete category_subsets;
                    pruningTime += sinc::currentTimeInNano() - time_start;
                    return true;
                }
            }
        }
        for (MultiSet<int>* const& category_subset : *category_subsets) {
            delete category_subset;
        }
        delete category_subsets;
    }
    pruningTime += sinc::currentTimeInNano() - time_start;
    return false;
}

using sinc::MultiSet;
std::unordered_set<MultiSet<int>*>* Rule::categorySubsets(int const subsetSize) const {
    std::unordered_set<MultiSet<int>*>* subsets = new std::unordered_set<MultiSet<int>*>();
    if (0 == subsetSize) {
        subsets->insert(new MultiSet<int>());
    } else {
        int functorTemplate[subsetSize]{0};
        templateSubsetsHandler(subsets, functorTemplate, subsetSize, subsetSize - 1, FIRST_BODY_PRED_IDX);
    }
    return subsets;
}

void Rule::templateSubsetsHandler(
    std::unordered_set<MultiSet<int>*>* const subsets, int* const templateArr, int const arrLength, int const depth, int const startIdx
) const {
    if (0 < depth) {
        for (int pred_idx = startIdx; pred_idx < structure.size(); pred_idx++) {
            templateArr[depth] = structure[pred_idx].getPredSymbol();
            templateSubsetsHandler(subsets, templateArr, arrLength, depth-1, pred_idx+1);
        }
    } else {
        for (int pred_idx = startIdx; pred_idx < structure.size(); pred_idx++) {
            templateArr[depth] = structure[pred_idx].getPredSymbol();
            MultiSet<int>* category = new MultiSet<int>(templateArr, arrLength);
            if (!subsets->insert(category).second) {
                delete category;
            }
        }
    }
}

bool Rule::insufficientCoverage() {
    uint64_t time_start = sinc::currentTimeInNano();
    if (MinFactCoverage >= recordCoverage()) {
        add2TabuMap();
        pruningTime += sinc::currentTimeInNano() - time_start;
        return true;
    }
    pruningTime += sinc::currentTimeInNano() - time_start;
    return false;
}

void Rule::add2TabuMap() {
    MultiSet<int>* functor_mset = new MultiSet<int>();
    for (int pred_idx = FIRST_BODY_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
        functor_mset->add(structure[pred_idx].getPredSymbol());
    }
    tabuMapType::iterator itr = category2TabuSetMap.find(functor_mset);
    if (category2TabuSetMap.end() == itr) {
        /* Put new category */
        fingerprintCacheType* tabu_set = new fingerprintCacheType();
        tabu_set->insert(fingerprint);
        category2TabuSetMap.emplace(functor_mset, tabu_set);
    } else {
        /* Add to existing category */
        itr->second->insert(fingerprint);
        delete functor_mset;
    }
}

void Rule::updateEval() {
    uint64_t time_start = sinc::currentTimeInNano();
    eval = calculateEval();
    evalTime += sinc::currentTimeInNano() - time_start;
}

void Rule::specCase1UpdateStructure(int const predIdx, int const argIdx, int const varId) {
    Predicate& target_predicate = structure[predIdx];
    target_predicate.setArg(argIdx, ARG_VARIABLE(varId));
    limitedVarArgs[varId]->emplace_back(predIdx, argIdx);
    length++;
    updateFingerprint();
}

void Rule::specCase2UpdateStructure(int const predSymbol, int const arity, int const argIdx, int const varId) {
    Predicate& target_predicate = structure.emplace_back(predSymbol, arity);
    target_predicate.setArg(argIdx, ARG_VARIABLE(varId));
    limitedVarArgs[varId]->emplace_back(structure.size() - 1, argIdx);
    length++;
    updateFingerprint();
}

void Rule::specCase3UpdateStructure(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) {
    Predicate& target_predicate1 = structure[predIdx1];
    Predicate& target_predicate2 = structure[predIdx2];
    int new_var = ARG_VARIABLE(limitedVarArgs.size());
    target_predicate1.setArg(argIdx1, new_var);
    target_predicate2.setArg(argIdx2, new_var);
    std::vector<ArgLocation>* var_args = new std::vector<ArgLocation>();
    var_args->emplace_back(predIdx1, argIdx1);
    var_args->emplace_back(predIdx2, argIdx2);
    limitedVarArgs.push_back(var_args);
    length++;
    updateFingerprint();
}

void Rule::specCase4UpdateStructure(int const predSymbol, int const arity, int const argIdx1, int const predIdx2, int const argIdx2) {
    Predicate& target_predicate1 = structure.emplace_back(predSymbol, arity);
    Predicate& target_predicate2 = structure[predIdx2];
    int new_var = ARG_VARIABLE(limitedVarArgs.size());
    target_predicate1.setArg(argIdx1, new_var);
    target_predicate2.setArg(argIdx2, new_var);
    std::vector<ArgLocation>* var_args = new std::vector<ArgLocation>();
    var_args->emplace_back(structure.size() - 1, argIdx1);
    var_args->emplace_back(predIdx2, argIdx2);
    limitedVarArgs.push_back(var_args);
    length++;
    updateFingerprint();
}

void Rule::specCase5UpdateStructure(int const predIdx, int const argIdx, int const constant) {
    Predicate& predicate = structure[predIdx];
    predicate.setArg(argIdx, ARG_CONSTANT(constant));
    length++;
    updateFingerprint();
}

void Rule::generalizeUpdateStructure(int const predIdx, int const argIdx) {
    Predicate& predicate = structure[predIdx];
    int removed_argument = predicate.getArg(argIdx);
    predicate.setArg(argIdx, ARG_EMPTY_VALUE);

    /* Rearrange the var ids if the removed argument is an LV */
    if (ARG_IS_VARIABLE(removed_argument)) {
        int removed_var_id = ARG_DECODE(removed_argument);
        std::vector<ArgLocation>* var_args = limitedVarArgs[removed_var_id];
        if (2 >= var_args->size()) {
            /* The LV should be removed */
            /* Change the id of the latest LV to the removed one */
            /* Note that the removed one may also be the latest LV */
            int latest_var_id = limitedVarArgs.size() - 1;
            limitedVarArgs[removed_var_id] = limitedVarArgs[latest_var_id];
            limitedVarArgs.pop_back();

            /* Clear the argument as well as the UV due to the remove (if applicable) */
            for (ArgLocation const& var_arg: *var_args) {
                structure[var_arg.predIdx].setArg(var_arg.argIdx, ARG_EMPTY_VALUE);
            }
            delete var_args;
            if (removed_var_id != latest_var_id) {
                for (ArgLocation const& var_arg: *(limitedVarArgs[removed_var_id])) {
                    structure[var_arg.predIdx].setArg(var_arg.argIdx, removed_argument);
                }
            }
        } else {
            /* Remove the occurrence only */
            for (std::vector<ArgLocation>::iterator itr = limitedVarArgs[removed_var_id]->begin();
                itr != limitedVarArgs[removed_var_id]->end(); itr++)
            {
                if (predIdx == itr->predIdx && argIdx == itr->argIdx) {
                    limitedVarArgs[removed_var_id]->erase(itr);
                    break;
                }
            }
        }
    }
    length--;

    /* The removal may result in a predicate where all arguments are empty, remove the predicate if it is not the head */
    std::vector<Predicate>::iterator itr = structure.begin();
    itr++;  // Skip the head
    while (structure.end() != itr) {
        Predicate& body_pred = *itr;
        bool is_empty_pred = true;
        for (int arg_idx = 0; arg_idx < itr->getArity(); arg_idx++) {
            int argument = itr->getArg(arg_idx);
            if (ARG_IS_NON_EMPTY(argument)) {
                is_empty_pred = false;
                break;
            }
        }
        if (is_empty_pred) {
            int const removed_idx = itr - structure.begin();
            itr = structure.erase(itr);
            for (std::vector<ArgLocation>* const& arg_locs: limitedVarArgs) {
                for (ArgLocation & arg_loc: *arg_locs) {
                    if (removed_idx < arg_loc.predIdx) {
                        arg_loc.predIdx--;
                    }
                }
            }
        } else {
            itr++;
        }
    }
    updateFingerprint();
}

/**
 * BareRule
 */
using sinc::BareRule;

BareRule::BareRule(int const headPredSymbol, int arity, fingerprintCacheType& _fingerprintCache, tabuMapType& _category2TabuSetMap) :
    Rule(headPredSymbol, arity, _fingerprintCache, _category2TabuSetMap), returningEval(Eval(0, 0, 0)) {}

BareRule::BareRule(const BareRule& another) : Rule(another), returningEval(Eval(another.returningEval)),
    returningCounterexamples(another.returningCounterexamples), returningEvidence(another.returningEvidence) {}

BareRule::~BareRule() {}

BareRule* BareRule::clone() const {
    return new BareRule(*this);
}

using sinc::EvidenceBatch;
EvidenceBatch* BareRule::getEvidenceAndMarkEntailment() {
    return returningEvidence;
}

void BareRule::releaseMemory() {}

using sinc::Record;
std::unordered_set<Record>* BareRule::getCounterexamples() const {
    return returningCounterexamples;
}

Eval const& BareRule::getEval() const {
    return returningEval;
}

double BareRule::recordCoverage() {
    return coverage;
}

Eval BareRule::calculateEval() {
    return returningEval;
}

UpdateStatus BareRule::specCase1HandlerPrePruning(int const predIdx, int const argIdx, int const varId) {
    return case1PrePruningStatus;
}

UpdateStatus BareRule::specCase1HandlerPostPruning(int const predIdx, int const argIdx, int const varId) {
    return case1PostPruningStatus;
}

UpdateStatus BareRule::specCase2HandlerPrePruning(int const predSymbol, int const arity, int const argIdx, int const varId) {
    return case2PrePruningStatus;
}

UpdateStatus BareRule::specCase2HandlerPostPruning(int const predSymbol, int const arity, int const argIdx, int const varId) {
    return case2PostPruningStatus;
}

UpdateStatus BareRule::specCase3HandlerPrePruning(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) {
    return case3PrePruningStatus;
}

UpdateStatus BareRule::specCase3HandlerPostPruning(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) {
    return case3PostPruningStatus;
}

UpdateStatus BareRule::specCase4HandlerPrePruning(int const predSymbol, int const arity, int const argIdx1, int const predIdx2, int const argIdx2) {
    return case4PrePruningStatus;
}

UpdateStatus BareRule::specCase4HandlerPostPruning(int const predSymbol, int const arity, int const argIdx1, int const predIdx2, int const argIdx2) {
    return case4PostPruningStatus;
}

UpdateStatus BareRule::specCase5HandlerPrePruning(int const predIdx, int const argIdx, int const constant) {
    return case5PrePruningStatus;
}

UpdateStatus BareRule::specCase5HandlerPostPruning(int const predIdx, int const argIdx, int const constant) {
    return case5PostPruningStatus;
}

UpdateStatus BareRule::generalizeHandlerPrePruning(int const predIdx, int const argIdx) {
    return generalizationPrePruningStatus;
}

UpdateStatus BareRule::generalizeHandlerPostPruning(int const predIdx, int const argIdx) {
    return generalizationPostPruningStatus;
}
