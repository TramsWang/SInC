#include <iostream>
#include "src/util/common.h"
#include <unordered_set>

using namespace sinc;
using std::cout;
using std::endl;

int main(int, char**){
    std::cout << "Hello, from Sinc xx!\n";
    std::unordered_set<Record*> set;
    set.emplace(new Record(3));
    set.emplace(new Record(new int[3]{1, 2, 3}, 3));
    set.emplace(new Record(new int[3]{1, 2, 3}, 3));
    set.emplace(new Record(new int[2]{3, 2}, 2));
    for (auto itr = set.begin(); itr != set.end(); itr++) {
        int *args = (*itr)->getArgs();
        cout << "Record: [" << args[0];
        for (int i = 1; i < (*itr)->getArity(); i++) {
            cout << ',' << args[i];
        }
        cout << ']' << endl;
        cout.flush();
    }
}
