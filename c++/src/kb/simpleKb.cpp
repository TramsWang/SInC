#include "simpleKb.h"
#include "../util/util.h"
#include <string.h>
#include <algorithm>
#include <bitset>
#include <cmath>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

/**
 * KbException
 */
using sinc::KbException;
KbException::KbException() : message("SInC Runtime Error") {}

KbException::KbException(const std::string& msg) : message(msg) {}

const char* KbException::what() const throw() {
    return message.c_str();
}

/**
 * SplitRecords
 */
using sinc::SplitRecords;
SplitRecords::SplitRecords(std::vector<int*>* const _entailedRecords, std::vector<int*>* const _nonEntailedRecords) :
    entailedRecords(_entailedRecords), nonEntailedRecords(_nonEntailedRecords) {}

SplitRecords::~SplitRecords() {
    delete entailedRecords;
    delete nonEntailedRecords;
}

/**
 * SimpleRelation
 */
using sinc::SimpleRelation;
double SimpleRelation::minConstantCoverage = DEFAULT_MIN_CONSTANT_COVERAGE;

int** SimpleRelation::loadFile(const path& filePath, int const arity, int const totalRecords) {
    IntReader reader(filePath.c_str());
    int** records = new int*[totalRecords];
    for (int i = 0; i < totalRecords; i++) {
        int* record = new int[arity];
        for (int arg_idx = 0; arg_idx < arity; arg_idx++) {
            record[arg_idx] = reader.next();
        }
        records[i] = record;
    }
    reader.close();
    return records;
}

SimpleRelation::SimpleRelation(const std::string& _name, int const _id, int** _records, int const _arity, int const _totalRecords) : 
    IntTable(_records, _totalRecords, _arity), name(strdup(_name.c_str())), id(_id), maintainRecords(false),
    entailmentFlags(new int[NUM_FLAG_INTS(_totalRecords)]{0}), flagLength(NUM_FLAG_INTS(_totalRecords)) {}

SimpleRelation::SimpleRelation(
            const std::string& _name, int const _id, int const _arity, int const _totalRecords, const path& _filePath
) : IntTable(loadFile(_filePath, _arity, _totalRecords), _totalRecords, _arity, true), name(strdup(_name.c_str())), id(_id),
    maintainRecords(true), entailmentFlags(new int[NUM_FLAG_INTS(_totalRecords)]{0}), flagLength(NUM_FLAG_INTS(_totalRecords)) {}

SimpleRelation::~SimpleRelation() {
    free((void*)name);  // as `strdup()` uses `malloc()`
    if (maintainRecords) {
        int** const rows = sortedRowsByCols[0];
        for (int i = 0; i < totalRows; i++) {
            delete[] rows[i];
        }
    }
    delete[] entailmentFlags;
}

void SimpleRelation::setAsEntailed(int* const record) {
    int idx = whereIs(record);
    if (0 <= idx) {
        setEntailmentFlag(idx);
    }
}

void SimpleRelation::setAsNotEntailed(int* const record) {
    int idx = whereIs(record);
    if (0 <= idx) {
        unsetEntailmentFlag(idx);
    }
}

void SimpleRelation::setAllAsEntailed(int** const records, int const length) {
    std::sort(records, records + length, comparator);
    int** const this_rows = sortedRowsByCols[0];
    int idx = 0;
    int idx2 = 0;
    while (idx < totalRows && idx2 < length) {
        int* const row = this_rows[idx];
        int* const row2 = records[idx2];
        if (comparator(row, row2)) {    // row < row2
            idx = std::lower_bound(this_rows + idx + 1, this_rows + totalRows, row2, comparator) - this_rows;
        } else if (comparator(row2, row)) { // row > row2
            idx2 = std::lower_bound(records + idx2 + 1, records + length, row, comparator) - records;
        } else {    // row == row2
            setEntailmentFlag(idx);
            idx++;
            idx2++;
        }
    }
}

bool SimpleRelation::isEntailed(int* const record) const {
    int idx = whereIs(record);
    return (0 <= idx) && 0 != entailment(idx);
}

