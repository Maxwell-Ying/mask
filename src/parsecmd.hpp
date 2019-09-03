#ifndef PARSECMD_HPP
#define PARSECMD_HPP

#include <string>
#include <memory>
#include <exception>
#include <regex>
#include <unordered_map>
#include <unordered_set>

const char VECTOR_DELIMITER = ',';

namespace util {
    typedef std::string String;

    template <typename T>
    T toLocalString(T&& t) {
        return std::forward<T>(t);
    }

    inline size_t stringLength(const String &s) {
       return s.length();
    }

    inline String& stringAppend(String &s, String a) {
        return s.append(std::move(a));
    }

    inline String& stringAppend(String& s, size_t n, char c) {
        return s.append(n, c);
    }

    template <typename Iterator>
    String & stringAppend(String& s, Iterator begin, Iterator end) {
        return s.append(begin, end);
    }

    template <typename T>
    String toUTF8String(T&& t) {
        return std::forward<T>(t);
    }

    inline bool empty(const String& s) {
        return s.empty();
    }
} // namespace util

namespace util {
    namespace {
        const String LQUOTE("\'");
        const String RQUOTE("\'");
    }
    class Value : public std::enable_shared_from_this<Value> {
        public:

        virtual ~Value() = default;

        virtual std::shared_ptr<Value> clone() const = 0;

        virtual void parse(const String& text) const = 0;

        virtual void parse() const = 0;

        virtual bool is_container() const = 0;

        virtual bool has_default() const = 0;

        virtual String get_default_value() const = 0;

        virtual bool has_implicit() const = 0;

        virtual String get_implicit_value() const = 0;

        virtual std::shared_ptr<Value> default_value(const String& value) = 0;

        virtual std::shared_ptr<Value> implicit_value(const String& value) = 0;

        virtual std::shared_ptr<Value> no_implicit_value() = 0;

        virtual bool is_boolean() const = 0;
    };

    class OptionException : public std::exception {
        public:
        OptionException(const String& message): m_message(message) {}

        virtual const char * whar() const noexcept {
            return m_message.c_str();
        }

        private:
        String m_message;
    };

    class OptionSpecException : public OptionException {
        public :
        OptionSpecException(const String& message) 
        :OptionException(message) { }
    };

    class OptionParseException : public OptionException {
        public :
        OptionParseException(const String& message) 
        :OptionException(message) { }
    };

    class option_exists_error : public OptionSpecException {
        public:
        option_exists_error(const String& option)
        : OptionSpecException("Option " + LQUOTE + option + RQUOTE + 
        " already exists")  {}
    };

    class invalid_option_format_error : public OptionSpecException {
        public:
        invalid_option_format_error(const String& format)
        : OptionSpecException("Invalid option format " + LQUOTE + format + RQUOTE)
        {}
    };

    class option_syntax_exception : public OptionParseException {
        public:
        option_syntax_exception(const String& text)
        : OptionParseException("Argument " + LQUOTE + text + RQUOTE +
        " starts with a - but has incorrect syntax") {}
    };

    class option_not_exists_exception : public OptionParseException {
        public:
        option_not_exists_exception(const String& option)
        : OptionParseException("Option " + LQUOTE + option + RQUOTE + 
        " does not exist")  {}
    };

    class missing_argument_exception : public OptionParseException {
        public:
        missing_argument_exception(const std::string& option)
        : OptionParseException("Option " + LQUOTE + option + RQUOTE +
        " is missing an argument")  {}
    };

    class option_requires_argument_exception : public OptionParseException {
        public:
        option_requires_argument_exception(const std::string& option)
        : OptionParseException("Option " + LQUOTE + option + RQUOTE + 
        " requires an argument") {}
    };

    class option_not_has_argument_exception : public OptionParseException {
        public:
        option_not_has_argument_exception( const std::string& option, const std::string& arg)
        : OptionParseException("Option " + LQUOTE + option + RQUOTE +
        " does not take an argument, but argument " +
        LQUOTE + arg + RQUOTE + " given") {}
    };

    class option_not_present_exception : public OptionParseException {
        public:
        option_not_present_exception(const std::string& option)
        : OptionParseException("Option " + LQUOTE + option + RQUOTE + 
        " not present") {}
    };

