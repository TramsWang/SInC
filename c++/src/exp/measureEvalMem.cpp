#include "../impl/sincWithCache.h"
#include <sys/resource.h>

using namespace sinc;

#define NUM_RECORDS 1000000

int main(int argc, char const *argv[]) {
    long mem_begin = getMaxRss();
    std::unordered_map<Record, std::unordered_set<Record>*> body_gv_binding_2_plv_bindings;
    for (int i = 0; i < NUM_RECORDS; i++) {
        std::unordered_set<Record>* set = new std::unordered_set<Record>();
        Record key(new int[2], 2);
        set->emplace(new int[2], 2);
        body_gv_binding_2_plv_bindings.emplace(key, set);
    }
    long mem_finished = getMaxRss();
    std::cout << (mem_finished - mem_begin) << std::endl;
    return 0;
}
