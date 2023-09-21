#pragma once
#include <filesystem>
#include <vector>
#include <gtest/gtest.h>

namespace fs = std::filesystem;
namespace sinc {
    namespace test{
        class TestKbManager {
        public:
            static const fs::path MEM_DIR_PATH;

            TestKbManager();
            ~TestKbManager();
            // void appendTmpFile(const fs::path& tmpFilePath);
            const fs::path& createTmpDir();
            void cleanUpKb();
            const char* getKbName() const;
            const fs::path& getKbPath() const;
            // const char* getCkbName() const;
            // const fs::path& getCkbPath() const;

        protected:
            const char* kbName;
            fs::path kbPath;
            std::vector<fs::path> tmpPaths;
            // const char* const ckbName;
            // fs::path ckbPath;

            void createTestKb();
            void createTestMapFiles();
            void createTestRelationFiles();
            // void createTestCkb();
            // void createCkbMapFiles();
            // void createCkbNecessaryRelationFiles();
            // void HypothesisFile();
            // void CounterexampleFiles();
            void removeDir(const fs::path& dir);
        };
    }
}