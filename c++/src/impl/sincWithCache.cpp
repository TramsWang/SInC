#include "sincWithCache.h"
#include <stdarg.h>
#include <cmath>
#include <algorithm>
#include <sys/resource.h>

/**
 * CompliedBlock
 */
using sinc::CompliedBlock;

std::vector<CompliedBlock*> CompliedBlock::pool;

CompliedBlock* CompliedBlock::create(int** _complianceSet, int const _totalRows, int const _totalCols, bool _maintainComplianceSet) {
    CompliedBlock* cb = new CompliedBlock(_complianceSet, _totalRows, _totalCols, _maintainComplianceSet);
    registerCb(cb);
    return cb;
}

void CompliedBlock::registerCb(CompliedBlock* cb) {
    pool.push_back(cb);
}

void CompliedBlock::clearPool() {
    for (CompliedBlock* const& cbp: pool) {
        delete cbp;
    }
    pool.clear();
}

size_t CompliedBlock::totalNumCbs() {
    return pool.size();
}

size_t CompliedBlock::totalCbMemoryCost() {
    size_t size = sizeof(pool) + sizeof(CompliedBlock*) * pool.capacity();
    for (CompliedBlock* const& cb: pool) {
        size += cb->memoryCost();
    }
    return size;
}

CompliedBlock::CompliedBlock(int** _complianceSet, int const _totalRows, int const _totalCols, bool _maintainComplianceSet): 
    complianceSet(_complianceSet), totalRows(_totalRows), totalCols(_totalCols), indices(nullptr),
    mainTainComplianceSet(_maintainComplianceSet) {}

CompliedBlock::~CompliedBlock() {
    if (mainTainComplianceSet) {
        delete[] complianceSet;
    }
    if (nullptr != indices) {
        for (int col_idx = 0; col_idx < totalCols; col_idx++) {
            for (std::pair<const int, std::vector<int*>*> const& kv: indices[col_idx]) {
                delete kv.second;
            }
        }
        delete[] indices;
    }
}

void CompliedBlock::buildIndices() {
    if (nullptr == indices) {
        indices = new std::unordered_map<int, std::vector<int*>*>[totalCols];
        for (int i = 0; i < totalRows; i++) {
            int* row = complianceSet[i];
            for (int arg_idx = 0; arg_idx < totalCols; arg_idx++) {
                std::unordered_map<int, std::vector<int*>*>& map = indices[arg_idx];
                int const val = row[arg_idx];
                std::unordered_map<int, std::vector<int*>*>::iterator itr = map.find(val);
                std::vector<int*>* set;
                if (map.end() == itr) {
                    set = new std::vector<int*>();
                    map.emplace(val, set);
                } else {
                    set = itr->second;
                }
                set->push_back(row);
            }
        }
    }
}

int* const* CompliedBlock::getComplianceSet() const {
    return complianceSet;
}

int CompliedBlock::getTotalRows() const {
    return totalRows;
}

int CompliedBlock::getTotalCols() const {
    return totalCols;
}

size_t CompliedBlock::memoryCost() const {
    size_t size = sizeof(CompliedBlock);
    if (mainTainComplianceSet) {
        size += sizeof(int*) * totalRows;
    }
    if (nullptr != indices) {
        size += sizeof(std::unordered_map<int, std::vector<int*>*>*) * totalCols;
        for (int i = 0; i < totalCols; i++) {
            std::unordered_map<int, std::vector<int*>*>& map = indices[i];
            size += sizeof(std::pair<int, std::vector<int*>*>) * map.bucket_count() * std::max(1.0f, map.max_load_factor());
            for (std::pair<const int, std::vector<int*>*> const& kv: map) {
                size += sizeof(std::vector<int*>) + sizeof(int*) * kv.second->capacity();
            }
        }
    }
    return size;
}

void CompliedBlock::showComplianceSet() const {
    for (int i = 0; i < totalRows; i++) {
        for (int j = 0; j < totalCols; j++) {
            std::cout << complianceSet[i][j] << ',';
        }
        std::cout << std::endl;
    }
}

CompliedBlock* CompliedBlock::getSlice(const CompliedBlock& cb, int const col, int const val) {
    std::unordered_map<int, std::vector<int*>*>::iterator itr = cb.indices[col].find(val);
    if (cb.indices[col].end() == itr) {
        return nullptr;
    }
    return CompliedBlock::create(toArray(*(itr->second)), itr->second->size(), cb.totalCols, true);
}

std::vector<CompliedBlock*>* CompliedBlock::splitSlices(const CompliedBlock& cb, int const col) {
    std::vector<CompliedBlock*>* vec = new std::vector<CompliedBlock*>();
    for (std::pair<const int, std::vector<int*>*> const& kv: cb.indices[col]) {
        vec->push_back(CompliedBlock::create(toArray(*(kv.second)), kv.second->size(), cb.totalCols, true));
    }
    return vec;
}

const sinc::MatchedSubCbs* CompliedBlock::matchSlices(
    const CompliedBlock& cb1, int const col1, const CompliedBlock& cb2, int const col2
) {
    MatchedSubCbs* result = new MatchedSubCbs();
    std::unordered_map<int, std::vector<int*>*> map1 = cb1.indices[col1];
    std::unordered_map<int, std::vector<int*>*> map2 = cb2.indices[col2];
    for (std::pair<const int, std::vector<int*>*> const& kv1: map1) {
        std::unordered_map<int, std::vector<int*>*>::iterator itr = map2.find(kv1.first);
        if (map2.end() != itr) {
            result->cbs1.push_back(CompliedBlock::create(toArray(*(kv1.second)), kv1.second->size(), cb1.totalCols, true));
            result->cbs2.push_back(CompliedBlock::create(toArray(*(itr->second)), itr->second->size(), cb2.totalCols, true));
        }
    }
    if (result->cbs1.empty()) {
        delete result;
        return nullptr;
    }
    return result;
}

const std::vector<CompliedBlock*>* CompliedBlock::matchSlices(const CompliedBlock& cb, int const col1, int const col2) {
    std::vector<CompliedBlock*>* vec = new std::vector<CompliedBlock*>();
    for (std::pair<const int, std::vector<int*>*> const& kv: cb.indices[col1]) {
        std::vector<int*> rows;
        for (int* const& row: *(kv.second)) {
            if (kv.first == row[col2]) {
                rows.push_back(row);
            }
        }
        if (!rows.empty()) {
            vec->push_back(CompliedBlock::create(toArray(rows), rows.size(), cb.totalCols, true));
        }
    }
    if (vec->empty()) {
        delete vec;
        return nullptr;
    }
    return vec;
}

/**
 * CachedSincPerfMonitor
 */
using sinc::CachedSincPerfMonitor;

void CachedSincPerfMonitor::show(std::ostream& os) {
    os << "\n### Cached SInC Performance Info ###\n\n";
    os << "--- Time Cost ---\n";
    printf(
        os, "(ms) %10s %10s %10s %10s %10s %10s %10s %10s %10s\n",
        "Copy", "E+.Upd", "E+.Prune", "T.Upd", "E.Upd", "E+.Idx", "T.Idx", "E.Idx", "Total"
    );
    printf(
        os, "     %10d %10d %10d %10d %10d %10d %10d %10d %10d\n\n",
        NANO_TO_MILL(copyTime),
        NANO_TO_MILL(posCacheUpdateTime), NANO_TO_MILL(prunedPosCacheUpdateTime), NANO_TO_MILL(entCacheUpdateTime), NANO_TO_MILL(allCacheUpdateTime),
        NANO_TO_MILL(posCacheIndexingTime), NANO_TO_MILL(entCacheIndexingTime), NANO_TO_MILL(allCacheIndexingTime),
        NANO_TO_MILL(copyTime + posCacheUpdateTime + prunedPosCacheUpdateTime + entCacheUpdateTime + allCacheUpdateTime + 
            posCacheIndexingTime + entCacheIndexingTime + allCacheIndexingTime)
    );

    os << "--- Memory Cost ---\n";
    printf(
        os, "%10s %10s %10s %10s %10s %10s %10s %10s %10s\n",
        "#CB", "CB", "CB(%)", "CE", "CE(%)", "FpC", "FpC(%)", "TbM", "TbM(%)"
    );
    rusage usage;
    if (0 != getrusage(RUSAGE_SELF, &usage)) {
        std::cerr << "Failed to get `rusage`" << std::endl;
        usage.ru_maxrss = 1024 * 1024 * 1024;   // set to 1T
    }
    printf(
        os, "%10d %10s %10.2f %10s %10.2f %10s %10.2f %10s %10.2f\n\n",
        CompliedBlock::totalNumCbs(), formatMemorySize(cbMemCost).c_str(), ((double) cbMemCost) / usage.ru_maxrss * 100.0,
        formatMemorySize(cacheEntryMemCost).c_str(), ((double) cacheEntryMemCost) / usage.ru_maxrss * 100.0,
        formatMemorySize(fingerprintCacheMemCost).c_str(), ((double) fingerprintCacheMemCost) / usage.ru_maxrss * 100.0,
        formatMemorySize(tabuMapMemCost).c_str(), ((double) tabuMapMemCost) / usage.ru_maxrss * 100.0
    );

    os << "--- Statistics ---\n";
    printf(
        os, "# %10s %10s %10s %10s %10s %10s %10s\n",
        "E+.Avg", "T.Avg", "E.Avg", "E+.Max", "T.Max", "E.Max", "Rules"
    );
    printf(
        os, "  %10.2f %10.2f %10.2f %10d %10d %10d %10d\n\n",
        ((double) posCacheEntriesTotal) / totalGeneratedRules,
        ((double) entCacheEntriesTotal) / totalGeneratedRules,
        ((double) allCacheEntriesTotal) / totalGeneratedRules,
        posCacheEntriesMax, entCacheEntriesMax, allCacheEntriesMax, totalGeneratedRules
    );
}

