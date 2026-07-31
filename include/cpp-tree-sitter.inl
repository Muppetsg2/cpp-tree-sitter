namespace ts
{
    namespace details
    {
        /////////////////////////////////////////////////////////////////////////////
        // QueryErrorHelper
        /////////////////////////////////////////////////////////////////////////////

        CPP_TS_API std::string QueryErrorHelper::format_error(std::uint32_t error_offset, const TSQueryError &error)
        {
            std::string type_name;
            switch (error)
            {
                case TSQueryErrorSyntax:
                    type_name = "Syntax";
                    break;
                case TSQueryErrorNodeType:
                    type_name = "Node Type";
                    break;
                case TSQueryErrorField:
                    type_name = "Field";
                    break;
                case TSQueryErrorCapture:
                    type_name = "Capture";
                    break;
                case TSQueryErrorStructure:
                    type_name = "Structure";
                    break;
                case TSQueryErrorLanguage:
                    type_name = "Language";
                    break;
                default:
                    type_name = "Unknown";
                    break;
            }
            return "Tree-sitter Query Error at offset " + std::to_string(error_offset) + " (Type: " + type_name + ")";
        }

        CPP_TS_API void QueryErrorHelper::validate(std::uint32_t error_offset, TSQueryError &error)
        {
            if (error == TSQueryErrorNone)
            {
                return;
            }
            throw std::runtime_error(format_error(error_offset, error));
        }

#if defined(CPP_TS_FEATURE_WASM)

        /////////////////////////////////////////////////////////////////////////////
        // WasmErrorHelper
        /////////////////////////////////////////////////////////////////////////////

        CPP_TS_API std::string WasmErrorHelper::format_error(const TSWasmError &error)
        {
            std::string kind_name;
            switch (error.kind)
            {
                case TSWasmErrorKindParse:
                    kind_name = "Parse";
                    break;
                case TSWasmErrorKindCompile:
                    kind_name = "Compile";
                    break;
                case TSWasmErrorKindInstantiate:
                    kind_name = "Instantiate";
                    break;
                case TSWasmErrorKindAllocate:
                    kind_name = "Allocate";
                    break;
                default:
                    kind_name = "Unknown";
                    break;
            }

            std::string message = error.message ? error.message : "No message";

            return "Tree-sitter Wasm Error [" + kind_name + "]: " + message;
        }

        CPP_TS_API void WasmErrorHelper::validate(TSWasmError &error)
        {
            if (error.kind == TSWasmErrorKindNone)
            {
                return;
            }

            std::string err_msg = format_error(error);

            if (error.message)
            {
                std::free(error.message);
            }

            throw std::runtime_error(err_msg);
        }

        CPP_TS_API std::string get_grammar_name_from_file(StringViewParameter path)
        {
#if CPP_TS_HAS_CXX17
            std::string name = std::filesystem::path(path).stem().string();
#else
            std::string filename = std::string(path);
            std::size_t slash    = filename.find_last_of("/\\");
            std::size_t dot      = filename.find_last_of('.');

            std::size_t start = (slash == std::string::npos) ? 0 : slash + 1;
            std::size_t end   = (dot == std::string::npos || dot < start) ? filename.length() : dot;

            std::string name = filename.substr(start, end - start);
#endif // CPP_TS_HAS_CXX17
            const std::string prefix = "tree-sitter-";

            if (name.compare(0, prefix.length(), prefix) == 0)
            {
                return name.substr(prefix.length());
            }

            return name;
        }

#endif // defined(CPP_TS_FEATURE_WASM)

        CPP_TS_API StringViewReturn make_view(const char *str) noexcept
        {
            return str ? StringViewReturn(str) : StringViewReturn();
        }

        CPP_TS_API StringViewReturn make_view(const char *str, std::size_t length) noexcept
        {
            return (str && length > 0) ? StringViewReturn(str, length) : StringViewReturn();
        }
    } // namespace details

    /////////////////////////////////////////////////////////////////////////////
    // Point
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API void Point::edit(const InputEdit &edit, std::uint32_t &byte_offset)
    {
        edit.validate();

        TSPoint     raw_point = static_cast<TSPoint>(*this);
        TSInputEdit raw_edit  = static_cast<TSInputEdit>(edit);

        ts_point_edit(&raw_point, &byte_offset, &raw_edit);

        row    = raw_point.row;
        column = raw_point.column;
    }

    /////////////////////////////////////////////////////////////////////////////
    // Range
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API void Range::edit(const InputEdit &edit)
    {
        edit.validate();

        TSRange     raw_range = static_cast<TSRange>(*this);
        TSInputEdit raw_edit  = static_cast<TSInputEdit>(edit);

        ts_range_edit(&raw_range, &raw_edit);

        point.start = raw_range.start_point;
        point.end   = raw_range.end_point;
        byte.start  = raw_range.start_byte;
        byte.end    = raw_range.end_byte;
    }

#if CPP_TS_HAS_CXX17
    CPP_TS_API thread_local const Input *Input::current_input_ptr = nullptr;
#endif // CPP_TS_HAS_CXX17

    CPP_TS_API Input::Input()
        : read_(nullptr), decode_(nullptr), encoding_(InputEncoding::UTF8), impl_{ std::make_unique<TSInput>() }
    {
        impl_->payload = const_cast<Input *>(this);
        impl_->read    = read_proxy;
    }

    CPP_TS_API Input::Input(ReadFunction read, InputEncoding encoding)
        : read_(read ? std::move(read) : nullptr)
        , decode_(nullptr)
        , encoding_(encoding == InputEncoding::Custom ? InputEncoding::UTF8 : encoding)
        , impl_{ std::make_unique<TSInput>() }
    {
        impl_->payload = const_cast<Input *>(this);
        impl_->read    = read_proxy;
    }

    CPP_TS_API Input::Input(ReadFunction read, ts::DecodeFunction decode)
        : read_(read ? std::move(read) : nullptr)
        , decode_(decode ? std::move(decode) : nullptr)
        , encoding_(decode ? InputEncoding::Custom : InputEncoding::UTF8)
        , impl_{ std::make_unique<TSInput>() }
    {
        impl_->payload = const_cast<Input *>(this);
        impl_->read    = read_proxy;
    }

    CPP_TS_API Input::Input(Input &&other) noexcept
        : read_(std::move(other.read_))
        , decode_(std::move(other.decode_))
        , encoding_(other.encoding_)
        , impl_(std::move(other.impl_))
    {
        if (impl_)
        {
            impl_->payload = this;
        }
    }

    CPP_TS_API const char *Input::read_proxy(void          *payload,
                                             std::uint32_t  byte_index,
                                             TSPoint        position,
                                             std::uint32_t *bytes_read)
    {
        auto *self = static_cast<Input *>(payload);
        if (self && self->read_)
        {
            details::StringViewReturn result = self->read_(byte_index, position, bytes_read);
            return result.data();
        }
        *bytes_read = 0;
        return nullptr;
    }

    CPP_TS_API std::uint32_t Input::decode_proxy(const std::uint8_t *string,
                                                 std::uint32_t       length,
                                                 std::int32_t       *code_point) noexcept
    {
#if CPP_TS_HAS_CXX17
        const Input *ptr = current_input_ptr;
#else
        const Input *ptr = current_ptr();
#endif
        if (ptr && ptr->decode_)
        {
            return ptr->decode_({ string, length }, code_point);
        }
        return 0;
    }

    CPP_TS_API Input &Input::operator=(Input &&other) noexcept
    {
        if (this != &other)
        {
            read_     = std::move(other.read_);
            decode_   = std::move(other.decode_);
            encoding_ = other.encoding_;
            impl_     = std::move(other.impl_);
            if (impl_)
            {
                impl_->payload = this;
            }
        }
        return *this;
    }

    CPP_TS_API Input::operator TSInput() const
    {
        TSInput c_input{};
        c_input.payload  = const_cast<Input *>(this);
        c_input.read     = read_proxy;
        c_input.encoding = static_cast<TSInputEncoding>(encoding_);
        c_input.decode   = decode_ ? decode_proxy : nullptr;
        return c_input;
    }

    CPP_TS_API Input::operator TSInput *() const noexcept
    {
        impl_->encoding = static_cast<TSInputEncoding>(encoding_);
        impl_->decode   = decode_ ? decode_proxy : nullptr;
        return impl_.get();
    }

    /////////////////////////////////////////////////////////////////////////////
    // LanguageView
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API details::StringViewReturn LanguageView::getSymbolName(Symbol symbol) const
    {
        return isValid() ? details::make_view(ts_language_symbol_name(impl_, symbol)) : details::StringViewReturn("");
    }

    CPP_TS_API SymbolType LanguageView::getSymbolType(Symbol symbol) const
    {
        return isValid() ? static_cast<SymbolType>(ts_language_symbol_type(impl_, symbol)) : SymbolType::TypeRegular;
    }

    CPP_TS_API Symbol LanguageView::getSymbolForName(details::StringViewParameter name, bool isNamed) const
    {
        if (!isValid())
        {
            return 0;
        }

        if (name.size() > std::numeric_limits<std::uint32_t>::max())
        {
            throw std::length_error("Tree-sitter: Input name exceeds maximum size of 4GB");
        }

        return ts_language_symbol_for_name(impl_, name.data(), static_cast<std::uint32_t>(name.size()), isNamed);
    }

    CPP_TS_API details::StringViewReturn LanguageView::getFieldNameForID(FieldID id) const
    {
        return isValid() ? details::make_view(ts_language_field_name_for_id(impl_, id)) : details::StringViewReturn("");
    }

    CPP_TS_API FieldID LanguageView::getFieldIDForName(details::StringViewParameter name) const
    {
        if (!isValid())
        {
            return 0;
        }

        if (name.size() > std::numeric_limits<std::uint32_t>::max())
        {
            throw std::length_error("Tree-sitter: Input name exceeds maximum size of 4GB");
        }

        return ts_language_field_id_for_name(impl_, name.data(), static_cast<std::uint32_t>(name.size()));
    }

    CPP_TS_API std::vector<details::StringViewReturn> LanguageView::getAllFieldsNames() const
    {
        if (!isValid())
        {
            return {};
        }

        const TSLanguage   *raw_lang = impl_;
        const std::uint32_t count    = ts_language_field_count(raw_lang);

        if (count == 0)
        {
            return {};
        }

        std::vector<details::StringViewReturn> names;
        names.reserve(static_cast<std::size_t>(count));

        for (std::uint32_t i = 1; i <= count; ++i)
        {
            names.emplace_back(details::make_view(ts_language_field_name_for_id(raw_lang, i)));
        }

        return names;
    }

    CPP_TS_API std::vector<Symbol> LanguageView::getAllSuperTypes() const noexcept
    {
        if (!isValid())
        {
            return {};
        }

        std::uint32_t   count = 0;
        const TSSymbol *array = ts_language_supertypes(impl_, &count);
        if (!array || count == 0)
        {
            return {};
        }

        return std::vector<Symbol>(array, array + count);
    }

    CPP_TS_API std::vector<Symbol> LanguageView::getAllSubTypesForSuperType(Symbol supertype) const noexcept
    {
        if (!isValid())
        {
            return {};
        }

        std::uint32_t   count = 0;
        const TSSymbol *array = ts_language_subtypes(impl_, supertype, &count);
        if (!array || count == 0)
        {
            return {};
        }

        return std::vector<Symbol>(array, array + count);
    }

    CPP_TS_API LookaheadIterator LanguageView::getLookaheadIterator(StateID state) const
    {
        if (!isValid())
        {
            throw std::logic_error("Tree-sitter: Can't create LookaheadIterator for invalid Language");
        }

        return LookaheadIterator(impl_, state);
    }

    CPP_TS_API details::StringViewReturn LanguageView::getName() const
    {
        return isValid() ? details::make_view(ts_language_name(impl_)) : details::StringViewReturn("");
    }

#if CPP_TS_HAS_CXX17
    CPP_TS_API std::optional<LanguageMetadata> LanguageView::getMetadata() const noexcept
    {
        static constexpr std::optional<LanguageMetadata> null_value = std::nullopt;
#else
    CPP_TS_API LanguageMetadata LanguageView::getMetadata() const noexcept
    {
        static constexpr LanguageMetadata null_value = { 0, 0, 0 };
#endif // CPP_TS_HAS_CXX17

        if (!isValid())
        {
            return null_value;
        }

        const TSLanguageMetadata *metadata = ts_language_metadata(impl_);
        if (!metadata)
        {
            return null_value;
        }

        return LanguageMetadata{ metadata->major_version, metadata->minor_version, metadata->patch_version };
    }

    /////////////////////////////////////////////////////////////////////////////
    // Language
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API Language &Language::operator=(const Language &other) noexcept
    {
        if (this != &other)
        {
            impl_.reset(other.impl_.get() != nullptr ? ts_language_copy(other.impl_.get()) : nullptr);
        }
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////
    // Node
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API details::StringViewReturn Node::getSourceRange(details::StringViewParameter source) const
    {
        if (isNull())
        {
            return {};
        }

        Extent<std::uint32_t> extents = getByteRange();
        std::size_t           end     = static_cast<std::size_t>(extents.end);
        if (end > source.size())
        {
            return "";
        }
        std::size_t start = static_cast<std::size_t>(extents.start);
        return source.substr(start, end - start);
    }

    CPP_TS_API std::string Node::getSourceText(details::StringViewParameter source) const
    {
        return isNull() ? "" : std::string(getSourceRange(source));
    }

    CPP_TS_API Node Node::getChildByType(details::StringViewParameter name) const
    {
        if (name.size() > std::numeric_limits<std::uint32_t>::max())
        {
            throw std::length_error("Tree-sitter: Input name exceeds maximum size of 4GB");
        }

        if (isNull())
        {
            return null();
        }

        LanguageView lang    = getLanguage();
        bool         isNamed = true;
        Symbol       sym     = lang.getSymbolForName(name, isNamed);

        if (sym == 0)
        {
            isNamed = false;
            sym     = lang.getSymbolForName(name, isNamed);
        }

        if (sym == 0)
        {
            return null();
        }

        if (isNamed)
        {
            for (Node child = getFirstNamedChild(); !child.isNull(); child = child.getNextNamedSibling())
            {
                if (child.getSymbol() == sym)
                {
                    return child;
                }
            }
        }
        else
        {
            for (Node child = getFirstChild(); !child.isNull(); child = child.getNextSibling())
            {
                if (child.getSymbol() == sym)
                {
                    return child;
                }
            }
        }

        return null();
    }

    CPP_TS_API Node Node::getChildBySymbol(Symbol sym) const noexcept
    {
        if (isNull())
        {
            return null();
        }

        for (Node child = getFirstChild(); !child.isNull(); child = child.getNextSibling())
        {
            if (child.getSymbol() == sym)
            {
                return child;
            }
        }
        return null();
    }

    CPP_TS_API Node Node::getNamedChildByType(details::StringViewParameter name) const
    {
        if (name.size() > std::numeric_limits<std::uint32_t>::max())
        {
            throw std::length_error("Tree-sitter: Input name exceeds maximum size of 4GB");
        }

        if (isNull())
        {
            return null();
        }

        LanguageView lang = getLanguage();
        Symbol       sym  = lang.getSymbolForName(name, true);

        if (sym == 0)
        {
            return null();
        }

        for (Node child = getFirstNamedChild(); !child.isNull(); child = child.getNextNamedSibling())
        {
            if (child.getSymbol() == sym)
            {
                return child;
            }
        }
        return null();
    }

    CPP_TS_API Node Node::getNamedChildBySymbol(Symbol sym) const noexcept
    {
        if (isNull())
        {
            return null();
        }

        for (Node child = getFirstNamedChild(); !child.isNull(); child = child.getNextNamedSibling())
        {
            if (child.getSymbol() == sym)
            {
                return child;
            }
        }
        return null();
    }

    CPP_TS_API Node Node::getChildByFieldName(details::StringViewParameter name) const
    {
        if (name.size() > std::numeric_limits<std::uint32_t>::max())
        {
            throw std::length_error("Tree-sitter: Input name exceeds maximum size of 4GB");
        }
        return isNull()
                     ? null()
                     : Node{ ts_node_child_by_field_name(impl_, name.data(), static_cast<std::uint32_t>(name.size())) };
    }

    CPP_TS_API void Node::edit(const InputEdit &edit)
    {
        if (isNull())
        {
            throw std::logic_error("Tree-sitter: Couldn't change null Node");
        }

        edit.validate();

        TSInputEdit raw_edit = static_cast<TSInputEdit>(edit);
        ts_node_edit(&impl_, &raw_edit);
    }

    CPP_TS_API TreeCursor Node::getCursor() const noexcept
    {
        return TreeCursor(impl_);
    }

    /////////////////////////////////////////////////////////////////////////////
    // Tree
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API Tree::Tree(const Tree &other) noexcept
    {
        is_valid_ = other.is_valid_;
        if (is_valid_ && other.impl_.get() != nullptr)
        {
            impl_.reset(ts_tree_copy(other.impl_.get()));
        }
        else
        {
            is_valid_ = false;
            impl_.reset();
        }
    }

    CPP_TS_API Tree::Tree(Tree &&other) noexcept : impl_{ std::move(other.impl_) }, is_valid_{ other.is_valid_ }
    {
        other.is_valid_ = false;
    }

    CPP_TS_API void Tree::edit(const InputEdit &edit)
    {
        if (!isValid())
        {
            throw std::logic_error("Tree-sitter: Couldn't change invalid Tree");
        }

        edit.validate();

        TSInputEdit raw_edit = static_cast<TSInputEdit>(edit);
        ts_tree_edit(impl_.get(), &raw_edit);
    }

    CPP_TS_API std::vector<Range> Tree::getIncludedRanges() const
    {
        if (!isValid())
        {
            return {};
        }

        std::uint32_t count = 0;
        TSRange      *array = ts_tree_included_ranges(impl_.get(), &count);
        if (!array)
        {
            return {};
        }

        if (count == 0)
        {
            std::free(array);
            return {};
        }

        std::vector<Range> vec;
        vec.reserve(count);
        for (std::uint32_t i = 0; i < count; ++i)
        {
            vec.emplace_back(array[i]);
        }

        std::free(array);
        return vec;
    }

    CPP_TS_API std::vector<Range> Tree::getChangedRanges(const Tree &old_tree, const Tree &new_tree)
    {
        if (!old_tree.isValid() || !new_tree.isValid())
        {
            return {};
        }

        std::uint32_t count = 0;
        TSRange      *array = ts_tree_get_changed_ranges(static_cast<const TSTree *>(old_tree),
                                                         static_cast<const TSTree *>(new_tree),
                                                         &count);
        if (!array)
        {
            return {};
        }

        if (count == 0)
        {
            std::free(array);
            return {};
        }

        std::vector<Range> vec;
        vec.reserve(count);
        for (std::uint32_t i = 0; i < count; ++i)
        {
            vec.emplace_back(array[i]);
        }

        std::free(array);
        return vec;
    }

    CPP_TS_API void Tree::printDotGraph(details::StringViewParameter path)
    {
        if (!isValid() || path.empty())
        {
            return;
        }

        std::string path_str(path.data(), path.size());

#if defined(_WIN32)
        FILE *file = nullptr;
        fopen_s(&file, path_str.c_str(), "w");
#else
        FILE *file = fopen(path_str.c_str(), "w");
#endif // defined(_WIN32)
        printDotGraph(file);

        if (file != nullptr)
        {
            fclose(file);
        }
    }

    CPP_TS_API Tree &Tree::operator=(const Tree &other)
    {
        if (this != &other)
        {
            is_valid_ = other.is_valid_;
            if (is_valid_ && other.impl_.get() != nullptr)
            {
                impl_.reset(ts_tree_copy(other.impl_.get()));
            }
            else
            {
                is_valid_ = false;
                impl_.reset();
            }
        }
        return *this;
    }

    CPP_TS_API Tree &Tree::operator=(Tree &&other) noexcept
    {
        if (this != &other)
        {
            impl_           = std::move(other.impl_);
            is_valid_       = other.is_valid_;
            other.is_valid_ = false;
        }
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////
    // Parser
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API Parser::Parser(LanguageView language) : impl_{ ts_parser_new(), ts_parser_delete }
    {
        if (!language.isValid())
        {
            throw std::runtime_error("Tree-sitter: Passed Language for Parser was invalid");
        }

#if CPP_TS_HAS_CXX23
        if (auto res = setLanguage(language); !res.has_value())
        {
            throw std::runtime_error(res.error());
        }
#else
        if (!setLanguage(language))
        {
            throw std::runtime_error("Tree-sitter: Couldn't set language (version mismatch)");
        }
#endif // CPP_TS_HAS_CXX23
    }

#if defined(CPP_TS_FEATURE_WASM)
    CPP_TS_API Parser::Parser(LanguageView language, WasmStore &store) : impl_{ ts_parser_new(), ts_parser_delete }
    {
        if (!language.isValid())
        {
            throw std::runtime_error("Tree-sitter: Passed Language for Parser was invalid");
        }

        setWasmStore(store);

#if CPP_TS_HAS_CXX23
        if (auto res = setLanguage(language); !res.has_value())
        {
            throw std::runtime_error(res.error());
        }
#else
        if (!setLanguage(language))
        {
            throw std::runtime_error("Tree-sitter: Couldn't set language (version mismatch)");
        }
#endif // CPP_TS_HAS_CXX23
    }
#endif // defined(CPP_TS_FEATURE_WASM)

    CPP_TS_API Parser::Parser(Parser &&other) noexcept
        : impl_{ std::move(other.impl_) }
        , current_logger_{ std::move(other.current_logger_) }
        , language_set_{ other.language_set_ }
#if defined(CPP_TS_FEATURE_WASM)
        , wasm_store_set_{ other.wasm_store_set_ }
#endif // defined(CPP_TS_FEATURE_WASM)
    {
        other.language_set_ = false;

#if defined(CPP_TS_FEATURE_WASM)
        other.wasm_store_set_ = false;
#endif // defined(CPP_TS_FEATURE_WASM)

        if (current_logger_)
        {
            setLogger(current_logger_);
        }
    }

#if CPP_TS_HAS_CXX23
    CPP_TS_API std::expected<Parser, std::string> Parser::create(LanguageView language) noexcept
    {
        if (!language.isValid())
        {
            return std::unexpected("Tree-sitter: Passed Language for Parser was invalid");
        }

        Parser parser;
        if (auto res = parser.setLanguage(language); !res.has_value())
        {
            return std::unexpected(res.error());
        }
        return parser;
    }

#if defined(CPP_TS_FEATURE_WASM)
    CPP_TS_API std::expected<Parser, std::string> Parser::create(LanguageView language, WasmStore &store) noexcept
    {
        if (!language.isValid())
        {
            return std::unexpected("Tree-sitter: Passed Language for Parser was invalid");
        }

        Parser parser;
        parser.setWasmStore(store);
        if (auto res = parser.setLanguage(language); !res.has_value())
        {
            store = parser.takeWasmStore();
            return std::unexpected(res.error());
        }

        return parser;
    }
#endif // defined(CPP_TS_FEATURE_WASM)
#endif // CPP_TS_HAS_CXX23

#if CPP_TS_HAS_CXX23
    CPP_TS_API std::expected<bool, std::string> Parser::setLanguage(LanguageView language)
    {
#if defined(CPP_TS_FEATURE_WASM)
        if (language.isWasm() && !hasWasmStore())
        {
            return std::unexpected("Tree-sitter: Parser needs WasmStore to process Wasm Language");
        }
#endif // defined(CPP_TS_FEATURE_WASM)
        language_set_ = ts_parser_set_language(impl_.get(), language);
        if (!language_set_)
        {
            return std::unexpected("Tree-sitter: Language version mismatch (ABI incompatibility)");
        }
        return language_set_;
    }
#else
    CPP_TS_API bool Parser::setLanguage(LanguageView language)
#if defined(CPP_TS_FEATURE_WASM)
    {
        if (language.isWasm() && !hasWasmStore())
        {
            throw std::logic_error("Tree-sitter: Parser needs WasmStore to process Wasm Language");
        }
#else
            noexcept
    {
#endif // defined(CPP_TS_FEATURE_WASM)
        language_set_ = ts_parser_set_language(impl_.get(), language);
        return language_set_;
    }
#endif // CPP_TS_HAS_CXX23

#if CPP_TS_HAS_CXX20
    CPP_TS_API bool Parser::setIncludedRanges(std::span<const Range> ranges)
#else
    CPP_TS_API bool Parser::setIncludedRanges(const std::vector<Range> &ranges)
#endif // CPP_TS_HAS_CXX20
    {
        if (ranges.empty())
        {
            return ts_parser_set_included_ranges(impl_.get(), nullptr, 0);
        }

        std::vector<TSRange> c_ranges;
        c_ranges.reserve(ranges.size());
        for (const auto &r : ranges)
        {
            c_ranges.push_back(static_cast<TSRange>(r));
        }

        std::sort(c_ranges.begin(),
                  c_ranges.end(),
                  [](const TSRange &a, const TSRange &b) { return a.start_byte < b.start_byte; });

        std::vector<TSRange> merged;
        merged.reserve(c_ranges.size());
        merged.push_back(c_ranges[0]);

        for (std::size_t i = 1; i < c_ranges.size(); ++i)
        {
            TSRange &last    = merged.back();
            TSRange &current = c_ranges[i];

            if (current.start_byte <= last.end_byte)
            {
                if (current.end_byte > last.end_byte)
                {
                    last.end_byte  = current.end_byte;
                    last.end_point = current.end_point;
                }
            }
            else
            {
                merged.push_back(current);
            }
        }

        if (merged.size() > std::numeric_limits<std::uint32_t>::max())
        {
            throw std::length_error(
                    "Tree-sitter: Ranges size exceeds maximum size. (Max size is equal to UINT32_MAX value)");
        }

        return ts_parser_set_included_ranges(impl_.get(), merged.data(), static_cast<std::uint32_t>(merged.size()));
    }

#if defined(CPP_TS_FEATURE_WASM)
    CPP_TS_API void Parser::setWasmStore(WasmStore &store) noexcept
    {
        ts_parser_set_wasm_store(impl_.get(), store);
        wasm_store_set_ = true;
        store.setInvalid();
    }
#endif // defined(CPP_TS_FEATURE_WASM)

    CPP_TS_API std::vector<Range> Parser::getIncludedRanges() const noexcept
    {
        std::uint32_t  count = 0;
        const TSRange *array = ts_parser_included_ranges(impl_.get(), &count);
        if (!array || count == 0)
        {
            return {};
        }

        std::vector<Range> vec;
        vec.reserve(count);
        for (std::uint32_t i = 0; i < count; ++i)
        {
            vec.emplace_back(array[i]);
        }
        return vec;
    }

#if defined(CPP_TS_FEATURE_WASM)
    CPP_TS_API WasmStore Parser::takeWasmStore() noexcept
    {
        LanguageView current = getCurrentLanguage();
        if (current.isValid() && current.isWasm())
        {
            language_set_ = false;
        }
        TSWasmStore *raw_store = ts_parser_take_wasm_store(impl_.get());
        wasm_store_set_        = false;
        return WasmStore{ raw_store };
    }
#endif // defined(CPP_TS_FEATURE_WASM)

    CPP_TS_API Tree Parser::parse(Input                                     &input,
                                  details::OptionalParam<Tree>               old_tree,
                                  details::OptionalParam<const ParseOptions> options)
    {
        if (!hasLanguage())
        {
            throw std::logic_error("Tree-sitter: Cannot parse without a language. Use setLanguage() first.");
        }

#if CPP_TS_HAS_CXX17
        Input::current_input_ptr = &input;
#else
        Input::current_ptr() = &input;
#endif // CPP_TS_HAS_CXX17

        TSTree               *raw_old_tree = details::get_raw<Tree, TSTree>(old_tree);
        const TSParseOptions *raw_options  = details::get_raw<const ParseOptions, const TSParseOptions>(options);

        TSTree *new_tree = raw_options ? ts_parser_parse_with_options(impl_.get(), raw_old_tree, input, *raw_options)
                                       : ts_parser_parse(impl_.get(), raw_old_tree, input);

#if CPP_TS_HAS_CXX17
        Input::current_input_ptr = nullptr;
#else
        Input::current_ptr() = nullptr;
#endif // CPP_TS_HAS_CXX17
        return Tree(new_tree);
    }

    CPP_TS_API Tree Parser::parseString(details::StringViewParameter buffer, details::OptionalParam<Tree> old_tree)
    {
        if (!hasLanguage())
        {
            throw std::logic_error("Tree-sitter: Cannot parse without a language. Use setLanguage() first.");
        }

        if (buffer.size() > std::numeric_limits<std::uint32_t>::max())
        {
            throw std::length_error("Tree-sitter: Input buffer exceeds maximum size of 4GB");
        }

        TSTree *raw_old_tree = details::get_raw<Tree, TSTree>(old_tree);
        return Tree{
            ts_parser_parse_string(impl_.get(), raw_old_tree, buffer.data(), static_cast<std::uint32_t>(buffer.size()))
        };
    }

    CPP_TS_API Tree Parser::parseStringEncoded(details::StringViewParameter buffer,
                                               InputEncoding                encoding,
                                               details::OptionalParam<Tree> old_tree)
    {
        if (!hasLanguage())
        {
            throw std::logic_error("Tree-sitter: Cannot parse without a language. Use setLanguage() first.");
        }

        if (buffer.size() > std::numeric_limits<std::uint32_t>::max())
        {
            throw std::length_error("Tree-sitter: Input buffer exceeds maximum size of 4GB");
        }

        TSTree *raw_old_tree = details::get_raw<Tree, TSTree>(old_tree);
        return Tree{ ts_parser_parse_string_encoding(impl_.get(),
                                                     raw_old_tree,
                                                     buffer.data(),
                                                     static_cast<std::uint32_t>(buffer.size()),
                                                     static_cast<TSInputEncoding>(encoding)) };
    }

    CPP_TS_API void Parser::enableDotGraphs(int file_descriptor)
    {
        if (file_descriptor >= 0)
        {
            int duplicated_fd = CPP_TS_DUP(file_descriptor);
            if (duplicated_fd == -1)
            {
                throw std::runtime_error("Tree-sitter: Failed to duplicate file descriptor for DOT graphs");
            }
            ts_parser_print_dot_graphs(impl_.get(), duplicated_fd);
        }
        else
        {
            ts_parser_print_dot_graphs(impl_.get(), -1);
        }
    }

    CPP_TS_API void Parser::enableDotGraphs(details::StringViewParameter path)
    {
        if (path.empty())
        {
            return;
        }

        std::string path_str(path.data(), path.size());

#if defined(_WIN32)
        FILE *file = nullptr;
        fopen_s(&file, path_str.c_str(), "w");
#else
        FILE *file = fopen(path_str.c_str(), "w");
#endif // defined(_WIN32)
        enableDotGraphs(file);

        if (file != nullptr)
        {
            fclose(file);
        }
    }

    CPP_TS_API void Parser::setLogger(LoggerFunction logger_func)
    {
        current_logger_ = std::move(logger_func);

        TSLogger ts_logger{};
        ts_logger.payload = this;
        ts_logger.log     = [](void *payload, TSLogType log_type, const char *buffer)
        {
            if (payload)
            {
                auto *self = static_cast<Parser *>(payload);
                if (self->current_logger_)
                {
                    self->current_logger_(static_cast<LogType>(log_type), buffer);
                }
            }
        };

        ts_parser_set_logger(impl_.get(), ts_logger);
    }

    CPP_TS_API Parser &Parser::operator=(Parser &&other) noexcept
    {
        if (this != &other)
        {
            impl_           = std::move(other.impl_);
            current_logger_ = std::move(other.current_logger_);
            language_set_   = other.language_set_;

            other.language_set_ = false;

#if defined(CPP_TS_FEATURE_WASM)
            wasm_store_set_       = other.wasm_store_set_;
            other.wasm_store_set_ = false;
#endif
            if (current_logger_)
            {
                setLogger(current_logger_);
            }
            else
            {
                removeLogger();
            }
        }
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////
    // TreeCursor
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API TreeCursor::TreeCursor(TSNode node) noexcept
    {
        is_valid_ = !ts_node_is_null(node);
        impl_     = is_valid_ ? ts_tree_cursor_new(node) : TSTreeCursor{};
    }

    CPP_TS_API TreeCursor::TreeCursor(const TSTreeCursor &cursor) noexcept : impl_{ ts_tree_cursor_copy(&cursor) }
    {
        TSNode node = ts_tree_cursor_current_node(&impl_);
        is_valid_   = !ts_node_is_null(node);
    }

    CPP_TS_API bool TreeCursor::operator==(const TreeCursor &other) const noexcept
    {
        if (is_valid_ != other.is_valid_)
        {
            return false;
        }

        if (!is_valid_)
        {
            return true; // Both invalid, conceptually equal.
        }

        return getCurrentNode() == other.getCurrentNode();
    }

    /////////////////////////////////////////////////////////////////////////////
    // Query
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API Query::Query(LanguageView language, details::StringViewParameter source)
    {
        if (!language.isValid())
        {
            throw std::runtime_error("Tree-sitter: Passed Language for Query was invalid");
        }

        if (source.size() > std::numeric_limits<std::uint32_t>::max())
        {
            throw std::length_error("Tree-sitter: Input source exceeds maximum size of 4GB");
        }

        std::uint32_t error_offset;
        TSQueryError  error_type;

        TSQuery *query = ts_query_new(language,
                                      source.data(),
                                      static_cast<std::uint32_t>(source.size()),
                                      &error_offset,
                                      &error_type);

        details::QueryErrorHelper::validate(error_offset, error_type);

        if (!query)
        {
            throw std::runtime_error("Tree-sitter: Failed to create Query");
        }

        impl_.reset(query);
    }

#if CPP_TS_HAS_CXX23
    CPP_TS_API std::expected<Query, std::string> Query::create(LanguageView                 language,
                                                               details::StringViewParameter source) noexcept
    {
        if (!language.isValid())
        {
            return std::unexpected("Tree-sitter: Passed Language for Query was invalid");
        }

        if (source.size() > std::numeric_limits<std::uint32_t>::max())
        {
            return std::unexpected("Tree-sitter: Input source exceeds maximum size of 4GB");
        }

        std::uint32_t error_offset;
        TSQueryError  error_type;

        TSQuery *query = ts_query_new(language,
                                      source.data(),
                                      static_cast<std::uint32_t>(source.size()),
                                      &error_offset,
                                      &error_type);

        if (error_type != TSQueryErrorNone)
        {
            return std::unexpected(details::QueryErrorHelper::format_error(error_offset, error_type));
        }

        if (!query)
        {
            return std::unexpected("Tree-sitter: Failed to create Query");
        }

        return Query(query);
    }
#endif // CPP_TS_HAS_CXX23

    CPP_TS_API std::vector<QueryPredicateStep> Query::getAllPredicatesForPattern(std::uint32_t pattern_index) const
    {
        std::uint32_t               count = 0;
        const TSQueryPredicateStep *array = ts_query_predicates_for_pattern(impl_.get(), pattern_index, &count);

        if (!array || count == 0)
        {
            return {};
        }

        std::vector<QueryPredicateStep> vec;
        vec.reserve(count);
        for (std::uint32_t i = 0; i < count; ++i)
        {
            vec.emplace_back(array[i]);
        }
        return vec;
    }

    CPP_TS_API void Query::disableCapture(details::StringViewParameter name)
    {
        if (name.size() > std::numeric_limits<std::uint32_t>::max())
        {
            throw std::length_error("Tree-sitter: Input name exceeds maximum size of 4GB");
        }

        ts_query_disable_capture(impl_.get(), name.data(), static_cast<std::uint32_t>(name.size()));
    }

    /////////////////////////////////////////////////////////////////////////////
    // QueryCursor
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API void QueryCursor::exec(const Query                                     &query,
                                      Node                                             node,
                                      details::OptionalParam<const QueryCursorOptions> query_options)
    {
        const TSQueryCursorOptions
                *raw_options = details::get_raw<const QueryCursorOptions, const TSQueryCursorOptions>(query_options);

        if (raw_options)
        {
            ts_query_cursor_exec_with_options(impl_.get(), query, node, raw_options);
        }
        else
        {
            ts_query_cursor_exec(impl_.get(), query, node);
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    // LookaheadIterator
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API LookaheadIterator::LookaheadIterator(LanguageView language, StateID state)
    {
        if (!language.isValid())
        {
            throw std::runtime_error("Tree-sitter: Passed Language for lookahead iterator is not valid");
        }

        impl_.reset(ts_lookahead_iterator_new(language, state));
        if (!impl_)
        {
            throw std::runtime_error("Tree-sitter: Invalid parse state for lookahead iterator");
        }
    }

#if defined(CPP_TS_FEATURE_WASM)
    /////////////////////////////////////////////////////////////////////////////
    // WasmStore
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API WasmStore::WasmStore(WasmEngine *engine)
    {
        TSWasmError  error{ TSWasmErrorKindNone, nullptr };
        TSWasmStore *store = ts_wasm_store_new(engine, &error);

        details::WasmErrorHelper::validate(error);

        if (!store)
        {
            throw std::runtime_error("Tree-sitter: Failed to create Wasm store");
        }

        impl_.reset(store);
        is_valid_ = impl_.get() != nullptr;
    }

    CPP_TS_API WasmStore::WasmStore(WasmStore &&other) noexcept
        : impl_{ std::move(other.impl_) }, is_valid_{ other.is_valid_ }
    {
        other.is_valid_ = false;
    }

#if CPP_TS_HAS_CXX23
    CPP_TS_API std::expected<WasmStore, std::string> WasmStore::create(WasmEngine *engine) noexcept
    {
        TSWasmError  error{ TSWasmErrorKindNone, nullptr };
        TSWasmStore *store = ts_wasm_store_new(engine, &error);

        if (error.kind != TSWasmErrorKindNone)
        {
            std::string err_msg = details::WasmErrorHelper::format_error(error);
            if (error.message)
            {
                std::free(error.message);
            }

            return std::unexpected(err_msg);
        }

        if (!store)
        {
            return std::unexpected("Tree-sitter: Failed to create Wasm store");
        }

        return WasmStore(store);
    }
#endif // CPP_TS_HAS_CXX23

    CPP_TS_API Language WasmStore::loadLanguage(details::StringViewParameter name,
                                                details::StringViewParameter wasm_buffer)
    {
        if (!isValid())
        {
            throw std::logic_error("Tree-sitter: Wasm Store is invalid");
        }

        if (wasm_buffer.size() > std::numeric_limits<std::uint32_t>::max())
        {
            throw std::length_error("Tree-sitter: Input wasm_buffer exceeds maximum size of 4GB");
        }

        TSWasmError       error{ TSWasmErrorKindNone, nullptr };
        const TSLanguage *lang = ts_wasm_store_load_language(impl_.get(),
                                                             name.data(),
                                                             wasm_buffer.data(),
                                                             static_cast<std::uint32_t>(wasm_buffer.size()),
                                                             &error);

        details::WasmErrorHelper::validate(error);
        return Language(lang);
    }

    CPP_TS_API Language WasmStore::loadLanguage(details::StringViewParameter file_path)
    {
        if (!isValid())
        {
            throw std::logic_error("Tree-sitter: Wasm Store is invalid");
        }

        if (file_path.size() > std::numeric_limits<std::uint32_t>::max())
        {
            throw std::length_error("Tree-sitter: Input file_path exceeds maximum size of 4GB");
        }

        const std::string path(file_path);
        std::ifstream     file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            throw std::runtime_error("Tree-sitter: Failed to open wasm file: " + path);
        }

        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        if (size < 0)
        {
            throw std::runtime_error("Tree-sitter: Failed to determine file size: " + path);
        }
        file.seekg(0, std::ios::beg);

        std::string buffer;
        buffer.reserve(static_cast<std::size_t>(size));
        buffer.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        std::string lang_name = details::get_grammar_name_from_file(path);
        return loadLanguage(lang_name, buffer);
    }

#if CPP_TS_HAS_CXX23
    CPP_TS_API std::expected<Language, std::string> WasmStore::loadLanguageExpected(
            details::StringViewParameter name,
            details::StringViewParameter wasm_buffer) noexcept
    {
        if (!isValid())
        {
            return std::unexpected("Tree-sitter: Wasm Store is invalid");
        }

        if (wasm_buffer.size() > std::numeric_limits<std::uint32_t>::max())
        {
            return std::unexpected("Tree-sitter: Input wasm_buffer exceeds maximum size of 4GB");
        }

        TSWasmError       error{ TSWasmErrorKindNone, nullptr };
        const TSLanguage *lang = ts_wasm_store_load_language(impl_.get(),
                                                             name.data(),
                                                             wasm_buffer.data(),
                                                             static_cast<std::uint32_t>(wasm_buffer.size()),
                                                             &error);

        if (error.kind != TSWasmErrorKindNone)
        {
            std::string err_msg = details::WasmErrorHelper::format_error(error);
            if (error.message)
            {
                std::free(error.message);
            }

            return std::unexpected(err_msg);
        }

        return Language(lang);
    }

    CPP_TS_API std::expected<Language, std::string> WasmStore::loadLanguageExpected(
            details::StringViewParameter file_path)
    {
        if (!isValid())
        {
            return std::unexpected("Tree-sitter: Wasm Store is invalid");
        }

        if (file_path.size() > std::numeric_limits<std::uint32_t>::max())
        {
            return std::unexpected("Tree-sitter: Input file_path exceeds maximum size of 4GB");
        }

        const std::string path(file_path);
        std::ifstream     file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            return std::unexpected("Tree-sitter: Failed to open wasm file: " + path);
        }

        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        if (size < 0)
        {
            return std::unexpected("Tree-sitter: Failed to determine file size: " + path);
        }
        file.seekg(0, std::ios::beg);

        std::string buffer;
        buffer.reserve(static_cast<std::size_t>(size));
        buffer.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        std::string lang_name = details::get_grammar_name_from_file(path);
        return loadLanguageExpected(lang_name, buffer);
    }
#endif // CPP_TS_HAS_CXX23

    CPP_TS_API std::size_t WasmStore::getLanguageCount() const
    {
        if (!isValid())
        {
            throw std::logic_error("Tree-sitter: Wasm Store is invalid");
        }

        return ts_wasm_store_language_count(impl_.get());
    }

    CPP_TS_API WasmStore &WasmStore::operator=(WasmStore &&other) noexcept
    {
        if (this != &other)
        {
            impl_           = std::move(other.impl_);
            is_valid_       = other.is_valid_;
            other.is_valid_ = false;
        }
        return *this;
    }
#endif // defined(CPP_TS_FEATURE_WASM)

} // namespace ts
