#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "intTable.h"
#include <filesystem>
#include <unordered_set>
#include "../util/common.h"
#include "../rule/rule.h"

#define BITS_PER_INT (sizeof(int) * 8)
#define REL_INFO_FILE_NAME "Relations.tsv"
#define REL_DATA_FILE_SUFFIX ".rel"
#define MAP_FILE_NUMERATION_START 1
#define MAX_MAP_ENTRIES 1000000
#define MAP_FILE_PREFIX "map"
#define MAP_FILE_SUFFIX ".tsv"
#define COUNTEREXAMPLE_FILE_SUFFIX ".ceg"
#define HYPOTHESIS_FILE_NAME "rules.hyp"
#define SUPPLEMENTARY_CONSTANTS_FILE_NAME "supplementary.cst"
#define DEFAULT_MIN_CONSTANT_COVERAGE 0.25

#define NUM_FLAG_INTS(NUM) (NUM / BITS_PER_INT + ((0 == NUM % BITS_PER_INT) ? 0 : 1))

using std::filesystem::path;

namespace sinc {
    /**
     * Exception class thrown when errors occur within KB operations.
     *
     * @since 2.0
     */
    class KbException : public std::exception {
    public:
        KbException();
        KbException(const std::string& msg);
        virtual const char* what() const throw();

    private:
        std::string message;
    };

    /**
     * This class is used to denote which records are already entailed in the relation and which are not.
     *
     * @since 2.1
     */
    class SplitRecords {
    public:
        std::vector<int*>* const entailedRecords;
        std::vector<int*>* const nonEntailedRecords;

        /**
         * NOTE: The two pointers WILL be released by the created object.
         */
        SplitRecords(std::vector<int*>* const entailedRecords, std::vector<int*>* const nonEntailedRecords);
        ~SplitRecords();
    };

    /**
     * This class stores the records in a relation as an integer table.
     *
     * A relation can be dumped into local file system as a regular file. This complies with the following format:
     *   - The files are binary files, each of which only contains `arity`x`#records` integers.
     *   - The integers stored in one `.rel` file is row oriented, each row corresponds to one record in the relation. The
     *     records are stored in the file in order, i.e., in the order of: 1st row 1st col, 1st row 2nd col, ..., ith row
     *     jth col, ith row (j+1)th col, ...
     *
     * @since 2.1
     */
    class SimpleRelation : public IntTable {
    public:
        /** The threshold for pruning useful constants */
        static double minConstantCoverage;

        /** Relation name */
        const char* const name;
        /** The ID number of the relation */
        int const id;
        /** 
         * Denote whether records should be maintained by this object. If `true`, the records will be released in destructor.
         * 
         * This will be set to `true` if the object is constructed by loading from files.
         */
        bool maintainRecords;

        /**
         * This method loads a relation file as a 2D array of integers. Please refer to "KbRelation" for the file format.
         *
         * @param filePath     The file containing the relation data
         * @param arity        The arity of the relation
         * @param totalRecords The number of records in the relation
         */
        static int** loadFile(const path& filePath, int const arity, int const totalRecords);

        /**
         * Create a relation directly from a list of records
         */
        SimpleRelation(const std::string& name, int const id, int** records, int const arity, int const totalRecords);

        /**
         * Create a relation from a relation file
         * @throws IOException
         */
        SimpleRelation(const std::string& name, int const id, int const arity, int const totalRecords, const path& filePath);

        ~SimpleRelation();

        /**
         * Set a record as entailed if it is in the relation.
         * 
         * NOTE: Record SHOULD have the same arity as the relation.
         */
        void setAsEntailed(int* const record);

        /**
         * Set a record as not entailed if it is in the relation.
         * 
         * NOTE: Record SHOULD have the same arity as the relation.
         */
        void setAsNotEntailed(int* const record);

        /**
         * Set all records in the list as entailed in the relation if presented. The method WILL sort the records by the
         * alphabetical order.
         * 
         * NOTE: Records SHOULD have the same arity as the relation. Length should NOT be 0.
         * 
         * NOTE: This method may rearrange the order of the records
         */
        void setAllAsEntailed(int** const records, int const length);

