#include "sincWithCache.h"
#include <stdarg.h>
#include <cmath>
#include <algorithm>
#include <sys/resource.h>

/** This var is used for calculating the maximum memory cost during evaluation */
static size_t _evaluation_memory_cost = 0;
static double max_gv_bindings = 0;

/**
 * MatchedSubCbs
 */
using sinc::MatchedSubCbs;
size_t MatchedSubCbs::calcMemoryCost() const {
    return sizeof(MatchedSubCbs) + sizeof(CompliedBlock*) * (cbs1.capacity() + cbs2.capacity());
}

/**
 * Identifiers for CB update operations
 */
using sinc::CbOprGetSlice;
CbOprGetSlice::CbOprGetSlice(int const _id, int const _col, int const _val) : id(_id), col(_col), val(_val) {}

bool CbOprGetSlice::operator==(const CbOprGetSlice& another) const {
    return id == another.id && col == another.col && val == another.val;
}

size_t CbOprGetSlice::hash() const {
    size_t h = id * 31 + col;
    h = h * 31 + val;
    return h;
}

size_t std::hash<CbOprGetSlice>::operator()(const CbOprGetSlice& r) const {
    return r.hash();
}

size_t std::hash<const CbOprGetSlice>::operator()(const CbOprGetSlice& r) const {
    return r.hash();
}

using sinc::CbOprSplitSlices;
CbOprSplitSlices::CbOprSplitSlices(int const _id, int const _col) : id(_id), col(_col) {}

bool CbOprSplitSlices::operator==(const CbOprSplitSlices& another) const {
    return id == another.id && col == another.col;
}

size_t CbOprSplitSlices::hash() const {
    return id * 31 + col;
}

size_t std::hash<CbOprSplitSlices>::operator()(const CbOprSplitSlices& r) const {
    return r.hash();
}

size_t std::hash<const CbOprSplitSlices>::operator()(const CbOprSplitSlices& r) const {
    return r.hash();
}

using sinc::CbOprMatchSlicesOneCb;
CbOprMatchSlicesOneCb::CbOprMatchSlicesOneCb(int const _id, int const _col1, int const _col2) : id(_id), col1(_col1), col2(_col2) {}

bool CbOprMatchSlicesOneCb::operator==(const CbOprMatchSlicesOneCb& another) const {
    return id == another.id && col1 == another.col1 && col2 == another.col2;
}

size_t CbOprMatchSlicesOneCb::hash() const  {
    size_t h = id * 31 + col1;
    h = h * 31 + col2;
    return h;
}

size_t std::hash<CbOprMatchSlicesOneCb>::operator()(const CbOprMatchSlicesOneCb& r) const {
    return r.hash();
}

size_t std::hash<const CbOprMatchSlicesOneCb>::operator()(const CbOprMatchSlicesOneCb& r) const {
    return r.hash();
}

using sinc::CbOprMatchSlicesTwoCbs;
CbOprMatchSlicesTwoCbs::CbOprMatchSlicesTwoCbs(int const _id1, int const _col1, int const _id2, int const _col2) : 
    id1(_id1), col1(_col1), id2(_id2), col2(_col2) {}

bool CbOprMatchSlicesTwoCbs::operator==(const CbOprMatchSlicesTwoCbs& another) const {
    return id1 == another.id1 && col1 == another.col1 && id2 == another.id2 && col2 == another.col2;
}

size_t CbOprMatchSlicesTwoCbs::hash() const {
    size_t h = id1 * 31 + col1;
    h = h * 31 + id2;
    h = h * 31 + col2;
    return h;
}

size_t std::hash<CbOprMatchSlicesTwoCbs>::operator()(const CbOprMatchSlicesTwoCbs& r) const {
    return r.hash();
}

size_t std::hash<const CbOprMatchSlicesTwoCbs>::operator()(const CbOprMatchSlicesTwoCbs& r) const {
    return r.hash();
}

/**
 * CompliedBlock
 */
using sinc::CompliedBlock;
using sinc::IntTable;
using sinc::MatchedSubCbs;

std::vector<CompliedBlock*> CompliedBlock::pool;
std::unordered_map<void*, CompliedBlock*> CompliedBlock::mapCreation;
std::unordered_map<CbOprGetSlice, CompliedBlock*> CompliedBlock::mapGetSlice;
std::unordered_map<CbOprSplitSlices, std::vector<CompliedBlock*>*> CompliedBlock::mapSplitSlices;
std::unordered_map<CbOprMatchSlicesOneCb, std::vector<CompliedBlock*>*> CompliedBlock::mapMatchSlicesOneCb;
std::unordered_map<CbOprMatchSlicesTwoCbs, MatchedSubCbs*> CompliedBlock::mapMatchSlicesTwoCbs;
size_t CompliedBlock::numCreation = 0;
size_t CompliedBlock::numCreationHit = 0;
size_t CompliedBlock::numGetSlice = 0;
size_t CompliedBlock::numGetSliceHit = 0;
size_t CompliedBlock::numSplitSlices = 0;
size_t CompliedBlock::numSplitSlicesHit = 0;
size_t CompliedBlock::numMatchSlices1 = 0;
size_t CompliedBlock::numMatchSlices1Hit = 0;
size_t CompliedBlock::numMatchSlices2 = 0;
size_t CompliedBlock::numMatchSlices2Hit = 0;

CompliedBlock* CompliedBlock::create(int** const complianceSet, int const totalRows, int const totalCols, bool maintainComplianceSet) {
    // numCreation++;
    // std::unordered_map<void *, sinc::CompliedBlock*>::iterator itr = mapCreation.find(complianceSet);
    // if (mapCreation.end() == itr) {
    //     CompliedBlock* cb = new CompliedBlock(pool.size(), complianceSet, totalRows, totalCols, maintainComplianceSet);
    //     registerCb(cb);
    //     mapCreation.emplace(complianceSet, cb);
    //     return cb;
    // } else {
    //     numCreationHit++;
    //     return itr->second;
    // }
    CompliedBlock* cb = new CompliedBlock(pool.size(), complianceSet, totalRows, totalCols, maintainComplianceSet);
    registerCb(cb);
    return cb;
}

CompliedBlock* CompliedBlock::create(
    int** _complianceSet, int const _totalRows, int const _totalCols, IntTable* _indices,
    bool _maintainComplianceSet, bool _maintainIndices
) {
    numCreation++;
    std::unordered_map<void *, sinc::CompliedBlock*>::iterator itr = mapCreation.find(_complianceSet);
    if (mapCreation.end() == itr) {
        CompliedBlock* cb = new CompliedBlock(pool.size(), _complianceSet, _totalRows, _totalCols, _indices, _maintainComplianceSet, _maintainIndices);
        registerCb(cb);
        mapCreation.emplace(_complianceSet, cb);
        return cb;
    } else {
        numCreationHit++;
        return itr->second;
    }
}

CompliedBlock* CompliedBlock::getSlice(const CompliedBlock& cb, int const col, int const val) {
    numGetSlice++;
    CbOprGetSlice opr(cb.id, col, val);
    std::unordered_map<sinc::CbOprGetSlice, sinc::CompliedBlock*>::iterator itr = mapGetSlice.find(opr);
    if (mapGetSlice.end() == itr) {
        IntTable::sliceType* slice = cb.getIndices().getSlice(col, val);
        if (nullptr != slice) { // assertion: must be non-empty
            CompliedBlock* new_cb = new CompliedBlock(pool.size(), toArray<int*>(*slice), slice->size(), cb.totalCols, true);
            registerCb(new_cb);
            mapGetSlice.emplace(opr, new_cb);
            IntTable::releaseSlice(slice);
            return new_cb;
        } else {
            IntTable::releaseSlice(slice);
            return nullptr;
        }
    } else {
        numGetSliceHit++;
        return itr->second;
    }
}

std::vector<CompliedBlock*> const& CompliedBlock::splitSlices(const CompliedBlock& cb, int const col) {
    numSplitSlices++;
    CbOprSplitSlices opr(cb.id, col);
    std::unordered_map<sinc::CbOprSplitSlices, std::vector<sinc::CompliedBlock*>*>::iterator itr = mapSplitSlices.find(opr);
    if (mapSplitSlices.end() == itr) {
        IntTable::slicesType* slices = cb.getIndices().splitSlices(col);
        std::vector<CompliedBlock*>* cbs = new std::vector<CompliedBlock*>();
        cbs->reserve(slices->size());
        for (IntTable::sliceType* slice: *slices) {
            CompliedBlock* new_cb = new CompliedBlock(pool.size(), toArray<int*>(*slice), slice->size(), cb.totalCols, true);
            registerCb(new_cb);
            cbs->push_back(new_cb);
        }
        mapSplitSlices.emplace(opr, cbs);
        IntTable::releaseSlices(slices);
        return *cbs;
    } else {
        numSplitSlicesHit++;
        return *(itr->second);
    }
}

const MatchedSubCbs* CompliedBlock::matchSlices(
    const CompliedBlock& cb1, int const col1, const CompliedBlock& cb2, int const col2
) {
    /* Map symmetric operations into one entry. The CB with smaller ID goes the first */
    int _id1, _col1, _id2, _col2;
    if (cb1.id <= cb2.id) {
        _id1 = cb1.id;
        _col1 = col1;
        _id2 = cb2.id;
        _col2 = col2;
    } else {
        _id1 = cb2.id;
        _col1 = col2;
        _id2 = cb1.id;
        _col2 = col1;
    }
    numMatchSlices2++;
    CbOprMatchSlicesTwoCbs opr(_id1, _col1, _id2, _col2);
    std::unordered_map<sinc::CbOprMatchSlicesTwoCbs, sinc::MatchedSubCbs*>::iterator itr = mapMatchSlicesTwoCbs.find(opr);
    if (mapMatchSlicesTwoCbs.end() == itr) {
        MatchedSubTables* slices;
        int arity1;
        int arity2;
        if (cb1.id <= cb2.id) {
            slices = IntTable::matchSlices(cb1.getIndices(), col1, cb2.getIndices(), col2);
            arity1 = cb1.totalCols;
            arity2 = cb2.totalCols;
        } else {
            slices = IntTable::matchSlices(cb2.getIndices(), col2, cb1.getIndices(), col1);
            arity1 = cb2.totalCols;
            arity2 = cb1.totalCols;
        }
        int const num_slices = slices->slices1->size();
        if (0 == num_slices) {
            mapMatchSlicesTwoCbs.emplace(opr, nullptr);
            delete slices;
            return nullptr;
        }
        MatchedSubCbs* sub_cbs = new MatchedSubCbs();
        sub_cbs->cbs1.reserve(num_slices);
        sub_cbs->cbs2.reserve(num_slices);
        for (int i = 0; i < num_slices; i++) {
            IntTable::sliceType& slice1 = *(*(slices->slices1))[i];
            CompliedBlock* new_cb1 = new CompliedBlock(
                pool.size(), toArray<int*>(slice1), slice1.size(), arity1, true
            );
            registerCb(new_cb1);
            sub_cbs->cbs1.push_back(new_cb1);

            IntTable::sliceType& slice2 = *(*(slices->slices2))[i];
            CompliedBlock* new_cb2 = new CompliedBlock(
                pool.size(), toArray<int*>(slice2), slice2.size(), arity2, true
            );
            registerCb(new_cb2);
            sub_cbs->cbs2.push_back(new_cb2);
        }
        mapMatchSlicesTwoCbs.emplace(opr, sub_cbs);
        delete slices;
        return sub_cbs;
    } else {
        numMatchSlices2Hit++;
        return itr->second;
    }
}

const std::vector<CompliedBlock*>* CompliedBlock::matchSlices(const CompliedBlock& cb, int const col1, int const col2) {
    int _col1, _col2;
    if (col1 <= col2) {
        _col1 = col1;
        _col2 = col2;
    } else {
        _col1 = col2;
        _col2 = col1;
    }
    numMatchSlices1++;
    CbOprMatchSlicesOneCb opr(cb.id, _col1, _col2);
    std::unordered_map<sinc::CbOprMatchSlicesOneCb, std::vector<sinc::CompliedBlock*>*>::iterator itr = mapMatchSlicesOneCb.find(opr);
    if (mapMatchSlicesOneCb.end() == itr) {
        IntTable::slicesType* slices = cb.getIndices().matchSlices(col1, col2);
        if (slices->empty()) {
            mapMatchSlicesOneCb.emplace(opr, nullptr);
            IntTable::releaseSlices(slices);
            return nullptr;
        }
        std::vector<CompliedBlock*>* sub_cbs = new std::vector<CompliedBlock*>();
        sub_cbs->reserve(slices->size());
        for (IntTable::sliceType* const& slice: *slices) {
            CompliedBlock* new_cb = new CompliedBlock(pool.size(), toArray<int*>(*slice), slice->size(), cb.totalCols, true);
            registerCb(new_cb);
            sub_cbs->push_back(new_cb);
        }
        mapMatchSlicesOneCb.emplace(opr, sub_cbs);
        IntTable::releaseSlices(slices);
        return sub_cbs;
    } else {
        numMatchSlices1Hit++;
        return itr->second;
    }
}

