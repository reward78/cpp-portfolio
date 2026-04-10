#pragma once

#include <algorithm>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename Key, typename Value>
struct KV {
	Key key;
	Value value;

	bool operator==(const KV& other) const = default;
};

template <typename Base, typename Joined>
struct JoinResult {
	Base base;
	std::optional<Joined> joined;

	bool operator==(const JoinResult& other) const = default;
};

namespace detail {

template <typename T>
using Decay = std::remove_cvref_t<T>;

template <typename RangeT, typename PredicateT>
struct FilterState {
	using Range = RangeT;
	using Predicate = PredicateT;
};

template <typename RangeT, typename FuncT>
struct TransformState {
	using Range = RangeT;
	using Func = FuncT;
};

template <typename RangeT>
struct RangeState {
	using Range = RangeT;
};

template <typename Range>
class DataFlow {
public:
	using iterator = decltype(std::begin(std::declval<Range&>()));
	using value_type = Decay<decltype(*std::begin(std::declval<Range&>()))>;

	explicit DataFlow(Range range)
		: range_(std::forward<Range>(range)) {
	}

	auto begin() { // NOLINT(readability-identifier-naming)
		return std::begin(range_);
	}

	auto end() { // NOLINT(readability-identifier-naming)
		return std::end(range_);
	}

	auto begin() const { // NOLINT(readability-identifier-naming)
		return std::begin(range_);
	}

	auto end() const { // NOLINT(readability-identifier-naming)
		return std::end(range_);
	}

private:
	Range range_;
};

struct Identity {
	template <typename T>
	constexpr decltype(auto) operator()(T&& value) const noexcept {
		return std::forward<T>(value);
	}
};

struct KVKeySelector {
	template <typename Key, typename Value>
	constexpr const Key& operator()(const KV<Key, Value>& value) const noexcept {
		return value.key;
	}
};

struct KVValueSelector {
	template <typename Key, typename Value>
	constexpr const Value& operator()(const KV<Key, Value>& value) const noexcept {
		return value.value;
	}
};

inline bool IsDelimiter(char ch, const std::string& delimiters) {
	return delimiters.find(ch) != std::string::npos;
}

inline std::string ReadAll(std::istream& stream) {
	std::ostringstream output;
	output << stream.rdbuf();
	return output.str();
}

inline std::string ReadAll(const std::string& value) {
	return value;
}

template <typename T>
class FilterView {
public:
	using Range = typename T::Range;
	using Predicate = typename T::Predicate;

	FilterView(Range range, Predicate predicate)
		: range_(std::move(range))
		, predicate_(std::move(predicate)) {
	}

	class Iterator {
	public:
		using BaseIterator = decltype(std::begin(std::declval<Range&>()));
		using iterator_category = std::input_iterator_tag;
		using value_type = Decay<decltype(*std::declval<BaseIterator&>())>;
		using difference_type = std::ptrdiff_t;

		Iterator(BaseIterator current, BaseIterator end, Predicate* predicate)
			: current_(current)
			, end_(end)
			, predicate_(predicate) {
			SkipUntilMatch();
		}

		decltype(auto) operator*() const {
			return *current_;
		}

		Iterator& operator++() {
			++current_;
			SkipUntilMatch();
			return *this;
		}

		void operator++(int) {
			++(*this);
		}

		bool operator==(const Iterator& other) const {
			return current_ == other.current_;
		}

	private:
		void SkipUntilMatch() {
			while (current_ != end_ && !std::invoke(*predicate_, *current_)) {
				++current_;
			}
		}

		BaseIterator current_;
		BaseIterator end_;
		Predicate* predicate_;
	};

	Iterator begin() { // NOLINT(readability-identifier-naming)
		return Iterator(std::begin(range_), std::end(range_), &predicate_);
	}

