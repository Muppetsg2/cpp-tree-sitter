#pragma once
#ifndef CPP_TREE_SITTER_BYTE_VIEW_HPP
#define CPP_TREE_SITTER_BYTE_VIEW_HPP

#include <cstddef>
#include <cstdint>

namespace ts
{
    namespace details
    {
        class ByteView
        {
        public:
            using value_type     = std::uint8_t;
            using data_type      = const value_type *;
            using pointer        = data_type;
            using const_pointer  = data_type;
            using const_iterator = data_type;
            using size_type      = std::size_t;

            static constexpr size_type npos = size_type(-1);

            ////////////////////////////////////////////////////////////////
            // Lifecycle
            ////////////////////////////////////////////////////////////////

            constexpr ByteView() noexcept : data_(nullptr), size_(0)
            {}

            constexpr ByteView(data_type data, size_type size) noexcept : data_(data), size_(data ? size : 0)
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

            ////////////////////////////////////////////////////////////////
            // Size
            ////////////////////////////////////////////////////////////////

            [[nodiscard]] constexpr size_type size() const noexcept
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

            [[nodiscard]] constexpr value_type operator[](size_type pos) const noexcept
            {
                return data_[pos];
            }

        private:
            data_type data_;
            size_type size_;
        };
    } // namespace details
} // namespace ts

#endif // CPP_TREE_SITTER_BYTE_VIEW_HPP
