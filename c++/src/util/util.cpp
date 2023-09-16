#include "util.h"
#include <unordered_map>
#include "common.h"
#include <fstream>

/**
 * MultiSet
 */
using sinc::MultiSet;
template<class T>
MultiSet<T>::MultiSet() : cntMap(maptype()), size(0) {}

template<class T>
MultiSet<T>::MultiSet(const MultiSet<T>& another) : cntMap(maptype(another.cntMap)), size(another.size) {}

template<class T>
MultiSet<T>::MultiSet(T* const elements, const int length) : cntMap(maptype()), size(0) {
    addAll(elements, length);
}

template<class T>
int MultiSet<T>::add(const T& element) {
    std::pair<typename std::unordered_map<T, int>::iterator, bool> ret = cntMap.emplace(element, 1);
    if (!ret.second) {
        ++(ret.first->second);
    }
    size++;
    return ret.first->second;
}

template<class T>
void MultiSet<T>::addAll(T* const elements, const int length) {
    for (int i = 0; i < length; i++) {
        add(elements[i]);
    }
}

template<class T>
void MultiSet<T>::addAll(const MultiSet<T>& another) {
    for (auto& kv: another.cntMap) {
        std::pair<typename std::unordered_map<T, int>::iterator, bool> ret = cntMap.emplace(kv.first, kv.second);
        size += kv.second;
        if (!ret.second) {
            ret.first->second += kv.second;
        }
    }
}

template<class T>
int MultiSet<T>::remove(const T& element) {
    typename std::unordered_map<T, int>::iterator itr = cntMap.find(element);
    if (itr != cntMap.end()) {
        size--;
        if (1 == itr->second) {
            cntMap.erase(itr);
            return 0;
        } else {
            return --(itr->second);
        }
    }
    return 0;
}

template<class T>
int MultiSet<T>::getSize() {
    return size;
}

template<class T>
int MultiSet<T>::differentValues() {
    return cntMap.size();
}

template<class T>
bool MultiSet<T>::subsetOf(const MultiSet<T>& another) {
    if (cntMap.size() > another.cntMap.size() || size > another.size) {
        return false;
    }
    for (auto itr = cntMap.begin(); itr != cntMap.end(); itr++) {
        typename std::unordered_map<T, int>::const_iterator another_itr = another.cntMap.find(itr->first);
        if (another_itr == another.cntMap.end() || another_itr->second < itr->second) {
            return false;
        }
    }
    return true;
}

template<class T>
int MultiSet<T>::itemCount(const T& element) {
    typename std::unordered_map<T, int>::iterator itr = cntMap.find(element);
    return (itr != cntMap.end()) ? itr->second : 0;
}

template<class T>
int MultiSet<T>::itemCount(T* const elements, int const length) {
    int total = 0;
    for (int i = 0; i < length; i++) {
        typename std::unordered_map<T, int>::iterator itr = cntMap.find(elements[i]);
        total += (itr != cntMap.end()) ? itr->second : 0;
    }
    return total;
}

template<class T>
const typename MultiSet<T>::maptype& MultiSet<T>::getCntMap() {
    return cntMap;
}

template<class T>
bool MultiSet<T>::operator==(const MultiSet<T> &another) const {
    if (size != another.size) {
        return false;
    }
    for (std::pair<T, int> const& kv: cntMap) {
        typename maptype::const_iterator itr = another.cntMap.find(kv.first);
        if (another.cntMap.end() == itr || kv.second != itr->second) {
            return false;
        }
    }
    return true;
    // return size == another.size && cntMap == another.cntMap;
}

template<class T>
size_t MultiSet<T>::hash() const {
    size_t h = size * 31 + cntMap.size();
    size_t _h = 0;
    std::hash<T> hasher;
    for (auto itr = cntMap.begin(); itr != cntMap.end(); itr++) {
        _h += hasher(itr->first);
    }
    h = h * 31 + _h;
    return h;
}

template<class T>
size_t std::hash<MultiSet<T>>::operator()(const MultiSet<T>& r) const {
    return r.hash();
}

template<class T>
size_t std::hash<MultiSet<T>*>::operator()(const MultiSet<T> *r) const {
    return r->hash();
}

template<class T>
bool std::equal_to<MultiSet<T>*>::operator()(const MultiSet<T> *r1, const MultiSet<T> *r2) const {
    return (*r1) == (*r2);
}