/**
 * VarInfo
 */
using sinc::VarInfo;

VarInfo::VarInfo() : tabIdx(-1), colIdx(-1), isPlv(false) {}

VarInfo::VarInfo(int const _tabIdx, int const _colIdx, bool _isPlv) : tabIdx(_tabIdx), colIdx(_colIdx), isPlv(_isPlv) {}

VarInfo::VarInfo(const VarInfo& another) : tabIdx(another.tabIdx), colIdx(another.colIdx), isPlv(another.isPlv) {}

bool VarInfo::isEmpty() const {
    return -1 == tabIdx;
}

VarInfo& VarInfo::operator=(VarInfo&& another) noexcept {
    tabIdx = another.tabIdx;
    colIdx = another.colIdx;
    isPlv = another.isPlv;
    return *this;
}

VarInfo& VarInfo::operator=(const VarInfo& another) noexcept {
    tabIdx = another.tabIdx;
    colIdx = another.colIdx;
    isPlv = another.isPlv;
    return *this;
}

bool VarInfo::operator==(const VarInfo &another) const {
    return tabIdx == another.tabIdx && colIdx == another.colIdx && isPlv == another.isPlv;
}

size_t VarInfo::hash() const {
    size_t h = tabIdx * 31 + colIdx;
    return h * 31 + isPlv;
}

size_t std::hash<VarInfo>::operator()(const VarInfo& r) const {
    return r.hash();
}

size_t std::hash<VarInfo*>::operator()(const VarInfo *r) const {
    return r->hash();
}

bool std::equal_to<VarInfo*>::operator()(const VarInfo *r1, const VarInfo *r2) const {
    return (*r1) == (*r2);
}

size_t std::hash<const VarInfo>::operator()(const VarInfo& r) const {
    return r.hash();
}

size_t std::hash<const VarInfo*>::operator()(const VarInfo *r) const {
    return r->hash();
}

bool std::equal_to<const VarInfo*>::operator()(const VarInfo *r1, const VarInfo *r2) const {
    return (*r1) == (*r2);
}

/**
 * BodyGvLinkInfo
 */
using sinc::BodyGvLinkInfo;
BodyGvLinkInfo::BodyGvLinkInfo(int const _bodyPredIdx, int const _bodyArgIdx, std::vector<int>* const _headVarLocs) : 
    bodyPredIdx(_bodyPredIdx), bodyArgIdx(_bodyArgIdx), headVarLocs(_headVarLocs) {}

BodyGvLinkInfo::~BodyGvLinkInfo() {
    delete headVarLocs;
}

/**
 * PlvLoc
 */
using sinc::PlvLoc;
PlvLoc::PlvLoc() : bodyPredIdx(-1), bodyArgIdx(-1), headArgIdx(-1) {}

PlvLoc::PlvLoc(int const _bodyPredIdx, int const _bodyArgIdx, int const _headArgIdx) :
    bodyPredIdx(_bodyPredIdx), bodyArgIdx(_bodyArgIdx), headArgIdx(_headArgIdx) {}

PlvLoc::PlvLoc(const PlvLoc& another) : bodyPredIdx(another.bodyPredIdx), bodyArgIdx(another.bodyArgIdx), headArgIdx(another.headArgIdx) {}

bool PlvLoc::isEmpty() const {
    return -1 == bodyPredIdx;
}

void PlvLoc::setEmpty() {
    bodyPredIdx = -1;
}

/**
 * CachedRule
 */
using sinc::CachedRule;
size_t CachedRule::cumulatedCacheEntryMemoryCost = 0;

CachedRule::CachedRule(
    int const headPredSymbol, int const arity, fingerprintCacheType& fingerprintCache, tabuMapType& category2TabuSetMap, SimpleKb& _kb
) : Rule(headPredSymbol, arity, fingerprintCache, category2TabuSetMap), kb(_kb)
{
    /* Initialize the E+-cache & T-cache */
    SimpleRelation* head_relation = kb.getRelation(headPredSymbol);
    SplitRecords* split_records = head_relation->splitByEntailment();
    std::vector<int*> const& non_entailed_record_vector = *(split_records->nonEntailedRecords);
    std::vector<int*> const& entailed_record_vector = *(split_records->entailedRecords);
    if (0 == entailed_record_vector.size()) {
        CompliedBlock* cb_head = CompliedBlock::create(head_relation->getAllRows(), head_relation->getTotalRows(), arity, false);
        cb_head->buildIndices();
        entryType* pos_init_entry = new entryType();
        pos_init_entry->push_back(cb_head);
        posCache = new entriesType();
        posCache->push_back(pos_init_entry);
        entCache = new entriesType();
    } else if (0 == non_entailed_record_vector.size()) {
        /* No record to entail, E+-cache and T-cache are both empty */
        posCache = new entriesType();
        entCache = new entriesType();
    } else {
        CompliedBlock* cb_head = CompliedBlock::create(
            toArray(non_entailed_record_vector), non_entailed_record_vector.size(), arity, true
        );
        cb_head->buildIndices();
        entryType* pos_init_entry = new entryType();
        pos_init_entry->push_back(cb_head);
        posCache = new entriesType();
        posCache->push_back(pos_init_entry);

        CompliedBlock* cb_head_ent = CompliedBlock::create(
            toArray(entailed_record_vector), entailed_record_vector.size(), arity, true
        );
        cb_head_ent->buildIndices();
        entryType* ent_init_entry = new entryType();
        ent_init_entry->push_back(cb_head_ent);
        entCache = new entriesType();
        entCache->push_back(ent_init_entry);
    }
    maintainPosCache = true;
    maintainEntCache = true;

    /* Initialize the E-cache */
    entryType* all_init_entry = new entryType();
    all_init_entry->push_back(nullptr); // Keep the same length of the cache entries
    allCache = new entriesType();
    allCache->push_back(all_init_entry);
    maintainAllCache = true;

    /* Initial evaluation */
    int pos_ent = non_entailed_record_vector.size();
    int already_ent = entailed_record_vector.size();
    double all_ent = pow(kb.totalConstants(), arity);
    eval = Eval(pos_ent, all_ent - already_ent, length);
    delete split_records;
}

CachedRule::CachedRule(const CachedRule& another) : Rule(another), kb(another.kb), posCache(another.posCache), maintainPosCache(false),
    entCache(another.entCache), maintainEntCache(false), allCache(another.allCache), maintainAllCache(false), 
    plvList(another.plvList)
{}

CachedRule::~CachedRule() {
    cumulatedCacheEntryMemoryCost -= getCacheEntryMemoryCost();
    if (maintainPosCache) {
        for (entryType* const& entry: *posCache) {
            delete entry;
        }
        delete posCache;
    }
    if (maintainEntCache) {
        for (entryType* const& entry: *entCache) {
            delete entry;
        }
        delete entCache;
    }
    if (maintainAllCache) {
        for (entryType* const& entry: *allCache) {
            delete entry;
        }
        delete allCache;
    }
}

CachedRule* CachedRule::clone() const {
    uint64_t time_start = currentTimeInNano();
    CachedRule* rule = new CachedRule(*this);
    rule->copyTime = currentTimeInNano() - time_start;
    return rule;
}

