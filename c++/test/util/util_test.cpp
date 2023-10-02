#include <gtest/gtest.h>
#include "../../src/util/util.h"
#include "../../src/util/common.h"
#include <filesystem>

using namespace sinc;

#define MEM_DIR "/dev/shm/"

TEST(TestUtil, TestMultiSetConstructor) {
    MultiSet<int> s1;
    s1.add(1);
    s1.add(2);
    s1.add(1);
    s1.add(3);

    EXPECT_EQ(s1.getSize(), 4);
    EXPECT_EQ(s1.differentValues(), 3);
    EXPECT_EQ(s1.itemCount(1), 2);
    EXPECT_EQ(s1.itemCount(2), 1);
    EXPECT_EQ(s1.itemCount(3), 1);
    EXPECT_EQ(s1.itemCount(0), 0);

    int* elements = new int[3]{1, 3, 2};
    EXPECT_EQ(s1.itemCount(elements, 3), 4);
    delete[] elements;

    elements = new int[4]{0, 3, 2, 1};
    EXPECT_EQ(s1.itemCount(elements, 4), 4);
    delete[] elements;

    MultiSet<int> s2(s1);

    EXPECT_EQ(s2.getSize(), 4);
    EXPECT_EQ(s2.differentValues(), 3);
    EXPECT_EQ(s2.itemCount(1), 2);
    EXPECT_EQ(s2.itemCount(2), 1);
    EXPECT_EQ(s2.itemCount(3), 1);
    EXPECT_EQ(s2.itemCount(0), 0);

    elements = new int[3]{1, 3, 2};
    EXPECT_EQ(s2.itemCount(elements, 3), 4);
    delete[] elements;

    elements = new int[4]{0, 3, 2, 1};
    EXPECT_EQ(s2.itemCount(elements, 4), 4);
    delete[] elements;

    elements = new int[4]{3, 1, 1, 2};
    MultiSet<int> s3(elements, 4);
    delete[] elements;

    EXPECT_EQ(s3.getSize(), 4);
    EXPECT_EQ(s3.differentValues(), 3);
    EXPECT_EQ(s3.itemCount(1), 2);
    EXPECT_EQ(s3.itemCount(2), 1);
    EXPECT_EQ(s3.itemCount(3), 1);
    EXPECT_EQ(s3.itemCount(0), 0);

    elements = new int[3]{1, 3, 2};
    EXPECT_EQ(s3.itemCount(elements, 3), 4);
    delete[] elements;

    elements = new int[4]{0, 3, 2, 1};
    EXPECT_EQ(s3.itemCount(elements, 4), 4);
    delete[] elements;
}

TEST(TestUtil, TestMultiSetAdd) {
    MultiSet<int> mset;
    mset.add(1);
    mset.add(2);
    mset.add(1);
    mset.add(3);

    EXPECT_EQ(mset.getSize(), 4);
    EXPECT_EQ(mset.differentValues(), 3);
    EXPECT_EQ(mset.itemCount(1), 2);
    EXPECT_EQ(mset.itemCount(2), 1);
    EXPECT_EQ(mset.itemCount(3), 1);
    EXPECT_EQ(mset.itemCount(0), 0);

    int* elements = new int[3]{1, 3, 2};
    EXPECT_EQ(mset.itemCount(elements, 3), 4);
    delete[] elements;

    elements = new int[4]{0, 3, 2, 1};
    EXPECT_EQ(mset.itemCount(elements, 4), 4);
    delete[] elements;
}

TEST(TestUtil, TestMultiSetAddAll) {
    int* elements = new int[4]{1, 3, 1, 2};
    MultiSet<int> mset;
    mset.addAll(elements, 4);
    delete[] elements;

    EXPECT_EQ(mset.getSize(), 4);
    EXPECT_EQ(mset.differentValues(), 3);
    EXPECT_EQ(mset.itemCount(1), 2);
    EXPECT_EQ(mset.itemCount(2), 1);
    EXPECT_EQ(mset.itemCount(3), 1);
    EXPECT_EQ(mset.itemCount(0), 0);

    elements = new int[3]{1, 3, 2};
    EXPECT_EQ(mset.itemCount(elements, 3), 4);
    delete[] elements;

    elements = new int[4]{0, 3, 2, 1};
    EXPECT_EQ(mset.itemCount(elements, 4), 4);
    delete[] elements;

    elements = new int[4]{3, 3, 5, 6};
    MultiSet<int> mset2;
    mset2.addAll(elements, 4);
    delete[] elements;
    mset2.addAll(mset);

    EXPECT_EQ(mset2.getSize(), 8);
    EXPECT_EQ(mset2.differentValues(), 5);
    EXPECT_EQ(mset2.itemCount(1), 2);
    EXPECT_EQ(mset2.itemCount(2), 1);
    EXPECT_EQ(mset2.itemCount(3), 3);
    EXPECT_EQ(mset2.itemCount(4), 0);
    EXPECT_EQ(mset2.itemCount(5), 1);
    EXPECT_EQ(mset2.itemCount(6), 1);
    EXPECT_EQ(mset2.itemCount(0), 0);

    elements = new int[8]{0, 1, 2, 3, 4, 5, 6, 7};
    EXPECT_EQ(mset2.itemCount(elements, 8), 8);
    delete[] elements;

    elements = new int[4]{0, 3, 2, 1};
    EXPECT_EQ(mset2.itemCount(elements, 4), 6);
    delete[] elements;
}

