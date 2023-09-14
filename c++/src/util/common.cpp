#include "common.h"
#include <cstring>
#include <sstream>

/* 
 * Container Helpers
 */
template<class T>
void sinc::clearSet(std::unordered_set<T*>& set) {
    // for (auto itr = set.begin(); itr != set.end(); itr++) {
    for (const auto itr: set) {
        T* ptr = *itr;
        delete ptr;
    }
    set.clear();
}

/*
 * Record
 */
using sinc::Record;
Record::Record(int a): arity(a), args(new int[a]{0}) {}

Record::Record(int *g, int a): args(g), arity(a) {}

Record::~Record() {
    delete[] args;
}

void Record::setArgs(int *newArgs) {
    for (int i = 0; i < arity; i++) {
        args[i] = newArgs[i];
    }
}

bool Record::operator==(const Record& another) const {
    if (this->arity != another.arity) {
        return false;
    }
    for (int i = 0; i < this->arity; i++) {
        if (this->args[i] != another.args[i]) {
            return false;
        }
    }
    return true;
}

size_t Record::hash() const {
    size_t h = arity;
    for (int i = 0; i < arity; i++) {
        h = h * 31 + args[i];
    }
    return h;
}

size_t std::hash<Record>::operator()(const Record& r) const {
    return r.hash();
}

size_t std::hash<Record*>::operator()(const Record *r) const {
    return r->hash();
}

bool std::equal_to<Record*>::operator()(const Record *r1, const Record *r2) const {
    return (*r1) == (*r2);
}

/*
 * Predicate
 */
using sinc::Predicate;
Predicate::Predicate(int const p, int* const g, int const a): predSymbol(p), args(g), arity(a) {}

Predicate::Predicate(int const p, int const a): predSymbol(p), args(new int[a]{0}), arity(a) {}

int* copyIntArray(int* const arr,int  const length) {
    int* _arr = new int[length];
    for (int i = 0; i < length; i++) {
        _arr[i] = arr[i];
    }
    return _arr;
}

Predicate::Predicate(const Predicate& another): predSymbol(another.predSymbol), args(copyIntArray(another.args, another.arity)), arity(another.arity) {}

Predicate::~Predicate() {
    delete[] args;
}

bool Predicate::operator==(const Predicate &another) const {
    if (this->predSymbol != another.predSymbol) {
        return false;
    }
    for (int i = 0; i < this->arity; i++) {
        if (this->args[i] != another.args[i]) {
            return false;
        }
    }
    return true;
}

size_t Predicate::hash() const {
    size_t h = predSymbol;
    h = h * 31 + arity;
    for (int i = 0; i < arity; i++) {
        h = h * 31 + args[i];
    }
    return h;
}

std::string Predicate::toString() const {
    std::ostringstream os;
    os << predSymbol << '(';
    if (0 < arity) {
        if (ARG_IS_EMPTY(args[0])) {
            os << '?';
        } else if (ARG_IS_VARIABLE(args[0])) {
            os << 'X' << ARG_DECODE(args[0]);
        } else {
            os << ARG_DECODE(args[0]);
        }
        for (int i = 1; i < arity; i++) {
            os << ',';
            if (ARG_IS_EMPTY(args[i])) {
                os << '?';
            } else if (ARG_IS_VARIABLE(args[i])) {
                os << 'X' << ARG_DECODE(args[i]);
            } else {
                os << ARG_DECODE(args[i]);
            }
        }
    }
    os << ')';
    return os.str();
}

size_t std::hash<Predicate>::operator()(const Predicate& r) const {
    return r.hash();
}

size_t std::hash<Predicate*>::operator()(const Predicate *r) const {
    return r->hash();
}

bool std::equal_to<Predicate*>::operator()(const Predicate *r1, const Predicate *r2) const {
    return (*r1) == (*r2);
}

/**
 * SincException
 */
using sinc::SincException;
SincException::SincException() : message("SInC Runtime Error") {}

SincException::SincException(const std::string& msg) : message(msg) {}

const char* SincException::what() const throw() {
    return message.c_str();
}

/**
 * InterruptionSignal
 */
using sinc::InterruptionSignal;
InterruptionSignal::InterruptionSignal() : message("SInC Interruption Signal") {}

InterruptionSignal::InterruptionSignal(const std::string& msg) : message(msg) {}

const char* InterruptionSignal::what() const throw() {
    return message.c_str();
}

/**
 * ParsedArg
 */
using sinc::ParsedArg;
ParsedArg::~ParsedArg() {
    if (nullptr != name) {
        delete name;
    }
}

ParsedArg* ParsedArg::constant(const std::string& constSymbol) {
    // char *cstr = new char [constSymbol.length()+1];
    // std::strcpy(cstr, constSymbol.c_str());
    return new ParsedArg(strdup(constSymbol.c_str()), 0);
}

ParsedArg* ParsedArg::variable(int const id) {
    return new ParsedArg(nullptr, id);
}

bool ParsedArg::isVariable() const {
    return nullptr == name;
}

bool ParsedArg::isConstant() const {
    return nullptr != name;
}

bool ParsedArg::operator==(const ParsedArg &another) const {
    if (id != another.id) {
        return false;
    }
    if (nullptr == name) {
        return nullptr == another.name;
    } else {
        return nullptr != another.name && 0 == std::strcmp(name, another.name);
    }
}

size_t ParsedArg::hash() const {
    size_t h = id;
    if (nullptr == name) {
        h *= 31;
    } else {
        for (int i = 0; 0 != name[i]; i++) {
            h = h * 31 + name[i];
        }
    }
    return h;
}