void CachedRule::updateCacheIndices() {
    uint64_t time_start = currentTimeInNano();
    for (entryType* const& entry: *posCache) {
        for (CompliedBlock* const& cb: *entry) {
            cb->buildIndices();
        }
    }
    uint64_t time_pos_done = currentTimeInNano();
    for (entryType* const& entry: *entCache) {
        for (CompliedBlock* const& cb: *entry) {
            cb->buildIndices();
        }
    }
    uint64_t time_ent_done = currentTimeInNano();
    for (entryType* const& entry: *allCache) {
        for (int pred_idx = FIRST_BODY_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
            (*entry)[pred_idx]->buildIndices();
        }
    }
    uint64_t time_all_done = currentTimeInNano();
    posCacheIndexingTime = time_pos_done - time_start;
    entCacheIndexingTime = time_ent_done - time_pos_done;
    allCacheIndexingTime = time_all_done - time_ent_done;
}

sinc::EvidenceBatch* CachedRule::getEvidenceAndMarkEntailment() {
    EvidenceBatch* evidence_batch = new EvidenceBatch(structure.size());
    for (int i = 0; i < structure.size(); i++) {
        Predicate const& predicate = structure[i];
        evidence_batch->predicateSymbolsInRule[i] = predicate.getPredSymbol();
        evidence_batch->aritiesInRule[i] = predicate.getArity();
    }

    SimpleRelation& target_relation = *(kb.getRelation(getHead().getPredSymbol()));
    for (entryType* const& cache_entry: *posCache) {
        /* Find the grounding body */
        int** grounding_template = new int*[structure.size()];
        for (int pred_idx = FIRST_BODY_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
            grounding_template[pred_idx] = (*cache_entry)[pred_idx]->getComplianceSet()[0];
        }

        /* Find all entailed records */
        CompliedBlock const& cb = *(*cache_entry)[HEAD_PRED_IDX];
        int* const* head_records = cb.getComplianceSet();
        if (1 < cb.getTotalRows()) {
            for (int i = 0; i < cb.getTotalRows(); i++) {
                int* head_record = head_records[i];
                if (target_relation.entailIfNot(head_record)) {
                    int** grounding = new int*[structure.size()];
                    std::copy(grounding_template + 1, grounding_template + structure.size(), grounding + 1);
                    grounding[HEAD_PRED_IDX] = head_record;
                    evidence_batch->evidenceList.push_back(grounding);
                }
            }
            delete[] grounding_template;
        } else {
            int* head_record = head_records[0];
            if (target_relation.entailIfNot(head_record)) {
                grounding_template[HEAD_PRED_IDX] = head_record;
                evidence_batch->evidenceList.push_back(grounding_template);
            } else {
                delete[] grounding_template;
            }
        }
    }
    return evidence_batch;
}

std::unordered_set<sinc::Record>* CachedRule::getCounterexamples() const {
    /* Find all variables in the head */
    std::unordered_map<int, std::vector<int>*> head_only_vid_2_loc_map;  // GVs will be removed later
    int uv_id = usedLimitedVars();
    Predicate const& head_pred = getHead();
    int const head_arity = head_pred.getArity();
    for (int arg_idx = 0; arg_idx < head_arity; arg_idx++) {
        int const argument = head_pred.getArg(arg_idx);
        if (ARG_IS_EMPTY(argument)) {
            std::vector<int>* list = new std::vector<int>();
            list->push_back(arg_idx);
            head_only_vid_2_loc_map.emplace(uv_id, list);   // assertion: success
            uv_id++;
        } else if (ARG_IS_VARIABLE(argument)) {
            int const vid = ARG_DECODE(argument);
            std::vector<int>* arg_locs;
            std::unordered_map<int, std::vector<int>*>::iterator itr = head_only_vid_2_loc_map.find(vid);
            if (head_only_vid_2_loc_map.end() == itr) {
                arg_locs = new std::vector<int>();
                head_only_vid_2_loc_map.emplace(vid, arg_locs);
            } else {
                arg_locs = itr->second;
            }
            arg_locs->push_back(arg_idx);
        }
    }

    /* Find GVs and PLVs in the body and their links to the head */
    /* List and group the argument indices of PLVs in each predicate */
    std::vector<BodyGvLinkInfo> pred_idx_2_plv_links[structure.size()];
    int pred_with_plvs_cnt = 0;
    for (int vid = 0; vid < plvList.size(); vid++) {
        PlvLoc const& plv_loc = plvList[vid];
        if (!plv_loc.isEmpty()) {
            std::unordered_map<int, std::vector<int> *>::iterator itr = head_only_vid_2_loc_map.find(vid);    // assertion: will be found
            std::vector<int>* head_var_locs = itr->second;
            head_only_vid_2_loc_map.erase(itr);
            if (pred_idx_2_plv_links[plv_loc.bodyPredIdx].empty()) {
                pred_idx_2_plv_links[plv_loc.bodyPredIdx].reserve(usedLimitedVars());
                pred_with_plvs_cnt++;
            }
            pred_idx_2_plv_links[plv_loc.bodyPredIdx].emplace_back(plv_loc.bodyPredIdx, plv_loc.bodyArgIdx, head_var_locs);
        }
    }
    std::vector<BodyGvLinkInfo> body_gv_links;
    body_gv_links.reserve(usedLimitedVars());
    for (int pred_idx = FIRST_BODY_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
        Predicate const& body_pred = structure[pred_idx];
        for (int arg_idx = 0; arg_idx < body_pred.getArity(); arg_idx++) {
            int argument = body_pred.getArg(arg_idx);
            if (ARG_IS_VARIABLE(argument)) {
                std::unordered_map<int, std::vector<int> *>::iterator itr = head_only_vid_2_loc_map.find(ARG_DECODE(argument));
                if (head_only_vid_2_loc_map.end() != itr) {
                    std::vector<int>* head_gv_locs = itr->second;
                    head_only_vid_2_loc_map.erase(itr);
                    body_gv_links.emplace_back(pred_idx, arg_idx, head_gv_locs);
                }
            }
        }
    }

    /* Bind GVs in the head, producing templates */
    std::unordered_set<Record>* head_templates = new std::unordered_set<Record>();
    if (0 == pred_with_plvs_cnt) {
        /* No PLV, bind all GVs as templates */
        for (entryType* const& cache_entry: *allCache) {
            int* new_template = new int[head_arity];
            std::copy(head_pred.getArgs(), head_pred.getArgs() + head_arity, new_template);
            for (BodyGvLinkInfo const& gv_link: body_gv_links) {
                int const val = (*cache_entry)[gv_link.bodyPredIdx]->getComplianceSet()[0][gv_link.bodyArgIdx];
                for (int const& head_arg_idx: *gv_link.headVarLocs) {
                    new_template[head_arg_idx] = val;
                }
            }
            std::pair<std::unordered_set<Record>::iterator, bool> ret = head_templates->emplace(new_template, head_arity);
            if (!ret.second) {
                delete[] new_template;
            }
        }
    } else {
        /* There are PLVs in the body */
        /* Find the bindings combinations of the GVs and the PLVs */
        int* base_template = new int[head_arity];
        std::copy(head_pred.getArgs(), head_pred.getArgs() + head_arity, base_template);
        std::unordered_set<Record> plv_bindings_within_pred_sets[pred_with_plvs_cnt];
        std::vector<BodyGvLinkInfo>* plv_link_lists[pred_with_plvs_cnt]{};
        for (entryType* const& cache_entry: *allCache) {
            /* Bind the GVs first */
            for (BodyGvLinkInfo const& gv_link: body_gv_links) {
                int const val = (*cache_entry)[gv_link.bodyPredIdx]->getComplianceSet()[0][gv_link.bodyArgIdx];
                for (int const& head_arg_idx: *gv_link.headVarLocs) {
                    base_template[head_arg_idx] = val;
                }
            }

            /* Find the combinations of PLV bindings */
            /* Note: the PLVs in the same predicate should be bind at the same time according to the records in the
                compliance set, and find the cartesian products of the groups of PLVs bindings. */
            {
                int i = 0;
                for (int body_pred_idx = FIRST_BODY_PRED_IDX; body_pred_idx < structure.size(); body_pred_idx++) {
                    std::vector<BodyGvLinkInfo>& plv_links = pred_idx_2_plv_links[body_pred_idx];
                    if (plv_links.empty()) {
                        continue;
                    }
                    std::unordered_set<Record>& plv_bindings = plv_bindings_within_pred_sets[i];
                    CompliedBlock const& cb = *((*cache_entry)[body_pred_idx]);
                    for (int j = 0; j < cb.getTotalRows(); j++) {
                        int* const cs_record = cb.getComplianceSet()[j];
                        int* const plv_binding_within_a_pred = new int[plv_links.size()];
                        for (int k = 0; k < plv_links.size(); k++) {
                            plv_binding_within_a_pred[k] = cs_record[plv_links[k].bodyArgIdx];
                        }
                        std::pair<std::unordered_set<Record>::iterator, bool> ret = plv_bindings.emplace(
                            plv_binding_within_a_pred, plv_links.size()
                        );
                        if (!ret.second) {
                            delete[] plv_binding_within_a_pred;
                        }
                    }
                    plv_link_lists[i] = &plv_links;
                    i++;
                }
            }
            addBodyPlvBindings2HeadTemplates(
                *head_templates, plv_bindings_within_pred_sets, plv_link_lists, base_template, 0, pred_with_plvs_cnt, head_arity
            );
            for (std::unordered_set<Record>& plv_bindings_within_pred_set: plv_bindings_within_pred_sets) {
                for (Record const& record: plv_bindings_within_pred_set) {
                    delete[] record.getArgs();
                }
                plv_bindings_within_pred_set.clear();
            }
        }
        delete[] base_template;
    }

    /* Extend head templates */
    SimpleRelation* target_relation = kb.getRelation(head_pred.getPredSymbol());
    if (head_only_vid_2_loc_map.empty()) {
        /* No need to extend UVs */
        for (std::unordered_set<Record>::iterator itr = head_templates->begin(); itr != head_templates->end(); ) {
            if (target_relation->hasRow(itr->getArgs())) {
                delete[] itr->getArgs();
                itr = head_templates->erase(itr);
            } else {
                itr++;
            }
        }
        return head_templates;
    } else {
        /* [a] Extend UVs in the templates */
        std::unordered_set<Record>* counter_example_set = new std::unordered_set<Record>();
        std::vector<int>* head_only_var_loc_lists[head_only_vid_2_loc_map.size()]{};
        int num_lists = 0;
        for (std::pair<const int, std::vector<int>*> const& kv: head_only_vid_2_loc_map) {
            head_only_var_loc_lists[num_lists] = kv.second;
            num_lists++;
        }
        for (Record const& head_template: *head_templates) {
            expandHeadUvs4CounterExamples(
                *target_relation, *counter_example_set, head_template.getArgs(), head_only_var_loc_lists, 0, num_lists
            );
            delete[] head_template.getArgs();
        }
        delete head_templates;
        for (std::pair<const int, std::vector<int>*> const& kv: head_only_vid_2_loc_map) {
            delete kv.second;
        }
        return counter_example_set;
    }
    return nullptr; // assertion: this line wil NOT be reached
}