void CompliedBlock::reserveMemSpace(SimpleKb const& kb) {
    int num_relations = kb.totalRelations();
    mapCreation.reserve(num_relations);
    std::vector<SimpleRelation*> const& relations = *(kb.getRelations());
    int est_get_slice_size = 0;
    int est_split_slices_size = 0;
    int est_match_slices_one_cb_size = 0;
    int est_match_slices_two_cbs_size = 0;
    for (int rel_idx = 0; rel_idx < num_relations; rel_idx++) {
        SimpleRelation const& relation = *(relations[rel_idx]);
        int arity = relation.getTotalCols();
        std::vector<int>** promising_constants = kb.getPromisingConstants(rel_idx);
        for (int col_idx = 0; col_idx < arity; col_idx++) {
            est_get_slice_size += promising_constants[col_idx]->size();
        }
        // est_split_slices_size += arity;
        est_match_slices_one_cb_size += arity * arity;
        est_match_slices_two_cbs_size += arity;
    }
    est_get_slice_size *= num_relations;
    est_split_slices_size = kb.totalConstants();
    // est_match_slices_one_cb_size *= num_relations;
    est_match_slices_two_cbs_size *= est_match_slices_two_cbs_size;
    int est_pool_size = num_relations + est_get_slice_size + est_split_slices_size + est_match_slices_one_cb_size +
        est_match_slices_two_cbs_size;
    mapGetSlice.reserve(est_get_slice_size);
    mapSplitSlices.reserve(est_split_slices_size);
    mapMatchSlicesOneCb.reserve(est_match_slices_one_cb_size);
    mapMatchSlicesTwoCbs.reserve(est_match_slices_two_cbs_size);
    pool.reserve(est_pool_size);
}

void CompliedBlock::clearPool() {
    for (CompliedBlock* const& cbp: pool) {
        delete cbp;
    }
    pool.clear();
    mapCreation.clear();
    mapGetSlice.clear();
    for (std::pair<const CbOprSplitSlices, std::vector<CompliedBlock*>*> const& kv: mapSplitSlices) {
        delete kv.second;
    }
    mapSplitSlices.clear();
    for (std::pair<const CbOprMatchSlicesOneCb, std::vector<CompliedBlock*>*> const& kv: mapMatchSlicesOneCb) {
        if (nullptr != kv.second) {
            delete kv.second;
        }
    }
    mapMatchSlicesOneCb.clear();
    for (std::pair<const CbOprMatchSlicesTwoCbs, MatchedSubCbs*> const& kv: mapMatchSlicesTwoCbs) {
        if (nullptr != kv.second) {
            delete kv.second;
        }
    }
    mapMatchSlicesTwoCbs.clear();
}

size_t CompliedBlock::totalNumCbs() {
    return pool.size();
}

size_t CompliedBlock::getNumCreation() {
    return numCreation;
}

size_t CompliedBlock::getNumCreationHit() {
    return numCreationHit;
}

size_t CompliedBlock::getNumCreationIndices() {
    return mapCreation.size();
}

size_t CompliedBlock::getNumGetSlice() {
    return numGetSlice;
}

size_t CompliedBlock::getNumGetSliceHit() {
    return numGetSliceHit;
}

size_t CompliedBlock::getNumGetSliceIndices() {
    return mapGetSlice.size();
}

size_t CompliedBlock::getNumSplitSlices() {
    return numSplitSlices;
}

size_t CompliedBlock::getNumSplitSlicesHit() {
    return numSplitSlicesHit;
}

size_t CompliedBlock::getNumSplitSlicesIndices() {
    return mapSplitSlices.size();
}

size_t CompliedBlock::getNumMatchSlices1() {
    return numMatchSlices1;
}

size_t CompliedBlock::getNumMatchSlices1Hit() {
    return numMatchSlices1Hit;
}

size_t CompliedBlock::getNumMatchSlices1Indices() {
    return mapMatchSlicesOneCb.size();
}

size_t CompliedBlock::getNumMatchSlices2() {
    return numMatchSlices2;
}

size_t CompliedBlock::getNumMatchSlices2Hit() {
    return numMatchSlices2Hit;
}

size_t CompliedBlock::getNumMatchSlices2Indices() {
    return mapMatchSlicesTwoCbs.size();
}

size_t CompliedBlock::totalCbMemoryCost() {
    size_t size = sizeof(pool) + sizeof(CompliedBlock*) * pool.capacity();
    for (CompliedBlock* const& cb: pool) {
        size += cb->memoryCost();
    }
    size += sizeOfUnorderedMap(
        mapCreation.bucket_count(), mapCreation.max_load_factor(), sizeof(std::pair<void*, CompliedBlock*>), sizeof(mapCreation)
    );
    size += sizeOfUnorderedMap(
        mapGetSlice.bucket_count(), mapGetSlice.max_load_factor(), sizeof(std::pair<CbOprGetSlice, CompliedBlock*>), sizeof(mapGetSlice)
    );
    size += sizeOfUnorderedMap(
        mapSplitSlices.bucket_count(), mapSplitSlices.max_load_factor(),
        sizeof(std::pair<CbOprSplitSlices, std::vector<CompliedBlock*>*>), sizeof(mapSplitSlices)
    );
    for (std::pair<CbOprSplitSlices, std::vector<CompliedBlock*>*> const& kv: mapSplitSlices) {
        size += sizeof(std::vector<CompliedBlock*>) + sizeof(CompliedBlock*) * kv.second->capacity();
    }
    size += sizeOfUnorderedMap(
        mapMatchSlicesOneCb.bucket_count(), mapMatchSlicesOneCb.max_load_factor(),
        sizeof(std::pair<CbOprMatchSlicesOneCb, std::vector<CompliedBlock*>*>), sizeof(mapMatchSlicesOneCb)
    );
    for (std::pair<CbOprMatchSlicesOneCb, std::vector<CompliedBlock*>*> const& kv: mapMatchSlicesOneCb) {
        if (nullptr != kv.second) {
            size += sizeof(std::vector<CompliedBlock*>) + sizeof(CompliedBlock*) * kv.second->capacity();
        }
    }
    size += sizeOfUnorderedMap(
        mapMatchSlicesTwoCbs.bucket_count(), mapMatchSlicesTwoCbs.max_load_factor(),
        sizeof(std::pair<CbOprMatchSlicesTwoCbs, MatchedSubCbs*>), sizeof(mapMatchSlicesTwoCbs)
    );
    for (std::pair<CbOprMatchSlicesTwoCbs, MatchedSubCbs*> const& kv: mapMatchSlicesTwoCbs) {
        if (nullptr != kv.second) {
            size += kv.second->calcMemoryCost();
        }
    }
    return size;
}

CompliedBlock::~CompliedBlock() {
    if (mainTainComplianceSet) {
        delete[] complianceSet;
    }
    if (maintainIndices) {
        delete indices;
    }
}

void CompliedBlock::buildIndices() {
    if (nullptr == indices) {
        indices = new IntTable(complianceSet, totalRows, totalCols);
        maintainIndices = true;
    }
}

int CompliedBlock::getId() const {
    return id;
}

int* const* CompliedBlock::getComplianceSet() const {
    return complianceSet;
}

const IntTable& CompliedBlock::getIndices() const {
    return *indices;
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
        size += sizeof(int*) * totalRows + sizeof(int);
    }
    if (maintainIndices) {
        size += indices->memoryCost();
    }
    return size;
}

void CompliedBlock::showComplianceSet() const {
    std::cout << "Compliance Set:\n";
    for (int i = 0; i < totalRows; i++) {
        for (int j = 0; j < totalCols; j++) {
            std::cout << complianceSet[i][j] << ',';
        }
        std::cout << std::endl;
    }
}

void CompliedBlock::showAll() const {
    std::cout << "Compliance Set:\n";
    for (int i = 0; i < totalRows; i++) {
        for (int j = 0; j < totalCols; j++) {
            std::cout << complianceSet[i][j] << ',';
        }
        std::cout << std::endl;
    }
    std::cout << "Indices:\n";
    indices->showRows();
}

void CompliedBlock::registerCb(CompliedBlock* cb) {
    pool.push_back(cb);
}

CompliedBlock::CompliedBlock(
    int const _id, int** _complianceSet, int const _totalRows, int const _totalCols, bool _maintainComplianceSet
): id(_id), complianceSet(_complianceSet), totalRows(_totalRows), totalCols(_totalCols), indices(nullptr),
    mainTainComplianceSet(_maintainComplianceSet), maintainIndices(false) {}

CompliedBlock::CompliedBlock(int const _id, int** _complianceSet, int const _totalRows, int const _totalCols, IntTable* _indices,
    bool _maintainComplianceSet, bool _maintainIndices
) : id(_id), complianceSet(_complianceSet), totalRows(_totalRows), totalCols(_totalCols), indices(_indices),
    mainTainComplianceSet(_maintainComplianceSet), maintainIndices(_maintainIndices && nullptr != _indices) {}

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
        os, "%10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s\n",
        "#CB", "CB", "CB(%)", "CE", "CE(%)", "FpC", "FpC(%)", "TbM", "TbM(%)", "Eval", "Eval(%)"
    );
    rusage usage;
    if (0 != getrusage(RUSAGE_SELF, &usage)) {
        std::cerr << "Failed to get `rusage`" << std::endl;
        usage.ru_maxrss = 1024 * 1024 * 1024;   // set to 1T
    }
    printf(
        os, "%10d %10s %10.2f %10s %10.2f %10s %10.2f %10s %10.2f %10s %10.2f\n\n",
        maxCbPoolSize, formatMemorySize(cbMemCost).c_str(), ((double) cbMemCost) / usage.ru_maxrss * 100.0,
        formatMemorySize(cacheEntryMemCost).c_str(), ((double) cacheEntryMemCost) / usage.ru_maxrss * 100.0,
        formatMemorySize(fingerprintCacheMemCost).c_str(), ((double) fingerprintCacheMemCost) / usage.ru_maxrss * 100.0,
        formatMemorySize(tabuMapMemCost).c_str(), ((double) tabuMapMemCost) / usage.ru_maxrss * 100.0,
        formatMemorySize(maxEvalMemCost).c_str(), ((double) maxEvalMemCost) / usage.ru_maxrss * 100.0
    );

    os << "--- CB Statistics ---\n";
    printf(
        os, "# %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s\n",
        "CB", "Crt", "Crt(Hit)", "Crt(Hit%)", "Get", "Get(Hit)", "Get(Hit%)", "Spl", "Spl(Hit)", "Spl(Hit%)", "Mt1", "Mt1(Hit)", "Mt1(Hit%)", "Mt2", "Mt2(Hit)", "Mt2(Hit%)", "Total.Opr", "Total.Hit", "Total.Hit%"
    );
    size_t total_opr = CompliedBlock::getNumCreation() + CompliedBlock::getNumGetSlice() + CompliedBlock::getNumSplitSlices() + CompliedBlock::getNumMatchSlices1() + CompliedBlock::getNumMatchSlices2();
    size_t total_hit = CompliedBlock::getNumCreationHit() + CompliedBlock::getNumGetSliceHit() + CompliedBlock::getNumSplitSlicesHit() + CompliedBlock::getNumMatchSlices1Hit() + CompliedBlock::getNumMatchSlices2Hit();
    printf(
        os, "  %10d %10d %10d %10.2f %10d %10d %10.2f %10d %10d %10.2f %10d %10d %10.2f %10d %10d %10.2f %10d %10d %10.2f\n\n",
        maxCbPoolSize,
        CompliedBlock::getNumCreation(), CompliedBlock::getNumCreationHit(), ((double)CompliedBlock::getNumCreationHit() / CompliedBlock::getNumCreation()) * 100.0,
        CompliedBlock::getNumGetSlice(), CompliedBlock::getNumGetSliceHit(), ((double)CompliedBlock::getNumGetSliceHit() / CompliedBlock::getNumGetSlice()) * 100.0,
        CompliedBlock::getNumSplitSlices(), CompliedBlock::getNumSplitSlicesHit(), ((double)CompliedBlock::getNumSplitSlicesHit() / CompliedBlock::getNumSplitSlices()) * 100.0,
        CompliedBlock::getNumMatchSlices1(), CompliedBlock::getNumMatchSlices1Hit(), ((double)CompliedBlock::getNumMatchSlices1Hit() / CompliedBlock::getNumMatchSlices1()) * 100.0,
        CompliedBlock::getNumMatchSlices2(), CompliedBlock::getNumMatchSlices2Hit(), ((double)CompliedBlock::getNumMatchSlices2Hit() / CompliedBlock::getNumMatchSlices2()) * 100.0,
        total_opr, total_hit, ((double)total_hit / total_opr) * 100.0
    );

    printf(
        os, "# %10s %10s %10s %10s %10s %10s\n",
        "Crt.Idx", "Get.Idx", "Spl.Idx", "Mt1.Idx", "Mt2.Idx", "Total.Idx"
    );
    printf(
        os, "  %10d %10d %10d %10d %10d %10d\n\n",
        CompliedBlock::getNumCreationIndices(),
        CompliedBlock::getNumGetSliceIndices(),
        CompliedBlock::getNumSplitSlicesIndices(),
        CompliedBlock::getNumMatchSlices1Indices(),
        CompliedBlock::getNumMatchSlices2Indices(),
        maxCbPoolIdxSize
    );

    os << "--- Cache Statistics ---\n";
    printf(
        os, "# %10s %10s %10s %10s %10s %10s %10s %10s\n",
        "E+.Avg", "T.Avg", "E.Avg", "E+.Max", "T.Max", "E.Max", "Avg.CF(E)", "Rules"
    );
    printf(
        os, "  %10.2f %10.2f %10.2f %10d %10d %10d %10.4f %10d\n\n",
        ((double) posCacheEntriesTotal) / totalGeneratedRules,
        ((double) entCacheEntriesTotal) / totalGeneratedRules,
        ((double) allCacheEntriesTotal) / totalGeneratedRules,
        posCacheEntriesMax, entCacheEntriesMax, allCacheEntriesMax,
        ((double) totalCacheFragmentsInAllCache) / totalGeneratedRules,
        totalGeneratedRules
    );

    os << "--- Eval Statistics ---\n" << "Max GV Bindings: " << max_gv_bindings << std::endl;
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
 * CacheFragment
 */
