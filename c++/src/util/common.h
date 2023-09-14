#pragma once

#include <functional>
#include <unordered_set>
#include <unordered_map>

/**
 * The following macros in namespace "sinc" specify debug levels, indicating "conditional compilation". The higher the level is,
 * the more debugging information will be recorded. Higher level information contains all lower level info.
 *
 * @since 2.0
 */
namespace sinc {
    #define DEBUG_NONE 0
    #define DEBUG_VERBOSE 1
    #define DEBUG_DEBUG 2
    #define DEBUG_LEVEL DEBUG_NONE
}

/**
 * The namespace "sinc" is for all elements used in SInC system.
 */
namespace sinc {
    /**
     * This function releases all pointers in the set and clear the set.
     */
    template<class T>
    void clearSet(std::unordered_set<T*>& set);
}

/**
 * The following definitions in name space "sinc" specify conventions of argument structures and operations.
 *
 * Integer value 0 is a special value denoting empty argument, other values using the lowest 31 bits are numerations
 * mapped to constant symbols and variables. The highest bit is a flag distinguishing variables from constants. If the
 * highest bit is 0, the argument value is a constant symbol; otherwise, it is a variable ID.
 *
 * The range of applicable constant is [1, 2^31 - 1]; [0, 2^31 - 1] for variable IDs.
 *
 * NOTE: only arguments in Horn rules should use this class to encode.
 *
 */
namespace sinc {
    /** Zero is a special value denoting an empty argument, which is neither constant nor variable. */
    #define ARG_EMPTY_VALUE 0

    /** The highest bit of an integer flags a variable if it is 1. */
    #define ARG_FLAG_VARIABLE (1 << 31)

    /** The highest bit of an integer flags a constant if it is 0. */
    #define ARG_FLAG_CONSTANT (~ARG_FLAG_VARIABLE)

    /**
     * Encode a constant argument by its numeration value. The encoding discards the highest bit.
     * 
     * NOTE: Encoding is needed only when the arguments are used in predicates in rules.
     *
     * @param N Numeration value of the constant symbol
     */
    #define ARG_CONSTANT(N) (ARG_FLAG_CONSTANT & (N))

    /**
     * Encode a variable argument by the variable ID. The encoding discards the highest bit.
     *
     * NOTE: Encoding is needed only when the arguments are used in predicates in rules.
     *
     * @param N Variable ID
     */
    #define ARG_VARIABLE(N) (ARG_FLAG_VARIABLE | (N))

    /**
     * Check if the argument is empty.
     */
    #define ARG_IS_EMPTY(A) (0 == (A))

    /**
     * Check if the argument is non-empty.
     */
    #define ARG_IS_NON_EMPTY(A) (0 != (A))

    /**
     * Check if the argument is a variable (specifically, a limited variable; an unlimited variable is denoted by empty
     * value).
     */
    #define ARG_IS_VARIABLE(A) (0 != (ARG_FLAG_VARIABLE & (A)))

    /**
     * Check if the argument is a constant.
     */
    #define ARG_IS_CONSTANT(A) (0 != A && 0 == (ARG_FLAG_VARIABLE & (A)))

    /**
     * Get the encoded value in the argument.
     */
    #define ARG_DECODE(A) (ARG_FLAG_CONSTANT & (A))
}

namespace sinc {
    /**
     * Wrapper class for an integer array, which is, a record.
     * 
     * @since 2.0
     */
    class Record {
    public:
        int* const args;
        int const arity;

        Record(int arity);
        Record(int *args, int arity);
        ~Record();

        /**
         * This method copies the contents in "newArgs" to the array. It assumes the two arrays have the same length.
         */
        void setArgs(int *newArgs);

        bool operator==(const Record &another) const;
        size_t hash() const;
    };
}

using sinc::Record;
/**
 * This is for hashing "Record" in unordered containers.
 */
template <>
struct std::hash<Record> {
    size_t operator()(const Record& r) const;
};

/**
 * This is for hashing "Record*" in unordered containers.
 */