    class argument_incorrect_type : public OptionParseException {
        public:
        argument_incorrect_type(const std::string& arg)
        : OptionParseException("Argument " + LQUOTE + arg + RQUOTE + 
        " failed to parse") {}
    };

    class option_required_exception : public OptionParseException {
        public:
        option_required_exception(const std::string& option)
        : OptionParseException("Option " + LQUOTE + option + RQUOTE + 
        " is required but not present") {}
    };

    template <typename T>
    void throw_or_mimic(const String& text) {
        static_assert(std::is_base_of<std::exception, T>::value,
                      "throw_or_mimic only works on std::exception and deriving classes");
        T exception{text};
        std::cerr << exception.what() << std::endl;
        std::cerr << "Abort (exceptions)" << std::endl;
        std::abort();
    }

    namespace values {
        namespace {
            std::basic_regex<char> integer_pattern
            ("(-)?(0x)?([0-9a-zA-Z]+)|((0x)?0)");
            std::basic_regex<char> truthy_pattern
            ("(t|T)(rue)?|1");
            std::basic_regex<char> falsy_pattern
            ("(f|F)(alse)?|0");
        }

        namespace detail {
            template <typename T, bool B>
            struct SignedCheck;

            template <typename T>
            struct SignedCheck<T, true> {
                template <typename U>
                void operator() (bool negative, U u, const String& text) {
                    if (negative) {
                        if (u>static_cast<U>((std::numeric_limits<T>::min)())) {
                            throw_or_mimic<argument_incorrect_type>(text);
                        }
                    } else {
                        if (u>static_cast<U>((std::numeric_limits<T>::max)())) {
                            throw_or_mimic<argument_incorrect_type>(text);
                        }
                    }
                }
            };

            template <typename T>
            struct SignedCheck<T, false> {
                template <typename U>
                void operator() (bool, U, const String &) {}
            };

            template <typename T, typename U>
            void check_signed_range(bool negative, U value, const String& text) {
                SignedCheck<T, std::numeric_limits<T>::is_signed>()(negative, value, text);
            }
        } // namespace detail
        
        template <typename R, typename T>
        R checked_negative(T&& t, const String&, std::true_type) {
            return -static_cast<R>(t-1)-1;
        }

        template <typename R, typename T>
        T checked_negative(T&& t, const String& text, std::false_type) {
            throw_or_mimic<argument_incorrect_type>(text);
            return t;
        }

        template <typename T>
        void integer_parser(const String& text, T& value) {
            std::smatch match;
            std::regex_match(text, match, integer_pattern);

            if (match.length() == 0) {
                throw_or_mimic<argument_incorrect_type>(text);
            }

            if (match.length(4) > 0) {
                value = 0;
                return;
            }

            using US = typename std::make_unsigned<T>::type;

            constexpr bool is_signed = std::numeric_limits<T>::is_signed;
            const bool negative = match.length(1) > 0;
            const uint8_t base = match.length(2) > 0 ? 16 : 10;

            auto value_match = match[3];

            US result = 0;

            for (auto i=value_match.first; i!= value_match.second; i++) {
                US digit = 0;
                char c = *i;

                if (c >= '0' &&c <= '9') {
                    digit = static_cast<US>(c-'0');
                } else if (base == 16 && c >= 'a' && c <= 'f') {
                    digit = static_cast<US>(c-'a'+10);
                } else if (base == 16 && c >= 'A' && c <= 'F') {
                    digit = static_cast<US>(c-'A'+10);
                } else {
                    throw_or_mimic<argument_incorrect_type>(text);
                }

                US next = result * base + digit;
                if (result > next) {
                    throw_or_mimic<argument_incorrect_type>(text);
                }
                
                result = next;
            }

            detail::check_signed_range<T>(negative, result, text);

            if (negative) {
                value = checked_negative<T>(result, text, std::integral_constant<bool, is_signed>());
            } else {
                value = static_cast<T>(result);
            }
        }

        template <typename T>
        void stringstream_parser(const String& text, T& value) {
            std::stringstream in(text);
            in >> value;
            if (!in) {
                throw_or_mimic<argument_incorrect_type>(text);
            }
        }

