#include "sincWithEstimation.h"
#include <cmath>
#include <sys/resource.h>
#include <algorithm>
#include <limits>

/**
 * VarLink
 */
using sinc::VarLink;


VarLink::VarLink(int const _predIdx, int const _fromVid, int const _fromArgidx, int const _toVid, int const _toArgIdx) :
    predIdx(_predIdx), fromVid(_fromVid), fromArgIdx(_fromArgidx), toVid(_toVid), toArgIdx(_toArgIdx) {}

VarLink::VarLink(const VarLink& another) : predIdx(another.predIdx), fromVid(another.fromVid), fromArgIdx(another.fromArgIdx),
    toVid(another.toVid), toArgIdx(another.toArgIdx) {}

VarLink::VarLink(const VarLink&& another) : predIdx(another.predIdx), fromVid(another.fromVid), fromArgIdx(another.fromArgIdx),
    toVid(another.toVid), toArgIdx(another.toArgIdx) {}


VarLink& VarLink::operator=(const VarLink& another) {
    predIdx = another.predIdx;
    fromVid = another.fromVid;
    fromArgIdx = another.fromArgIdx;
    toVid = another.toVid;
    toArgIdx = another.toArgIdx;
    return *this;
}

bool VarLink::operator==(const VarLink& another) const {
    return fromVid == another.fromVid && toVid == another.toVid;
}

size_t VarLink::hash() const {
    return fromVid * 31 + toVid;
}

size_t std::hash<VarLink>::operator()(const VarLink& r) const {
    return r.hash();
}

size_t std::hash<VarLink*>::operator()(const VarLink *r) const {
    return r->hash();
}

bool std::equal_to<VarLink*>::operator()(const VarLink *r1, const VarLink *r2) const {
    return (*r1) == (*r2);
}

size_t std::hash<const VarLink>::operator()(const VarLink& r) const {
    return r.hash();
}

size_t std::hash<const VarLink*>::operator()(const VarLink *r) const {
    return r->hash();
}

bool std::equal_to<const VarLink*>::operator()(const VarLink *r1, const VarLink *r2) const {
    return (*r1) == (*r2);
}

/**
 * VarPair
 */
using sinc::VarPair;

VarPair::VarPair(int const _vid1, int const _vid2) : vid1(_vid1), vid2(_vid2) {}

VarPair::VarPair(const VarPair& another) : vid1(another.vid1), vid2(another.vid2) {}

VarPair::VarPair(const VarPair&& another) : vid1(another.vid1), vid2(another.vid2) {}

VarPair& VarPair::operator=(const VarPair& another) {
    vid1 = another.vid1;
    vid2 = another.vid2;
    return *this;
}

bool VarPair::operator==(const VarPair& another) const {
    return vid1 == another.vid1 && vid2 == another.vid2;
}

size_t VarPair::hash() const {
    return vid1 * 31 + vid2;
}

size_t std::hash<VarPair>::operator()(const VarPair& r) const {
    return r.hash();
}

size_t std::hash<VarPair*>::operator()(const VarPair *r) const {
    return r->hash();
}

bool std::equal_to<VarPair*>::operator()(const VarPair *r1, const VarPair *r2) const {
    return (*r1) == (*r2);
}

size_t std::hash<const VarPair>::operator()(const VarPair& r) const {
    return r.hash();
}

size_t std::hash<const VarPair*>::operator()(const VarPair *r) const {
    return r->hash();
}

bool std::equal_to<const VarPair*>::operator()(const VarPair *r1, const VarPair *r2) const {
    return (*r1) == (*r2);
}

/**
 * BodyVarLinkManager
 */
using sinc::BodyVarLinkManager;

BodyVarLinkManager::BodyVarLinkManager(std::vector<Predicate>* const _rule, int const currentVars) : rule(_rule) {
    varLabels.reserve(currentVars);
    varLinkGraph.reserve(currentVars);
    for (int i = 0; i < currentVars; i++) {
        varLabels.push_back(i);
        varLinkGraph.emplace_back();
    }
    for (int pred_idx = FIRST_BODY_PRED_IDX; pred_idx < rule->size(); pred_idx++) {
        Predicate const& predicate = (*rule)[pred_idx];
        for (int arg_idx1 = 0; arg_idx1 < predicate.getArity(); arg_idx1++) {
            int argument1 = predicate.getArg(arg_idx1);
            if (ARG_IS_VARIABLE(argument1)) {
                int var_id1 = ARG_DECODE(argument1);
                int var_label1 = varLabels[var_id1];
                for (int arg_idx2 = arg_idx1 + 1; arg_idx2 < predicate.getArity(); arg_idx2++) {
                    int argument2 = predicate.getArg(arg_idx2);
                    if (ARG_IS_VARIABLE(argument2)) {
                        int var_id2 = ARG_DECODE(argument2);
                        int var_label2 = varLabels[var_id2];
                        if (var_label1 != var_label2) {
                            for (int vid = 0; vid < currentVars; vid++) {
                                if (varLabels[vid] == var_label2) {
                                    varLabels[vid] = var_label1;
                                }
                            }
                        }
                    }
                }
                break;
            }
        }
    }

    for (int pred_idx = FIRST_BODY_PRED_IDX; pred_idx < rule->size(); pred_idx++) {
        Predicate const& predicate = (*rule)[pred_idx];
        for (int arg_idx1 = 0; arg_idx1 < predicate.getArity(); arg_idx1++) {
            int argument1 = predicate.getArg(arg_idx1);
            if (ARG_IS_VARIABLE(argument1)) {
                int var_id1 = ARG_DECODE(argument1);
                for (int arg_idx2 = arg_idx1 + 1; arg_idx2 < predicate.getArity(); arg_idx2++) {
                    int argument2 = predicate.getArg(arg_idx2);
                    if (ARG_IS_VARIABLE(argument2)) {
                        int var_id2 = ARG_DECODE(argument2);
                        if (var_id1 != var_id2) {
                            varLinkGraph[var_id1].emplace(pred_idx, var_id1, arg_idx1, var_id2, arg_idx2);
                            varLinkGraph[var_id2].emplace(pred_idx, var_id2, arg_idx2, var_id1, arg_idx1);
                        }
                    }
                }
            }
        }
    }
}

BodyVarLinkManager::BodyVarLinkManager(const BodyVarLinkManager& another, std::vector<Predicate>* const newRule) : 
    rule(newRule), varLabels(another.varLabels), varLinkGraph(another.varLinkGraph) {}

void BodyVarLinkManager::specOprCase1(int const predIdx, int const argIdx, int const varId) {
    if (HEAD_PRED_IDX == predIdx) {
        return;
    }
    int var_label = varLabels[varId];
    Predicate const& predicate = (*rule)[predIdx];
    for (int arg_idx2 = 0; arg_idx2 < predicate.getArity(); arg_idx2++) {
        int argument2 = predicate.getArg(arg_idx2);
        if (ARG_IS_VARIABLE(argument2) && arg_idx2 != argIdx) {
            int var_id2 = ARG_DECODE(argument2);
            int var_label2 = varLabels[var_id2];
            if (var_label != var_label2) {
                for (int vid = 0; vid < varLabels.size(); vid++) {
                    if (varLabels[vid] == var_label2) {
                        varLabels[vid] = var_label;
                    }
                }
            }
            break;
        }
    }

    for (int arg_idx2 = 0; arg_idx2 < predicate.getArity(); arg_idx2++) {
        int argument2 = predicate.getArg(arg_idx2);
        if (ARG_IS_VARIABLE(argument2)) {
            int var_id2 = ARG_DECODE(argument2);
            if (varId != var_id2) {
                varLinkGraph[varId].emplace(predIdx, varId, argIdx, var_id2, arg_idx2);
                varLinkGraph[var_id2].emplace(predIdx, var_id2, arg_idx2, varId, argIdx);
            }
        }
    }
}

void BodyVarLinkManager::specOprCase3(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) {
    if (HEAD_PRED_IDX == predIdx1) {
        if (HEAD_PRED_IDX == predIdx2) {
            varLabels.push_back(varLabels.size());
        } else {
            int var_label2 = -1;
            Predicate const& predicate = (*rule)[predIdx2];
            for (int arg_idx = 0; arg_idx < predicate.getArity(); arg_idx++) {
                int argument = predicate.getArg(arg_idx);
                if (ARG_IS_VARIABLE(argument) && arg_idx != argIdx2) {
                    var_label2 = varLabels[ARG_DECODE(argument)];
                    break;
                }
            }
            varLabels.push_back(var_label2);
        }
    } else if (HEAD_PRED_IDX == predIdx2) {
        int var_label1 = -1;
        Predicate const& predicate = (*rule)[predIdx1];
        for (int arg_idx = 0; arg_idx < predicate.getArity(); arg_idx++) {
            int argument = predicate.getArg(arg_idx);
            if (ARG_IS_VARIABLE(argument) && arg_idx != argIdx1) {
                var_label1 = varLabels[ARG_DECODE(argument)];
                break;
            }
        }
        varLabels.push_back(var_label1);
    } else {
        int var_label1 = -1;
        Predicate const& predicate = (*rule)[predIdx1];
        for (int arg_idx = 0; arg_idx < predicate.getArity(); arg_idx++) {
            int argument = predicate.getArg(arg_idx);
            if (ARG_IS_VARIABLE(argument) && arg_idx != argIdx1) {
                var_label1 = varLabels[ARG_DECODE(argument)];
                break;
            }
        }
        int var_label2 = -1;
        Predicate const& predicate2 = (*rule)[predIdx2];
        for (int arg_idx = 0; arg_idx < predicate2.getArity(); arg_idx++) {
            int argument = predicate2.getArg(arg_idx);
            if (ARG_IS_VARIABLE(argument) && arg_idx != argIdx2) {
                var_label2 = varLabels[ARG_DECODE(argument)];
                break;
            }
        }
        if (var_label1 != var_label2) {
            for (int vid = 0; vid < varLabels.size(); vid++) {
                if (varLabels[vid] == var_label2) {
                    varLabels[vid] = var_label1;
                }
            }
        }
        varLabels.push_back(var_label1);
    }

    int new_vid = varLinkGraph.size();
    varLinkGraph.emplace_back();
    if (HEAD_PRED_IDX != predIdx1) {
        Predicate const& predicate = (*rule)[predIdx1];
        for (int arg_idx = 0; arg_idx < predicate.getArity(); arg_idx++) {
            int argument = predicate.getArg(arg_idx);
            if (ARG_IS_VARIABLE(argument)) {
                int vid = ARG_DECODE(argument);
                if (new_vid != vid) {
                    varLinkGraph[new_vid].emplace(predIdx1, new_vid, argIdx1, vid, arg_idx);
                    varLinkGraph[vid].emplace(predIdx1, vid, arg_idx, new_vid, argIdx1);
                }
            }
        }
    }
    if (HEAD_PRED_IDX != predIdx2 && predIdx1 != predIdx2) {
        Predicate const& predicate = (*rule)[predIdx2];
        for (int arg_idx = 0; arg_idx < predicate.getArity(); arg_idx++) {
            int argument = predicate.getArg(arg_idx);
            if (ARG_IS_VARIABLE(argument)) {
                int vid = ARG_DECODE(argument);
                if (new_vid != vid) {
                    varLinkGraph[new_vid].emplace(predIdx2, new_vid, argIdx2, vid, arg_idx);
                    varLinkGraph[vid].emplace(predIdx2, vid, arg_idx, new_vid, argIdx2);
                }
            }
        }
    }
}

