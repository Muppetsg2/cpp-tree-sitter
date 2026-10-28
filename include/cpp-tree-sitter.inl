namespace ts
{
    namespace details
    {
#pragma region QUERY_ERROR_HELPER

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

        CPP_TS_API Status QueryErrorHelper::to_status(std::uint32_t error_offset, TSQueryError error)
        {
            ErrorKind kind = ErrorKind::Unknown;
            switch (error)
            {
                case TSQueryErrorNone:
                    return Status();
                case TSQueryErrorSyntax:
                    kind = ErrorKind::QuerySyntax;
                    break;
                case TSQueryErrorNodeType:
                    kind = ErrorKind::QueryNodeType;
                    break;
                case TSQueryErrorField:
                    kind = ErrorKind::QueryField;
                    break;
                case TSQueryErrorCapture:
                    kind = ErrorKind::QueryCapture;
                    break;
                case TSQueryErrorStructure:
                    kind = ErrorKind::QueryStructure;
                    break;
                case TSQueryErrorLanguage:
                    kind = ErrorKind::QueryLanguage;
                    break;
                default:
                    break;
            }
            return Status(kind, format_error(error_offset, error), error_offset);
        }

#pragma endregion

#pragma region WASM_ERROR_HELPER

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

        CPP_TS_API Status WasmErrorHelper::to_status(TSWasmError &error)
        {
            ErrorKind kind = ErrorKind::Unknown;
            switch (error.kind)
            {
                case TSWasmErrorKindNone:
                    return Status();
                case TSWasmErrorKindParse:
                    kind = ErrorKind::WasmParse;
                    break;
                case TSWasmErrorKindCompile:
                    kind = ErrorKind::WasmCompile;
                    break;
                case TSWasmErrorKindInstantiate:
                    kind = ErrorKind::WasmInstantiate;
                    break;
                case TSWasmErrorKindAllocate:
                    kind = ErrorKind::WasmAllocate;
                    break;
                default:
                    break;
            }

            const Owned<char> message(error.message);
            error.message = nullptr;

            TSWasmError copy{ error.kind, message.get() };
            return Status(kind, format_error(copy));
        }

        CPP_TS_API std::string get_grammar_name_from_file(StringViewParameter path)
        {
            const std::string filename(path.data(), path.size());
            const std::size_t slash = filename.find_last_of("/\\");
            const std::size_t dot   = filename.find_last_of('.');

            const std::size_t start = (slash == std::string::npos) ? 0 : slash + 1;
            const std::size_t end   = (dot == std::string::npos || dot <= start) ? filename.length() : dot;

            std::string       name   = filename.substr(start, end - start);
            const std::string prefix = "tree-sitter-";

            if (name.compare(0, prefix.length(), prefix) == 0)
            {
                return name.substr(prefix.length());
            }

            return name;
        }

#endif // defined(CPP_TS_FEATURE_WASM)

#pragma endregion

#pragma region PARSER_HELPERS

        /////////////////////////////////////////////////////////////////////////////
        // Parser helpers
        /////////////////////////////////////////////////////////////////////////////

        CPP_TS_API Status assign_language(TSParser                *parser,
                                          LanguageView             language,
                                          CPP_TS_MAYBE_UNUSED bool has_wasm_store) noexcept
        {
            if (ts_parser_set_language(parser, language))
            {
                return Status();
            }

#if defined(CPP_TS_FEATURE_WASM)
            if (language.isWasm() && !has_wasm_store)
            {
                return Status(ErrorKind::WasmStoreRequired,
                              "Tree-sitter: Parser needs WasmStore to process Wasm Language");
            }
#endif // defined(CPP_TS_FEATURE_WASM)

            if (!language.isParseable())
            {
                return Status(ErrorKind::LanguageRejected,
                              "Tree-sitter: Language cannot be used for parsing (ts_language_is_parseable returned "
                              "false)");
            }

            return Status(ErrorKind::LanguageRejected,
                          "Tree-sitter: Language rejected by tree-sitter (ABI version incompatible with the runtime)");
        }

#pragma endregion

#pragma region TREE_CURSOR_HELPERS

        /////////////////////////////////////////////////////////////////////////////
        // TreeCursor helpers
        /////////////////////////////////////////////////////////////////////////////

        CPP_TS_API bool is_zero_initialized(const TSTreeCursor &cursor) noexcept
        {
            for (std::size_t i = 0; i < sizeof(cursor.context) / sizeof(cursor.context[0]); ++i)
            {
                if (cursor.context[i] != 0)
                {
                    return false;
                }
            }
            return cursor.tree == nullptr && cursor.id == nullptr;
        }

#pragma endregion
    } // namespace details

#pragma region POINT

    /////////////////////////////////////////////////////////////////////////////
    // Point
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API void Point::edit(const InputEdit &edit, std::uint32_t &byte_offset)
    {
        editImpl(edit, byte_offset).throwIfFailed();
    }

#if CPP_TS_HAS_EXPECTED
    CPP_TS_API std::expected<void, Error> Point::tryEdit(const InputEdit &edit, std::uint32_t &byte_offset)
    {
        return details::to_expected(editImpl(edit, byte_offset));
    }
#endif // CPP_TS_HAS_EXPECTED

    CPP_TS_API details::Status Point::editImpl(const InputEdit &edit, std::uint32_t &byte_offset) noexcept
    {
        if (!edit.isValid())
        {
            return details::Status(ErrorKind::InvalidEdit, details::invalid_edit_message);
        }

        TSPoint     raw_point = static_cast<TSPoint>(*this);
        TSInputEdit raw_edit  = static_cast<TSInputEdit>(edit);

        ts_point_edit(&raw_point, &byte_offset, &raw_edit);

        row    = raw_point.row;
        column = raw_point.column;
        return details::Status();
    }

#pragma endregion

#pragma region RANGE

    /////////////////////////////////////////////////////////////////////////////
    // Range
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API void Range::edit(const InputEdit &edit)
    {
        editImpl(edit).throwIfFailed();
    }

#if CPP_TS_HAS_EXPECTED
    CPP_TS_API std::expected<void, Error> Range::tryEdit(const InputEdit &edit)
    {
        return details::to_expected(editImpl(edit));
    }
#endif // CPP_TS_HAS_EXPECTED

    CPP_TS_API details::Status Range::editImpl(const InputEdit &edit) noexcept
    {
        if (!edit.isValid())
        {
            return details::Status(ErrorKind::InvalidEdit, details::invalid_edit_message);
        }

        TSRange     raw_range = static_cast<TSRange>(*this);
        TSInputEdit raw_edit  = static_cast<TSInputEdit>(edit);

        ts_range_edit(&raw_range, &raw_edit);

        point.start = raw_range.start_point;
        point.end   = raw_range.end_point;
        byte.start  = raw_range.start_byte;
        byte.end    = raw_range.end_byte;
        return details::Status();
    }

#pragma endregion

#pragma region INPUT

    /////////////////////////////////////////////////////////////////////////////
    // Input
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API Input::Input()
        : read_(nullptr), decode_(nullptr), encoding_(InputEncoding::UTF8), impl_{ details::make_unique<TSInput>() }
    {
        impl_->payload = this;
        impl_->read    = read_proxy;
    }

    CPP_TS_API Input::Input(ReadFunction read, InputEncoding encoding)
        : read_(read ? std::move(read) : nullptr)
        , decode_(nullptr)
        , encoding_(encoding == InputEncoding::Custom ? InputEncoding::UTF8 : encoding)
        , impl_{ details::make_unique<TSInput>() }
    {
        impl_->payload = this;
        impl_->read    = read_proxy;
    }

    CPP_TS_API Input::Input(ReadFunction read, DecodeFunction decode)
        : read_(read ? std::move(read) : nullptr)
        , decode_(decode ? std::move(decode) : nullptr)
        , encoding_(decode_ ? InputEncoding::Custom : InputEncoding::UTF8)
        , impl_{ details::make_unique<TSInput>() }
    {
        impl_->payload = this;
        impl_->read    = read_proxy;
    }

    CPP_TS_API Input::Input(Input &&other) noexcept
        : read_(std::move(other.read_))
        , decode_(std::move(other.decode_))
        , encoding_(other.encoding_)
        , impl_(std::move(other.impl_))
        , pending_exception_(std::move(other.pending_exception_))
    {
        if (impl_)
        {
            impl_->payload = this;
        }
    }

    CPP_TS_API Input &Input::operator=(Input &&other) noexcept
    {
        if (this != &other)
        {
            read_              = std::move(other.read_);
            decode_            = std::move(other.decode_);
            encoding_          = other.encoding_;
            impl_              = std::move(other.impl_);
            pending_exception_ = std::move(other.pending_exception_);
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

    CPP_TS_API const char *Input::read_proxy(void          *payload,
                                             std::uint32_t  byte_index,
                                             TSPoint        position,
                                             std::uint32_t *bytes_read) noexcept
    {
        *bytes_read = 0;

        auto *self = static_cast<const Input *>(payload);
        // After a callback failure the input reports its end, which finishes the parse as quickly as possible.
        if (!self || !self->read_ || self->pending_exception_)
        {
            return nullptr;
        }

        try
        {
            details::StringViewReturn result = self->read_(byte_index, position, bytes_read);
            // The callback may either report the length through `bytes_read` or only return a sized view.
            if (*bytes_read == 0 && !result.empty())
            {
                *bytes_read = details::checked_length(result.size(), "Chunk returned by the read callback");
            }
            return *bytes_read == 0 ? nullptr : result.data();
        }
        catch (...)
        {
            // Exceptions must not cross the C frames of tree-sitter. Parser::parse rethrows it.
            self->pending_exception_ = std::current_exception();
            *bytes_read              = 0;
            return nullptr;
        }
    }

    CPP_TS_API std::uint32_t Input::decode_proxy(const std::uint8_t *string,
                                                 std::uint32_t       length,
                                                 std::int32_t       *code_point) noexcept
    {
        const Input *self = current();
        if (self && self->decode_ && !self->pending_exception_)
        {
            try
            {
                return self->decode_(ByteView{ string, length }, code_point);
            }
            catch (...)
            {
                self->pending_exception_ = std::current_exception();
            }
        }

        // Report an invalid character (TS_DECODE_ERROR) consuming one byte, so the lexer always makes progress.
        *code_point = -1;
        return 1;
    }

#pragma endregion

#pragma region LANGUAGE_VIEW

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

        // tree-sitter compares names with strncmp, so an empty name would match "ERROR" (ts_builtin_sym_error).
        if (name.empty())
        {
            return 0;
        }

        return ts_language_symbol_for_name(impl_,
                                           details::safe_data(name),
                                           static_cast<std::uint32_t>(name.size()),
                                           isNamed);
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

        return ts_language_field_id_for_name(impl_, details::safe_data(name), static_cast<std::uint32_t>(name.size()));
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
            details::Status(ErrorKind::InvalidObject, details::invalid_lookahead_view_message).raise();
        }

        return LookaheadIterator(impl_, state);
    }

#if CPP_TS_HAS_EXPECTED
    CPP_TS_API std::expected<LookaheadIterator, Error> LanguageView::tryGetLookaheadIterator(StateID state) const
    {
        if (!isValid())
        {
            return details::to_unexpected(
                    details::Status(ErrorKind::InvalidObject, details::invalid_lookahead_view_message));
        }

        return LookaheadIterator::create(*this, state);
    }
#endif // CPP_TS_HAS_EXPECTED

    CPP_TS_API details::StringViewReturn LanguageView::getName() const
    {
        return isValid() ? details::make_view(ts_language_name(impl_)) : details::StringViewReturn("");
    }

#if CPP_TS_HAS_OPTIONAL
    CPP_TS_API std::optional<LanguageMetadata> LanguageView::getMetadata() const noexcept
    {
        static constexpr std::optional<LanguageMetadata> null_value = std::nullopt;
#else
    CPP_TS_API LanguageMetadata LanguageView::getMetadata() const noexcept
    {
        static constexpr LanguageMetadata null_value = { 0, 0, 0 };
#endif // CPP_TS_HAS_OPTIONAL

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

#pragma endregion

#pragma region LANGUAGE

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

#pragma endregion

#pragma region NODE

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
        return isNull() ? null()
                        : Node{ ts_node_child_by_field_name(impl_,
                                                            details::safe_data(name),
                                                            static_cast<std::uint32_t>(name.size())) };
    }

    CPP_TS_API void Node::edit(const InputEdit &edit)
    {
        editImpl(edit).throwIfFailed();
    }

#if CPP_TS_HAS_EXPECTED
    CPP_TS_API std::expected<void, Error> Node::tryEdit(const InputEdit &edit)
    {
        return details::to_expected(editImpl(edit));
    }
#endif // CPP_TS_HAS_EXPECTED

    CPP_TS_API TreeCursor Node::getCursor() const noexcept
    {
        return TreeCursor(impl_);
    }

    CPP_TS_API details::Status Node::editImpl(const InputEdit &edit) noexcept
    {
        if (isNull())
        {
            return details::Status(ErrorKind::InvalidObject, "Tree-sitter: Couldn't change null Node");
        }

        if (!edit.isValid())
        {
            return details::Status(ErrorKind::InvalidEdit, details::invalid_edit_message);
        }

        TSInputEdit raw_edit = static_cast<TSInputEdit>(edit);
        ts_node_edit(&impl_, &raw_edit);
        return details::Status();
    }

#pragma endregion

#pragma region TREE

    /////////////////////////////////////////////////////////////////////////////
    // Tree
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API Tree::Tree(const Tree &other) noexcept : impl_{ other.impl_ ? ts_tree_copy(other.impl_.get()) : nullptr }
    {}

    CPP_TS_API Tree &Tree::operator=(const Tree &other)
    {
        if (this != &other)
        {
            impl_.reset(other.impl_ ? ts_tree_copy(other.impl_.get()) : nullptr);
        }
        return *this;
    }

    CPP_TS_API Tree::Tree(Tree &&other) noexcept : impl_{ std::move(other.impl_) }
    {}

    CPP_TS_API Tree &Tree::operator=(Tree &&other) noexcept
    {
        impl_ = std::move(other.impl_);
        return *this;
    }

    CPP_TS_API void Tree::edit(const InputEdit &edit)
    {
        editImpl(edit).throwIfFailed();
    }

#if CPP_TS_HAS_EXPECTED
    CPP_TS_API std::expected<void, Error> Tree::tryEdit(const InputEdit &edit)
    {
        return details::to_expected(editImpl(edit));
    }
#endif // CPP_TS_HAS_EXPECTED

    CPP_TS_API std::vector<Range> Tree::getIncludedRanges() const
    {
        if (!isValid())
        {
            return {};
        }

        std::uint32_t count = 0;
        // Owned immediately, so the array is released even if building the vector throws.
        const details::Owned<TSRange> array{ ts_tree_included_ranges(impl_.get(), &count) };
        if (!array || count == 0)
        {
            return {};
        }

        return std::vector<Range>(array.get(), array.get() + count);
    }

    CPP_TS_API std::vector<Range> Tree::getChangedRanges(const Tree &old_tree, const Tree &new_tree)
    {
        if (!old_tree.isValid() || !new_tree.isValid())
        {
            return {};
        }

        std::uint32_t                 count = 0;
        const details::Owned<TSRange> array{
            ts_tree_get_changed_ranges(old_tree.impl_.get(), new_tree.impl_.get(), &count)
        };
        if (!array || count == 0)
        {
            return {};
        }

        return std::vector<Range>(array.get(), array.get() + count);
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

    CPP_TS_API details::Status Tree::editImpl(const InputEdit &edit) noexcept
    {
        if (!isValid())
        {
            return details::Status(ErrorKind::InvalidObject, "Tree-sitter: Couldn't change invalid Tree");
        }

        if (!edit.isValid())
        {
            return details::Status(ErrorKind::InvalidEdit, details::invalid_edit_message);
        }

        TSInputEdit raw_edit = static_cast<TSInputEdit>(edit);
        ts_tree_edit(impl_.get(), &raw_edit);
        return details::Status();
    }

#pragma endregion

#pragma region PARSER

    /////////////////////////////////////////////////////////////////////////////
    // Parser
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API Parser::Parser(LanguageView language) : impl_{ ts_parser_new() }
    {
        const details::Status status = init(language);
        if (!status.ok())
        {
            status.raiseAs<std::runtime_error>();
        }
    }

#if defined(CPP_TS_FEATURE_WASM)
    CPP_TS_API Parser::Parser(LanguageView language, WasmStore &store) : impl_{ ts_parser_new() }
    {
        const details::Status status = init(language, store);
        if (!status.ok())
        {
            status.raiseAs<std::runtime_error>();
        }
    }
#endif // defined(CPP_TS_FEATURE_WASM)

    CPP_TS_API Parser::Parser(Parser &&other) noexcept
        : impl_{ std::move(other.impl_) }
        , logger_{ std::move(other.logger_) }
#if defined(CPP_TS_FEATURE_WASM)
        , wasm_store_set_{ other.wasm_store_set_ }
#endif // defined(CPP_TS_FEATURE_WASM)
    {
#if defined(CPP_TS_FEATURE_WASM)
        other.wasm_store_set_ = false;
#endif // defined(CPP_TS_FEATURE_WASM)
    }

    CPP_TS_API Parser &Parser::operator=(Parser &&other) noexcept
    {
        if (this != &other)
        {
            impl_   = std::move(other.impl_);
            logger_ = std::move(other.logger_);
#if defined(CPP_TS_FEATURE_WASM)
            wasm_store_set_       = other.wasm_store_set_;
            other.wasm_store_set_ = false;
#endif // defined(CPP_TS_FEATURE_WASM)
        }
        return *this;
    }

#if CPP_TS_HAS_EXPECTED
    CPP_TS_API std::expected<Parser, Error> Parser::create(LanguageView language)
    {
        Parser                parser;
        const details::Status status = parser.init(language);
        if (!status.ok())
        {
            return details::to_unexpected(status);
        }
        return parser;
    }

#if defined(CPP_TS_FEATURE_WASM)
    CPP_TS_API std::expected<Parser, Error> Parser::create(LanguageView language, WasmStore &store)
    {
        Parser                parser;
        const details::Status status = parser.init(language, store);
        if (!status.ok())
        {
            return details::to_unexpected(status);
        }
        return parser;
    }
#endif // defined(CPP_TS_FEATURE_WASM)
#endif // CPP_TS_HAS_EXPECTED

    CPP_TS_API bool Parser::setLanguage(LanguageView language)
#if !defined(CPP_TS_FEATURE_WASM)
            noexcept
#endif // !defined(CPP_TS_FEATURE_WASM)
    {
        const details::Status status = assignLanguage(language);
#if defined(CPP_TS_FEATURE_WASM)
        // A missing store is a usage error of the wrapper, not a property of the language.
        if (!status.ok() && status.kind() == ErrorKind::WasmStoreRequired)
        {
            status.raise();
        }
#endif // defined(CPP_TS_FEATURE_WASM)
        return status.ok();
    }

#if CPP_TS_HAS_EXPECTED
    CPP_TS_API std::expected<void, Error> Parser::trySetLanguage(LanguageView language)
    {
        return details::to_expected(assignLanguage(language));
    }
#endif // CPP_TS_HAS_EXPECTED

#if CPP_TS_HAS_SPAN
    CPP_TS_API bool Parser::setIncludedRanges(std::span<const Range> ranges)
#else
    CPP_TS_API bool Parser::setIncludedRanges(const std::vector<Range> &ranges)
#endif // CPP_TS_HAS_SPAN
    {
        if (!impl_)
        {
            return false;
        }

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
            TSRange       &last    = merged.back();
            const TSRange &current = c_ranges[i];

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

        return ts_parser_set_included_ranges(impl_.get(),
                                             merged.data(),
                                             details::checked_length(merged.size(), "Ranges size"));
    }

#if defined(CPP_TS_FEATURE_WASM)
    CPP_TS_API void Parser::setWasmStore(WasmStore &store) noexcept
    {
        if (!impl_)
        {
            impl_.reset(ts_parser_new());
        }

        ts_parser_set_wasm_store(impl_.get(), store);
        wasm_store_set_ = store.isValid();
        store.setInvalid();
    }
#endif // defined(CPP_TS_FEATURE_WASM)

    CPP_TS_API std::vector<Range> Parser::getIncludedRanges() const noexcept
    {
        if (!impl_)
        {
            return {};
        }

        std::uint32_t  count = 0;
        const TSRange *array = ts_parser_included_ranges(impl_.get(), &count);
        if (!array || count == 0)
        {
            return {};
        }

        return std::vector<Range>(array, array + count);
    }

#if defined(CPP_TS_FEATURE_WASM)
    CPP_TS_API WasmStore Parser::takeWasmStore() noexcept
    {
        if (!impl_)
        {
            return WasmStore{ static_cast<TSWasmStore *>(nullptr) };
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
        Tree                  result{ nullptr };
        const details::Status status = parseImpl(result, input, old_tree, options);
        return treeOrThrow(result, status);
    }

    CPP_TS_API Tree Parser::parseString(details::StringViewParameter buffer, details::OptionalParam<Tree> old_tree)
    {
        Tree                  result{ nullptr };
        const details::Status status = parseStringImpl(result, buffer, nullptr, old_tree);
        return treeOrThrow(result, status);
    }

    CPP_TS_API Tree Parser::parseStringEncoded(details::StringViewParameter buffer,
                                               InputEncoding                encoding,
                                               details::OptionalParam<Tree> old_tree)
    {
        Tree                  result{ nullptr };
        const details::Status status = parseStringImpl(result, buffer, &encoding, old_tree);
        return treeOrThrow(result, status);
    }

#if CPP_TS_HAS_EXPECTED
    CPP_TS_API std::expected<Tree, Error> Parser::tryParse(Input                                     &input,
                                                           details::OptionalParam<Tree>               old_tree,
                                                           details::OptionalParam<const ParseOptions> options)
    {
        Tree                  result{ nullptr };
        const details::Status status = parseImpl(result, input, old_tree, options);
        if (!status.ok())
        {
            return details::to_unexpected(status);
        }
        return result;
    }

    CPP_TS_API std::expected<Tree, Error> Parser::tryParseString(details::StringViewParameter buffer,
                                                                 details::OptionalParam<Tree> old_tree)
    {
        Tree                  result{ nullptr };
        const details::Status status = parseStringImpl(result, buffer, nullptr, old_tree);
        if (!status.ok())
        {
            return details::to_unexpected(status);
        }
        return result;
    }

    CPP_TS_API std::expected<Tree, Error> Parser::tryParseStringEncoded(details::StringViewParameter buffer,
                                                                        InputEncoding                encoding,
                                                                        details::OptionalParam<Tree> old_tree)
    {
        Tree                  result{ nullptr };
        const details::Status status = parseStringImpl(result, buffer, &encoding, old_tree);
        if (!status.ok())
        {
            return details::to_unexpected(status);
        }
        return result;
    }
#endif // CPP_TS_HAS_EXPECTED

    CPP_TS_API void Parser::enableDotGraphs(int file_descriptor)
    {
        enableDotGraphsImpl(file_descriptor).throwIfFailed();
    }

#if CPP_TS_HAS_EXPECTED
    CPP_TS_API std::expected<void, Error> Parser::tryEnableDotGraphs(int file_descriptor)
    {
        return details::to_expected(enableDotGraphsImpl(file_descriptor));
    }
#endif // CPP_TS_HAS_EXPECTED

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
        if (!logger_func)
        {
            removeLogger();
            return;
        }

        if (!impl_)
        {
            impl_.reset(ts_parser_new());
        }

        std::unique_ptr<LoggerState> state = details::make_unique<LoggerState>();
        state->function                    = std::move(logger_func);

        TSLogger ts_logger{};
        ts_logger.payload = state.get();
        ts_logger.log     = &Parser::loggerProxy;
        ts_parser_set_logger(impl_.get(), ts_logger);

        logger_ = std::move(state);
    }

    CPP_TS_API void Parser::loggerProxy(void *payload, TSLogType log_type, const char *buffer) noexcept
    {
        auto *state = static_cast<LoggerState *>(payload);
        if (!state || !state->function || state->pending_exception)
        {
            return;
        }

        try
        {
            state->function(static_cast<LogType>(log_type), buffer);
        }
        catch (...)
        {
            state->pending_exception = std::current_exception();
        }
    }

    CPP_TS_API details::Status Parser::init(LanguageView language) noexcept
    {
        if (!language.isValid())
        {
            return details::Status(ErrorKind::InvalidLanguage, details::invalid_parser_language_message);
        }
        return assignLanguage(language);
    }

#if defined(CPP_TS_FEATURE_WASM)
    CPP_TS_API details::Status Parser::init(LanguageView language, WasmStore &store) noexcept
    {
        if (!language.isValid())
        {
            return details::Status(ErrorKind::InvalidLanguage, details::invalid_parser_language_message);
        }

        setWasmStore(store);
        const details::Status status = assignLanguage(language);
        if (!status.ok())
        {
            // Give the store back, the caller still owns it when construction fails.
            store = takeWasmStore();
        }
        return status;
    }
#endif // defined(CPP_TS_FEATURE_WASM)

    CPP_TS_API details::Status Parser::assignLanguage(LanguageView language) noexcept
    {
        if (!impl_)
        {
            impl_.reset(ts_parser_new());
        }

#if defined(CPP_TS_FEATURE_WASM)
        return details::assign_language(impl_.get(), language, wasm_store_set_);
#else
        return details::assign_language(impl_.get(), language, false);
#endif // defined(CPP_TS_FEATURE_WASM)
    }

    CPP_TS_API details::Status Parser::enableDotGraphsImpl(int file_descriptor) noexcept
    {
        if (!impl_)
        {
            return details::Status();
        }

        if (file_descriptor >= 0)
        {
            // tree-sitter takes ownership of the descriptor (closes it later), so it gets a duplicate.
            int duplicated_fd = CPP_TS_DUP(file_descriptor);
            if (duplicated_fd == -1)
            {
                return details::Status(ErrorKind::SystemError,
                                       "Tree-sitter: Failed to duplicate file descriptor for DOT graphs");
            }
            ts_parser_print_dot_graphs(impl_.get(), duplicated_fd);
        }
        else
        {
            ts_parser_print_dot_graphs(impl_.get(), -1);
        }
        return details::Status();
    }

    CPP_TS_API details::Status Parser::checkCanParse() const noexcept
    {
        if (!hasLanguage())
        {
            return details::Status(ErrorKind::NoLanguage,
                                   "Tree-sitter: Cannot parse without a language. Use setLanguage() first.");
        }
        return details::Status();
    }

    CPP_TS_API details::Status Parser::finishParse(Tree &result, TSTree *raw_tree, const TSTree *old_tree)
    {
        result = Tree{ raw_tree };

        if (logger_)
        {
            details::rethrow_if_set(logger_->pending_exception);
        }

        if (raw_tree != nullptr)
        {
            return details::Status();
        }

        if (old_tree != nullptr && ts_tree_language(old_tree) != ts_parser_language(impl_.get()))
        {
            return details::Status(ErrorKind::OldTreeLanguage,
                                   "Tree-sitter: old_tree was parsed with a different language than the parser");
        }
        return details::Status(ErrorKind::ParseFailed,
                               "Tree-sitter: The parse returned no tree (e.g. it was cancelled by the progress "
                               "callback)");
    }

    CPP_TS_API details::Status Parser::parseImpl(Tree                                      &result,
                                                 Input                                     &input,
                                                 details::OptionalParam<Tree>               old_tree,
                                                 details::OptionalParam<const ParseOptions> options)
    {
        TSTree               *raw_old_tree = details::get_raw<Tree, TSTree>(old_tree);
        const details::Status can_parse    = checkCanParse();
        if (!can_parse.ok())
        {
            return can_parse;
        }

        const ParseOptions   *wrapper_options = options.get();
        const TSParseOptions *raw_options     = nullptr;
        if (wrapper_options)
        {
            raw_options = static_cast<const TSParseOptions *>(*wrapper_options);
        }

        TSTree *new_tree = nullptr;
        {
            Input::ScopedCurrent current_input(input);
            new_tree = raw_options ? ts_parser_parse_with_options(impl_.get(), raw_old_tree, input, *raw_options)
                                   : ts_parser_parse(impl_.get(), raw_old_tree, input);
        }

        const details::Status status = finishParse(result, new_tree, raw_old_tree);

        details::rethrow_if_set(input.pending_exception_);
        if (wrapper_options)
        {
            wrapper_options->rethrowPendingException();
        }
        return status;
    }

    CPP_TS_API details::Status Parser::parseStringImpl(Tree                        &result,
                                                       details::StringViewParameter buffer,
                                                       const InputEncoding         *encoding,
                                                       details::OptionalParam<Tree> old_tree)
    {
        TSTree               *raw_old_tree = details::get_raw<Tree, TSTree>(old_tree);
        const details::Status can_parse    = checkCanParse();
        if (!can_parse.ok())
        {
            return can_parse;
        }

        const details::Status length_status = details::check_length(buffer.size(), "Input buffer");
        if (!length_status.ok())
        {
            return length_status;
        }

        const std::uint32_t length = static_cast<std::uint32_t>(buffer.size());
        TSTree *new_tree           = encoding ? ts_parser_parse_string_encoding(impl_.get(),
                                                                                raw_old_tree,
                                                                                details::safe_data(buffer),
                                                                                length,
                                                                                static_cast<TSInputEncoding>(*encoding))
                                              : ts_parser_parse_string(impl_.get(),
                                                                       raw_old_tree,
                                                                       details::safe_data(buffer),
                                                                       length);
        return finishParse(result, new_tree, raw_old_tree);
    }

    CPP_TS_API Tree Parser::treeOrThrow(Tree &result, const details::Status &status)
    {
        if (!status.ok() && status.kind() != ErrorKind::ParseFailed)
        {
            status.raise();
        }
        return std::move(result);
    }

#pragma endregion

#pragma region TREE_CURSOR

    /////////////////////////////////////////////////////////////////////////////
    // TreeCursor
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API TreeCursor::TreeCursor(TSNode node) noexcept
    {
        is_valid_ = !ts_node_is_null(node);
        impl_     = is_valid_ ? ts_tree_cursor_new(node) : TSTreeCursor{};
    }

    CPP_TS_API TreeCursor::TreeCursor(const TSTreeCursor &cursor) noexcept
        : impl_{}, is_valid_(!details::is_zero_initialized(cursor))
    {
        if (is_valid_)
        {
            impl_ = ts_tree_cursor_copy(&cursor);
        }
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

#pragma endregion

#pragma region QUERY

    /////////////////////////////////////////////////////////////////////////////
    // Query
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API Query::Query(LanguageView language, details::StringViewParameter source)
    {
        compile(language, source, impl_).throwIfFailed();
    }

#if CPP_TS_HAS_EXPECTED
    CPP_TS_API std::expected<Query, Error> Query::create(LanguageView language, details::StringViewParameter source)
    {
        details::Owned<TSQuery> query;
        const details::Status   status = compile(language, source, query);
        if (!status.ok())
        {
            return details::to_unexpected(status);
        }
        return Query(query.release());
    }
#endif // CPP_TS_HAS_EXPECTED

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

        ts_query_disable_capture(impl_.get(), details::safe_data(name), static_cast<std::uint32_t>(name.size()));
    }

    CPP_TS_API details::Status Query::compile(LanguageView                 language,
                                              details::StringViewParameter source,
                                              details::Owned<TSQuery>     &result)
    {
        if (!language.isValid())
        {
            return details::Status(ErrorKind::InvalidLanguage, "Tree-sitter: Passed Language for Query was invalid");
        }

        const details::Status length_status = details::check_length(source.size(), "Input source");
        if (!length_status.ok())
        {
            return length_status;
        }

        std::uint32_t error_offset = 0;
        TSQueryError  error_type   = TSQueryErrorNone;
        result.reset(ts_query_new(language,
                                  details::safe_data(source),
                                  static_cast<std::uint32_t>(source.size()),
                                  &error_offset,
                                  &error_type));

        const details::Status status = details::QueryErrorHelper::to_status(error_offset, error_type);
        if (!status.ok())
        {
            result.reset();
            return status;
        }

        if (!result)
        {
            return details::Status(ErrorKind::Unknown, "Tree-sitter: Failed to create Query");
        }
        return details::Status();
    }

#pragma endregion

#pragma region QUERY_CURSOR

    /////////////////////////////////////////////////////////////////////////////
    // QueryCursor
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API void QueryCursor::exec(const Query                                     &query,
                                      Node                                             node,
                                      details::OptionalParam<const QueryCursorOptions> query_options)
    {
        execImpl(query, node, query_options).throwIfFailed();
    }

#if CPP_TS_HAS_EXPECTED
    CPP_TS_API std::expected<void, Error> QueryCursor::tryExec(
            const Query                                     &query,
            Node                                             node,
            details::OptionalParam<const QueryCursorOptions> query_options)
    {
        return details::to_expected(execImpl(query, node, query_options));
    }
#endif // CPP_TS_HAS_EXPECTED

    CPP_TS_API bool QueryCursor::progressProxy(TSQueryCursorState *state) noexcept
    {
        auto *self = state ? static_cast<ExecState *>(state->payload) : nullptr;
        if (!self || !self->progress)
        {
            return false;
        }

        if (self->cancelled)
        {
            return true;
        }

        try
        {
            QueryCursorState cpp_state(state);
            self->cancelled = self->progress(&cpp_state);
        }
        catch (...)
        {
            // Exceptions must not cross the C frames of tree-sitter: stop the execution and rethrow later.
            self->pending_exception = std::current_exception();
            self->cancelled         = true;
        }
        return self->cancelled;
    }

    CPP_TS_API details::Status QueryCursor::execImpl(const Query                                     &query,
                                                     Node                                             node,
                                                     details::OptionalParam<const QueryCursorOptions> query_options)
    {
        if (!query.isValid())
        {
            return details::Status(ErrorKind::InvalidArgument,
                                   "Tree-sitter: Cannot execute an invalid (moved-from) Query");
        }

        const QueryCursorOptions *options = query_options.get();

        if (options && options->getProgressCallback())
        {
            if (!state_)
            {
                state_ = details::make_unique<ExecState>();
            }

            state_->progress                      = options->getProgressCallback();
            state_->pending_exception             = nullptr;
            state_->cancelled                     = false;
            state_->raw_options.payload           = state_.get();
            state_->raw_options.progress_callback = &QueryCursor::progressProxy;

            ts_query_cursor_exec_with_options(impl_.get(), query, node, &state_->raw_options);
        }
        else
        {
            if (state_)
            {
                state_->progress          = nullptr;
                state_->pending_exception = nullptr;
                state_->cancelled         = false;
            }
            ts_query_cursor_exec(impl_.get(), query, node);
        }
        return details::Status();
    }

#pragma endregion

#pragma region LOOKAHEAD_ITERATOR

    /////////////////////////////////////////////////////////////////////////////
    // LookaheadIterator
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API LookaheadIterator::LookaheadIterator(LanguageView language, StateID state)
    {
        make(language, state, impl_).throwIfFailed();
    }

#if CPP_TS_HAS_EXPECTED
    CPP_TS_API std::expected<LookaheadIterator, Error> LookaheadIterator::create(LanguageView language, StateID state)
    {
        details::Owned<TSLookaheadIterator> iterator;
        const details::Status               status = make(language, state, iterator);
        if (!status.ok())
        {
            return details::to_unexpected(status);
        }
        return LookaheadIterator(iterator.release());
    }
#endif // CPP_TS_HAS_EXPECTED

    CPP_TS_API details::Status LookaheadIterator::make(LanguageView                         language,
                                                       StateID                              state,
                                                       details::Owned<TSLookaheadIterator> &result) noexcept
    {
        if (!language.isValid())
        {
            return details::Status(ErrorKind::InvalidLanguage,
                                   "Tree-sitter: Passed Language for lookahead iterator is not valid");
        }

        result.reset(ts_lookahead_iterator_new(language, state));
        if (!result)
        {
            return details::Status(ErrorKind::InvalidParseState,
                                   "Tree-sitter: Invalid parse state for lookahead iterator");
        }
        return details::Status();
    }

#pragma endregion

#pragma region WASM_STORE

#if defined(CPP_TS_FEATURE_WASM)
    /////////////////////////////////////////////////////////////////////////////
    // WasmStore
    /////////////////////////////////////////////////////////////////////////////

    CPP_TS_API WasmStore::WasmStore(WasmEngine *engine)
    {
        make(engine, impl_).throwIfFailed();
    }

    CPP_TS_API WasmStore::WasmStore(WasmStore &&other) noexcept : impl_{ std::move(other.impl_) }
    {}

    CPP_TS_API WasmStore &WasmStore::operator=(WasmStore &&other) noexcept
    {
        impl_ = std::move(other.impl_);
        return *this;
    }

#if CPP_TS_HAS_EXPECTED
    CPP_TS_API std::expected<WasmStore, Error> WasmStore::create(WasmEngine *engine)
    {
        details::Owned<TSWasmStore> store;
        const details::Status       status = make(engine, store);
        if (!status.ok())
        {
            return details::to_unexpected(status);
        }
        return WasmStore(store.release());
    }
#endif // CPP_TS_HAS_EXPECTED

    CPP_TS_API Language WasmStore::loadLanguage(details::StringViewParameter name,
                                                details::StringViewParameter wasm_buffer)
    {
        Language result;
        loadImpl(name, wasm_buffer, result).throwIfFailed();
        return result;
    }

    CPP_TS_API Language WasmStore::loadLanguage(details::StringViewParameter file_path)
    {
        Language result;
        loadFileImpl(file_path, result).throwIfFailed();
        return result;
    }

    CPP_TS_API std::size_t WasmStore::getLanguageCount() const
    {
        checkValid().throwIfFailed();
        return ts_wasm_store_language_count(impl_.get());
    }

#if CPP_TS_HAS_EXPECTED
    CPP_TS_API std::expected<Language, Error> WasmStore::tryLoadLanguage(details::StringViewParameter name,
                                                                         details::StringViewParameter wasm_buffer)
    {
        Language              result;
        const details::Status status = loadImpl(name, wasm_buffer, result);
        if (!status.ok())
        {
            return details::to_unexpected(status);
        }
        return result;
    }

    CPP_TS_API std::expected<Language, Error> WasmStore::tryLoadLanguage(details::StringViewParameter file_path)
    {
        Language              result;
        const details::Status status = loadFileImpl(file_path, result);
        if (!status.ok())
        {
            return details::to_unexpected(status);
        }
        return result;
    }

    CPP_TS_API std::expected<std::size_t, Error> WasmStore::tryGetLanguageCount() const
    {
        const details::Status status = checkValid();
        if (!status.ok())
        {
            return details::to_unexpected(status);
        }
        return ts_wasm_store_language_count(impl_.get());
    }
#endif // CPP_TS_HAS_EXPECTED

    CPP_TS_API details::Status WasmStore::make(WasmEngine *engine, details::Owned<TSWasmStore> &result)
    {
        TSWasmError error{ TSWasmErrorKindNone, nullptr };
        result.reset(ts_wasm_store_new(engine, &error));

        const details::Status status = details::WasmErrorHelper::to_status(error);
        if (!status.ok())
        {
            result.reset();
            return status;
        }

        if (!result)
        {
            return details::Status(ErrorKind::Unknown, "Tree-sitter: Failed to create Wasm store");
        }
        return details::Status();
    }

    CPP_TS_API details::Status WasmStore::checkValid() const noexcept
    {
        if (!isValid())
        {
            return details::Status(ErrorKind::InvalidObject, "Tree-sitter: Wasm Store is invalid");
        }
        return details::Status();
    }

    CPP_TS_API details::Status WasmStore::loadImpl(details::StringViewParameter name,
                                                   details::StringViewParameter wasm_buffer,
                                                   Language                    &result)
    {
        details::Status status = checkValid();
        if (!status.ok())
        {
            return status;
        }

        status = details::check_length(wasm_buffer.size(), "Input wasm_buffer");
        if (!status.ok())
        {
            return status;
        }

        const std::string name_str(name.data(), name.size());
        TSWasmError       error{ TSWasmErrorKindNone, nullptr };
        const TSLanguage *lang = ts_wasm_store_load_language(impl_.get(),
                                                             name_str.c_str(),
                                                             wasm_buffer.data(),
                                                             static_cast<std::uint32_t>(wasm_buffer.size()),
                                                             &error);

        result = Language(lang);
        return details::WasmErrorHelper::to_status(error);
    }

    CPP_TS_API details::Status WasmStore::loadFileImpl(details::StringViewParameter file_path, Language &result)
    {
        details::Status status = checkValid();
        if (!status.ok())
        {
            return status;
        }

        status = details::check_length(file_path.size(), "Input file_path");
        if (!status.ok())
        {
            return status;
        }

        const std::string path(file_path.data(), file_path.size());
        std::ifstream     file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            return details::Status(ErrorKind::FileError, "Tree-sitter: Failed to open wasm file: " + path);
        }

        const std::streamsize size = file.tellg();
        if (size < 0)
        {
            return details::Status(ErrorKind::FileError, "Tree-sitter: Failed to determine file size: " + path);
        }
        file.seekg(0, std::ios::beg);

        std::string buffer;
        buffer.reserve(static_cast<std::size_t>(size));
        buffer.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        const std::string lang_name = details::get_grammar_name_from_file(path);
        return loadImpl(lang_name, buffer, result);
    }

#endif // defined(CPP_TS_FEATURE_WASM)

#pragma endregion

} // namespace ts
