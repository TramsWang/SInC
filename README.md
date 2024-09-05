# SInC: **S**emantic **In**ductive **C**ompressor
SInC is a **S**emantic **In**ductive **C**ompressor on relational data (knowledge bases, KBs).
It splits DBs into two parts where one can be semantically inferred by the other, thus the inferable part is reduced for compression.
The compression is realized by iteratively mining for first-order Horn rules until there are no more records that can be effectively covered by any other pattern.
Followings are related research papers of this project:
- R. Wang, D. Sun, and R. K. Wong, “Symbolic minimization on relational data,” *IEEE Transactions on Knowledge and Data Engineering*, vol. 35, no. 9, pp. 9307-9318, 2023.
- R. Wang, R. Wong, and D. Sun, “Estimation-based optimizations for the semantic compression of RDF knowledge bases,” *Information Processsing and Management* vol. 61, no. 5, p. 103799, 2024.
- R. Wang, D. Sun, and R. K. Wong, “RDF knowledge base summarization by inducing first-order horn rules,” in *Machine Learning and Knowledge Discovery in Databases - European Conference, ECML PKDD,Proceedings, Part II* ser. Lecture Notes in Computer Science, vol. 13714. Springer, 2022, pp. 188-204.
- R. Wang, D. Sun, R. K. Wong, R. Ranjan, and A. Y. Zomaya, “Sinc: Semantic approach and enhancement for relational data compression,” *Knowledge-Based Systems* vol. 258, p. 110001, 2022.
- R. Wang, D. Sun, R. K. Wong, and R. Ranjan, “Horn rule discovery with batched caching and rule identifier for proficient compressor of knowledge data,” *Software: Practice and Experience* vol. 53, no. 3, pp. 682-703, 2023.

## 0. NOTE
The Java version of SInC is outdated and will no longer be updated.
The latest version of SInC is in the dir `c++`, written in C++.
Please refer to `/c++/README.md` for specifications of this project.

## 1. Prerequisites
SInC is implemented in Java and require version 11+.
All of its dependencies are publicly accessible via Maven.

### 1.1 Input Data Format
Input knowledge bases of *SInC* follow a numerated relational format (See `sinc2.kb.NumeratedKb`)`.
In this format, name strings are converted to integer numbers to reduce memory cost and improve processing efficiency.
A KB can be dumped into the local file system.
A dumped KB is a directory (named by the KB name) that contains multiple files:
- A numeration mapping file: Please refer to class `sinc2.kb.NumerationMap`
- Multiple relation files: Please refer to class `sinc2.kb.KbRelation`
- Meta information files:
  - There may be multiple files with extension `.meta` to store arbitrary meta information of the KB.
    The files are customized by other utilities and are not in a fixed format.

The `NumeratedKb` class can also be used to build a KB from scratch.
The following examples show the usage of the KB:

1. Construction, Query & Dump
```java
class ConstructKb {
    public static void main(String[] args) {
        /* Construction */
        NumeratedKb kb = new NumeratedKb("test");
        kb.addRecord("family", new String[]{"alice", "bob", "catherine"});
        kb.addRecord(1, new String[]{"diana", "erick", "frederick"});
        kb.mapName("gabby");// 8
        kb.mapName("harry");// 9
        kb.mapName("isaac");// 10
        kb.mapName("jena");// 11
        kb.mapName("kyle");// 12
        kb.mapName("lily");// 13
        kb.addRecord("family", new Record(new int[]{8, 9, 10}));
        kb.addRecord(1, new Record(new int[]{11, 12, 13}));
        
        /* Query */
        kb.hasRecord("family", new String[]{"alice", "bob", "catherine"});
        kb.hasRecord(1, new String[]{"diana", "erick", "frederick"});
        kb.hasRecord("family", new Record(new int[]{8, 9, 10}));
        kb.hasRecord(1, new Record(new int[]{11, 12, 13}));
        
        /* Dump */
        kb.dump("./some/directory");
    }
}
```
2. Load & Modify
```java
class LoadKb {
    public static void main(String[] args) {
        NumeratedKb kb = new NumeratedKb("name", "./some/directory");
        kb.remove("family", new String[]{"alice", "bob", "catherine"});
        kb.addRecord(1, new Record(new int[]{11, 12, 13}));
    }
}
```

### 1.2 Output Data Format
The output format is similar to the input.
Besides the mapping files and the relation files, the output KB also contains three more components:
- Several counterexample relations;
- A hypothesis set;
- A supplementary constant set.
For detailed instructions, please refer to class `sinc2.kb.CompressedKb`

### 1.3 Logs
The output contents are redirected to a `.log` file in the compressed KB directory.
You may change the value of member `LEVEL` in `sinc2.common.DebugLevel` and recompile for more verbose output.

### 1.4 KB Instance with Less Memory Cost
The `sinc2.kb.NumeratedKb` consumes a lot of memory space to provide a flexible and efficient operation of a KB instance.
The total memory space are dozens more of the disk space taken by all relation files in a local file system.
In order to reduce the memory space, you can use the static unmodifiable KB class `sinc2.kb.SimpleKb`.
Instances of this class will take about only 4 times the disk space of all relations files.

## 2. Use SInC Implementation
The basic class for compression is the abstract class `sinc2.SInC`.
To compress a KB, simply create a SInC implementation object and invoke the `run()` method on it.
For exmaple:

```java
import sinc2.SInC;
import sinc2.SincConfig;
import sinc2.impl.base.SincBasic;
import sinc2.kb.CompressedKb;
import sinc2.rule.EvalMetric;
import sinc2.rule.Rule;