	Iterator end() { // NOLINT(readability-identifier-naming)
		return Iterator(std::end(range_), std::end(range_), &predicate_);
	}

private:
	Range range_;
	Predicate predicate_;
};

template <typename T>
class TransformView {
public:
	using Range = typename T::Range;
	using Func = typename T::Func;

	TransformView(Range range, Func func)
		: range_(std::move(range))
		, func_(std::move(func)) {
	}

	class Iterator {
	public:
		using BaseIterator = decltype(std::begin(std::declval<Range&>()));
		using iterator_category = std::input_iterator_tag;
		using value_type = Decay<std::invoke_result_t<Func&, decltype(*std::declval<BaseIterator&>())>>;
		using difference_type = std::ptrdiff_t;

		Iterator(BaseIterator current, Func* func)
			: current_(current)
			, func_(func) {
		}

		value_type operator*() const {
			return std::invoke(*func_, *current_);
		}

		Iterator& operator++() {
			++current_;
			return *this;
		}

		void operator++(int) {
			++(*this);
		}

		bool operator==(const Iterator& other) const {
			return current_ == other.current_;
		}

	private:
		BaseIterator current_;
		Func* func_;
	};

	Iterator begin() { // NOLINT(readability-identifier-naming)
		return Iterator(std::begin(range_), &func_);
	}

	Iterator end() { // NOLINT(readability-identifier-naming)
		return Iterator(std::end(range_), &func_);
	}

private:
	Range range_;
	Func func_;
};

template <typename T>
class DropNulloptView {
public:
	using Range = typename T::Range;

	explicit DropNulloptView(Range range)
		: range_(std::move(range)) {
	}

	class Iterator {
	public:
		using BaseIterator = decltype(std::begin(std::declval<Range&>()));
		using OptionalType = Decay<decltype(*std::declval<BaseIterator&>())>;
		using value_type = typename OptionalType::value_type;
		using iterator_category = std::input_iterator_tag;
		using difference_type = std::ptrdiff_t;

		Iterator(BaseIterator current, BaseIterator end)
			: current_(current)
			, end_(end) {
			SkipEmpty();
		}

		value_type operator*() const {
			return **current_;
		}

		Iterator& operator++() {
			++current_;
			SkipEmpty();
			return *this;
		}

		void operator++(int) {
			++(*this);
		}

		bool operator==(const Iterator& other) const {
			return current_ == other.current_;
		}

	private:
		void SkipEmpty() {
			while (current_ != end_ && !current_->has_value()) {
				++current_;
			}
		}

		BaseIterator current_;
		BaseIterator end_;
	};

	Iterator begin() { // NOLINT(readability-identifier-naming)
		return Iterator(std::begin(range_), std::end(range_));
	}

	Iterator end() { // NOLINT(readability-identifier-naming)
		return Iterator(std::end(range_), std::end(range_));
	}

private:
	Range range_;
};

template <typename T>
class SplitView {
public:
	using Range = typename T::Range;

	SplitView(Range range, std::string delimiters)
		: range_(std::move(range))
		, delimiters_(std::move(delimiters)) {
	}

	class Iterator {
	public:
		using BaseIterator = decltype(std::begin(std::declval<Range&>()));
		using iterator_category = std::input_iterator_tag;
		using value_type = std::string;
		using difference_type = std::ptrdiff_t;

		Iterator(BaseIterator current, BaseIterator end, const std::string* delimiters)
			: current_(current)
			, end_(end)
			, delimiters_(delimiters) {
			Advance();
		}

		const std::string& operator*() const {
			return current_token_;
		}

		Iterator& operator++() {
			Advance();
			return *this;
		}

		void operator++(int) {
			++(*this);
		}

		bool operator==(const Iterator& other) const {
			return current_ == other.current_ && source_offset_ == other.source_offset_ && finished_ == other.finished_;
		}

