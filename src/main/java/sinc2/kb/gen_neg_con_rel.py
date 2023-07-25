import struct
import numpy as np
import bisect
import os
import time
import psutil

# This file is used for generating negative KB by constant relevance.
# However, the following code only generate part of KB, with no weight information included.
# To complete the weight for negative samples, use the "completeWeight()" method in Java class "NegSampler".
# Relative Java code is in the following package: "sinc2.kb"
# To use this script, copy and paste it to your working directory other than the locating Java package.

DUMP_DIR = "negConstRelevance"
MAX_HOPS = 3
MAX_TOP_K = 5
MAX_ROUNDS = 5
MAX_PERCENTAGE = 50


INT_SIZE = struct.calcsize('i')

def loadRawRelation(fname, arity, totalRecords):
    '''
    Return: 
        int[][]     A list of records loaded from binary relation file
    '''
    format_str = "<" + "i" * arity
    buffer_size = INT_SIZE * arity
    records = [None] * totalRecords
    relation_file = open(fname, "rb")
    for i in range(totalRecords):
        records[i] = struct.unpack(format_str, relation_file.read(buffer_size))
    relation_file.close()
    return records

def loadPosKb(kbDir):
    meta_file = open("%s/Relations.tsv" % kbDir, 'r')
    relations = []
    for line in meta_file.readlines():
        rel_name, arity_str, total_records_str = line.split('\t')
        arity = int(arity_str)
        total_records = int(total_records_str)
        relation = loadRawRelation("%s/%d.rel" % (kbDir, len(relations)), arity, total_records)
        relations.append(relation)
        relation.sort()
    meta_file.close()
    return relations

def loadRelevanceLists(fname):
    with open(fname, "rb") as rel_file:
        total_constants = struct.unpack("<i", rel_file.read(INT_SIZE))[0]
        rel_lists = [None] * total_constants
        for const_idx in range(total_constants):
            total_tuples = struct.unpack("<i", rel_file.read(INT_SIZE))[0]
            rel_tuples = [None] * total_tuples
            rel_lists[const_idx] = rel_tuples
            for i in range(total_tuples):
                rel_tuples[i] = (struct.unpack("<i", rel_file.read(INT_SIZE))[0], struct.unpack("<f", rel_file.read(INT_SIZE))[0])
    return rel_lists

def calcRelevanceMatrices(relevanceLists):
    TOTAL_CONSTANTS = len(relevanceLists)

    # Calculate relevance matrix
    direct_relevance_matrix = np.zeros((TOTAL_CONSTANTS, TOTAL_CONSTANTS))
    for const_idx in range(TOTAL_CONSTANTS):
        for rel_const, relevance in relevanceLists[const_idx]:
            direct_relevance_matrix[const_idx][rel_const - 1] = relevance
    rand_walk_matrices = [None] * (MAX_HOPS + 1)
    rand_walk_matrices[0] = direct_relevance_matrix / 2
    for hop in range(1, MAX_HOPS + 1):
        print("Matrix Exp: %d" % (hop + 1))
        rand_walk_matrices[hop] = np.matmul(rand_walk_matrices[hop-1], rand_walk_matrices[0])

    relevance_matrices = [None] * (MAX_HOPS + 1)
    relevance_matrices[0] = direct_relevance_matrix
    for hop in range(1, MAX_HOPS + 1):
        print("Relevance Matrix: %d" % (hop + 1))
        rel_matrix = rand_walk_matrices[0].copy()
        for i in range(1, hop):
            rel_matrix += rand_walk_matrices[i]
        rel_matrix += rand_walk_matrices[hop] * 2
        relevance_matrices[hop] = rel_matrix
        # relevance_matrices[hop] = relevance_matrices[hop - 1] / 2 + np.matmul(relevance_matrices[hop - 1], direct_relevance_matrix) / 2
    return relevance_matrices

def calcSortIndexMatrices(relevanceMatrices):
    # Calculate rank matrix (rank starts from 1)
    sort_index_matrices = []
    for hop in range(MAX_HOPS + 1):
        sort_index_matrices.append(np.argsort(relevanceMatrices[hop], 1))
    return sort_index_matrices

