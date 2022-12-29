#pragma once

#include <cassert>

#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <fstream>
#include <variant>

#include "symbol.h"


namespace tiny {

    class SourceLocation {
    public:
        SourceLocation(std::string const & filename, size_t line, size_t col):
            line_(line),
            col_(col) {
            Filenames & f = Filenames_();
            auto i = f.lookup.find(filename);
            if (i == f.lookup.end()) {
                i = f.lookup.insert(std::make_pair(filename, f.names.size())).first;
                f.names.push_back(filename);
            }
            file_ = i->second;
        }

        std::string const & file() const {
            return Filenames_().names[file_];
        }

        size_t line() const {
            return line_;
        }

        size_t col() const {
            return col_;
        }

    private:

        friend class Lexer;

        size_t file_;
        size_t line_;
        size_t col_;

        struct Filenames {
            std::vector<std::string> names;
            std::unordered_map<std::string, size_t> lookup;
        };

        static Filenames & Filenames_() {
            static Filenames singleton;
            return singleton;
        }

        friend std::ostream & operator << (std::ostream & s, SourceLocation const & l) {
            s << l.file() << " [" << l.line() << ", " << l.col() << "]";
            return s;
        }

    }; // tiny::SourceLocation

    class Token {
    public:
        enum class Kind {
            EoF,
            Identifier,
            Operator,
            Integer,
            Double,
            StringSingleQuoted,
            StringDoubleQuoted
        }; // tiny::Token::Kind

        static Token EoF(SourceLocation const & l) {
            return Token{Token::Kind::EoF, l};
        }

        Token(std::string const & symbol, SourceLocation const & l, bool isOperator = false):
            kind_{isOperator ? Kind::Operator : Kind::Identifier},
            location_{l},
            contents_{Symbol{symbol}} {
        }

        Token(char symbol, SourceLocation const & l):
            kind_{Kind::Identifier},
            location_{l},
            contents_{Symbol{std::string{symbol}}} {
        }

        Token(int value, SourceLocation const & l):
            kind_{Kind::Integer},
            location_{l},
            contents_{value} {
        }

        Token(double value, SourceLocation const & l):
            kind_{Kind::Double},
            location_{l},
            contents_{value} {
        }

        Token(std::string const & literal, char quote, SourceLocation const & l):
            kind_{quote == '"' ? Kind::StringDoubleQuoted : Kind::StringSingleQuoted },
            location_{l},
            contents_{literal} {
        }

        Kind kind() const {
            return kind_;
        }

        SourceLocation const & location() const {
            return location_;
        }

        Symbol valueSymbol() const {
            assert(kind_ == Kind::Identifier || kind_ == Kind::Operator);
            return std::get<Symbol>(contents_);
        }

        int valueInt() const {
            assert(kind_ == Kind::Integer);
            return std::get<int>(contents_);
        }

        double valueDouble() const {
            assert(kind_ == Kind::Double);
            return std::get<double>(contents_);
        }

        std::string const & valueString() const {
            assert(kind_ == Kind::StringSingleQuoted || kind_ == Kind::StringDoubleQuoted);
            return std::get<std::string>(contents_);
        }

        bool operator == (Kind kind) const {
            return kind_ == kind;
        }

        bool operator != (Kind kind) const {
            return kind_ != kind;
        }

        bool operator == (Symbol symbol) const {
            return (kind_ == Kind::Identifier || kind_ == Kind::Operator) && valueSymbol() == symbol;
        }

        bool operator != (Symbol symbol) const {
            return (kind_ != Kind::Identifier && kind_ != Kind::Operator) || valueSymbol() != symbol;
        }

    private:

        Token(Kind kind, SourceLocation const & l):
            kind_{kind},
            location_{l} {
        }

        Kind kind_;
        SourceLocation location_;

        std::variant<int, double, Symbol, std::string> contents_;

        friend std::ostream & operator << (std::ostream & s, Token::Kind kind) {
            switch (kind) {
                case Kind::EoF:
                    s << "EoF";
                    break;
                case Kind::Identifier:
                    s << "identifier";
                    break;
                case Kind::Operator:
                    s << "operator";
                    break;
                case Kind::Integer:
                    s << "integer literal";
                    break;
                case Kind::Double:
                    s << "double literal";
                    break;
                case Kind::StringSingleQuoted:
                    s << "single quoted string";
                    break;
                case Kind::StringDoubleQuoted:
                    s << "double quoted string";
                    break;
                default:
                    s << "!! Unknown token kind";

            }
            return s;
        }

