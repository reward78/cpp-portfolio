#include <circular_buffer.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(ExtendableBufferTest, simpleTest) {
    circular_buffer<int, true> cb(5);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    cb.push_back(4);
    cb.push_back(5);
    cb.push_back(6);
    ASSERT_THAT(cb, testing::ElementsAre(1, 2, 3, 4, 5, 6));
}

TEST(ExtendableBufferTest, insertInTheMiddle) {
    circular_buffer<int, true> cb(5);
    cb.push_back(1);
    cb.push_back(2);
    cb.push_back(3);
    cb.push_back(4);
    cb.push_back(5);

    cb.insert(cb.begin() + 2, {6, 7, 8});
    ASSERT_THAT(cb, testing::ElementsAre(1, 2, 6, 7, 8, 3, 4, 5));
}