void CachedRule::releaseMemory() {
    cumulatedCacheEntryMemoryCost -= getCacheEntryMemoryCost();
    if (maintainPosCache) {
        for (entryType* const& entry: *posCache) {
            delete entry;
        }
        delete posCache;
        maintainPosCache = false;
    }
    if (maintainEntCache) {
        for (entryType* const& entry: *entCache) {
            delete entry;
        }
        delete entCache;
        maintainEntCache = false;
    }
    if (maintainAllCache) {
        for (entryType* const& entry: *allCache) {
            delete entry;
        }
        delete allCache;
        maintainAllCache = false;
    }
}

uint64_t CachedRule::getCopyTime() const {
    return copyTime;
}

uint64_t CachedRule::getPosCacheUpdateTime() const {
    return posCacheUpdateTime;
}

uint64_t CachedRule::getEntCacheUpdateTime() const {
    return entCacheUpdateTime;
}

uint64_t CachedRule::getAllCacheUpdateTime() const {
    return allCacheUpdateTime;
}

uint64_t CachedRule::getPosCacheIndexingTime() const {
    return posCacheIndexingTime;
}

uint64_t CachedRule::getEntCacheIndexingTime() const {
    return entCacheIndexingTime;
}

uint64_t CachedRule::getAllCacheIndexingTime() const {
    return allCacheIndexingTime;
}

const CachedRule::entriesType& CachedRule::getPosCache() const {
    return *posCache;
}

const CachedRule::entriesType& CachedRule::getEntCache() const {
    return *entCache;
}

const CachedRule::entriesType& CachedRule::getAllCache() const {
    return *allCache;
}

size_t CachedRule::getCacheEntryMemoryCost() {
    if (0 != cacheMemoryCost) {
        return cacheMemoryCost;
    }
    if (maintainPosCache) {
        cacheMemoryCost += sizeof(entriesType) + sizeof(entryType*) * posCache->capacity() + sizeof(entryType) * posCache->size();
        size_t total_capacity = 0;
        for (entryType* entry: *posCache) {
            total_capacity += entry->capacity();
        }
        cacheMemoryCost += total_capacity * sizeof(CompliedBlock*);
    }
    if (maintainEntCache) {
        cacheMemoryCost += sizeof(entriesType) + sizeof(entryType*) * entCache->capacity() + sizeof(entryType) * entCache->size();
        size_t total_capacity = 0;
        for (entryType* entry: *entCache) {
            total_capacity += entry->capacity();
        }
        cacheMemoryCost += total_capacity * sizeof(CompliedBlock*);
    }
    if (maintainAllCache) {
        cacheMemoryCost += sizeof(entriesType) + sizeof(entryType*) * allCache->capacity() + sizeof(entryType) * allCache->size();
        size_t total_capacity = 0;
        for (entryType* entry: *allCache) {
            total_capacity += entry->capacity();
        }
        cacheMemoryCost += total_capacity * sizeof(CompliedBlock*);
    }
    return cacheMemoryCost;
}

size_t CachedRule::addCumulatedCacheEntryMemoryCost(CachedRule* rule) {
    cumulatedCacheEntryMemoryCost += rule->getCacheEntryMemoryCost();
    return cumulatedCacheEntryMemoryCost;
}

size_t CachedRule::getCumulatedCacheEntryMemoryCost() {
    return cumulatedCacheEntryMemoryCost;
}

void CachedRule::obtainPosCache() {
    if (!maintainPosCache) {
        entriesType* entries = new entriesType();
        for (entryType* const& entry: *posCache) {
            entries->emplace_back(new entryType(*entry));
        }
        posCache = entries;
        maintainPosCache = true;
    }
}

void CachedRule::obtainEntCache() {
    if (!maintainEntCache) {
        entriesType* entries = new entriesType();
        for (entryType* const& entry: *entCache) {
            entries->emplace_back(new entryType(*entry));
        }
        entCache = entries;
        maintainEntCache = true;
    }
}

void CachedRule::obtainAllCache() {
    if (!maintainAllCache) {
        entriesType* entries = new entriesType();
        for (entryType* const& entry: *allCache) {
            entries->emplace_back(new entryType(*entry));
        }
        allCache = entries;
        maintainAllCache = true;
    }
}

double CachedRule::recordCoverage() {
    std::unordered_set<int*> entailed_head;
    for (entryType* const& entry: *posCache) {
        CompliedBlock& cb = *((*entry)[HEAD_PRED_IDX]);
        int* const* const rows = cb.getComplianceSet();
        for (int i = 0; i < cb.getTotalRows(); i++) {
            entailed_head.insert(rows[i]);
        }
    }
    return ((double) entailed_head.size()) / kb.getRelation(getHead().getPredSymbol())->getTotalRows();
}

