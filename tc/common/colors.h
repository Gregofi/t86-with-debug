#pragma once

#if (defined _WIN32)
#include <Windows.h>
#undef min
#undef max
#endif

#include <cassert>
#include <iostream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <optional>
#include <ranges>
#include <sstream>
#include "symbol.h"


namespace colors {

    /** Initializes the terminal on Windows.

        While on Linux color escape sequences work by default, this may not be the case on Windows so this simple initializer sets the console mode to process output terminal sequences which allows us to do all kinds of pretty things.

        Note that this only works on recent Windows 10 versions and will not compile on older systems, which frankly is not a problem in 2021.
     */
    inline void initializeTerminal() {
#if (defined _WIN32)
        HANDLE cout = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD cmode;
        GetConsoleMode(cout, & cmode);
        cmode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(cout, cmode);
#endif
    }

    enum class color {
        reset,
        black,
        red,
        green,
        yellow,
        blue,
        magenta,
        cyan,
        white,
        gray,
        lightRed,
        lightGreen,
        lightYellow,
        lightBlue,
        lightMagenta,
        lightCyan,
        lightGray,
    };

    inline std::ostream & operator << (std::ostream & s, color c) {
        switch (c) {
        case color::black:
            s << "\033[30m";
            break;
        case color::red:
            s << "\033[31m";
            break;
        case color::green:
            s << "\033[32m";
            break;
        case color::yellow:
            s << "\033[33m";
            break;
        case color::blue:
            s << "\033[34m";
            break;
        case color::magenta:
            s << "\033[35m";
            break;
        case color::cyan:
            s << "\033[36m";
            break;
        case color::lightGray:
            s << "\033[37m";
            break;
        case color::reset:
            s << "\033[39m";
            break;
        case color::gray:
            s << "\033[90m";
            break;
        case color::lightRed:
            s << "\033[91m";
            break;
        case color::lightGreen:
            s << "\033[92m";
            break;
        case color::lightYellow:
            s << "\033[93m";
            break;
        case color::lightBlue:
            s << "\033[94m";
            break;
        case color::lightMagenta:
            s << "\033[95m";
            break;
        case color::lightCyan:
            s << "\033[96m";
            break;
        case color::white:
            s << "\033[97m";
            break;
        }
        return s;
    }

    inline char const * bg(color c) {
        switch (c) {
        case color::reset:
            return "\033[49m";
        case color::black:
            return "\033[40m";
        case color::red:
            return "\033[41m";
        case color::green:
            return "\033[42m";
        case color::yellow:
            return "\033[43m";
        case color::blue:
            return "\033[44m";
        case color::magenta:
            return "\033[45m";
        case color::cyan:
            return "\033[46m";
        case color::white:
            return "\033[47m";
        case color::gray:
            return "\033[100m";
        case color::lightRed:
            return "\033[101m";
        case color::lightGreen:
            return "\033[102m";
        case color::lightYellow:
            return "\033[103m";
        case color::lightBlue:
            return "\033[104m";
        case color::lightMagenta:
            return "\033[105m";
        case color::lightCyan:
            return "\033[106m";
        case color::lightGray:
            return "\033[107m";
        default:
            assert(false && "Color not supported");
            return "";
        }
    }

    /** A simple color printer that allows some basic output formatting and manipulation.
     */
    class ColorPrinter {
    public:

        using Manipulator = std::function<void (ColorPrinter &)>;

        struct ManipulatorStr {
            using ManipulatorFunction = std::function<void (ColorPrinter &, std::string const &)>;
            ManipulatorFunction f;
            std::string value;
        };

        template<typename T>
        static std::string colorize(T & what) {
            std::stringstream ss;
            ColorPrinter p{ss};
            p.startLine();
            what.print(p);
            p << color::reset;
            return ss.str();
        }

        ColorPrinter(std::ostream & s):
            s_{s} {
        }

        size_t indent() const { return indent_; }
        void setIndent(size_t indent) { indent_ = indent; }