template <>
struct std::hash<Record*> {
    size_t operator()(const Record *r) const;
};

/**
 * This is for checking equivalence "Record*" in unordered containers.
 */
template <>
struct std::equal_to<Record*> {
    bool operator()(const Record *r1, const Record *r2) const;
};

namespace sinc {
    /**
     * The class for predicates. The predicate symbol and the arguments in a predicate are represented by the numerations mapped to
     * the names.
     *
     * @since 1.0
     */
    class Predicate {
    public:
        int const predSymbol;
        int* const args;
        int const arity;

        /**
         * Initialize by the predicate symbol and the arguments specifically.
         */
        Predicate(int const predSymbol, int* const args, int const arity);

        /**
         * Initialize by the functor and empty arguments (indicated by the arity).
         *
         * @param arity The arity of the predicate
         */
        Predicate(int const predSymbol, int const arity);

        Predicate(const Predicate& another);

        ~Predicate();

        bool operator==(const Predicate &another) const;
        size_t hash() const;

        std::string toString() const;
        //Todo: std::string toString(const NumerationMap& map) const;
        //Todo: std::string toString(const SimpleKb& kb) const;
    };
}

using sinc::Predicate;
/**
 * This is for hashing "Predicate" in unordered containers.
 */
template <>
struct std::hash<Predicate> {
    size_t operator()(const Predicate& r) const;
};

/**
 * This is for hashing "Predicate*" in unordered containers.
 */
template <>
struct std::hash<Predicate*> {
    size_t operator()(const Predicate *r) const;
};

/**
 * This is for checking equivalence "Predicate*" in unordered containers.
 */
template <>
struct std::equal_to<Predicate*> {
    bool operator()(const Predicate *r1, const Predicate *r2) const;
};

namespace sinc {
    /**
     * This exception will be thrown when SInC runtime gets errors.
     * 
     * @since 1.0
     */
    class SincException : public std::exception {
    public:
        SincException();
        SincException(const std::string& msg);
        virtual const char* what() const throw();

    private:
        std::string message;
    };

    /**
     * Interruption exception thrown when the workflow of SInC is interrupted. On receiving the interruption signal, SInC
     * stops the current rule mining procedure and compress the KB by the current hypothesis set.
     *
     * @since 1.0
     */
    class InterruptionSignal : public std::exception {
    public:
        InterruptionSignal();
        InterruptionSignal(const std::string& msg);
        virtual const char* what() const throw();

    private:
        std::string message;
    };
}

namespace sinc {
    /**
     * This class denotes the arguments parsed from plain-text strings.
     *
     * @since 2.0
     */
    class ParsedArg {
    public:
        /** If the argument is a constant, the name denotes the constant symbol. Otherwise, it is nullptr */
        const char* const name;
        /** If the argument is a variable, the id denotes the variable id. */
        int const id;

        ParsedArg(const ParsedArg& another);
        ~ParsedArg();

        /**
         * Create an instance for a constant symbol.
         */
        static ParsedArg* constant(const std::string& constSymbol);

        /**
         * Create an instance for a variable.
         */
        static ParsedArg* variable(int const id);

        bool isVariable() const;
        bool isConstant() const;

        bool operator==(const ParsedArg &another) const;
        size_t hash() const;
        std::string toString() const;

    private:
        ParsedArg(const char* const name, int const id);
    };
}

using sinc::ParsedArg;
/**
 * This is for hashing "ParsedArg" in unordered containers.
 */
template <>
struct std::hash<ParsedArg> {
    size_t operator()(const ParsedArg& r) const;
};

/**
 * This is for hashing "ParsedArg*" in unordered containers.
 */
template <>
struct std::hash<ParsedArg*> {
    size_t operator()(const ParsedArg *r) const;
};

/**
 * This is for checking equivalence "ParsedArg*" in unordered containers.
 */
template <>
struct std::equal_to<ParsedArg*> {
    bool operator()(const ParsedArg *r1, const ParsedArg *r2) const;
};

