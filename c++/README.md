# SInC: **S**emantic **In**ductive **C**ompressor
SInC is a **S**emantic **In**ductive **C**ompressor on relational data (knowledge bases, KBs).
It splits DBs into two parts where one can be semantically inferred by the other, thus the inferable part is reduced for compression.
The compression is realized by iteratively mining for first-order Horn rules until there are no more records that can be effectively covered by any other pattern.
Followings are related research papers of this project:

<html>
<div>
<style>
ol {
  counter-reset: item;
  margin-left: 1em;
  padding-left: 0;
}
ol > li {
  display: block;
  margin-bottom: .5em;
  margin-left: 2em;
}
ol > li::before {
  display: inline-block;
  content: "["counter(item) "]";
  counter-increment: item;
  width: 2em;
  margin-left: -2em;
}
</style>
<ol>
    <li>R. Wang, D. Sun, and R. K. Wong, “Symbolic minimization on relational data,” <i>IEEE Transactions on Knowledge and Data Engineering</i>, vol. 35, no. 9, pp. 9307-9318, 2023.</li>
    <li>R. Wang, R. Wong, and D. Sun, “Estimation-based optimizations for the semantic compression of RDF knowledge bases,” <i>Information Processsing and Management</i> vol. 61, no. 5, p. 103799, 2024.</li>
    <li>R. Wang, D. Sun, and R. K. Wong, “RDF knowledge base summarization by inducing first-order horn rules,” in <i>Machine Learning and Knowledge Discovery in Databases - European Conference, ECML PKDD,Proceedings, Part II</i> ser. Lecture Notes in Computer Science, vol. 13714. Springer, 2022, pp. 188-204.</li>
    <li>R. Wang, D. Sun, R. K. Wong, R. Ranjan, and A. Y. Zomaya, “Sinc: Semantic approach and enhancement for relational data compression,” <i>Knowledge-Based Systems</i> vol. 258, p. 110001, 2022.</li>
    <li>R. Wang, D. Sun, R. K. Wong, and R. Ranjan, “Horn rule discovery with batched caching and rule identifier for proficient compressor of knowledge data,” <i>Software: Practice and Experience</i> vol. 53, no. 3, pp. 682-703, 2023.</li>
</ol>
</div>
</html>

## 1. Prerequisites
*SInC* is implemented in C++ and require version 17+.
Compilation specifications have been provided in file `CMakeLists.txt`.

### 1.1 Input Data Format
Input knowledge bases of *SInC* follow a numerated relational format (See the classes comments of class `SimpleKb` and `NumeratedKb` in `src/kb/simpleKb.h` and `src/kb/numeratedKb.h`, respectively).
In this format, name strings are mapped to integer numbers to reduce memory cost and improve processing efficiency.
Relations are modeled as tables of integers.

<!-- Class `NumeratedKb` can also be used to build a KB from scratch.
The following examples show the usage of the KB: -->

### 1.2 Output Data Format
The output format is similar to the input, except that the relation files stores tuples that remains in the KB.
The output KB also contains three more components:
- Counterexample relation files;
- A hypothesis file;
- A supplementary constant file.
For detailed instructions, please refer to class `SimpleCompressedKb` in `src/kb/SimpleKb.h`

### 1.3 Logs
The output contents are redirected to a `log.meta` file in the compressed KB directory.
You may change the definition of macro `DEBUG_LEVEL` in `src/util/common.h` and recompile for more verbose output.

## 2. Usage

### 2.1 Compilation and Testing

Use `cmake` to compile with the provided instruction file `CMakeLists.txt`.
The target `sinc` is the SInC program.

**NOTE**: Current implementation relies on fetching `gflags` and `googletest` online.

