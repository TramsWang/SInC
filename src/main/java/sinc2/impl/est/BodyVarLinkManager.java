package sinc2.impl.est;

import sinc2.common.Argument;
import sinc2.common.Predicate;
import sinc2.rule.Rule;

import java.util.*;

/**
 * This class is used for quickly determine whether two LVs in a rule is linked in the body. It also searches for the
 * shortest link path of these vars.
 *
 * @since 2.1
 */
public class BodyVarLinkManager {

    /** The structure of the rule */
    protected final List<Predicate> rule;
    /** The labels of LVs, denoting the linked component they belong to */
    protected final List<Integer> varLabels;
    /** The link graph of LVs. There is an edge between two vars if they appear in a same predicate */
    protected final List<Set<VarLink>> varLinkGraph;

    /**
     * Construct an instance
     *
     * @param rule        The current structure of the rule
     * @param currentVars The number of LVs used in the rule
     */
    public BodyVarLinkManager(List<Predicate> rule, int currentVars) {
        this.rule = rule;
        varLabels = new ArrayList<>(currentVars);
        varLinkGraph = new ArrayList<>(currentVars);
        for (int i = 0; i < currentVars; i++) {
            varLabels.add(i);
            varLinkGraph.add(new HashSet<>());
        }
        for (int pred_idx = Rule.FIRST_BODY_PRED_IDX; pred_idx < rule.size(); pred_idx++) {
            Predicate predicate = rule.get(pred_idx);
            for (int arg_idx1 = 0; arg_idx1 < predicate.args.length; arg_idx1++) {
                int argument1 = predicate.args[arg_idx1];
                if (Argument.isVariable(argument1)) {
                    int var_id1 = Argument.decode(argument1);
                    int var_label1 = varLabels.get(var_id1);
                    for (int arg_idx2 = arg_idx1 + 1; arg_idx2 < predicate.args.length; arg_idx2++) {
                        int argument2 = predicate.args[arg_idx2];
                        if (Argument.isVariable(argument2)) {
                            int var_id2 = Argument.decode(argument2);
                            int var_label2 = varLabels.get(var_id2);
                            if (var_label1 != var_label2) {
                                for (int vid = 0; vid < currentVars; vid++) {
                                    if (varLabels.get(vid) == var_label2) {
                                        varLabels.set(vid, var_label1);
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
            }
        }

        for (int pred_idx = Rule.FIRST_BODY_PRED_IDX; pred_idx < rule.size(); pred_idx++) {
            Predicate predicate = rule.get(pred_idx);
            for (int arg_idx1 = 0; arg_idx1 < predicate.args.length; arg_idx1++) {
                int argument1 = predicate.args[arg_idx1];
                if (Argument.isVariable(argument1)) {
                    int var_id1 = Argument.decode(argument1);
                    for (int arg_idx2 = arg_idx1 + 1; arg_idx2 < predicate.args.length; arg_idx2++) {
                        int argument2 = predicate.args[arg_idx2];
                        if (Argument.isVariable(argument2)) {
                            int var_id2 = Argument.decode(argument2);
                            if (var_id1 != var_id2) {
                                varLinkGraph.get(var_id1).add(new VarLink(pred_idx, var_id1, arg_idx1, var_id2, arg_idx2));
                                varLinkGraph.get(var_id2).add(new VarLink(pred_idx, var_id2, arg_idx2, var_id1, arg_idx1));
                            }
                        }
                    }
                }
            }
        }
    }

    /**
     * Copy constructor.
     *
     * NOTE: the var link manager instance should be linked to the copied rule structure, instead of the original one
     *
     * @param another Another instance
     * @param newRule The copied rule structure in the copied rule
     */
    public BodyVarLinkManager(BodyVarLinkManager another, List<Predicate> newRule) {
        this.rule = newRule;
        this.varLabels = new ArrayList<>(another.varLabels);
        this.varLinkGraph = new ArrayList<>(another.varLinkGraph.size());
        for (Set<VarLink> links: another.varLinkGraph) {
            this.varLinkGraph.add(new HashSet<>(links));
        }
    }

    /**
     * Update the instance via case-1 specialization
     */
    public void specOprCase1(int predIdx, int argIdx, int varId) {
        if (Rule.HEAD_PRED_IDX == predIdx) {
            return;
        }
        int var_label = varLabels.get(varId);
        Predicate predicate = rule.get(predIdx);
        for (int arg_idx2 = 0; arg_idx2 < predicate.args.length; arg_idx2++) {
            int argument2 = predicate.args[arg_idx2];
            if (Argument.isVariable(argument2) && arg_idx2 != argIdx) {
                int var_id2 = Argument.decode(argument2);
                int var_label2 = varLabels.get(var_id2);
                if (var_label != var_label2) {
                    for (int vid = 0; vid < varLabels.size(); vid++) {
                        if (varLabels.get(vid) == var_label2) {
                            varLabels.set(vid, var_label);
                        }
                    }
                }
                break;
            }
        }

        for (int arg_idx2 = 0; arg_idx2 < predicate.args.length; arg_idx2++) {
            int argument2 = predicate.args[arg_idx2];
            if (Argument.isVariable(argument2)) {
                int var_id2 = Argument.decode(argument2);
                if (varId != var_id2) {
                    varLinkGraph.get(varId).add(new VarLink(predIdx, varId, argIdx, var_id2, arg_idx2));
                    varLinkGraph.get(var_id2).add(new VarLink(predIdx, var_id2, arg_idx2, varId, argIdx));
                }
            }
        }
    }

    /**
     * Update the instance via case-3 specialization operation
     */
    public void specOprCase3(int predIdx1, int argIdx1, int predIdx2, int argIdx2) {
        if (Rule.HEAD_PRED_IDX == predIdx1) {
            if (Rule.HEAD_PRED_IDX == predIdx2) {
                varLabels.add(varLabels.size());
            } else {
                int var_label2 = -1;
                Predicate predicate = rule.get(predIdx2);
                for (int arg_idx = 0; arg_idx < predicate.args.length; arg_idx++) {
                    int argument = predicate.args[arg_idx];
                    if (Argument.isVariable(argument) && arg_idx != argIdx2) {
                        var_label2 = varLabels.get(Argument.decode(argument));
                        break;
                    }
                }
                varLabels.add(var_label2);
            }
        } else if (Rule.HEAD_PRED_IDX == predIdx2) {
            int var_label1 = -1;
            Predicate predicate = rule.get(predIdx1);
            for (int arg_idx = 0; arg_idx < predicate.args.length; arg_idx++) {
                int argument = predicate.args[arg_idx];
                if (Argument.isVariable(argument) && arg_idx != argIdx1) {
                    var_label1 = varLabels.get(Argument.decode(argument));
                    break;
                }
            }
            varLabels.add(var_label1);
        } else {
            int var_label1 = -1;
            Predicate predicate = rule.get(predIdx1);
            for (int arg_idx = 0; arg_idx < predicate.args.length; arg_idx++) {
                int argument = predicate.args[arg_idx];
                if (Argument.isVariable(argument) && arg_idx != argIdx1) {
                    var_label1 = varLabels.get(Argument.decode(argument));
                    break;
                }
            }
            int var_label2 = -1;
            predicate = rule.get(predIdx2);
            for (int arg_idx = 0; arg_idx < predicate.args.length; arg_idx++) {
                int argument = predicate.args[arg_idx];
                if (Argument.isVariable(argument) && arg_idx != argIdx2) {
                    var_label2 = varLabels.get(Argument.decode(argument));
                    break;
                }
            }
            if (var_label1 != var_label2) {
                for (int vid = 0; vid < varLabels.size(); vid++) {
                    if (varLabels.get(vid) == var_label2) {
                        varLabels.set(vid, var_label1);
                    }
                }
            }
            varLabels.add(var_label1);
        }

        int new_vid = varLinkGraph.size();
        varLinkGraph.add(new HashSet<>());
        if (Rule.HEAD_PRED_IDX != predIdx1) {
            Predicate predicate = rule.get(predIdx1);
            for (int arg_idx = 0; arg_idx < predicate.args.length; arg_idx++) {
                int argument = predicate.args[arg_idx];
                if (Argument.isVariable(argument)) {
                    int vid = Argument.decode(argument);
                    if (new_vid != vid) {
                        varLinkGraph.get(new_vid).add(new VarLink(predIdx1, new_vid, argIdx1, vid, arg_idx));
                        varLinkGraph.get(vid).add(new VarLink(predIdx1, vid, arg_idx, new_vid, argIdx1));
                    }
                }
            }
        }
        if (Rule.HEAD_PRED_IDX != predIdx2 && predIdx1 != predIdx2) {
            Predicate predicate = rule.get(predIdx2);
            for (int arg_idx = 0; arg_idx < predicate.args.length; arg_idx++) {
                int argument = predicate.args[arg_idx];
                if (Argument.isVariable(argument)) {
                    int vid = Argument.decode(argument);
                    if (new_vid != vid) {
                        varLinkGraph.get(new_vid).add(new VarLink(predIdx2, new_vid, argIdx2, vid, arg_idx));
                        varLinkGraph.get(vid).add(new VarLink(predIdx2, vid, arg_idx, new_vid, argIdx2));
                    }
                }
            }
        }
    }

    /**
     * Update the instance via case-4 specialization operation. Here only the index of the predicate that was NOT newly
     * added should be passed here, as well as the corresponding argument index.
     *
     * @param predIdx The index of the predicate that was NOT newly added
     * @param argIdx  Corresponding index of the argument in the predicate
     */
    public void specOprCase4(int predIdx, int argIdx) {
        int new_vid = varLinkGraph.size();
        varLinkGraph.add(new HashSet<>());
        if (Rule.HEAD_PRED_IDX == predIdx) {
            varLabels.add(varLabels.size());
        } else {
            int var_label = -1;
            Predicate predicate = rule.get(predIdx);
            for (int arg_idx = 0; arg_idx < predicate.args.length; arg_idx++) {
                int argument = predicate.args[arg_idx];
                if (Argument.isVariable(argument) && arg_idx != argIdx) {
                    var_label = varLabels.get(Argument.decode(argument));
                    break;
                }
            }
            varLabels.add(var_label);

            for (int arg_idx = 0; arg_idx < predicate.args.length; arg_idx++) {
                int argument = predicate.args[arg_idx];
                if (Argument.isVariable(argument)) {
                    int vid = Argument.decode(argument);
                    if (new_vid != vid) {
                        varLinkGraph.get(new_vid).add(new VarLink(predIdx, new_vid, argIdx, vid, arg_idx));
                        varLinkGraph.get(vid).add(new VarLink(predIdx, vid, arg_idx, new_vid, argIdx));
                    }
                }
            }
        }
    }

    /**
     * Return the newly linked LV pairs if the rule is updated by a case-1 specialization.
     *
     * @param predIdx NOTE: this parameter should not be 0 (Rule.HEAD_PRED_IDX) here
     */
    public Set<VarPair> assumeSpecOprCase1(int predIdx, int argIdx, int varId) {
        int var_label = varLabels.get(varId);
        Predicate predicate = rule.get(predIdx);
        int var_label2 = -1;
        for (int arg_idx2 = 0; arg_idx2 < predicate.args.length; arg_idx2++) {
            int argument2 = predicate.args[arg_idx2];
            if (Argument.isVariable(argument2) && arg_idx2 != argIdx) {
                var_label2 = varLabels.get(Argument.decode(argument2));
                break;
            }
        }
        Set<VarPair> new_linked_pairs = new HashSet<>();
        if (var_label != var_label2) {
            for (int vid = 0; vid < varLabels.size(); vid++) {
                if (var_label == varLabels.get(vid)) {
                    for (int vid2 = 0; vid2 < varLabels.size(); vid2++) {
                        if (var_label2 == varLabels.get(vid2)) {
                            new_linked_pairs.add(new VarPair(vid, vid2));
                        }
                    }
                }
            }
        }
        return new_linked_pairs;
    }

    /**
     * Return the newly linked LV pairs if the rule is updated by a case-3 specialization.
     *
     * @param predIdx1 NOTE: this parameter should not be 0 (Rule.HEAD_PRED_IDX) here, and predIdx1 != predIdx2
     * @param predIdx2 NOTE: this parameter should not be 0 (Rule.HEAD_PRED_IDX) here, and predIdx1 != predIdx2
     */
    public Set<VarPair> assumeSpecOprCase3(int predIdx1, int argIdx1, int predIdx2, int argIdx2) {
        int var_label1 = -1;
        Predicate predicate = rule.get(predIdx1);
        for (int arg_idx = 0; arg_idx < predicate.args.length; arg_idx++) {
            int argument = predicate.args[arg_idx];
            if (Argument.isVariable(argument) && arg_idx != argIdx1) {
                var_label1 = varLabels.get(Argument.decode(argument));
                break;
            }
        }
        int var_label2 = -1;
        predicate = rule.get(predIdx2);
        for (int arg_idx = 0; arg_idx < predicate.args.length; arg_idx++) {
            int argument = predicate.args[arg_idx];
            if (Argument.isVariable(argument) && arg_idx != argIdx2) {
                var_label2 = varLabels.get(Argument.decode(argument));
                break;
            }
        }
        Set<VarPair> new_linked_pairs = new HashSet<>();
        int new_vid = varLabels.size();
        if (var_label1 != var_label2) {
            for (int vid1 = 0; vid1 < varLabels.size(); vid1++) {
                if (var_label1 == varLabels.get(vid1)) {
                    for (int vid2 = 0; vid2 < varLabels.size(); vid2++) {
                        if (var_label2 == varLabels.get(vid2)) {
                            new_linked_pairs.add(new VarPair(vid1, vid2));
                        }
                    }
                    new_linked_pairs.add(new VarPair(vid1, new_vid));
                }
            }
        }
        for (int vid2 = 0; vid2 < varLabels.size(); vid2++) {
            if (var_label2 == varLabels.get(vid2)) {
                new_linked_pairs.add(new VarPair(vid2, new_vid));
            }
        }
        return new_linked_pairs;
    }

    /**
     * Find the shortest (directed) link path between two LVs.
     *
     * @return NULL if the two LVs are not linked.
     */
    public VarLink[] shortestPath(int fromVid, int toVid) {
        if (!Objects.equals(varLabels.get(fromVid), varLabels.get(toVid))) {
            /* The two variables are not linked */
            return null;
        }

        /* The two variables are certainly linked */
        boolean[] visited_vid = new boolean[varLinkGraph.size()];
        visited_vid[fromVid] = true;
        List<VarLink> bfs = new ArrayList<>();
        List<Integer> predecessor_idx = new ArrayList<>();
        for (VarLink link: varLinkGraph.get(fromVid)) {
            if (!visited_vid[link.toVid]) {
                if (link.toVid == toVid) {
                    return new VarLink[]{link};
                }
                bfs.add(link);
                predecessor_idx.add(-1);
                visited_vid[link.toVid] = true;
            }
        }

        /* The two variables are linked by paths no shorter than 2 */
        return shortestPathBfsHandler(visited_vid, bfs, predecessor_idx, toVid);
    }

    /**
     * This method is to continue a BFS search based on a certain starting status.
     *
     * @param visitedVid     An array denoting whether an LV has been visited
     * @param bfs            The BFS array of edges
     * @param predecessorIdx The array where each element denoting the index of the predecessor edge of the corresponding edge in 'bfs'
     * @param toVid          The target LV
     * @return The shortest path to the target LV, or NULL if no such path.
     */
    protected VarLink[] shortestPathBfsHandler(boolean[] visitedVid, List<VarLink> bfs, List<Integer> predecessorIdx, int toVid) {
        for (int i = 0; i < bfs.size(); i++) {
            VarLink link = bfs.get(i);
            for (VarLink next_link: varLinkGraph.get(link.toVid)) {
                if (!visitedVid[next_link.toVid]) {
                    if (next_link.toVid == toVid) {
                        List<VarLink> reversed_path = new ArrayList<>();
                        reversed_path.add(next_link);
                        reversed_path.add(link);
                        for (int edge_idx = predecessorIdx.get(i); edge_idx >= 0; edge_idx = predecessorIdx.get(edge_idx)) {
                            reversed_path.add(bfs.get(edge_idx));
                        }
                        VarLink[] path = new VarLink[reversed_path.size()];
                        for (int edge_idx = 0; edge_idx < path.length; edge_idx++) {
                            path[edge_idx] = reversed_path.get(path.length - 1 - edge_idx);
                        }
                        return path;
                    }
                    bfs.add(next_link);
                    predecessorIdx.add(i);
                    visitedVid[next_link.toVid] = true;
                }
            }
        }
        return null;
    }

    /**
     * This method is for finding the shortest path from the given argument to an LV.
     *
     * @param predIdx        The index of the predicate of the starting argument
     * @param argIdx         The corresponding argument index
     * @param assumedFromVid An assumed ID of the argument. NOTE: this ID should be different from existing LVs
     * @param toVid          The target LV
     * @return The shortest path, or NULL of none.
     */
    protected VarLink[] assumeShortestPathHandler(int predIdx, int argIdx, int assumedFromVid, int toVid) {
        boolean[] visited_vid = new boolean[varLinkGraph.size()];
        if (assumedFromVid >= 0 && assumedFromVid < visited_vid.length) {
            visited_vid[assumedFromVid] = true;
        }
        List<VarLink> bfs = new ArrayList<>();
        List<Integer> predecessor_idx = new ArrayList<>();
        int[] pred_args = rule.get(predIdx).args;
        for (int arg_idx = 0; arg_idx < pred_args.length; arg_idx++) {
            int argument = pred_args[arg_idx];
            if (Argument.isVariable(argument)) {
                int vid = Argument.decode(argument);
                if (!visited_vid[vid]) {
                    VarLink link = new VarLink(predIdx, assumedFromVid, argIdx, vid, arg_idx);
                    if (vid == toVid) {
                        return new VarLink[]{link};
                    }
                    bfs.add(link);
                    predecessor_idx.add(-1);
                    visited_vid[vid] = true;
                }
            }
        }
        return shortestPathBfsHandler(visited_vid, bfs, predecessor_idx, toVid);
    }

    /**
     * Find the shortest path between two LVs if a certain UV is turned to an existing LV via case-1 specialization.
     *
     * @param predIdx NOTE: this parameter should not be 0 (Rule.HEAD_PRED_IDX) here
     * @return NULL if no such path
     */
    public VarLink[] assumeShortestPathCase1(int predIdx, int argIdx, int varId, int fromVid, int toVid) {
        int start_vid = fromVid;    // start_vid -> varId
        int end_vid = toVid;        // new arg -> end_vid
        boolean reversed = false;
        if (Objects.equals(varLabels.get(varId), varLabels.get(toVid))) {
            start_vid = toVid;
            end_vid = fromVid;
            reversed = true;
        }
        VarLink[] path_start_2_varid;
        if (start_vid == varId) {
            path_start_2_varid = new VarLink[0];
        } else {
            path_start_2_varid = shortestPath(start_vid, varId);
        }
        if (null == path_start_2_varid) {
            return null;
        }

        /* Find the path from new arg to end_vid */
        VarLink[] path_new_arg_2_end_vid;
        if (end_vid == varId) {
            path_new_arg_2_end_vid = new VarLink[0];
        } else {
            path_new_arg_2_end_vid = assumeShortestPathHandler(predIdx, argIdx, varId, end_vid);
        }
        if (null == path_new_arg_2_end_vid) {
            return null;
        }
        VarLink[] path = new VarLink[path_start_2_varid.length + path_new_arg_2_end_vid.length];
        if (reversed) {
            for (int i = 0; i < path_new_arg_2_end_vid.length; i++) {
                VarLink original_link = path_new_arg_2_end_vid[path_new_arg_2_end_vid.length - 1 - i];
                path[i] = new VarLink(
                        original_link.predIdx, original_link.toVid, original_link.toArgIdx, original_link.fromVid, original_link.fromArgIdx
                );
            }
            for (int i = 0; i < path_start_2_varid.length; i++) {
                VarLink original_link = path_start_2_varid[path_start_2_varid.length - 1 - i];
                path[path_new_arg_2_end_vid.length + i] = new VarLink(
                        original_link.predIdx, original_link.toVid, original_link.toArgIdx, original_link.fromVid, original_link.fromArgIdx
                );
            }
        } else {
            System.arraycopy(path_start_2_varid, 0, path, 0, path_start_2_varid.length);
            System.arraycopy(path_new_arg_2_end_vid, 0, path, path_start_2_varid.length, path_new_arg_2_end_vid.length);
        }
        return path;
    }

    /**
     * Find the shortest path between two LVs if 2 certain UVs are turned to an existing LV via case-3 specialization.
     *
     * @param predIdx1 NOTE: this parameter should not be 0 (Rule.HEAD_PRED_IDX) here
     * @param predIdx2 NOTE: this parameter should not be 0 (Rule.HEAD_PRED_IDX) here
     * @return NULL if no such path
     */
    public VarLink[] assumeShortestPathCase3(int predIdx1, int argIdx1, int predIdx2, int argIdx2, int fromVid, int toVid) {
        int _from_pred_idx = predIdx1;
        int _from_arg_idx = argIdx1;
        int _to_pred_idx = predIdx2;
        int _to_arg_idx = argIdx2;
        {
            int[] pred_args = rule.get(predIdx1).args;
            for (int argument: pred_args) {
                if (Argument.isVariable(argument)) {
                    if (Objects.equals(varLabels.get(Argument.decode(argument)), varLabels.get(toVid))) {
                        _from_pred_idx = predIdx2;
                        _from_arg_idx = argIdx2;
                        _to_pred_idx = predIdx1;
                        _to_arg_idx = argIdx1;
                    }
                    break;
                }
            }
        }

        VarLink[] reversed_from_2_from_path = assumeShortestPathHandler(_from_pred_idx, _from_arg_idx, -1, fromVid);
        if (null == reversed_from_2_from_path) {
            return null;
        }
        VarLink[] to_2_to_path = assumeShortestPathHandler(_to_pred_idx, _to_arg_idx, -1, toVid);
        if (null == to_2_to_path) {
            return null;
        }
        VarLink[] path = new VarLink[reversed_from_2_from_path.length + to_2_to_path.length];
        for (int i = 0; i < reversed_from_2_from_path.length; i++) {
            VarLink original_link = reversed_from_2_from_path[reversed_from_2_from_path.length - 1 - i];
            path[i] = new VarLink(
                    original_link.predIdx, original_link.toVid, original_link.toArgIdx, original_link.fromVid, original_link.fromArgIdx
            );
        }
        System.arraycopy(to_2_to_path, 0, path, reversed_from_2_from_path.length, to_2_to_path.length);
        return path;
    }

    /**
     * Find the shortest path between two LVs if 2 certain UVs are turned to an existing LV via case-3 specialization.
     *
     * NOTE: this method assumes another UV is in the head, and the end of the path is the newly converted LV. Therefore,
     * there is no parameter specifying the target LV.
     *
     * @param predIdx NOTE: this parameter should not be 0 (Rule.HEAD_PRED_IDX) here
     * @return NULL if no such path
     */
    public VarLink[] assumeShortestPathCase3(int predIdx, int argIdx, int fromVid) {
        VarLink[] reversed_path = assumeShortestPathHandler(predIdx, argIdx, -1, fromVid);
        VarLink[] path = new VarLink[reversed_path.length];
        for (int i = 0; i < reversed_path.length; i++) {
            VarLink original_link = reversed_path[reversed_path.length - 1 - i];
            path[i] = new VarLink(
                    original_link.predIdx, original_link.toVid, original_link.toArgIdx, original_link.fromVid, original_link.fromArgIdx
            );
        }
        return path;
    }
}