        /**
         * Check whether a record is in the relation and is entailed.
         * 
         * NOTE: Record SHOULD have the same arity as the relation.
         */
        bool isEntailed(int* const record) const;

        /**
         * If the record is in the relation and has not been marked as entailed, mark the record as entailed and return true.
         * Otherwise, return false.
         * 
         * NOTE: Record SHOULD have the same arity as the relation.
         */
        bool entailIfNot(int* const record);

        /**
         * Return the total number of entailed records in this relation.
         */
        int totalEntailedRecords() const;

        /**
         * Find the promising constants according to current records.
         * 
         * NOTE: The array and the pointers in the array SHOULD be maintained by USER.
         * 
         * @return Return an array of pointers to `std::vector<int>`. The length of the array is the arity of the relation, which is
         * `getTotalCols()`.
         */
        std::vector<int>** getPromisingConstants() const;

        /**
         * A helper function that releases the pointers in the promising constants array
         */
        static void releasePromisingConstants(std::vector<int>** promisingConstants, int const arity);

        /**
         * Dump all records to a binary file. The format is the same as "KbRelation".
         *
         * @param filePath The path to where the relation file should be stored.
         */
        void dump(const path& filePath) const;

        /**
         * Write the records that are not entailed and identified by FVS to a binary file. The format is the same as
         * "KbRelation".
         *
         * @param filePath The path to where the relation file should be stored.
         */
        void dumpNecessaryRecords(const path& filePath, const std::vector<int*>& fvsRecords) const;

        /**
         * Lable constants that have been reserved in the remaining records.
         * 
         * NOTE: The number of bits in the array should be sufficient for the number of constants in the relation.
         */
        void setFlagOfReservedConstants(int* const flags) const;

        /**
         * Split rows in the relation by whether they are flagged as entailed.
         * 
         * NOTE: The returned pointer SHOULD be maintained by USER.
         */
        SplitRecords* splitByEntailment() const;

        size_t memoryCost() const override;

    protected:
        /** The flags are used to denote whether a record has been marked entailed */
        int* const entailmentFlags;
        /** The number of integers of the array `entailmentFlags` */
        int const flagLength;

        /**
         * Set the idx-th bit corresponding as true.
         */
        void setEntailmentFlag(int const idx);

        /**
         * Set the idx-th bit corresponding as false.
         */
        void unsetEntailmentFlag(int const idx);

        /**
         * Get the entailment bit of the idx-th record. The parameter should satisfy: 0 <= idx < totalRows.
         *
         * @return 0 if the bit is 0, non-zero otherwise.
         */
        int entailment(int const idx) const;
    };

    /**
     * A simple in-memory, read-only KB. The values in the KB are converted to integers so each relation in the KB is a 2D
     * table of integers. The estimated size of the memory cost, at the worst case, is about 3 times the size of the disk
     * space taken by all relation files.
     *
     * Given that the constant numerations are in a continuous integer span [1, n], only one integer, n, denoting the total
     * number of constants are needed to refer to all constants.
     * 
     * A KB can be dumped into the local file system. A dumped KB is a directory (named by the KB name) containing multiple
     * files:
     *   - The numeration mapping file: Please refer to class `sinc2.util.kb.NumerationMap` in Java project `SInC`.
     *   - The relation name file "Relations.tsv" : Relation information will be listed in order in this file (i.e., the
     *   line numbers are the IDs of relations). The columns are:
     *     1) relation name
     *     2) arity
     *     3) total records.
     *   - The relation files: The names of the relation files are "<ID>.rel", as the relation names may contain illegal
     *     characters for file names. Please refer to class 'sinc2.util.kb.KbRelation' for structure details.
     *   - Meta information files:
     *       - There may be multiple files with extension `.meta` to store arbitrary meta information of the KB.
     *       - The files are customized by other utilities and are not in a fixed format.
     *
     * The dump of the numerated KB ensures that the numeration of all entities are mapped into a continued integer span:
     * (0, n], where n is the total number of different entity names
     *
     * @since 2.1
     */
    class SimpleKb {
    public:
        /**
         * Get the path object to the dir of a KB
         */
        static path getKbDirPath(const char* const kbName, const path& basePath);