	private:
		void Advance() {
			while (true) {
				if (!buffer_loaded_) {
					if (current_ == end_) {
						finished_ = true;
						source_offset_ = 0;
						return;
					}
					buffer_ = ReadAll(*current_);
					buffer_loaded_ = true;
					source_offset_ = 0;
				}

				if (source_offset_ < buffer_.size()) {
					std::size_t token_end = source_offset_;
					while (token_end < buffer_.size() && !IsDelimiter(buffer_[token_end], *delimiters_)) {
						++token_end;
					}
					current_token_ = buffer_.substr(source_offset_, token_end - source_offset_);
					source_offset_ = token_end;
					if (source_offset_ < buffer_.size()) {
						++source_offset_;
					}
					finished_ = false;
					return;
				}

				++current_;
				buffer_loaded_ = false;
			}
		}

		BaseIterator current_;
		BaseIterator end_;
		const std::string* delimiters_;
		std::string buffer_;
		std::string current_token_;
		std::size_t source_offset_ = 0;
		bool buffer_loaded_ = false;
		bool finished_ = false;
	};

	Iterator begin() { // NOLINT(readability-identifier-naming)
		return Iterator(std::begin(range_), std::end(range_), &delimiters_);
	}

	Iterator end() { // NOLINT(readability-identifier-naming)
		return Iterator(std::end(range_), std::end(range_), &delimiters_);
	}

private:
	Range range_;
	std::string delimiters_;
};

template <typename Range>
using RangeValue = Decay<decltype(*std::begin(std::declval<Range&>()))>;

} // namespace detail

template <typename Range>
auto AsDataFlow(Range&& range) {
	return detail::DataFlow<Range&&>(std::forward<Range>(range));
}

template <typename Left, typename Adapter>
requires requires(Adapter&& adapter, Left&& left) {
	std::forward<Adapter>(adapter)(std::forward<Left>(left));
}
decltype(auto) operator|(Left&& left, Adapter&& adapter) {
	return std::forward<Adapter>(adapter)(std::forward<Left>(left));
}

template <typename Predicate>
class FilterAdapter {
public:
	explicit FilterAdapter(Predicate predicate)
		: predicate_(std::move(predicate)) {
	}

	template <typename Range>
	auto operator()(Range&& range) const {
		return detail::FilterView<detail::FilterState<std::remove_reference_t<Range>, std::remove_cvref_t<Predicate>>>(
			std::forward<Range>(range),
			predicate_
		);
	}

private:
	Predicate predicate_;
};

template <typename Predicate>
auto Filter(Predicate predicate) {
	return FilterAdapter<Predicate>(std::move(predicate));
}

template <typename Func>
class TransformAdapter {
public:
	explicit TransformAdapter(Func func)
		: func_(std::move(func)) {
	}

	template <typename Range>
	auto operator()(Range&& range) const {
		return detail::TransformView<detail::TransformState<std::remove_reference_t<Range>, std::remove_cvref_t<Func>>>(
			std::forward<Range>(range),
			func_
		);
	}

private:
	Func func_;
};

template <typename Func>
auto Transform(Func func) {
	return TransformAdapter<Func>(std::move(func));
}

class SplitAdapter {
public:
	explicit SplitAdapter(std::string delimiters)
		: delimiters_(std::move(delimiters)) {
	}

	template <typename Range>
	auto operator()(Range&& range) const {
		return detail::SplitView<detail::RangeState<std::remove_reference_t<Range>>>(
			std::forward<Range>(range),
			delimiters_
		);
	}

private:
	std::string delimiters_;
};

inline auto Split(std::string delimiters) {
	return SplitAdapter(std::move(delimiters));
}

class DropNulloptAdapter {
public:
	template <typename Range>
	auto operator()(Range&& range) const {
		return detail::DropNulloptView<detail::RangeState<std::remove_reference_t<Range>>>(
			std::forward<Range>(range)
		);
	}
};

inline auto DropNullopt() {
	return DropNulloptAdapter();
}