std::string ParsedArg::toString() const {
    std::ostringstream os;
    if (nullptr == name) {
        os << 'X' << id;
    } else {
        os << name;
    }
    return os.str();
}

ParsedArg::ParsedArg(const char* const _name, int const _id) : name(_name), id(_id) {}

ParsedArg::ParsedArg(const ParsedArg& another) : 
    name((nullptr == another.name)? nullptr : strdup(another.name)), id(another.id) {}

size_t std::hash<ParsedArg>::operator()(const ParsedArg& r) const {
    return r.hash();
}

size_t std::hash<ParsedArg*>::operator()(const ParsedArg *r) const {
    return r->hash();
}

bool std::equal_to<ParsedArg*>::operator()(const ParsedArg *r1, const ParsedArg *r2) const {
    return (*r1) == (*r2);
}

/**
 * ParsedPred
 */
using sinc::ParsedPred;
ParsedPred::ParsedPred(const std::string& _predSymbol, ParsedArg** const _args, int const _arity) : predSymbol(_predSymbol), args(_args), arity(_arity) {}

ParsedPred::ParsedPred(const std::string& _predSymbol, int const _arity) : predSymbol(_predSymbol), args(new ParsedArg*[_arity]{0}), arity(_arity) {}

ParsedArg** copyParsedArgs(ParsedArg** const args, int const length) {
    ParsedArg** _args = new ParsedArg*[length];
    for (int i = 0; i < length; i++) {
        _args[i] = (nullptr == args[i])? nullptr : new ParsedArg(*args[i]);
    }
    return _args;
}

ParsedPred::ParsedPred(const ParsedPred& another): predSymbol(another.predSymbol), args(copyParsedArgs(another.args, another.arity)), arity(another.arity) {}

ParsedPred::~ParsedPred() {
    for (int i = 0; i < arity; i++) {
        if (nullptr != args[i]) {
            delete args[i];
        }
    }
    delete[] args;
}

bool ParsedPred::operator==(const ParsedPred &another) const {
    if (predSymbol != another.predSymbol || arity != another.arity) {
        return false;
    }
    for (int i = 0; i < arity; i++) {
        if (nullptr == args[i]) {
            if (nullptr != another.args[i]) {
                return false;
            }
        } else if ((nullptr == another.args[i]) || !(*(args[i]) == *(another.args[i]))) {
            return false;
        }
    }
    return true;
}

size_t ParsedPred::hash() const {
    size_t h = std::hash<std::string>()(predSymbol);
    h = h * 31 + arity;
    for (int i = 0; i < arity; i++) {
        h = h * 31 + ((nullptr == args[i]) ? 0 : args[i]->hash());
    }
    return h;
}

std::string ParsedPred::toString() const {
    std::ostringstream os;
    os << predSymbol << '(';
    if (0 < arity) {
        if (nullptr == args[0]) {
            os << '?';
        } else {
            os << args[0]->toString();
        }
        for (int i = 1; i < arity; i++) {
            os << ',';
            if (nullptr == args[i]) {
                os << '?';
            } else {
                os << args[i]->toString();
            }
        }
    }
    os << ')';
    return os.str();
}

size_t std::hash<ParsedPred>::operator()(const ParsedPred& r) const {
    return r.hash();
}

size_t std::hash<ParsedPred*>::operator()(const ParsedPred *r) const {
    return r->hash();
}

bool std::equal_to<ParsedPred*>::operator()(const ParsedPred *r1, const ParsedPred *r2) const {
    return (*r1) == (*r2);
}

/**
 * ArgLocation
 */
using sinc::ArgLocation;
ArgLocation::ArgLocation(int const _predIdx, int const _argIdx): predIdx(_predIdx), argIdx(_argIdx) {}

bool ArgLocation::operator==(const ArgLocation &another) const {
    return predIdx == another.predIdx && argIdx == another.argIdx;
}
size_t ArgLocation::hash() const {
    size_t h = predIdx;
    h = h * 31 + argIdx;
    return h;
}

size_t std::hash<ArgLocation>::operator()(const ArgLocation& r) const {
    return r.hash();
}

size_t std::hash<ArgLocation*>::operator()(const ArgLocation *r) const {
    return r->hash();
}

bool std::equal_to<ArgLocation*>::operator()(const ArgLocation *r1, const ArgLocation *r2) const {
    return (*r1) == (*r2);
}

/**
 * ArgIndicator 
 */
ArgIndicator* ArgIndicator::constantIndicator(int const constNumeration) {
    return new ArgIndicator(constNumeration, -1);
}

ArgIndicator* ArgIndicator::variableIndicator(int const functor, int const idx) {
    return new ArgIndicator(functor, idx);
}

bool ArgIndicator::operator==(const ArgIndicator &another) const {
    return functor == another.functor && idx == another.idx;
}

size_t ArgIndicator::hash() const {
    size_t h = functor;
    h = h * 31 + idx;
    return h;
}

ArgIndicator::ArgIndicator(int const _functor, int const _idx) : functor(_functor), idx(_idx) {}

size_t std::hash<ArgIndicator>::operator()(const ArgIndicator& r) const {
    return r.hash();
}

size_t std::hash<ArgIndicator*>::operator()(const ArgIndicator *r) const {
    return r->hash();
}

bool std::equal_to<ArgIndicator*>::operator()(const ArgIndicator *r1, const ArgIndicator *r2) const {
    return (*r1) == (*r2);
}
