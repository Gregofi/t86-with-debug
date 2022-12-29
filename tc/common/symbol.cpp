
#include "symbol.h"

namespace tiny {

    Symbol const Symbol::Inc{"++"};
    Symbol const Symbol::Dec{"--"};
    Symbol const Symbol::Add{"+"};
    Symbol const Symbol::Sub{"-"};
    Symbol const Symbol::Mul{"*"};
    Symbol const Symbol::Div{"/"};
    Symbol const Symbol::Mod{"%"};
    Symbol const Symbol::ShiftLeft{"<<"};
    Symbol const Symbol::ShiftRight{">>"};
    Symbol const Symbol::Eq{"=="};
    Symbol const Symbol::NEq{"!="};
    Symbol const Symbol::Lt{"<"};
    Symbol const Symbol::Gt{">"};
    Symbol const Symbol::Lte{"<="};
    Symbol const Symbol::Gte{">="};
    Symbol const Symbol::BitAnd{"&"};
    Symbol const Symbol::BitOr{"|"};
    Symbol const Symbol::And{"&&"};
    Symbol const Symbol::Or{"||"};
    Symbol const Symbol::Not{"!"};
    Symbol const Symbol::Neg{"~"};
    Symbol const Symbol::Xor{"^"};
    Symbol const Symbol::Dot{"."};
    Symbol const Symbol::Semicolon{";"};
    Symbol const Symbol::Colon{":"};
    Symbol const Symbol::ArrowR{"->"};
    Symbol const Symbol::Comma{","};
    Symbol const Symbol::ParOpen{"("};
    Symbol const Symbol::ParClose{")"};
    Symbol const Symbol::SquareOpen{"["};
    Symbol const Symbol::SquareClose{"]"};
    Symbol const Symbol::CurlyOpen{"{"};
    Symbol const Symbol::CurlyClose{"}"};
    Symbol const Symbol::Assign{"="};
    Symbol const Symbol::Backtick{"`"};

    Symbol const Symbol::KwBreak{"break"};
    Symbol const Symbol::KwCase{"case"};
    Symbol const Symbol::KwCast{"cast"};
    Symbol const Symbol::KwChar{"char"};
    Symbol const Symbol::KwContinue{"continue"};
    Symbol const Symbol::KwDefault{"default"};
    Symbol const Symbol::KwDefine{"define"};
    Symbol const Symbol::KwDefmacro{"defmacro"};
    Symbol const Symbol::KwDo{"do"};
    Symbol const Symbol::KwDouble{"double"};
    Symbol const Symbol::KwElse{"else"};
    Symbol const Symbol::KwFor{"for"};
    Symbol const Symbol::KwIf{"if"};
    Symbol const Symbol::KwInt{"int"};
    Symbol const Symbol::KwReturn{"return"};
    Symbol const Symbol::KwStruct{"struct"};
    Symbol const Symbol::KwSwitch{"switch"};
    Symbol const Symbol::KwTypedef{"typedef"};
    Symbol const Symbol::KwVoid{"void"};
    Symbol const Symbol::KwWhile{"while"};

} // namespace tiny