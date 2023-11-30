#include "app.h"
#include "sincWithCache.h"
#include "sincWithEstimation.h"
#include <gflags/gflags.h>  // Todo: gflags may not release all resource it used. Replace by parsing it manually

/**
 * Main
 */
using sinc::Main;
using sinc::SincConfig;

static bool validatePositiveInt(const char* flagName, int32_t value) {
    if (0 < value) {
        return true;
    }
    std::cout << "Invalid value for -" << flagName << ": " << value << " (should be > 0)\n";
    return false;
}

static bool validateNonNegativeDouble(const char* flagName, double value) {
    if (0.0 <= value) {
        return true;
    }
    std::cout << "Invalid value for -" << flagName << ": " << value << " (should be >= 0)\n";
    return false;
}

static bool validateNormalizedDouble(const char* flagName, double value) {
    if (0.0 <= value && 1.0 >= value) {
        return true;
    }
    std::cout << "Invalid value for -" << flagName << ": " << value << " (should be >= 0 and <= 1)\n";
    return false;
}

static bool validateEvalMetric(const char* flagName, std::string const& value) {
    if (0 == value.compare("τ") || 0 == value.compare("δ") || 0 == value.compare("h")) {
        return true;
    }
    std::cout << "Invalid value for -" << flagName << ": " << value << " (should be 'τ', 'δ', or 'h')\n";
    return false;
}

static bool validateInputPath(const char* flagName, std::string const& value) {
    int idx = value.find(',');
    if (std::string::npos != idx && 0 != idx && value.length() - 1 != idx) {
        return true;
    }
    std::cout << "Invalid value for -" << flagName << ": " << value << " (should be separated by ',' into two parts)\n";
    return false;
}

static bool validateOutputPath(const char* flagName, std::string const& value) {
    if (value.empty()) {
        return true;
    }
    int idx = value.find(',');
    if (std::string::npos != idx && 0 != idx) {
        return true;
    }
    std::cout << "Invalid value for -" << flagName << ": " << value << " (should be empty or be separated by ',' into two parts, the second could be empty)\n";
    return false;
}

static bool validateNegKbPath(const char* flagName, std::string const& value) {
    if (value.empty()) {
        return true;
    }
    int idx = value.find(',');
    if (std::string::npos != idx && 0 != idx) {
        return true;
    }
    std::cout << "Invalid value for -" << flagName << ": " << value << " (should be empty or be separated by ',' into two parts, the second could be empty)\n";
    return false;
}

DEFINE_string(I, ".,.", "The path to the input KB and the name of the KB (separated by ',')");
DEFINE_string(O, "", "The path to where the output/compressed KB is stored and the name of the output KB (separated by ','). If not specified, '.' will be used and a default name will be assigned.");
DEFINE_string(N, "", "The path to the negative KB and the name of the KB (separated by ','). If the path is specified and non-empty, negative sampling is turned on. If the negative KB name is non-empty, the negative samples are used; Otherwise, the 'adversarial' sampling model is adopted.");
DEFINE_double(g, 2.0, "The budget factor of negative sampling. (default 2.0)");
DEFINE_bool(w, false, "Whether negative samples have different weight. This is only affective when negative sampling is turned on. (default true)");
DEFINE_int32(t, 1, "The number of threads (default 1)");
DEFINE_bool(v, false, "Validate result after compression (default false)");
DEFINE_int32(r, 0, "The first `r` relations as targets if r > 0");
DEFINE_int32(b, 5, "Beam search width (Default 5)");
DEFINE_string(e, "τ", "Select in the evaluation metrics (default τ). Available options are: τ (Compression Rate), δ (Compression Capacity), h (Information Gain)");
DEFINE_double(f, 0.05, "Set fact coverage threshold (Default 0.05)");
DEFINE_double(c, 0.25, "Set fact constant threshold (Default 0.25)");
DEFINE_double(p, 1.0, "Set stopping compression rate (Default 1.0)");
DEFINE_double(o, 0, "Use rule mining estimation and set observation ratio (Default 0.0). If the value is set >= 1.0, estimation is turned on and the rule mining estimation model is applied.");