bool SimpleRelation::entailIfNot(int* const record) {
    int idx = whereIs(record);
    if (0 <= idx && 0 == entailment(idx)) {
        setEntailmentFlag(idx);
        return true;
    }
    return false;
}

int SimpleRelation::totalEntailedRecords() const {
    int cnt = 0;
    for (int i = 0; i < flagLength; i++) {
        cnt += std::bitset<BITS_PER_INT>(entailmentFlags[i]).count();
    }
    return cnt;
}

std::vector<int>** SimpleRelation::getPromisingConstants() const {
    std::vector<int>** promising_constants_by_cols = new std::vector<int>*[totalCols];
    int threshold = (int) ceil(totalRows * minConstantCoverage);
    for (int col = 0; col < totalCols; col++) {
        std::vector<int>* promising_constants = new std::vector<int>();
        int* const values = valuesByCols[col];
        int* const start_offsets = startOffsetsByCols[col];
        int const length = valuesByColsLengths[col];
        for (int i = 0; i < length; i++) {
            if (threshold <= start_offsets[i + 1] - start_offsets[i]) {
                promising_constants->push_back(values[i]);
            }
        }
        promising_constants_by_cols[col] = promising_constants;
    }
    return promising_constants_by_cols;
}

void SimpleRelation::releasePromisingConstants(std::vector<int>** promisingConstants, int const arity) {
    for (int col = 0; col < arity; col++) {
        delete promisingConstants[col];
    }
    delete[] promisingConstants;
}

void SimpleRelation::dump(const path& filePath) const {
    IntWriter writer(filePath.c_str());
    int** const rows = sortedRowsByCols[0];
    for (int i = 0; i < totalRows; i++) {
        int* const record = rows[i];
        for (int arg_idx = 0; arg_idx < totalCols; arg_idx++) {
            writer.write(record[arg_idx]);
        }
    }
    writer.close();
}

void SimpleRelation::dumpNecessaryRecords(
    const path& filePath, const std::vector<int*>& fvsRecords
) const {
    IntWriter writer(filePath.c_str());
    int** const records = sortedRowsByCols[0];
    for (int idx = 0; idx < totalRows; idx++) {
        if (0 == entailment(idx)) {
            int* const record = records[idx];
            for (int arg_idx = 0; arg_idx < totalCols; arg_idx++) {
                writer.write(record[arg_idx]);
            }
        }
    }
    for (int* const& record: fvsRecords) {
        for (int arg_idx = 0; arg_idx < totalCols; arg_idx++) {
            writer.write(record[arg_idx]);
        }
    }
    writer.close();
}

void SimpleRelation::setFlagOfReservedConstants(int* const flags) const {
    int** const records = sortedRowsByCols[0];
    for (int idx = 0; idx < totalRows; idx++) {
        if (0 == entailment(idx)) {
            int* const record = records[idx];
            for (int arg_idx = 0; arg_idx < totalCols; arg_idx++) {
                int arg = record[arg_idx];
                flags[arg / BITS_PER_INT] |= 1 << (arg % BITS_PER_INT);
            }
        }
    }
}

SplitRecords* SimpleRelation::splitByEntailment() const {
    int already_entailed_cnt = totalEntailedRecords();
    int** const rows = sortedRowsByCols[0];
    if (0 == already_entailed_cnt) {
        return new SplitRecords(new std::vector<int*>(), new std::vector<int*>(rows, rows + totalRows));
    }
    if (totalRows == already_entailed_cnt) {
        return new SplitRecords(new std::vector<int*>(rows, rows + totalRows), new std::vector<int*>());
    }
    std::vector<int*>* const entailed_records = new std::vector<int*>();
    std::vector<int*>* const non_entailed_records = new std::vector<int*>();
    entailed_records->reserve(already_entailed_cnt);
    non_entailed_records->reserve(totalRows - already_entailed_cnt);
    int idx = 0;
    for (int i = 0; i < flagLength; i++) {
        int flags = entailmentFlags[i];
        int limit = std::min(idx + (int)BITS_PER_INT, totalRows);
        int mask = 1;
        while (idx < limit) {
            if (0 == (flags & mask)) {
                /* Not entailed */
                non_entailed_records->push_back(rows[idx]);
            } else {
                /* Entailed */
                entailed_records->push_back(rows[idx]);
            }
            idx++;
            mask = mask << 1;
        }
    }
    return new SplitRecords(entailed_records, non_entailed_records);
}

