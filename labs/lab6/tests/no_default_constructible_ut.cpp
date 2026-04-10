#include <circular_buffer.h>

#include <gtest/gtest.h>

struct no_default_constructible {
    no_default_constructible() = delete;

    no_default_constructible(int) {}
};

/*
    Тест проверяет, что ваш контейнер умеет работать с типами, у которых нет конструктора по-умолчанию
*/
TEST(no_default_constructible, canConstruct) {
    circular_buffer<no_default_constructible> buffer(1);
    buffer.push_back(no_default_constructible(1));
    ASSERT_EQ(buffer.size(), 1);
}
