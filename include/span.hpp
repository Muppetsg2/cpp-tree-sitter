#pragma once
#ifndef CPP_TREE_SITTER_SPAN_HPP
#define CPP_TREE_SITTER_SPAN_HPP

// STL
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <vector>

namespace ts
{
    namespace details
    {
        /**
         * @brief Non-owning view over a contiguous sequence of objects.
         * @details Minimal replacement of `std::span` (C++20) used when the standard library does not provide it. Only
         * the subset of the interface needed by the wrapper is implemented, with the same semantics as `std::span`.
         * @tparam T Element type. Use a const-qualified type for a read-only view.
         */
        template <typename T>
        class Span
        {
        public:
#pragma region TYPE_ALIASES

            ////////////////////////////////////////////////////////////////
            // Type Aliases
            ////////////////////////////////////////////////////////////////

            /// @brief Type of the viewed elements (may be const).
            using element_type = T;

            /// @brief Type of the viewed elements without cv-qualifiers.
            using value_type = typename std::remove_cv<T>::type;

            /// @brief Pointer to an element.
            using pointer = T *;

            /// @brief Pointer to a const element.
            using const_pointer = const T *;

            /// @brief Reference to an element.
            using reference = T &;

            /// @brief Reference to a const element.
            using const_reference = const T &;

            /// @brief Iterator over the elements.
            using iterator = T *;

            /// @brief Iterator over the const elements.
            using const_iterator = const T *;

            /// @brief Reverse iterator over the elements.
            using reverse_iterator = std::reverse_iterator<iterator>;

            /// @brief Reverse iterator over the const elements.
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

            /// @brief Type of sizes and indices.
            using size_type = std::size_t;

            /// @brief Type of iterator differences.
            using difference_type = std::ptrdiff_t;

#pragma endregion

#pragma region LIFECYCLE

            ////////////////////////////////////////////////////////////////
            // Lifecycle
            ////////////////////////////////////////////////////////////////

            /// @brief Creates an empty view.
            constexpr Span() noexcept : data_(nullptr), size_(0)
            {}

            /**
             * @brief Creates a view over a contiguous sequence.
             * @param data Pointer to the first element.
             * @param size Number of elements in the sequence.
             *
             * @note A null data pointer results in an empty view.
             */
            constexpr Span(pointer data, size_type size) noexcept : data_(data), size_(data ? size : 0)
            {}

            /**
             * @brief Creates a view over a C array.
             * @tparam N Number of elements in the array.
             * @param array Array to view.
             */
            template <std::size_t N>
            constexpr Span(T (&array)[N]) noexcept : data_(array), size_(N)
            {}

            /**
             * @brief Creates a view over the elements of a vector.
             * @tparam U Element type of the vector.
             * @tparam Alloc Allocator of the vector.
             * @param vector Vector whose storage is viewed. Its storage must outlive the view.
             */
            template <typename U,
                      typename Alloc,
                      typename = typename std::enable_if<std::is_convertible<U (*)[], T (*)[]>::value>::type>
            Span(std::vector<U, Alloc> &vector) noexcept : data_(vector.data()), size_(vector.size())
            {}

            /**
             * @brief Creates a read-only view over the elements of a vector.
             * @tparam U Element type of the vector.
             * @tparam Alloc Allocator of the vector.
             * @param vector Vector whose storage is viewed. Its storage must outlive the view.
             */
            template <typename U,
                      typename Alloc,
                      typename = typename std::enable_if<std::is_convertible<const U (*)[], T (*)[]>::value>::type,
                      typename = void>
            Span(const std::vector<U, Alloc> &vector) noexcept : data_(vector.data()), size_(vector.size())
            {}

            /**
             * @brief Converts a view over mutable elements into a view over const elements.
             * @tparam U Element type of the other view.
             * @param other View to convert.
             */
            template <typename U,
                      typename = typename std::enable_if<std::is_convertible<U (*)[], T (*)[]>::value>::type>
            constexpr Span(const Span<U> &other) noexcept : data_(other.data()), size_(other.size())
            {}

#pragma endregion

#pragma region DATA