size_t SimpleRelation::memoryCost() const {
    size_t size = IntTable::memoryCost() - sizeof(IntTable) + sizeof(SimpleRelation);
    size += sizeof(int) * flagLength + sizeof(char) * (strlen(name) + 1);
    if (maintainRecords) {
        size += sizeof(int) * totalRows * totalCols;    // size of records
    }
    return size;
}

void SimpleRelation::setEntailmentFlag(int const idx) {
    entailmentFlags[idx / BITS_PER_INT] |= 0x1 << (idx % BITS_PER_INT);
}

void SimpleRelation::unsetEntailmentFlag(int const idx) {
    entailmentFlags[idx / BITS_PER_INT] &= ~(0x1 << (idx % BITS_PER_INT));
}

int SimpleRelation::entailment(int const idx) const {
    return entailmentFlags[idx / BITS_PER_INT] & (0x1 << (idx % BITS_PER_INT));
}

/**
 * SimpleKb
 */
using sinc::SimpleKb;

path SimpleKb::getKbDirPath(const char* const kbName, const path& basePath) {
    return basePath / path(kbName);
}

path SimpleKb::getRelInfoFilePath(const char* const kbName, const path& basePath) {
    return basePath / path(kbName) / path(REL_INFO_FILE_NAME);
}

path SimpleKb::getRelDataFilePath(int const relId, const char* const kbName, const path& basePath) {
    std::ostringstream os;
    os << relId << REL_DATA_FILE_SUFFIX;
    return basePath / path(kbName) / path(os.str());
}

path SimpleKb::getMapFilePath(const path& kbDirPath, int const mapNum) {
    std::ostringstream os;
    os << MAP_FILE_PREFIX << mapNum << MAP_FILE_SUFFIX;
    return kbDirPath / path(os.str());
}

SimpleKb::SimpleKb(const std::string& _name, const path& _basePath) : name(strdup(_name.c_str())), promisingConstants(nullptr),
    relations(new std::vector<SimpleRelation*>()), relationNameMap(new std::unordered_map<std::string, SimpleRelation*>())
{
    path kb_dir_path = getKbDirPath(name, _basePath);
    path rel_info_file_path = getRelInfoFilePath(name, _basePath);
    std::ifstream rel_info_file(rel_info_file_path.c_str(), std::ios::in);
    std::string line;
    int line_num = 0;
    constants = 0;
    if (rel_info_file.is_open()) {
        while (std::getline(rel_info_file, line)) {
            std::stringstream ls(line);
            std::string rel_name;
            std::string arity_str;
            std::string total_records_str;
            std::getline(ls, rel_name, '\t');
            std::getline(ls, arity_str, '\t');
            std::getline(ls, total_records_str, '\t');
            path rel_file_path = getRelDataFilePath(line_num, name, _basePath);
            line_num++;
            if (fs::exists(rel_file_path)) {
                /* Load from file */
                int rel_id = relations->size();
                SimpleRelation* relation = new SimpleRelation(
                    rel_name, rel_id, std::stoi(arity_str), std::stoi(total_records_str), rel_file_path
                );
                relations->push_back(relation);
                relationNameMap->emplace(rel_name, relation);
                constants = std::max(constants, relation->maxValue());
            }   // If no file found, the relation is empty, and thus should not be included in the SimpleKb
        }
        rel_info_file.close();

        /* Collect relation names */
        relationNames = new const char*[relations->size()];
        for (int rel_id = 0; rel_id < relations->size(); rel_id++) {
            relationNames[rel_id] = (*relations)[rel_id]->name;
        }
    } else {
        std::cerr << "Failed to load a SimpleKb: " << "Failed to open file: " << rel_info_file_path << std::endl;
    }
}