        inline void parse_value(const String& text, uint8_t& value) {
            integer_parser(text, value);
        }
        inline void parse_value(const String& text, int8_t& value) {
            integer_parser(text, value);
        }
        inline void parse_value(const String& text, uint16_t& value) {
            integer_parser(text, value);
        }
        inline void parse_value(const String& text, int16_t& value) {
            integer_parser(text, value);
        }
        inline void parse_value(const String& text, uint32_t& value) {
            integer_parser(text, value);
        }
        inline void parse_value(const String& text, int32_t& value) {
            integer_parser(text, value);
        }
        inline void parse_value(const String& text, uint64_t& value) {
            integer_parser(text, value);
        }
        inline void parse_value(const String& text, int64_t& value) {
            integer_parser(text, value);
        }

        inline void parse_value(const String& text, bool& value) {
            std::smatch match;
            std::regex_match(text, match, truthy_pattern);

            if (!match.empty()) {
                value = true;
                return;
            }

            std::regex_match(text, match, falsy_pattern);
            if (!match.empty()) {
                value = false;
                return;
            }

            throw_or_mimic<argument_incorrect_type>(text);
        }

        inline void parse_value(const String& text, String& value) {
            value = text;
        }

        template <typename T>
        void parse_value(const String& text, T& value) {
            stringstream_parser(text, value);
        }

        template <typename T>
        void parse_value(const String& text, std::vector<T>& value) {
            std::stringstream in(text);
            std::string token;
            while (in.eof() == false && std::getline(in, token, VECTOR_DELIMITER)) {
                T v;
                parse_value(token, v);
                value.emplace_back(std::move(v));
            }
        }

        inline void parse_value(const String& text, char& c) {
            if (text.length() != 1) {
                throw_or_mimic<argument_incorrect_type>(text);
            }
            c = text[0];
        }

        template <typename T>
        struct type_is_container {
            static constexpr bool value = false;
        };
        
        template <typename T>
        struct type_is_container<std::vector<T>> {
            static constexpr bool value = true;
        };

        template <typename T>
        class abstract_value : public Value {
            using Self = abstract_value<T>;

            public:
            abstract_value() : m_result(std::make_shared<T>()), m_store(m_result.get()){}

            abstract_value(T* t) : m_store(t) {}

            virtual ~abstract_value() = default;

            abstract_value(const abstract_value& rhs) {
                if (rhs.m_result) {
                    m_result = std::make_shared<T>();
                    m_store = m_result.get();
                } else {
                    m_store = rhs.m_store;
                }
                m_default = rhs.m_default;
                m_implicit = rhs.m_implicit;
                m_default_value = rhs.m_default_value;
                m_implicit_value = rhs.m_implicit_value;
            }

            void parse(const String& text) const {
                parse_value(text, *m_store);
            }

            bool is_container() const {
                return type_is_container<T>::value;
            }

            void parse() const {
                parse_value(m_default_value, *m_store);
            }

            bool has_default() const {
                return m_default;
            }

            bool has_implicit() const {
                return m_implicit;
            }

            std::shared_ptr<Value> default_value(const String& value) {
                m_default = true;
                m_default_value = value;
                return shared_from_this();
            }

            std::shared_ptr<Value> implicit_value(const String& value) {
                m_implicit = true;
                m_implicit_value = value;
                return shared_from_this();
            }

            std::shared_ptr<Value> no_implicit_value() {
                m_implicit = false;
                return shared_from_this();
            }

            String get_default_value() const {
                return m_default_value;
            }

            String get_implicit_value() const {
                return m_implicit_value;
            }

            bool is_boolean() const {
                return std::is_same<T, bool>::value;
            }

            const T& get() const {
                if (m_store == nullptr) {
                    return *m_result;
                } else {
                    return *m_store;
                }
            }

            protected:
            std::shared_ptr<T> m_result;
            T* m_store;

            bool m_default = false;
            bool m_implicit = false;

            String m_default_value;
            String m_implicit_value;
        };

        template <typename T>
        class standard_value : public abstract_value<T> {
            public:
            using abstract_value<T>::abstract_value;

            std::shared_ptr<Value> clone() const {
                return std::make_shared<standard_value<T>>(*this);
            }
        };

        template<>
        class standard_value<bool> : public abstract_value<bool> {
            public:
            ~standard_value() = default;

            standard_value() {
                set_default_and_implicit();
            }

            standard_value(bool* b) : abstract_value(b) {
                set_default_and_implicit();
            }

            std::shared_ptr<Value> clone() const {
                return std::make_shared<standard_value<bool>>(*this);
            }

