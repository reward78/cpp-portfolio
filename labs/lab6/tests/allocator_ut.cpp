#include <circular_buffer.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <concepts>
#include <type_traits>
class node_tag {};

class some_obj {
public:
    static inline int constructor_called_ = 0;
    static inline int destructor_called_ = 0;

    some_obj() {
        ++constructor_called_;
    }

    ~some_obj() {
        ++destructor_called_;
    }
};


template<class Alloc>
concept AllocatorRequirements = requires(Alloc alloc, std::size_t n)
{
    { *alloc.allocate(n) } -> std::same_as<typename Alloc::value_type&>;
    { alloc.deallocate(alloc.allocate(n), n) };
} && std::copy_constructible<Alloc>
  && std::equality_comparable<Alloc>;


template<typename T>
class test_allocator {
public:
    using value_type = T;
    using pointer = T*;
    using size_type = size_t;
    using is_always_equal = std::true_type;

    static inline int allocation_count_ = 0;
    static inline int elements_allocated_ = 0;

    test_allocator() = default;

    template<typename U>
    test_allocator(const test_allocator<U>& other) {
    }

    pointer allocate(size_type sz) {
        if constexpr (std::is_same_v<T, some_obj>) {
            ++test_allocator<some_obj>::allocation_count_;
            test_allocator<some_obj>::elements_allocated_ += sz;
        } else {
            ++test_allocator<node_tag>::allocation_count_;
            test_allocator<node_tag>::elements_allocated_ += sz;
        }
        return static_cast<pointer>(std::aligned_alloc(alignof(T), sizeof(T) * sz));
    }

    void deallocate(pointer p, std::size_t) {
        std::free(p);
    }

    bool operator==(const test_allocator& other) const {
        return true;
    }

};


static_assert(AllocatorRequirements<test_allocator<some_obj>>);

class work_with_allocator_test : public testing::Test {
public:
    void SetUp() override {
        some_obj::constructor_called_ = 0;
        some_obj::destructor_called_ = 0;

        test_allocator<some_obj>::allocation_count_ = 0;
        test_allocator<some_obj>::elements_allocated_ = 0;

        test_allocator<node_tag>::allocation_count_ = 0;
        test_allocator<node_tag>::elements_allocated_ = 0;
    }

};

/*
    В тесте задаётся Capacity = 5, Extended = false.

    Ожидается, что будет:
        1. 1 аллокация буфера
*/
TEST_F(work_with_allocator_test, reserve) {
    test_allocator<some_obj> allocator;
    circular_buffer<some_obj, false, test_allocator<some_obj>> buffer(5, allocator);

    ASSERT_EQ(test_allocator<some_obj>::allocation_count_, 1);
    ASSERT_EQ(test_allocator<some_obj>::elements_allocated_, 5);

    ASSERT_EQ(some_obj::constructor_called_, 0);
    ASSERT_EQ(some_obj::destructor_called_, 0);
}

/*
    В тесте задаётся Capacity = 5, Extended = false, а далее добавляется 5 элементов.

    Ожидается, что будет:
        1. 1 аллокация буфера
        2. 5 конструкторов и деструкторов у some_obj
*/
TEST_F(work_with_allocator_test, simplePushBack) {
    test_allocator<some_obj> allocator;
    circular_buffer<some_obj, false, test_allocator<some_obj>> buffer(5, allocator);
    for (int i = 0; i < 5; ++i) {
        buffer.push_back(some_obj{});
    }

    ASSERT_EQ(test_allocator<some_obj>::allocation_count_, 1);
    ASSERT_EQ(test_allocator<some_obj>::elements_allocated_, 5);

    ASSERT_EQ(some_obj::constructor_called_, 5);
    ASSERT_EQ(some_obj::destructor_called_, 5);
}

/*
    В тесте задаётся Capacity = 5, Extended = true, а далее добавляется 6 элементов.

    Ожидается, что будет:
        1. 2 аллокации буфера
        2. 6 конструкторов и деструкторов у some_obj
*/
#ifdef RUN_EXT_TESTS
TEST_F(work_with_allocator_test, extendedPushBack) {
    test_allocator<some_obj> allocator;
    circular_buffer<some_obj, true, test_allocator<some_obj>> buffer(5, allocator);
    for (int i = 0; i < 6; ++i) {
        buffer.push_back(some_obj{});
    }

    ASSERT_EQ(test_allocator<some_obj>::allocation_count_, 2);
    ASSERT_EQ(test_allocator<some_obj>::elements_allocated_, 15);

    ASSERT_EQ(some_obj::constructor_called_, 6);
    ASSERT_EQ(some_obj::destructor_called_, 6);
}
#endif