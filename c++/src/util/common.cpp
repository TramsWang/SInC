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
Record::Record(int *g, int a): args(g), arity(a) {}

Record::Record(const Record& another) : args(another.args), arity(another.arity) {}

Record::Record(Record&& another): args(another.args), arity(another.arity) {}

void Record::setArgs(int* const newArgs, int const _arity) {
    args = newArgs;
    arity = _arity;
}

int* Record::getArgs() const {
    return args;
}

int Record::getArity() const {
    return arity;
}

Record& Record::operator=(Record&& another) {
    args = another.args;
    arity = another.arity;
    return *this;
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

size_t std::hash<const Record>::operator()(const Record& r) const {
    return r.hash();
}

size_t std::hash<Record*>::operator()(const Record *r) const {
    return r->hash();
}

size_t std::hash<const Record*>::operator()(const Record *r) const {
    return r->hash();
}

bool std::equal_to<Record*>::operator()(const Record *r1, const Record *r2) const {
    return (*r1) == (*r2);
}

bool std::equal_to<const Record*>::operator()(const Record *r1, const Record *r2) const {
    return (*r1) == (*r2);
}

/*
 * Predicate
 */
using sinc::Predicate;
Predicate::Predicate(int const p, int* const g, int const a): predSymbol(p), args(g), arity(a), releaseArg(false) {}

Predicate::Predicate(int const p, int const a): predSymbol(p), args(new int[a]{0}), arity(a), releaseArg(true) {}

int* copyIntArray(int* const arr,int  const length) {
    int* _arr = new int[length];
    for (int i = 0; i < length; i++) {
        _arr[i] = arr[i];
    }
    return _arr;
}

Predicate::Predicate(const Predicate& another): predSymbol(another.predSymbol), arity(another.arity) {
    if (another.releaseArg) {
        args = copyIntArray(another.args, another.arity);
        releaseArg = true;
    } else {
        args = another.args;
        releaseArg = false;
    }
}

Predicate::~Predicate() {
    if (releaseArg) {
        delete[] args;
    }
}

int Predicate::getPredSymbol() const {
    return predSymbol;
}

int* Predicate::getArgs() const {
    return args;
}

int Predicate::getArg(int const idx) const {
    return args[idx];
}

void Predicate::setArg(int const idx, int const arg) {
    args[idx] = arg;
}

int Predicate::getArity() const {
    return arity;
}

void Predicate::maintainArgs() {
    releaseArg = true;
}

Predicate& Predicate::operator=(Predicate&& another) noexcept {
    if (this == &another) {
        return *this;
    }
    if (releaseArg) {
        delete[] args;
    }
    predSymbol = another.predSymbol;
    args = another.args;
    arity = another.arity;
    releaseArg = another.releaseArg;
    another.args = nullptr;
    another.releaseArg = false;
    return *this;
}

bool Predicate::operator==(const Predicate &another) const {
    if (this->predSymbol != another.predSymbol || this->arity != another.arity) {
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

std::string Predicate::toString(const char* const names[]) const {
    std::ostringstream os;
    os << names[predSymbol] << '(';
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

size_t std::hash<const Predicate>::operator()(const Predicate& r) const {
    return r.hash();
}

size_t std::hash<Predicate*>::operator()(const Predicate *r) const {
    return r->hash();
}

size_t std::hash<const Predicate*>::operator()(const Predicate *r) const {
    return r->hash();
}

bool std::equal_to<Predicate*>::operator()(const Predicate *r1, const Predicate *r2) const {
    return (*r1) == (*r2);
}

bool std::equal_to<const Predicate*>::operator()(const Predicate *r1, const Predicate *r2) const {
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
        free((void*)name);  // as `strdup()` uses `malloc()`
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

const char* ParsedArg::getName() const {
    return name;
}

int ParsedArg::getId() const {
    return id;
}

ParsedArg& ParsedArg::operator=(ParsedArg&& another) {
    if (this == &another) {
        return *this;
    }
    if (nullptr != name) {
        free((void*)name);
    }
    name = another.name;
    id = another.id;
    another.name = nullptr;
    return *this;
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

size_t std::hash<const ParsedArg>::operator()(const ParsedArg& r) const {
    return r.hash();
}

size_t std::hash<const ParsedArg*>::operator()(const ParsedArg *r) const {
    return r->hash();
}

bool std::equal_to<const ParsedArg*>::operator()(const ParsedArg *r1, const ParsedArg *r2) const {
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
    if (nullptr != args) {
        for (int i = 0; i < arity; i++) {
            if (nullptr != args[i]) {
                delete args[i];
            }
        }
        delete[] args;
    }
}

const std::string& ParsedPred::getPredSymbol() const {
    return predSymbol;
}

ParsedArg** ParsedPred::getArgs() const {
    return args;
}

int ParsedPred::getArity() const {
    return arity;
}

ParsedArg* ParsedPred::getArg(int const idx) const {
    return args[idx];
}

void ParsedPred::setArg(int const idx, ParsedArg* const arg) {
    args[idx] = arg;
}

ParsedPred& ParsedPred::operator=(ParsedPred&& another) {
    if (this == &another) {
        return *this;
    }
    if (nullptr != args) {
        for (int i = 0; i < arity; i++) {
            if (nullptr != args[i]) {
                delete args[i];
            }
        }
        delete[] args;
    }
    args = another.args;
    predSymbol = another.predSymbol;
    arity = another.arity;
    another.args = nullptr;
    return *this;
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

size_t std::hash<const ParsedPred>::operator()(const ParsedPred& r) const {
    return r.hash();
}

size_t std::hash<const ParsedPred*>::operator()(const ParsedPred *r) const {
    return r->hash();
}

bool std::equal_to<const ParsedPred*>::operator()(const ParsedPred *r1, const ParsedPred *r2) const {
    return (*r1) == (*r2);
}

/**
 * ArgLocation
 */
using sinc::ArgLocation;
ArgLocation::ArgLocation(int const _predIdx, int const _argIdx): predIdx(_predIdx), argIdx(_argIdx) {}

ArgLocation::ArgLocation(const ArgLocation& another) : predIdx(another.predIdx), argIdx(another.argIdx) {}

ArgLocation& ArgLocation::operator=(ArgLocation&& another) {
    predIdx = another.predIdx;
    argIdx = another.argIdx;
    return *this;
}

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

size_t std::hash<const ArgLocation>::operator()(const ArgLocation& r) const {
    return r.hash();
}

size_t std::hash<const ArgLocation*>::operator()(const ArgLocation *r) const {
    return r->hash();
}

bool std::equal_to<const ArgLocation*>::operator()(const ArgLocation *r1, const ArgLocation *r2) const {
    return (*r1) == (*r2);
}

/**
 * ArgIndicator 
 */
using sinc::ArgIndicator;
ArgIndicator* ArgIndicator::constantIndicator(int const constNumeration) {
    return new ArgIndicator(constNumeration, -1);
}

ArgIndicator* ArgIndicator::variableIndicator(int const functor, int const idx) {
    return new ArgIndicator(functor, idx);
}

int ArgIndicator::getFunctor() const {
    return functor;
}

int ArgIndicator::getIdx() const {
    return idx;
}

ArgIndicator& ArgIndicator::operator=(ArgIndicator&& another) {
    functor = another.functor;
    idx = another.idx;
    return *this;
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

size_t std::hash<const ArgIndicator>::operator()(const ArgIndicator& r) const {
    return r.hash();
}

size_t std::hash<const ArgIndicator*>::operator()(const ArgIndicator *r) const {
    return r->hash();
}

bool std::equal_to<const ArgIndicator*>::operator()(const ArgIndicator *r1, const ArgIndicator *r2) const {
    return (*r1) == (*r2);
}