def calcRankedRelevanceLists(relevanceLists):
    '''
    Return:
        int[hop][const idx][const]
    '''
    total_constants = len(relevanceLists)
    relevance_matrices = calcRelevanceMatrices(relevanceLists)
    sort_index_matrices = calcSortIndexMatrices(relevance_matrices)
    ranked_relevance_lists = []
    for hop in range(MAX_HOPS + 1):
        print("Rank Matrix: %d" % (hop + 1))
        relevance_matrix = relevance_matrices[hop]
        sort_index_matrix = sort_index_matrices[hop]
        ranked_relevance_list = [None] * total_constants
        ranked_relevance_lists.append(ranked_relevance_list)
        for const_idx in range(total_constants):
            ranked_relevant_consts = []
            ranked_relevance_list[const_idx] = ranked_relevant_consts
            for rank in range(1, total_constants + 1):
                rel_const_idx = sort_index_matrix[const_idx][total_constants - rank]
                if 0 == relevance_matrix[const_idx][rel_const_idx]:
                    break
                ranked_relevant_consts.append(rel_const_idx + 1)
    return ranked_relevance_lists

def isNegativeRecord(negRecord, posRelation):
    insert_idx = bisect.bisect_left(posRelation, negRecord)
    return insert_idx >= len(posRelation) or negRecord != posRelation[insert_idx]

def genNegKb(posRelations, sortIndexMatrix, relevanceMatrix, topK):
    total_relations = len(posRelations)
    total_constants = sortIndexMatrix.shape[0]
    neg_relations = [None] * total_relations

    # Add top K relevant samples
    for rel_idx in range(total_relations):
        pos_relation = posRelations[rel_idx]
        neg_relation = set()
        neg_relations[rel_idx] = neg_relation
        for pos_rec in pos_relation:
            k = 1
            rank = 1
            while k <= topK and rank < total_constants:
                rel_const = sortIndexMatrix[pos_rec[0] - 1][total_constants - rank] + 1
                rank += 1
                neg_rec = list(pos_rec)
                neg_rec[1] = rel_const
                neg_rec = tuple(neg_rec)
                if isNegativeRecord(neg_rec, pos_relation) and (neg_rec not in neg_relation):
                    neg_relation.add(neg_rec)
                    k += 1
            k = 1
            rank = 1
            while k <= topK and rank < total_constants:
                rel_const = sortIndexMatrix[pos_rec[1] - 1][total_constants - rank] + 1
                rank += 1
                neg_rec = list(pos_rec)
                neg_rec[0] = rel_const
                neg_rec = tuple(neg_rec)
                if isNegativeRecord(neg_rec, pos_relation) and (neg_rec not in neg_relation):
                    neg_relation.add(neg_rec)
                    k += 1

    # Add the least relevant sample
    for rel_idx in range(total_relations):
        pos_relation = posRelations[rel_idx]
        neg_relation = neg_relations[rel_idx]
        for pos_rec in pos_relation:
            # # Bottom 1
            # for const_idx in sortIndexMatrix[pos_rec[0] - 1]:
            #     if 0 != relevanceMatrix[pos_rec[0] - 1][const_idx]:
            #         neg_rec = list(pos_rec)
            #         neg_rec[1] = const_idx + 1
            #         neg_rec = tuple(neg_rec)
            #         if isNegativeRecord(neg_rec, pos_relation) and (neg_rec not in neg_relation):
            #             neg_relation.add(neg_rec)
            #             break
            # for const_idx in sortIndexMatrix[pos_rec[1] - 1]:
            #     if 0 != relevanceMatrix[pos_rec[1] - 1][const_idx]:
            #         neg_rec = list(pos_rec)
            #         neg_rec[0] = const_idx + 1
            #         neg_rec = tuple(neg_rec)
            #         if isNegativeRecord(neg_rec, pos_relation) and (neg_rec not in neg_relation):
            #             neg_relation.add(neg_rec)
            #             break

            # Irrelevant 1
            for const_idx in sortIndexMatrix[pos_rec[0] - 1]:
                neg_rec = list(pos_rec)
                neg_rec[1] = const_idx + 1
                neg_rec = tuple(neg_rec)
                if isNegativeRecord(neg_rec, pos_relation) and (neg_rec not in neg_relation):
                    neg_relation.add(neg_rec)
                    break
            for const_idx in sortIndexMatrix[pos_rec[1] - 1]:
                neg_rec = list(pos_rec)
                neg_rec[0] = const_idx + 1
                neg_rec = tuple(neg_rec)
                if isNegativeRecord(neg_rec, pos_relation) and (neg_rec not in neg_relation):
                    neg_relation.add(neg_rec)
                    break

    return neg_relations