            private:
            void set_default_and_implicit() {
                m_default = true;
                m_default_value = "false";
                m_implicit = true;
                m_implicit_value = "true";
            }
        };
    } // namespace values
    
    template<typename T>
    std::shared_ptr<Value> value() {
        return std::make_shared<values::standard_value<T>>();
    }

    template<typename T>
    std::shared_ptr<Value> value(T& t) {
        return std::make_shared<values::standard_value<T>>(&t);
    }

    class OptionAdder;

    class OptionDetails {
        public:
        OptionDetails(const String& short_, const String& long_, 
        const String& desc, std::shared_ptr<const Value> val) :
        m_short(short_), m_long(long_), m_desc(desc), m_value(val), m_count(0) {}

        OptionDetails(const OptionDetails& rhs) : m_desc(rhs.m_desc), m_count(rhs.m_count) {
            m_value = rhs.m_value->clone();
        }

        OptionDetails(OptionDetails&& rhs) = default;

        const String& description() const {
            return m_desc;
        }

        const Value& value() const {
            return *m_value;
        }

        std::shared_ptr<Value> make_storage() const {
            return m_value->clone();
        }

        const String& short_name() const {
            return m_short;
        }

        const String& long_name() const {
            return m_long;
        }

        private:
        String m_short;
        String m_long;
        String m_desc;
        std::shared_ptr<const Value> m_value;
        int m_count;
    };

    struct HelpOptionDetails {
        String s;
        String l;
        String desc;
        bool has_default;
        String default_value;
        bool has_implicit;
        String implicit_value;
        String arg_help;
        bool is_container;
        bool is_boolean;
    };

    struct HelpGroupDetails {
        String name;
        String description;
        std::vector<HelpOptionDetails> options;
    };

    class OptionValue {
        public:
        void parse(std::shared_ptr<const OptionDetails> details, const String& text) {
            ensure_value(details);
            ++m_count;
            m_value->parse(text);
        }

        void parse_default(std::shared_ptr<const OptionDetails> details) {
            ensure_value(details);
            m_default = true;
            m_value->parse();
        }

        size_t count() const noexcept {
            return m_count;
        }

        bool has_default() const noexcept {
            return m_count;
        }

        template <typename T>
        const T& as() const {
            if (m_value == nullptr) {
                throw_or_mimic<std::domain_error>("No value");
            }
            return dynamic_cast<const values::standard_value<T>&>(*m_value).get();
        }

        private:
        void ensure_value(std::shared_ptr<const OptionDetails> details) {
            if (m_value == nullptr) {
                m_value = details->make_storage();
            }
        }

        std::shared_ptr<Value> m_value;
        size_t m_count = 0;
        bool m_default = false;
    };
    class KeyValue {
        public:
        KeyValue(String key_, String value_): 
        m_key(std::move(key_)), m_value(std::move(value_)) {}

        const String& key() const {
            return m_key;
        }
        
        const String& value() const {
            return m_value;
        }

        template<typename T>
        T as() const {
            T result;
            values::parse_value(m_value, result);
            return result;
        }

        private:
        String m_key;
        String m_value;
    };

    class ParseResult {
        public:
        ParseResult(const std::shared_ptr<
                           std::unordered_map<String, 
                            std::shared_ptr<OptionDetails>>>,
                    std::vector<String>, bool allow_unrecognised, 
                    int&, char**&);
        
        size_t count(const String& o) const {
            auto iter = m_options->find(o);
            if (iter == m_options->end()) {
                return 0;
            }
            auto riter = m_results.find(iter->second);

            return riter->second.count();
        }

        const OptionValue& operator[](const String& option) const {
            auto iter = m_options->find(option);
            if (iter == m_options->end()) {
                throw_or_mimic<option_not_present_exception>(option);
            }
            auto riter = m_results.find(iter->second);
            return riter->second;
        }

        const std::vector<KeyValue>& argumemts() const {
            return m_sequential;
        }

        private:
        void parse(int& argc, char**& argv);

        void add_to_option(const String& option, const String& arg);

        bool consume_positional(String a);

        void parse_option(std::shared_ptr<OptionDetails> value,
                          const String& name, const String& arg="");

        void parse_default(std::shared_ptr<OptionDetails> details);

        void checked_parse_arg(int argc, char* argv[], int& crrent, 
                             std::shared_ptr<OptionDetails> value, 
                             const String& name);