        void startLine() {
            if (lineNumbers) {
                s_ << std::setw(6) << lineNumber << (++lineNumber_) << std::setw(0) << " ";
            }
            if (indent_ != 0)
                s_ << std::setw(indent_) << " " << std::setw(0);
        }

        void newLine() {
            s_ << std::endl;
            startLine();
        }

        color identifier = color::lightGreen;
        color keyword = color::green;
        color symbol = color::lightBlue;
        color numberLiteral = color::lightRed;
        color charLiteral = color::lightMagenta;
        color stringLiteral = color::lightMagenta;
        color comment = color::gray;
        color type = color::cyan;
        color lineNumber = color::gray;

        size_t tabSize = 4;
        bool lineNumbers = true;

        ColorPrinter & operator << (Manipulator  manip) {
            manip(*this);
            return *this;
        }

        ColorPrinter & operator << (ManipulatorStr manip) {
            manip.f(*this, manip.value);
            return *this;
        }

        ColorPrinter & operator << (color c) {
            s_ << c;
            return *this;
        }

        ColorPrinter & operator << (char const * str) {
            s_ << str;
            return *this;
        }

        ColorPrinter & operator << (std::string const & str) {
            s_ << str;
            return *this;
        }

        ColorPrinter & operator << (int64_t value) {
            s_ << numberLiteral << value;
            return *this;
        }

        ColorPrinter & operator << (int value) {
            s_ << numberLiteral << value;
            return *this;
        }

        ColorPrinter & operator << (size_t value) {
            s_ << numberLiteral << value;
            return *this;
        }

        ColorPrinter & operator << (double value) {
            s_ << numberLiteral << value;
            return *this;
        }

        ColorPrinter & operator << (char value) {
            s_ << charLiteral << value;
            return *this;
        }

        ColorPrinter & operator << (tiny::Symbol value) {
            s_ << identifier << value.name();
            return *this;
        }

        /**
         * @brief Convenience operator to allow better chaining.
         * 
         * @param p 
         * @return ColorPrinter& - Always returns this (not p).
         */
        ColorPrinter & operator << ([[maybe_unused]] ColorPrinter &p) {
            return *this;
        }

    private:
        size_t lineNumber_ = 0;
        size_t indent_ = 0;
        std::ostream & s_;

        
    }; 

    inline void INDENT(ColorPrinter & p) {
        p.setIndent(p.indent() + p.tabSize);
    }

    inline void DEDENT(ColorPrinter & p) {
        p.setIndent(p.indent() - p.tabSize);
    }

    inline void NEWLINE(ColorPrinter & p) {
        p.newLine();
    }

    inline ColorPrinter::ManipulatorStr IDENT(std::string const & str) {
        return ColorPrinter::ManipulatorStr{
            [](ColorPrinter & p, std::string const & s) { p << p.identifier << s; },
            str
        };
    }

    inline ColorPrinter::ManipulatorStr SYMBOL(std::string const & str) {
        return ColorPrinter::ManipulatorStr{
            [](ColorPrinter & p, std::string const & s) { p << p.symbol << s; },
            str
        };
    }

    inline ColorPrinter::ManipulatorStr KEYWORD(std::string const & str) {
        return ColorPrinter::ManipulatorStr{
            [](ColorPrinter & p, std::string const & s) { p << p.keyword << s; },
            str
        };
    }

    inline ColorPrinter::ManipulatorStr LITERAL(std::string const & str) {
        return ColorPrinter::ManipulatorStr{
            [](ColorPrinter & p, std::string const & s) { p << p.stringLiteral << s; },
            str
        };
    }

    inline ColorPrinter::ManipulatorStr TYPE(char const * str) {
        return ColorPrinter::ManipulatorStr{
            [](ColorPrinter & p, std::string const & s) { p << p.type << s; },
            str
        };
    }

    inline ColorPrinter::ManipulatorStr TYPE(std::string const & str) { return TYPE(str.c_str()); }

} // namespace colors