sinc::Eval CachedRule::calculateEval() const {
    /* Find all variables in the head */
    std::unordered_set<int> head_only_lv_args;  // For the head only LVs
    int head_uv_cnt = 0;
    Predicate const& head_pred = getHead();
    for (int arg_idx = 0; arg_idx < head_pred.getArity(); arg_idx++) {
        int argument = head_pred.getArg(arg_idx);
        if (ARG_IS_EMPTY(argument)) {
            head_uv_cnt++;
        } else if (ARG_IS_VARIABLE(argument)) {
            head_only_lv_args.insert(argument);    // The GVs will be removed later
        }
    }

    /* Find the first location of GVs in the body */
    std::vector<ArgLocation> body_gv_locs;   // PLVs are not included
    for (int pred_idx = FIRST_BODY_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
        Predicate const& body_pred = structure[pred_idx];
        for (int arg_idx = 0; arg_idx < body_pred.getArity(); arg_idx++) {
            int argument = body_pred.getArg(arg_idx);
            if (head_only_lv_args.erase(argument) && plvList[ARG_DECODE(argument)].isEmpty()) {
                body_gv_locs.emplace_back(pred_idx, arg_idx);
            }
        }
    }

    /* Count the number of all entailments */
    bool no_plv_in_rule = true;
    for (PlvLoc const& plv_loc: plvList) {
        if (!plv_loc.isEmpty()) {
            no_plv_in_rule = false;
            break;
        }
    }
    int body_gv_plv_bindings_cnt = 0;
    if (no_plv_in_rule) {
        /* Count all combinations of body GVs */
        std::unordered_set<Record> body_gv_bindings;
        for (entryType* const& cache_entry: *allCache) {
            int* binding = new int[body_gv_locs.size()];
            for (int i = 0; i < body_gv_locs.size(); i++) {
                ArgLocation const& loc = body_gv_locs[i];
                binding[i] = (*cache_entry)[loc.predIdx]->getComplianceSet()[0][loc.argIdx];
            }
            std::pair<std::unordered_set<Record>::iterator, bool> ret = body_gv_bindings.emplace(binding, body_gv_locs.size());
            if (!ret.second) {
                delete[] binding;
            }
        }
        body_gv_plv_bindings_cnt = body_gv_bindings.size();
        for (Record const& record: body_gv_bindings) {
            delete[] record.getArgs();
        }
    } else {
        /* List argument indices of PLVs in each predicate */
        std::vector<int> plv_arg_index_lists[structure.size()]; // Predicate index is the index of the array
        int preds_containing_plvs = 0;
        for (PlvLoc const& plv_loc: plvList) {
            if (!plv_loc.isEmpty()) {
                if (plv_arg_index_lists[plv_loc.bodyPredIdx].empty()) {
                    preds_containing_plvs++;
                }
                plv_arg_index_lists[plv_loc.bodyPredIdx].push_back(plv_loc.bodyArgIdx);
            }
        }

        /* Count the number of the combinations of GVs and PLVs */
        std::unordered_map<Record, std::unordered_set<Record>*> body_gv_binding_2_plv_bindings;
        std::unordered_set<Record> plv_bindings_within_pred_sets[preds_containing_plvs];
        for (entryType* const& cache_entry: *allCache) {
            /* Find the GV combination */
            int* gv_binding = new int[body_gv_locs.size()];
            for (int i = 0; i < body_gv_locs.size(); i++) {
                ArgLocation const& loc = body_gv_locs[i];
                gv_binding[i] = (*cache_entry)[loc.predIdx]->getComplianceSet()[0][loc.argIdx];
            }

            /* Find the combinations of PLV bindings */
            /* Note: the PLVs in the same predicate should be bind at the same time according to the records in the
                compliance set, and find the cartesian products of the groups of PLVs bindings. */
            int total_binding_length = 0;
            {
                int i = 0;
                for (int body_pred_idx = FIRST_BODY_PRED_IDX; body_pred_idx < structure.size(); body_pred_idx++) {
                    std::vector<int>& plv_arg_idxs = plv_arg_index_lists[body_pred_idx];
                    if (!plv_arg_idxs.empty()) {
                        std::unordered_set<Record>& plv_bindings = plv_bindings_within_pred_sets[i];
                        CompliedBlock const& cb = *((*cache_entry)[body_pred_idx]);
                        int* const* rows = cb.getComplianceSet();
                        for (int k = 0; k < cb.getTotalRows(); k++) {
                            int* cs_record = rows[k];
                            int* plv_binding_within_pred = new int[plv_arg_idxs.size()];
                            for (int j = 0; j < plv_arg_idxs.size(); j++) {
                                plv_binding_within_pred[j] = cs_record[plv_arg_idxs[j]];
                            }
                            std::pair<std::unordered_set<Record>::iterator, bool> ret = plv_bindings.emplace(
                                plv_binding_within_pred, plv_arg_idxs.size()
                            );
                            if (!ret.second) {
                                delete[] plv_binding_within_pred;
                            }
                        }
                        i++;
                        total_binding_length += plv_arg_idxs.size();
                    }
                }
            }
            std::unordered_set<Record>* complete_plv_bindings;
            Record gv_binding_rec(gv_binding, body_gv_locs.size());
            std::unordered_map<Record, std::unordered_set<Record>*>::iterator itr = body_gv_binding_2_plv_bindings.find(gv_binding_rec);
            if (body_gv_binding_2_plv_bindings.end() == itr) {
                complete_plv_bindings = new std::unordered_set<Record>();
                body_gv_binding_2_plv_bindings.emplace(gv_binding_rec, complete_plv_bindings);
            } else {
                complete_plv_bindings = itr->second;
                delete[] gv_binding;
            }
            int* arg_template = new int[total_binding_length];
            addCompleteBodyPlvBindings(
                    *complete_plv_bindings, plv_bindings_within_pred_sets, arg_template, 0, 0, preds_containing_plvs, total_binding_length
            );
            delete[] arg_template;
            for (std::unordered_set<Record>& plv_bindings_within_pred_set: plv_bindings_within_pred_sets) {
                for (Record const& record: plv_bindings_within_pred_set) {
                    delete[] record.getArgs();
                }
                plv_bindings_within_pred_set.clear();
            }
        }
        for (std::pair<const Record, std::unordered_set<Record>*> const& kv: body_gv_binding_2_plv_bindings) {
            body_gv_plv_bindings_cnt += kv.second->size();
            delete[] kv.first.getArgs();
            for (Record const& record: *(kv.second)) {
                delete[] record.getArgs();
            }
            delete kv.second;
        }
    }
    double all_entails = body_gv_plv_bindings_cnt * std::pow(
            kb.totalConstants(), head_uv_cnt + head_only_lv_args.size()
    );
    
    /* Count for the total and new positive entailments */
    std::unordered_set<int*> newly_proved;
    std::unordered_set<int*> already_proved;
    if (0 == head_uv_cnt) {
        /* No UV in the head, PAR is the record */
        for (entryType* const& cache_entry : *posCache) {
            int* record = (*cache_entry)[HEAD_PRED_IDX]->getComplianceSet()[0];
            newly_proved.insert(record);
        }
        for (entryType* const& cache_entry: *entCache) {
            int* record = (*cache_entry)[HEAD_PRED_IDX]->getComplianceSet()[0];
            already_proved.insert(record);
        }
    } else {
        /* UVs in the head, find all records in the CSs */
        for (entryType* const& cache_entry: *posCache) {
            CompliedBlock const& cb = *((*cache_entry)[HEAD_PRED_IDX]);
            int* const* rows = cb.getComplianceSet();
            for (int i = 0; i < cb.getTotalRows(); i++) {
                int* record = rows[i];
                newly_proved.insert(record);
            }
        }
        for (entryType* const& cache_entry: *entCache) {
            CompliedBlock const& cb = *((*cache_entry)[HEAD_PRED_IDX]);
            int* const* rows = cb.getComplianceSet();
            for (int i = 0; i < cb.getTotalRows(); i++) {
                int* record = rows[i];
                already_proved.insert(record);
            }
        }
    }

    /* Update evaluation score */
    /* Those already proved should be excluded from the entire entailment set. Otherwise, they are counted as negative ones */
    return Eval(newly_proved.size(), all_entails - already_proved.size(), length);
}

void CachedRule::addCompleteBodyPlvBindings(
        std::unordered_set<Record>& completeBindings, std::unordered_set<Record>* const plvBindingSets, int* const argTemplate,
        int const bindingSetIdx, int const templateStartIdx, int const numSets, int const arity
) const {
    std::unordered_set<Record>& plv_bindings = plvBindingSets[bindingSetIdx];
    std::unordered_set<Record>::iterator itr = plv_bindings.begin();
    int binding_length = itr->getArity();
    if (bindingSetIdx == numSets - 1) {
        /* Complete each template and add to the set */
        for (; plv_bindings.end() != itr; itr++) {
            std::copy(itr->getArgs(), itr->getArgs() + binding_length, argTemplate + templateStartIdx);
            int* new_arg = new int[arity];
            std::copy(argTemplate, argTemplate + arity, new_arg);
            std::pair<std::unordered_set<Record>::iterator, bool> ret = completeBindings.emplace(new_arg, arity);
            if (!ret.second) {
                delete[] new_arg;
            }
        }
    } else {
        /* Complete part of the template and move to next recursion */
        for (; plv_bindings.end() != itr; itr++) {
            std::copy(itr->getArgs(), itr->getArgs() + binding_length, argTemplate + templateStartIdx);
            addCompleteBodyPlvBindings(
                completeBindings, plvBindingSets, argTemplate, bindingSetIdx + 1, templateStartIdx + binding_length, numSets, arity
            );
        }
    }
}