        const std::shared_ptr<
            std::unordered_map<std::string, std::shared_ptr<OptionDetails>>
            > m_options;
        std::vector<std::string> m_positional;
        std::vector<std::string>::iterator m_next_positional;
        std::unordered_set<std::string> m_positional_set;
        std::unordered_map<std::shared_ptr<OptionDetails>, OptionValue> m_results;

        bool m_allow_unrecognised;

        std::vector<KeyValue> m_sequential;
    };

    struct Option {
        Option(const String& opts, const String& desc,
               const std::shared_ptr<const Value>& value = ::util::value<bool>(),
               const String& arg_help = "") :
               opts_(opts), desc_(desc), value_(value), arg_help_(arg_help) {}
        
        String opts_;
        String desc_;
        std::shared_ptr<const Value> value_;
        String arg_help_;
    };

    class Options {
        typedef std::unordered_map<std::string, std::shared_ptr<OptionDetails>>
            OptionMap;
        public:

        Options(std::string program, String help_string = "")
        : m_program(std::move(program))
        , m_help_string(toLocalString(std::move(help_string)))
        , m_custom_help("[OPTION]")
        , m_positional_help("positional parameters")
        , m_show_positional(false)
        , m_allow_unrecognised(false)
        , m_options(std::make_shared<OptionMap>())
        , m_next_positional(m_positional.end()) {}

        Options& positional_help(String help_text) {
            m_positional_help = std::move(help_text);
            return *this;
        }

        Options& custom_help(String help_text) {
            m_custom_help = std::move(help_text);
            return *this;
        }

        Options& show_positional_help() {
            m_show_positional = true;
            return *this;
        }

        Options& allow_unrecognised_options() {
            m_allow_unrecognised = true;
            return *this;
        }

        ParseResult parse(int& argc, char**& argv);

        OptionAdder add_options(String group = "");

        void add_options(const String& group, 
                         std::initializer_list<Option> options);

        void add_option(const String& group, const Option& option);

        void add_option(const String& group, const String& s,
                        const String& l, String desc,
                        std::shared_ptr<const Value> value, String arg_help);
        
        void parse_positional(String option);

        void parse_positional(std::vector<String> options);
        
        void parse_positional(std::initializer_list<String> options);

        template <typename Iterator>
        void parse_positional(Iterator begin, Iterator end) {
            parse_positional(std::vector<String>{begin, end});
        }

        String help(const std::vector<String>& groups = {}) const;

        const std::vector<String> groups() const;

        const HelpGroupDetails& group_help(const String& group) const;

        private:

        void add_one_option(const String& option,
                            std::shared_ptr<OptionDetails> details);

        String help_one_group(const String& group) const;

        void generate_group_help(String& result, 
                                const std::vector<String>& groups) const;
        
        void generate_all_groups_help(String& result) const;


        std::string m_program;
        String m_help_string;
        std::string m_custom_help;
        std::string m_positional_help;
        bool m_show_positional;
        bool m_allow_unrecognised;

        std::shared_ptr<OptionMap> m_options;
        std::vector<std::string> m_positional;
        std::vector<std::string>::iterator m_next_positional;
        std::unordered_set<std::string> m_positional_set;

        //mapping from groups to help options
        std::map<std::string, HelpGroupDetails> m_help;
    };

    class OptionAdder {
        public:
        OptionAdder(Options& options, String group)
        : m_options(options), m_group(std::move(group)) {}

        OptionAdder& operator() (const String& opts, const String& desc,
                                 std::shared_ptr<const Value> value = ::util::value<bool>(),
                                 String arg_help = "");
        
        private:
        Options& m_options;
        String m_group;
    };

    namespace {
        constexpr int OPTION_LONGEST = 30;
        constexpr int OPTION_DESC_GAP = 2;

        std::basic_regex<char> option_matcher(
            "--([[:alnum:]][-_[:alnum:]]+)(=(.*))?|-([[:alnum:]]+)");
        
        std::basic_regex<char> option_specifier(
            "(([[:alnum:]]),)?[ ]*([[:alnum:]][-_[:alnum:]]*)?");