void BodyVarLinkManager::specOprCase4(int const predIdx, int const argIdx) {
    int new_vid = varLinkGraph.size();
    varLinkGraph.emplace_back();
    if (HEAD_PRED_IDX == predIdx) {
        varLabels.push_back(varLabels.size());
    } else {
        int var_label = -1;
        Predicate const& predicate = (*rule)[predIdx];
        for (int arg_idx = 0; arg_idx < predicate.getArity(); arg_idx++) {
            int argument = predicate.getArg(arg_idx);
            if (ARG_IS_VARIABLE(argument) && arg_idx != argIdx) {
                var_label = varLabels[ARG_DECODE(argument)];
                break;
            }
        }
        varLabels.push_back(var_label);

        for (int arg_idx = 0; arg_idx < predicate.getArity(); arg_idx++) {
            int argument = predicate.getArg(arg_idx);
            if (ARG_IS_VARIABLE(argument)) {
                int vid = ARG_DECODE(argument);
                if (new_vid != vid) {
                    varLinkGraph[new_vid].emplace(predIdx, new_vid, argIdx, vid, arg_idx);
                    varLinkGraph[vid].emplace(predIdx, vid, arg_idx, new_vid, argIdx);
                }
            }
        }
    }
}

std::unordered_set<VarPair>* BodyVarLinkManager::assumeSpecOprCase1(int const predIdx, int const argIdx, int const varId) const {
    int var_label = varLabels[varId];
    Predicate const& predicate = (*rule)[predIdx];
    int var_label2 = -1;
    for (int arg_idx2 = 0; arg_idx2 < predicate.getArity(); arg_idx2++) {
        int argument2 = predicate.getArg(arg_idx2);
        if (ARG_IS_VARIABLE(argument2) && arg_idx2 != argIdx) {
            var_label2 = varLabels[ARG_DECODE(argument2)];
            break;
        }
    }
    if (var_label != var_label2) {
        std::unordered_set<VarPair>* new_linked_pairs = new std::unordered_set<VarPair>();
        for (int vid = 0; vid < varLabels.size(); vid++) {
            if (var_label == varLabels[vid]) {
                for (int vid2 = 0; vid2 < varLabels.size(); vid2++) {
                    if (var_label2 == varLabels[vid2]) {
                        new_linked_pairs->emplace(vid, vid2);
                    }
                }
            }
        }
        return new_linked_pairs;
    } else {
        return nullptr;
    }
}

std::unordered_set<VarPair>* BodyVarLinkManager::assumeSpecOprCase3(
    int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2
) const {
    int var_label1 = -1;
    Predicate const& predicate = (*rule)[predIdx1];
    for (int arg_idx = 0; arg_idx < predicate.getArity(); arg_idx++) {
        int argument = predicate.getArg(arg_idx);
        if (ARG_IS_VARIABLE(argument) && arg_idx != argIdx1) {
            var_label1 = varLabels[ARG_DECODE(argument)];
            break;
        }
    }
    int var_label2 = -1;
    Predicate const& predicate2 = (*rule)[predIdx2];
    for (int arg_idx = 0; arg_idx < predicate2.getArity(); arg_idx++) {
        int argument = predicate2.getArg(arg_idx);
        if (ARG_IS_VARIABLE(argument) && arg_idx != argIdx2) {
            var_label2 = varLabels[ARG_DECODE(argument)];
            break;
        }
    }
    if (var_label1 != var_label2) {
        std::unordered_set<VarPair>* new_linked_pairs = new std::unordered_set<VarPair>();
        for (int vid1 = 0; vid1 < varLabels.size(); vid1++) {
            if (var_label1 == varLabels[vid1]) {
                for (int vid2 = 0; vid2 < varLabels.size(); vid2++) {
                    if (var_label2 == varLabels[vid2]) {
                        new_linked_pairs->emplace(vid1, vid2);
                    }
                }
            }
        }
        return new_linked_pairs;
    } else {
        return nullptr;
    }
}

std::vector<VarLink>* BodyVarLinkManager::shortestPath(int fromVid, int toVid) const {
    if (varLabels[fromVid] != varLabels[toVid]) {
        /* The two variables are not linked */
        return nullptr;
    }

    /* The two variables are certainly linked */
    bool visited_vid[varLinkGraph.size()]{};
    visited_vid[fromVid] = true;
    std::vector<VarLink> bfs;
    std::vector<int> predecessor_idx;
    for (VarLink const& link: varLinkGraph[fromVid]) {
        if (!visited_vid[link.toVid]) {
            if (link.toVid == toVid) {
                std::vector<VarLink>* path = new std::vector<VarLink>();
                path->push_back(link);
                return path;
            }
            bfs.push_back(link);
            predecessor_idx.push_back(-1);
            visited_vid[link.toVid] = true;
        }
    }

    /* The two variables are linked by paths no shorter than 2 */
    return shortestPathBfsHandler(visited_vid, bfs, predecessor_idx, toVid);
}

std::vector<VarLink>* BodyVarLinkManager::assumeShortestPathCase1(
    int const predIdx, int const argIdx, int const varId, int const fromVid, int const toVid
) const {
    int start_vid = fromVid;    // start_vid -> varId
    int end_vid = toVid;        // new arg -> end_vid
    bool reversed = false;
    if (varLabels[varId] == varLabels[toVid]) {
        start_vid = toVid;
        end_vid = fromVid;
        reversed = true;
    }
    std::vector<VarLink>* path_start_2_varid;
    if (start_vid == varId) {
        path_start_2_varid = nullptr;
    } else {
        path_start_2_varid = shortestPath(start_vid, varId);
        if (nullptr == path_start_2_varid) {
            return nullptr;
        }
    }

    /* Find the path from new arg to end_vid */
    std::vector<VarLink>* path_new_arg_2_end_vid;
    if (end_vid == varId) {
        path_new_arg_2_end_vid = nullptr;
    } else {
        path_new_arg_2_end_vid = assumeShortestPathHandler(predIdx, argIdx, varId, end_vid);
        if (nullptr == path_new_arg_2_end_vid) {
            if (nullptr != path_start_2_varid) {
                delete path_start_2_varid;
            }
            return nullptr;
        }
    }
    std::vector<VarLink>* path = new std::vector<VarLink>();
    int total_length = (nullptr == path_start_2_varid) ? 0 : path_start_2_varid->size() +
        (nullptr == path_new_arg_2_end_vid) ? 0 : path_new_arg_2_end_vid->size();
    path->reserve(total_length);
    if (reversed) {
        if (nullptr != path_new_arg_2_end_vid) {
            for (int i = 0; i < path_new_arg_2_end_vid->size(); i++) {
                VarLink const& original_link = (*path_new_arg_2_end_vid)[path_new_arg_2_end_vid->size() - 1 - i];
                path->emplace_back(
                    original_link.predIdx, original_link.toVid, original_link.toArgIdx, original_link.fromVid, original_link.fromArgIdx
                );
            }
        }
        if (nullptr != path_start_2_varid) {
            for (int i = 0; i < path_start_2_varid->size(); i++) {
                VarLink const& original_link = (*path_start_2_varid)[path_start_2_varid->size() - 1 - i];
                path->emplace_back(
                    original_link.predIdx, original_link.toVid, original_link.toArgIdx, original_link.fromVid, original_link.fromArgIdx
                );
            }
        }
    } else {
        if (nullptr != path_start_2_varid) {
            path->insert(path->end(), path_start_2_varid->begin(), path_start_2_varid->end());
        }
        if (nullptr != path_new_arg_2_end_vid) {
            path->insert(path->end(), path_new_arg_2_end_vid->begin(), path_new_arg_2_end_vid->end());
        }
    }
    if (nullptr != path_start_2_varid) {
        delete path_start_2_varid;
    }
    if (nullptr != path_new_arg_2_end_vid) {
        delete path_new_arg_2_end_vid;
    }
    return path;
}

std::vector<VarLink>* BodyVarLinkManager::assumeShortestPathCase3(
    int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2, int const fromVid, int const toVid
) const {
    int _from_pred_idx = predIdx1;
    int _from_arg_idx = argIdx1;
    int _to_pred_idx = predIdx2;
    int _to_arg_idx = argIdx2;
    {
        Predicate const& pred = (*rule)[predIdx1];
        for (int arg_idx = 0; arg_idx < pred.getArity(); arg_idx++) {
            int argument = pred.getArg(arg_idx);
            if (ARG_IS_VARIABLE(argument)) {
                if (varLabels[ARG_DECODE(argument)] == varLabels[toVid]) {
                    _from_pred_idx = predIdx2;
                    _from_arg_idx = argIdx2;
                    _to_pred_idx = predIdx1;
                    _to_arg_idx = argIdx1;
                }
                break;
            }
        }
    }

    std::vector<VarLink>* reversed_from_2_from_path = assumeShortestPathHandler(_from_pred_idx, _from_arg_idx, -1, fromVid);
    if (nullptr == reversed_from_2_from_path) {
        return nullptr;
    }
    std::vector<VarLink>* to_2_to_path = assumeShortestPathHandler(_to_pred_idx, _to_arg_idx, -1, toVid);
    if (nullptr == to_2_to_path) {
        if (nullptr != reversed_from_2_from_path) {
            delete reversed_from_2_from_path;
        }
        return nullptr;
    }
    std::vector<VarLink>* path = new std::vector<VarLink>();
    int total_length = reversed_from_2_from_path->size() + to_2_to_path->size();
    path->reserve(total_length);
    for (int i = 0; i < reversed_from_2_from_path->size(); i++) {
        VarLink const& original_link = (*reversed_from_2_from_path)[reversed_from_2_from_path->size() - 1 - i];
        path->emplace_back(
            original_link.predIdx, original_link.toVid, original_link.toArgIdx, original_link.fromVid, original_link.fromArgIdx
        );
    }
    path->insert(path->end(), to_2_to_path->begin(), to_2_to_path->end());
    delete reversed_from_2_from_path;
    delete to_2_to_path;
    return path;
}

std::vector<VarLink>* BodyVarLinkManager::assumeShortestPathCase3(int const predIdx, int const argIdx, int const fromVid) const {
    std::vector<VarLink>* reversed_path = assumeShortestPathHandler(predIdx, argIdx, -1, fromVid);
    if (nullptr == reversed_path) {
        return nullptr;
    }
    std::vector<VarLink>* path = new std::vector<VarLink>();
    path->reserve(reversed_path->size());
    for (int i = 0; i < reversed_path->size(); i++) {
        VarLink const& original_link = (*reversed_path)[reversed_path->size() - 1 - i];
        path->emplace_back(
            original_link.predIdx, original_link.toVid, original_link.toArgIdx, original_link.fromVid, original_link.fromArgIdx
        );
    }
    delete reversed_path;
    return path;
}