SimpleKb::SimpleKb(
    const std::string& _name, int*** const _relations, std::string* const _relNames, int* const _arities, int* const _totalRows,
    int const _numReltaions) : name(strdup(_name.c_str())), promisingConstants(nullptr), relationNames(new const char*[_numReltaions]),
    relations(new std::vector<SimpleRelation*>()), relationNameMap(new std::unordered_map<std::string, SimpleRelation*>())
{
    constants = 0;
    for (int i = 0; i < _numReltaions; i++) {
        SimpleRelation* relation = new SimpleRelation(_relNames[i], i, _relations[i], _arities[i], _totalRows[i]);
        relations->push_back(relation);
        relationNameMap->emplace(_relNames[i], relation);
        constants = std::max(constants, relation->maxValue());
        relationNames[i] = relation->name;
    }
}

SimpleKb::SimpleKb(const SimpleKb& another) : name(strdup(another.name)),
    relations(new std::vector<SimpleRelation*>(*(another.relations))), relationNames(new const char*[another.relations->size()]),
    relationNameMap(new std::unordered_map<std::string, SimpleRelation*>(*(another.relationNameMap)))
{
    if (nullptr == another.promisingConstants) {
        promisingConstants = nullptr;
    } else {
        promisingConstants = new std::vector<int>**[relations->size()];
        for (int rel_id = 0; rel_id < relations->size(); rel_id++) {
            SimpleRelation* relation = (*relations)[rel_id];
            promisingConstants[rel_id] = new std::vector<int>*[relation->getTotalCols()];
            for (int col = 0; col < relation->getTotalCols(); col++) {
                promisingConstants[rel_id][col] = new std::vector<int>(*(another.promisingConstants[rel_id][col]));
            }
            relationNames[rel_id] = (*relations)[rel_id]->name;
        }
    }
}

SimpleKb::~SimpleKb() {
    free((void*)name);  // as `strdup()` uses `malloc()`
    releasePromisingConstants();
    for (SimpleRelation* const& r: *relations) {
        delete r;
    }
    delete relations;
    delete relationNameMap;
    delete[] relationNames;
}

void SimpleKb::releasePromisingConstants() {
    if (nullptr == promisingConstants) {
        return;
    }

    for (int rel_id = 0; rel_id < relations->size(); rel_id++) {
        SimpleRelation::releasePromisingConstants(promisingConstants[rel_id], (*relations)[rel_id]->getTotalCols());
    }
    delete[] promisingConstants;
}

void SimpleKb::dump(const path& basePath, std::string* mappedNames) const {
    /* Check & create dir */
    path kb_dir = getKbDirPath(name, basePath);
    if (!fs::exists(kb_dir) && !fs::create_directories(kb_dir)) {
        std::cerr << "Failed to dump SimpleKb '" << name << "': Failed to create dir: " << kb_dir << std::endl;
        return;
    }

    /* Dump mappings */
    int map_num = MAP_FILE_NUMERATION_START;
    std::ofstream ofs(getMapFilePath(kb_dir, map_num), std::ios::out);
    int records_cnt = 0;
    for (int i = 1; i < constants; i++) {
        if (MAX_MAP_ENTRIES <= records_cnt) {
            ofs.close();
            map_num++;
            records_cnt = 0;
            ofs = std::ofstream(getMapFilePath(kb_dir, map_num), std::ios::out);
        }
        ofs << mappedNames[i] << '\n';
        records_cnt++;
    }
    ofs.close();

    /* Dump relations */
    ofs = std::ofstream(getRelInfoFilePath(name, basePath), std::ios::out);
    for (int rel_id = 0; rel_id < relations->size(); rel_id++) {
        SimpleRelation* relation = (*relations)[rel_id];
        ofs << relation->name << '\t' << relation->getTotalCols() << '\t' << relation->getTotalRows() << '\n';
        if (0 < relation->getTotalRows()) {
            /* Dump only non-empty relations */
            relation->dump(getRelDataFilePath(rel_id, name, basePath));
        }
    }
    ofs.close();
}

SimpleRelation* SimpleKb::getRelation(const std::string& name) const {
    std::unordered_map<std::string, SimpleRelation*>::const_iterator kv = relationNameMap->find(name);
    return (relationNameMap->end() == kv) ? nullptr : kv->second;
}

SimpleRelation* SimpleKb::getRelation(int const id) const {
    return (id >=0 && id < relations->size()) ? (*relations)[id] : nullptr;
}