        friend std::ostream & operator << (std::ostream & s, Token const & t) {
            switch (t.kind()) {
                case Kind::EoF:
                    s << "EoF";
                    break;
                case Kind::Identifier:
                case Kind::Operator:
                    s << t.valueSymbol().name();
                    break;
                case Kind::Integer:
                    s << t.valueInt();
                    break;
                case Kind::Double:
                    s << t.valueDouble();
                    break;
                case Kind::StringSingleQuoted:
                    s << '\'' << t.valueString() << '\'';
                    break;
                case Kind::StringDoubleQuoted:
                    s << '"' << t.valueString() << '"';
                    break;
                default:
                    s << "!! Unknown token kind";
            }
            return s;
        }

        friend std::ostream & operator << (std::ostream & s, std::vector<Token> const & tokens) {
            std::string filename;
            for (Token const & t : tokens) {
                if (filename != t.location().file()) {
                    filename = t.location().file();
                    s << filename << ":" << std::endl;
                }
                s << "    " << t << " [" << t.location().line() << ", " << t.location().col() << "]" << std::endl;
            }
            return s;
        }

    }; // tiny::Token

    class ParserError : public std::runtime_error {
    public:
        ParserError(std::string const & what, SourceLocation const & location,  bool eof = false):
            std::runtime_error{what},
            location_{location},
            eof_{eof} {
        }

        SourceLocation const & location() const {
            return location_;
        }

        bool eof() const {
            return eof_;
        }

    private:
        SourceLocation location_;
        bool eof_;
    };

    /** Lexical Analyzer common for all tiny languages.

     */
    class Lexer {
    public:

        static std::vector<Token> TokenizeFile(std::string const & filename) {
            std::ifstream f;
            f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            f.open(filename);
            Lexer l(f, filename);
            l.tokenize();
            return std::move(l.tokens_);
        }

        static std::vector<Token> Tokenize(std::string const & text, std::string const & filename) {
            std::stringstream s{text};
            Lexer l{s, filename};
            l.tokenize();
            return std::move(l.tokens_);
        }

        static bool IsLetter(char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        }

        static int IsDigit(char c, size_t base = 10) {
            switch (base) {
                case 10:
                    if (c >= '0' && c <= '9')
                        return c - '0';
                    break;
                default:
                    assert(false && "Unknown base");
            }
            return -1;
        }

        static bool IsIdentifierStart(char c) {
            return IsLetter(c) || c == '_';
        }

        static bool IsIdentifier(char c) {
            return IsIdentifierStart(c) || IsDigit(c) >= 0;
        }

    private:

        Lexer(std::istream & input, std::string const & filename):
            i_{input},
            l_{filename, 1, 1} {
        }

        /** Actually does the tokenization.
         */
        void tokenize() {
            while (! eof()) {
                char c = top();
                // skip whitespace
                if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                    pop();
                    continue;
                }
                SourceLocation start{l_};
                // determine what we have here
                c = pop();
                switch (c) {
                    case '/':
                        if (condPop('/'))
                            singleLineComment();
                        else if (condPop('*'))
                            multiLineComment(start);
                        else if (condPop('='))
                            tokens_.push_back(Token{"/=", start, true});
                        else
                            tokens_.push_back(Token{"/", start, true});
                        break;
                    case '+':
                        if (condPop('+'))
                            tokens_.push_back(Token{"++", start, true});
                        else if (condPop('='))
                            tokens_.push_back(Token{"+=", start, true});
                        else
                            tokens_.push_back(Token{"+", start, true});
                        break;
                    case '-':
                        if (condPop('-'))
                            tokens_.push_back(Token{"--", start, true});
                        else if (condPop('='))
                            tokens_.push_back(Token{"-=", start, true});
                        else if (condPop('>'))
                            tokens_.push_back(Token{"->", start, true});
                        else
                            tokens_.push_back(Token{"-", start, true});
                        break;
                    case '*':
                        if (condPop('='))
                            tokens_.push_back(Token{"*=", start, true});
                        else
                            tokens_.push_back(Token{"*", start, true});
                        break;
                    case '!':
                        if (condPop('='))
                            tokens_.push_back(Token{"!=", start, true});
                        else
                            tokens_.push_back(Token{"!", start, true});
                        break;
                    case '=':
                        if (condPop('='))
                            tokens_.push_back(Token{"==", start, true});
                        else
                            tokens_.push_back(Token{"=", start, true});
                        break;
                    case '<':
                        if (condPop('<'))
                            tokens_.push_back(Token{"<<", start, true});
                        else if (condPop('='))
                            tokens_.push_back(Token{"<=", start, true});
                        else
                            tokens_.push_back(Token{"<", start, true});
                        break;
                    case '>':
                        if (condPop('>'))
                            tokens_.push_back(Token{">>", start, true});
                        else if (condPop('='))
                            tokens_.push_back(Token{">=", start, true});
                        else
                            tokens_.push_back(Token{">", start, true});
                        break;
                    case '|':
                        if (condPop('|'))
                            tokens_.push_back(Token{"||", start, true});
                        else
                            tokens_.push_back(Token{"|", start, true});
                        break;
                    case '&':
                        if (condPop('&'))
                            tokens_.push_back(Token{"&&", start, true});
                        else
                            tokens_.push_back(Token{"&", start, true});
                        break;
                    case '%':
                    case '.':
                    case ',':
                    case ';':
                    case ':':
                    case '?':
                    case '[':
                    case ']':
                    case '(':
                    case ')':
                    case '{':
                    case '}':
                    case '~':
                    case '`':
                        tokens_.push_back(Token{c, start});
                        break;
                    case '\'':
                    case '"':
                        stringLiteral(start, c);
                        break;
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        numericLiteral(start, c);
                        break;
                    default:
                        if (IsIdentifierStart(c))
                            identifier(start, c);
                        else
                            throw ParserError{"Undefined character", l_};
                }
            }
            // make sure that last token is always end of file
            tokens_.push_back(Token::EoF(l_));
        }