/* Instantiate MultiSet<T> by the following types, so the linker would not be confused: */
template class MultiSet<int>;
template class std::hash<MultiSet<int>>;
template class std::hash<MultiSet<int>*>;
template class std::equal_to<MultiSet<int>*>;
template class MultiSet<sinc::ArgLocation>;
template class std::hash<MultiSet<sinc::ArgLocation>>;
template class std::hash<MultiSet<sinc::ArgLocation>*>;
template class std::equal_to<MultiSet<sinc::ArgLocation>*>;
template class MultiSet<sinc::ArgIndicator*>;
template class std::hash<MultiSet<sinc::ArgIndicator*>>;
template class std::hash<MultiSet<sinc::ArgIndicator*>*>;
template class std::equal_to<MultiSet<sinc::ArgIndicator*>*>;
template class MultiSet<sinc::MultiSet<sinc::ArgIndicator*>*>;
template class std::hash<MultiSet<sinc::MultiSet<sinc::ArgIndicator*>*>>;
template class std::hash<MultiSet<sinc::MultiSet<sinc::ArgIndicator*>*>*>;
template class std::equal_to<MultiSet<sinc::MultiSet<sinc::ArgIndicator*>*>*>;

/**
 * IntWriter
 */
using sinc::IntWriter;
IntWriter::IntWriter(const char* filePath) : ofs(std::ofstream(filePath, std::ios::out | std::ios::binary)) {}

void IntWriter::write(const int& i) {
    ofs.put(i);
    ofs.put(i >> 8);
    ofs.put(i >> 16);
    ofs.put(i >> 24);
}

void IntWriter::close() {
    ofs.close();
}

/**
 * IntReader
 */
using sinc::IntReader;
IntReader::IntReader(const char* filePath) : ifs(std::ifstream(filePath, std::ios::in | std::ios::binary)) {}

int IntReader::next() {
    int i = ifs.get() & 0xff;
    i |= (ifs.get() & 0xff) << 8;
    i |= (ifs.get() & 0xff) << 16;
    i |= ifs.get() << 24;
    return i;
}

void IntReader::close() {
    ifs.close();
}

/**
 * DisjointSet
 */
using sinc::DisjointSet;
DisjointSet::DisjointSet(int capacity) : sets(new int[capacity]), length(capacity) {
    for (int i = 0; i < length; i++) {
        sets[i] = i;
    }
}

DisjointSet::~DisjointSet() {
    delete[] sets;
}

int DisjointSet::findSet(int idx) {
    while (sets[idx] != idx) {
        idx = sets[idx];
    }
    return idx;
}

void DisjointSet::unionSets(int idx1, int idx2) {
    int set1 = findSet(idx1);
    int set2 = findSet(idx2);
    if (set1 != set2) {
        sets[set1] = set2;
    }
}

int DisjointSet::totalSets() {
    int cnt = 0;
    for (int i = 0; i < length; i++) {
        if (i == sets[i]) {
            cnt++;
        }
    }
    return cnt;
}

/**
 * ComparableArray
 */
using sinc::ComparableArray;

template<class T>
ComparableArray<T>::ComparableArray(T* _arr, int const _length) : arr(_arr), length(_length) {}

template<class T>
ComparableArray<T>::~ComparableArray() {
    delete[] arr;
}

template<class T>
bool ComparableArray<T>::operator==(const ComparableArray<T> &another) const {
    if (length != another.length) {
        return false;
    }
    for (int i = 0; i < length; i++) {
        if (!(arr[i] == another.arr[i])) {
            return false;
        }
    }
    return true;
}

template<class T>
size_t ComparableArray<T>::hash() const {
    size_t h = length;
    std::hash<T> hasher;
    for (int i = 0; i < length; i++) {
        h = h * 31 + hasher(arr[i]);
    }
    return h;
}

template<class T>
size_t std::hash<ComparableArray<T>>::operator()(const ComparableArray<T>& r) const {
    return r.hash();
}

template<class T>
size_t std::hash<ComparableArray<T>*>::operator()(const ComparableArray<T> *r) const {
    return r->hash();
}

template<class T>
bool std::equal_to<ComparableArray<T>*>::operator()(const ComparableArray<T> *r1, const ComparableArray<T> *r2) const {
    return (*r1) == (*r2);
}

/* Instantiate ComparableArray<T> by the following types, so the linker would not be confused: */
template class ComparableArray<sinc::Record>;
template class std::hash<ComparableArray<sinc::Record>>;
template class std::hash<ComparableArray<sinc::Record>*>;
template class std::equal_to<ComparableArray<sinc::Record>*>;