        String format_option(const HelpOptionDetails& o) {
            auto& s = o.s;
            auto& l = o.l;

            String result = "  ";

            if (s.size() > 0) {
                result += "-" + toLocalString(s) + ",";
            } else {
                result += "  ";
            }

            if (l.size() > 0) {
                result += " --" + toLocalString(l);
            }

            auto arg = o.arg_help.size() > 0? toLocalString(o.arg_help) : "arg";
            if (!o.is_boolean) {
                if (o.has_implicit) {
                    result += " [=" + arg + "(+" + toLocalString(o.implicit_value) + ")]";
                } else {
                    result += " " + arg;
                }
            }
            return result;
        }

        String format_description(const HelpOptionDetails& o, size_t start, size_t width) {
            auto desc = o.desc;

            if (o.has_default && (!o.is_boolean || o.default_value != "false")) {
                desc += toLocalString(" (default: " + o.default_value + ")");
            }
            String result;
            auto current = std::begin(desc);
            auto startLine = current;
            auto lastSpace = current;
            auto size = size_t{};

            while (current != std::end(desc)) {
                if (*current == ' ') {
                    lastSpace = current;
                }
                if (*current == '\n') {
                    startLine = current+1;
                    lastSpace = startLine;
                } else if (size > width) {
                    if (lastSpace == startLine) {
                        stringAppend(result, startLine, current+1);
                        stringAppend(result, "\n");
                        stringAppend(result, start, ' ');
                        startLine = current + 1;
                        lastSpace = startLine;
                    } else {
                        stringAppend(result, startLine, lastSpace);
                        stringAppend(result, "\n");
                        stringAppend(result, start, ' ');
                        startLine = lastSpace + 1;
                    }
                    size = 0;
                } else {
                    ++ size;
                }
                ++ current;
            }
            stringAppend(result, startLine, current);
            return result;
        }
    }

    inline ParseResult::ParseResult(
        const std::shared_ptr<std::unordered_map<String, std::shared_ptr<OptionDetails>>> options,
        std::vector<String> positional, bool allow_unrecognized, int& argc, char**& argv) 
        : m_options(options), m_positional(std::move(positional))
        , m_next_positional(m_positional.begin()), m_allow_unrecognised(allow_unrecognized) {
        parse(argc, argv);
    }
    
    inline void Options::add_options(const String& group, 
                                     std::initializer_list<Option> options) {
        OptionAdder option_adder(*this, group);
        for (const auto & option: options) {
            option_adder(option.opts_, option.desc_, option.value_, option.arg_help_);
        }
    }

    inline OptionAdder Options::add_options(String group) {
        return OptionAdder(*this, std::move(group));
    }

    inline OptionAdder& OptionAdder::operator()(const String& opts, const String& desc,
                                                std::shared_ptr<const Value> value, 
                                                String arg_help) {
        std::match_results<const char*> result;
        std::regex_match(opts.c_str(), result, option_specifier);

        if (result.empty()) {
            throw_or_mimic<invalid_option_format_error>(opts);
        }
        const auto& short_match = result[2];
        const auto& long_match = result[3];

        if (!short_match.length() && !long_match.length()) {
            throw_or_mimic<invalid_option_format_error>(opts);
        } else if (long_match.length() == 1 && short_match.length()) {
            throw_or_mimic<invalid_option_format_error>(opts);
        }

        auto option_names = [](const std::sub_match<const char*>& short_, 
                               const std::sub_match<const char*>& long_) {
            if (long_.length() == 1) {
                return std::make_tuple(long_.str(), short_.str());
            } else {
                return std::make_tuple(short_.str(), long_.str());
            }
        } (short_match, long_match);

        m_options.add_option(m_group, std::get<0>(option_names), std::get<1>(option_names),
                             desc, value, std::move(arg_help));
        
        return *this;
    }

    inline void ParseResult::parse_default(std::shared_ptr<OptionDetails> details) {
        m_results[details].parse_default(details);
    }

    inline void ParseResult::parse_option(std::shared_ptr<OptionDetails> value, 
                                          const String& , const String& arg) {
        auto & result = m_results[value];
        result.parse(value, arg);
        m_sequential.emplace_back(value->long_name(), arg);
    }

    inline void ParseResult::checked_parse_arg(int argc, char* argv[], int& current,
        std::shared_ptr<OptionDetails> value, const String& name) {
        if (current + 1 >= argc) {
            if (value->value().has_implicit()) {
                parse_option(value, name, value->value().get_implicit_value());
            } else {
                throw_or_mimic<missing_argument_exception>(name);
            }
        } else {
            if (value->value().has_implicit()) {
                parse_option(value, name, value->value().get_implicit_value());
            } else {
                parse_option(value, name, argv[current+1]);
                ++current;
            }
        }
    }