        void singleLineComment() {
            while (!eof() && top() != '\n')
                pop();
        }

        void multiLineComment(SourceLocation start) {
            while (true) {
                if (eof())
                    throw ParserError("Unterminated multi-line comment", start, true);
                if (condPop('*') && !eof() && condPop('/'))
                    return;
                if (!eof())
                    pop();
            }
        }

        void stringLiteral(SourceLocation start, char quote) {
            std::string literal;
            while (!condPop(quote)) {
                if (eof())
                    throw ParserError{"Unterminated string literal", start, true};
                if (condPop('\\')) {
                    if (eof())
                        throw ParserError{"Unterminated escape sequence", l_, true};
                    char c = pop();
                    switch (c) {
                        case '\'':
                        case '\\':
                        case '"':
                            literal += c;
                            break;
                        case '\n':
                            break;
                        case 'n':
                            literal += '\n';
                            break;
                        case 'r':
                            literal += '\r';
                            break;
                        case 't':
                            literal += '\t';
                            break;
                        default:
                            throw (ParserError{"Unsupported escape character", l_});
                    }
                } else {
                    literal += pop();
                }
            }
            tokens_.push_back(Token{literal, quote, start});
        }

        // TODO support more bases
        void numericLiteral(SourceLocation start, char firstDigit) {
            int number = IsDigit(firstDigit, 10);
            assert(number >= 0);
            while (!eof()) {
                int next = IsDigit(top());
                if (next < 0)
                    break;
                number = (number * 10) + next;
                pop();
            }
            // see if we are dealing with a floating point number
            if (condPop('.')) {
                if (eof() || IsDigit(top()) == -1)
                    throw ParserError{"Digit must follow after decimal dot", l_, eof()};
                double fraction = number;
                double divider = 10;
                while (! eof()) {
                    int next = IsDigit(top());
                    if (next < 0)
                        break;
                    fraction = fraction + (next / divider);
                    divider *= 10;
                    pop();
                }
                tokens_.push_back(Token{fraction, start});
            } else {
                tokens_.push_back(Token{number, start});
            }
        }

        void identifier(SourceLocation start, char firstLetter) {
            std::string ident{firstLetter};
            while (!eof() && IsIdentifier(top()))
                ident += pop();
            tokens_.push_back(Token{ident, start});
        }

        bool eof() const {
            return i_.peek() == EOF;
        }

        char top() {
            assert(! eof());
            return static_cast<char>(i_.peek());
        }

        char pop() {
            assert(! eof());
            char c = static_cast<char>(i_.get());
            if (c == '\n') {
                l_.col_ = 1;
                ++l_.line_;
            } else {
                ++l_.col_;
            }
            return c;
        }

        bool condPop(char c) {
            if (eof() || top() != c)
                return false;
            pop();
            return true;
        }

        std::istream & i_;
        SourceLocation l_;

        std::vector<Token> tokens_;

    }; // tiny::Lexer

} // namespace tiny