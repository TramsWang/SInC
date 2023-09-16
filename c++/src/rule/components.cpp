#include "components.h"
#include <cmath>
#include <sstream>

/**
 * EvalMetric
 */
using sinc::EvalMetric;

const std::string EvalMetric::SYMBOL_COMPRESSION_RATIO = std::string(_SYMBOL_COMPRESSION_RATIO);
const std::string EvalMetric::SYMBOL_COMPRESSION_CAPACITY = std::string(_SYMBOL_COMPRESSION_CAPACITY);
const std::string EvalMetric::SYMBOL_INFO_GAIN = std::string(_SYMBOL_INFO_GAIN);
const std::string EvalMetric::DESC_COMPRESSION_RATIO = std::string(_DESC_COMPRESSION_RATIO);
const std::string EvalMetric::DESC_COMPRESSION_CAPACITY = std::string(_DESC_COMPRESSION_CAPACITY);
const std::string EvalMetric::DESC_INFO_GAIN = std::string(_DESC_INFO_GAIN);

const std::string& EvalMetric::getSymbol(Value v) {
    switch (v) {
        case CompressionCapacity:
            return SYMBOL_COMPRESSION_CAPACITY;
        case InfoGain:
            return SYMBOL_INFO_GAIN;
        case CompressionRatio:
        default:
            return SYMBOL_COMPRESSION_RATIO;
    }
}

const std::string& EvalMetric::getDescription(Value v) {
    switch (v) {
        case CompressionCapacity:
            return DESC_COMPRESSION_CAPACITY;
        case InfoGain:
            return DESC_INFO_GAIN;
        case CompressionRatio:
        default:
            return DESC_COMPRESSION_RATIO;
    }
}

EvalMetric::Value EvalMetric::getBySymbol(const std::string& symbol) {
    if (SYMBOL_COMPRESSION_RATIO == symbol) {
        return CompressionRatio;
    } else if (SYMBOL_COMPRESSION_CAPACITY == symbol) {
        return CompressionCapacity;
    } else if (SYMBOL_INFO_GAIN == symbol) {
        return InfoGain;
    } else {
        return CompressionRatio;
    }
}

/**
 * Eval
 */
using sinc::Eval;
Eval::Eval(double const _posEtls, double const _allEtls, int const _ruleLength) :
    posEtls(_posEtls), allEtls(_allEtls), negEtls(_allEtls - _posEtls), ruleLength(_ruleLength)
{
    double tmp_ratio = posEtls / (allEtls + ruleLength);
    compRatio = std::isnan(tmp_ratio) ? 0 : tmp_ratio;
    compCapacity = posEtls - negEtls - ruleLength;
    infoGain = (0 == posEtls || 0 == compRatio) ? (-1.0/0.0) : posEtls * std::log(1+compRatio);
}

double Eval::value(EvalMetric::Value type) const {
    switch (type) {
        case EvalMetric::CompressionCapacity:
            return compCapacity;
        case EvalMetric::CompressionRatio:
            return compRatio;
        case EvalMetric::InfoGain:
            return infoGain;
        default:
            return 0;
    }
}

bool Eval::useful() const {
    return 0 < compCapacity;
}

double Eval::getAllEtls() const {
    return allEtls;
}

double Eval::getPosEtls() const {
    return posEtls;
}

double Eval::getNegEtls() const {
    return negEtls;
}

double Eval::getRuleLength() const {
    return ruleLength;
}

std::string Eval::toString() const {
    std::ostringstream os;
    os << "(+)" << posEtls << "; (-)" << negEtls << "; |" << ruleLength << "|; δ=" << compCapacity << "; τ=" << "; h=" << infoGain;
    return os.str();
}

/**
 * PredicateWithClass
 */
using sinc::PredicateWithClass;

PredicateWithClass::PredicateWithClass(int const _functor, int const _arity) : functor(_functor), arity(_arity),
    classArgs(new MultiSet<ArgIndicator*>*[arity]{0}) {}

PredicateWithClass::~PredicateWithClass() {
    delete[] classArgs;
}

bool PredicateWithClass::operator==(const PredicateWithClass &another) const {
    if (functor != another.functor || arity != another.arity) {
        return false;
    }
    for (int i = 0; i < arity; i++) {
        if (!(*(classArgs[i]) == *(another.classArgs[i]))) {
            return false;
        }
    }
    return true;
}

size_t PredicateWithClass::hash() const {
    size_t h = functor * 31 + arity;
    for (int i = 0; i < arity; i++) {
        h = h * 31 + classArgs[i]->hash();
    }
    return h;
}

size_t std::hash<PredicateWithClass>::operator()(const PredicateWithClass& r) const {
    return r.hash();
}

size_t std::hash<PredicateWithClass*>::operator()(const PredicateWithClass *r) const {
    return r->hash();
}

bool std::equal_to<PredicateWithClass*>::operator()(const PredicateWithClass *r1, const PredicateWithClass *r2) const {
    return (*r1) == (*r2);
}

/**
 * Fingerprint
 */