    inline void ParseResult::add_to_option(const String& option, const String& arg) {
        auto iter = m_options->find(option);
        if (iter == m_options->end()) {
            throw_or_mimic<option_not_exists_exception>(option);
        }
        parse_option(iter->second, option, arg);
    }

    inline bool ParseResult::consume_positional(String a) {
        while (m_next_positional != m_positional.end()) {
            auto iter = m_options->find(*m_next_positional);
            if (iter != m_options->end()) {
                auto& result = m_results[iter->second];
                if (!iter->second->value().is_container()) {
                    if (result.count() == 0) {
                        add_to_option(*m_next_positional, a);
                        ++m_next_positional;
                        return true;
                    } else {
                        ++m_next_positional;
                        continue;
                    }
                } else {
                    add_to_option(*m_next_positional, a);
                    return true;
                }
            } else {
                throw_or_mimic<option_not_exists_exception>(*m_next_positional);
            }
        }
        return false;
    }

    inline void Options::parse_positional(String option) {
        parse_positional(std::vector<String>{std::move(option)});
    }

    inline void Options::parse_positional(std::vector<String> options) {
        m_positional = std::move(options);
        m_next_positional = m_positional.begin();
        m_positional_set.insert(m_positional.begin(), m_positional.end());
    }

    inline void Options::parse_positional(std::initializer_list<String> options) {
        parse_positional(std::vector<String>(std::move(options)));
    }

    inline ParseResult Options::parse(int& argc, char**& argv) {
        ParseResult result(m_options, m_positional, m_allow_unrecognised, argc, argv);
        return result;
    }

    inline void ParseResult::parse(int& argc, char**& argv) {
        int current = 1;
        int nextKeep = 1;
        bool consume_remaining = false;

        while (current != argc) {
            if (strcmp(argv[current], "--") == 0) {
                consume_remaining = true;
                ++ current;
                break;
            }

            std::match_results<const char*> result;
            std::regex_match(argv[current], result, option_matcher);

            if (result.empty()) {
                if (argv[current][0] == '-' && argv[current][1] != '\0') {
                    if (!m_allow_unrecognised) {
                        throw_or_mimic<option_syntax_exception>(argv[current]);
                    }
                }
                if (consume_positional(argv[current])) {
                } else {
                    argv[nextKeep] = argv[current];
                    ++nextKeep;
                }
            } else {
                if (result[4].length() != 0) {
                    const String & s = result[4];
                    for (std::size_t i=0; i!= s.size(); i++) {
                        String name(1, s[i]);
                        auto iter = m_options->find(name);
                        if (iter == m_options->end()) {
                            if (m_allow_unrecognised) {
                                continue;
                            } else {
                                throw_or_mimic<option_not_exists_exception>(name);
                            }
                        }

                        auto value = iter->second;
                        if (i+1 == s.size()) {
                            checked_parse_arg(argc, argv, current, value, name);
                        } else if (value->value().has_implicit()) {
                            parse_option(value, name, value->value().get_implicit_value());
                        } else {
                            throw_or_mimic<option_requires_argument_exception>(name);
                        }
                    }
                } else if (result[1].length() != 0) {
                    const String& name = result[1];
                    auto iter = m_options->find(name);
                    if (iter == m_options->end()) {
                        if (m_allow_unrecognised) {
                            argv[nextKeep] = argv[current];
                            ++nextKeep;
                            ++current;
                            continue;
                        } else {
                            throw_or_mimic<option_not_exists_exception>(name);
                        }
                    }
                    auto opt = iter->second;

                    if (result[2].length() != 0) {
                        parse_option(opt, name, result[3]);
                    } else {
                        checked_parse_arg(argc, argv, current, opt, name);
                    }
                }
            }
            ++current;
        }

        for (auto& opt: *m_options) {
            auto& detail = opt.second;
            auto& value = detail->value();

            auto& store = m_results[detail];
            if (value.has_default() && !store.count() && !store.has_default()) {
                parse_default(detail);
            }
        }

        if (consume_remaining) {
            while (current < argc) {
                if (!consume_positional(argv[current])) {
                    break;
                }
                ++current;
            }
            while (current != argc) {
                argv[nextKeep] = argv[current];
                ++nextKeep;
                ++current;
            }
        }

        argc = nextKeep;
    }