TEST(TestUtil, TestMultiSetRemove) {
    MultiSet<int> mset;
    mset.add(1);
    mset.add(2);
    mset.add(1);
    mset.add(3);
    
    EXPECT_EQ(mset.remove(0), 0);
    EXPECT_EQ(mset.getSize(), 4);
    EXPECT_EQ(mset.differentValues(), 3);
    EXPECT_EQ(mset.itemCount(1), 2);
    EXPECT_EQ(mset.itemCount(2), 1);
    EXPECT_EQ(mset.itemCount(3), 1);
    EXPECT_EQ(mset.itemCount(0), 0);

    int* elements = new int[3]{1, 3, 2};
    EXPECT_EQ(mset.itemCount(elements, 3), 4);
    delete[] elements;

    elements = new int[4]{0, 3, 2, 1};
    EXPECT_EQ(mset.itemCount(elements, 4), 4);
    delete[] elements;
    
    EXPECT_EQ(mset.remove(1), 1);
    EXPECT_EQ(mset.getSize(), 3);
    EXPECT_EQ(mset.differentValues(), 3);
    EXPECT_EQ(mset.itemCount(1), 1);
    EXPECT_EQ(mset.itemCount(2), 1);
    EXPECT_EQ(mset.itemCount(3), 1);
    EXPECT_EQ(mset.itemCount(0), 0);

    elements = new int[3]{1, 3, 2};
    EXPECT_EQ(mset.itemCount(elements, 3), 3);
    delete[] elements;

    elements = new int[4]{0, 3, 2, 1};
    EXPECT_EQ(mset.itemCount(elements, 4), 3);
    delete[] elements;
    
    EXPECT_EQ(mset.remove(2), 0);
    EXPECT_EQ(mset.getSize(), 2);
    EXPECT_EQ(mset.differentValues(), 2);
    EXPECT_EQ(mset.itemCount(1), 1);
    EXPECT_EQ(mset.itemCount(2), 0);
    EXPECT_EQ(mset.itemCount(3), 1);
    EXPECT_EQ(mset.itemCount(0), 0);

    elements = new int[3]{1, 3, 2};
    EXPECT_EQ(mset.itemCount(elements, 3), 2);
    delete[] elements;

    elements = new int[4]{0, 3, 2, 1};
    EXPECT_EQ(mset.itemCount(elements, 4), 2);
    delete[] elements;
}

TEST(TestUtil, TestMultiSetSubsetOf) {
    int* elements = new int[4]{1, 3, 1, 2};
    MultiSet<int> s1;
    s1.addAll(elements, 4);
    delete[] elements;

    elements = new int[3]{1, 1, 3};
    MultiSet<int> s2;
    s2.addAll(elements, 3);
    delete[] elements;

    elements = new int[3]{2, 5, 3};
    MultiSet<int> s3;
    s3.addAll(elements, 3);
    delete[] elements;

    elements = new int[4]{1, 1, 1, 3};
    MultiSet<int> s4;
    s4.addAll(elements, 4);
    delete[] elements;

    EXPECT_TRUE(s2.subsetOf(s1));
    EXPECT_TRUE(s2.subsetOf(s4));
    EXPECT_FALSE(s1.subsetOf(s2));
    EXPECT_FALSE(s4.subsetOf(s2));
    EXPECT_FALSE(s3.subsetOf(s1));
    EXPECT_FALSE(s3.subsetOf(s1));
}

TEST(TestUtil, TestMultiSetEquivalence) {
    int* elements = new int[4]{1, 3, 1, 2};
    MultiSet<int> s1;
    s1.addAll(elements, 4);
    delete[] elements;

    elements = new int[4]{1, 1, 3, 2};
    MultiSet<int> s2;
    s2.addAll(elements, 4);
    delete[] elements;

    MultiSet<int> s3;
    s3.add(3);
    s3.add(2);
    s3.add(1);
    s3.add(1);

    elements = new int[4]{1, 1, 1, 3};
    MultiSet<int> s4;
    s4.addAll(elements, 4);
    delete[] elements;

    EXPECT_TRUE(s1 == s2);
    EXPECT_TRUE(s1 == s3);
    EXPECT_TRUE(s2 == s3);
    EXPECT_TRUE(s3 == s2);
    EXPECT_TRUE(s3 == s1);
    EXPECT_TRUE(s2 == s1);
    EXPECT_FALSE(s1 == s4);
    EXPECT_FALSE(s4 == s2);
    EXPECT_FALSE(s4 == s3);

    std::hash<MultiSet<int>> hasher;
    EXPECT_EQ(hasher(s1), hasher(s2));
    EXPECT_EQ(hasher(s1), hasher(s3));
    EXPECT_NE(hasher(s1), hasher(s4));
}

