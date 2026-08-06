#pragma once
#ifndef CPP_TREE_SITTER_BYTE_VIEW_HPP
#define CPP_TREE_SITTER_BYTE_VIEW_HPP

// STL
#include <cstddef>
#include <cstdint>

namespace ts
{
    namespace details
    {
        /// @brief Non-owning read-only view over a contiguous byte sequence.
        class ByteView
        {
        public:
#pragma region TYPE_ALIASES

            ////////////////////////////////////////////////////////////////
            // Type Aliases
            ////////////////////////////////////////////////////////////////

            using value_type     = std::uint8_t;
            using data_type      = const value_type *;
            using pointer        = data_type;
            using const_pointer  = data_type;
            using const_iterator = data_type;
            using size_type      = std::size_t;

#pragma endregion

#pragma region CONSTANTS

            ////////////////////////////////////////////////////////////////
            // Constants
            ////////////////////////////////////////////////////////////////

            /// @brief Special value representing an invalid or unavailable position.
            static constexpr size_type npos = size_type(-1);

#pragma endregion

#pragma region LIFECYCLE

            ////////////////////////////////////////////////////////////////
            // Lifecycle
            ////////////////////////////////////////////////////////////////

            /// @brief Creates an empty byte view.
            constexpr ByteView() noexcept : data_(nullptr), size_(0)
            {}

            /**
             * @brief Creates a view over a contiguous byte sequence.
             * @param data Pointer to the first byte.
             * @param size Number of bytes in the sequence.
             *
             * @note A null data pointer results in an empty view.
             */
            constexpr ByteView(data_type data, size_type size) noexcept : data_(data), size_(data ? size : 0)
            {}

#pragma endregion

#pragma region DATA

            ////////////////////////////////////////////////////////////////
            // Data
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Returns a pointer to the first byte.
             * @return Pointer to the first byte.
             */
            [[nodiscard]] constexpr const_pointer data() const noexcept
            {
                return data_;
            }

            /**
             * @brief Returns an iterator to the first byte.
             * @return Iterator to the first byte.
             */
            [[nodiscard]] constexpr const_iterator begin() const noexcept
            {
                return data_;
            }

            /**
             * @brief Returns an iterator past the last byte.
             * @return Iterator past the last byte.
             */
            [[nodiscard]] constexpr const_iterator end() const noexcept
            {
                return data_ + size_;
            }

            /**
             * @brief Returns a const iterator to the first byte.
             * @return Const iterator to the first byte.
             */
            [[nodiscard]] constexpr const_iterator cbegin() const noexcept
            {
                return data_;
            }

            /**
             * @brief Returns a const iterator past the last byte.
             * @return Const iterator past the last byte.
             */
            [[nodiscard]] constexpr const_iterator cend() const noexcept
            {
                return data_ + size_;
            }

#pragma endregion

#pragma region SIZE

            ////////////////////////////////////////////////////////////////
            // Size
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Returns the number of bytes in the view.
             * @return Number of bytes in the view.
             */
            [[nodiscard]] constexpr size_type size() const noexcept
            {
                return size_;
            }

            /**
             * @brief Returns the maximum representable view size.
             * @return Maximum representable view size.
             */
            [[nodiscard]] constexpr size_type max_size() const noexcept
            {
                return npos;
            }

            /**
             * @brief Returns whether the view contains no bytes.
             * @return Returns true if the view contains no bytes, false otherwise.
             */
            [[nodiscard]] constexpr bool empty() const noexcept
            {
                return size_ == 0;
            }

#pragma endregion

#pragma region ELEMENT_ACCESS

            ////////////////////////////////////////////////////////////////
            // Element Access
            ////////////////////////////////////////////////////////////////

            /**
             * @brief Returns the byte at the specified position.
             * @param pos Zero-based position of the byte.
             * @return The byte at the specified position.
             */
            [[nodiscard]] constexpr value_type operator[](size_type pos) const noexcept
            {
                return data_[pos];
            }

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
    } // namespace details
} // namespace ts

#endif // CPP_TREE_SITTER_BYTE_VIEW_HPP