namespace sinc {
    /**
     * This class is for the predicates parsed from plain-text strings.
     *
     * @since 2.0
     */
    class ParsedPred {
    public:
        /** The name of the predicate symbol */
        std::string const predSymbol;
        /**
         * The list of arguments. Elements in the array will be deleted in the destructor of this class. 
         * 
         * NOTE: When replacing elements in the argument list, remember to delete replaced elements manually.
         */
        ParsedArg** const args;
        int const arity;

        /**
         * Construct by explicitly assign a list of arguments.
         * 
         * @param args This list and elements within will be released by the created object
         */
        ParsedPred(const std::string& predSymbol, ParsedArg** const args, int const arity);

        /**
         * Construct a predicate and create an empty arguement list.
         */
        ParsedPred(const std::string& predSymbol, int const arity);
        ParsedPred(const ParsedPred& another);
        ~ParsedPred();

        bool operator==(const ParsedPred &another) const;
        size_t hash() const;
        std::string toString() const;
    };
}

using sinc::ParsedPred;
/**
 * This is for hashing "ParsedPred" in unordered containers.
 */
template <>
struct std::hash<ParsedPred> {
    size_t operator()(const ParsedPred& r) const;
};

/**
 * This is for hashing "ParsedPred*" in unordered containers.
 */
template <>
struct std::hash<ParsedPred*> {
    size_t operator()(const ParsedPred *r) const;
};

/**
 * This is for checking equivalence "ParsedPred*" in unordered containers.
 */
template <>
struct std::equal_to<ParsedPred*> {
    bool operator()(const ParsedPred *r1, const ParsedPred *r2) const;
};

namespace sinc {
    /**
     * Objects of this class denote locations of arguments in a rule.
     *
     * @since 2.0
     */
    class ArgLocation {
    public:
        int const predIdx;
        int const argIdx;

        ArgLocation(int const predIdx, int const argIdx);

        bool operator==(const ArgLocation &another) const;
        size_t hash() const;
    };
}

using sinc::ArgLocation;
/**
 * This is for hashing "ArgLocation" in unordered containers.
 */
template <>
struct std::hash<ArgLocation> {
    size_t operator()(const ArgLocation& r) const;
};

/**
 * This is for hashing "ArgLocation*" in unordered containers.
 */
template <>
struct std::hash<ArgLocation*> {
    size_t operator()(const ArgLocation *r) const;
};

/**
 * This is for checking equivalence "ArgLocation*" in unordered containers.
 */
template <>
struct std::equal_to<ArgLocation*> {
    bool operator()(const ArgLocation *r1, const ArgLocation *r2) const;
};

namespace sinc {
    /**
     * The indicator class used in the fingerprint of rules. An indicator denotes a constant or a location in a predicate.
     *
     * @since 1.0
     */
    class ArgIndicator {
    public:
        int const functor;
        int const idx;

        /**
         * Create an indicator of a constant
         *
         * @param constNumeration The numeration of the constant name
         * @return The constant indicator
         */
        static ArgIndicator* constantIndicator(int const constNumeration);

        /**
         * Create an indicator of a variable
         *
         * @param functor The numeration of the functor name
         * @param idx The argument index of the variable in the predicate
         * @return The variable indicator
         */
        static ArgIndicator* variableIndicator(int const functor, int const idx);

        bool operator==(const ArgIndicator &another) const;
        size_t hash() const;

    private:
        ArgIndicator(int const functor, int const idx);
    };
}

using sinc::ArgIndicator;
/**
 * This is for hashing "ArgIndicator" in unordered containers.
 */
template <>
struct std::hash<ArgIndicator> {
    size_t operator()(const ArgIndicator& r) const;
};

/**
 * This is for hashing "ArgIndicator*" in unordered containers.
 */
template <>
struct std::hash<ArgIndicator*> {
    size_t operator()(const ArgIndicator *r) const;
};

/**
 * This is for checking equivalence "ArgIndicator*" in unordered containers.
 */
template <>
struct std::equal_to<ArgIndicator*> {
    bool operator()(const ArgIndicator *r1, const ArgIndicator *r2) const;
};
