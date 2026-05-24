#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace std;

struct Measurement {
    double microseconds = 0.0;
    long long charComparisons = 0;
};

class StringGenerator {
public:
    static const string& alphabet() {
        static const string chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789"
            "!@#%:;^&*()-.";
        return chars;
    }

    explicit StringGenerator(unsigned seed = 42) : rng(seed) {}

    vector<string> randomArray(int n) {
        vector<string> result;
        result.reserve(n);
        uniform_int_distribution<int> lenDist(10, 200);
        uniform_int_distribution<int> charDist(0, (int)alphabet().size() - 1);

        for (int i = 0; i < n; ++i) {
            int len = lenDist(rng);
            string s;
            s.reserve(len);
            for (int j = 0; j < len; ++j) {
                s.push_back(alphabet()[charDist(rng)]);
            }
            result.push_back(s);
        }
        return result;
    }

    vector<string> reversedArray(int n) {
        vector<string> result = randomArray(n);
        sort(result.begin(), result.end());
        reverse(result.begin(), result.end());
        return result;
    }

    vector<string> nearlySortedArray(int n) {
        vector<string> result = randomArray(n);
        sort(result.begin(), result.end());

        uniform_int_distribution<int> posDist(0, n - 1);
        int swaps = max(1, n / 20);
        for (int i = 0; i < swaps; ++i) {
            swap(result[posDist(rng)], result[posDist(rng)]);
        }
        return result;
    }

    vector<string> commonPrefixArray(int n) {
        vector<string> result;
        result.reserve(n);
        uniform_int_distribution<int> tailLenDist(4, 60);
        uniform_int_distribution<int> charDist(0, (int)alphabet().size() - 1);

        for (int i = 0; i < n; ++i) {
            string s = "common_prefix_" + to_string(i % 10) + "_";
            int len = tailLenDist(rng);
            for (int j = 0; j < len; ++j) {
                s.push_back(alphabet()[charDist(rng)]);
            }
            result.push_back(s);
        }
        return result;
    }
private:
    mt19937 rng;
};

class StringSortTester {
public:
    using SortFunction = void (*)(vector<string>&, long long&);

    Measurement measure(const vector<string>& input, SortFunction sorter, int repeats) {
        Measurement total;
        for (int r = 0; r < repeats; ++r) {
            vector<string> data = input;
            long long comparisons = 0;
            auto start = chrono::high_resolution_clock::now();
            sorter(data, comparisons);
            auto finish = chrono::high_resolution_clock::now();

            if (!is_sorted(data.begin(), data.end())) {
                cerr << "Sorting error\n";
                exit(1);
            }

            total.microseconds += chrono::duration<double, micro>(finish - start).count();
            total.charComparisons += comparisons;
        }
        total.microseconds /= repeats;
        total.charComparisons /= repeats;
        return total;
    }

    static bool countedLess(const string& a, const string& b, long long& comparisons) {
        size_t n = min(a.size(), b.size());
        for (size_t i = 0; i < n; ++i) {
            ++comparisons;
            if (a[i] < b[i]) return true;
            if (a[i] > b[i]) return false;
        }
        return a.size() < b.size();
    }

    static int charAt(const string& s, int d, long long& comparisons) {
        ++comparisons;
        if (d >= (int)s.size()) return -1;
        return (unsigned char)s[d];
    }

    static int charAtNoCount(const string& s, int d) {
        if (d >= (int)s.size()) return -1;
        return (unsigned char)s[d];
    }

    static void standardQuickSort(vector<string>& a, long long& comparisons) {
        quickSort(a, 0, (int)a.size() - 1, comparisons);
    }

    static void standardMergeSort(vector<string>& a, long long& comparisons) {
        vector<string> buffer(a.size());
        mergeSort(a, buffer, 0, (int)a.size(), comparisons);
    }

    static void ternaryStringQuickSort(vector<string>& a, long long& comparisons) {
        stringQuickSort(a, 0, (int)a.size() - 1, 0, comparisons);
    }

    static void lcpStringMergeSort(vector<string>& a, long long& comparisons) {
        vector<LcpItem> items;
        items.reserve(a.size());
        for (const string& s : a) items.push_back({s, 0});
        vector<LcpItem> sorted = lcpMergeSort(items, comparisons);
        for (size_t i = 0; i < sorted.size(); ++i) a[i] = sorted[i].value;
    }

