#pragma once
#ifndef CPP_TREE_SITTER_STRING_VIEW_HPP
#define CPP_TREE_SITTER_STRING_VIEW_HPP

#include <cstddef>
#include <ios>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <string>

namespace ts
{
    namespace details
    {
        class StringView
        {
        public:
            using value_type             = char;
            using data_type              = const value_type *;
            using size_type              = std::size_t;
            using pointer                = data_type;
            using const_pointer          = data_type;
            using const_reference        = const value_type &;
            using const_iterator         = data_type;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

            static constexpr size_type npos = size_type(-1);

            ////////////////////////////////////////////////////////////////
            // Lifecycle
            ////////////////////////////////////////////////////////////////

            constexpr StringView() noexcept : data_(nullptr), size_(0)
            {}

            constexpr StringView(const StringView &) noexcept = default;

            StringView &operator=(const StringView &) noexcept = default;

            constexpr StringView(data_type s, size_type count) noexcept : data_(s), size_(s ? count : 0)
            {}

            constexpr StringView(data_type s) noexcept : data_(s), size_(s ? traits_length(s) : 0)
            {}

            StringView(const std::string &s) noexcept : data_(s.data()), size_(s.size())
            {}

            constexpr StringView(std::nullptr_t) noexcept : StringView()
            {}

            ////////////////////////////////////////////////////////////////
            // Data
            ////////////////////////////////////////////////////////////////

            [[nodiscard]] constexpr const_pointer data() const noexcept
            {
                return data_;
            }

            [[nodiscard]] constexpr const_iterator begin() const noexcept
            {
                return data_;
            }

            [[nodiscard]] constexpr const_iterator end() const noexcept
            {
                return data_ + size_;
            }

            [[nodiscard]] constexpr const_iterator cbegin() const noexcept
            {
                return data_;
            }

            [[nodiscard]] constexpr const_iterator cend() const noexcept
            {
                return data_ + size_;
            }

            [[nodiscard]] const_reverse_iterator rbegin() const noexcept
            {
                return const_reverse_iterator(end());
            }

            [[nodiscard]] const_reverse_iterator rend() const noexcept
            {
                return const_reverse_iterator(begin());
            }

            [[nodiscard]] const_reverse_iterator crbegin() const noexcept
            {
                return const_reverse_iterator(cend());
            }

            [[nodiscard]] const_reverse_iterator crend() const noexcept
            {
                return const_reverse_iterator(cbegin());
            }

            ////////////////////////////////////////////////////////////////
            // Size
            ////////////////////////////////////////////////////////////////

            [[nodiscard]] constexpr size_type size() const noexcept
            {
                return size_;
            }

            [[nodiscard]] constexpr size_type length() const noexcept
            {
                return size_;
            }

            [[nodiscard]] constexpr size_type max_size() const noexcept
            {
                return npos;
            }

            [[nodiscard]] constexpr bool empty() const noexcept
            {
                return size_ == 0;
            }

            ////////////////////////////////////////////////////////////////
            // Element access
            ////////////////////////////////////////////////////////////////

            [[nodiscard]] constexpr const_reference operator[](size_type pos) const noexcept
            {
                return data_[pos];
            }

            [[nodiscard]] constexpr const_reference at(size_type pos) const
            {
                return (pos >= size_) ? throw std::out_of_range("StringView::at") : data_[pos];
            }

            [[nodiscard]] constexpr const_reference front() const noexcept
            {
                return data_[0];
            }

            [[nodiscard]] constexpr const_reference back() const noexcept
            {
                return data_[size_ - 1];
            }

            ////////////////////////////////////////////////////////////////
            // Manipulation
            ////////////////////////////////////////////////////////////////

            constexpr void remove_prefix(size_type n) noexcept
            {
                data_ += n;
                size_ -= n;
            }

            constexpr void remove_suffix(size_type n) noexcept
            {
                size_ -= n;
            }

            constexpr void swap(StringView &other) noexcept
            {
                const data_type d = data_;
                data_             = other.data_;
                other.data_       = d;

                const size_type s = size_;
                size_             = other.size_;
                other.size_       = s;
            }

            [[nodiscard]] constexpr StringView substr(size_type pos = 0, size_type count = npos) const
            {
                return (pos > size_) ? throw std::out_of_range("StringView::substr")
                                     : StringView(data_ + pos, (count < size_ - pos) ? count : size_ - pos);
            }