TEST(TestUtil, TestIntIO) {
    using std::filesystem::path;
    path test_file_path = path(MEM_DIR) / path("test_io.dat");
    const char* test_file = test_file_path.c_str();
    IntWriter writer(test_file);
    int i1 = 1;
    int i2 = 0xdeadbeef;
    int i3 = (1 << 31) | 1013;
    writer.write(i1);
    writer.write(i2);
    writer.write(i3);
    writer.close();

    IntReader reader(test_file);
    EXPECT_EQ(reader.next(), i1);
    EXPECT_EQ(reader.next(), i2);
    EXPECT_EQ(reader.next(), i3);
    reader.close();

    std::filesystem::remove(test_file_path);
}

TEST(TestUtil, TestDisjointSet) {
    /* Test 1: {0, 1, 2}, {3, 4} */
    DisjointSet s1(5);
    EXPECT_EQ(s1.totalSets(), 5);

    s1.unionSets(0, 2);
    s1.unionSets(1, 2);
    s1.unionSets(4, 4);
    s1.unionSets(4, 3);
    EXPECT_EQ(s1.totalSets(), 2);
    EXPECT_EQ(s1.findSet(0), s1.findSet(1));
    EXPECT_EQ(s1.findSet(0), s1.findSet(2));
    EXPECT_EQ(s1.findSet(1), s1.findSet(2));
    EXPECT_EQ(s1.findSet(3), s1.findSet(4));
    EXPECT_NE(s1.findSet(0), s1.findSet(3));

    /* Test 2: {0, 1, 2, 3, 4} */
    DisjointSet s2(5);
    EXPECT_EQ(s2.totalSets(), 5);

    s2.unionSets(0, 2);
    s2.unionSets(1, 2);
    s2.unionSets(1, 4);
    s2.unionSets(4, 3);
    EXPECT_EQ(s2.totalSets(), 1);
    EXPECT_EQ(s2.findSet(0), s2.findSet(1));
    EXPECT_EQ(s2.findSet(0), s2.findSet(2));
    EXPECT_EQ(s2.findSet(1), s2.findSet(3));
    EXPECT_EQ(s2.findSet(3), s2.findSet(4));

    /* Test 3: {0, 1}, {2}, {3, 4} */
    DisjointSet s3(5);
    EXPECT_EQ(s3.totalSets(), 5);

    s3.unionSets(0, 1);
    s3.unionSets(1, 1);
    s3.unionSets(4, 4);
    s3.unionSets(4, 3);
    EXPECT_EQ(s3.totalSets(), 3);
    EXPECT_EQ(s3.findSet(0), s3.findSet(1));
    EXPECT_EQ(s3.findSet(3), s3.findSet(4));
    EXPECT_NE(s3.findSet(0), s3.findSet(2));
    EXPECT_NE(s3.findSet(3), s3.findSet(2));
    EXPECT_NE(s3.findSet(1), s3.findSet(3));
}

TEST(TestUtil, TestComparableArray) {
    ComparableArray<Record> a1(new Record[3]{
        Record(new int[3]{1, 2, 3}, 3),
        Record(new int[2]{5, 0}, 2),
        Record(new int[4]{0}, 4)
    }, 3);

    ComparableArray<Record> a2(new Record[3]{
        Record(new int[3]{1, 2, 3}, 3),
        Record(new int[2]{5, 0}, 2),
        Record(new int[4]{0}, 4)
    }, 3);

    ComparableArray<Record> a3(new Record[3]{
        Record(new int[2]{5, 0}, 2),
        Record(new int[3]{1, 2, 3}, 3),
        Record(new int[4]{0}, 4)
    }, 3);

    EXPECT_TRUE(a1 == a2);
    EXPECT_FALSE(a1 == a3);

    std::hash<ComparableArray<Record>> hasher;
    EXPECT_EQ(hasher(a1), hasher(a2));
    EXPECT_NE(hasher(a1), hasher(a3));

    for (int i = 0; i < 3; i++) {
        delete[] a1.arr[i].getArgs();
        delete[] a2.arr[i].getArgs();
        delete[] a3.arr[i].getArgs();
    }
}

TEST(TestPerformanceMonitor, TestFormatting) {
    class Monitor4Test : public PerformanceMonitor {
    public:
        void show(std::ostream& os) override {
            PerformanceMonitor::printf(os, "My name is %10s. I'm %d years old. Pi is %.6f", "Trams", 30, 3.141592653);
        }
    };

    std::ostringstream os;
    Monitor4Test monitor;
    monitor.show(os);
    EXPECT_STREQ(os.str().c_str(), "My name is      Trams. I'm 30 years old. Pi is 3.141593");
}