class AsVectorAdapter {
public:
	template <typename Range>
	auto operator()(Range&& range) const {
		using Value = detail::Decay<decltype(*std::begin(range))>;
		std::vector<Value> result;
		for (auto&& value : range) {
			result.push_back(value);
		}
		return result;
	}
};

inline auto AsVector() {
	return AsVectorAdapter();
}

template <typename Stream>
class WriteAdapter {
public:
	WriteAdapter(Stream& stream, char delimiter)
		: stream_(stream)
		, delimiter_(delimiter) {
	}

	template <typename Range>
	std::size_t operator()(Range&& range) const {
		std::size_t written = 0;
		for (auto&& value : range) {
			stream_ << value << delimiter_;
			++written;
		}
		return written;
	}

private:
	Stream& stream_;
	char delimiter_;
};

template <typename Stream>
auto Write(Stream& stream, char delimiter) {
	return WriteAdapter<Stream>(stream, delimiter);
}

template <typename Stream>
auto Out(Stream& stream) {
	return Write(stream, '\n');
}

class OpenFilesAdapter {
public:
	template <typename Range>
	auto operator()(Range&& range) const {
		std::vector<std::ifstream> result;
		for (auto&& path : range) {
			std::ifstream input(path);
			result.push_back(std::move(input));
		}
		return result;
	}
};

inline auto OpenFiles() {
	return OpenFilesAdapter();
}

inline auto Dir(const std::filesystem::path& path, bool recursive) {
	std::vector<std::filesystem::path> result;
	if (!std::filesystem::exists(path)) {
		return result;
	}

	if (recursive) {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
			if (entry.is_regular_file()) {
				result.push_back(entry.path());
			}
		}
	} else {
		for (const auto& entry : std::filesystem::directory_iterator(path)) {
			if (entry.is_regular_file()) {
				result.push_back(entry.path());
			}
		}
	}
	return result;
}

template <typename RightRange, typename LeftKeySelector, typename RightKeySelector>
class JoinAdapter {
public:
	JoinAdapter(RightRange right_range, LeftKeySelector left_key_selector, RightKeySelector right_key_selector)
		: right_range_(std::move(right_range))
		, left_key_selector_(std::move(left_key_selector))
		, right_key_selector_(std::move(right_key_selector)) {
	}

	template <typename LeftRange>
	auto operator()(LeftRange&& left_range) const {
		using LeftValue = detail::RangeValue<LeftRange>;
		using RightValue = detail::RangeValue<RightRange>;
		using LeftProjection = detail::Decay<std::invoke_result_t<detail::Identity&, LeftValue>>;
		using RightProjection = detail::Decay<std::invoke_result_t<detail::Identity&, RightValue>>;
		using Key = detail::Decay<std::invoke_result_t<LeftKeySelector&, LeftValue>>;
		using Result = JoinResult<LeftProjection, RightProjection>;

		std::unordered_map<Key, RightProjection> index;
		for (auto&& value : right_range_) {
			index.emplace(std::invoke(right_key_selector_, value), std::invoke(detail::Identity{}, value));
		}

		std::vector<Result> result;
		for (auto&& value : left_range) {
			auto key = std::invoke(left_key_selector_, value);
			auto found = index.find(key);
			if (found != index.end()) {
				result.push_back(Result{std::invoke(detail::Identity{}, value), found->second});
			} else {
				result.push_back(Result{std::invoke(detail::Identity{}, value), std::nullopt});
			}
		}
		return result;
	}

private:
	RightRange right_range_;
	LeftKeySelector left_key_selector_;
	RightKeySelector right_key_selector_;
};

template <typename RightRange>
class KVJoinAdapter {
public:
	explicit KVJoinAdapter(RightRange right_range)
		: right_range_(std::move(right_range)) {
	}

