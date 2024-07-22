#include "testKbUtils.h"
#include <cstdlib>
#include <string>
#include <string.h>
#include <fstream>
#include "../../src/kb/simpleKb.h"
#include "../../src/util/util.h"

using sinc::test::TestKbManager;
const fs::path TestKbManager::MEM_DIR_PATH = fs::path("/dev/shm");

TestKbManager::TestKbManager() : kbName(), kbPath() {
    srand((unsigned)time(NULL));
    int id[4] {rand(), rand(), rand(), rand()};
    kbName = strdup((std::to_string(id[0]) + std::to_string(id[1]) + std::to_string(id[2]) + std::to_string(id[3])).c_str());
    kbPath = MEM_DIR_PATH / fs::path(kbName);
    createTestKb();
}

TestKbManager::~TestKbManager() {
    free((void*)kbName);    // as `strdup()` uses `malloc()`
}

const fs::path& TestKbManager::createTmpDir() {
    int id[4] {rand(), rand(), rand(), rand()};
    fs::path dir_name(std::to_string(id[0]) + std::to_string(id[1]) + std::to_string(id[2]) + std::to_string(id[3]));
    return tmpPaths.emplace_back(MEM_DIR_PATH / dir_name);
}

void TestKbManager::cleanUpKb() {
    removeDir(kbPath);
    for (fs::path const& p: tmpPaths) {
        removeDir(p);
    }
}

const char* TestKbManager::getKbName() const {
    return kbName;
}

const fs::path& TestKbManager::getKbPath() const {
    return kbPath;
}

void TestKbManager::createTestKb() {
    ASSERT_FALSE(!fs::exists(kbPath) && !fs::create_directories(kbPath));
    createTestMapFiles();
    createTestRelationFiles();
}

void TestKbManager::createTestMapFiles() {
    std::ofstream ofs(SimpleKb::getMapFilePath(kbPath, 1), std::ios::out);
    ofs << "\n\n\n";
    ofs.close();

    ofs.open(SimpleKb::getMapFilePath(kbPath, 2), std::ios::out);
    ofs << "alice\n";
    ofs << "bob\n";
    ofs << "catherine\n";
    ofs << "diana\n";
    ofs << "erick\n";
    ofs << "frederick\n";
    ofs.close();

    ofs.open(SimpleKb::getMapFilePath(kbPath, 3), std::ios::out);
    ofs << "gabby\n";
    ofs << "harry\n";
    ofs << "isaac\n";
    ofs << "jena\n";
    ofs << "kyle\n";
    ofs << "lily\n";
    ofs.close();

    ofs.open(SimpleKb::getMapFilePath(kbPath, 4), std::ios::out);
    ofs << "marvin\n";
    ofs << "nataly\n";
    ofs.close();
}

void TestKbManager::createTestRelationFiles() {
    std::ofstream ofs(SimpleKb::getRelInfoFilePath(kbName, path(MEM_DIR_PATH)), std::ios::out);
    ofs << "family\t3\t4\n";
    ofs << "mother\t2\t4\n";
    ofs << "father\t2\t4\n";
    ofs.close();

    fs::path rel_path1 = kbPath / path("0.rel");
    IntWriter writer(rel_path1.c_str());
    writer.write(4);
    writer.write(5);
    writer.write(6);
    writer.write(7);
    writer.write(8);
    writer.write(9);
    writer.write(10);
    writer.write(11);
    writer.write(12);
    writer.write(13);
    writer.write(14);
    writer.write(15);
    writer.close();

    fs::path rel_path2 = kbPath / path("1.rel");
    writer = IntWriter(rel_path2.c_str());
    writer.write(4);
    writer.write(6);
    writer.write(7);
    writer.write(9);
    writer.write(10);
    writer.write(12);
    writer.write(13);
    writer.write(15);
    writer.close();

    fs::path rel_path3 = kbPath / path("2.rel");
    writer = IntWriter(rel_path3.c_str());
    writer.write(5);
    writer.write(6);
    writer.write(8);
    writer.write(9);
    writer.write(11);
    writer.write(12);
    writer.write(16);
    writer.write(17);
    writer.close();
}

void TestKbManager::removeDir(const fs::path& dir) {
    ASSERT_NE(fs::remove_all(dir), 0) << dir;
}