std::vector<VarLink>* BodyVarLinkManager::shortestPathBfsHandler(
    bool* const visitedVid, std::vector<VarLink>& bfs, std::vector<int>& predecessorIdx, int const toVid
) const {
    for (int i = 0; i < bfs.size(); i++) {
        for (VarLink const& next_link: varLinkGraph[bfs[i].toVid]) {
            if (!visitedVid[next_link.toVid]) {
                if (next_link.toVid == toVid) {
                    std::vector<VarLink> reversed_path;
                    reversed_path.push_back(next_link);
                    reversed_path.push_back(bfs[i]);
                    for (int edge_idx = predecessorIdx[i]; edge_idx >= 0; edge_idx = predecessorIdx[edge_idx]) {
                        reversed_path.push_back(bfs[edge_idx]);
                    }
                    std::vector<VarLink>* path = new std::vector<VarLink>();
                    path->reserve(reversed_path.size());
                    for (int edge_idx = 0; edge_idx < reversed_path.size(); edge_idx++) {
                        path->push_back(reversed_path[reversed_path.size() - 1 - edge_idx]);
                    }
                    return path;
                }
                bfs.push_back(next_link);
                predecessorIdx.push_back(i);
                visitedVid[next_link.toVid] = true;
            }
        }
    }
    return nullptr;
}

std::vector<VarLink>* BodyVarLinkManager::assumeShortestPathHandler(
    int const predIdx, int const argIdx, int const assumedFromVid, int const toVid
) const {
    bool visited_vid[varLinkGraph.size()]{};
    if (assumedFromVid >= 0 && assumedFromVid < varLinkGraph.size()) {
        visited_vid[assumedFromVid] = true;
    }
    std::vector<VarLink> bfs;
    std::vector<int> predecessor_idx;
    Predicate const& pred = (*rule)[predIdx];
    for (int arg_idx = 0; arg_idx < pred.getArity(); arg_idx++) {
        int argument = pred.getArg(arg_idx);
        if (ARG_IS_VARIABLE(argument)) {
            int vid = ARG_DECODE(argument);
            if (!visited_vid[vid]) {
                if (vid == toVid) {
                    std::vector<VarLink>* path = new std::vector<VarLink>();
                    path->emplace_back(predIdx, assumedFromVid, argIdx, vid, arg_idx);
                    return path;
                }
                bfs.emplace_back(predIdx, assumedFromVid, argIdx, vid, arg_idx);
                predecessor_idx.push_back(-1);
                visited_vid[vid] = true;
            }
        }
    }
    return shortestPathBfsHandler(visited_vid, bfs, predecessor_idx, toVid);
}

/**
 * SpecOprWithScore
 */
using sinc::SpecOprWithScore;
SpecOprWithScore::SpecOprWithScore(SpecOpr const* _opr, Eval const& _eval) : opr(_opr), estEval(_eval) {}

SpecOprWithScore::SpecOprWithScore(SpecOprWithScore&& another) : opr(another.opr), estEval(another.estEval) {
    another.opr = nullptr;
}

SpecOprWithScore& SpecOprWithScore::operator=(SpecOprWithScore&& another) {
    if (nullptr != opr) {
        delete opr;
    }
    opr = another.opr;
    another.opr = nullptr;
    return *this;
}

SpecOprWithScore::~SpecOprWithScore() {
    if (nullptr != opr) {
        delete opr;
    }
}

/**
 * EstSincPerfMonitor
 */
using sinc::EstSincPerfMonitor;

