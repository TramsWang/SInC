#include <iostream>
#include "src/base/sinc.h"
#include "src/util/util.h"
#include "src/rule/rule.h"
#include <unordered_set>

using namespace sinc;
using std::cout;
using std::endl;

int main(int, char**){
    std::cout << "Hello, from Sinc xx!\n";
    // std::unordered_set<Record*> set;
    // set.emplace(new Record(3));
    // set.emplace(new Record(new int[3]{1, 2, 3}, 3));
    // set.emplace(new Record(new int[3]{1, 2, 3}, 3));
    // set.emplace(new Record(new int[2]{3, 2}, 2));
    // for (auto itr = set.begin(); itr != set.end(); itr++) {
    //     int *args = (*itr)->getArgs();
    //     cout << "Record: [" << args[0];
    //     for (int i = 1; i < (*itr)->getArity(); i++) {
    //         cout << ',' << args[i];
    //     }
    //     cout << ']' << endl;
    //     cout.flush();
    // }

    Rule::fingerprintCacheType cache;
    Rule::tabuMapType tabu;
    BareRule rule(-1, 1, cache, tabu);

    // RelationMiner::nodeMapType node_map;
    // RelationMiner::depGraphType dependency_graph;
    // std::vector<Rule*> hypothesis;
    // std::unordered_set<Record> counterexamples;
    // int*** rows = new int**[1] {
    //     new int*[1] {
    //         new int[1] {1}
    //     }
    // };
    // std::string rel_names[1] {"rel"};
    // int arities[1] {1};
    // int total_rows[1] {1};
    // SimpleKb kb("test", rows, rel_names, arities, total_rows, 1);
    // // RelationMiner4Test rel_miner(
    // //     kb, 0, EvalMetric::Value::CompressionCapacity, 1, 1.0,
    // //     node_map, dependency_graph, hypothesis, counterexamples, std::cout
    // // );
}