sinc::UpdateStatus CachedRule::specCase1HandlerPrePruning(int const predIdx, int const argIdx, int const varId) {
    uint64_t time_start = currentTimeInNano();
    obtainPosCache();
    for (ArgLocation const& arg_loc: *(limitedVarArgs[varId])) {
        /* Find an argument that shares the same variable with the target */
        if (arg_loc.predIdx != predIdx || arg_loc.argIdx != argIdx) {   // Don't compare with the target argument
            /* Split */
            posCache = splitCacheEntries(posCache, predIdx, argIdx, arg_loc.predIdx, arg_loc.argIdx);
            break;
        }
    }
    uint64_t time_done = currentTimeInNano();
    posCacheUpdateTime = time_done - time_start;
    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::specCase1HandlerPostPruning(int const predIdx, int const argIdx, int const varId) {
    uint64_t time_start = currentTimeInNano();
    obtainEntCache();
    for (ArgLocation const& arg_loc: *(limitedVarArgs[varId])) {
        /* Find an argument that shares the same variable with the target */
        if (arg_loc.predIdx != predIdx || arg_loc.argIdx != argIdx) {
            /* Split */
            entCache = splitCacheEntries(entCache, predIdx, argIdx, arg_loc.predIdx, arg_loc.argIdx);
            break;
        }
    }
    uint64_t time_ent_done = currentTimeInNano();

    obtainAllCache();
    if (HEAD_PRED_IDX != predIdx) { // No need to update the E-cache if the update is in the head
        PlvLoc& plv_loc = plvList[varId];
        if (!plv_loc.isEmpty()) {
            /* Match the existing PLV, split */
            allCache = splitCacheEntries(allCache, predIdx, argIdx, plv_loc.bodyPredIdx, plv_loc.bodyArgIdx);
            plv_loc.setEmpty();
        } else {
            bool found = false;
            for (ArgLocation const& arg_loc: *(limitedVarArgs[varId])) {
                /* Find an argument in the body that shares the same variable with the target */
                if (HEAD_PRED_IDX != arg_loc.predIdx && (arg_loc.predIdx != predIdx || arg_loc.argIdx != argIdx)) { // Don't compare with the target argument
                    /* Split */
                    allCache = splitCacheEntries(allCache, predIdx, argIdx, arg_loc.predIdx, arg_loc.argIdx);
                    found = true;
                    break;
                }
            }
            if (!found) {
                /* No matching LV in the body, record as a PLV */
                int const arg_var = ARG_VARIABLE(varId);
                Predicate const& head_pred = getHead();
                for (int arg_idx = 0; arg_idx < head_pred.getArity(); arg_idx++) {
                    if (arg_var == head_pred.getArg(arg_idx)) {
                        plvList[varId].bodyPredIdx = predIdx;
                        plvList[varId].bodyArgIdx = argIdx;
                        plvList[varId].headArgIdx = arg_idx;
                        break;
                    }
                }
            }
        }
    }

    uint64_t time_all_done = currentTimeInNano();
    entCacheUpdateTime = time_ent_done - time_start;
    allCacheUpdateTime = time_all_done - time_ent_done;

    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::specCase2HandlerPrePruning(int const predSymbol, int const arity, int const argIdx, int const varId) {
    uint64_t time_start = currentTimeInNano();
    obtainPosCache();
    SimpleRelation* new_relation = kb.getRelation(predSymbol);
    for (ArgLocation const& arg_loc: *(limitedVarArgs[varId])) {
        /* Find an argument that shares the same variable with the target */
        if (arg_loc.predIdx != structure.size() - 1 || arg_loc.argIdx != argIdx) {  // Don't find in the appended predicate
            /* Append + Split */
            entriesType* tmp_cache = appendCacheEntries(posCache, new_relation);
            posCache = splitCacheEntries(tmp_cache, structure.size() - 1, argIdx, arg_loc.predIdx, arg_loc.argIdx);
            break;
        }
    }
    uint64_t time_done = currentTimeInNano();
    posCacheUpdateTime = time_done - time_start;
    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::specCase2HandlerPostPruning(int const predSymbol, int const arity, int const argIdx, int const varId) {
    uint64_t time_start = currentTimeInNano();
    obtainEntCache();
    SimpleRelation* new_relation = kb.getRelation(predSymbol);
    for (ArgLocation const& arg_loc: *(limitedVarArgs[varId])) {
        /* Find an argument that shares the same variable with the target */
        if (arg_loc.predIdx != structure.size() - 1 || arg_loc.argIdx != argIdx) {  // Don't find in the appended predicate
            /* Append + Split */
            entriesType* tmp_cache = appendCacheEntries(entCache, new_relation);
            entCache = splitCacheEntries(tmp_cache, structure.size() - 1, argIdx, arg_loc.predIdx, arg_loc.argIdx);
            break;
        }
    }
    uint64_t time_ent_done = currentTimeInNano();

    obtainAllCache();
    PlvLoc& plv_loc = plvList[varId];
    if (!plv_loc.isEmpty()) {
        /* Match the found PLV, append then split */
        entriesType* tmp_cache = appendCacheEntries(allCache, new_relation);
        allCache = splitCacheEntries(tmp_cache, structure.size() - 1, argIdx, plv_loc.bodyPredIdx, plv_loc.bodyArgIdx);
        plv_loc.setEmpty();
    } else {
        bool found = false;
        for (ArgLocation const& arg_loc: *(limitedVarArgs[varId])) {
            /* Find an argument in the body that shares the same variable with the target */
            if (HEAD_PRED_IDX != arg_loc.predIdx && (arg_loc.predIdx != structure.size() - 1 || arg_loc.argIdx != argIdx)) { // Don't compare with the target argument
                /* Append + Split */
                entriesType* tmp_cache = appendCacheEntries(allCache, new_relation);
                allCache = splitCacheEntries(tmp_cache, structure.size() - 1, argIdx, arg_loc.predIdx, arg_loc.argIdx);
                found = true;
                break;
            }
        }
        if (!found) {
            /* No matching LV in the body, record as a PLV, and append */
            int arg_var = ARG_VARIABLE(varId);
            Predicate const& head_pred = getHead();
            for (int arg_idx = 0; arg_idx < head_pred.getArity(); arg_idx++) {
                if (arg_var == head_pred.getArg(arg_idx)) {
                    plvList[varId].bodyPredIdx = structure.size() - 1;
                    plvList[varId].bodyArgIdx = argIdx;
                    plvList[varId].headArgIdx = arg_idx;
                    break;
                }
            }
            allCache = appendCacheEntries(allCache, new_relation);
        }
    }
    uint64_t time_all_done = currentTimeInNano();
    entCacheUpdateTime = time_ent_done - time_start;
    allCacheUpdateTime = time_all_done - time_ent_done;

    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::specCase3HandlerPrePruning(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) {
    uint64_t time_start = currentTimeInNano();
    obtainPosCache();
    posCache = splitCacheEntries(posCache, predIdx1, argIdx1, predIdx2, argIdx2);
    uint64_t time_done = currentTimeInNano();
    posCacheUpdateTime = time_done - time_start;
    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::specCase3HandlerPostPruning(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) {
    uint64_t time_start = currentTimeInNano();
    obtainEntCache();
    entCache = splitCacheEntries(entCache, predIdx1, argIdx1, predIdx2, argIdx2);
    uint64_t time_ent_done = currentTimeInNano();

    obtainAllCache();
    if (HEAD_PRED_IDX != predIdx1 || HEAD_PRED_IDX != predIdx2) {   // At least one modified predicate is in the body. Otherwise, nothing should be done.
        if (HEAD_PRED_IDX == predIdx1 || HEAD_PRED_IDX == predIdx2) {   // One is the head and the other is not
            /* The new variable is a PLV */
            if (HEAD_PRED_IDX == predIdx1) {
                plvList.emplace_back(predIdx2, argIdx2, argIdx1);
            } else {
                plvList.emplace_back(predIdx1, argIdx1, argIdx2);
            }
        } else {    // Both are in the body
            /* The new variable is not a PLV, split */
            plvList.emplace_back();
            allCache = splitCacheEntries(allCache, predIdx1, argIdx1, predIdx2, argIdx2);
        }
    } else {
        /* Both are in the head. Extend the PLV list */
        plvList.emplace_back();
    }
    uint64_t time_all_done = currentTimeInNano();
    entCacheUpdateTime = time_ent_done - time_start;
    allCacheUpdateTime = time_all_done - time_ent_done;

    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::specCase4HandlerPrePruning(
    int const predSymbol, int const arity, int const argIdx1, int const predIdx2, int const argIdx2
) {
    uint64_t time_start = currentTimeInNano();
    obtainPosCache();
    SimpleRelation* new_relation = kb.getRelation(predSymbol);
    entriesType* tmp_cache = appendCacheEntries(posCache, new_relation);
    posCache = splitCacheEntries(tmp_cache, structure.size() - 1, argIdx1, predIdx2, argIdx2);
    uint64_t time_done = currentTimeInNano();
    posCacheUpdateTime = time_done - time_start;
    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::specCase4HandlerPostPruning(
    int const predSymbol, int const arity, int const argIdx1, int const predIdx2, int const argIdx2
) {
    long time_start = currentTimeInNano();
    SimpleRelation* new_relation = kb.getRelation(predSymbol);
    obtainEntCache();
    entriesType* tmp_cache = appendCacheEntries(entCache, new_relation);
    entCache = splitCacheEntries(tmp_cache, structure.size() - 1, argIdx1, predIdx2, argIdx2);
    uint64_t time_ent_done = currentTimeInNano();

    obtainAllCache();
    if (HEAD_PRED_IDX == predIdx2) {   // One is the head and the other is not
        /* The new variable is a PLV, append */
        plvList.emplace_back(structure.size() - 1, argIdx1, argIdx2);
        allCache = appendCacheEntries(allCache, new_relation);
    } else {    // Both are in the body
        /* The new variable is not a PLV, append then split */
        plvList.emplace_back();
        tmp_cache = appendCacheEntries(allCache, new_relation);
        allCache = splitCacheEntries(tmp_cache, structure.size() - 1, argIdx1, predIdx2, argIdx2);
    }
    uint64_t time_all_done = currentTimeInNano();
    entCacheUpdateTime = time_ent_done - time_start;
    allCacheUpdateTime = time_all_done - time_ent_done;

    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::specCase5HandlerPrePruning(int const predIdx, int const argIdx, int const constant) {
    uint64_t time_start = currentTimeInNano();
    obtainPosCache();
    posCache = assignCacheEntries(posCache, predIdx, argIdx, constant);
    uint64_t time_done = currentTimeInNano();
    posCacheUpdateTime = time_done - time_start;
    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::specCase5HandlerPostPruning(int const predIdx, int const argIdx, int const constant) {
    uint64_t time_start = currentTimeInNano();
    obtainEntCache();
    entCache = assignCacheEntries(entCache, predIdx, argIdx, constant);
    uint64_t time_ent_done = currentTimeInNano();

    obtainAllCache();
    if (HEAD_PRED_IDX != predIdx) { // No need to update the E-cache if the update is in the head
        /* Assign */
        allCache = assignCacheEntries(allCache, predIdx, argIdx, constant);
    }
    uint64_t time_all_done = currentTimeInNano();
    entCacheUpdateTime = time_ent_done - time_start;
    allCacheUpdateTime = time_all_done - time_ent_done;

    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::generalizeHandlerPrePruning(int const predIdx, int const argIdx) {
    return UpdateStatus::Invalid;
}

sinc::UpdateStatus CachedRule::generalizeHandlerPostPruning(int const predIdx, int const argIdx) {
    return UpdateStatus::Invalid;
}

CachedRule::entriesType* CachedRule::appendCacheEntries(entriesType* cache, IntTable* const newRelation) {
    CompliedBlock* cb = CompliedBlock::create(
        newRelation->getAllRows(), newRelation->getTotalRows(), newRelation->getTotalCols(), false
    );
    cb->buildIndices();
    entriesType* new_cache = new entriesType();
    for (entryType* const& entry: *cache) {
        entryType* new_entry = new entryType(*entry);
        new_entry->push_back(cb);
        new_cache->push_back(new_entry);
        delete entry;
    }
    delete cache;
    return new_cache;
}

CachedRule::entriesType* CachedRule::splitCacheEntries(entriesType* cache, int predIdx1, int argIdx1, int predIdx2, int argIdx2) {
    entriesType* new_cache = new entriesType();
    if (predIdx1 == predIdx2) {
        for (entryType* const& cache_entry: *cache) {
            CompliedBlock const& cb = *((*cache_entry)[predIdx1]);
            std::vector<CompliedBlock*> const* slices = CompliedBlock::matchSlices(cb, argIdx1, argIdx2);
            if (nullptr != slices) {
                for (CompliedBlock* const& slice: *slices) {
                    entryType* new_entry = new entryType(*cache_entry);
                    (*new_entry)[predIdx1] = slice;
                    new_cache->push_back(new_entry);
                }
                delete slices;
            }
            delete cache_entry;
        }
    } else {
        for (entryType* const& cache_entry : *cache) {
            CompliedBlock const& cb1 = *((*cache_entry)[predIdx1]);
            CompliedBlock const& cb2 = *((*cache_entry)[predIdx2]);
            MatchedSubCbs const* slices = CompliedBlock::matchSlices(cb1, argIdx1, cb2, argIdx2);
            if (nullptr != slices) {
                for (int i = 0; i < slices->cbs1.size(); i++) {
                    CompliedBlock* slice1 = (slices->cbs1)[i];
                    CompliedBlock* slice2 = (slices->cbs2)[i];
                    entryType* new_entry = new entryType(*cache_entry);
                    (*new_entry)[predIdx1] = slice1;
                    (*new_entry)[predIdx2] = slice2;
                    new_cache->push_back(new_entry);
                }
                delete slices;
            }
            delete cache_entry;
        }
    }
    delete cache;
    return new_cache;
}

CachedRule::entriesType* CachedRule::assignCacheEntries(entriesType* cache, int predIdx, int argIdx, int constant) {
    entriesType* new_cache = new entriesType();
    for (entryType* const& cache_entry: *cache) {
        CompliedBlock const& cb = *((*cache_entry)[predIdx]);
        CompliedBlock* slice = CompliedBlock::getSlice(cb, argIdx, constant);
        if (nullptr != slice) {
            entryType* new_entry = new entryType(*cache_entry);
            (*new_entry)[predIdx] = slice;
            new_cache->push_back(new_entry);
        }
        delete cache_entry;
    }
    delete cache;
    return new_cache;
}

void CachedRule::addBodyPlvBindings2HeadTemplates(
        std::unordered_set<Record>& headTemplates, std::unordered_set<Record>* const plvBindingSets,
        std::vector<BodyGvLinkInfo>** const plvLinkLists, int* const argTemplate, int const linkIdx, int const numSets,
        int const arity
) const {
    std::unordered_set<Record>& plv_bindings = plvBindingSets[linkIdx];
    std::vector<BodyGvLinkInfo>& plv_links = *(plvLinkLists[linkIdx]);
    if (linkIdx == numSets - 1) {
        /* Finish the last group of PLVs, add to the template set */
        for (Record const& plv_binding: plv_bindings) {
            for (int i = 0; i < plv_binding.getArity(); i++) {
                BodyGvLinkInfo const& plv_link = plv_links[i];
                for (int const& head_arg_idx: *(plv_link.headVarLocs)) {
                    argTemplate[head_arg_idx] = plv_binding.getArgs()[i];
                }
            }
            int* cloned_args = new int[arity];
            std::copy(argTemplate, argTemplate + arity, cloned_args);
            std::pair<std::unordered_set<Record>::iterator, bool> ret = headTemplates.emplace(cloned_args, arity);
            if (!ret.second) {
                delete[] cloned_args;
            }
        }
    } else {
        /* Add current binding to the template and move to the next recursion */
        for (Record const& plv_binding: plv_bindings) {
            for (int i = 0; i < plv_binding.getArity(); i++) {
                BodyGvLinkInfo const& plv_link = plv_links[i];
                for (int const& head_arg_idx: *(plv_link.headVarLocs)) {
                    argTemplate[head_arg_idx] = plv_binding.getArgs()[i];
                }
            }
            addBodyPlvBindings2HeadTemplates(headTemplates, plvBindingSets, plvLinkLists, argTemplate, linkIdx + 1, numSets, arity);
        }
    }
}

void CachedRule::expandHeadUvs4CounterExamples(
    SimpleRelation const& targetRelation, std::unordered_set<Record>& counterexamples, int* const argTemplate,
    std::vector<int>** varLocs, int const idx, int const numVectors
) const {
    std::vector<int>& locations = *(varLocs[idx]);
    if (idx < numVectors - 1) {
        /* Expand current UV and move to the next recursion */
        for (int constant_symbol = 1; constant_symbol <= kb.totalConstants(); constant_symbol++) {
            int argument = ARG_CONSTANT(constant_symbol);
            for (int loc: locations) {
                argTemplate[loc] = argument;
            }
            expandHeadUvs4CounterExamples(targetRelation, counterexamples, argTemplate, varLocs, idx + 1, numVectors);
        }
    } else {
        /* Expand the last UV and add to counterexample set if it is */
        int const arity = targetRelation.getTotalCols();
        for (int constant_symbol = 1; constant_symbol <= kb.totalConstants(); constant_symbol++) {
            int argument = ARG_CONSTANT(constant_symbol);
            for (int loc: locations) {
                argTemplate[loc] = argument;
            }
            if (!targetRelation.hasRow(argTemplate)) {
                int* cloned_args = new int[arity];
                std::copy(argTemplate, argTemplate + arity, cloned_args);
                counterexamples.emplace(cloned_args, arity);
            }
        }
    }
}

/**
 * RelationMinerWithCachedRule
 */
using sinc::RelationMinerWithCachedRule;

RelationMinerWithCachedRule::RelationMinerWithCachedRule(
    SimpleKb& kb, int const targetRelation, EvalMetric::Value evalMetric, int const beamwidth, double const stopCompressionRatio,
    nodeMapType& predicate2NodeMap, depGraphType& dependencyGraph, std::vector<Rule*>& hypothesis,
    std::unordered_set<Record>& counterexamples, std::ostream& logger
) : RelationMiner(
        kb, targetRelation, evalMetric, beamwidth, stopCompressionRatio, predicate2NodeMap, dependencyGraph, hypothesis,
        counterexamples, logger
) {}

RelationMinerWithCachedRule::~RelationMinerWithCachedRule() {
    for (Rule::fingerprintCacheType* const& cache: fingerprintCaches) {
        for (const Fingerprint* const& fp: *cache) {
            delete fp;
        }
        delete cache;
    }
}

size_t RelationMinerWithCachedRule::getFingerprintCacheMemCost() const {
    size_t size = sizeof(fingerprintCaches) + sizeof(Rule::fingerprintCacheType*) * fingerprintCaches.capacity() +
        sizeof(Rule::fingerprintCacheType) * fingerprintCaches.size();
    for (Rule::fingerprintCacheType* const& cache: fingerprintCaches) {
        size += sizeof(Fingerprint*) * cache->bucket_count() * std::max(1.0f, cache->max_load_factor());
        for (const Fingerprint* const& fp: *cache) {
            size += fp->getMemCost();
        }
    }
    return size;
}

size_t RelationMinerWithCachedRule::getTabuMapMemCost() const {
    size_t size = sizeof(Rule::tabuMapType) + 
        (sizeof(std::pair<MultiSet<int>*, Rule::fingerprintCacheType*>) + sizeof(MultiSet<int>) + sizeof(Rule::fingerprintCacheType)) * 
        tabuMap.bucket_count() * std::max(1.0f, tabuMap.max_load_factor());
    for (std::pair<MultiSet<int>*, Rule::fingerprintCacheType*> const& kv: tabuMap) {
        size += sizeof(std::pair<int, int>) * kv.first->getCntMap().bucket_count() * std::max(1.0f, kv.first->getCntMap().max_load_factor());
        size += sizeof(Fingerprint*) * kv.second->bucket_count() * std::max(1.0f, kv.second->max_load_factor());
    }
    return size;
}

sinc::Rule* RelationMinerWithCachedRule::getStartRule() {
    Rule::fingerprintCacheType* cache = new Rule::fingerprintCacheType();
    fingerprintCaches.push_back(cache);
    CachedRule* rule = new CachedRule(targetRelation, kb.getRelation(targetRelation)->getTotalCols(), *cache, tabuMap, kb);
    monitor.cacheEntryMemCost = std::max(monitor.cacheEntryMemCost, CachedRule::addCumulatedCacheEntryMemoryCost(rule));
    return rule;
}

void RelationMinerWithCachedRule::selectAsBeam(Rule* r) {
    CachedRule* rule = (CachedRule*) r;
    rule->updateCacheIndices();
    monitor.posCacheIndexingTime += rule->getPosCacheIndexingTime();
    monitor.entCacheIndexingTime += rule->getEntCacheIndexingTime();
    monitor.allCacheIndexingTime += rule->getAllCacheIndexingTime();
}

int RelationMinerWithCachedRule::checkThenAddRule(
    UpdateStatus updateStatus, Rule* const updatedRule, Rule& originalRule, Rule** candidates
) {
    CachedRule* rule = (CachedRule*) updatedRule;
    if (UpdateStatus::Normal == updateStatus) {
        monitor.posCacheEntriesTotal += rule->getPosCache().size();
        monitor.entCacheEntriesTotal += rule->getEntCache().size();
        monitor.allCacheEntriesTotal += rule->getAllCache().size();
        monitor.posCacheEntriesMax = std::max(monitor.posCacheEntriesMax, (int)rule->getPosCache().size());
        monitor.entCacheEntriesMax = std::max(monitor.entCacheEntriesMax, (int)rule->getEntCache().size());
        monitor.entCacheEntriesMax = std::max(monitor.allCacheEntriesMax, (int)rule->getAllCache().size());
        monitor.totalGeneratedRules++;
        monitor.posCacheUpdateTime += rule->getPosCacheUpdateTime();
        monitor.entCacheUpdateTime += rule->getEntCacheUpdateTime();
        monitor.allCacheUpdateTime += rule->getAllCacheUpdateTime();
    } else {
        monitor.prunedPosCacheUpdateTime += rule->getPosCacheUpdateTime();
    }
    monitor.copyTime += rule->getCopyTime();
    monitor.cacheEntryMemCost = std::max(monitor.cacheEntryMemCost, CachedRule::addCumulatedCacheEntryMemoryCost(rule));

    return RelationMiner::checkThenAddRule(updateStatus, updatedRule, originalRule, candidates);
}

/**
 * SincWithCache
 */
using sinc::SincWithCache;

SincWithCache::SincWithCache(SincConfig* const config) : SInC(config) {}

SincWithCache::SincWithCache(SincConfig* const config, SimpleKb* const kb) : SInC(config, kb) {}

sinc::SincRecovery* SincWithCache::createRecovery() {
    return nullptr; // Todo: Implement here
}

sinc::RelationMiner* SincWithCache::createRelationMiner(int const targetRelationNum) {
    return new RelationMinerWithCachedRule(
        *kb, targetRelationNum, config->evalMetric, config->beamwidth, config->stopCompressionRatio, predicate2NodeMap,
        dependencyGraph, compressedKb->getHypothesis(), compressedKb->getCounterexampleSet(targetRelationNum), *logger
    );
}

void SincWithCache::finalizeRelationMiner(RelationMiner* miner) {
    SInC::finalizeRelationMiner(miner);
    RelationMinerWithCachedRule* rel_miner = (RelationMinerWithCachedRule*) miner;
    monitor.posCacheUpdateTime += rel_miner->monitor.posCacheUpdateTime;
    monitor.prunedPosCacheUpdateTime += rel_miner->monitor.prunedPosCacheUpdateTime;
    monitor.entCacheUpdateTime += rel_miner->monitor.entCacheUpdateTime;
    monitor.allCacheUpdateTime += rel_miner->monitor.allCacheUpdateTime;
    monitor.posCacheIndexingTime += rel_miner->monitor.posCacheIndexingTime;
    monitor.entCacheIndexingTime += rel_miner->monitor.entCacheIndexingTime;
    monitor.allCacheIndexingTime += rel_miner->monitor.allCacheIndexingTime;
    monitor.posCacheEntriesTotal += rel_miner->monitor.posCacheEntriesTotal;
    monitor.entCacheEntriesTotal += rel_miner->monitor.entCacheEntriesTotal;
    monitor.allCacheEntriesTotal += rel_miner->monitor.allCacheEntriesTotal;
    monitor.posCacheEntriesMax = std::max(monitor.posCacheEntriesMax, rel_miner->monitor.posCacheEntriesMax);
    monitor.entCacheEntriesMax = std::max(monitor.entCacheEntriesMax, rel_miner->monitor.entCacheEntriesMax);
    monitor.allCacheEntriesMax = std::max(monitor.allCacheEntriesMax, rel_miner->monitor.allCacheEntriesMax);
    monitor.totalGeneratedRules += rel_miner->monitor.totalGeneratedRules;
    monitor.copyTime += rel_miner->monitor.copyTime;
    monitor.cacheEntryMemCost = std::max(monitor.cacheEntryMemCost, rel_miner->monitor.cacheEntryMemCost);
    monitor.fingerprintCacheMemCost = std::max(monitor.fingerprintCacheMemCost, rel_miner->getFingerprintCacheMemCost());
    monitor.tabuMapMemCost = std::max(monitor.tabuMapMemCost, rel_miner->getTabuMapMemCost());
}

void SincWithCache::showMonitor() {
    SInC::showMonitor();

    /* Calculate memory cost */
    monitor.cbMemCost = CompliedBlock::totalCbMemoryCost() / 1024;
    monitor.cacheEntryMemCost /= 1024;
    monitor.fingerprintCacheMemCost /= 1024;
    monitor.tabuMapMemCost /= 1024;

    monitor.show(*logger);
}

void SincWithCache::finish() {
    CompliedBlock::clearPool();
}
