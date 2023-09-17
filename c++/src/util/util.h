#pragma once

#include <unordered_map>
#include <vector>
#include <fstream>
#include <array>

namespace sinc {
    /**
     * The multi-set class. Member functions are designed for logic rule mining. In SInC, the template class T will
     * only be instantiate to "int" and "ArgIndicator". Thus, objects in this container will directly store values via
     * copy constructor.
     *
     * @param <T> The type of elements in the set
     *
     * @since 1.0
     */
    template <class T>
    class MultiSet {
    public:
        typedef std::unordered_map<T, int> maptype;

        MultiSet();
        MultiSet(const MultiSet<T>& another);
        MultiSet(T* const elements, const int length);

        /**
         * @return The number of `element` after insertion.
         */
        int add(const T& element);
        void addAll(T* const elements, const int length);
        void addAll(const MultiSet<T>& another);

        /**
         * Remove an element "element" from the multiset.
         *
         * @return the number of remaining elements that are the same as "element" in the multiset
         */
        int remove(const T& element);

        /**
         * Return the number of total elements in the set.
         */
        int getSize();

        /**
         * Return the number of different values in this set.
         */
        int differentValues();

        /**
         * Check whether this set is a subset of another.
         */
        bool subsetOf(const MultiSet<T>& another);

        /**
         * Return the number of elements that are equivalent to the given one.
         */
        int itemCount(const T& element);

        /**
         * Return the total number of elements that are equivalent to one of those in the array
         */
        int itemCount(T* const elements, int const length);

        /**
         * Get the map of the numbers of elements in this set
         */
        const maptype& getCntMap();

        bool operator==(const MultiSet &another) const;
        size_t hash() const;

    private:
        maptype cntMap;
        int size;
    };
}

/**
 * This is for hashing "MultiSet" in unordered containers.
 */
template <class T>
struct std::hash<sinc::MultiSet<T>> {
    size_t operator()(const sinc::MultiSet<T>& r) const;
};

/**
 * This is for hashing "MultiSet*" in unordered containers.
 */
template <class T>
struct std::hash<sinc::MultiSet<T>*> {
    size_t operator()(const sinc::MultiSet<T> *r) const;
};

/**
 * This is for checking equivalence "MultiSet*" in unordered containers.
 */
template <class T>
struct std::equal_to<sinc::MultiSet<T>*> {
    bool operator()(const sinc::MultiSet<T> *r1, const sinc::MultiSet<T> *r2) const;
};

namespace sinc {
    /**
     * This class is used for writing integers into binary files. Writing follows little endian protocol.
     *
     * @since 2.1
     */
    class IntWriter {
    public:
        /**
         * Construct a writer by the path to the target file
         */
        IntWriter(const char* filePath);

        /**
         * Write an integer to the file
         */
        void write(const int& i);

        void close();

    private:
        std::ofstream ofs;
    };

    /**
     * This class is used for reading integers from binary files. Given that the users of this class always know the total
     * number of integers that should be read from files, the reading method does not check the return value for better
     * performance.
     *
     * @since 2.1
     */
    class IntReader {
    public:
        /**
         * Construct a reader by the path to the target file
         */
        IntReader(const char* filePath);

        /**
         * Read an integer from file
         */
        int next();

        void close();

    private:
        std::ifstream ifs;
    };

    /**
     * The disjoint set algorithm that efficiently merges sets of elements.
     *
     * @since 1.0
     */
    class DisjointSet {
    public:
        /**
         * Create an instance by the number of elements.
         */
        DisjointSet(int capacity);

        ~DisjointSet();

        /**
         * Return the set label of the "idx"-th element
         */
        int findSet(int idx);

        /**
         * Union the two sets of the "idx1"-th and the "idx2"-th elements
         */
        void unionSets(int idx1, int idx2);

        /**
         * Return the total number of sets of the elements.
         */
        int totalSets();

    private:
        int* const sets;
        int length;
    };
}

namespace sinc {
    /**
     * A wrapper class for the array type that overrides the 'equal_to' and 'hash' method.
     *
     * @param <T> The type of the element in the array.
     *
     * @since 1.0
     */
    template<class T>
    class ComparableArray {
    public:
        T* const arr;
        int const length;

        /**
         * Construct by an array and the length. The pointer to the array will be maintained and freed by the "ComparableArray"
         * object
         */
        ComparableArray(T* arr, int const length);
        ~ComparableArray();

        bool operator==(const ComparableArray<T> &another) const;
        size_t hash() const;
    };
}

/**
 * This is for hashing "ComparableArray<T>" in unordered containers.
 */
template <class T>
struct std::hash<sinc::ComparableArray<T>> {
    size_t operator()(const sinc::ComparableArray<T>& r) const;
};

/**
 * This is for hashing "ComparableArray<T>*" in unordered containers.
 */
template <class T>
struct std::hash<sinc::ComparableArray<T>*> {
    size_t operator()(const sinc::ComparableArray<T> *r) const;
};

/**
 * This is for checking equivalence "ComparableArray<T>*" in unordered containers.
 */
template <class T>
struct std::equal_to<sinc::ComparableArray<T>*> {
    bool operator()(const sinc::ComparableArray<T> *r1, const sinc::ComparableArray<T> *r2) const;
};

namespace sinc {
    /**
     * This method copy the data in a vector to an array.
     * 
     * NOTE: The returned pointer should be deleted by "delete[]".
     */
    template <class T>
    T* toArray(std::vector<T> v) {
        T* arr = new T[v.size()];
        std::copy(v.begin(), v.end(), arr);
        return arr;
    }
}