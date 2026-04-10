#include <circular_buffer.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string>
#include <vector>

template <typename T>
class circular_buffer_int_test : public testing::Test {};

using circular_buffer_int_types = testing::Types<
    circular_buffer<int, false>
    #ifdef RUN_EXT_TESTS
    , circular_buffer<int, true>
    #endif
>;
TYPED_TEST_SUITE(circular_buffer_int_test, circular_buffer_int_types);

template <typename T>
class circular_buffer_string_test : public testing::Test {};

using circular_buffer_string_types = testing::Types<
    circular_buffer<std::string, false>
    #ifdef RUN_EXT_TESTS
    , circular_buffer<std::string, true>
    #endif
>;
TYPED_TEST_SUITE(circular_buffer_string_test, circular_buffer_string_types);

TYPED_TEST(circular_buffer_int_test, alternatingPush) {
    TypeParam cb(6);
    for (int i = 0; i < 6; ++i) {
        if (i % 2 == 0) {
            cb.push_back(i);
        } else {
            cb.push_front(i);
        }
    }

    ASSERT_THAT(cb, testing::ElementsAre(5, 3, 1, 0, 2, 4));
}

TYPED_TEST(circular_buffer_string_test, pushingComplicatedObjects) {
    TypeParam cb(3);
    cb.push_back("aaa");
    cb.push_back("bbb");
    cb.push_back("ccc");
    ASSERT_THAT(cb, testing::ElementsAre("aaa", "bbb", "ccc"));
}

TYPED_TEST(circular_buffer_int_test, simplePopTest) {
    TypeParam cb(5);

    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);

    cb.push_front(4);
    cb.push_front(5);

    cb.pop_back();
    cb.pop_front();

    ASSERT_THAT(cb, testing::ElementsAre(4, 1, 2));
}

TYPED_TEST(circular_buffer_int_test, popFromEmpty) {
    TypeParam cb(1);
    cb.push_back(1);
    cb.pop_back();

    try {
        cb.pop_back();
        FAIL();
    } catch (...) {}

    try {
        cb.pop_front();
        FAIL();
    } catch (...) {}

    SUCCEED();
}

TYPED_TEST(circular_buffer_int_test, eraseOneElement) {
    TypeParam cb = {1, 2, 3, 4, 5};

    cb.erase(cb.cbegin() + 2);

    ASSERT_THAT(cb, testing::ElementsAre(1, 2, 4, 5));
}

TYPED_TEST(circular_buffer_int_test, eraseSequence) {
    TypeParam cb(5);
    for (int i = 0; i < 5; ++i) {
        if (i % 2 == 0) {
            cb.push_back(i);
        } else {
            cb.push_front(i);
        }
    }
    cb.erase(cb.cbegin() + 1, cb.cend() - 1);

    ASSERT_THAT(cb, testing::ElementsAre(3, 4));
}

TYPED_TEST(circular_buffer_int_test, nValues) {
    TypeParam cb(5);

    cb.assign(5, 10);

    ASSERT_THAT(cb, testing::ElementsAre(10, 10, 10, 10, 10));
}

TYPED_TEST(circular_buffer_int_test, assignIterator) {
    TypeParam cb(5);
    std::vector<int> v = {1, 2, 3, 4, 5};

    cb.assign(v.begin() + 1, v.end() - 1);

    ASSERT_THAT(cb, testing::ElementsAre(2, 3, 4));
}

TYPED_TEST(circular_buffer_int_test, initializerList) {
    TypeParam cb = {5, 4, 3, 2, 1};

    cb.assign({1, 2, 3, 4, 5});

    ASSERT_THAT(cb, testing::ElementsAre(1, 2, 3, 4, 5));
}

TYPED_TEST(circular_buffer_int_test, expand) {
    TypeParam cb = {1, 2, 3, 4, 5};
    cb.resize(7, 6);

    ASSERT_THAT(cb, testing::ElementsAre(1, 2, 3, 4, 5, 6, 6));
}

TYPED_TEST(circular_buffer_int_test, shrink) {
    TypeParam cb = {1, 2, 3, 4, 5};
    cb.resize(3);

    ASSERT_THAT(cb, testing::ElementsAre(1, 2, 3));
}

TYPED_TEST(circular_buffer_int_test, insertValue) {
    TypeParam cb(5); // {3, 1, 0, 2, 4}
    for (int i = 0; i < 5; ++i) {
        if (i % 2 == 0) {
            cb.push_back(i);
        } else {
            cb.push_front(i);
        }
    }
    cb.insert(cb.begin() + 2, 10);

    ASSERT_THAT(cb, testing::ElementsAre(3, 1, 10, 0, 2, 4));
}

TYPED_TEST(circular_buffer_int_test, insertNValues) {
    TypeParam cb(7); // {3, 1, 0, 2, 4}
    for (int i = 0; i < 5; ++i) {
        if (i % 2 == 0) {
            cb.push_back(i);
        } else {
            cb.push_front(i);
        }
    }
    cb.insert(cb.begin() + 2, 2, 8);

    ASSERT_THAT(cb, testing::ElementsAre(3, 1, 8, 8, 0, 2, 4));
}

TYPED_TEST(circular_buffer_int_test, insertIterators) {
    TypeParam cb(8); // {3, 1, 0, 2, 4}
    std::vector<int> v = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; ++i) {
        if (i % 2 == 0) {
            cb.push_back(i);
        } else {
            cb.push_front(i);
        }
    }
    cb.insert(cb.begin() + 2, v.begin() + 1, v.end() - 1);

    ASSERT_THAT(cb, testing::ElementsAre(3, 1, 2, 3, 4, 0, 2, 4));
}

TYPED_TEST(circular_buffer_int_test, insertInitializerList) {
    TypeParam cb(8); // {3, 1, 0, 2, 4}
    for (int i = 0; i < 5; ++i) {
        if (i % 2 == 0) {
            cb.push_back(i);
        } else {
            cb.push_front(i);
        }
    }
    cb.insert(cb.begin() + 2, {2, 3, 4});

    ASSERT_THAT(cb, testing::ElementsAre(3, 1, 2, 3, 4, 0, 2, 4));
}

TYPED_TEST(circular_buffer_int_test, simpleTest) {
    TypeParam cb(5); // {3, 1, 0, 2, 4}
    for (int i = 0; i < 5; ++i) {
        if (i % 2 == 0) {
            cb.push_back(i);
        } else {
            cb.push_front(i);
        }
    }

    ASSERT_EQ(cb.front(), 3);
}