DEFINE_validator(I, &validateInputPath);
DEFINE_validator(O, &validateOutputPath);
DEFINE_validator(N, &validateNegKbPath);
DEFINE_validator(g, &validateNonNegativeDouble);
DEFINE_validator(t, &validatePositiveInt);
DEFINE_validator(b, &validatePositiveInt);
DEFINE_validator(e, &validateEvalMetric);
DEFINE_validator(f, &validateNormalizedDouble);
DEFINE_validator(c, &validateNormalizedDouble);
DEFINE_validator(p, &validateNormalizedDouble);
DEFINE_validator(o, &validateNonNegativeDouble);

SincConfig* Main::parseConfig(int argc, char** argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);

    /* Input path & name */
    int idx = FLAGS_I.find(',');
    std::string input_path = FLAGS_I.substr(0, idx);
    std::string input_name = FLAGS_I.substr(idx + 1);
    std::cout << "Input set to: " << input_path << ',' << input_name << std::endl;

    /* Output path & name */
    std::string output_path;
    std::string output_name;
    if (!FLAGS_O.empty()) {
        idx = FLAGS_O.find(',');
        output_path = FLAGS_O.substr(0, idx);
        output_name = (FLAGS_O.length() - 1 == idx) ? (input_name + "_comp") : FLAGS_O.substr(idx + 1);
        std::cout << "Output set to: " << output_path << ',' << output_name << std::endl;
    } else {
        output_path = ".";
        output_name = input_name + "_comp";
    }

    /* Negative sampling parameters */
    std::string negkb_path;
    std::string negkb_name;
    if (!FLAGS_N.empty()) {
        idx = FLAGS_N.find(',');
        negkb_path = FLAGS_N.substr(0, idx);
        negkb_name = FLAGS_N.substr(idx + 1);
        std::cout << "Negative KB set to: " << negkb_path << ',' << negkb_name << std::endl;

        if (2.0 != FLAGS_g) {
            std::cout << "\tBudget factor set to: " << FLAGS_g << std::endl;
        }

        std::cout << "\tNegative Weight: " << (FLAGS_w ? "ON" : "OFF") << std::endl;
        std::cout << "\tAdversarial Model: " << (negkb_name.empty() ? "ON" : "OFF") << std::endl;
    }

    /* Run-time parameters */
    if (1 != FLAGS_t) {
        std::cout << "Threads: " << FLAGS_t << std::endl;
    }
    if (FLAGS_v) {
        std::cout << "Validation: " << (FLAGS_v ? "yes" : "no") << std::endl;
    }
    if (FLAGS_r) {
        std::cout << "Max Number of Relations: " << FLAGS_r << std::endl;
    }
    if (5 != FLAGS_b) {
        std::cout << "Beamwidth: " << FLAGS_b << std::endl;
    }
    if (0 != FLAGS_e.compare("τ")) {
        std::cout << "Evaluation Metric: " << FLAGS_e << std::endl;
    }
    if (0.05 != FLAGS_f) {
        std::cout << "Fact coverage: " << FLAGS_f << std::endl;
    }
    if (0.25 != FLAGS_c) {
        std::cout << "Constant coverage: " << FLAGS_c << std::endl;
    }
    if (1.0 != FLAGS_p) {
        std::cout << "Stopping compression: " << FLAGS_p << std::endl;
    }
    if (0 != FLAGS_o) {
        std::cout << "Observation ratio: " << FLAGS_o << std::endl;
    }

    return new sinc::SincConfig(
        input_path.c_str(), input_name.c_str(), output_path.c_str(), output_name.c_str(), FLAGS_t, FLAGS_v, FLAGS_r, FLAGS_b,
        EvalMetric::getBySymbol(FLAGS_e), FLAGS_f, FLAGS_c, FLAGS_p, FLAGS_o, negkb_path.c_str(), negkb_name.c_str(), FLAGS_g, FLAGS_w
    );
}

void Main::sincMain(int argc, char** argv) {
    SincConfig* config = parseConfig(argc, argv);
    SInC* sinc = nullptr;
    if (1.0 > FLAGS_o) {
        sinc = new SincWithCache(config);
    } else {
        sinc = new SincWithEstimation(config);
    }
    sinc->run();
    delete sinc;
}