    static void msdRadixSort(vector<string>& a, long long& comparisons) {
        vector<string> aux(a.size());
        msdSort(a, aux, 0, (int)a.size() - 1, 0, comparisons, false);
    }

    static void msdRadixSortWithQuickSwitch(vector<string>& a, long long& comparisons) {
        vector<string> aux(a.size());
        msdSort(a, aux, 0, (int)a.size() - 1, 0, comparisons, true);
    }

private:
    struct LcpItem {
        string value;
        int lcpPrev = 0;
    };

    struct CmpResult {
        int cmp = 0;
        int lcp = 0;
    };

    static CmpResult compareFrom(const string& a, const string& b, int start, long long& comparisons) {
        int i = start;
        while (i < (int)a.size() && i < (int)b.size()) {
            ++comparisons;
            if (a[i] < b[i]) return {-1, i};
            if (a[i] > b[i]) return {1, i};
            ++i;
        }
        if (a.size() == b.size()) return {0, i};
        return {(a.size() < b.size()) ? -1 : 1, i};
    }

    static void quickSort(vector<string>& a, int left, int right, long long& comparisons) {
        if (left >= right) return;
        string pivot = a[(left + right) / 2];
        int i = left;
        int j = right;
        while (i <= j) {
            while (countedLess(a[i], pivot, comparisons)) ++i;
            while (countedLess(pivot, a[j], comparisons)) --j;
            if (i <= j) {
                swap(a[i], a[j]);
                ++i;
                --j;
            }
        }
        if (left < j) quickSort(a, left, j, comparisons);
        if (i < right) quickSort(a, i, right, comparisons);
    }

    static void mergeSort(vector<string>& a, vector<string>& buffer, int left, int right, long long& comparisons) {
        if (right - left <= 1) return;
        int mid = (left + right) / 2;
        mergeSort(a, buffer, left, mid, comparisons);
        mergeSort(a, buffer, mid, right, comparisons);

        int i = left;
        int j = mid;
        int k = left;
        while (i < mid && j < right) {
            if (!countedLess(a[j], a[i], comparisons)) buffer[k++] = a[i++];
            else buffer[k++] = a[j++];
        }
        while (i < mid) buffer[k++] = a[i++];
        while (j < right) buffer[k++] = a[j++];
        for (int p = left; p < right; ++p) a[p] = buffer[p];
    }

    static void stringQuickSort(vector<string>& a, int left, int right, int depth, long long& comparisons) {
        if (left >= right) return;
        int lt = left;
        int gt = right;
        int pivot = charAt(a[(left + right) / 2], depth, comparisons);
        int i = left;

        while (i <= gt) {
            int current = charAt(a[i], depth, comparisons);
            if (current < pivot) swap(a[lt++], a[i++]);
            else if (current > pivot) swap(a[i], a[gt--]);
            else ++i;
        }

        stringQuickSort(a, left, lt - 1, depth, comparisons);
        if (pivot >= 0) stringQuickSort(a, lt, gt, depth + 1, comparisons);
        stringQuickSort(a, gt + 1, right, depth, comparisons);
    }

    static vector<LcpItem> lcpMergeSort(const vector<LcpItem>& input, long long& comparisons) {
        if (input.size() <= 1) return input;

        size_t mid = input.size() / 2;
        vector<LcpItem> left(input.begin(), input.begin() + mid);
        vector<LcpItem> right(input.begin() + mid, input.end());
        left = lcpMergeSort(left, comparisons);
        right = lcpMergeSort(right, comparisons);
        return mergeLcp(left, right, comparisons);
    }