            [[nodiscard]] size_type copy(char *dest, size_type count, size_type pos = 0) const
            {
                if (pos > size_)
                {
                    throw std::out_of_range("StringView::copy");
                }
                const size_type n = (count < size_ - pos) ? count : size_ - pos;
                std::char_traits<char>::copy(dest, data_ + pos, n);
                return n;
            }

            ////////////////////////////////////////////////////////////////
            // Comparision
            ////////////////////////////////////////////////////////////////

            [[nodiscard]] constexpr int compare(StringView v) const noexcept
            {
                const size_type rlen = (size_ < v.size_) ? size_ : v.size_;
                const int       cmp  = compare_n(data_, v.data_, rlen);
                return cmp != 0 ? cmp : (size_ < v.size_ ? -1 : (size_ > v.size_ ? 1 : 0));
            }

            [[nodiscard]] constexpr int compare(const char *s) const noexcept
            {
                return compare(StringView(s));
            }

            [[nodiscard]] int compare(const std::string &s) const noexcept
            {
                return compare(StringView(s));
            }

            [[nodiscard]] constexpr friend bool operator==(StringView lhs, StringView rhs) noexcept
            {
                return lhs.size_ == rhs.size_ && compare_n(lhs.data_, rhs.data_, lhs.size_) == 0;
            }

            [[nodiscard]] constexpr friend bool operator!=(StringView lhs, StringView rhs) noexcept
            {
                return !(lhs == rhs);
            }

            [[nodiscard]] constexpr friend bool operator<(StringView lhs, StringView rhs) noexcept
            {
                return lhs.compare(rhs) < 0;
            }

            [[nodiscard]] constexpr friend bool operator<=(StringView lhs, StringView rhs) noexcept
            {
                return lhs.compare(rhs) <= 0;
            }

            [[nodiscard]] constexpr friend bool operator>(StringView lhs, StringView rhs) noexcept
            {
                return lhs.compare(rhs) > 0;
            }

            [[nodiscard]] constexpr friend bool operator>=(StringView lhs, StringView rhs) noexcept
            {
                return lhs.compare(rhs) >= 0;
            }

            ////////////////////////////////////////////////////////////////
            // Conversion
            ////////////////////////////////////////////////////////////////

            [[nodiscard]] explicit operator std::string() const
            {
                return size_ ? std::string(data_, size_) : std::string();
            }

        private:
            static constexpr size_type traits_length(const char *s) noexcept
            {
#if defined(__GNUC__) || defined(__clang__) || (defined(_MSC_VER) && _MSC_VER >= 1926)
                return __builtin_strlen(s);
#else
                return non_constexpr_strlen(s);
#endif // defined(__GNUC__) || defined(__clang__) || (defined(_MSC_VER) && _MSC_VER >= 1926)
            }

            static constexpr int compare_n(const char *a, const char *b, size_type n) noexcept
            {
#if defined(__GNUC__) || defined(__clang__) || (defined(_MSC_VER) && _MSC_VER >= 1926)
                return n == 0 ? 0 : __builtin_memcmp(a, b, n);
#else
                return n == 0 ? 0 : non_constexpr_memcmp(a, b, n);
#endif // defined(__GNUC__) || defined(__clang__) || (defined(_MSC_VER) && _MSC_VER >= 1926)
            }

#if !(defined(__GNUC__) || defined(__clang__) || (defined(_MSC_VER) && _MSC_VER >= 1926))
            static size_type non_constexpr_strlen(const char *s) noexcept
            {
                size_type n = 0;
                while (s[n] != '\0')
                {
                    ++n;
                }
                return n;
            }

            static int non_constexpr_memcmp(const char *a, const char *b, size_type n) noexcept
            {
                for (size_type i = 0; i < n; ++i)
                {
                    const unsigned char ca = static_cast<unsigned char>(a[i]);
                    const unsigned char cb = static_cast<unsigned char>(b[i]);
                    if (ca != cb)
                    {
                        return ca < cb ? -1 : 1;
                    }
                }
                return 0;
            }
#endif // !(defined(__GNUC__) || defined(__clang__) || (defined(_MSC_VER) && _MSC_VER >= 1926))

        private:
            data_type data_;
            size_type size_;
        };

        inline std::ostream &operator<<(std::ostream &os, StringView v)
        {
            return os.write(v.data(), static_cast<std::streamsize>(v.size()));
        }
    } // namespace details
} // namespace ts

#endif // CPP_TREE_SITTER_STRING_VIEW_HPP
