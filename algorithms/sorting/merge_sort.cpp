#include <iostream>
#include <vector>

void Merge(std::vector<int>& values, int left, int mid, int right) {
    std::vector<int> buffer;
    buffer.reserve(right - left + 1);

    int first = left;
    int second = mid + 1;

    while (first <= mid && second <= right) {
        if (values[first] <= values[second]) {
            buffer.push_back(values[first++]);
        } else {
            buffer.push_back(values[second++]);
        }
    }

    while (first <= mid) {
        buffer.push_back(values[first++]);
    }

    while (second <= right) {
        buffer.push_back(values[second++]);
    }

    for (int i = 0; i < static_cast<int>(buffer.size()); ++i) {
        values[left + i] = buffer[i];
    }
}

void MergeSort(std::vector<int>& values, int left, int right) {
    if (left >= right) {
        return;
    }

    const int mid = left + (right - left) / 2;
    MergeSort(values, left, mid);
    MergeSort(values, mid + 1, right);
    Merge(values, left, mid, right);
}

int main() {
    int size = 0;
    std::cin >> size;

    std::vector<int> values(size);
    for (int i = 0; i < size; ++i) {
        std::cin >> values[i];
    }

    if (!values.empty()) {
        MergeSort(values, 0, static_cast<int>(values.size()) - 1);
    }

    for (int i = 0; i < size; ++i) {
        if (i > 0) {
            std::cout << ' ';
        }
        std::cout << values[i];
    }
    std::cout << '\n';

    return 0;
}
