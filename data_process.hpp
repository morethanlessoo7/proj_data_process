#pragma once
#include <algorithm>
#include <exception>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>
#include <future>

template <typename, template <typename...> typename>
struct is_instance_impl : public std::false_type {
};

template <typename T, template <typename...> typename U>
struct is_instance_impl<U<T>, U> : public std::true_type {
};

template <typename T, template <typename...> typename U>
using is_instance = is_instance_impl<std::remove_cvref_t<std::remove_pointer_t<std::decay_t<T>>>, U>;

template <typename T, template <typename...> typename U>
inline constexpr bool is_instance_v = is_instance<T, U>::value;

class TooFewRankException : public std::exception {
   public:
    const char *what() const noexcept override {
        return "too few ranks exception";
    }
};

class NotSortedException : public std::exception {
   public:
    const char *what() const noexcept override {
        return "not a sorted array here";
    }
};

template <typename T, std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, bool> = true>
class DataProcess {
    using Pci = int;

   private:
    std::map<T, size_t> rank_map;
    size_t size;
    std::vector<int> const &pci_list;

   public:
    template <typename Container, std::enable_if_t<(
                                                       is_instance_v<Container, std::list> ||
                                                       is_instance_v<Container, std::vector> ||
                                                       is_instance_v<Container, std::set> ||
                                                       is_instance_v<Container, std::unordered_set>)&&std::is_convertible_v<typename Container::value_type, T>,
                                                   bool> = true>
    inline std::vector<double> get_freq(Container const &arr) {
        std::vector<double> freq(size, 0.);
        for (auto &t : arr) {
            auto iter = rank_map.lower_bound(t);
            if (iter == rank_map.end()) {
                freq[0] += 1;
            } else {
                freq[iter->second] += 1;
            }
        }
        double N = arr.size();
        for (auto &f : freq) {
            f /= N;
        }
        return freq;
    }

    /**
    * analysis one list.
    * iter should contain a pair: (pci, T)
    * */
    template <typename Iter,
              std::enable_if_t<std::is_same_v<typename Iter::value_type, std::pair<Pci, T>>, bool> = true>
    std::vector<double> analysis_one(Iter const begin, Iter const end) {
        std::unordered_map<Pci, std::list<T>> pci_data_map;
        for (auto iter = begin; iter != end; ++iter) {
            pci_data_map[iter->first].emplace_back(iter->second);
        }
        size_t sz = size + 1;
        size_t N = pci_list.size() * sz;
        std::vector<double> ret;
        ret.reserve(N);
        size_t S = 0;
        for (auto &pci : pci_list) {
            try {
                auto const &arr = pci_data_map.at(pci);
                auto freq = get_freq(arr);
                ret.emplace_back(arr.size());
                S += arr.size();
                ret.insert(ret.end(), freq.begin(), freq.end());
            } catch (std::out_of_range const &) {
            }
        }
        for (int i = 0; i < N; i += sz) {
            ret[i] /= S;
        }
        return ret;
    }

    /**
    * undefined behavior if not satisfying the condition: begin < interval <= end
    * */
    template <typename Iter,
              std::enable_if_t<std::is_same_v<typename Iter::value_type, std::pair<Pci, T>>, bool> = true>
    inline std::list<std::vector<double>> analysis(Iter const begin, Iter const interval, Iter const end, int thread_num = 64) {
        Iter iter = begin, iter2 = interval;
        std::list<std::vector<double>> ret;
        while (iter2 != end) {
            ret.emplace_back(analysis_one(iter++, iter2++));
        }
        return ret;
    }

    /**
    * @param rank_arr 各个等级的分界，应当是一个有序数组
    * */
    DataProcess(std::vector<T> const &rank_arr, std::vector<int> const &pci_list) : size(rank_arr.size()), pci_list(pci_list) {
        if (size < 2) {
            throw TooFewRankException();
        }
        if (size < 20) {
            std::cerr << "WARN: too few ranks(" << size << ") which may reduce precision." << std::endl;
        }

        auto prev = rank_arr.begin();
        auto cur(prev + 1);
        while (cur != rank_arr.end()) {
            if (*prev++ > *cur++) {
                throw NotSortedException();
            }
        }

        auto hint(rank_map.end());
        for (size_t i = 0; i < size; ++i) {
            hint = rank_map.emplace_hint(hint, rank_arr[i], i);
        }
    }
};