using sinc::CacheFragment;

CacheFragment::CacheFragment(IntTable* const firstRelation, int const relationSymbol) {
    entries = new entriesType();

    partAssignedRule.emplace_back(relationSymbol, firstRelation->getTotalCols());
    entryType* first_entry = new entryType();
    CompliedBlock* cb = CompliedBlock::create(
        firstRelation->getAllRows(), firstRelation->getTotalRows(), firstRelation->getTotalCols(), firstRelation, false, false
    );
    first_entry->push_back(cb);
    entries->push_back(first_entry);
}

CacheFragment::CacheFragment(CompliedBlock* const firstCb, int const relationSymbol) {
    entries = new entriesType();

    partAssignedRule.emplace_back(relationSymbol, firstCb->getTotalCols());
    entryType* first_entry = new entryType();
    first_entry->push_back(firstCb);
    entries->push_back(first_entry);
}

CacheFragment::CacheFragment(int const relationSymbol, int const arity) {
    entries = new entriesType();
    partAssignedRule.emplace_back(relationSymbol, arity);
}

CacheFragment::CacheFragment(const CacheFragment& another) : partAssignedRule(another.partAssignedRule),
    entries(new entriesType()), varInfoList(another.varInfoList) 
{
    entries->reserve(another.entries->size());
    for (entryType* const& entry: *(another.entries)) {
        entries->push_back(new entryType(*entry));
    }
}

CacheFragment::~CacheFragment() {
    releaseEntries();
}

void CacheFragment::updateCase1a(int const tabIdx, int const colIdx, int const vid) {
    partAssignedRule[tabIdx].setArg(colIdx, ARG_VARIABLE(vid));
    if (vid < varInfoList.size() && !varInfoList[vid].isEmpty()) {
        /* Filter the two columns */
        VarInfo& var_info = varInfoList[vid];
        if (var_info.isPlv) {
            /* Split by the two columns */
            var_info.isPlv = false;
            splitCacheEntries(var_info.tabIdx, var_info.colIdx, tabIdx, colIdx);
        } else {
            /* Match the new column to the original */
            matchCacheEntries(var_info.tabIdx, var_info.colIdx, tabIdx, colIdx);
        }
    } else {
        /* Record as a PLV */
        addVarInfo(vid, tabIdx, colIdx, true);
    }
}