bool SimpleKb::hasRecord(const std::string& relationName, int* const record) const {
    SimpleRelation* relation = getRelation(relationName);
    return (nullptr != relation) && relation->hasRow(record);
}

bool SimpleKb::hasRecord(int const relationId, int* const record) const {
    SimpleRelation* relation = getRelation(relationId);
    return (nullptr != relation) && relation->hasRow(record);
}

void SimpleKb::setAsEntailed(const std::string& relationName, int* const record) {
    SimpleRelation* relation = getRelation(relationName);
    if (nullptr != relation) {
        relation->setAsEntailed(record);
    }
}

void SimpleKb::setAsEntailed(int const relationId, int* const record) {
    SimpleRelation* relation = getRelation(relationId);
    if (nullptr != relation) {
        relation->setAsEntailed(record);
    }
}

void SimpleKb::setAsNotEntailed(const std::string& relationName, int* const record) {
    SimpleRelation* relation = getRelation(relationName);
    if (nullptr != relation) {
        relation->setAsNotEntailed(record);
    }
}

void SimpleKb::setAsNotEntailed(int const relationId, int* const record) {
    SimpleRelation* relation = getRelation(relationId);
    if (nullptr != relation) {
        relation->setAsNotEntailed(record);
    }
}

void SimpleKb::updatePromisingConstants() {
    if (nullptr == promisingConstants) {
        promisingConstants = new std::vector<int>**[relations->size()];
        for (int i = 0; i < relations->size(); i++) {
            promisingConstants[i] = (*relations)[i]->getPromisingConstants();
        }
    }
}

std::vector<int>** SimpleKb::getPromisingConstants(int relId) const {
    return promisingConstants[relId];
}

const char* SimpleKb::getName() const {
    return name;
}

std::vector<SimpleRelation*>* SimpleKb::getRelations() const {
    return relations;
}

int SimpleKb::totalRelations() const {
    return relations->size();
}

int SimpleKb::totalRecords() const {
    int cnt = 0;
    for (SimpleRelation* const& relation: *relations) {
        cnt += relation->getTotalRows();
    }
    return cnt;
}

int SimpleKb::totalConstants() const {
    return constants;
}

const char* const * SimpleKb::getRelationNames() const {
    return relationNames;
}

size_t SimpleKb::memoryCost() const {
    size_t size = sizeof(SimpleKb) + sizeof(char) * (strlen(name) + 1) + sizeof(*relations) + 
        (sizeof(SimpleRelation*) + sizeof(char*)) * relations->size() + 
        sizeof(*relationNameMap);
    for (int i = 0; i < relations->size(); i++) {
        SimpleRelation* relation = (*relations)[i];
        std::vector<int>** const& promising_constant = promisingConstants[i];
        size += relation->memoryCost();
        size += sizeof(std::vector<int>*) * relation->getTotalCols();
        for (int arg_idx = 0; arg_idx < relation->getTotalCols(); arg_idx++) {
            std::vector<int>& v = *(promising_constant[arg_idx]);
            size += sizeof(int) * v.size() + sizeof(v);
        }
    }
    size += sizeof(std::vector<int>**) * relations->size();
    for (std::pair<const std::string, SimpleRelation*> const& kv: *relationNameMap) {
        size += sizeof(kv) + sizeof(char) * (kv.first.length() + 1);
    }
    return size;
}

/**
 * SimpleCompressedKb
 */
using sinc::SimpleCompressedKb;

path SimpleCompressedKb::getCounterexampleFilePath(int const relId, const char* const kbName, const path& basePath) {
    std::ostringstream os;
    os << relId << COUNTEREXAMPLE_FILE_SUFFIX;
    return basePath / path(kbName) / path(os.str());
}

SimpleCompressedKb::SimpleCompressedKb(const std::string& _name, SimpleKb* const _originalKb) : name(strdup(_name.c_str())),
    originalKb(_originalKb), fvsRecords(new std::vector<int*>[_originalKb->totalRelations()]),
    counterexampleSets(new std::unordered_set<Record>[_originalKb->totalRelations()]) {}

