#pragma once
#ifndef CPP_TREE_SITTER_STRING_VIEW_HPP
#define CPP_TREE_SITTER_STRING_VIEW_HPP

// STL
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
        /// @brief Non-owning read-only view over a contiguous character sequence.
        class StringView
        {
        public:
#pragma region TYPE_ALIASES

            ////////////////////////////////////////////////////////////////
            // Type Aliases
            ////////////////////////////////////////////////////////////////

            using value_type             = char;
            using data_type              = const value_type *;
            using size_type              = std::size_t;
            using pointer                = data_type;
            using const_pointer          = data_type;
            using const_reference        = const value_type &;
            using const_iterator         = data_type;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

#pragma endregion

#pragma region CONSTANTS

            ////////////////////////////////////////////////////////////////
            // Constants
            ////////////////////////////////////////////////////////////////

            /// @brief Special value indicating that no valid position or size was specified.
            static constexpr size_type npos = size_type(-1);

#pragma endregion

#pragma region LIFECYCLE

            ////////////////////////////////////////////////////////////////
            // Lifecycle
            ////////////////////////////////////////////////////////////////

            /// @brief Creates an empty string view.
            constexpr StringView() noexcept : data_(nullptr), size_(0)
            {}

            /**
             * @brief Creates a copy of another string view.
             * @param other String view to copy.
             */
            constexpr StringView(const StringView &other) noexcept = default;

            /**
             * @brief Replaces the view with a copy of another string view.
             * @param other String view to copy.
             * @return Reference to this string view.
             */
            StringView &operator=(const StringView &other) noexcept = default;

            /**
             * @brief Creates a string view over a character sequence of the specified length.
             * @param s Pointer to the first character.
             * @param count Number of characters in the sequence.
             */
            constexpr StringView(data_type s, size_type count) noexcept : data_(s), size_(s ? count : 0)
            {}

            /**
             * @brief Creates a string view over a null-terminated character sequence.
             * @param s Pointer to the null-terminated character sequence.
             */
            constexpr StringView(data_type s) noexcept : data_(s), size_(s ? traits_length(s) : 0)
            {}

            /**
             * @brief Creates a string view over the contents of a string.
             * @param s String whose character data is referenced by the view.
             *
             * @note The referenced string must remain alive and must not invalidate its character storage while the
             * view is in use.
             */
            StringView(const std::string &s) noexcept : data_(s.data()), size_(s.size())
            {}

            /**
             * @brief Creates an empty string view from a null pointer.
             * @param nullptr Null pointer used to initialize an empty view.
             */
            constexpr StringView(std::nullptr_t) noexcept : StringView()
            {}

#pragma endregion

#pragma region DATA

            ////////////////////////////////////////////////////////////////
            // Data
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Returns a pointer to the first character.
             * @return Pointer to the first character, or nullptr for an empty default-constructed view.
             */
            [[nodiscard]] constexpr const_pointer data() const noexcept
            {
                return data_;
            }

            /**
             * @brief Returns an iterator to the first character.
             * @return Iterator pointing to the first character.
             */
            [[nodiscard]] constexpr const_iterator begin() const noexcept
            {
                return data_;
            }

            /**
             * @brief Returns an iterator past the last character.
             * @return Iterator pointing past the last character.
             */
            [[nodiscard]] constexpr const_iterator end() const noexcept
            {
                return data_ + size_;
            }

            /**
             * @brief Returns a const iterator to the first character.
             * @return Const iterator pointing to the first character.
             */
            [[nodiscard]] constexpr const_iterator cbegin() const noexcept
            {
                return data_;
            }

            /**
             * @brief Returns a const iterator past the last character.
             * @return Const iterator pointing past the last character.
             */
            [[nodiscard]] constexpr const_iterator cend() const noexcept
            {
                return data_ + size_;
            }

            /**
             * @brief Returns a reverse iterator to the last character.
             * @return Reverse iterator pointing to the last character.
             */
            [[nodiscard]] const_reverse_iterator rbegin() const noexcept
            {
                return const_reverse_iterator(end());
            }

            /**
             * @brief Returns a reverse iterator past the first character.
             * @return Reverse iterator pointing past the first character.
             */
            [[nodiscard]] const_reverse_iterator rend() const noexcept
            {
                return const_reverse_iterator(begin());
            }

            /**
             * @brief Returns a const reverse iterator to the last character.
             * @return Const reverse iterator pointing to the last character.
             */
            [[nodiscard]] const_reverse_iterator crbegin() const noexcept
            {
                return const_reverse_iterator(cend());
            }

            /**
             * @brief Returns a const reverse iterator past the first character.
             * @return Const reverse iterator pointing past the first character.
             */
            [[nodiscard]] const_reverse_iterator crend() const noexcept
            {
                return const_reverse_iterator(cbegin());
            }

#pragma endregion

#pragma region SIZE

            ////////////////////////////////////////////////////////////////
            // Size
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Returns the number of characters in the view.
             * @return Number of characters in the view.
             */
            [[nodiscard]] constexpr size_type size() const noexcept
            {
                return size_;
            }

            /**
             * @brief Returns the number of characters in the view.
             * @return Number of characters in the view.
             */
            [[nodiscard]] constexpr size_type length() const noexcept
            {
                return size_;
            }

            /**
             * @brief Returns the maximum representable view size.
             * @return Maximum number of characters representable by the view.
             */
            [[nodiscard]] constexpr size_type max_size() const noexcept
            {
                return npos;
            }

            /**
             * @brief Checks whether the view contains no characters.
             * @return Returns true if the view is empty, otherwise false.
             */
            [[nodiscard]] constexpr bool empty() const noexcept
            {
                return size_ == 0;
            }

#pragma endregion

#pragma region ELEMENT_ACCESS

            ////////////////////////////////////////////////////////////////
            // Element access
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Returns the character at the specified position.
             * @param pos Zero-based position of the character.
             * @return Character at the specified position.
             */
            [[nodiscard]] constexpr const_reference operator[](size_type pos) const noexcept
            {
                return data_[pos];
            }

            /**
             * @brief Returns the character at the specified position with bounds checking.
             * @param pos Zero-based position of the character.
             * @return Character at the specified position.
             * @throws std::out_of_range If pos is outside the view.
             */
            [[nodiscard]] constexpr const_reference at(size_type pos) const
            {
                return (pos >= size_) ? throw std::out_of_range("StringView::at") : data_[pos];
            }

            /**
             * @brief Returns the first character in the view.
             * @return Reference to the first character.
             */
            [[nodiscard]] constexpr const_reference front() const noexcept
            {
                return data_[0];
            }

            /**
             * @brief Returns the last character in the view.
             * @return Reference to the last character.
             */
            [[nodiscard]] constexpr const_reference back() const noexcept
            {
                return data_[size_ - 1];
            }

#pragma endregion

#pragma region MANIPULATION

            ////////////////////////////////////////////////////////////////
            // Manipulation
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Removes characters from the beginning of the view.
             * @param n Number of characters to remove.
             */
            constexpr void remove_prefix(size_type n) noexcept
            {
                data_ += n;
                size_ -= n;
            }

            /**
             * @brief Removes characters from the end of the view.
             * @param n Number of characters to remove.
             */
            constexpr void remove_suffix(size_type n) noexcept
            {
                size_ -= n;
            }

            /**
             * @brief Exchanges the contents of this view with another view.
             * @param other String view to exchange contents with.
             */
            constexpr void swap(StringView &other) noexcept
            {
                const data_type d = data_;
                data_             = other.data_;
                other.data_       = d;

                const size_type s = size_;
                size_             = other.size_;
                other.size_       = s;
            }

            /**
             * @brief Returns a view over a substring of the current view.
             * @param pos Starting position of the substring.
             * @param count Maximum number of characters in the substring.
             * @return String view containing the requested substring.
             * @throws std::out_of_range If pos is greater than the view size.
             */
            [[nodiscard]] constexpr StringView substr(size_type pos = 0, size_type count = npos) const
            {
                return (pos > size_) ? throw std::out_of_range("StringView::substr")
                                     : StringView(data_ + pos, (count < size_ - pos) ? count : size_ - pos);
            }

            /**
             * @brief Copies characters from the view into a destination buffer.
             * @param dest Destination buffer.
             * @param count Maximum number of characters to copy.
             * @param pos Starting position in the view.
             * @return Number of characters copied.
             * @throws std::out_of_range If pos is greater than the view size.
             */
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

#pragma endregion

#pragma region COMPARISON

            ////////////////////////////////////////////////////////////////
            // Comparison
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Compares the view with another string view.
             * @param v String view to compare against.
             * @return Negative value if this view precedes v, zero if they are equal, or positive value if this view
             * follows v.
             */
            [[nodiscard]] constexpr int compare(StringView v) const noexcept
            {
                const size_type rlen = (size_ < v.size_) ? size_ : v.size_;
                const int       cmp  = compare_n(data_, v.data_, rlen);
                return cmp != 0 ? cmp : (size_ < v.size_ ? -1 : (size_ > v.size_ ? 1 : 0));
            }

            /**
             * @brief Compares the view with a null-terminated character sequence.
             * @param s Null-terminated character sequence to compare against.
             * @return Negative value if this view precedes s, zero if they are equal, or positive value if this view
             * follows s.
             */
            [[nodiscard]] constexpr int compare(const char *s) const noexcept
            {
                return compare(StringView(s));
            }

            /**
             * @brief Compares the view with a string.
             * @param s String to compare against.
             * @return Negative value if this view precedes s, zero if they are equal, or positive value if this view
             * follows s.
             */
            [[nodiscard]] int compare(const std::string &s) const noexcept
            {
                return compare(StringView(s));
            }

            /**
             * @brief Checks whether two string views contain the same sequence of characters.
             * @param lhs Left-hand string view.
             * @param rhs Right-hand string view.
             * @return Returns true if both views contain the same characters, otherwise false.
             */
            [[nodiscard]] constexpr friend bool operator==(StringView lhs, StringView rhs) noexcept
            {
                return lhs.size_ == rhs.size_ && compare_n(lhs.data_, rhs.data_, lhs.size_) == 0;
            }

            /**
             * @brief Checks whether two string views contain different character sequences.
             * @param lhs Left-hand string view.
             * @param rhs Right-hand string view.
             * @return Returns true if the views differ, otherwise false.
             */
            [[nodiscard]] constexpr friend bool operator!=(StringView lhs, StringView rhs) noexcept
            {
                return !(lhs == rhs);
            }

            /**
             * @brief Checks whether one string view precedes another lexicographically.
             * @param lhs Left-hand string view.
             * @param rhs Right-hand string view.
             * @return Returns true if lhs is lexicographically less than rhs, otherwise false.
             */
            [[nodiscard]] constexpr friend bool operator<(StringView lhs, StringView rhs) noexcept
            {
                return lhs.compare(rhs) < 0;
            }

            /**
             * @brief Checks whether one string view precedes or equals another lexicographically.
             * @param lhs Left-hand string view.
             * @param rhs Right-hand string view.
             * @return True if lhs is lexicographically less than or equal to rhs, otherwise false.
             */
            [[nodiscard]] constexpr friend bool operator<=(StringView lhs, StringView rhs) noexcept
            {
                return lhs.compare(rhs) <= 0;
            }

            /**
             * @brief Checks whether one string view follows another lexicographically.
             * @param lhs Left-hand string view.
             * @param rhs Right-hand string view.
             * @return Returns true if lhs is lexicographically greater than rhs, otherwise false.
             */
            [[nodiscard]] constexpr friend bool operator>(StringView lhs, StringView rhs) noexcept
            {
                return lhs.compare(rhs) > 0;
            }

            /**
             * @brief Checks whether one string view follows or equals another lexicographically.
             * @param lhs Left-hand string view.
             * @param rhs Right-hand string view.
             * @return Returns true if lhs is lexicographically greater than or equal to rhs, otherwise false.
             */
            [[nodiscard]] constexpr friend bool operator>=(StringView lhs, StringView rhs) noexcept
            {
                return lhs.compare(rhs) >= 0;
            }

#pragma endregion

#pragma region CONVERSION

            ////////////////////////////////////////////////////////////////
            // Conversion
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Converts the view into an owning string.
             * @return String containing a copy of the characters in the view.
             */
            [[nodiscard]] explicit operator std::string() const
            {
                return size_ ? std::string(data_, size_) : std::string();
            }

#pragma endregion

        private:
#pragma region PRIVATE_METHODS

            ////////////////////////////////////////////////////////////////
            // Private Methods
            ////////////////////////////////////////////////////////////////

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

#pragma endregion

        private:
#pragma region FIELDS

            ////////////////////////////////////////////////////////////////
            // Fields
            ////////////////////////////////////////////////////////////////

            data_type data_;
            size_type size_;

#pragma endregion
        };
#pragma region IO

        ////////////////////////////////////////////////////////////////
        // IO
        ////////////////////////////////////////////////////////////////

        /**
         * @brief Writes the contents of a string view to an output stream.
         * @param os Output stream receiving the characters.
         * @param v String view to write.
         * @return Reference to the output stream.
         */
        inline std::ostream &operator<<(std::ostream &os, StringView v)
        {
            return os.write(v.data(), static_cast<std::streamsize>(v.size()));
        }

#pragma endregion
    } // namespace details
} // namespace ts

#endif // CPP_TREE_SITTER_STRING_VIEW_HPP