	template <typename LeftRange>
	auto operator()(LeftRange&& left_range) const {
		using LeftElement = detail::RangeValue<LeftRange>;
		using RightElement = detail::RangeValue<RightRange>;
		using Key = detail::Decay<decltype(detail::KVKeySelector{}(std::declval<LeftElement>()))>;
		using LeftValue = detail::Decay<decltype(detail::KVValueSelector{}(std::declval<LeftElement>()))>;
		using RightValue = detail::Decay<decltype(detail::KVValueSelector{}(std::declval<RightElement>()))>;
		using Result = JoinResult<LeftValue, RightValue>;

		std::unordered_map<Key, RightValue> index;
		for (auto&& value : right_range_) {
			index.emplace(detail::KVKeySelector{}(value), detail::KVValueSelector{}(value));
		}

		std::vector<Result> result;
		for (auto&& value : left_range) {
			auto found = index.find(detail::KVKeySelector{}(value));
			if (found != index.end()) {
				result.push_back(Result{detail::KVValueSelector{}(value), found->second});
			} else {
				result.push_back(Result{detail::KVValueSelector{}(value), std::nullopt});
			}
		}
		return result;
	}

private:
	RightRange right_range_;
};

template <typename RightRange>
auto Join(RightRange&& right_range) {
	return KVJoinAdapter<std::remove_reference_t<RightRange>>(std::forward<RightRange>(right_range));
}

template <typename RightRange, typename LeftKeySelector, typename RightKeySelector>
auto Join(RightRange&& right_range, LeftKeySelector left_key_selector, RightKeySelector right_key_selector) {
	return JoinAdapter<std::remove_reference_t<RightRange>, LeftKeySelector, RightKeySelector>(
		std::forward<RightRange>(right_range),
		std::move(left_key_selector),
		std::move(right_key_selector)
	);
}

template <typename InitialValue, typename Aggregator, typename KeySelector>
class AggregateByKeyAdapter {
public:
	AggregateByKeyAdapter(InitialValue initial_value, Aggregator aggregator, KeySelector key_selector)
		: initial_value_(std::move(initial_value))
		, aggregator_(std::move(aggregator))
		, key_selector_(std::move(key_selector)) {
	}

	template <typename Range>
	auto operator()(Range&& range) const {
		using Value = detail::RangeValue<Range>;
		using Key = detail::Decay<std::invoke_result_t<KeySelector&, Value>>;
		using Accumulator = detail::Decay<InitialValue>;

		std::unordered_map<Key, std::size_t> positions;
		std::vector<std::pair<Key, Accumulator>> result;

		for (auto&& value : range) {
			Key key = std::invoke(key_selector_, value);
			auto [it, inserted] = positions.emplace(key, result.size());
			if (inserted) {
				result.emplace_back(key, initial_value_);
			}
			std::invoke(aggregator_, value, result[it->second].second);
		}

		return result;
	}

private:
	InitialValue initial_value_;
	Aggregator aggregator_;
	KeySelector key_selector_;
};

template <typename InitialValue, typename Aggregator, typename KeySelector>
auto AggregateByKey(InitialValue initial_value, Aggregator aggregator, KeySelector key_selector) {
	return AggregateByKeyAdapter<InitialValue, Aggregator, KeySelector>(
		std::move(initial_value),
		std::move(aggregator),
		std::move(key_selector)
	);
}

class SplitExpectedAdapter {
public:
	template <typename Range>
	auto operator()(Range&& range) const {
		using ExpectedType = detail::RangeValue<Range>;
		using Value = typename ExpectedType::value_type;
		using Error = typename ExpectedType::error_type;

		std::vector<Error> errors;
		std::vector<Value> values;

		for (auto&& item : range) {
			if (item.has_value()) {
				values.push_back(*item);
			} else {
				errors.push_back(item.error());
			}
		}

		return std::make_pair(
			detail::DataFlow<std::vector<Error>>(std::move(errors)),
			detail::DataFlow<std::vector<Value>>(std::move(values))
		);
	}
};

inline auto SplitExpected() {
	return SplitExpectedAdapter();
}