void CacheFragment::updateCase1b(IntTable* const newRelation, int const relationSymbol, int const colIdx, int const vid) {
    Predicate& new_pred = partAssignedRule.emplace_back(relationSymbol, newRelation->getTotalCols());
    new_pred.setArg(colIdx, ARG_VARIABLE(vid));
    VarInfo& var_info = varInfoList[vid];    // Assertion: this shall NOT be empty

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

void CacheFragment::updateCase1c(CacheFragment const& fragment, int const tabIdx, int const colIdx, int const vid) {
    /* Merge PAR */
    int original_tabs = partAssignedRule.size();
    for (Predicate const& predicate: fragment.partAssignedRule) {
        partAssignedRule.push_back(predicate);
    }
    partAssignedRule[original_tabs + tabIdx].setArg(colIdx, ARG_VARIABLE(vid));

    /* Merge LV info */
    for (int i = fragment.varInfoList.size() - 1; i >= 0; i--) {
        VarInfo const& var_info = fragment.varInfoList[i];
        if (!var_info.isEmpty() && (i >= this->varInfoList.size() || this->varInfoList[i].isEmpty())) {
            addVarInfo(i, original_tabs + var_info.tabIdx, var_info.colIdx, var_info.isPlv);
        }
    }

    /* Merge entries */
    const2EntriesMapType* merging_frag_const_2_entries_map = calcConst2EntriesMap(
        fragment.getEntries(), tabIdx, colIdx, fragment.getPartAssignedRule()[tabIdx].getArity()
    );
    VarInfo& var_info = varInfoList[vid];    // Assertion: this shall NOT be empty
    if (var_info.isPlv) {
        var_info.isPlv = false;
        const2EntriesMapType* base_frag_const_2_entries_map = calcConst2EntriesMap(
            *entries, var_info.tabIdx, var_info.colIdx, partAssignedRule[var_info.tabIdx].getArity()
        );
        mergeFragmentEntries(*base_frag_const_2_entries_map, *merging_frag_const_2_entries_map);
        releaseConst2EntryMap(base_frag_const_2_entries_map);
    } else {
        mergeFragmentEntries(*entries, var_info.tabIdx, var_info.colIdx, *merging_frag_const_2_entries_map);
    }
    releaseConst2EntryMap(merging_frag_const_2_entries_map);
}

void CacheFragment::updateCase2a(int const tabIdx1, int const colIdx1, int const tabIdx2, int const colIdx2, int const newVid) {
    /* Modify PAR */
    addVarInfo(newVid, tabIdx1, colIdx1, false);
    int var_arg = ARG_VARIABLE(newVid);
    partAssignedRule[tabIdx1].setArg(colIdx1, var_arg);
    partAssignedRule[tabIdx2].setArg(colIdx2, var_arg);

    /* Modify cache entries */
    splitCacheEntries(tabIdx1, colIdx1, tabIdx2, colIdx2);
}

void CacheFragment::updateCase2b(
    IntTable* const newRelation, int const relationSymbol, int const colIdx1, int const tabIdx2, int const colIdx2, int const newVid
) {
    /* Modify PAR */
    addVarInfo(newVid, tabIdx2, colIdx2, false);
    int var_arg = ARG_VARIABLE(newVid);
    partAssignedRule[tabIdx2].setArg(colIdx2, var_arg);
    Predicate& new_pred = partAssignedRule.emplace_back(relationSymbol, newRelation->getTotalCols());
    new_pred.setArg(colIdx1, var_arg);

    /* Modify cache entries */
    splitCacheEntries(tabIdx2, colIdx2, newRelation, colIdx1);
}

void CacheFragment::updateCase2c(
    int const tabIdx, int const colIdx, CacheFragment const& fragment, int const tabIdx2, int const colIdx2, int const newVid
) {
    /* Merge PAR */
    int original_tabs = partAssignedRule.size();
    for (Predicate const& predicate: fragment.partAssignedRule) {
        partAssignedRule.push_back(predicate);
    }
    int var_arg = ARG_VARIABLE(newVid);
    partAssignedRule[tabIdx].setArg(colIdx, var_arg);
    partAssignedRule[original_tabs + tabIdx2].setArg(colIdx2, var_arg);

    /* Merge LV info */
    addVarInfo(newVid, tabIdx, colIdx, false);
    for (int i = fragment.varInfoList.size() - 1; i >= 0; i--) {
        VarInfo const& var_info = fragment.varInfoList[i];
        if (!var_info.isEmpty() && (i >= this->varInfoList.size() || this->varInfoList[i].isEmpty())) {
            addVarInfo(i, original_tabs + var_info.tabIdx, var_info.colIdx, var_info.isPlv);
        }
    }

    /* Merge entries */
    const2EntriesMapType* merging_frag_const_2_entries_map = calcConst2EntriesMap(
        fragment.getEntries(), tabIdx2, colIdx2, fragment.getPartAssignedRule()[tabIdx2].getArity()
    );
    const2EntriesMapType* base_frag_const_2_entries_map = calcConst2EntriesMap(
        *entries, tabIdx, colIdx, partAssignedRule[tabIdx].getArity()
    );
    mergeFragmentEntries(*base_frag_const_2_entries_map, *merging_frag_const_2_entries_map);
    releaseConst2EntryMap(merging_frag_const_2_entries_map);
    releaseConst2EntryMap(base_frag_const_2_entries_map);
}

void CacheFragment::updateCase3(int const tabIdx, int const colIdx, int const constant) {
    /* Modify PAR */
    partAssignedRule[tabIdx].setArg(colIdx, ARG_CONSTANT(constant));

    /* Modify cache entries */
    assignCacheEntries(tabIdx, colIdx, constant);
}

void CacheFragment::buildIndices() {
    for (entryType* const& entry: *entries) {
        for (CompliedBlock* const& cb: *entry) {
            cb->buildIndices();
        }
    }
}

bool CacheFragment::hasLv(int const vid) const {
    return (vid < varInfoList.size()) && !varInfoList[vid].isEmpty();
}

int CacheFragment::countCombinations(std::vector<int> const& vids) const {
    std::vector<VarInfo> lvs;
    std::vector<int> plv_col_index_lists[partAssignedRule.size()];    // Table index is the index of the array
    std::vector<int> tab_idxs_with_plvs;
    lvs.reserve(vids.size());
    tab_idxs_with_plvs.reserve(vids.size());
    for (int const& vid: vids) {
        VarInfo const& var_info = varInfoList[vid];    // This shall NOT be empty
        if (var_info.isPlv) {
            if (plv_col_index_lists[var_info.tabIdx].empty()) {
                tab_idxs_with_plvs.push_back(var_info.tabIdx);
                plv_col_index_lists[var_info.tabIdx].reserve(vids.size());
            }
            plv_col_index_lists[var_info.tabIdx].push_back(var_info.colIdx);
        } else {
            lvs.push_back(var_info);
        }
    }
    _evaluation_memory_cost += sizeof(lvs) + sizeof(VarInfo) * lvs.capacity() + sizeof(plv_col_index_lists) + 
        sizeof(tab_idxs_with_plvs ) + sizeof(int) * tab_idxs_with_plvs.capacity();
    for (int i = 0; i < partAssignedRule.size(); i++) {
        _evaluation_memory_cost += sizeof(int) * plv_col_index_lists[i].capacity();
    }

    int const total_plvs = vids.size() - lvs.size();
    int total_unique_bindings = 0;
    if (0 == total_plvs) {
        /* No PLV. Just count all combinations of LVs */
        std::unordered_set<Record> lv_bindings;
        lv_bindings.reserve(entries->size());
        for (entryType* const& cache_entry: *entries) {
            int* binding = new int[lvs.size()];
            for (int i = 0; i < lvs.size(); i++) {
                VarInfo const& lv_info = lvs[i];
                binding[i] = (*cache_entry)[lv_info.tabIdx]->getComplianceSet()[0][lv_info.colIdx];
            }
            if (!lv_bindings.emplace(binding, lvs.size()).second) {
                delete[] binding;
            }
        }
        total_unique_bindings = lv_bindings.size();
        _evaluation_memory_cost += sizeOfUnorderedSet(
            lv_bindings.bucket_count(), lv_bindings.max_load_factor(), sizeof(Record), sizeof(lv_bindings)
        ) + (sizeof(int) * lvs.size() + sizeof(int)) * total_unique_bindings;
        for (Record const& r: lv_bindings) {
            delete[] r.getArgs();
        }
    } else  {
        std::unordered_map<Record, std::unordered_set<Record>*> lv_bindings_2_plv_bindings;
        lv_bindings_2_plv_bindings.reserve(entries->size());
        std::unordered_set<Record> plv_bindings_within_tab_sets[tab_idxs_with_plvs.size()];
        _evaluation_memory_cost += sizeof(plv_bindings_within_tab_sets);
        size_t _max_mem_cost_plv_bindings_within_tab_sets = 0;
        for (entryType* const& cache_entry: *entries) {
            /* Find LV binding first */
            int* lv_binding = new int[lvs.size()];
            for (int i = 0; i < lvs.size(); i++) {
                VarInfo const& lv_info = lvs[i];
                lv_binding[i] = (*cache_entry)[lv_info.tabIdx]->getComplianceSet()[0][lv_info.colIdx];
            }

            const Record rec_lv_binding(lv_binding, lvs.size());
            std::unordered_set<Record>* complete_plv_bindings;
            std::unordered_map<Record, std::unordered_set<Record>*>::iterator itr = lv_bindings_2_plv_bindings.find(rec_lv_binding);
            if (lv_bindings_2_plv_bindings.end() == itr) {
                complete_plv_bindings = new std::unordered_set<Record>();
                lv_bindings_2_plv_bindings.emplace(rec_lv_binding, complete_plv_bindings);
            } else {
                delete[] lv_binding;
                complete_plv_bindings = itr->second;
            }

            /* Cartesian product all the PLVs */
            for (int i = 0; i < tab_idxs_with_plvs.size(); i++) {
                int tab_idx = tab_idxs_with_plvs[i];
                std::vector<int>& plv_col_idxs = plv_col_index_lists[tab_idx];
                std::unordered_set<Record>& plv_bindings = plv_bindings_within_tab_sets[i];
                CompliedBlock* cb = (*cache_entry)[tab_idx];
                const int* const* compliance_records = cb->getComplianceSet();
                for (int row_idx = 0; row_idx < cb->getTotalRows(); row_idx++) {
                    const int* cs_record = compliance_records[row_idx];
                    int* plv_binding_within_tab = new int[plv_col_idxs.size()];
                    for (int j = 0; j < plv_col_idxs.size(); j++) {
                        plv_binding_within_tab[j] = cs_record[plv_col_idxs[j]];
                    }
                    if (!plv_bindings.emplace(plv_binding_within_tab, plv_col_idxs.size()).second) {
                        delete[] plv_binding_within_tab;
                    }
                }
            }

            size_t _current_mem_cost_plv_bindings_within_tab_sets = 0;
            if (1 == tab_idxs_with_plvs.size()) {
                /* No need to perform Cartesian product */
                std::unordered_set<Record>& set = plv_bindings_within_tab_sets[0];
                _current_mem_cost_plv_bindings_within_tab_sets += sizeOfUnorderedSet(
                    set.bucket_count(), set.max_load_factor(), sizeof(Record), sizeof(set)
                ) + (sizeof(int) * set.begin()->getArity() + sizeof(int)) * set.size();
                for (Record const& r: set) {
                    if (!complete_plv_bindings->insert(r).second) {
                        delete[] r.getArgs();
                    }
                }
                set.clear();
            } else {
                /* Cartesian product required */
                int arg_template[total_plvs]{};
                addCompletePlvBindings(
                    *complete_plv_bindings, plv_bindings_within_tab_sets, arg_template, 0, 0, tab_idxs_with_plvs.size()
                );

                /* Release Cartesian product resources */
                for (int i = 0; i < tab_idxs_with_plvs.size(); i++) {
                    std::unordered_set<Record>& set = plv_bindings_within_tab_sets[i];
                    _current_mem_cost_plv_bindings_within_tab_sets += sizeOfUnorderedSet(
                        set.bucket_count(), set.max_load_factor(), sizeof(Record), sizeof(set)
                    ) + (sizeof(int) * set.begin()->getArity() + sizeof(int)) * set.size();
                    for (Record const& r: set) {
                        delete[] r.getArgs();
                    }
                    set.clear();
                }
            }
            _max_mem_cost_plv_bindings_within_tab_sets = std::max(
                _max_mem_cost_plv_bindings_within_tab_sets, _current_mem_cost_plv_bindings_within_tab_sets
            );
        }
        _evaluation_memory_cost += _max_mem_cost_plv_bindings_within_tab_sets + sizeOfUnorderedMap(
            lv_bindings_2_plv_bindings.bucket_count(), lv_bindings_2_plv_bindings.max_load_factor(),
            sizeof(std::pair<Record, std::unordered_set<Record>*>), sizeof(lv_bindings_2_plv_bindings)
        );
        for (std::pair<const Record, std::unordered_set<Record>*> const& kv: lv_bindings_2_plv_bindings) {
            std::unordered_set<Record>& set = *(kv.second);
            _evaluation_memory_cost += sizeOfUnorderedSet(
                set.bucket_count(), set.max_load_factor(), sizeof(Record), sizeof(set)
            ) + (sizeof(int) * set.begin()->getArity() + sizeof(int)) * set.size();
            total_unique_bindings += set.size();
            delete[] kv.first.getArgs();
            for (Record const& r: set) {
                delete[] r.getArgs();
            }
            delete kv.second;
        }
    }
    return total_unique_bindings;
}

std::unordered_set<sinc::Record>* CacheFragment::enumerateCombinations(std::vector<int> const& vids) const {
    /* Split LVs and PLVs and find the locations of the vars */
    /* Also, group PLVs within tabs */
    std::vector<VarInfo> lvs;
    std::vector<int> lv_template_idxs;
    std::vector<int> plv_col_idx_lists[partAssignedRule.size()];    // Table index is the index of the array
    std::vector<int> plv_2_template_idx_lists[partAssignedRule.size()];
    std::vector<int> tab_idxs_with_plvs;
    lvs.reserve(vids.size());
    lv_template_idxs.reserve(vids.size());
    tab_idxs_with_plvs.reserve(vids.size());
    for (int template_idx = 0; template_idx < vids.size(); template_idx++) {
        VarInfo const& var_info = varInfoList[vids[template_idx]];    // This shall NOT be empty
        if (var_info.isPlv) {
            if (plv_col_idx_lists[var_info.tabIdx].empty()) {
                tab_idxs_with_plvs.push_back(var_info.tabIdx);
                plv_col_idx_lists[var_info.tabIdx].reserve(vids.size());
                plv_2_template_idx_lists[var_info.tabIdx].reserve(vids.size());
            }
            plv_col_idx_lists[var_info.tabIdx].push_back(var_info.colIdx);
            plv_2_template_idx_lists[var_info.tabIdx].push_back(template_idx);
        } else {
            lv_template_idxs.push_back(template_idx);
            lvs.push_back(var_info);
        }
    }

    std::unordered_set<Record>* bindings = new std::unordered_set<Record>();
    bindings->reserve(entries->size());
    if (tab_idxs_with_plvs.empty()) {
        /* No PLV. Just enumerate all combinations of LVs */
        for (entryType* const& cache_entry: *entries) {
            int* binding = new int[vids.size()];
            for (int template_idx = 0; template_idx < lvs.size(); template_idx++) {
                VarInfo const& lv_info = lvs[template_idx];
                binding[template_idx] = (*cache_entry)[lv_info.tabIdx]->getComplianceSet()[0][lv_info.colIdx];
            }
            if (!bindings->emplace(binding, lvs.size()).second) {
                delete[] binding;
            }
        }
    } else  {
        int* binding_with_only_lvs = new int[vids.size()];
        std::unordered_set<Record> plv_bindings_within_tab_sets[tab_idxs_with_plvs.size()];
        std::vector<int>* plv_2_template_idx_within_tab_lists[tab_idxs_with_plvs.size()];
        for (entryType* const& cache_entry: *entries) {
            /* Find LV binding first */
            for (int i = 0; i < lvs.size(); i++) {
                VarInfo const& lv_info = lvs[i];
                binding_with_only_lvs[lv_template_idxs[i]] = (*cache_entry)[lv_info.tabIdx]->getComplianceSet()[0][lv_info.colIdx];
            }

            /* Cartesian product all the PLVs */
            for (int i = 0; i < tab_idxs_with_plvs.size(); i++) {
                int tab_idx = tab_idxs_with_plvs[i];
                std::vector<int>& plv_col_idxs = plv_col_idx_lists[tab_idx];
                std::unordered_set<Record>& plv_bindings = plv_bindings_within_tab_sets[i];
                plv_2_template_idx_within_tab_lists[i] = &(plv_2_template_idx_lists[tab_idx]);
                CompliedBlock* cb = (*cache_entry)[tab_idx];
                const int* const* compliance_records = cb->getComplianceSet();
                for (int row_idx = 0; row_idx < cb->getTotalRows(); row_idx++) {
                    const int* cs_record = compliance_records[row_idx];
                    int* plv_binding_within_tab = new int[plv_col_idxs.size()];
                    for (int j = 0; j < plv_col_idxs.size(); j++) {
                        plv_binding_within_tab[j] = cs_record[plv_col_idxs[j]];
                    }
                    if (!plv_bindings.emplace(plv_binding_within_tab, plv_col_idxs.size()).second) {
                        delete[] plv_binding_within_tab;
                    }
                }
            }
            addPlvBindings2Templates(
                    *bindings, plv_bindings_within_tab_sets, plv_2_template_idx_within_tab_lists, binding_with_only_lvs, 0,
                    tab_idxs_with_plvs.size(), vids.size()
            );

            /* Release Cartesian product resources */
            for (int i = 0; i < tab_idxs_with_plvs.size(); i++) {
                for (Record const& r: plv_bindings_within_tab_sets[i]) {
                    delete[] r.getArgs();
                }
                plv_bindings_within_tab_sets[i].clear();
            }
        }
        delete[] binding_with_only_lvs;
    }
    return bindings;
}

bool CacheFragment::isEmpty() const {
    return entries->empty();
}

void CacheFragment::clear() {
    for (entryType* entry: *entries) {
        delete entry;
    }
    entries->clear();
}

const CacheFragment::entriesType& CacheFragment::getEntries() const {
    return *entries;
}

CacheFragment::entryType* CacheFragment::getEntry(int const idx) const {
    return (*entries)[idx];
}

int CacheFragment::countTableSize(int const tabIdx) const {
    /* Rows in compliance sets are all from original KB, thus the pointer of two rows are equal iff two rows are equal */
    std::unordered_set<const int*> records;
    records.reserve(entries->size());
    for (entryType* entry: *entries) {
        CompliedBlock* cb = (*entry)[tabIdx];
        const int* const* cs = cb->getComplianceSet();
        records.insert(cs, cs + cb->getTotalRows());
    }
    return records.size();
}

int CacheFragment::totalTables() const {
    return partAssignedRule.size();
}

std::vector<sinc::Predicate> const& CacheFragment::getPartAssignedRule() const {
    return partAssignedRule;
}

std::vector<VarInfo> const& CacheFragment::getVarInfoList() const {
    return varInfoList;
}

size_t CacheFragment::getMemoryCost() const {
    size_t size = sizeof(CacheFragment) + sizeof(Predicate) * partAssignedRule.capacity() + sizeof(VarInfo) * varInfoList.capacity();
    size += sizeof(entriesType) + sizeof(entryType*) * entries->capacity() + sizeof(entryType) * entries->size();
    size_t total_capacity = 0;
    for (entryType* entry: *entries) {
        total_capacity += entry->capacity();
    }
    size += total_capacity * sizeof(CompliedBlock*);
    return size;
}

void CacheFragment::showEntry(entryType const& entry) {
    std::cout << "+++\n";
    for (CompliedBlock* const& cb: entry) {
        std::cout << "---\n";
        cb->showComplianceSet();
    }
    // std::cout << "+++" << std::endl;
}

void CacheFragment::showEntries(entriesType const& entries) {
    std::cout << "Entries:\n";
    for (entryType* const& entry: entries) {
        std::cout << "ooo\n";
        showEntry(*entry);
    }
}

void CacheFragment::splitCacheEntries(int const tabIdx1, int const colIdx1, int const tabIdx2, int const colIdx2) {
    entriesType* new_entries = new entriesType();
    if (tabIdx1 == tabIdx2) {
        for (entryType* const& cache_entry: *entries) {
            CompliedBlock& cb = *(*cache_entry)[tabIdx1];
            const std::vector<CompliedBlock*>* slices = CompliedBlock::matchSlices(cb, colIdx1, colIdx2);
            if (nullptr != slices) {
                for (CompliedBlock* const& new_cb: *slices) {
                    entryType* new_entry = new entryType(*cache_entry);
                    (*new_entry)[tabIdx1] = new_cb;
                    new_entries->push_back(new_entry);
                }
            }
        }
    } else {
        for (entryType* const& cache_entry: *entries) {
            CompliedBlock& cb1 = *(*cache_entry)[tabIdx1];
            CompliedBlock& cb2 = *(*cache_entry)[tabIdx2];
            const MatchedSubCbs* slices = CompliedBlock::matchSlices(cb1, colIdx1, cb2, colIdx2);
            const std::vector<CompliedBlock*> *cbs1;
            const std::vector<CompliedBlock*> *cbs2;
            if (cb1.getId() <= cb2.getId()) { // Handle symmetric operations
                cbs1 = &(slices->cbs1);
                cbs2 = &(slices->cbs2);
            } else {
                cbs1 = &(slices->cbs2);
                cbs2 = &(slices->cbs1);
            }
            if (nullptr != slices) {
                for (int i = 0; i < cbs1->size(); i++) {
                    CompliedBlock* new_cb1 = (*cbs1)[i];
                    CompliedBlock* new_cb2 = (*cbs2)[i];
                    entryType* new_entry = new entryType(*cache_entry);
                    (*new_entry)[tabIdx1] = new_cb1;
                    (*new_entry)[tabIdx2] = new_cb2;
                    new_entries->push_back(new_entry);
                }
            }
        }
    }
    releaseEntries();
    entries = new_entries;
}

void CacheFragment::splitCacheEntries(int const tabIdx1, int const colIdx1, IntTable* const newRelation, int const colIdx2) {
    entriesType* new_entries = new entriesType();
    for (entryType* const& cache_entry : *entries) {
        CompliedBlock& cb1 = *(*cache_entry)[tabIdx1];
        CompliedBlock& cb2 = *CompliedBlock::create(
            newRelation->getAllRows(), newRelation->getTotalRows(), newRelation->getTotalCols(), newRelation, false, false
        );
        const MatchedSubCbs* slices = CompliedBlock::matchSlices(cb1, colIdx1, cb2, colIdx2);
        const std::vector<CompliedBlock*> *cbs1;
        const std::vector<CompliedBlock*> *cbs2;
        if (cb1.getId() <= cb2.getId()) { // Handle symmetric operations
            cbs1 = &(slices->cbs1);
            cbs2 = &(slices->cbs2);
        } else {
            cbs1 = &(slices->cbs2);
            cbs2 = &(slices->cbs1);
        }
        if (nullptr != slices) {
            for (int i = 0; i < cbs1->size(); i++) {
                CompliedBlock* new_cb1 = (*cbs1)[i];
                CompliedBlock* new_cb2 = (*cbs2)[i];
                entryType* new_entry = new entryType(*cache_entry);
                (*new_entry)[tabIdx1] = new_cb1;
                new_entry->push_back(new_cb2);
                new_entries->push_back(new_entry);
            }
        }
    }
    releaseEntries();
    entries = new_entries;
}

void CacheFragment::matchCacheEntries(
    int const matchedTabIdx, int const matchedColIdx, int const matchingTabIdx, int const matchingColIdx
) {
    entriesType* new_entries = new entriesType();
    if (matchedTabIdx == matchingTabIdx) {
        for (entryType* const& cache_entry : *entries) {
            CompliedBlock& cb = *(*cache_entry)[matchedTabIdx];
            int const matched_constant = cb.getComplianceSet()[0][matchedColIdx];
            CompliedBlock* new_cb = CompliedBlock::getSlice(cb, matchingColIdx, matched_constant);
            if (nullptr != new_cb) {
                entryType* new_entry = new entryType(*cache_entry);
                (*new_entry)[matchedTabIdx] = new_cb;
                new_entries->push_back(new_entry);
            }
        }
    } else {
        for (entryType* const& cache_entry : *entries) {
            CompliedBlock& matched_cb = *(*cache_entry)[matchedTabIdx];
            CompliedBlock& matching_cb = *(*cache_entry)[matchingTabIdx];
            int const matched_constant = matched_cb.getComplianceSet()[0][matchedColIdx];
            CompliedBlock* new_cb = CompliedBlock::getSlice(matching_cb, matchingColIdx, matched_constant);
            if (nullptr != new_cb) {
                entryType* new_entry = new entryType(*cache_entry);
                (*new_entry)[matchingTabIdx] = new_cb;
                new_entries->push_back(new_entry);
            }
        }
    }
    releaseEntries();
    entries = new_entries;
}

void CacheFragment::matchCacheEntries(
    int const matchedTabIdx, int const matchedColIdx, IntTable* const newRelation, int matchingColIdx
) {
    entriesType* new_entries = new entriesType();
    for (entryType* const& cache_entry : *entries) {
        CompliedBlock& matched_cb = *(*cache_entry)[matchedTabIdx];
        int const matched_constant = matched_cb.getComplianceSet()[0][matchedColIdx];
        CompliedBlock& new_rel_cb = *CompliedBlock::create(
            newRelation->getAllRows(), newRelation->getTotalRows(), newRelation->getTotalCols(), newRelation, false, false
        );
        CompliedBlock* new_cb = CompliedBlock::getSlice(new_rel_cb, matchingColIdx, matched_constant);
        if (nullptr != new_cb) {
            entryType* new_entry = new entryType(*cache_entry);
            new_entry->push_back(new_cb);
            new_entries->push_back(new_entry);
        }
    }
    releaseEntries();
    entries = new_entries;
}

void CacheFragment::assignCacheEntries(int const tabIdx, int const colIdx, int const constant) {
    entriesType* new_entries = new entriesType();
    for (entryType* const& cache_entry : *entries) {
        CompliedBlock& cb = *(*cache_entry)[tabIdx];
        CompliedBlock* new_cb = CompliedBlock::getSlice(cb, colIdx, constant);
        if (nullptr != new_cb) {
            entryType* new_entry = new entryType(*cache_entry);
            (*new_entry)[tabIdx] = new_cb;
            new_entries->push_back(new_entry);
        }
    }
    releaseEntries();
    entries = new_entries;
}

void CacheFragment::addVarInfo(int const vid, int const tabIdx, int const colIdx, bool const isPlv) {
    if (vid < varInfoList.size()) {
        varInfoList[vid].tabIdx = tabIdx;
        varInfoList[vid].colIdx = colIdx;
        varInfoList[vid].isPlv = isPlv;
    } else {
        varInfoList.reserve(vid + 1);
        for (int i = varInfoList.size(); i < vid; i++) {
            varInfoList.emplace_back(-1, -1, false);
        }
        varInfoList.emplace_back(tabIdx, colIdx, isPlv);
    }
}

CacheFragment::const2EntriesMapType* CacheFragment::calcConst2EntriesMap(
    entriesType const& entries, int const tabIdx, int const colIdx, int const arity
) {
    const2EntriesMapType* const_2_entries_map = new const2EntriesMapType();
    for (entryType* const& cache_entry: entries) {
        CompliedBlock& cb = *(*cache_entry)[tabIdx];
        const std::vector<CompliedBlock*>& slices = CompliedBlock::splitSlices(cb, colIdx);
        for (CompliedBlock* const& slice: slices) {
            int const constant = slice->getComplianceSet()[0][colIdx];
            entriesType* entries_of_the_value;
            const2EntriesMapType::iterator itr = const_2_entries_map->find(constant);
            if (itr == const_2_entries_map->end()) {
                entries_of_the_value = new entriesType();
                const_2_entries_map->emplace(constant, entries_of_the_value);
            } else {
                entries_of_the_value = itr->second;
            }
            entryType* new_entry = new entryType(*cache_entry);
            (*new_entry)[tabIdx] = slice;
            entries_of_the_value->push_back(new_entry);
        }
    }
    return const_2_entries_map;
}

void CacheFragment::mergeFragmentEntries(
    const2EntriesMapType const& baseConst2EntriesMap, const2EntriesMapType const& mergingConst2EntriesMap
) {
    entriesType* new_entries = new entriesType();
    for (std::pair<const int, entriesType*> const& base_map_kv: baseConst2EntriesMap) {
        const2EntriesMapType::const_iterator merging_itr = mergingConst2EntriesMap.find(base_map_kv.first);
        if (mergingConst2EntriesMap.end() != merging_itr) {
            for (entryType* base_entry: *(base_map_kv.second)) {
                for (entryType* merging_entry: *(merging_itr->second)) {
                    entryType* new_entry = new entryType(*base_entry);
                    new_entry->reserve(new_entry->size() + merging_entry->size());
                    new_entry->insert(new_entry->end(), merging_entry->begin(), merging_entry->end());
                    new_entries->push_back(new_entry);
                }
            }
        }
    }
    releaseEntries();
    entries = new_entries;
}

void CacheFragment::mergeFragmentEntries(
    entriesType const& baseEntries, int const tabIdx, int const colIdx, const2EntriesMapType const& mergingConst2EntriesMap
) {
    entriesType* new_entries = new entriesType();
    for (entryType* const& base_entry: baseEntries) {
        int const constant = (*base_entry)[tabIdx]->getComplianceSet()[0][colIdx];
        const2EntriesMapType::const_iterator merging_itr = mergingConst2EntriesMap.find(constant);
        if (mergingConst2EntriesMap.end() != merging_itr) {
            for (entryType* merging_entry: *(merging_itr->second)) {
                entryType* new_entry = new entryType(*base_entry);
                new_entry->reserve(new_entries->size() + merging_entry->size());
                new_entry->insert(new_entry->end(), merging_entry->begin(), merging_entry->end());
                new_entries->push_back(new_entry);
            }
        }
    }
    releaseEntries();
    entries = new_entries;
}

void CacheFragment::releaseConst2EntryMap(const2EntriesMapType* map) {
    for (std::pair<const int, entriesType*> const& kv : *map) {
        for (entryType* const& entry: *(kv.second)) {
            delete entry;
        }
        delete kv.second;
    }
    delete map;
}

void CacheFragment::addCompletePlvBindings(
    std::unordered_set<Record>& completeBindings, std::unordered_set<Record>* const plvBindingSets, int* const argTemplate,
    int const bindingSetIdx, int const templateStartIdx, int const numSets
) const {
    std::unordered_set<Record> const& plv_bindings = plvBindingSets[bindingSetIdx];
    std::unordered_set<Record>::const_iterator itr = plv_bindings.begin();
    int binding_length = itr->getArity();   // assertion: at least one element will be in the set
    if (bindingSetIdx == numSets - 1) {
        /* Complete each template and add to the set */
        while (plv_bindings.end() != itr) {
            int* plv_binding = itr->getArgs();
            int* copy_start = argTemplate + templateStartIdx;
            for (int i = 0; i < binding_length; i++) {
                copy_start[i] = plv_binding[i];
            }
            int template_length = templateStartIdx + binding_length;
            int* copied_binding = new int[template_length];
            for (int i = 0; i < template_length; i++) {
                copied_binding[i] = argTemplate[i];
            }
            if (!completeBindings.emplace(copied_binding, template_length).second) {
                delete[] copied_binding;
            }
            itr++;
        }
    } else {
        /* Complete part of the template and move to next recursion */
        while (plv_bindings.end() != itr) {
            int* plv_binding = itr->getArgs();
            int* copy_start = argTemplate + templateStartIdx;
            for (int i = 0; i < binding_length; i++) {
                copy_start[i] = plv_binding[i];
            }
            addCompletePlvBindings(
                    completeBindings, plvBindingSets, argTemplate, bindingSetIdx+1, templateStartIdx+binding_length, numSets
            );
            itr++;
        }
    }
}

void CacheFragment::addPlvBindings2Templates(
    std::unordered_set<Record>& templateSet, std::unordered_set<Record>* const plvBindingSets,
    std::vector<int>** const plv2TemplateIdxLists, int* const argTemplate, int const setIdx, int const numSets, int const templateLength
) const {
    std::unordered_set<Record> const& plv_bindings = plvBindingSets[setIdx];
    std::vector<int> const& plv_2_template_idxs = *(plv2TemplateIdxLists[setIdx]);
    if (setIdx == numSets - 1) {
        /* Finish the last group of PLVs, add to the template set */
        for (Record const& plv_binding: plv_bindings) {
            for (int i = 0; i < plv_binding.getArity(); i++) {
                argTemplate[plv_2_template_idxs[i]] = plv_binding.getArgs()[i];
            }
            int* copied_binding = new int[templateLength];
            for (int i = 0; i < templateLength; i++) {
                copied_binding[i] = argTemplate[i];
            }
            if (!templateSet.emplace(copied_binding, templateLength).second) {
                delete[] copied_binding;
            }
        }
    } else {
        /* Add current binding to the template and move to the next recursion */
        for (Record const& plv_binding: plv_bindings) {
            for (int i = 0; i < plv_binding.getArity(); i++) {
                argTemplate[plv_2_template_idxs[i]] = plv_binding.getArgs()[i];
            }
            addPlvBindings2Templates(templateSet, plvBindingSets, plv2TemplateIdxLists, argTemplate, setIdx + 1, numSets, templateLength);
        }
    }
}

void CacheFragment::releaseEntries() {
    for (entryType* entry: *entries) {
        delete entry;
    }
    delete entries;
    entries = nullptr;
}

/**
 * TabInfo
 */
using sinc::TabInfo;

TabInfo::TabInfo(int const _fragmentIdx, int const _tabIdx) : fragmentIdx(_fragmentIdx), tabIdx(_tabIdx) {}

TabInfo::TabInfo(const TabInfo& another) : fragmentIdx(another.fragmentIdx), tabIdx(another.tabIdx) {}

bool TabInfo::isEmpty() const {
    return -1 == fragmentIdx;
}

TabInfo& TabInfo::operator=(TabInfo&& another) noexcept {
    fragmentIdx = another.fragmentIdx;
    tabIdx = another.tabIdx;
    return *this;
}

TabInfo& TabInfo::operator=(const TabInfo& another) noexcept {
    fragmentIdx = another.fragmentIdx;
    tabIdx = another.tabIdx;
    return *this;
}

bool TabInfo::operator==(const TabInfo &another) const {
    return fragmentIdx == another.fragmentIdx && tabIdx == another.tabIdx;
}

/**
 * CachedRule
 */
using sinc::CachedRule;
size_t CachedRule::cumulatedCacheEntryMemoryCost = 0;

CachedRule::CachedRule(
    int const headPredSymbol, int const arity, fingerprintCacheType& fingerprintCache, tabuMapType& category2TabuSetMap, SimpleKb& _kb,
    std::unordered_set<Record> const* counterexamples
) : Rule(headPredSymbol, arity, fingerprintCache, category2TabuSetMap), kb(_kb)
{
    /* Initialize the E+-cache & T-cache */
    SimpleRelation* head_relation = kb.getRelation(headPredSymbol);
    // SplitRecords* split_records = head_relation->splitByEntailment();
    // std::vector<int*> const& non_entailed_record_vector = *(split_records->nonEntailedRecords);
    // std::vector<int*> const& entailed_record_vector = *(split_records->entailedRecords);
    // if (0 == entailed_record_vector.size()) {
    //     posCache = new CacheFragment(head_relation, headPredSymbol);
    //     entCache = new CacheFragment(headPredSymbol, arity);
    // } else if (0 == non_entailed_record_vector.size()) {
    //     /* No record to entail, E+-cache and T-cache are both empty */
    //     posCache = new CacheFragment(headPredSymbol, arity);
    //     entCache = new CacheFragment(headPredSymbol, arity);
    // } else {
    //     int** non_entailed_records = toArray(non_entailed_record_vector);
    //     int** entailed_records = toArray(entailed_record_vector);
    //     IntTable* non_entailed_record_table = new IntTable(non_entailed_records, non_entailed_record_vector.size(), arity);
    //     IntTable* entailed_record_table = new IntTable(entailed_records, entailed_record_vector.size(), arity);
    //     posCache = new CacheFragment(
    //         CompliedBlock::create(non_entailed_records, non_entailed_record_vector.size(), arity, non_entailed_record_table, true, true), 
    //         headPredSymbol
    //     );
    //     entCache = new CacheFragment(
    //         CompliedBlock::create(entailed_records, entailed_record_vector.size(), arity, entailed_record_table, true, true),
    //         headPredSymbol
    //     );
    // }
    posCache = new CacheFragment(head_relation, headPredSymbol);
    maintainPosCache = true;
    // maintainEntCache = true;

    /* Initialize the E-cache */
    allCache = new std::vector<CacheFragment*>();
    predIdx2AllCacheTableInfo.emplace_back(-1, -1);    // head is not mapped to any fragment
    maintainAllCache = true;

    // /* Initialize the C-cache */
    // int already_ceg = 0;
    // if (nullptr == counterexamples || counterexamples->empty()) {
    //     cegCache = new CacheFragment(headPredSymbol, arity);
    // } else {
    //     already_ceg = counterexamples->size();
    //     int** existing_cegs = new int*[already_ceg];
    //     int i = 0;
    //     for (Record const& record: *counterexamples) {
    //         existing_cegs[i] = record.getArgs();
    //         i++;
    //     }
    //     IntTable* ceg_records = new IntTable(existing_cegs, already_ceg, arity);
    //     cegCache = new CacheFragment(
    //         CompliedBlock::create(existing_cegs, already_ceg, arity, ceg_records, true, true), headPredSymbol
    //     );
    // }
    // maintainCegCache = true;

    /* Initial evaluation */
    // int pos_ent = non_entailed_record_vector.size();
    // int already_ent = entailed_record_vector.size();
    int already_ent = head_relation->totalEntailedRecords();
    int pos_ent = head_relation->getTotalRows() - already_ent;
    double all_ent = pow(kb.totalConstants(), arity);
    eval = Eval(pos_ent, all_ent - already_ent, length);
    // eval = Eval(pos_ent, all_ent - already_ent - already_ceg, length);
    // delete split_records;
}

CachedRule::CachedRule(const CachedRule& another) : Rule(another), kb(another.kb), posCache(another.posCache), maintainPosCache(false),
    // entCache(another.entCache), maintainEntCache(false), 
    allCache(another.allCache), maintainAllCache(false),
    // cegCache(another.cegCache), maintainCegCache(false),
    predIdx2AllCacheTableInfo(another.predIdx2AllCacheTableInfo)
{}

CachedRule::~CachedRule() {
    cumulatedCacheEntryMemoryCost -= getCacheEntryMemoryCost();
    if (maintainPosCache) {
        delete posCache;
    }
    // if (maintainEntCache) {
    //     delete entCache;
    // }
    if (maintainAllCache) {
        for (CacheFragment* const& fragment: *allCache) {
            delete fragment;
        }
        delete allCache;
    }
    // if (maintainCegCache) {
    //     delete cegCache;
    // }
}

CachedRule* CachedRule::clone() const {
    uint64_t time_start = currentTimeInNano();
    CachedRule* rule = new CachedRule(*this);
    rule->copyTime = currentTimeInNano() - time_start;
    return rule;
}

void CachedRule::updateCacheIndices() {
    uint64_t time_start = currentTimeInNano();
    posCache->buildIndices();
    uint64_t time_pos_done = currentTimeInNano();
    // entCache->buildIndices();
    // uint64_t time_ent_done = currentTimeInNano();
    for (CacheFragment* const& fragment: *allCache) {
        fragment->buildIndices();
    }
    uint64_t time_all_done = currentTimeInNano();
    // cegCache->buildIndices();
    // uint64_t time_ceg_done = currentTimeInNano();
    posCacheIndexingTime = time_pos_done - time_start;
    // entCacheIndexingTime = time_ent_done - time_pos_done;
    // allCacheIndexingTime = time_all_done - time_ent_done;
    allCacheIndexingTime = time_all_done - time_pos_done;
    // cegCacheIndexingTime = time_ceg_done - time_all_done;
}

sinc::EvidenceBatch* CachedRule::getEvidenceAndMarkEntailment() {
    EvidenceBatch* evidence_batch = new EvidenceBatch(structure.size());
    for (int i = 0; i < structure.size(); i++) {
        Predicate const& predicate = structure[i];
        evidence_batch->predicateSymbolsInRule[i] = predicate.getPredSymbol();
        evidence_batch->aritiesInRule[i] = predicate.getArity();
    }

    SimpleRelation& target_relation = *(kb.getRelation(getHead().getPredSymbol()));
    for (CacheFragment::entryType* const& cache_entry: posCache->getEntries()) {
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

    std::unordered_set<Record>* head_templates = new std::unordered_set<Record>();
    int* cloned_args = new int[head_arity];
    std::copy(head_pred.getArgs(), head_pred.getArgs() + head_arity, cloned_args);
    if (allCache->empty()) {
        /* If body is empty, create a single empty head template. The execution flow will go to line [a] */
        head_templates->emplace(cloned_args, head_arity); // copy constant in head if any
    } else {
        /* Locate and remove GVs in head */
        std::vector<int> gvids_in_all_cache_fragments[allCache->size()];
        std::vector<std::vector<int>> head_arg_idx_lists[allCache->size()];
        for (int frag_idx = 0; frag_idx < allCache->size(); frag_idx++) {
            gvids_in_all_cache_fragments[frag_idx].reserve(head_arity);
            head_arg_idx_lists[frag_idx].reserve(head_arity);
        }
        for (std::unordered_map<int, std::vector<int>*>::iterator itr = head_only_vid_2_loc_map.begin();
                itr != head_only_vid_2_loc_map.end();) {
            const int& head_vid = itr->first;
            std::vector<int>& head_var_arg_idxs = *(itr->second);
            bool found = false;
            for (int frag_idx = 0; frag_idx < allCache->size(); frag_idx++) {
                if ((*allCache)[frag_idx]->hasLv(head_vid)) {
                    gvids_in_all_cache_fragments[frag_idx].push_back(head_vid);
                    head_arg_idx_lists[frag_idx].push_back(head_var_arg_idxs);
                    found = true;
                    break;
                }
            }
            if (found) {
                delete itr->second;
                itr = head_only_vid_2_loc_map.erase(itr);
            } else {
                itr++;
            }
        }

        /* Enumerate GV bindings in fragments and generate head templates */
        std::vector<int> valid_frag_indices;
        valid_frag_indices.reserve(allCache->size());
        std::unordered_set<Record>* bindings_in_fragments[allCache->size()]{};
        for (int frag_idx = 0; frag_idx < allCache->size(); frag_idx++) {
            if (!gvids_in_all_cache_fragments[frag_idx].empty()) {
                valid_frag_indices.push_back(frag_idx);
                bindings_in_fragments[frag_idx] = (*allCache)[frag_idx]->enumerateCombinations(gvids_in_all_cache_fragments[frag_idx]); // TODO: Directly instantiate bindings to template, so that the following function call can be removed
            }
        }
        generateHeadTemplates(
            *head_templates, bindings_in_fragments, head_arg_idx_lists, cloned_args, valid_frag_indices, head_arity, 0
        );
        for (int frag_idx = 0; frag_idx < allCache->size(); frag_idx++) {
            std::unordered_set<Record>* bindings_in_fragment = bindings_in_fragments[frag_idx];
            if (nullptr != bindings_in_fragment) {
                for (Record const& r: *bindings_in_fragment) {
                    delete[] r.getArgs();
                }
                delete bindings_in_fragment;
            }
        }
        delete[] cloned_args;
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
        delete posCache;
        maintainPosCache = false;
    }
    // if (maintainEntCache) {
    //     delete entCache;
    //     maintainEntCache = false;
    // }
    if (maintainAllCache) {
        for (CacheFragment* const& fragment: *allCache) {
            delete fragment;
        }
        delete allCache;
        maintainAllCache = false;
    }
    // if (maintainCegCache) {
    //     delete cegCache;
    //     maintainCegCache = false;
    // }
}

uint64_t CachedRule::getCopyTime() const {
    return copyTime;
}

uint64_t CachedRule::getPosCacheUpdateTime() const {
    return posCacheUpdateTime;
}

// uint64_t CachedRule::getEntCacheUpdateTime() const {
//     return entCacheUpdateTime;
// }

uint64_t CachedRule::getAllCacheUpdateTime() const {
    return allCacheUpdateTime;
}

// uint64_t CachedRule::getCegCacheUpdateTime() const {
//     return cegCacheUpdateTime;
// }

uint64_t CachedRule::getPosCacheIndexingTime() const {
    return posCacheIndexingTime;
}

// uint64_t CachedRule::getEntCacheIndexingTime() const {
//     return entCacheIndexingTime;
// }

uint64_t CachedRule::getAllCacheIndexingTime() const {
    return allCacheIndexingTime;
}

// uint64_t CachedRule::getCegCacheIndexingTime() const {
//     return cegCacheIndexingTime;
// }

const CacheFragment& CachedRule::getPosCache() const {
    return *posCache;
}

// const CacheFragment& CachedRule::getEntCache() const {
//     return *entCache;
// }

const std::vector<CacheFragment*>& CachedRule::getAllCache() const {
    return *allCache;
}

// const CacheFragment& CachedRule::getCegCache() const {
//     return *cegCache;
// }

size_t CachedRule::getCacheEntryMemoryCost() {
    if (0 != cacheEntryMemoryCost) {
        return cacheEntryMemoryCost;
    }
    if (maintainPosCache) {
        cacheEntryMemoryCost += posCache->getMemoryCost();
    }
    // if (maintainEntCache) {
    //     cacheEntryMemoryCost += entCache->getMemoryCost();
    // }
    if (maintainAllCache) {
        for (CacheFragment* const& cf: *allCache) {
            cacheEntryMemoryCost += cf->getMemoryCost();
        }
    }
    // if (maintainCegCache) {
    //     cacheEntryMemoryCost += cegCache->getMemoryCost();
    // }
    return cacheEntryMemoryCost;
}

size_t CachedRule::addCumulatedCacheEntryMemoryCost(CachedRule* rule) {
    cumulatedCacheEntryMemoryCost += rule->getCacheEntryMemoryCost();
    return cumulatedCacheEntryMemoryCost;
}

size_t CachedRule::getCumulatedCacheEntryMemoryCost() {
    return cumulatedCacheEntryMemoryCost;
}

size_t CachedRule::getEvaluationMemoryCost() const {
    return evaluationMemoryCost;
}

void CachedRule::obtainPosCache() {
    if (!maintainPosCache) {
        posCache = new CacheFragment(*posCache);
        maintainPosCache = true;
    }
}

// void CachedRule::obtainEntCache() {
//     if (!maintainEntCache) {
//         entCache = new CacheFragment(*entCache);
//         maintainEntCache = true;
//     }
// }

void CachedRule::obtainAllCache() {
    if (!maintainAllCache) {
        std::vector<CacheFragment*>* _all_cache = new std::vector<CacheFragment*>();
        _all_cache->reserve(allCache->size());
        for (CacheFragment* const& fragment: *allCache) {
            _all_cache->push_back(new CacheFragment(*fragment));
        }
        allCache = _all_cache;
        maintainAllCache = true;
    }
}

// void CachedRule::obtainCegCache() {
//     if (!maintainCegCache) {
//         cegCache = new CacheFragment(*cegCache);
//         maintainCegCache = true;
//     }
// }

double CachedRule::recordCoverage() {
    int new_pos_ent = 0;
    SimpleRelation const& head_relation = *kb.getRelation(getHead().getPredSymbol());
    std::unordered_set<const void*> used_rows;
    std::unordered_set<const void*> used_cbs;
    used_rows.reserve(head_relation.getTotalRows());
    used_cbs.reserve(posCache->getEntries().size());
    for (CacheFragment::entryType* const& entry: posCache->getEntries()) {
        CompliedBlock const* cb = (*entry)[HEAD_PRED_IDX];
        if (used_cbs.emplace(cb).second) {
            int* const* const rows = cb->getComplianceSet();
            for (int i = 0; i < cb->getTotalRows(); i++) {
                int* const row = rows[i];
                if (used_rows.emplace(row).second && !head_relation.isEntailed(row)) {
                    new_pos_ent++;
                }
            }
        }
    }
    return ((double) new_pos_ent) / kb.getRelation(getHead().getPredSymbol())->getTotalRows();
}

sinc::Eval CachedRule::calculateEval() {
    _evaluation_memory_cost = 0;
    evaluationMemoryCost = 0;
    long rss_begin = getMaxRss();

    /* Find all variables in the head */
    std::unordered_set<int> head_only_lvs;  // For the head only LVs
    int head_uv_cnt = 0;
    Predicate const& head_pred = getHead();
    for (int arg_idx = 0; arg_idx < head_pred.getArity(); arg_idx++) {
        int argument = head_pred.getArg(arg_idx);
        if (ARG_IS_EMPTY(argument)) {
            head_uv_cnt++;
        } else if (ARG_IS_VARIABLE(argument)) {
            head_only_lvs.insert(ARG_DECODE(argument));    // The GVs will be removed later
        }
    }
    _evaluation_memory_cost += sizeOfUnorderedSet(
        head_only_lvs.bucket_count(), head_only_lvs.max_load_factor(), sizeof(int), sizeof(head_only_lvs)
    );

    /* Locate and remove GVs */
    std::vector<int> gvs_in_all_cache_fragments[allCache->size()];
    for (std::unordered_set<int>::iterator itr = head_only_lvs.begin(); itr != head_only_lvs.end(); ) {
        int const& head_lv = *itr;
        bool found = false;
        for (int frag_idx = 0; frag_idx < allCache->size(); frag_idx++) {
            if ((*allCache)[frag_idx]->hasLv(head_lv)) {
                gvs_in_all_cache_fragments[frag_idx].push_back(head_lv);
                found = true;
                break;
            }
        }
        if (found) {
            itr = head_only_lvs.erase(itr);
        } else {
            itr++;
        }
    }
    _evaluation_memory_cost += sizeof(gvs_in_all_cache_fragments);

    /* Count the number of entailments */
    double all_ent = pow(kb.totalConstants(), head_uv_cnt + head_only_lvs.size());
    double _gv_bindings = 1;
    for (int i = 0; i < allCache->size(); i++) {
        std::vector<int> const& vids = gvs_in_all_cache_fragments[i];
        _evaluation_memory_cost += sizeof(int) * vids.capacity();
        size_t _cost = _evaluation_memory_cost;
        if (!vids.empty()) {
            CacheFragment const& fragment = *(*allCache)[i];
            int num_combinations = fragment.countCombinations(vids);
            all_ent *= num_combinations;
            _gv_bindings *= num_combinations;
            evaluationMemoryCost = std::max(evaluationMemoryCost, _evaluation_memory_cost);
            _evaluation_memory_cost = _cost;
        }
    }
    max_gv_bindings = std::max(max_gv_bindings, _gv_bindings);
    int new_pos_ent = 0;
    int already_ent = 0;
    SimpleRelation const& head_relation = *kb.getRelation(head_pred.getPredSymbol());
    std::unordered_set<const void*> used_rows;
    std::unordered_set<const void*> used_cbs;
    used_rows.reserve(head_relation.getTotalRows());
    used_cbs.reserve(posCache->getEntries().size());
    for (CacheFragment::entryType* const& entry: posCache->getEntries()) {
        CompliedBlock const* cb = (*entry)[HEAD_PRED_IDX];
        if (used_cbs.emplace(cb).second) {
            int* const* const rows = cb->getComplianceSet();
            for (int i = 0; i < cb->getTotalRows(); i++) {
                int* const row = rows[i];
                if (used_rows.emplace(row).second) {
                    if (head_relation.isEntailed(row)) {
                        already_ent++;
                    } else {
                        new_pos_ent++;
                    }
                }
            }
        }
    }
    _evaluation_memory_cost += sizeOfUnorderedSet(
        used_rows.bucket_count(), used_rows.max_load_factor(), sizeof(void*), sizeof(used_rows)
    ) + sizeOfUnorderedSet(
        used_cbs.bucket_count(), used_cbs.max_load_factor(), sizeof(void*), sizeof(used_cbs)
    );
    // evaluationMemoryCost = std::max(evaluationMemoryCost, _evaluation_memory_cost);
    // int already_ceg = cegCache->countTableSize(HEAD_PRED_IDX);
    long rss_finished = getMaxRss();
    evaluationMemoryCost = (rss_finished - rss_begin) * 1024;

    /* Update evaluation score */
    /* Those already proved should be excluded from the entire entailment set. Otherwise, they are counted as negative ones */
    return Eval(
        new_pos_ent, all_ent - already_ent, length, 
        eval.value(EvalMetric::Value::CompressionRatio), eval.value(EvalMetric::Value::InfoGain)
    );
    // return Eval(
    //     new_pos_ent, all_ent - already_ent - already_ceg, length, 
    //     eval.value(EvalMetric::Value::CompressionRatio), eval.value(EvalMetric::Value::InfoGain)
    // );
}

sinc::UpdateStatus CachedRule::specCase1HandlerPrePruning(int const predIdx, int const argIdx, int const varId) {
    uint64_t time_start = currentTimeInNano();
    obtainPosCache();
    posCache->updateCase1a(predIdx, argIdx, varId);
    uint64_t time_done = currentTimeInNano();
    posCacheUpdateTime = time_done - time_start;
    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::specCase1HandlerPostPruning(int const predIdx, int const argIdx, int const varId) {
    uint64_t time_start = currentTimeInNano();
    // obtainEntCache();
    // entCache->updateCase1a(predIdx, argIdx, varId);
    // uint64_t time_ent_done = currentTimeInNano();
    // obtainCegCache();
    // cegCache->updateCase1a(predIdx, argIdx, varId);
    // uint64_t time_ceg_done = currentTimeInNano();

    obtainAllCache();
    if (HEAD_PRED_IDX != predIdx) { // No need to update the E-cache if the update is in the head
        TabInfo const& tab_info = predIdx2AllCacheTableInfo[predIdx];
        CacheFragment* fragment = (*allCache)[tab_info.fragmentIdx];
        bool cache_empty = false;
        if (fragment->hasLv(varId)) {
            /* Update within the fragment */
            fragment->updateCase1a(tab_info.tabIdx, argIdx, varId);
            cache_empty = fragment->isEmpty();
        } else {
            /* Find the fragment that has the LV */
            bool not_found = true;
            for (int frag_idx2 = 0; frag_idx2 < allCache->size(); frag_idx2++) {
                CacheFragment& fragment2 = *(*allCache)[frag_idx2];
                if (fragment2.hasLv(varId)) {
                    /* Find the LV in another fragment, merge the two fragments */
                    not_found = false;
                    int fragment_tab_idx = tab_info.tabIdx;
                    mergeFragmentIndices(frag_idx2, tab_info.fragmentIdx);
                    fragment2.updateCase1c(*fragment, fragment_tab_idx, argIdx, varId);
                    cache_empty = fragment2.isEmpty();
                    delete fragment;
                    break;
                }
            }
            if (not_found) {
                /* Update within the target fragment */
                /* The var will be a PLV, so this fragment will not be empty */
                fragment->updateCase1a(tab_info.tabIdx, argIdx, varId);
            }
        }
        if (cache_empty) {
            for (CacheFragment* const& _fragment: *allCache) {
                _fragment->clear();
            }
        }
    }
    uint64_t time_all_done = currentTimeInNano();
    // cegCacheUpdateTime = time_ceg_done - time_start;
    // entCacheUpdateTime = time_ent_done - time_start;
    allCacheUpdateTime = time_all_done - time_start;

    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::specCase2HandlerPrePruning(int const predSymbol, int const arity, int const argIdx, int const varId) {
    uint64_t time_start = currentTimeInNano();
    obtainPosCache();
    SimpleRelation* new_relation = kb.getRelation(predSymbol);
    posCache->updateCase1b(new_relation, predSymbol, argIdx, varId);
    uint64_t time_done = currentTimeInNano();
    posCacheUpdateTime = time_done - time_start;
    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::specCase2HandlerPostPruning(int const predSymbol, int const arity, int const argIdx, int const varId) {
    uint64_t time_start = currentTimeInNano();
    // obtainEntCache();
    SimpleRelation* new_relation = kb.getRelation(predSymbol);
    // entCache->updateCase1b(new_relation, predSymbol, argIdx, varId);
    // uint64_t time_ent_done = currentTimeInNano();
    // obtainCegCache();
    // cegCache->updateCase1b(new_relation, predSymbol, argIdx, varId);
    // uint64_t time_ceg_done = currentTimeInNano();

    obtainAllCache();
    CacheFragment* updated_fragment = nullptr;
    for (int frag_idx = 0; frag_idx < allCache->size(); frag_idx++) {
        CacheFragment* fragment = (*allCache)[frag_idx];
        if (fragment->hasLv(varId)) {
            /* Append the new relation to the fragment that contains the LV */
            updated_fragment = fragment;
            predIdx2AllCacheTableInfo.emplace_back(frag_idx, fragment->getPartAssignedRule().size());
            fragment->updateCase1b(new_relation, predSymbol, argIdx, varId);
            break;
        }
    }
    if (nullptr == updated_fragment) {
        /* The LV has not been included in body yet. Create a new fragment */
        updated_fragment = new CacheFragment(new_relation, predSymbol);
        updated_fragment->updateCase1a(0, argIdx, varId);
        predIdx2AllCacheTableInfo.emplace_back(allCache->size(), 0);
        allCache->push_back(updated_fragment);
    }
    if (updated_fragment->isEmpty()) {
        for (CacheFragment* const& _fragment: *allCache) {
            _fragment->clear();
        }
    }
    uint64_t time_all_done = currentTimeInNano();
    // cegCacheUpdateTime = time_ceg_done - time_start;
    // entCacheUpdateTime = time_ent_done - time_start;
    allCacheUpdateTime = time_all_done - time_start;

    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::specCase3HandlerPrePruning(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) {
    uint64_t time_start = currentTimeInNano();
    obtainPosCache();
    posCache->updateCase2a(predIdx1, argIdx1, predIdx2, argIdx2, usedLimitedVars() - 1);
    uint64_t time_done = currentTimeInNano();
    posCacheUpdateTime = time_done - time_start;
    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::specCase3HandlerPostPruning(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) {
    uint64_t time_start = currentTimeInNano();
    // obtainEntCache();
    int new_vid = usedLimitedVars() - 1;
    // entCache->updateCase2a(predIdx1, argIdx1, predIdx2, argIdx2, new_vid);
    // uint64_t time_ent_done = currentTimeInNano();
    // obtainCegCache();
    // cegCache->updateCase2a(predIdx1, argIdx1, predIdx2, argIdx2, new_vid);
    // uint64_t time_ceg_done = currentTimeInNano();

    obtainAllCache();
    TabInfo const& tab_info1 = predIdx2AllCacheTableInfo[predIdx1];
    TabInfo const& tab_info2 = predIdx2AllCacheTableInfo[predIdx2];
    CacheFragment* fragment1 = nullptr;
    CacheFragment* fragment2 = nullptr;
    if (HEAD_PRED_IDX == predIdx1) {
        if (HEAD_PRED_IDX != predIdx2) {
            /* Update the fragment of predIdx2 only */
            fragment2 = (*allCache)[tab_info2.fragmentIdx];
            fragment2->updateCase1a(tab_info2.tabIdx, argIdx2, new_vid);
        }   //  Otherwise, update is in the head only, no need to change E-cache
    } else {
        fragment1 = (*allCache)[tab_info1.fragmentIdx];
        if (HEAD_PRED_IDX != predIdx2) {
            /* Update two predicates together */
            if (tab_info1.fragmentIdx == tab_info2.fragmentIdx) {
                /* Update within one fragment */
                fragment1->updateCase2a(tab_info1.tabIdx, argIdx1, tab_info2.tabIdx, argIdx2, new_vid);
            } else {
                /* Merge two fragments */
                fragment2 = (*allCache)[tab_info2.fragmentIdx];
                int fragment2_tab_idx = tab_info2.tabIdx;
                mergeFragmentIndices(tab_info1.fragmentIdx, tab_info2.fragmentIdx);
                fragment1->updateCase2c(tab_info1.tabIdx, argIdx1, *fragment2, fragment2_tab_idx, argIdx2, new_vid);
                delete fragment2;
                fragment2 = nullptr;
            }
        } else {
            /* Update the fragment of predIdx1 only */
            fragment1->updateCase1a(tab_info1.tabIdx, argIdx1, new_vid);
        }
    }
    if ((nullptr != fragment1 && fragment1->isEmpty()) || (nullptr != fragment2 && fragment2->isEmpty())) {
        for (CacheFragment* _fragment: *allCache) {
            _fragment->clear();
        }
    }
    uint64_t time_all_done = currentTimeInNano();
    // cegCacheUpdateTime = time_ceg_done - time_start;
    // entCacheUpdateTime = time_ent_done - time_start;
    allCacheUpdateTime = time_all_done - time_start;

    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::specCase4HandlerPrePruning(
    int const predSymbol, int const arity, int const argIdx1, int const predIdx2, int const argIdx2
) {
    uint64_t time_start = currentTimeInNano();
    obtainPosCache();
    SimpleRelation* new_relation = kb.getRelation(predSymbol);
    posCache->updateCase2b(new_relation, predSymbol, argIdx1, predIdx2, argIdx2, usedLimitedVars() - 1);
    uint64_t time_done = currentTimeInNano();
    posCacheUpdateTime = time_done - time_start;
    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::specCase4HandlerPostPruning(
    int const predSymbol, int const arity, int const argIdx1, int const predIdx2, int const argIdx2
) {
    long time_start = currentTimeInNano();
    SimpleRelation* new_relation = kb.getRelation(predSymbol);
    // obtainEntCache();
    int new_vid = usedLimitedVars() - 1;
    // entCache->updateCase2b(new_relation, predSymbol, argIdx1, predIdx2, argIdx2, new_vid);
    // uint64_t time_ent_done = currentTimeInNano();
    // obtainCegCache();
    // cegCache->updateCase2b(new_relation, predSymbol, argIdx1, predIdx2, argIdx2, new_vid);
    // uint64_t time_ceg_done = currentTimeInNano();

    obtainAllCache();
    if (HEAD_PRED_IDX == predIdx2) {   // One is the head and the other is not
        /* Create a new fragment for the new predicate */
        predIdx2AllCacheTableInfo.emplace_back(allCache->size(), 0);
        CacheFragment* fragment = new CacheFragment(new_relation, predSymbol);
        fragment->updateCase1a(0, argIdx1, new_vid);
        allCache->push_back(fragment);
    } else {    // Both are in the body
        CacheFragment& fragment = *(*allCache)[predIdx2AllCacheTableInfo[predIdx2].fragmentIdx];
        predIdx2AllCacheTableInfo.emplace_back(predIdx2AllCacheTableInfo[predIdx2].fragmentIdx, fragment.getPartAssignedRule().size());
        fragment.updateCase2b(new_relation, predSymbol, argIdx1, predIdx2AllCacheTableInfo[predIdx2].tabIdx, argIdx2, new_vid);
        if (fragment.isEmpty()) {
            for (CacheFragment* const& _fragment: *allCache) {
                _fragment->clear();
            }
        }
    }
    uint64_t time_all_done = currentTimeInNano();
    // cegCacheUpdateTime = time_ceg_done - time_start;
    // entCacheUpdateTime = time_ent_done - time_start;
    allCacheUpdateTime = time_all_done - time_start;

    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::specCase5HandlerPrePruning(int const predIdx, int const argIdx, int const constant) {
    uint64_t time_start = currentTimeInNano();
    obtainPosCache();
    posCache->updateCase3(predIdx, argIdx, constant);
    uint64_t time_done = currentTimeInNano();
    posCacheUpdateTime = time_done - time_start;
    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::specCase5HandlerPostPruning(int const predIdx, int const argIdx, int const constant) {
    uint64_t time_start = currentTimeInNano();
    // obtainEntCache();
    // entCache->updateCase3(predIdx, argIdx, constant);
    // uint64_t time_ent_done = currentTimeInNano();
    // obtainCegCache();
    // cegCache->updateCase3(predIdx, argIdx, constant);
    // uint64_t time_ceg_done = currentTimeInNano();

    obtainAllCache();
    if (HEAD_PRED_IDX != predIdx) { // No need to update the E-cache if the update is in the head
        TabInfo const& tab_info = predIdx2AllCacheTableInfo[predIdx];
        CacheFragment& fragment = *(*allCache)[tab_info.fragmentIdx];
        fragment.updateCase3(tab_info.tabIdx, argIdx, constant);
        if (fragment.isEmpty()) {
            for (CacheFragment* const& _fragment: *allCache) {
                _fragment->clear();
            }
        }
    }
    uint64_t time_all_done = currentTimeInNano();
    // cegCacheUpdateTime = time_ceg_done - time_start;
    // entCacheUpdateTime = time_ent_done - time_start;
    allCacheUpdateTime = time_all_done - time_start;

    return UpdateStatus::Normal;
}

sinc::UpdateStatus CachedRule::generalizeHandlerPrePruning(int const predIdx, int const argIdx) {
    return UpdateStatus::Invalid;
}

sinc::UpdateStatus CachedRule::generalizeHandlerPostPruning(int const predIdx, int const argIdx) {
    return UpdateStatus::Invalid;
}

void CachedRule::mergeFragmentIndices(int const baseFragmentIdx, int const mergingFragmentIdx) {
    /* Replace the merging fragment with the last */
    int last_frag_idx = allCache->size() - 1;
    int tabs_in_base = (*allCache)[baseFragmentIdx]->getPartAssignedRule().size();
    (*allCache)[mergingFragmentIdx] = (*allCache)[last_frag_idx];   // The merging fragment will be released in the caller

    /* Update predicate-fragment mapping */
    for (std::vector<TabInfo>::iterator itr = predIdx2AllCacheTableInfo.begin() + 1;    // skip head
            itr != predIdx2AllCacheTableInfo.end(); itr++) {
        TabInfo& tab_info = *itr;
        if (mergingFragmentIdx == tab_info.fragmentIdx) {
            tab_info.fragmentIdx = baseFragmentIdx;
            tab_info.tabIdx += tabs_in_base;
        }
        tab_info.fragmentIdx = (last_frag_idx == tab_info.fragmentIdx) ? mergingFragmentIdx : tab_info.fragmentIdx;
    }

    /* Remove the last fragment */
    allCache->pop_back();
}

void CachedRule::generateHeadTemplates(
    std::unordered_set<Record>& headTemplates, std::unordered_set<Record>** const bindingsInFragments,
    std::vector<std::vector<int>>* const headArgIdxLists, int* const argTemplate,
    std::vector<int> const& validFragmentIndices, int const headArity, int const idx
) const {
    std::unordered_set<Record>* bindings_in_fragment = bindingsInFragments[idx];
    std::vector<std::vector<int>>& gv_links = headArgIdxLists[idx];
    if (idx == validFragmentIndices.size() - 1) {
        /* Finish the last group of bindings, add to the template set */
        for (Record const& binding: *bindings_in_fragment) {
            for (int i = 0; i < binding.getArity(); i++) {
                std::vector<int>& head_arg_idxs = gv_links[i];
                for (int const& head_arg_idx: head_arg_idxs) {
                    argTemplate[head_arg_idx] = binding.getArgs()[i];
                }
            }
            int* cloned_args = new int[headArity];
            std::copy(argTemplate, argTemplate + headArity, cloned_args);
            if (!headTemplates.emplace(cloned_args, headArity).second) {
                delete[] cloned_args;
            }
        }
    } else {
        /* Add current binding to the template and move to the next recursion */
        for (Record const& binding: *bindings_in_fragment) {
            for (int i = 0; i < binding.getArity(); i++) {
                std::vector<int>& head_arg_idxs = gv_links[i];
                for (int const& head_arg_idx: head_arg_idxs) {
                    argTemplate[head_arg_idx] = binding.getArgs()[i];
                }
            }
            generateHeadTemplates(
                headTemplates, bindingsInFragments, headArgIdxLists, argTemplate, validFragmentIndices, headArity, idx + 1
            );
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
    size_t size = sizeof(fingerprintCaches) + sizeof(Rule::fingerprintCacheType*) * fingerprintCaches.capacity();
    for (Rule::fingerprintCacheType* const& cache: fingerprintCaches) {
        size += sizeOfUnorderedSet(
            cache->bucket_count(), cache->max_load_factor(), sizeof(Fingerprint*), sizeof(Rule::fingerprintCacheType)
        );
        for (const Fingerprint* const& fp: *cache) {
            size += fp->getMemCost();
        }
    }
    return size;
}

size_t RelationMinerWithCachedRule::getTabuMapMemCost() const {
    size_t size = sizeof(Rule::tabuMapType) + sizeOfUnorderedMap(
        tabuMap.bucket_count(), tabuMap.max_load_factor(), sizeof(std::pair<MultiSet<int>*, Rule::fingerprintCacheType*>), sizeof(tabuMap)
    );
    for (std::pair<MultiSet<int>*, Rule::fingerprintCacheType*> const& kv: tabuMap) {
        size += kv.first->getMemoryCost();
        Rule::fingerprintCacheType const& cache = *(kv.second);
        size += sizeOfUnorderedSet(cache.bucket_count(), cache.max_load_factor(), sizeof(Fingerprint*), sizeof(cache));
    }
    return size;
}

sinc::Rule* RelationMinerWithCachedRule::getStartRule() {
    Rule::fingerprintCacheType* cache = new Rule::fingerprintCacheType();
    fingerprintCaches.push_back(cache);
    CachedRule* rule =  new CachedRule(targetRelation, kb.getRelation(targetRelation)->getTotalCols(), *cache, tabuMap, kb, &counterexamples);
    monitor.cacheEntryMemCost = std::max(monitor.cacheEntryMemCost, CachedRule::addCumulatedCacheEntryMemoryCost(rule));
    return rule;
}

void RelationMinerWithCachedRule::selectAsBeam(Rule* r) {
    CachedRule* rule = (CachedRule*) r;
    rule->updateCacheIndices();
    monitor.posCacheIndexingTime += rule->getPosCacheIndexingTime();
    // monitor.entCacheIndexingTime += rule->getEntCacheIndexingTime();
    monitor.allCacheIndexingTime += rule->getAllCacheIndexingTime();
}

int RelationMinerWithCachedRule::checkThenAddRule(
    UpdateStatus updateStatus, Rule* const updatedRule, Rule& originalRule, Rule** candidates
) {
    CachedRule* rule = (CachedRule*) updatedRule;
    if (UpdateStatus::Normal == updateStatus) {
        monitor.posCacheEntriesTotal += rule->getPosCache().getEntries().size();
        // monitor.entCacheEntriesTotal += rule->getEntCache().getEntries().size();
        int all_cache_entries = 0;
        if (!rule->getAllCache().empty()) {
            for (CacheFragment* const& fragment : rule->getAllCache()) {
                all_cache_entries += fragment->getEntries().size();
            }
            all_cache_entries /= rule->getAllCache().size();
        }
        monitor.allCacheEntriesTotal += all_cache_entries;
        monitor.posCacheEntriesMax = std::max(monitor.posCacheEntriesMax, (int)rule->getPosCache().getEntries().size());
        // monitor.entCacheEntriesMax = std::max(monitor.entCacheEntriesMax, (int)rule->getEntCache().getEntries().size());
        monitor.allCacheEntriesMax = std::max(monitor.allCacheEntriesMax, all_cache_entries);
        monitor.totalGeneratedRules++;
        monitor.totalCacheFragmentsInAllCache += rule->getAllCache().size();
        monitor.posCacheUpdateTime += rule->getPosCacheUpdateTime();
        // monitor.entCacheUpdateTime += rule->getEntCacheUpdateTime();
        monitor.allCacheUpdateTime += rule->getAllCacheUpdateTime();
    } else {
        monitor.prunedPosCacheUpdateTime += rule->getPosCacheUpdateTime();
    }
    monitor.copyTime += rule->getCopyTime();
    monitor.cacheEntryMemCost = std::max(monitor.cacheEntryMemCost, CachedRule::addCumulatedCacheEntryMemoryCost(rule));
    monitor.maxEvalMemCost = std::max(monitor.maxEvalMemCost, rule->getEvaluationMemoryCost());

    return RelationMiner::checkThenAddRule(updateStatus, updatedRule, originalRule, candidates);
}

/**
 * SincWithCache
 */
using sinc::SincWithCache;

SincWithCache::SincWithCache(SincConfig* const config) : SInC(config) {}

SincWithCache::SincWithCache(SincConfig* const config, SimpleKb* const kb) : SInC(config, kb) {}

void SincWithCache::getTargetRelations(int* & targetRelationIds, int& numTargets) {
    SInC::getTargetRelations(targetRelationIds, numTargets);
    CompliedBlock::reserveMemSpace(*kb);
}

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
    // monitor.entCacheUpdateTime += rel_miner->monitor.entCacheUpdateTime;
    monitor.allCacheUpdateTime += rel_miner->monitor.allCacheUpdateTime;
    monitor.posCacheIndexingTime += rel_miner->monitor.posCacheIndexingTime;
    // monitor.entCacheIndexingTime += rel_miner->monitor.entCacheIndexingTime;
    monitor.allCacheIndexingTime += rel_miner->monitor.allCacheIndexingTime;
    monitor.posCacheEntriesTotal += rel_miner->monitor.posCacheEntriesTotal;
    // monitor.entCacheEntriesTotal += rel_miner->monitor.entCacheEntriesTotal;
    monitor.allCacheEntriesTotal += rel_miner->monitor.allCacheEntriesTotal;
    monitor.posCacheEntriesMax = std::max(monitor.posCacheEntriesMax, rel_miner->monitor.posCacheEntriesMax);
    // monitor.entCacheEntriesMax = std::max(monitor.entCacheEntriesMax, rel_miner->monitor.entCacheEntriesMax);
    monitor.allCacheEntriesMax = std::max(monitor.allCacheEntriesMax, rel_miner->monitor.allCacheEntriesMax);
    monitor.totalGeneratedRules += rel_miner->monitor.totalGeneratedRules;
    monitor.totalCacheFragmentsInAllCache += rel_miner->monitor.totalCacheFragmentsInAllCache;
    monitor.copyTime += rel_miner->monitor.copyTime;
    monitor.cacheEntryMemCost = std::max(monitor.cacheEntryMemCost, rel_miner->monitor.cacheEntryMemCost);
    monitor.fingerprintCacheMemCost = std::max(monitor.fingerprintCacheMemCost, rel_miner->getFingerprintCacheMemCost());
    monitor.tabuMapMemCost = std::max(monitor.tabuMapMemCost, rel_miner->getTabuMapMemCost());
    monitor.maxEvalMemCost = std::max(monitor.maxEvalMemCost, rel_miner->monitor.maxEvalMemCost);
    monitor.maxCbPoolSize = std::max(monitor.maxCbPoolSize, CompliedBlock::totalNumCbs());
    size_t total_idx = CompliedBlock::getNumCreationIndices() + CompliedBlock::getNumGetSliceIndices() + CompliedBlock::getNumSplitSlicesIndices() + CompliedBlock::getNumMatchSlices1Indices() + CompliedBlock::getNumMatchSlices2Indices();
    monitor.maxCbPoolIdxSize = std::max(monitor.maxCbPoolIdxSize, total_idx);
    CompliedBlock::clearPool();

    /* Log memory usage */
    rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    (*logger) << "Finalized Max Mem:" << monitor.formatMemorySize(usage.ru_maxrss) << std::endl;

}

void SincWithCache::showMonitor() {
    SInC::showMonitor();

    /* Calculate memory cost */
    monitor.cbMemCost = CompliedBlock::totalCbMemoryCost() / 1024;
    monitor.cacheEntryMemCost /= 1024;
    monitor.fingerprintCacheMemCost /= 1024;
    monitor.tabuMapMemCost /= 1024;
    monitor.maxEvalMemCost /= 1024;

    monitor.show(*logger);
}

void SincWithCache::finish() {
    CompliedBlock::clearPool();
}