        /**
         * Get the path object to the relation information file of a KB.
         */
        static path getRelInfoFilePath(const char* const kbName, const path& basePath);

        /**
         * Get the path object to a relation data file
         */
        static path getRelDataFilePath(int const relId, const char* const kbName, const path& basePath);

        /**
         * Get the path object to a mapping file from integers to constant names
        */
        static path getMapFilePath(const path& kbDirPath, int const mapNum);

        /**
         * Load a KB from local file system.
         *
         * @param name The name of the KB
         * @param basePath The base path to the dir of the KB
        */
        SimpleKb(const std::string& name, const path& basePath);

        /**
         * Create a KB from a list of relations (a relation here is a list of int arrays).
         *
         * NOTE: Suppose the integers in the relations lie in the integer span: [1, n], each integer "i" in [1, n] should
         * appear in at least one argument. Otherwise, the number of constants will NOT be correct.
         *
         * @param kbName             The name of the KB
         * @param relations          The list of relations
         * @param relNames           The list of names of the corresponding relations
         */
        SimpleKb(
            const std::string& name, int*** const relations, std::string* relNames, int* const arities, int* const totalRows,
            int const numReltaions
        );

        SimpleKb(const SimpleKb& another);

        ~SimpleKb();

        /**
         * Dump the KB into local file system.
         * 
         * @param mappedNames An array of names of the constant numerations. `mappedNames[i]` is the name of constant `i`.
         */
        void dump(const path& basePath, std::string* mappedNames) const;

        /**
         * @return `nullptr` if the relation does not exist
         */
        SimpleRelation* getRelation(const std::string& name) const;

        /**
         * @return `nullptr` if the relation does not exist
         */
        SimpleRelation* getRelation(int const id) const;

        /**
         * NOTE: `record` must have the same arity as the relation.
         */
        bool hasRecord(const std::string& relationName, int* const record) const;

        /**
         * NOTE: `record` must have the same arity as the relation.
         */
        bool hasRecord(int const relationId, int* const record) const;

        /**
         * NOTE: `record` must have the same arity as the relation.
         */
        void setAsEntailed(const std::string& relationName, int* const record);

        /**
         * NOTE: `record` must have the same arity as the relation.
         */
        void setAsEntailed(int const relationId, int* const record);

        /**
         * NOTE: `record` must have the same arity as the relation.
         */
        void setAsNotEntailed(const std::string& relationName, int* const record);

        /**
         * NOTE: `record` must have the same arity as the relation.
         */
        void setAsNotEntailed(int const relationId, int* const record);

        void updatePromisingConstants();

        /**
         * NOTE: the pointers in the array should NOT be released by USER.
         */
        std::vector<int>** getPromisingConstants(int relId) const;

        /**
         * NOTE: the pointer should NOT be released by USER.
         */
        const char* getName() const;

        /**
         * NOTE: the pointer should NOT be released by USER.
         */
        std::vector<SimpleRelation*>* getRelations() const;

        int totalRelations() const;

        int totalRecords() const;

        int totalConstants() const;

        const char* const * getRelationNames() const;

        size_t memoryCost() const;

    protected:
        /** The name of the KB */
        const char* const name;
        /** The list of relation pointers. The ID of each relation is its index in the list */
        std::vector<SimpleRelation*>* const relations;
        const char** relationNames;
        /** The map from relation names to IDs */
        std::unordered_map<std::string, SimpleRelation*>* const relationNameMap;
        /** 
         * The list of promising constants in the corresponding relations. The i-th promising constant in the c-th column of the r-th
         * relation is at `(*promisingConstants[r][c])[i]`
         */
        std::vector<int>*** promisingConstants;
        /** The total number of constants in the KB */
        int constants;

        /**
         * Release the memory resources of promising constants;
         */
        void releasePromisingConstants();
    };