def genNegKbByPercent(posRelations, sortIndexMatrix, topP, rounds):
    total_relations = len(posRelations)
    total_constants = sortIndexMatrix.shape[0]
    neg_relations = [None] * total_relations
    modulo = round(total_constants * topP / 100)    # top P%

    # Add top P% relevant constants in-turn
    for rel_idx in range(total_relations):
        pos_relation = posRelations[rel_idx]
        neg_relation = set()
        neg_relations[rel_idx] = neg_relation
        i = 0
        for r in range(rounds):
            for pos_rec in pos_relation:
                for ii in range(total_constants):
                    rank = i % modulo + 1
                    rel_const = sortIndexMatrix[pos_rec[0] - 1][total_constants - rank] + 1
                    neg_rec = list(pos_rec)
                    neg_rec[1] = rel_const
                    neg_rec = tuple(neg_rec)
                    i += 1
                    if isNegativeRecord(neg_rec, pos_relation) and (neg_rec not in neg_relation):
                        neg_relation.add(neg_rec)
                        break
                for ii in range(total_constants):
                    rank = i % modulo + 1
                    rel_const = sortIndexMatrix[pos_rec[1] - 1][total_constants - rank] + 1
                    neg_rec = list(pos_rec)
                    neg_rec[0] = rel_const
                    neg_rec = tuple(neg_rec)
                    i += 1
                    if isNegativeRecord(neg_rec, pos_relation) and (neg_rec not in neg_relation):
                        neg_relation.add(neg_rec)
                        break

    return neg_relations

def genNegKbByAllPercent(posRelations, rankedRelevanceLists, rounds):
    total_relations = len(posRelations)
    total_constants = len(rankedRelevanceLists)
    neg_relations = [None] * total_relations

    # Add top P% relevant constants in-turn
    for rel_idx in range(total_relations):
        print("Rel: %d" % (rel_idx))
        pos_relation = posRelations[rel_idx]
        neg_relation = set()
        neg_relations[rel_idx] = neg_relation
        i = 0
        for r in range(rounds):
            for pos_rec in pos_relation:
                ranked_relevance_list = rankedRelevanceLists[pos_rec[0] - 1]
                for ii in range(total_constants):
                    rel_const = ranked_relevance_list[i % len(ranked_relevance_list)]
                    neg_rec = list(pos_rec)
                    neg_rec[1] = rel_const
                    neg_rec = tuple(neg_rec)
                    i += 1
                    if isNegativeRecord(neg_rec, pos_relation) and (neg_rec not in neg_relation):
                        neg_relation.add(neg_rec)
                        break
                ranked_relevance_list = rankedRelevanceLists[pos_rec[1] - 1]
                for ii in range(total_constants):
                    rel_const = ranked_relevance_list[i % len(ranked_relevance_list)]
                    neg_rec = list(pos_rec)
                    neg_rec[0] = rel_const
                    neg_rec = tuple(neg_rec)
                    i += 1
                    if isNegativeRecord(neg_rec, pos_relation) and (neg_rec not in neg_relation):
                        neg_relation.add(neg_rec)
                        break

    return neg_relations