            ////////////////////////////////////////////////////////////////
            // Data
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Returns a pointer to the first element.
             * @return Pointer to the first element.
             */
            constexpr pointer data() const noexcept
            {
                return data_;
            }

            /**
             * @brief Returns an iterator to the first element.
             * @return Iterator to the first element.
             */
            constexpr iterator begin() const noexcept
            {
                return data_;
            }

            /**
             * @brief Returns an iterator past the last element.
             * @return Iterator past the last element.
             */
            constexpr iterator end() const noexcept
            {
                return data_ + size_;
            }

            /**
             * @brief Returns a reverse iterator to the last element.
             * @return Reverse iterator to the last element.
             */
            reverse_iterator rbegin() const noexcept
            {
                return reverse_iterator(end());
            }

            /**
             * @brief Returns a reverse iterator before the first element.
             * @return Reverse iterator before the first element.
             */
            reverse_iterator rend() const noexcept
            {
                return reverse_iterator(begin());
            }

#pragma endregion

#pragma region SIZE

            ////////////////////////////////////////////////////////////////
            // Size
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Returns the number of elements in the view.
             * @return Number of elements in the view.
             */
            constexpr size_type size() const noexcept
            {
                return size_;
            }

            /**
             * @brief Returns the size of the viewed sequence in bytes.
             * @return Size of the viewed sequence in bytes.
             */
            constexpr size_type size_bytes() const noexcept
            {
                return size_ * sizeof(T);
            }

            /**
             * @brief Returns whether the view contains no elements.
             * @return Returns true if the view contains no elements, false otherwise.
             */
            constexpr bool empty() const noexcept
            {
                return size_ == 0;
            }

#pragma endregion

#pragma region ELEMENT_ACCESS

            ////////////////////////////////////////////////////////////////
            // Element Access
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Returns the element at the specified position.
             * @param pos Zero-based position of the element. Must be smaller than `size()`.
             * @return Reference to the element at the specified position.
             */
            constexpr reference operator[](size_type pos) const noexcept
            {
                return data_[pos];
            }

            /**
             * @brief Returns the first element. The view must not be empty.
             * @return Reference to the first element.
             */
            constexpr reference front() const noexcept
            {
                return data_[0];
            }

            /**
             * @brief Returns the last element. The view must not be empty.
             * @return Reference to the last element.
             */
            constexpr reference back() const noexcept
            {
                return data_[size_ - 1];
            }

#pragma endregion

#pragma region SUBVIEWS

            ////////////////////////////////////////////////////////////////
            // Subviews
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Returns a view over the first `count` elements.
             * @param count Number of elements. Must not exceed `size()`.
             * @return View over the first `count` elements.
             */
            constexpr Span first(size_type count) const noexcept
            {
                return Span(data_, count);
            }

            /**
             * @brief Returns a view over the last `count` elements.
             * @param count Number of elements. Must not exceed `size()`.
             * @return View over the last `count` elements.
             */
            constexpr Span last(size_type count) const noexcept
            {
                return Span(data_ + (size_ - count), count);
            }

            /**
             * @brief Returns a view over a part of this view.
             * @param offset Index of the first element. Must not exceed `size()`.
             * @param count Number of elements, or `size_type(-1)` to view everything up to the end.
             * @return View over the selected elements.
             */
            constexpr Span subspan(size_type offset, size_type count = size_type(-1)) const noexcept
            {
                return Span(data_ + offset, count == size_type(-1) ? size_ - offset : count);
            }

#pragma endregion

        private:
#pragma region FIELDS

            ////////////////////////////////////////////////////////////////
            // Fields
            ////////////////////////////////////////////////////////////////

            /// @brief First viewed element (may be null for an empty view).
            pointer data_;

            /// @brief Number of viewed elements.
            size_type size_;

#pragma endregion
        };
    } // namespace details
} // namespace ts

#endif // CPP_TREE_SITTER_SPAN_HPP
