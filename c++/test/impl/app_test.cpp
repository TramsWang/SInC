#include <gtest/gtest.h>
#include "../../src/impl/app.h"
#include <string.h>

using namespace sinc;

TEST(TestMain, TestParseConfig) {
    const char* argv[24] {
        "progname",
        "-I",
        "inputdir,inputname",
        "-O",
        "outputdir,outputname",
        "-N",
        "negdir,negname",
        "-g",
        "1.58",
        "-t",
        "9",
        "-v",
        "-b",
        "2",
        "-e",
        "h",
        "-f",
        "0.33",
        "-c",
        "0.88",
        "-p",
        "0.9",
        "-o",
        "2.2"
    };
    int argc = 24;
    SincConfig* config = Main::parseConfig(argc, (char**)argv);
    EXPECT_STREQ("inputdir", config->basePath.c_str());
    EXPECT_STREQ("inputname", config->kbName);
    EXPECT_STREQ("outputdir", config->dumpPath.c_str());
    EXPECT_STREQ("outputname", config->dumpName);
    EXPECT_STREQ("negdir", config->negKbBasePath.c_str());
    EXPECT_STREQ("negname", config->negKbName);
    EXPECT_EQ(1.58, config->budgetFactor);
    EXPECT_FALSE(config->weightedNegSamples);
    EXPECT_EQ(9, config->threads);
    EXPECT_TRUE(config->validation);
    EXPECT_EQ(2, config->beamwidth);
    EXPECT_EQ(EvalMetric::Value::InfoGain, config->evalMetric);
    EXPECT_EQ(0.33, config->minFactCoverage);
    EXPECT_EQ(0.88, config->minConstantCoverage);
    EXPECT_EQ(0.9, config->stopCompressionRatio);
    EXPECT_EQ(2.2, config->observationRatio);
    delete config;
}