    /**
     * This class is for the compressed KB. It extends the numerated KB in four perspectives:
     *   1. The compressed KB contains counterexample relations:
     *      - The counterexamples are stored into '.ceg' files. The file format is the same as '.rel' files. The names of the
     *        files are '<relation ID>.ceg'.
     *      - If there is no counterexample in a relation, the counterexample relation file will not be created.
     *   2. The compressed KB contains a hypothesis set:
     *      - The hypothesis set is stored into a 'rules.hyp' file. The rules are written in the form of plain text, one per
     *        line.
     *      - If there is no rule in the hypothesis set, the file will not be created.
     *   3. The compressed KB contains a supplementary constant set:
     *      - The supplementary constant set is stored into a 'supplementary.cst' file. Constant numerations in the mapping
     *        are stored in the file.
     *        Note: The numeration mapping in a compressed KB contains all mappings as the original KB does.
     *      - If there is no element in the supplementary set, the file will not be created.
     *   4. The relation info file is extended by a new column, referring to the number of corresponding counterexamples.
     * The necessary facts are stored as is in the original KB.
     * 
     * NOTE: Pointers to rules in `hypothesis` and pointers to arguments of records in `counterexampleSets` SHOULD be maintained by
     * the `SimpleCompressedKb` object.
     * 
     * NOTE: Pointers to `int` arrays in `fvsRecords` are from existing records in the original KB, thus are NOT maintained by
     * `SimpleCompressedKb` object.
     *
     * @since 2.1
     */
    class SimpleCompressedKb {
    public:
        static path getCounterexampleFilePath(int const relId, const char* const kbName, const path& basePath);

        /**
         * Construct an empty compressed KB.
         *
         * @param name       The name of the compressed KB
         * @param originalKb The original KB
         */
        SimpleCompressedKb(const std::string& name, SimpleKb* const originalKb);

        ~SimpleCompressedKb();

        /**
         * Add a single record identified by FVS solver to `fvsRecords`.
         * 
         * NOTE: The pointer `record` SHOULD be maintained by USER.
         */
        void addFvsRecord(int const relId, int* const record);

        /**
         * Add some counterexamples to the KB.
         * 
         * NOTE: Pointers to arguments of records in `records` ARE now maintained by the `SimpleCompressedKb` object and should NOT
         * released by USER.
         *
         * @param relId   The id of the corresponding relation
         * @param records The counterexamples
         */
        void addCounterexamples(int const relId, const std::unordered_set<Record>& records);

        void addHypothesisRules(const std::vector<Rule*>& rules);

        /**
         * Update the supplementary constant set.
         */
        void updateSupplementaryConstants();

        /**
         * NOTE: This function will call `updateSupplementaryConstants()`
         */
        void dump(const path& basePath);

        std::vector<Rule*>& getHypothesis();

        std::unordered_set<Record>& getCounterexampleSet(int const relId);

        int totalNecessaryRecords() const;

        int totalFvsRecords() const;

        int totalCounterexamples() const;

        int totalHypothesisSize() const;

        /** NOTE: `updateSupplementaryConstants()` should be called before calling this function */
        int totalSupplementaryConstants() const;

        const char* getName() const;

        const std::vector<int>& getSupplementaryConstants() const;

        size_t memoryCost() const;

    protected:
        /** The name of the compressed KB */
        const char* const name;
        /** The reference to the original KB. The original KB is used for determining the necessary records and the missing
         *  constants. */
        SimpleKb* const originalKb;
        /** The hypothesis set, i.e., a list of rules.
         * 
         *  NOTE: The pointers in the list SHOULD be maintained by the object.
         */
        std::vector<Rule*> hypothesis;
        /** The records included by FVS in each corresponding relation */
        std::vector<int*>* fvsRecords;
        /** The counterexample sets. The ith set is correspondent to the ith relation in the original KB. */
        std::unordered_set<Record>* counterexampleSets;
        /** The constants marked in a supplementary set. Otherwise, they are lost due to removal of facts. */
        std::vector<int> supplementaryConstants;
    };
}