    inline void Options::add_option(const String& group, const Option& option) {
        add_options(group, {option});
    }

    inline void Options::add_option(const String& group, const String& s, const String& l,
        String desc, std::shared_ptr<const Value> value, String arg_help) {
        auto stringDesc = toLocalString(std::move(desc));
        auto option = std::make_shared<OptionDetails>(s, l, stringDesc, value);

        if (s.size() > 0) {
            add_one_option(s, option);
        }
        if (l.size() > 0) {
            add_one_option(l, option);
        }

        auto& options = m_help[group];

        options.options.emplace_back(HelpOptionDetails{
            s, l, stringDesc, value->has_default(), value->get_default_value(),
            value->has_implicit(), value->get_implicit_value(), std::move(arg_help),
            value->is_container(), value->is_boolean()
        });
    }

    inline void Options::add_one_option(const String& option, std::shared_ptr<OptionDetails> details) {
        auto in = m_options->emplace(option, details);
        if (!in.second) {
            throw_or_mimic<option_exists_error>(option);
        }
    }

    inline String Options::help_one_group(const String& g) const {
        typedef std::vector<std::pair<String, String>> OptionHelp;

        auto group = m_help.find(g);
        if (group == m_help.end()) {
            return "";
        }
        OptionHelp format;
        size_t longest = 0;
        String result;

        if (!g.empty()) {
            result += toLocalString(" " + g + " options:\n");
        }
        for (const auto& o: group->second.options) {
            if (m_positional_set.find(o.l) != m_positional_set.end() && !m_show_positional) {
                continue;
            }
            auto s = format_option(o);
            longest = (std::max)(longest, stringLength(s));
            format.push_back(std::make_pair(s, String()));
        }
        longest = (std::min)(longest, static_cast<size_t>(OPTION_LONGEST));

        auto allowed = size_t{76} - longest - OPTION_DESC_GAP;

        auto fiter = format.begin();
        for (const auto& o: group->second.options) {
            if (m_positional_set.find(o.l)!= m_positional_set.end() && !m_show_positional) {
                continue;
            }
            auto d = format_description(o, longest+OPTION_DESC_GAP, allowed);

            result += fiter->first;
            if (stringLength(fiter->first) > longest) {
                result += '\n';
                result += toLocalString(String(longest+OPTION_DESC_GAP, ' '));
            } else {
                result += toLocalString(String(longest+OPTION_DESC_GAP - stringLength(fiter->first), ' '));
            }
            result += d;
            result += '\n';

            ++fiter;
        }
        return result;
    }

    inline void Options::generate_group_help(String& result, 
        const std::vector<String>& print_groups) const {
        for (size_t i=0; i!= print_groups.size(); ++i) {
            const String& group_help_text = help_one_group(print_groups[i]);
            if (empty(group_help_text)) {
                continue;
            }
            result += group_help_text;
            if (i<print_groups.size() - 1) {
                result += '\n';
            }
        }
    }

    inline void Options::generate_all_groups_help(String& result) const {
        std::vector<String> all_group;
        all_group.reserve(m_help.size());

        for (auto& group : m_help) {
            all_group.push_back(group.first);
        }
        generate_group_help(result, all_group);
    }

    inline String Options::help(const std::vector<String>& help_groups) const {
        String result = m_help_string + "\nUsage:\n  " +
                        toLocalString(m_program) + " " +
                        toLocalString(m_custom_help);
        
        if (m_positional.size() > 0 && m_positional_help.size() > 0) {
            result += " " + toLocalString(m_positional_help);
        }

        result += "\n\n";

        if (help_groups.size() == 0) {
            generate_all_groups_help(result);
        } else {
            generate_group_help(result, help_groups);
        }
        return toUTF8String(result);
    }
    inline const std::vector<String> Options::groups() const {
        std::vector<String> g;
        std::transform(m_help.begin(), m_help.end(), std::back_inserter(g),
            [](const std::map<String, HelpGroupDetails>::value_type& pair){return pair.first;});
        return g;
    }

    inline const HelpGroupDetails& Options::group_help(const String& group) const {
        return m_help.at(group);
    }
}

#endif