SimpleCompressedKb::~SimpleCompressedKb() {
    free((void*)name);
    for (Rule* const& r: hypothesis) {
        delete r;
    }
    for (int rel_id = 0; rel_id < originalKb->totalRelations(); rel_id++) {
        for (Record const& r: counterexampleSets[rel_id]) {
            delete[] r.getArgs();
        }
    }
    delete[] fvsRecords;
    delete[] counterexampleSets;
}

void SimpleCompressedKb::addFvsRecord(int const relId, int* const record) {
    fvsRecords[relId].push_back(record);
}

void SimpleCompressedKb::addCounterexamples(int const relId, const std::unordered_set<Record>& records) {
    std::unordered_set<Record>& set = counterexampleSets[relId];
    for (Record const& r: records) {
        if (!set.emplace(r.getArgs(), r.getArity()).second) {
            delete[] r.getArgs();
        }
    }
}

void SimpleCompressedKb::addHypothesisRules(const std::vector<Rule*>& rules) {
    for (Rule* const& r: rules) {
        hypothesis.push_back(r);
    }
}

void SimpleCompressedKb::updateSupplementaryConstants() {
    /* Build flags for all constants */
    int num_bits = originalKb->totalConstants() + 1;    // constant numerations start form 1
    int flags[NUM_FLAG_INTS(num_bits)]{0};

    /* Remove all occurred arguments*/
    for (int i = 0; i < originalKb->totalRelations(); i++) {
        SimpleRelation* relation = originalKb->getRelation(i);
        relation->setFlagOfReservedConstants(flags);
        for (int* const& record: fvsRecords[i]) {
            for (int arg_idx = 0; arg_idx < relation->getTotalCols(); arg_idx++) {
                int arg = record[arg_idx];
                flags[arg / BITS_PER_INT] |= 1 << (arg % BITS_PER_INT);
            }
        }
        for (Record const& record: counterexampleSets[i]) {
            for (int arg_idx = 0; arg_idx < relation->getTotalCols(); arg_idx++) {
                int arg = record.getArgs()[arg_idx];
                flags[arg / BITS_PER_INT] |= 1 << (arg % BITS_PER_INT);
            }
        }
    }
    for (Rule* const& rule: hypothesis) {
        for (int pred_idx = 0; pred_idx < rule->numPredicates(); pred_idx++) {
            const Predicate& predicate = rule->getPredicate(pred_idx);
            for (int arg_idx = 0; arg_idx < predicate.getArity(); arg_idx++) {
                int argument = predicate.getArg(arg_idx);
                if (ARG_IS_CONSTANT(argument)) {
                    int arg = ARG_DECODE(argument);
                    flags[arg / BITS_PER_INT] |= 1 << (arg % BITS_PER_INT);
                }
            }
        }
    }

    /* Find all integers that are not flagged */
    flags[0] |= 0x1;    // 0 should not be marked as a supplementary constant
    supplementaryConstants.clear();
    int base_offset = 0;
    for (int sub_flag: flags) {
        for (int i = 0; i < BITS_PER_INT && base_offset + i <= originalKb->totalConstants(); i++) {
            if (0 == (sub_flag & 1)) {
                supplementaryConstants.push_back(base_offset + i);
            }
            sub_flag >>= 1;
        }
        base_offset += BITS_PER_INT;
    }
}