`test_sinc` is an executable to perform tests.
You can add new tests to `/test`.
If you do this, don't forget to update `CMakeLists.txt`.
All tests comply with the instructions of [GoogleTest](https://google.github.io/googletest/)

### 2.2 Run SInC Executable

Run the executable `sinc` with option `-help` will show you the list of all its options.
The followings are the options provided by SInC implementations:

```
-I (The path to the input KB and the name of the KB (separated by ','))
    type: string default: ".,."
-O (The path to where the output/compressed KB is stored and the name of
    the output KB (separated by ','). If not specified, '.' will be used and
    a default name will be assigned.) type: string default: ""
-b (Beam search width (Default 5)) type: int32 default: 5
-c (Set fact constant threshold (Default 0.25)) type: double default: 0.25
-e (Select in the evaluation metrics (default τ). Available options are:
    τ (Compression Rate), δ (Compression Capacity), h (Information Gain))
    type: string default: "τ"
-f (Set fact coverage threshold (Default 0.05)) type: double
    default: 0.050000000000000003
-p (Set stopping compression rate (Default 1.0)) type: double default: 1
-v (Validate result after compression (default false)) type: bool
    default: false
```

Some options are under development and will be explained in future publications:
```
-o (Use rule mining estimation and set observation ratio (Default 0.0). If
    the value is set >= 1.0, estimation is turned on and the rule mining
    estimation model is applied.) type: double default: 0
-N (The path to the negative KB and the name of the KB (separated by ',').
    If the path is specified and non-empty, negative sampling is turned on.
    If the negative KB name is non-empty, the negative samples are used;
    Otherwise, the 'adversarial' sampling model is adopted.) type: string
    default: ""
-w (Whether negative samples have different weight. This is only affective
    when negative sampling is turned on. (default true)) type: bool
    default: false
-g (The budget factor of negative sampling. (default 2.0)) type: double
    default: 2
-t (The number of threads (default 1)) type: int32 default: 1
```

Following options are used for debugging.
```
-B (Specify a list of relation IDs that should not be set as target
    (separated by ',')) type: string default: ""
-r (The first `r` relations as targets if r > 0) type: int32 default: 0
-M (Set the maximum memory consumption (GByte) during compression (Default
    1024)) type: int32 default: 1024
```

For example, run the following command will compress a KB named `MyKB` in path `path/to/kbs/`, and the compressed KB will be dumped into `/path/to/compressions/`, named by `CompKB`.

```sh
$ ./sinc -I path/to/kbs/,MyKB -O path/to/compressions/,CompKB -b 5 -e τ
```

In the above example, the beamwidth is set to 5 and the rule evaluation metric is τ (compression rate).
Usually, a beamwidth 5 suffices most cases, and τ will lead to better compressions.

If the target KB is large or the runtime memory is limited, you can use compression with estimation (options `-o`, please refer to the explanation above) to generate a quick compression, which will be, however, less effective.
Following shows an example:

```sh
$ ./sinc -I path/to/kbs/,MyKB -O path/to/compressions/,CompKB -b 5 -e τ -o 5.0
```
This functionality is currently under exploration, but should work in most cases.
If the compression is not satisfying, you may try to increase `-b` and `-o` a bit.

### 2.3 Use SInC Implementations in Your Code

A `SInC` object should be created with a `SincConfig` object, which encapsulates compression parameters.
Please refer to `/src/base/sinc.h` for the definition of class `SincConfig`.
After creation, call `run()` of the created object to compress.
Following shows an example:

```c++
SincConfig* config = new sinc::SincConfig(
    "path/to/kbs/", "MyKB", "path/to/compressions/", "CompKB",
    1, false, 0, "", 1024,
    5, EvalMetric::CompressionRatio, 0.05, 0.25, 1.0,
    0,              // parameter reserved for estimation sampling
    "", "", 0, true // parameters reserved for negative sampling
);
SInC* sinc = new SincWithCache(config);
sinc->run();
```

Currently, there are two implementations of `SInC`: `SincWithCache` and `SincWithEstimation`.
- `SincWithCache`: This is the basic implementation that employs batched caching and optimized underlying data structures.
If the input KB is not very large (less than 1M tuples and 100 relations), you can try this implementation.
- `SincWithEstimation`: This is an implementation with estimation pruning, which is still under explored yet. You should use this if the input KB is very large.