def dumpNegKb(negRelations, kbName, basePath):
    dump_dir = "%s/%s" % (basePath, kbName)
    os.makedirs(dump_dir, exist_ok=True)
    with open("%s/Relations.dat" % dump_dir, "wb") as meta_file:
        meta_file.write(struct.pack("<i", len(negRelations)))
        for neg_relation in negRelations:
            for rec in neg_relation:    # retrieve the first element in set
                meta_file.write(struct.pack("<i", len(rec)))
                break
            meta_file.write(struct.pack("<i", len(neg_relation)))
    for rel_idx in range(len(negRelations)):
        with open("%s/%d.neg" % (dump_dir, rel_idx), "wb") as rel_file:
            neg_relation = negRelations[rel_idx]
            for neg_rec in neg_relation:
                for arg in neg_rec:
                    rel_file.write(struct.pack("<i", arg))

if __name__ == "__main__":
    # for kb_name in ["Fs", "Fm", "UMLS", "Cs"]:
    # for kb_name in ["Cs", "WN18", "NELL-500"]:
    for kb_name in ["Cs"]:
        print(kb_name)
        kb = loadPosKb("pos/%s" % kb_name)
        relevance_lists = loadRelevanceLists("%s/%s.dat" % (DUMP_DIR, kb_name))
        # relevance_matrices = calcRelevanceMatrices(relevance_lists)
        # sort_index_matrices = calcSortIndexMatrices(relevance_matrices)
        time_start = time.time()
        mem_start = psutil.Process(os.getpid()).memory_info().rss
        ranked_relevance_lists = calcRankedRelevanceLists(relevance_lists)
        time_matrix_calc = time.time() - time_start
        # for hop in range(MAX_HOPS + 1):
        for hop in [3]:
            # for top_k in range(1, MAX_TOP_K + 1):
            #     print("Hop = %d, K = %d" % (hop, top_k))
            #     neg_kb = genNegKb(kb, sort_index_matrices[hop], relevance_matrices[hop], top_k)
            #     # dumpNegKb(neg_kb, "%s_neg_con_rel_h%dk%d" % (kb_name, hop, top_k), DUMP_DIR)
            #     # dumpNegKb(neg_kb, "%s_neg_con_rel_h%dk%d+" % (kb_name, hop, top_k), DUMP_DIR)
            #     dumpNegKb(neg_kb, "%s_neg_con_rel_h%dk%di" % (kb_name, hop, top_k), DUMP_DIR)

            # for top_p in range(10, MAX_PERCENTAGE + 1, 10):
            #     for rounds in range(1, MAX_ROUNDS + 1):
            #         print("Hop = %d, Percentage = %d, Rounds = %d" % (hop, top_p, rounds))
            #         neg_kb = genNegKbByPercent(kb, sort_index_matrices[hop], top_p, rounds)
            #         dumpNegKb(neg_kb, "%s_neg_con_rel_h%dp%dr%d" % (kb_name, hop, top_p, rounds), DUMP_DIR)

            # for rounds in range(1, MAX_ROUNDS + 1):
            for rounds in [5]:
                print("Hop = %d, Rounds = %d" % (hop, rounds))
                time_start = time.time()
                neg_kb = genNegKbByAllPercent(kb, ranked_relevance_lists[hop], rounds)
                time_gen = time.time() - time_start
                mem_cost = psutil.Process(os.getpid()).memory_info().rss - mem_start
                dumpNegKb(neg_kb, "%s_neg_con_rel_h%dr%d" % (kb_name, hop, rounds), DUMP_DIR)
                print("Time Cost: %.5f(s)" % (time_matrix_calc + time_gen))
                print("Memory Cost: %d(B)" % mem_cost)

            # # Dump ranked relevance lists
            # # Format:
            # #   - Total number of constants
            # #   - For each constant of 1, 2, ...:
            # #     - Total number of relevant constants
            # #     - Relevant constant numerations ... (relevance from high to low)
            # ranked_relevance_list = ranked_relevance_lists[hop]
            # with open("%s/%s_rank_list_h%d.dat" % (DUMP_DIR, kb_name, hop), "wb") as of:
            #     of.write(struct.pack("<i", len(ranked_relevance_list)))
            #     for list_of_a_const in ranked_relevance_list:
            #         of.write(struct.pack("<i", len(list_of_a_const)))
            #         for rel_const in list_of_a_const:
            #             of.write(struct.pack("<i", rel_const))