void SimpleCompressedKb::dump(const path& basePath) {
    path dir_path = basePath / path(name);
    if (!fs::exists(dir_path) && !fs::create_directories(dir_path)) {
        std::cerr << "Failed to dump Compressed Kb '" << name << "': Failed to create dir: " << dir_path << std::endl;
        return;
    }

    /* Dump necessary records & counterexamples */
    const char* names[originalKb->totalRelations()];
    std::ofstream ofs(SimpleKb::getRelInfoFilePath(name, basePath), std::ios::out);
    for (int rel_id = 0; rel_id < originalKb->totalRelations(); rel_id++) {
        SimpleRelation* relation = originalKb->getRelation(rel_id);
        relation->dumpNecessaryRecords(SimpleKb::getRelDataFilePath(rel_id, name, basePath), fvsRecords[rel_id]);
        names[rel_id] = relation->name;

        /* Dump counterexamples */
        const std::unordered_set<Record>& counterexamples = counterexampleSets[rel_id];
        if (0 < counterexamples.size()) {
            /* Dump only non-empty relations */
            IntWriter writer(getCounterexampleFilePath(rel_id, name, basePath).c_str());
            for (Record const& counterexample : counterexamples) {
                for (int arg_idx = 0; arg_idx < relation->getTotalCols(); arg_idx++) {
                    int arg = counterexample.getArgs()[arg_idx];
                    writer.write(arg);
                }
            }
            writer.close();
        }

        /* Dump relation info */
        int necessary_records = relation->getTotalRows() - relation->totalEntailedRecords() + fvsRecords[rel_id].size();
        ofs << relation->name << '\t' << relation->getTotalCols() << '\t' << necessary_records << '\t' << counterexamples.size() << '\n';
    }
    ofs.close();

    /* Dump hypothesis */
    if (0 < hypothesis.size()) {
        std::ofstream ofs(dir_path / path(HYPOTHESIS_FILE_NAME), std::ios::out);
        for (Rule* const& rule : hypothesis) {
            ofs << rule->toDumpString(names) << '\n';
        }
        ofs.close();
    }

    /* Dump supplementary constants */
    updateSupplementaryConstants();
    if (0 < supplementaryConstants.size()) {
        IntWriter writer((dir_path / path(SUPPLEMENTARY_CONSTANTS_FILE_NAME)).c_str());
        for (int const& i : supplementaryConstants) {
            writer.write(i);
        }
        writer.close();
    }
}

std::vector<sinc::Rule*>& SimpleCompressedKb::getHypothesis() {
    return hypothesis;
}

std::unordered_set<sinc::Record>& SimpleCompressedKb::getCounterexampleSet(int const relId) {
    return counterexampleSets[relId];
}

int SimpleCompressedKb::totalNecessaryRecords() const {
    int cnt = 0;
    for (int rel_id = 0; rel_id < originalKb->totalRelations(); rel_id++) {
        SimpleRelation* relation = originalKb->getRelation(rel_id);
        cnt += relation->getTotalRows() - relation->totalEntailedRecords();
    }
    cnt += totalFvsRecords();
    return cnt;
}

int SimpleCompressedKb::totalFvsRecords() const {
    int cnt = 0;
    for (int rel_id = 0; rel_id < originalKb->totalRelations(); rel_id++) {
        cnt += fvsRecords[rel_id].size();
    }
    return cnt;
}

int SimpleCompressedKb::totalCounterexamples() const {
    int cnt = 0;
    for (int rel_id = 0; rel_id < originalKb->totalRelations(); rel_id++) {
        cnt += counterexampleSets[rel_id].size();
    }
    return cnt;
}

int SimpleCompressedKb::totalHypothesisSize() const {
    int cnt = 0;
    for (Rule* const& rule: hypothesis) {
        cnt += rule->getLength();
    }
    return cnt;
}

int SimpleCompressedKb::totalSupplementaryConstants() const {
    return supplementaryConstants.size();
}

const char* SimpleCompressedKb::getName() const {
    return name;
}

const std::vector<int>& SimpleCompressedKb::getSupplementaryConstants() const {
    return supplementaryConstants;
}

size_t SimpleCompressedKb::memoryCost() const {
    size_t size = sizeof(SimpleCompressedKb) + sizeof(char) * (strlen(name) + 1) + 
        sizeof(Rule*) * hypothesis.size() + 
        (sizeof(std::vector<int*>) + sizeof(std::unordered_set<Record>)) * originalKb->totalRelations() + 
        sizeof(int) * supplementaryConstants.size();
    for (Rule* const& rule: hypothesis) {
        size += rule->memoryCost();
    }
    for (int i = 0; i < originalKb->totalRelations(); i++) {
        SimpleRelation* relation = originalKb->getRelation(i);
        std::vector<int*> const& fvs_records = fvsRecords[i];
        std::unordered_set<Record> const& counterexample_set = counterexampleSets[i];
        size += sizeof(int*) * fvs_records.size();
        size += (sizeof(Record) + sizeof(int) * relation->getTotalCols()) * counterexample_set.size();
    }
    return size;
}
