#pragma once

#include "../base/sinc.h"

namespace sinc {
    /**
     * This class provides an entry to the SInC tool
     * 
     * @since 2.0
     */
    class Main {
    public:
        static SincConfig* parseConfig(int argc, char** argv);
        static void sincMain(int argc, char** argv);
    };
}