void EstSincPerfMonitor::show(std::ostream& os) {
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
    printf(os, "%10s %10s %10s %10s %10s\n", "#CB", "CB", "CB(%)", "Est.Idx", "Est.Idx(%)");
    rusage usage;
    if (0 != getrusage(RUSAGE_SELF, &usage)) {
        std::cerr << "Failed to get `rusage`" << std::endl;
        usage.ru_maxrss = 1024 * 1024 * 1024;   // set to 1T
    }
    printf(
        os, "%10d %10s %10.2f %10s %10.2f\n\n",
        CompliedBlock::totalNumCbs(), formatMemorySize(cbMemCost).c_str(), ((double) cbMemCost) / usage.ru_maxrss * 100.0,
        formatMemorySize(maxEstIdxCost).c_str(), ((double) maxEstIdxCost) / usage.ru_maxrss * 100.0
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
        CompliedBlock::totalNumCbs(),
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
    size_t total_idx = CompliedBlock::getNumCreationIndices() + CompliedBlock::getNumGetSliceIndices() + CompliedBlock::getNumSplitSlicesIndices() + CompliedBlock::getNumMatchSlices1Indices() + CompliedBlock::getNumMatchSlices2Indices();
    printf(
        os, "  %10d %10d %10d %10d %10d %10d\n\n",
        CompliedBlock::getNumCreationIndices(),
        CompliedBlock::getNumGetSliceIndices(),
        CompliedBlock::getNumSplitSlicesIndices(),
        CompliedBlock::getNumMatchSlices1Indices(),
        CompliedBlock::getNumMatchSlices2Indices(),
        total_idx
    );

    os << "--- Cache Statistics ---\n";
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
 * EstRule
 */
using sinc::EstRule;

EstRule::EstRule(
    int const headPredSymbol, int const arity, fingerprintCacheType& fingerprintCache, tabuMapType& category2TabuSetMap, SimpleKb& _kb
) : Rule(headPredSymbol, arity, fingerprintCache, category2TabuSetMap), kb(_kb), bodyVarLinkManager(&structure, 0)
{
    /* Initialize the E+-cache & T-cache */
    SimpleRelation* head_relation = kb.getRelation(headPredSymbol);
    SplitRecords* split_records = head_relation->splitByEntailment();
    std::vector<int*> const& non_entailed_record_vector = *(split_records->nonEntailedRecords);
    std::vector<int*> const& entailed_record_vector = *(split_records->entailedRecords);
    if (0 == entailed_record_vector.size()) {
        posCache = new CacheFragment(head_relation, headPredSymbol);
        entCache = new CacheFragment(headPredSymbol, arity);
    } else if (0 == non_entailed_record_vector.size()) {
        /* No record to entail, E+-cache and T-cache are both empty */
        posCache = new CacheFragment(headPredSymbol, arity);
        entCache = new CacheFragment(headPredSymbol, arity);
    } else {
        int** non_entailed_records = toArray(non_entailed_record_vector);
        int** entailed_records = toArray(entailed_record_vector);
        IntTable* non_entailed_record_table = new IntTable(non_entailed_records, non_entailed_record_vector.size(), arity);
        IntTable* entailed_record_table = new IntTable(entailed_records, entailed_record_vector.size(), arity);
        posCache = new CacheFragment(
            CompliedBlock::create(non_entailed_records, non_entailed_record_vector.size(), arity, non_entailed_record_table, true, true), 
            headPredSymbol
        );
        entCache = new CacheFragment(
            CompliedBlock::create(entailed_records, entailed_record_vector.size(), arity, entailed_record_table, true, true),
            headPredSymbol
        );
    }
    maintainPosCache = true;
    maintainEntCache = true;

    /* Initialize the E-cache */
    allCache = new std::vector<CacheFragment*>();
    predIdx2AllCacheTableInfo.emplace_back(-1, -1);    // head is not mapped to any fragment
    maintainAllCache = true;

    /* Initial evaluation */
    int pos_ent = non_entailed_record_vector.size();
    int already_ent = entailed_record_vector.size();
    double all_ent = pow(kb.totalConstants(), arity);
    eval = Eval(pos_ent, all_ent - already_ent, length);
    delete split_records;
}

EstRule::EstRule(const EstRule& another) : Rule(another), kb(another.kb), posCache(another.posCache), maintainPosCache(false),
    entCache(another.entCache), maintainEntCache(false), allCache(another.allCache), maintainAllCache(false),
    predIdx2AllCacheTableInfo(another.predIdx2AllCacheTableInfo), bodyVarLinkManager(another.bodyVarLinkManager, &structure) {}

EstRule::~EstRule() {
    if (maintainPosCache) {
        delete posCache;
    }
    if (maintainEntCache) {
        delete entCache;
    }
    if (maintainAllCache) {
        for (CacheFragment* const& fragment: *allCache) {
            delete fragment;
        }
        delete allCache;
    }
}

EstRule* EstRule::clone() const {
    uint64_t time_start = currentTimeInNano();
    EstRule* rule = new EstRule(*this);
    rule->copyTime = currentTimeInNano() - time_start;
    return rule;
}

std::vector<SpecOprWithScore*>* EstRule::estimateSpecializations() {
    /* Gather values in columns */
    std::vector<MultiSet<int>*> column_values_in_pos_cache;
    std::vector<MultiSet<int>*> column_values_in_ent_cache;
    std::vector<MultiSet<int>*> column_values_in_all_cache;
    column_values_in_pos_cache.reserve(structure.size());
    column_values_in_ent_cache.reserve(structure.size());
    column_values_in_all_cache.reserve(structure.size());
    std::unordered_set<int*> entailed_records;
    bool vars_in_head[usedLimitedVars()]{};
    for (int pred_idx = HEAD_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
        Predicate const& predicate = structure[pred_idx];
        MultiSet<int>* arg_sets_pos = new MultiSet<int>[predicate.getArity()];
        MultiSet<int>* arg_sets_ent = new MultiSet<int>[predicate.getArity()];
        MultiSet<int>* arg_sets_all = new MultiSet<int>[predicate.getArity()];
        column_values_in_pos_cache.push_back(arg_sets_pos);
        column_values_in_ent_cache.push_back(arg_sets_ent);
        column_values_in_all_cache.push_back(arg_sets_all);
        if (HEAD_PRED_IDX == pred_idx) {
            for (int arg_idx = 0; arg_idx < predicate.getArity(); arg_idx++) {
                int arg = predicate.getArg(arg_idx);
                if (ARG_IS_VARIABLE(arg)) {
                    vars_in_head[ARG_DECODE(arg)] = true;
                }
            }
        }
    }
    for (CacheFragment::entryType* const& cache_entry: posCache->getEntries()) {
        for (int pred_idx = HEAD_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
            CompliedBlock const* cb = (*cache_entry)[pred_idx];
            MultiSet<int>* arg_sets = column_values_in_pos_cache[pred_idx];
            for (int arg_idx = 0; arg_idx < cb->getTotalCols(); arg_idx++) {
                MultiSet<int>& arg_set = arg_sets[arg_idx];
                for (int row_idx = 0; row_idx < cb->getTotalRows(); row_idx++) {
                    int* record = cb->getComplianceSet()[row_idx];
                    arg_set.add(record[arg_idx]);
                }
            }
        }
    }
    for (CacheFragment::entryType* cache_entry: entCache->getEntries()) {
        for (int pred_idx = HEAD_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
            CompliedBlock const* cb = (*cache_entry)[pred_idx];
            MultiSet<int>* arg_sets = column_values_in_ent_cache[pred_idx];
            for (int arg_idx = 0; arg_idx < cb->getTotalCols(); arg_idx++) {
                MultiSet<int>& arg_set = arg_sets[arg_idx];
                for (int row_idx = 0; row_idx < cb->getTotalRows(); row_idx++) {
                    int* record = cb->getComplianceSet()[row_idx];
                    arg_set.add(record[arg_idx]);
                }
            }
        }
        CompliedBlock const* head_cb = (*cache_entry)[HEAD_PRED_IDX];
        for (int row_idx = 0; row_idx < head_cb->getTotalRows(); row_idx++) {
            entailed_records.insert(head_cb->getComplianceSet()[row_idx]);
        }
    }
    for (int pred_idx = FIRST_BODY_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
        TabInfo const& tab_info = predIdx2AllCacheTableInfo[pred_idx];
        MultiSet<int>* arg_sets = column_values_in_all_cache[pred_idx];
        for (CacheFragment::entryType* cache_entry: (*allCache)[tab_info.fragmentIdx]->getEntries()) {
            CompliedBlock const* cb = (*cache_entry)[tab_info.tabIdx];
            for (int arg_idx = 0; arg_idx < cb->getTotalCols(); arg_idx++) {
                MultiSet<int>& arg_set = arg_sets[arg_idx];
                for (int row_idx = 0; row_idx < cb->getTotalRows(); row_idx++) {
                    int* record = cb->getComplianceSet()[row_idx];
                    arg_set.add(record[arg_idx]);
                }
            }
        }
    }
    double entailed_record_cnt = entailed_records.size();

    /* Find all empty arguments */
    std::vector<ArgLocation> empty_args;
    for (int pred_idx = HEAD_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
        Predicate const& predicate = structure[pred_idx];
        for (int arg_idx = 0; arg_idx < predicate.getArity(); arg_idx++) {
            if (ARG_IS_EMPTY(predicate.getArg(arg_idx))) {
                empty_args.emplace_back(pred_idx, arg_idx);
            }
        }
    }

    /* Estimate case 1 & 2 */
    std::vector<SpecOprWithScore*>* results = new std::vector<SpecOprWithScore*>();
    for (int var_id = 0; var_id < usedLimitedVars(); var_id++) {
        std::vector<ArgLocation> const& var_arg_locs = *(limitedVarArgs[var_id]);
        int const var_arg = ARG_VARIABLE(var_id);

        /* Case 1 */
        for (ArgLocation const& vacant: empty_args) {
            int another_arg_idx = -1;   // another arg in the same predicate with 'vacant' that shares the same var
            {
                Predicate const& vacant_pred = structure[vacant.predIdx];
                for (int arg_idx = 0; arg_idx < vacant_pred.getArity(); arg_idx++) {
                    if (var_arg == vacant_pred.getArg(arg_idx)) {
                        another_arg_idx = arg_idx;
                        break;
                    }
                }
            }

            double est_pos_ent, est_already_ent, est_all_ent;
            if (-1 != another_arg_idx) {
                /* There is another occurrence of var in the same predicate */
                // Todo: Random sampling may be applied here
                int remaining_rows = 0;
                for (CacheFragment::entryType* cache_entry: posCache->getEntries()) {
                    CompliedBlock const* cb = (*cache_entry)[vacant.predIdx];
                    for (int row_idx = 0; row_idx < cb->getTotalRows(); row_idx++) {
                        int* record = cb->getComplianceSet()[row_idx];
                        remaining_rows += (record[another_arg_idx] == record[vacant.argIdx]) ? 1 : 0;
                    }
                }
                est_pos_ent = eval.getPosEtls() / column_values_in_pos_cache[vacant.predIdx][vacant.argIdx].getSize() * remaining_rows;

                // Todo: Random sampling may be applied here
                if (0 == entailed_record_cnt) {
                    est_already_ent = 0;
                } else {
                    remaining_rows = 0;
                    for (CacheFragment::entryType* cache_entry : entCache->getEntries()) {
                        CompliedBlock const* cb = (*cache_entry)[vacant.predIdx];
                        for (int row_idx = 0; row_idx < cb->getTotalRows(); row_idx++) {
                            int* record = cb->getComplianceSet()[row_idx];
                            remaining_rows += (record[another_arg_idx] == record[vacant.argIdx]) ? 1 : 0;
                        }
                    }
                    est_already_ent = entailed_record_cnt / column_values_in_ent_cache[vacant.predIdx][vacant.argIdx].getSize() * remaining_rows;
                }

                if (HEAD_PRED_IDX == vacant.predIdx) {
                    est_all_ent = eval.getAllEtls() / kb.totalConstants();
                } else {
                    // Todo: Random sampling may be applied here
                    remaining_rows = 0;
                    TabInfo const& tab_info = predIdx2AllCacheTableInfo[vacant.predIdx];
                    for (CacheFragment::entryType* cache_entry: (*allCache)[tab_info.fragmentIdx]->getEntries()) {
                        CompliedBlock const* cb = (*cache_entry)[tab_info.tabIdx];
                        for (int row_idx = 0; row_idx < cb->getTotalRows(); row_idx++) {
                            int* record = cb->getComplianceSet()[row_idx];
                            remaining_rows += (record[another_arg_idx] == record[vacant.argIdx]) ? 1 : 0;
                        }
                    }
                    est_all_ent = eval.getAllEtls() / column_values_in_all_cache[vacant.predIdx][vacant.argIdx].getSize() * remaining_rows;
                }
            } else {
                /* Other occurrences of the var are in different predicates */
                double est_pos_ratios[var_arg_locs.size() + 1];
                {
                    MultiSet<int> const& vacant_column_values_pos = column_values_in_pos_cache[vacant.predIdx][vacant.argIdx];
                    for (int i = 0; i < var_arg_locs.size(); i++) {
                        ArgLocation const& var_arg_loc = var_arg_locs[i];
                        MultiSet<int> const& var_column_values_pos = column_values_in_pos_cache[var_arg_loc.predIdx][var_arg_loc.argIdx];
                        est_pos_ratios[i] = itemsCountOfKeys(var_column_values_pos, vacant_column_values_pos) / var_column_values_pos.getSize();
                    }
                    ArgLocation const& _arg_loc_of_var = var_arg_locs[0];
                    est_pos_ratios[var_arg_locs.size()] = itemsCountOfKeys(
                        vacant_column_values_pos, column_values_in_pos_cache[_arg_loc_of_var.predIdx][_arg_loc_of_var.argIdx]
                    ) / vacant_column_values_pos.getSize();
                }
                est_pos_ent = estimateRatiosInPosCache(est_pos_ratios, var_arg_locs.size() + 1) * eval.getPosEtls();

                if (0 == entailed_record_cnt) {
                    est_already_ent = 0;
                } else {
                    double est_ent_ratios[var_arg_locs.size() + 1];
                    MultiSet<int> const& vacant_column_values_ent = column_values_in_ent_cache[vacant.predIdx][vacant.argIdx];
                    for (int i = 0; i < var_arg_locs.size(); i++) {
                        ArgLocation const& var_arg_loc = var_arg_locs[i];
                        MultiSet<int> const& var_column_values_ent = column_values_in_ent_cache[var_arg_loc.predIdx][var_arg_loc.argIdx];
                        est_ent_ratios[i] = itemsCountOfKeys(var_column_values_ent, vacant_column_values_ent) / var_column_values_ent.getSize();
                    }
                    ArgLocation const& _arg_loc_of_var = var_arg_locs[0];
                    est_ent_ratios[var_arg_locs.size()] = itemsCountOfKeys(
                        vacant_column_values_ent, column_values_in_ent_cache[_arg_loc_of_var.predIdx][_arg_loc_of_var.argIdx]
                    ) / vacant_column_values_ent.getSize();
                    est_already_ent = estimateRatiosInPosCache(est_ent_ratios, var_arg_locs.size() + 1) * entailed_record_cnt;
                }

                if (HEAD_PRED_IDX == vacant.predIdx) {
                    /* Find a linked var with this one both in head and body (with the shortest path) */
                    std::vector<VarLink>* var_link_path = nullptr;
                    for (int head_vid = 0; head_vid < usedLimitedVars(); head_vid++) {
                        if (vars_in_head[head_vid]) {
                            std::vector<VarLink>* _var_link_path = bodyVarLinkManager.shortestPath(head_vid, var_id);
                            if (nullptr != _var_link_path) {
                                if (nullptr == var_link_path) {
                                    var_link_path = _var_link_path;
                                } else if (_var_link_path->size() < var_link_path->size()) {
                                    delete var_link_path;
                                    var_link_path = _var_link_path;
                                }
                            }
                        }
                    }
                    /* Theoretically, var_link_path != nullptr */
                    /* The new var is body-linked with some vars in the head, estimate according to the shortest path */
                    est_all_ent = eval.getAllEtls() / kb.totalConstants() * estimateLinkVarRatio(*var_link_path, column_values_in_all_cache);
                    delete var_link_path;
                } else {
                    std::unordered_set<VarPair>* new_linked_var_pairs = bodyVarLinkManager.assumeSpecOprCase1(vacant.predIdx, vacant.argIdx, var_id);
                    est_all_ent = eval.getAllEtls();
                    if (nullptr != new_linked_var_pairs) {
                        for (VarPair const& new_linked_var_pair: *new_linked_var_pairs) {
                            if (vars_in_head[new_linked_var_pair.vid1] && vars_in_head[new_linked_var_pair.vid2]) {
                                for (ArgLocation const& arg_loc_of_v2: *(limitedVarArgs[new_linked_var_pair.vid2])) {
                                    if (HEAD_PRED_IDX != arg_loc_of_v2.predIdx) {
                                        /* Find an occurrence of vid2 in the body to get the number of values assigned to that var */
                                        std::vector<VarLink>* var_link_path = bodyVarLinkManager.assumeShortestPathCase1(
                                            vacant.predIdx, vacant.argIdx, var_id, new_linked_var_pair.vid1, new_linked_var_pair.vid2
                                        );
                                        est_all_ent = est_all_ent / column_values_in_all_cache[arg_loc_of_v2.predIdx][arg_loc_of_v2.argIdx].differentValues()
                                            * estimateLinkVarRatio(*var_link_path, column_values_in_all_cache);
                                        delete var_link_path;
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                        delete new_linked_var_pairs;
                    }
                    /* Find other occurrences of this var in the body */
                    std::vector<ArgLocation> var_arg_locs_in_body;
                    for (ArgLocation arg_loc: var_arg_locs) {
                        if (HEAD_PRED_IDX != arg_loc.predIdx) {
                            var_arg_locs_in_body.push_back(arg_loc);
                        }
                    }
                    MultiSet<int> const& vacant_column_values = column_values_in_all_cache[vacant.predIdx][vacant.argIdx];
                    if (!var_arg_locs_in_body.empty()) {
                        double est_all_ratios[var_arg_locs_in_body.size() + 1];
                        for (int i = 0; i < var_arg_locs_in_body.size(); i++) {
                            ArgLocation const& arg_loc_of_var = var_arg_locs_in_body[i];
                            MultiSet<int> const& var_column_values = column_values_in_all_cache[arg_loc_of_var.predIdx][arg_loc_of_var.argIdx];
                            est_all_ratios[i] = itemsCountOfKeys(var_column_values, vacant_column_values) / var_column_values.getSize();
                        }
                        ArgLocation const& _arg_loc_of_var = var_arg_locs_in_body[0];
                        est_all_ratios[var_arg_locs_in_body.size()] = itemsCountOfKeys(
                            vacant_column_values, column_values_in_all_cache[_arg_loc_of_var.predIdx][_arg_loc_of_var.argIdx]
                        ) / vacant_column_values.getSize();
                        est_all_ent *= estimateRatiosInAllCache(est_all_ratios, var_arg_locs_in_body.size() + 1);
                    }   // Else: the newly added LV must be included in some newly linked pairs and handled by the previous loop
                }
            }
            results->push_back(new SpecOprWithScore(
                new SpecOprCase1(vacant.predIdx, vacant.argIdx, var_id),
                Eval(est_pos_ent, est_all_ent - est_already_ent, length + 1)
            ));
        }

        /* Case 2 */
        std::vector<ArgLocation> var_arg_locs_in_body;
        for (ArgLocation const& arg_loc: var_arg_locs) {
            if (HEAD_PRED_IDX != arg_loc.predIdx) {
                var_arg_locs_in_body.push_back(arg_loc);
            }
        }
        for (SimpleRelation* const& relation: *(kb.getRelations())) {
            for (int arg_idx = 0; arg_idx < relation->getTotalCols(); arg_idx++) {
                double est_pos_ratios[var_arg_locs.size()];
                int* const relation_column_values = relation->valuesInColumn(arg_idx);
                int const num_relation_column_values = relation->numValuesInColumn(arg_idx);
                for (int i = 0; i < var_arg_locs.size(); i++) {
                    ArgLocation const& arg_loc_of_var = var_arg_locs[i];
                    MultiSet<int> const& var_column_values_pos = column_values_in_pos_cache[arg_loc_of_var.predIdx][arg_loc_of_var.argIdx];
                    est_pos_ratios[i] = ((double) var_column_values_pos.itemCount(relation_column_values, num_relation_column_values)) / var_column_values_pos.getSize();
                }
                double est_pos_ent = estimateRatiosInPosCache(est_pos_ratios,var_arg_locs.size()) * eval.getPosEtls();

                double est_already_ent;
                if (0 == entailed_record_cnt) {
                    est_already_ent = 0;
                } else {
                    double est_ent_ratios[var_arg_locs.size()];
                    for (int i = 0; i < var_arg_locs.size(); i++) {
                        ArgLocation const arg_loc_of_var = var_arg_locs[i];
                        MultiSet<int> const& var_column_values_ent = column_values_in_ent_cache[arg_loc_of_var.predIdx][arg_loc_of_var.argIdx];
                        est_ent_ratios[i] = ((double) var_column_values_ent.itemCount(relation_column_values, num_relation_column_values)) / var_column_values_ent.getSize();
                    }
                    est_already_ent = estimateRatiosInPosCache(est_ent_ratios, var_arg_locs.size()) * entailed_record_cnt;
                }

                double est_all_ent;
                if (var_arg_locs_in_body.empty()) {
                    /* All occurrences of the var are in the head */
                    est_all_ent = eval.getAllEtls() / kb.totalConstants() * num_relation_column_values;
                } else {
                    double est_all_ratios[var_arg_locs_in_body.size()];
                    for (int i = 0; i < var_arg_locs_in_body.size(); i++) {
                        ArgLocation const& arg_loc_of_var = var_arg_locs_in_body[i];
                        MultiSet<int> const& var_column_values = column_values_in_all_cache[arg_loc_of_var.predIdx][arg_loc_of_var.argIdx];
                        est_all_ratios[i] = ((double) var_column_values.itemCount(relation_column_values, num_relation_column_values)) / var_column_values.getSize();
                    }
                    est_all_ent = estimateRatiosInAllCache(est_all_ratios, var_arg_locs_in_body.size()) * eval.getAllEtls();
                }
                results->push_back(new SpecOprWithScore(
                        new SpecOprCase2(relation->id, relation->getTotalCols(), arg_idx, var_id),
                        Eval(est_pos_ent, est_all_ent - est_already_ent, length + 1)
                ));
            }
        }
    }

    /* Estimate case 3 & 4 & 5 */
    for (int i = 0; i < empty_args.size(); i++) {
        /* Find the first empty argument */
        ArgLocation const& empty_arg_loc_1 = empty_args[i];
        MultiSet<int> const& empty_arg1_pos_column_values = column_values_in_pos_cache[empty_arg_loc_1.predIdx][empty_arg_loc_1.argIdx];
        MultiSet<int> const& empty_arg1_ent_column_values = column_values_in_ent_cache[empty_arg_loc_1.predIdx][empty_arg_loc_1.argIdx];
        MultiSet<int> const& empty_arg1_all_column_values = column_values_in_all_cache[empty_arg_loc_1.predIdx][empty_arg_loc_1.argIdx];

        /* Case 3 */
        for (int j = i + 1; j < empty_args.size(); j++) {
            /* Find another empty argument */
            ArgLocation const& empty_arg_loc_2 = empty_args[j];
            MultiSet<int> const& empty_arg2_pos_column_values = column_values_in_pos_cache[empty_arg_loc_2.predIdx][empty_arg_loc_2.argIdx];
            MultiSet<int> const& empty_arg2_ent_column_values = column_values_in_ent_cache[empty_arg_loc_2.predIdx][empty_arg_loc_2.argIdx];
            MultiSet<int> const& empty_arg2_all_column_values = column_values_in_all_cache[empty_arg_loc_2.predIdx][empty_arg_loc_2.argIdx];

            double est_pos_ent, est_all_ent, est_already_ent;
            if (empty_arg_loc_1.predIdx == empty_arg_loc_2.predIdx) {
                /* The new args are in the same predicate */
                // Todo: Random sampling may be applied here
                int remaining_rows = 0;
                for (CacheFragment::entryType* cache_entry: posCache->getEntries()) {
                    CompliedBlock const* cb = (*cache_entry)[empty_arg_loc_1.predIdx];
                    for (int row_idx = 0; row_idx < cb->getTotalRows(); row_idx++) {
                        int* record = cb->getComplianceSet()[row_idx];
                        remaining_rows += (record[empty_arg_loc_1.argIdx] == record[empty_arg_loc_2.argIdx]) ? 1 : 0;
                    }
                }
                est_pos_ent = eval.getPosEtls() / empty_arg1_pos_column_values.getSize() * remaining_rows;

                if (0 == entailed_record_cnt) {
                    est_already_ent = 0;
                } else {
                    // Todo: Random sampling may be applied here
                    remaining_rows = 0;
                    for (CacheFragment::entryType* cache_entry: entCache->getEntries()) {
                        CompliedBlock const* cb = (*cache_entry)[empty_arg_loc_1.predIdx];
                        for (int row_idx = 0; row_idx < cb->getTotalRows(); row_idx++) {
                            int* record = cb->getComplianceSet()[row_idx];
                            remaining_rows += (record[empty_arg_loc_1.argIdx] == record[empty_arg_loc_2.argIdx]) ? 1 : 0;
                        }
                    }
                    est_already_ent = entailed_record_cnt / empty_arg1_ent_column_values.getSize() * remaining_rows;
                }

                if (HEAD_PRED_IDX == empty_arg_loc_1.predIdx) {
                    est_all_ent = eval.getAllEtls() / kb.totalConstants();
                } else {
                    // Todo: Random sampling may be applied here
                    remaining_rows = 0;
                    TabInfo const& tab_info = predIdx2AllCacheTableInfo[empty_arg_loc_1.predIdx];
                    for (CacheFragment::entryType* cache_entry: (*allCache)[tab_info.fragmentIdx]->getEntries()) {
                        CompliedBlock const* cb = (*cache_entry)[tab_info.tabIdx];
                        for (int row_idx = 0; row_idx < cb->getTotalRows(); row_idx++) {
                            int* record = cb->getComplianceSet()[row_idx];
                            remaining_rows += (record[empty_arg_loc_1.argIdx] == record[empty_arg_loc_2.argIdx]) ? 1 : 0;
                        }
                    }
                    est_all_ent = eval.getAllEtls() / empty_arg1_all_column_values.getSize() * remaining_rows;
                }
            } else {
                /* The new args are in different predicates */
                double est_pos_ratio1 = itemsCountOfKeys(empty_arg1_pos_column_values, empty_arg2_pos_column_values) / empty_arg1_pos_column_values.getSize();
                double est_pos_ratio2 = itemsCountOfKeys(empty_arg2_pos_column_values, empty_arg1_pos_column_values) / empty_arg2_pos_column_values.getSize();
                double _pos_ratios[2] {est_pos_ratio1, est_pos_ratio2};
                est_pos_ent = estimateRatiosInPosCache(_pos_ratios, 2) * eval.getPosEtls();

                if (0 == entailed_record_cnt) {
                    est_already_ent = 0;
                } else {
                    double est_ent_ratio1 = itemsCountOfKeys(empty_arg1_ent_column_values, empty_arg2_ent_column_values) / empty_arg1_ent_column_values.getSize();
                    double est_ent_ratio2 = itemsCountOfKeys(empty_arg2_ent_column_values, empty_arg1_ent_column_values) / empty_arg2_ent_column_values.getSize();
                    double _ent_ratios[2] {est_ent_ratio1, est_ent_ratio2};
                    est_already_ent = estimateRatiosInPosCache(_ent_ratios, 2) * entailed_record_cnt;
                }

                if (HEAD_PRED_IDX == empty_arg_loc_1.predIdx) {
                    /* HEAD_PRED_IDX != empty_arg_loc_2.predIdx */
                    est_all_ent = eval.getAllEtls() / kb.totalConstants();
                    for (int head_vid = 0; head_vid < usedLimitedVars(); head_vid++) {
                        if (vars_in_head[head_vid]) {
                            std::vector<VarLink>* var_link_path = bodyVarLinkManager.assumeShortestPathCase3(empty_arg_loc_2.predIdx, empty_arg_loc_2.argIdx, head_vid);
                            if (nullptr != var_link_path) {
                                /* Estimate for linked vars */
                                est_all_ent *= estimateLinkVarRatio(*var_link_path, column_values_in_all_cache);
                                delete var_link_path;
                                break;
                            }
                        }
                    }
                    /* Theoretically, 'est_all_ent' will always be estimated in the above loop */
                } else  {
                    /* Here, both the arguments are in the body and in different predicates */
                    std::unordered_set<VarPair>* new_linked_var_pairs = bodyVarLinkManager.assumeSpecOprCase3(
                            empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx, empty_arg_loc_2.predIdx, empty_arg_loc_2.argIdx
                    );
                    est_all_ent = eval.getAllEtls();
                    if (nullptr != new_linked_var_pairs) {
                        for (VarPair const& new_linked_var_pair: *new_linked_var_pairs) {
                            if (vars_in_head[new_linked_var_pair.vid1] && vars_in_head[new_linked_var_pair.vid2]) {
                                for (ArgLocation const& arg_loc_of_v2_in_body: *(limitedVarArgs[new_linked_var_pair.vid2])) {
                                    if (HEAD_PRED_IDX != arg_loc_of_v2_in_body.predIdx) {
                                        std::vector<VarLink>* var_link_path = bodyVarLinkManager.assumeShortestPathCase3(
                                                empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx, empty_arg_loc_2.predIdx, empty_arg_loc_2.argIdx,
                                                new_linked_var_pair.vid1, new_linked_var_pair.vid2
                                        );
                                        est_all_ent /= column_values_in_all_cache[arg_loc_of_v2_in_body.predIdx][arg_loc_of_v2_in_body.argIdx].differentValues();
                                        if (nullptr != var_link_path) {
                                            est_all_ent *= estimateLinkVarRatio(*var_link_path, column_values_in_all_cache);
                                            delete var_link_path;
                                        }
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                        delete new_linked_var_pairs;
                    }
                    double est_all_ratio1 = itemsCountOfKeys(empty_arg1_all_column_values, empty_arg2_all_column_values) / empty_arg1_all_column_values.getSize();
                    double est_all_ratio2 = itemsCountOfKeys(empty_arg2_all_column_values, empty_arg1_all_column_values) / empty_arg2_all_column_values.getSize();
                    double _all_ratios[2] {est_all_ratio1, est_all_ratio2};
                    est_all_ent *= estimateRatiosInAllCache(_all_ratios, 2);
                }
            }
            results->push_back(new SpecOprWithScore(
                new SpecOprCase3(empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx, empty_arg_loc_2.predIdx, empty_arg_loc_2.argIdx),
                Eval(est_pos_ent, est_all_ent - est_already_ent, length + 1)
            ));
        }

        /* Case 4 */
        if (HEAD_PRED_IDX == empty_arg_loc_1.predIdx) {
            if (0 == entailed_record_cnt) {
                for (SimpleRelation* const& relation : *(kb.getRelations())) {
                    for (int arg_idx = 0; arg_idx < relation->getTotalCols(); arg_idx++) {
                        int* const relation_column_values = relation->valuesInColumn(arg_idx);
                        int const num_relation_column_values = relation->numValuesInColumn(arg_idx);
                        double est_pos_ent = eval.getPosEtls() / empty_arg1_pos_column_values.getSize() * empty_arg1_pos_column_values.itemCount(relation_column_values, num_relation_column_values);
                        double est_all_ent = eval.getAllEtls() / kb.totalConstants() * num_relation_column_values;
                        results->push_back(new SpecOprWithScore(
                            new SpecOprCase4(relation->id, relation->getTotalCols(), arg_idx, empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx),
                            Eval(est_pos_ent, est_all_ent, length + 1)
                        ));
                    }
                }
            } else {
                for (SimpleRelation* const& relation : *(kb.getRelations())) {
                    for (int arg_idx = 0; arg_idx < relation->getTotalCols(); arg_idx++) {
                        int* const relation_column_values = relation->valuesInColumn(arg_idx);
                        int const num_relation_column_values = relation->numValuesInColumn(arg_idx);
                        double est_pos_ent = eval.getPosEtls() / empty_arg1_pos_column_values.getSize() * empty_arg1_pos_column_values.itemCount(relation_column_values, num_relation_column_values);
                        double est_already_ent = entailed_record_cnt / empty_arg1_ent_column_values.getSize() * empty_arg1_ent_column_values.itemCount(relation_column_values, num_relation_column_values);
                        double est_all_ent = eval.getAllEtls() / kb.totalConstants() * num_relation_column_values;
                        results->push_back(new SpecOprWithScore(
                            new SpecOprCase4(relation->id, relation->getTotalCols(), arg_idx, empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx),
                            Eval(est_pos_ent, est_all_ent - est_already_ent, length + 1)
                        ));
                    }
                }
            }
        } else {
            if (0 == entailed_record_cnt) {
                for (SimpleRelation* const& relation : *(kb.getRelations())) {
                    for (int arg_idx = 0; arg_idx < relation->getTotalCols(); arg_idx++) {
                        int* const relation_column_values = relation->valuesInColumn(arg_idx);
                        int const num_relation_column_values = relation->numValuesInColumn(arg_idx);
                        double est_pos_ent = eval.getPosEtls() / empty_arg1_pos_column_values.getSize() * empty_arg1_pos_column_values.itemCount(relation_column_values, num_relation_column_values);
                        double est_all_ent = eval.getAllEtls() / empty_arg1_all_column_values.getSize() * empty_arg1_all_column_values.itemCount(relation_column_values, num_relation_column_values);
                        results->push_back(new SpecOprWithScore(
                            new SpecOprCase4(relation->id, relation->getTotalCols(), arg_idx, empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx),
                            Eval(est_pos_ent, est_all_ent, length + 1)
                        ));
                    }
                }
            } else {
                for (SimpleRelation* const& relation : *(kb.getRelations())) {
                    for (int arg_idx = 0; arg_idx < relation->getTotalCols(); arg_idx++) {
                        int* const relation_column_values = relation->valuesInColumn(arg_idx);
                        int const num_relation_column_values = relation->numValuesInColumn(arg_idx);
                        double est_pos_ent = eval.getPosEtls() / empty_arg1_pos_column_values.getSize() * empty_arg1_pos_column_values.itemCount(relation_column_values, num_relation_column_values);
                        double est_already_ent = entailed_record_cnt / empty_arg1_ent_column_values.getSize() * empty_arg1_ent_column_values.itemCount(relation_column_values, num_relation_column_values);
                        double est_all_ent = eval.getAllEtls() / empty_arg1_all_column_values.getSize() * empty_arg1_all_column_values.itemCount(relation_column_values, num_relation_column_values);
                        results->push_back(new SpecOprWithScore(
                            new SpecOprCase4(relation->id, relation->getTotalCols(), arg_idx, empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx),
                            Eval(est_pos_ent, est_all_ent - est_already_ent, length + 1)
                        ));
                    }
                }
            }
        }

        /* Case 5 */
        Predicate const& predicate1 = structure[empty_arg_loc_1.predIdx];
        std::vector<int>* const_list = kb.getPromisingConstants(predicate1.getPredSymbol())[empty_arg_loc_1.argIdx];
        if (HEAD_PRED_IDX == empty_arg_loc_1.predIdx) {
            double est_all_ent = eval.getAllEtls() / kb.totalConstants();
            if (0 == entailed_record_cnt) {
                for (int constant : *const_list) {
                    double est_pos_ent = eval.getPosEtls() / empty_arg1_pos_column_values.getSize() * empty_arg1_pos_column_values.itemCount(constant);
                    results->push_back(new SpecOprWithScore(
                        new SpecOprCase5(empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx, constant),
                        Eval(est_pos_ent, est_all_ent, length + 1)
                    ));
                }
            } else {
                for (int constant : *const_list) {
                    double est_pos_ent = eval.getPosEtls() / empty_arg1_pos_column_values.getSize() * empty_arg1_pos_column_values.itemCount(constant);
                    double est_already_ent = entailed_record_cnt / empty_arg1_ent_column_values.getSize() * empty_arg1_ent_column_values.itemCount(constant);
                    results->push_back(new SpecOprWithScore(
                        new SpecOprCase5(empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx, constant),
                        Eval(est_pos_ent, est_all_ent - est_already_ent, length + 1)
                    ));
                }
            }
        } else {
            if (0 == entailed_record_cnt) {
                for (int constant : *const_list) {
                    double est_pos_ent = eval.getPosEtls() / empty_arg1_pos_column_values.getSize() * empty_arg1_pos_column_values.itemCount(constant);
                    double est_all_ent = eval.getAllEtls() / empty_arg1_all_column_values.getSize() * empty_arg1_all_column_values.itemCount(constant);
                    results->push_back(new SpecOprWithScore(
                        new SpecOprCase5(empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx, constant),
                        Eval(est_pos_ent, est_all_ent, length + 1)
                    ));
                }
            } else {
                for (int constant : *const_list) {
                    double est_pos_ent = eval.getPosEtls() / empty_arg1_pos_column_values.getSize() * empty_arg1_pos_column_values.itemCount(constant);
                    double est_already_ent = entailed_record_cnt / empty_arg1_ent_column_values.getSize() * empty_arg1_ent_column_values.itemCount(constant);
                    double est_all_ent = eval.getAllEtls() / empty_arg1_all_column_values.getSize() * empty_arg1_all_column_values.itemCount(constant);
                    results->push_back(new SpecOprWithScore(
                        new SpecOprCase5(empty_arg_loc_1.predIdx, empty_arg_loc_1.argIdx, constant),
                        Eval(est_pos_ent, est_all_ent - est_already_ent, length + 1)
                    ));
                }
            }
        }
    }

    estIdxMemCost += (sizeof(std::vector<MultiSet<int>*>) + sizeof(MultiSet<int>*) * column_values_in_pos_cache.capacity()) * 3;
    for (int pred_idx = HEAD_PRED_IDX; pred_idx < structure.size(); pred_idx++) {
        Predicate const& predicate = getPredicate(pred_idx);
        for (int arg_idx = 0; arg_idx < predicate.getArity(); arg_idx++) {
        estIdxMemCost += column_values_in_pos_cache[pred_idx][arg_idx].getMemoryCost() + 
            column_values_in_ent_cache[pred_idx][arg_idx].getMemoryCost() +
            column_values_in_all_cache[pred_idx][arg_idx].getMemoryCost();
        }
        delete[] column_values_in_pos_cache[pred_idx];
        delete[] column_values_in_ent_cache[pred_idx];
        delete[] column_values_in_all_cache[pred_idx];
    }
    return results;
}

void EstRule::specCase1UpdateStructure(int const predIdx, int const argIdx, int const varId) {
    Rule::specCase1UpdateStructure(predIdx, argIdx, varId);
    bodyVarLinkManager.specOprCase1(predIdx, argIdx, varId);
}

void EstRule::specCase3UpdateStructure(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) {
    Rule::specCase3UpdateStructure(predIdx1, argIdx1, predIdx2, argIdx2);
    bodyVarLinkManager.specOprCase3(predIdx1, argIdx1, predIdx2, argIdx2);
}

void EstRule::specCase4UpdateStructure(
    int const predSymbol, int const arity, int const argIdx1, int const predIdx2, int const argIdx2
) {
    Rule::specCase4UpdateStructure(predSymbol, arity, argIdx1, predIdx2, argIdx2);
    bodyVarLinkManager.specOprCase4(predIdx2, argIdx2);
}

double EstRule::itemsCountOfKeys(sinc::MultiSet<int> const& counterSet, sinc::MultiSet<int> const& setOfKeys) const {
    int count = 0;
    for (auto& it: setOfKeys.getCntMap()) {
        count += counterSet.itemCount(it.first);
    }
    return count;
}

double EstRule::estimateRatiosInPosCache(double const* const ratios, int const length) const {
    double min = ratios[0];
    for (int i = 1; i < length; i++) {
        min = std::min(min, ratios[i]);
    }
    return min;
}

double EstRule::estimateRatiosInAllCache(double const* const ratios, int const length) const {
    double min = ratios[0];
    for (int i = 1; i < length; i++) {
        min = std::min(min, ratios[i]);
    }
    return min;
}

double EstRule::estimateLinkVarRatio(std::vector<VarLink> const& varLinkPath, std::vector<MultiSet<int>*> columnValuesInCache) const {
    double ratio = 1.0;
    for (VarLink const& link: varLinkPath) {
        MultiSet<int>* column_values_in_pred = columnValuesInCache[link.predIdx];
        ratio *= column_values_in_pred[link.fromArgIdx].getSize() / column_values_in_pred[link.fromArgIdx].differentValues();
    }
    return ratio;
}

/* The followings are methods in `CachedRule` */
void EstRule::updateCacheIndices() {
    uint64_t time_start = currentTimeInNano();
    posCache->buildIndices();
    uint64_t time_pos_done = currentTimeInNano();
    entCache->buildIndices();
    uint64_t time_ent_done = currentTimeInNano();
    for (CacheFragment* const& fragment: *allCache) {
        fragment->buildIndices();
    }
    uint64_t time_all_done = currentTimeInNano();
    posCacheIndexingTime = time_pos_done - time_start;
    entCacheIndexingTime = time_ent_done - time_pos_done;
    allCacheIndexingTime = time_all_done - time_ent_done;
}

sinc::EvidenceBatch* EstRule::getEvidenceAndMarkEntailment() {
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

std::unordered_set<sinc::Record>* EstRule::getCounterexamples() const {
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

void EstRule::releaseMemory() {
    if (maintainPosCache) {
        delete posCache;
        maintainPosCache = false;
    }
    if (maintainEntCache) {
        delete entCache;
        maintainEntCache = false;
    }
    if (maintainAllCache) {
        for (CacheFragment* const& fragment: *allCache) {
            delete fragment;
        }
        delete allCache;
        maintainAllCache = false;
    }
}

uint64_t EstRule::getCopyTime() const {
    return copyTime;
}

uint64_t EstRule::getPosCacheUpdateTime() const {
    return posCacheUpdateTime;
}

uint64_t EstRule::getEntCacheUpdateTime() const {
    return entCacheUpdateTime;
}

uint64_t EstRule::getAllCacheUpdateTime() const {
    return allCacheUpdateTime;
}

uint64_t EstRule::getPosCacheIndexingTime() const {
    return posCacheIndexingTime;
}

uint64_t EstRule::getEntCacheIndexingTime() const {
    return entCacheIndexingTime;
}

uint64_t EstRule::getAllCacheIndexingTime() const {
    return allCacheIndexingTime;
}

using sinc::CacheFragment;
const CacheFragment& EstRule::getPosCache() const {
    return *posCache;
}

const CacheFragment& EstRule::getEntCache() const {
    return *entCache;
}

const std::vector<CacheFragment*>& EstRule::getAllCache() const {
    return *allCache;
}

size_t EstRule::getEstIdxMemCost() const {
    return estIdxMemCost;
}

void EstRule::obtainPosCache() {
    if (!maintainPosCache) {
        posCache = new CacheFragment(*posCache);
        maintainPosCache = true;
    }
}

void EstRule::obtainEntCache() {
    if (!maintainEntCache) {
        entCache = new CacheFragment(*entCache);
        maintainEntCache = true;
    }
}

void EstRule::obtainAllCache() {
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

double EstRule::recordCoverage() {
    return ((double) posCache->countTableSize(HEAD_PRED_IDX)) / kb.getRelation(getHead().getPredSymbol())->getTotalRows();
}

sinc::Eval EstRule::calculateEval() {
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

    /* Count the number of entailments */
    double all_ent = pow(kb.totalConstants(), head_uv_cnt + head_only_lvs.size());
    for (int i = 0; i < allCache->size(); i++) {
        std::vector<int> const& vids = gvs_in_all_cache_fragments[i];
        if (!vids.empty()) {
            CacheFragment const& fragment = *(*allCache)[i];
            all_ent *= fragment.countCombinations(vids);
        }
    }
    int new_pos_ent = posCache->countTableSize(HEAD_PRED_IDX);
    int already_ent = entCache->countTableSize(HEAD_PRED_IDX);

    /* Update evaluation score */
    /* Those already proved should be excluded from the entire entailment set. Otherwise, they are counted as negative ones */
    return Eval(new_pos_ent, all_ent - already_ent, length);
}

sinc::UpdateStatus EstRule::specCase1HandlerPrePruning(int const predIdx, int const argIdx, int const varId) {
    uint64_t time_start = currentTimeInNano();
    obtainPosCache();
    posCache->updateCase1a(predIdx, argIdx, varId);
    uint64_t time_done = currentTimeInNano();
    posCacheUpdateTime = time_done - time_start;
    return UpdateStatus::Normal;
}

sinc::UpdateStatus EstRule::specCase1HandlerPostPruning(int const predIdx, int const argIdx, int const varId) {
    uint64_t time_start = currentTimeInNano();
    obtainEntCache();
    entCache->updateCase1a(predIdx, argIdx, varId);
    uint64_t time_ent_done = currentTimeInNano();

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
    entCacheUpdateTime = time_ent_done - time_start;
    allCacheUpdateTime = time_all_done - time_ent_done;

    return UpdateStatus::Normal;
}

sinc::UpdateStatus EstRule::specCase2HandlerPrePruning(int const predSymbol, int const arity, int const argIdx, int const varId) {
    uint64_t time_start = currentTimeInNano();
    obtainPosCache();
    SimpleRelation* new_relation = kb.getRelation(predSymbol);
    posCache->updateCase1b(new_relation, predSymbol, argIdx, varId);
    uint64_t time_done = currentTimeInNano();
    posCacheUpdateTime = time_done - time_start;
    return UpdateStatus::Normal;
}

sinc::UpdateStatus EstRule::specCase2HandlerPostPruning(int const predSymbol, int const arity, int const argIdx, int const varId) {
    uint64_t time_start = currentTimeInNano();
    obtainEntCache();
    SimpleRelation* new_relation = kb.getRelation(predSymbol);
    entCache->updateCase1b(new_relation, predSymbol, argIdx, varId);
    uint64_t time_ent_done = currentTimeInNano();

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
    entCacheUpdateTime = time_ent_done - time_start;
    allCacheUpdateTime = time_all_done - time_ent_done;

    return UpdateStatus::Normal;
}

sinc::UpdateStatus EstRule::specCase3HandlerPrePruning(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) {
    uint64_t time_start = currentTimeInNano();
    obtainPosCache();
    posCache->updateCase2a(predIdx1, argIdx1, predIdx2, argIdx2, usedLimitedVars() - 1);
    uint64_t time_done = currentTimeInNano();
    posCacheUpdateTime = time_done - time_start;
    return UpdateStatus::Normal;
}

sinc::UpdateStatus EstRule::specCase3HandlerPostPruning(int const predIdx1, int const argIdx1, int const predIdx2, int const argIdx2) {
    uint64_t time_start = currentTimeInNano();
    obtainEntCache();
    int new_vid = usedLimitedVars() - 1;
    entCache->updateCase2a(predIdx1, argIdx1, predIdx2, argIdx2, new_vid);
    uint64_t time_ent_done = currentTimeInNano();

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
    entCacheUpdateTime = time_ent_done - time_start;
    allCacheUpdateTime = time_all_done - time_ent_done;

    return UpdateStatus::Normal;
}

sinc::UpdateStatus EstRule::specCase4HandlerPrePruning(
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

sinc::UpdateStatus EstRule::specCase4HandlerPostPruning(
    int const predSymbol, int const arity, int const argIdx1, int const predIdx2, int const argIdx2
) {
    long time_start = currentTimeInNano();
    SimpleRelation* new_relation = kb.getRelation(predSymbol);
    obtainEntCache();
    int new_vid = usedLimitedVars() - 1;
    entCache->updateCase2b(new_relation, predSymbol, argIdx1, predIdx2, argIdx2, new_vid);
    uint64_t time_ent_done = currentTimeInNano();

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
    entCacheUpdateTime = time_ent_done - time_start;
    allCacheUpdateTime = time_all_done - time_ent_done;

    return UpdateStatus::Normal;
}

sinc::UpdateStatus EstRule::specCase5HandlerPrePruning(int const predIdx, int const argIdx, int const constant) {
    uint64_t time_start = currentTimeInNano();
    obtainPosCache();
    posCache->updateCase3(predIdx, argIdx, constant);
    uint64_t time_done = currentTimeInNano();
    posCacheUpdateTime = time_done - time_start;
    return UpdateStatus::Normal;
}

sinc::UpdateStatus EstRule::specCase5HandlerPostPruning(int const predIdx, int const argIdx, int const constant) {
    uint64_t time_start = currentTimeInNano();
    obtainEntCache();
    entCache->updateCase3(predIdx, argIdx, constant);
    uint64_t time_ent_done = currentTimeInNano();

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
    entCacheUpdateTime = time_ent_done - time_start;
    allCacheUpdateTime = time_all_done - time_ent_done;

    return UpdateStatus::Normal;
}

sinc::UpdateStatus EstRule::generalizeHandlerPrePruning(int const predIdx, int const argIdx) {
    return UpdateStatus::Invalid;
}

sinc::UpdateStatus EstRule::generalizeHandlerPostPruning(int const predIdx, int const argIdx) {
    return UpdateStatus::Invalid;
}

void EstRule::mergeFragmentIndices(int const baseFragmentIdx, int const mergingFragmentIdx) {
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

void EstRule::generateHeadTemplates(
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

void EstRule::expandHeadUvs4CounterExamples(
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
 * EstRelationMiner
 */
using sinc::EstRelationMiner;
EstRelationMiner::EstRelationMiner(
    SimpleKb& kb, int const targetRelation, EvalMetric::Value evalMetric, int const beamwidth, double const _observationRatio,
    double const stopCompressionRatio, nodeMapType& predicate2NodeMap, depGraphType& dependencyGraph,
    std::vector<Rule*>& hypothesis, std::unordered_set<Record>& counterexamples, std::ostream& logger
) : RelationMiner(
        kb, targetRelation, evalMetric, beamwidth, stopCompressionRatio, predicate2NodeMap, dependencyGraph, hypothesis,
        counterexamples, logger
) , observationRatio(_observationRatio) {}

EstRelationMiner::~EstRelationMiner() {
    for (Rule::fingerprintCacheType* const& cache: fingerprintCaches) {
        for (const Fingerprint* const& fp: *cache) {
            delete fp;
        }
        delete cache;
    }
}

using sinc::Rule;
Rule* EstRelationMiner::getStartRule() {
    Rule::fingerprintCacheType* cache = new Rule::fingerprintCacheType();
    fingerprintCaches.push_back(cache);
    return new EstRule(targetRelation, kb.getRelation(targetRelation)->getTotalCols(), *cache, tabuMap, kb);
}

void EstRelationMiner::selectAsBeam(Rule* r) {
    EstRule* rule = (EstRule*) r;
    rule->updateCacheIndices();
    monitor.posCacheIndexingTime += rule->getPosCacheIndexingTime();
    monitor.entCacheIndexingTime += rule->getEntCacheIndexingTime();
    monitor.allCacheIndexingTime += rule->getAllCacheIndexingTime();
}

Rule* EstRelationMiner::findRule() {
    /* Create the beams */
    Rule** beams = new Rule*[beamwidth]{};
    beams[0] = getStartRule();

    /* Find a local optimum (there is certainly a local optimum in the search routine) */
    while (true) {
        /* Find the candidates in the next round according to current beams */
        Rule** top_candidates = new Rule*[beamwidth]{};
        std::vector<SpecOprWithScore*>** estimated_spec_lists = new std::vector<SpecOprWithScore*>*[beamwidth]{};
        for (int i = 0; i < beamwidth && nullptr != beams[i]; i++) {
            Rule* const r = beams[i];
            selectAsBeam(r);
            logFormatter.printf("Extend: %s\n", r->toString(kb.getRelationNames()).c_str());
            logger.flush();

            std::vector<SpecOprWithScore*>* estimated_specs = ((EstRule*)r)->estimateSpecializations();
            std::sort(
                estimated_specs->begin(), estimated_specs->end(),
                [this](SpecOprWithScore* const& a, SpecOprWithScore* const& b) -> bool {
                    return a->estEval.value(evalMetric) > b->estEval.value(evalMetric);
                }
            );
            estimated_spec_lists[i] = estimated_specs;
            monitor.maxEstIdxCost = std::max(monitor.maxEstIdxCost, ((EstRule*)r)->getEstIdxMemCost());
        }
        findEstimatedSpecializations(beams, estimated_spec_lists, top_candidates);
        for (int i = 0; i < beamwidth && nullptr != beams[i]; i++) {
            for (SpecOprWithScore* const& s: *(estimated_spec_lists[i])) {
                delete s;
            }
            delete estimated_spec_lists[i];
        }
        delete[] estimated_spec_lists;

        if (!shouldContinue) {
            /* Stop the finding procedure at the current stage and return the best rule */
            Rule* best_rule = beams[0];
            for (int i = 1; i < beamwidth && nullptr != beams[i]; i++) {
                if (best_rule->getEval().value(evalMetric) < beams[i]->getEval().value(evalMetric)) {
                    delete best_rule;
                    best_rule = beams[i];
                } else {
                    delete beams[i];
                }
            }
            for (int i = 0; i < beamwidth && nullptr != top_candidates[i]; i++) {
                if (best_rule->getEval().value(evalMetric) < top_candidates[i]->getEval().value(evalMetric)) {
                    delete best_rule;
                    best_rule = top_candidates[i];
                } else {
                    delete top_candidates[i];
                }
            }
            if (nullptr != best_rule) {
                if (!best_rule->getEval().useful()) {
                    delete best_rule;
                    best_rule = nullptr;
                }
            }
            return best_rule;
        }

        /* Find the best in beams and candidates */
        Rule* best_beam = beams[0];
        for (int i = 1; i < beamwidth && nullptr != beams[i]; i++) {
            if (best_beam->getEval().value(evalMetric) < beams[i]->getEval().value(evalMetric)) {
                best_beam = beams[i];
            }
        }
        Rule* best_candidate = nullptr;
        if (nullptr != top_candidates[0]) {
            best_candidate = top_candidates[0];
            for (int i = 1; i < beamwidth && nullptr != top_candidates[i]; i++) {
                if (best_candidate->getEval().value(evalMetric) < top_candidates[i]->getEval().value(evalMetric)) {
                    best_candidate = top_candidates[i];
                }
            }
        }

        /* If there is a local optimum and it is the best among all, return the rule */
        if (nullptr == best_candidate || best_beam->getEval().value(evalMetric) >= best_candidate->getEval().value(evalMetric)) {
            /* If the best is not useful, return NULL */
            Rule* const ret = best_beam->getEval().useful() ? best_beam : nullptr;
            for (int i = 0; i < beamwidth && nullptr != beams[i]; i++) {
                if (ret != beams[i]) {
                    delete beams[i];
                }
            }
            delete[] beams;
            for (int i = 0; i < beamwidth && nullptr != top_candidates[i]; i++) {
                delete top_candidates[i];
            }
            delete[] top_candidates;
            return ret;
        }

        /* If the best rule reaches the stopping threshold, return the rule */
        /* The "best_candidate" is certainly not NULL if the workflow goes here */
        /* Assumption: the stopping threshold is no less than the threshold of usefulness */
        const Eval& best_eval = best_candidate->getEval();
        if (stopCompressionRatio <= best_eval.value(EvalMetric::Value::CompressionRatio) || 0 == best_eval.getNegEtls()) {
            Rule* const ret = best_eval.useful() ? best_candidate : nullptr;
            for (int i = 0; i < beamwidth && nullptr != beams[i]; i++) {
                delete beams[i];
            }
            delete[] beams;
            for (int i = 0; i < beamwidth && nullptr != top_candidates[i]; i++) {
                if (ret != top_candidates[i]) {
                    delete top_candidates[i];
                }
            }
            delete[] top_candidates;
            return ret;
        }

        /* Update the beams */
        for (int i = 0; i < beamwidth && nullptr != beams[i]; i++) {
            delete beams[i];
        }
        delete[] beams;
        beams = top_candidates;
    }
}

void EstRelationMiner::findEstimatedSpecializations(
    Rule** beams, std::vector<SpecOprWithScore*>** estimatedSpecLists, Rule** topCandidates
) {
    int observations = (int) std::round(beamwidth * observationRatio);
    int num_beams = 1;  // at least 1 beam in the "beams" array
    for (; num_beams < beamwidth && nullptr != beams[num_beams]; num_beams++); // find the exact number of beams in the array
    int idxs[num_beams]{};
    for (int i = 0; i < observations; i++) {
        int best_rule_idx = -1;
        double best_score = -std::numeric_limits<double>::infinity();
        for (int rule_idx = 0; rule_idx < num_beams; rule_idx++) {
            std::vector<SpecOprWithScore*>& est_spec_list = *(estimatedSpecLists[rule_idx]);
            int idx = idxs[rule_idx];
            if (idx < est_spec_list.size()) {
                double score = est_spec_list[idx]->estEval.value(evalMetric);
                if (best_score < score) {
                    best_score = score;
                    best_rule_idx = rule_idx;
                }
            }
        }
        if (-1 == best_rule_idx) {
            /* No more option to compare */
            break;
        }
        SpecOprWithScore* best_spec = (*(estimatedSpecLists[best_rule_idx]))[idxs[best_rule_idx]];
        idxs[best_rule_idx]++;
        Rule* copy = beams[best_rule_idx]->clone();
        UpdateStatus status = best_spec->opr->specialize(*copy);
        checkThenAddRule(status, copy, *(beams[best_rule_idx]), topCandidates);
    }
}

int EstRelationMiner::checkThenAddRule(UpdateStatus updateStatus, Rule* const updatedRule, Rule& originalRule, Rule** candidates) {
    EstRule* rule = (EstRule*) updatedRule;
    if (UpdateStatus::Normal == updateStatus) {
        monitor.posCacheEntriesTotal += rule->getPosCache().getEntries().size();
        monitor.entCacheEntriesTotal += rule->getEntCache().getEntries().size();
        int all_cache_entries = 0;
        if (!rule->getAllCache().empty()) {
            for (CacheFragment* const& fragment : rule->getAllCache()) {
                all_cache_entries += fragment->getEntries().size();
            }
            all_cache_entries /= rule->getAllCache().size();
        }
        monitor.allCacheEntriesTotal += all_cache_entries;
        monitor.posCacheEntriesMax = std::max(monitor.posCacheEntriesMax, (int)rule->getPosCache().getEntries().size());
        monitor.entCacheEntriesMax = std::max(monitor.entCacheEntriesMax, (int)rule->getEntCache().getEntries().size());
        monitor.allCacheEntriesMax = std::max(monitor.allCacheEntriesMax, all_cache_entries);
        monitor.totalGeneratedRules++;
        monitor.posCacheUpdateTime += rule->getPosCacheUpdateTime();
        monitor.entCacheUpdateTime += rule->getEntCacheUpdateTime();
        monitor.allCacheUpdateTime += rule->getAllCacheUpdateTime();
    } else {
        monitor.prunedPosCacheUpdateTime += rule->getPosCacheUpdateTime();
    }
    monitor.copyTime += rule->getCopyTime();

    return RelationMiner::checkThenAddRule(updateStatus, updatedRule, originalRule, candidates);
}

/**
 * SincWithEstimation
 */
using sinc::SincWithEstimation;
using sinc::SincRecovery;
using sinc::RelationMiner;
SincWithEstimation::SincWithEstimation(SincConfig* const config) : SInC(config) {}

SincWithEstimation::SincWithEstimation(SincConfig* const config, SimpleKb* const kb) : SInC(config, kb) {}

void SincWithEstimation::getTargetRelations(int* & targetRelationIds, int& numTargets) {
    SInC::getTargetRelations(targetRelationIds, numTargets);
    CompliedBlock::reserveMemSpace(*kb);
}

SincRecovery* SincWithEstimation::createRecovery() {
    return nullptr; // Todo: Implement here
}

RelationMiner* SincWithEstimation::createRelationMiner(int const targetRelationNum) {
    return new EstRelationMiner(
        *kb, targetRelationNum, config->evalMetric, config->beamwidth, config->observationRatio, config->stopCompressionRatio,
        predicate2NodeMap, dependencyGraph, compressedKb->getHypothesis(), compressedKb->getCounterexampleSet(targetRelationNum),
        *logger
    );
}

void SincWithEstimation::finalizeRelationMiner(RelationMiner* miner) {
    SInC::finalizeRelationMiner(miner);
    EstRelationMiner* rel_miner = (EstRelationMiner*) miner;
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
    monitor.maxEstIdxCost = std::max(monitor.maxEstIdxCost, rel_miner->monitor.maxEstIdxCost);
    CompliedBlock::clearPool();

    /* Log memory usage */
    rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    (*logger) << "Finalized Max Mem:" << monitor.formatMemorySize(usage.ru_maxrss) << std::endl;
}

void SincWithEstimation::showMonitor() {
    SInC::showMonitor();

    /* Calculate memory cost */
    monitor.cbMemCost = CompliedBlock::totalCbMemoryCost() / 1024;
    monitor.maxEstIdxCost /= 1024;

    monitor.show(*logger);
}

void SincWithEstimation::finish() {
    CompliedBlock::clearPool();
}