import java.util.List;

class RunSinc {
    public static void main(String[] args) {
        NumeratedKb kb = new NumeratedKb("KbName", "./some/directory");
        SincConfig config = new SincConfig(
                "./KB/parent/dir",
                "KbName",
                "./output/dir",
                "OutputKbName",
                1, // Threads (multithreading has not yet been implemented
                false,
                3,
                EvalMetric.CompressionRatio,
                0.05,
                0.25,
                0.9
        );
        SInC sinc = new SincBasic(config);
        sinc.run();
        CompressedKb compressed_kb = sinc.getCompressedKb();
        List<Rule> hypothesis = compressed_kb.getHypothesis();
    }
}
```

The basic implementation of *SInC* version 2.x is `sinc2.impl.base.SincBasic`.

## 3. Use SInC Jar

The class `sinc2.Main` provides a `main()` method that encloses all features.
To use this entry, you can package the whole project with dependencies and run with: `java -jar sinc.jar [Options]`.
The following displays the usage:

```
usage: java -jar sinc.jar [-b <b>] [-C <name>] [-c <cc>] [-e <name>] [-f
       <fc>] [-h] [-I <path>] [-K <name>] [-O <path>] [-p <scr>] [-t
       <#threads>] [-v]
 -b,--beam-width <b>         Beam search width (Default 3)
 -C,--comp-kb-name <name>    The name of the output/compressed KB
 -c,--const-coverage <cc>    Set constant coverage threshold (Default
                             0.250000)
 -e,--eval-metric <name>     Select in the evaluation metrics (Default δ).
                             Available options are: τ(Compression Rate),
                             δ(Compression Capacity), h(Information Gain)
 -f,--fact-coverage <fc>     Set fact coverage threshold (Default
                             0.050000)
 -h,--help                   Display this help
 -I,--input-path <path>      The path to the input KB
 -K,--kb-name <name>         The name of the input KB
 -O,--output-path <path>     The path to where the output/compressed KB is
                             stored
 -p,--stop-comp-rate <scr>   Set stopping compression rate (Default
                             0.900000)
 -t,--thread <#threads>      The number of threads
 -v,--validate               Validate result after compression
 ```