using sinc::Fingerprint;
using sinc::MultiSet;
Fingerprint::Fingerprint(const std::vector<Predicate>& _rule) : rule(_rule) {
    /* Count the number of LVs */
    /* Assumption: The IDs for the variables start from 0 and are continuous */
    int max_lv_id = -1;
    for (Predicate const& predicate: rule) {
        for (int arg_idx = 0; arg_idx < predicate.arity; arg_idx++) {
            int argument = predicate.args[arg_idx];
            if (ARG_IS_VARIABLE(argument)) {
                max_lv_id = std::max(max_lv_id, ARG_DECODE(argument));
            }
        }
    }
    equivalenceClassType** lv_equiv_classes = new equivalenceClassType*[max_lv_id+1];
    for (int vid = 0; vid <= max_lv_id; vid++) {
        lv_equiv_classes[vid] = new equivalenceClassType();
        equivalenceClassPtrs.push_back(lv_equiv_classes[vid]);
    }

    /* Construct equivalence classes */
    classedStructure.reserve(rule.size());
    for (Predicate const& predicate : rule) {
        PredicateWithClass& pred_with_class = classedStructure.emplace_back(predicate.predSymbol, predicate.arity);
        // PredicateWithClass& pred_with_class = classedStructure.back();
        for (int arg_idx = 0; arg_idx < predicate.arity; arg_idx++) {
            int argument = predicate.args[arg_idx];
            if (ARG_IS_EMPTY(argument)) {
                equivalenceClassType* eqc = new equivalenceClassType();
                equivalenceClassPtrs.push_back(eqc);
                eqc->add(ArgIndicator::variableIndicator(predicate.predSymbol, arg_idx));
                equivalenceClasses.add(eqc);
                pred_with_class.classArgs[arg_idx] = eqc;
            } else if (ARG_IS_VARIABLE(argument)) {
                int var_id = ARG_DECODE(argument);
                equivalenceClassType* eqc = lv_equiv_classes[var_id];
                ArgIndicator* var_indicator = ArgIndicator::variableIndicator(predicate.predSymbol, arg_idx);
                if (1 < eqc->add(var_indicator)) {
                    delete var_indicator;
                }
                pred_with_class.classArgs[arg_idx] = eqc;
            } else {
                int constant = ARG_DECODE(argument);
                equivalenceClassType* eqc = new equivalenceClassType();
                equivalenceClassPtrs.push_back(eqc);
                eqc->add(ArgIndicator::variableIndicator(predicate.predSymbol, arg_idx));
                eqc->add(ArgIndicator::constantIndicator(constant));
                equivalenceClasses.add(eqc);
                pred_with_class.classArgs[arg_idx] = eqc;
            }
        }
    }

    /* Add the equivalent classes for the LVs to the fingerprint */
    for (int vid = 0; vid <= max_lv_id; vid++) {
        equivalenceClasses.add(lv_equiv_classes[vid]);
    }

    /* Calculate hash code */
    hashCode = classedStructure[0].hash() * 31 + equivalenceClasses.hash();

    delete[] lv_equiv_classes;
}

Fingerprint::~Fingerprint() {
    for (equivalenceClassType* const& eqc: equivalenceClassPtrs) {
        releaseEquivalenceClass(eqc);
    }
}

const MultiSet<Fingerprint::equivalenceClassType*>& Fingerprint::getEquivalenceClasses() const {
    return equivalenceClasses;
}

const std::vector<PredicateWithClass>& Fingerprint::getClassedStructure() const {
    return classedStructure;
}

bool Fingerprint::generalizationOf(const Fingerprint& another) const {
    if (rule.size() > another.rule.size()) {
        return false;
    }
    const PredicateWithClass& head = classedStructure[0];
    const PredicateWithClass& another_head = another.classedStructure[0];
    if (!generalizationOf(head, another_head)) {
        return false;
    }
    if (2 == head.arity && (   // TODO: Is there any better way to do argument check in head?
        (head.classArgs[0] == head.classArgs[1] && another_head.classArgs[0] != another_head.classArgs[1]) ||
        (head.classArgs[0] != head.classArgs[1] && another_head.classArgs[0] == another_head.classArgs[1])
    )) {
        return false;
    }
    for (int pred_idx = 1; pred_idx < classedStructure.size(); pred_idx++) {
        const PredicateWithClass& predicate = classedStructure[pred_idx];
        bool found_specialization = false;
        for (int another_pred_idx = 1; another_pred_idx < another.classedStructure.size(); another_pred_idx++) {
            const PredicateWithClass& another_predicate = another.classedStructure[another_pred_idx];
            if (generalizationOf(predicate, another_predicate)) {
                found_specialization = true;
                break;
            }
        }
        if (!found_specialization) {
            return false;
        }
    }
    return true;
}

bool Fingerprint::operator==(const Fingerprint &another) const {
    return classedStructure[0] == another.classedStructure[0] && equivalenceClasses == another.equivalenceClasses;
}

size_t Fingerprint::hash() const {
    return hashCode;
}

void Fingerprint::releaseEquivalenceClass(equivalenceClassType* eqc) {
    for (std::pair<ArgIndicator*, int> const& kv: eqc->getCntMap()) {
        delete kv.first;
    }
    delete eqc;
}

bool Fingerprint::generalizationOf(const PredicateWithClass& predicate, const PredicateWithClass& specializedPredicate) {
    if (predicate.functor != specializedPredicate.functor || predicate.arity != specializedPredicate.arity) {
        return false;
    }
    for (int arg_idx = 0; arg_idx < predicate.arity; arg_idx++) {
        if (!predicate.classArgs[arg_idx]->subsetOf(*(specializedPredicate.classArgs[arg_idx]))) {
            return false;
        }
    }
    return true;
}