    static vector<LcpItem> mergeLcp(const vector<LcpItem>& left,
                                    const vector<LcpItem>& right,
                                    long long& comparisons) {
        vector<LcpItem> result;
        result.reserve(left.size() + right.size());

        size_t i = 0;
        size_t j = 0;
        int lcpLeft = 0;
        int lcpRight = 0;
        bool hasLast = false;

        while (i < left.size() && j < right.size()) {
            bool takeLeft;
            int selectedLcp = 0;
            int lcpBetween = 0;

            if (!hasLast) {
                CmpResult cmp = compareFrom(left[i].value, right[j].value, 0, comparisons);
                takeLeft = cmp.cmp <= 0;
                selectedLcp = 0;
                lcpBetween = cmp.lcp;
            } else if (lcpLeft < lcpRight) {
                takeLeft = false;
                selectedLcp = lcpRight;
                lcpBetween = lcpLeft;
            } else if (lcpLeft > lcpRight) {
                takeLeft = true;
                selectedLcp = lcpLeft;
                lcpBetween = lcpRight;
            } else {
                CmpResult cmp = compareFrom(left[i].value, right[j].value, lcpLeft, comparisons);
                takeLeft = cmp.cmp <= 0;
                selectedLcp = lcpLeft;
                lcpBetween = cmp.lcp;
            }

            if (takeLeft) {
                result.push_back({left[i].value, selectedLcp});
                ++i;
                lcpLeft = (i < left.size()) ? left[i].lcpPrev : 0;
                lcpRight = lcpBetween;
            } else {
                result.push_back({right[j].value, selectedLcp});
                ++j;
                lcpRight = (j < right.size()) ? right[j].lcpPrev : 0;
                lcpLeft = lcpBetween;
            }
            hasLast = true;
        }

        while (i < left.size()) {
            result.push_back({left[i].value, hasLast ? lcpLeft : 0});
            ++i;
            hasLast = true;
            lcpLeft = (i < left.size()) ? left[i].lcpPrev : 0;
        }
        while (j < right.size()) {
            result.push_back({right[j].value, hasLast ? lcpRight : 0});
            ++j;
            hasLast = true;
            lcpRight = (j < right.size()) ? right[j].lcpPrev : 0;
        }

        return result;
    }

    static void msdSort(vector<string>& a, vector<string>& aux, int left, int right,
                        int depth, long long& comparisons, bool useQuickSwitch) {
        if (left >= right) return;
        if (useQuickSwitch && right - left + 1 < (int)StringGenerator::alphabet().size()) {
            stringQuickSort(a, left, right, depth, comparisons);
            return;
        }

        const int R = 256;
        vector<int> count(R + 2, 0);

        for (int i = left; i <= right; ++i) {
            int c = charAt(a[i], depth, comparisons);
            count[c + 2]++;
        }
        for (int r = 0; r < R + 1; ++r) {
            count[r + 1] += count[r];
        }

        vector<int> start = count;
        for (int i = left; i <= right; ++i) {
            int c = charAtNoCount(a[i], depth);
            aux[count[c + 1]++] = a[i];
        }
        for (int i = left; i <= right; ++i) {
            a[i] = aux[i - left];
        }

        for (int r = 1; r <= R; ++r) {
            int lo = left + start[r];
            int hi = left + start[r + 1] - 1;
            if (lo < hi) msdSort(a, aux, lo, hi, depth + 1, comparisons, useQuickSwitch);
        }
    }
};

int main() {
    const int maxSize = 3000;
    const int step = 100;
    const int repeats = 5;

    StringGenerator generator(2026);
    StringSortTester tester;

    vector<pair<string, vector<string>>> datasets = {
        {"random", generator.randomArray(maxSize)},
        {"reversed", generator.reversedArray(maxSize)},
        {"nearly_sorted", generator.nearlySortedArray(maxSize)},
        {"common_prefix", generator.commonPrefixArray(maxSize)}
    };

    vector<pair<string, StringSortTester::SortFunction>> algorithms = {
        {"quick_sort", StringSortTester::standardQuickSort},
        {"merge_sort", StringSortTester::standardMergeSort},
        {"ternary_string_quick_sort", StringSortTester::ternaryStringQuickSort},
        {"lcp_string_merge_sort", StringSortTester::lcpStringMergeSort},
        {"msd_radix_sort", StringSortTester::msdRadixSort},
        {"msd_radix_sort_with_quick_switch", StringSortTester::msdRadixSortWithQuickSwitch}
    };

    ofstream out("data/results.csv");
    out << "dataset,size,algorithm,avg_time_us,avg_char_comparisons\n";
    cout << fixed << setprecision(2);

    for (const auto& dataset : datasets) {
        for (int size = step; size <= maxSize; size += step) {
            vector<string> current(dataset.second.begin(), dataset.second.begin() + size);
            for (const auto& algorithm : algorithms) {
                Measurement m = tester.measure(current, algorithm.second, repeats);
                out << dataset.first << ',' << size << ',' << algorithm.first << ','
                    << fixed << setprecision(2) << m.microseconds << ','
                    << m.charComparisons << '\n';

                cout << dataset.first << " n=" << setw(4) << size
                     << " " << setw(34) << left << algorithm.first
                     << " time=" << setw(10) << right << m.microseconds
                     << " us chars=" << m.charComparisons << '\n';
            }
        }
    }

    return 0;
}
