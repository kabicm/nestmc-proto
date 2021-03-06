#pragma once

#include <initializer_list>
#include <utility>
#include <util/iterutil.hpp>
#include <util/range.hpp>

namespace arb {
namespace util {

template <typename I, typename S = I>
class cyclic_iterator : public iterator_adaptor<cyclic_iterator<I,S>, I> {
    using base = iterator_adaptor<cyclic_iterator<I,S>, I>;
    friend class iterator_adaptor<cyclic_iterator<I,S>, I>;

    I begin_;
    I inner_;
    S end_;
    typename base::difference_type off_;   // offset from begin

    const I& inner() const {
        return inner_;
    }

    I& inner() {
        return inner_;
    }

public:
    using value_type = typename base::value_type;
    using difference_type = typename base::difference_type;

    cyclic_iterator() = default;

    template <typename Iter, typename Sentinel>
    cyclic_iterator(Iter&& iter, Sentinel&& sentinel)
        : begin_(std::forward<Iter>(iter)),
          inner_(std::forward<Iter>(iter)),
          end_(std::forward<Sentinel>(sentinel)),
          off_(0)
    { }

    cyclic_iterator(const cyclic_iterator& other)
        : begin_(other.begin_),
          inner_(other.inner_),
          end_(other.end_),
          off_(other.off_)
    { }

    cyclic_iterator(cyclic_iterator&& other)
        : begin_(std::move(other.begin_)),
          inner_(std::move(other.inner_)),
          end_(std::move(other.end_)),
          off_(other.off_)
    { }


    cyclic_iterator& operator=(const cyclic_iterator& other) {
        if (this != &other) {
            inner_ = other.inner_;
            begin_ = other.begin_;
            end_   = other.end_;
            off_   = other.off_;
        }

        return *this;
    }

    cyclic_iterator& operator=(cyclic_iterator&& other) {
        if (this != &other) {
            inner_ = std::move(other.inner_);
            begin_ = std::move(other.begin_);
            end_   = std::move(other.end_);
            off_   = other.off_;
        }

        return *this;
    }

    // forward and input iterator requirements
    value_type operator*() const {
        return *inner_;
    }

    value_type operator[](difference_type n) const {
        return *(*this + n);
    }

    cyclic_iterator& operator++() {
        if (++inner_ == end_) {
            // wrap around
            inner_ = begin_;
        }

        ++off_;
        return *this;
    }

    cyclic_iterator operator++(int) {
        cyclic_iterator iter(*this);
        ++(*this);
        return iter;
    }

    cyclic_iterator& operator--() {
        if (inner_ == begin_) {
            // wrap around; use upto() to handle efficiently the move to the end
            // in case inner_ is a bidirectional iterator
            inner_ = upto(inner_, end_);
        }
        else {
            --inner_;
        }

        --off_;
        return *this;
    }

    cyclic_iterator operator--(int) {
        cyclic_iterator iter(*this);
        --(*this);
        return iter;
    }

    cyclic_iterator& operator+=(difference_type n) {
        // wrap distance
        auto size = util::distance(begin_, end_);

        // calculate distance from begin
        auto pos = (off_ += n);
        if (pos < 0) {
            auto mod = -pos % size;
            pos = mod ? size - mod : 0;
        }
        else {
            pos = pos % size;
        }

        inner_ = std::next(begin_, pos);
        return *this;
    }

    cyclic_iterator& operator-=(difference_type n) {
        return this->operator+=(-n);
    }

    bool operator==(const cyclic_iterator& other) const {
        return begin_ == other.begin_ && off_ == other.off_;
    }

    bool operator!=(const cyclic_iterator& other) const {
        return !(*this == other);
    }

    cyclic_iterator operator-(difference_type n) const {
        cyclic_iterator c(*this);
        return c -= n;
    }

    difference_type operator-(const cyclic_iterator& other) const {
        return off_ - other.off_;
    }

    bool operator<(const cyclic_iterator& other) const {
        return off_ < other.off_;
    }

    // expose inner iterator for testing against a sentinel
    template <typename Sentinel>
    bool operator==(const Sentinel& s) const {
        return inner_ == s;
    }

    template <typename Sentinel>
    bool operator!=(const Sentinel& s) const {
        return !(inner_ == s);
    }
};

template <typename I, typename S>
cyclic_iterator<I, S> make_cyclic_iterator(const I& iter, const S& sentinel) {
    return cyclic_iterator<I, S>(iter, sentinel);
}


template <
    typename Seq,
    typename SeqIter = typename sequence_traits<Seq>::const_iterator,
    typename SeqSentinel = typename sequence_traits<Seq>::const_sentinel,
    typename = enable_if_t<std::is_same<SeqIter, SeqSentinel>::value>
>
range<cyclic_iterator<SeqIter, SeqSentinel> > cyclic_view(const Seq& s) {
    return { make_cyclic_iterator(util::cbegin(s), util::cend(s)),
             make_cyclic_iterator(util::cend(s), util::cend(s)) };
}

template <
    typename Seq,
    typename SeqIter = typename sequence_traits<Seq>::const_iterator,
    typename SeqSentinel = typename sequence_traits<Seq>::const_sentinel,
    typename = enable_if_t<!std::is_same<SeqIter, SeqSentinel>::value>
>
range<cyclic_iterator<SeqIter, SeqSentinel>, SeqSentinel>
cyclic_view(const Seq& s) {
    return { make_cyclic_iterator(util::cbegin(s), util::cend(s)), util::cend(s) };
}

// Handle initializer lists
template <typename T>
range<cyclic_iterator<typename std::initializer_list<T>::const_iterator,
                      typename std::initializer_list<T>::const_iterator> >
cyclic_view(const std::initializer_list<T> &list) {
    return { make_cyclic_iterator(util::cbegin(list), util::cend(list)),
             make_cyclic_iterator(util::cend(list), util::cend(list)) };
}

} // namespace util
} // namespace arb
