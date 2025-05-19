#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <cctype>    // For isalpha, isdigit, isalnum, isspace
#include <stdexcept> // For std::runtime_error, std::invalid_argument, std::out_of_range
#include <variant>
#include <iomanip> // For std::fixed, std::setprecision
#include "run_lexer.cpp"
// --- ОПРЕДЕЛЕНИЕ ФОРМАТА ОПС (Задача 6) ---
// Перечисление для кодов операций ОПС
enum class OPSCode
{
    // Операнды (специальные маркеры, чтобы знать, что находится в value)
    OP_INT_CONST,   // value is int_
    OP_FLOAT_CONST, // value is flo_
    OP_IDENT,       // value is str_

    // Арифметические
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,

    // Сравнения
    OP_LS,
    OP_LE,
    OP_GS,
    OP_GE,
    OP_EQ,
    OP_NE,

    // Управление потоком
    OP_JF,  // Условный переход (Jump if Zero)
    OP_JMP, // Безусловный переход

    // Память
    OP_ASSIGN, // Присваивание

    // Ввод/Вывод
    OP_READ,
    OP_PRINT,

    OP_ERROR,

    // Метки (для переходов)
    OP_LABEL // value is int_ (индекс в векторе OPS)
};

// Структура для одного элемента в последовательности ОПС
struct OPSElement
{
    OPSCode code; // Код операции или тип операнда

    // Значение элемента. Используем variant для гибкости.
    std::variant<int, float, std::string, size_t> value; // int/float для констант, string для имен переменных, size_t для адресов меток (индексов в векторе)

    OPSElement(OPSCode c, int v) : code(c), value(v) {}
    OPSElement(OPSCode c, float v) : code(c), value(v) {}
    OPSElement(OPSCode c, const std::string &v) : code(c), value(v) {}
    OPSElement(OPSCode c, size_t v) : code(c), value(v) {}
    OPSElement(OPSCode c) : code(c) {} // Для операций без явного значения (JMP, JF, +, =, etc.)
};

// Хранилище для сгенерированной последовательности ОПС (Задача 6)
std::vector<OPSElement> ops_code;

// --- ГЕНЕРАЦИЯ ОПС (Задача 4) ---

// Вспомогательная функция для добавления элемента в ОПС
// Перегружена для разных типов значений
void AddToOPS(const OPSElement &element)
{
    ops_code.push_back(element);
}

// Вспомогательная функция для генерации уникальных меток
size_t label_counter = 0;
std::string NewLabel()
{
    return "L" + std::to_string(label_counter++);
}

// --- СИНТАКСИЧЕСКИЙ АНАЛИЗАТОР (ПАРСЕР) ---
// (Рекурсивный спуск с генерацией ОПС)

class Parser
{
private:
    Lexer &lexer;       // Ссылка на лексер
    bool hasError;      // Флаг ошибки парсинга
    Token currentToken; // Текущий токен от лексера

    // Вспомогательные функции
    void expect(TokenType expectedType, const std::string &errorMessage);
    void expect(const std::string &expectedValue, const std::string &errorMessage);
    void consume();
    void error(const std::string &message);

    // Функции для каждого нетерминала грамматики
    void Start();
    void StatementList();
    void Statement();
    void Assignment();
    void Expression();
    void U(); // Для + и - хвоста выражения
    void Term();
    void V(); // Для * и / хвоста выражения
    void Factor();
    void Condition();
    // COMPOP не нужна как отдельная функция
    void Ifelse();
    void Loop();
    void InOutput(); // Объединяет Input и Output
    void Input();
    void Output();
    // EmptyStatement не нужна как отдельная функция

    // Вспомогательная функция для получения OPSCode из строки оператора
    OPSCode getOPSCode(const std::string &op_symbol);

public:
    Parser(Lexer &lexer); // Конструктор
    void parse();
    bool hasSyntaxError() const { return hasError; }

    // Для отладки: печать сгенерированной ОПС
    void printOPS() const;
};

// Конструктор парсера

Parser::Parser(Lexer &lexer) : lexer(lexer), hasError(false), currentToken(lexer.getNextToken()) {}

void Parser::parse()
{
    // Проверяем, нет ли ошибок от лексера на первом токене
    if (hasError)
    {
        std::cerr << "Parsing aborted due to initial lexical error." << std::endl;
        return; // Прерываем парсинг, если лексер уже выдал ошибку
    }
    // Начинаем разбор с начального символа грамматики (START)
    Start();

    if (!hasError)
    {
        std::cout << "Parsing successful: Syntax is correct." << std::endl;
        // !!! ЗДЕСЬ должен быть вызов функции для интерпретации сгенерированного OPS !!!
        // interpret_ops(ops_code); // Пример вызова
    }
    else
    {
        std::cerr << "Parsing failed: Syntax errors found." << std::endl;
    }
}

// Получает следующий токен
void Parser::consume()
{
    if (hasError)
        return;
    currentToken = lexer.getNextToken();
}

// Проверяет тип текущего токена и потребляет его
void Parser::expect(TokenType expectedType, const std::string &errorMessage)
{
    if (hasError)
        return;
    if (currentToken.type == expectedType)
    {
        consume();
        return;
    }
    error(errorMessage);
    hasError = true;
}

// Проверяет значение (для операторов/разделителей/ключевых слов) текущего токена и потребляет его
void Parser::expect(const std::string &expectedValue, const std::string &errorMessage)
{
    if (hasError)
        return;

    if ((currentToken.type == TokenType::OPERATOR ||
         currentToken.type == TokenType::DELIMITER ||
         currentToken.type == TokenType::KEYWORD) &&
        currentToken.str_ == expectedValue)
    {
        consume();
        return;
    }
    error(errorMessage);
    hasError = true;
}

// Выводит сообщение об ошибке парсинга
void Parser::error(const std::string &message)
{
    if (!hasError)
    {
        std::cerr << "Syntax Error at Row " << lexer.get_row() + 1 << ", Column " << lexer.get_column() + 1 << ": " << message << std::endl;
        hasError = true;
        // В реальном парсере здесь может быть логика восстановления после ошибки
    }
}

// Вспомогательная функция для получения OPSCode из строки оператора
OPSCode Parser::getOPSCode(const std::string &op_symbol)
{
    if (op_symbol == "+")
        return OPSCode::OP_ADD;
    if (op_symbol == "-")
        return OPSCode::OP_SUB;
    if (op_symbol == "*")
        return OPSCode::OP_MUL;
    if (op_symbol == "/")
        return OPSCode::OP_DIV;
    if (op_symbol == "<")
        return OPSCode::OP_LS;
    if (op_symbol == "<=")
        return OPSCode::OP_LE;
    if (op_symbol == ">")
        return OPSCode::OP_GS;
    if (op_symbol == ">=")
        return OPSCode::OP_GE;
    if (op_symbol == "==")
        return OPSCode::OP_EQ;
    if (op_symbol == "<>")
        return OPSCode::OP_NE;
    if (op_symbol == "=")
        return OPSCode::OP_ASSIGN;
    // ... добавьте другие операторы, если есть (AND, OR, NOT, etc.)
    // Если оператор не найден, это внутренняя ошибка или ошибка лексера
    std::cerr << "Internal Error: Unknown operator symbol '" << op_symbol << "' in getOPSCode." << std::endl;
    return OPSCode::OP_ERROR; // Нужен специальный код ошибки, или бросить исключение.
                              // Добавляем фиктивный OP_ERROR в OPSCode enum.
}

// --- Реализация функций для нетерминалов (по вашей грамматике) ---

// START -> STATEMENT_LIST EOF
void Parser::Start()
{
    if (hasError)
        return;
    StatementList();
}

// STATEMENT_LIST -> STATEMENT STATEMENT_LIST | ε
void Parser::StatementList()
{
    if (hasError)
        return;
    // FIRST set of STATEMENT: { ID, KEYWORD(if,while,for,read,print), DELIMITER(;) }
    if (currentToken.type == TokenType::ID ||
        (currentToken.type == TokenType::KEYWORD &&
         (currentToken.str_ == "if" || currentToken.str_ == "while" || currentToken.str_ == "for" || currentToken.str_ == "read" || currentToken.str_ == "print")) ||
        (currentToken.type == TokenType::DELIMITER && currentToken.str_ == ";"))
    {
        // Это не ε случай, ожидаем STATEMENT
        Statement();     // Разбираем один оператор (включая SC)
        StatementList(); // Рекурсивно разбираем остаток списка
    }
    // Иначе (ε случай) - ничего не делаем, просто выходим.
    // Follow set of STATEMENT_LIST: { EOF, R_BODY, ELSE_STATEMENT } - эти токены не могут начать STATEMENT
}

// STATEMENT -> ASSIGNMENT SC | IFELSE SC | LOOP SC | INPUT SC | OUTPUT SC | SC
void Parser::Statement()
{
    if (hasError)
        return;
    // Выбираем альтернативу на основе текущего токена
    if (currentToken.type == TokenType::ID)
    {
        Assignment();
        expect(";", "Expected ';' after assignment statement.");
    }
    else if (currentToken.type == TokenType::KEYWORD)
    {
        if (currentToken.str_ == "if")
        {
            Ifelse();
            expect(";", "Expected ';' after if/ifelse statement."); // Грамматика требует SC
        }
        else if (currentToken.str_ == "while")
        {
            Loop();
            expect(";", "Expected ';' after loop statement."); // Грамматика требует SC
        }
        else if (currentToken.str_ == "read" || currentToken.str_ == "print")
        {
            InOutput();
            expect(";", "Expected ';' after input/output statement."); // Грамматика требует SC
        }
        else
        {
            error("Syntax Error: Unexpected keyword '" + currentToken.str_ + "'.");
            consume();
        }
    }
    else if (currentToken.type == TokenType::DELIMITER && currentToken.str_ == ";")
    {
        // Это пустой оператор (просто ';')
        expect(";", "Internal Parser Error: Expected ';'"); // Потребляем SC
        // Семантически ничего не делаем для пустого оператора - OPS не генерируется
    }
    else
    {
        error("Syntax Error: Expected start of a statement (ID, keyword, or ';'), but got token type " + std::to_string(static_cast<int>(currentToken.type)));
        consume(); // Пропускаем ошибочный токен
    }
}

// ASSIGNMENT -> ID ASSIGN EXPRESSION
void Parser::Assignment()
{
    if (hasError)
        return;

    std::string var_name = currentToken.str_;
    expect(TokenType::ID, "Expected identifier in assignment.");
    if (hasError)
        return;

    expect("=", "Expected '=' in assignment.");
    if (hasError)
        return;

    Expression(); // Generates OPS for the expression

    // Semantic actions (after expression OPS is generated)
    AddToOPS(OPSElement(OPSCode::OP_IDENT, var_name)); // Variable (where to assign)
    AddToOPS(OPSElement(OPSCode::OP_ASSIGN));          // Assignment operator
}

// EXPRESSION -> TERM U
void Parser::Expression()
{
    if (hasError)
        return;
    Term(); // Generates OPS for the first Term
    U();    // Generates OPS for the rest (+/-)
}

// U -> ADD TERM U | SUB TERM U | ε
void Parser::U()
{
    if (hasError)
        return;
    // Check for '+' or '-' operator
    if (currentToken.type == TokenType::OPERATOR && (currentToken.str_ == "+" || currentToken.str_ == "-"))
    {
        std::string op_val = currentToken.str_;                        // Capture operator symbol
        expect(op_val, "Internal Parser Error: Expected '+' or '-'."); // Consume the operator
        if (hasError)
            return;

        Term(); // Generates OPS for the next Term

        U(); // Recurse for the rest of the tail

        // Semantic action (after operands and tail are parsed)
        AddToOPS(OPSElement(getOPSCode(op_val))); // Add the operator to OPS
    }
    // Epsilon case (nothing to do) is implicit when the token is not '+' or '-'
    // Follow set of U: {;, ), }, <, <=, >, >=, ==, <>, (AND, OR, NOT?), EOF}
}

// TERM -> FACTOR V
void Parser::Term()
{
    if (hasError)
        return;
    Factor(); // Generates OPS for the Factor
    V();      // Generates OPS for the rest (*, /)
}

// V -> MUL FACTOR V | DIV FACTOR V | ε
void Parser::V()
{
    if (hasError)
        return;
    // Check for '*' or '/' operator
    if (currentToken.type == TokenType::OPERATOR && (currentToken.str_ == "*" || currentToken.str_ == "/"))
    {
        std::string op_val = currentToken.str_;                        // Capture operator symbol
        expect(op_val, "Internal Parser Error: Expected '*' or '/'."); // Consume the operator
        if (hasError)
            return;

        Factor(); // Generates OPS for the next Factor

        V(); // Recurse for the rest of the tail

        // Semantic action (after operands and tail are parsed)
        AddToOPS(OPSElement(getOPSCode(op_val))); // Add the operator to OPS
    }
    // Epsilon case is implicit
    // Follow set of V: {+, -, ;, ), }, <, <=, >, >=, ==, <>, (AND, OR, NOT?), EOF}
}

// FACTOR -> ID | INT_CONST | FLOAT_CONST | L_BRACKET EXPRESSION R_BRACKET
void Parser::Factor()
{
    if (hasError)
        return;
    // Choose alternative based on current token
    if (currentToken.type == TokenType::ID)
    {
        // Semantic action: Add the variable to OPS (to push its value onto the stack)
        AddToOPS(OPSElement(OPSCode::OP_IDENT, currentToken.str_));
        expect(TokenType::ID, "Expected identifier in factor.");
        if (hasError)
            return;
    }
    else if (currentToken.type == TokenType::INT_CONST)
    {
        // Semantic action: Add the integer constant to OPS
        AddToOPS(OPSElement(OPSCode::OP_INT_CONST, currentToken.int_));
        expect(TokenType::INT_CONST, "Expected integer constant in factor.");
        if (hasError)
            return;
    }
    else if (currentToken.type == TokenType::FLOAT_CONST)
    {
        // Semantic action: Add the float constant to OPS
        AddToOPS(OPSElement(OPSCode::OP_FLOAT_CONST, currentToken.flo_));
        expect(TokenType::FLOAT_CONST, "Expected float constant in factor.");
        if (hasError)
            return;
    }
    else if (currentToken.type == TokenType::DELIMITER && currentToken.str_ == "(")
    {
        expect("(", "Expected '(' in factor.");
        if (hasError)
            return;
        Expression(); // Generates OPS for the expression inside parentheses
        expect(")", "Expected ')' after expression in factor.");
        if (hasError)
            return;
        // Semantic action: Parentheses only control parsing order, no OPS needed for them directly
    }
    else
    {
        error("Syntax Error: Expected identifier, number, or '(' in factor.");
        consume(); // Skip the erroneous token
    }
}

// COMPOP -> LS | LE | GS | GE | EQ | NE
// This non-terminal is handled directly within Condition function.

// CONDITION -> EXPRESSION COMPOP EXPRESSION
void Parser::Condition()
{
    if (hasError)
        return;
    Expression(); // Generates OPS for the left side of the comparison

    // Check and consume the comparison operator
    if (currentToken.type == TokenType::OPERATOR)
    {
        std::string op_val = currentToken.str_; // Capture operator symbol
        if (op_val == "<" || op_val == "<=" || op_val == ">" || op_val == ">=" || op_val == "==" || op_val == "<>")
        {
            expect(op_val, "Internal Parser Error: Expected comparison operator."); // Consume the operator
            if (hasError)
                return;
            Expression(); // Generates OPS for the right side of the comparison

            // Semantic action (after both expressions are parsed)
            AddToOPS(OPSElement(getOPSCode(op_val))); // Add the comparison operator to OPS
        }
        else
        {
            error("Syntax Error: Expected comparison operator (<, <=, >, >=, ==, <>), but got '" + op_val + "'.");
            consume();
        }
    }
    else
    {
        error("Syntax Error: Expected comparison operator after expression in condition.");
        consume();
    }
}

// IFELSE -> IF_STATEMENT L_BRACKET CONDITION R_BRACKET L_BODY STATEMENT_LIST R_BODY ELSE_STATEMENT L_BODY STATEMENT_LIST R_BODY
//        | IF_STATEMENT L_BRACKET CONDITION R_BRACKET L_BODY STATEMENT_LIST R_BODY
void Parser::Ifelse()
{
    if (hasError)
        return;
    expect("if", "Internal Parser Error: Expected 'if'."); // Keyword if
    if (hasError)
        return;
    expect("(", "Expected '(' after 'if'.");
    if (hasError)
        return;
    Condition(); // Generates OPS for the condition
    expect(")", "Expected ')' after condition.");
    if (hasError)
        return;

    // --- Semantic actions for the IF part ---
    // After condition, generate JF jump based on condition result
    std::string labelElse = NewLabel();                 // Label for the start of the else block (or end of if)
    AddToOPS(OPSElement(OPSCode::OP_LABEL, labelElse)); // Add label reference to OPS
    AddToOPS(OPSElement(OPSCode::OP_JF));               // Add JF command

    expect("{", "Expected '{' for if body.");
    if (hasError)
        return;
    StatementList(); // Generates OPS for the 'then' block
    expect("}", "Expected '}' after if body.");
    if (hasError)
        return;
    if (currentToken.type == TokenType::KEYWORD && currentToken.str_ == "else")
    {
        // --- Semantic actions for the ELSE part ---
        // Before the else block, generate a JMP to skip the else block if 'if' was true
        std::string labelEnd = NewLabel();                 // Label for the very end of if-else
        AddToOPS(OPSElement(OPSCode::OP_LABEL, labelEnd)); // Add label reference to OPS
        AddToOPS(OPSElement(OPSCode::OP_JMP));             // Add JMP command

        // Place the label for the start of the else block
        AddToOPS(OPSElement(OPSCode::OP_LABEL, ":")); // Add label definition to OPS

        expect("else", "Internal Parser Error: Expected 'else'."); // Keyword else
        if (hasError)
            return;
        expect("{", "Expected '{' for else body.");
        if (hasError)
            return;
        StatementList(); // Generates OPS for the 'else' block
        expect("}", "Expected '}' after else body.");
        if (hasError)
            return;

        // Place the label for the very end of if-else
        AddToOPS(OPSElement(OPSCode::OP_LABEL, labelEnd + ":")); // Add label definition to OPS
    }
    else
    {
        // --- Semantic actions for IF without ELSE ---
        // Place the label for the end of the if block (which was labelElse)
        AddToOPS(OPSElement(OPSCode::OP_LABEL, labelElse + ":")); // Add label definition to OPS
    }
    // Check for ELSE_STATEMENT
}
// LOOP -> WHILE_STATEMENT L_BRACKET CONDITION R_BRACKET L_BODY STATEMENT_LIST R_BODY
//      | FOR_STATEMENT L_BRACKET STATEMENT CONDITION SC STATEMENT R_BRACKET L_BODY STATEMENT_LIST R_BODY
void Parser::Loop()
{
    if (hasError)
        return;
    if (currentToken.type == TokenType::KEYWORD && currentToken.str_ == "while")
    {
        // --- Semantic actions for WHILE ---
        std::string labelStart = NewLabel(); // Label for the start of condition check
        std::string labelEnd = NewLabel();   // Label for the end of the loop

        // Place the label for the start of condition check
        AddToOPS(OPSElement(OPSCode::OP_LABEL, labelStart + ":")); // Add label definition to OPS

        expect("while", "Internal Parser Error: Expected 'while'."); // Keyword while
        if (hasError)
            return;
        expect("(", "Expected '(' after 'while'.");
        if (hasError)
            return;
        Condition(); // Generates OPS for the loop condition
        expect(")", "Expected ')' after condition.");
        if (hasError)
            return;

        // After condition, generate JF jump to exit the loop
        AddToOPS(OPSElement(OPSCode::OP_LABEL, labelEnd)); // Add label reference to OPS
        AddToOPS(OPSElement(OPSCode::OP_JF));              // Add JF command

        expect("{", "Expected '{' for while body.");
        if (hasError)
            return;
        StatementList(); // Generates OPS for the loop body
        expect("}", "Expected '}' after while body.");
        if (hasError)
            return;

        // After loop body, generate JMP to return to condition check
        AddToOPS(OPSElement(OPSCode::OP_LABEL, labelStart)); // Add label reference to OPS
        AddToOPS(OPSElement(OPSCode::OP_JMP));               // Add JMP command

        // Place the label for the end of the loop
        AddToOPS(OPSElement(OPSCode::OP_LABEL, labelEnd + ":")); // Add label definition to OPS
    }
    else
    {
        error("Syntax Error: Expected 'while' or 'for', but got '" + currentToken.str_ + "'.");
        consume();
    }
}

// INOUTPUT -> INPUT | OUTPUT
void Parser::InOutput()
{
    if (hasError)
        return;
    // Choose alternative based on current token
    if (currentToken.type == TokenType::KEYWORD && currentToken.str_ == "read")
    {
        Input(); // Parse Input (generates OPS inside)
    }
    else if (currentToken.type == TokenType::KEYWORD && currentToken.str_ == "print")
    {
        Output(); // Parse Output (generates OPS inside)
    }
    else
    {
        error("Internal Parser Error: Expected 'read' or 'print'.");
        consume();
    }
}

// INPUT -> READ L_BRACKET ID R_BRACKET
void Parser::Input()
{
    if (hasError)
        return;
    expect("read", "Internal Parser Error: Expected 'read'."); // Keyword read
    if (hasError)
        return;
    expect("(", "Expected '(' after 'read'.");
    if (hasError)
        return;

    // Semantic action: Add ident name and READ operator
    expect(TokenType::ID, "Expected identifier after 'read('.");
    if (hasError)
        return;
    AddToOPS(OPSElement(OPSCode::OP_IDENT, currentToken.str_));

    expect(")", "Expected ')' after identifier in 'read'.");
    if (hasError)
        return;
    AddToOPS(OPSElement(OPSCode::OP_READ));
}

// OUTPUT -> PRINT L_BRACKET EXPRESSION R_BRACKET
void Parser::Output()
{
    if (hasError)
        return;
    expect("print", "Internal Parser Error: Expected 'print'."); // Keyword print
    if (hasError)
        return;
    expect("(", "Expected '(' after 'print'.");
    if (hasError)
        return;

    Expression(); // Generates OPS for the expression to print

    expect(")", "Expected ')' after expression in 'print'.");
    if (hasError)
        return;
    // Semantic action: Add PRINT operator (after expression)
    AddToOPS(OPSElement(OPSCode::OP_PRINT));
}

// EMPTY_STATEMENT -> ε
// Handled implicitly in StatementList and Statement functions.
// When StatementList is called and the current token is in its Follow set,
// the function returns, effectively parsing ε.
// When Statement is called and the current token is ';', the Statement function
// explicitly handles the 'SC' case.

// --- Печать сгенерированной ОПС (для отладки) ---

void Parser::printOPS() const
{
    std::cout << "\n--- Generated OPS Code ---" << std::endl;
    if (ops_code.empty())
    {
        std::cout << "(Empty)" << std::endl;
        return;
    }

    // Mapping OPSCode to string for printing
    std::unordered_map<OPSCode, std::string> opsCodeToString = {
        {OPSCode::OP_INT_CONST, "INT"},
        {OPSCode::OP_FLOAT_CONST, "FLOAT"},
        {OPSCode::OP_IDENT, "ID"},
        {OPSCode::OP_ADD, "+"},
        {OPSCode::OP_SUB, "-"},
        {OPSCode::OP_MUL, "*"},
        {OPSCode::OP_DIV, "/"},
        {OPSCode::OP_LS, "<"},
        {OPSCode::OP_LE, "<="},
        {OPSCode::OP_GS, ">"},
        {OPSCode::OP_GE, ">="},
        {OPSCode::OP_EQ, "=="},
        {OPSCode::OP_NE, "<>"},
        {OPSCode::OP_JF, "JF"},
        {OPSCode::OP_JMP, "JMP"},
        {OPSCode::OP_ASSIGN, "="},
        {OPSCode::OP_READ, "READ"},
        {OPSCode::OP_PRINT, "PRINT"},
        {OPSCode::OP_LABEL, "LABEL"}
        // Add other ops if needed
    };

    for (size_t i = 0; i < ops_code.size(); ++i)
    {
        const auto &element = ops_code[i];
        // Print index for easier label reference
        // std::cout << std::setw(4) << i << ": "; // Optional: print index

        if (element.code == OPSCode::OP_LABEL)
        {
            // Label definition
            std::cout << std::get<std::string>(element.value) << std::endl; // Print label name (e.g., "L1:")
        }
        else
        {
            // Other elements
            auto it = opsCodeToString.find(element.code);
            std::string codeStr = (it != opsCodeToString.end()) ? it->second : "UNKNOWN";

            std::cout << codeStr;

            // Print value based on code type
            switch (element.code)
            {
            case OPSCode::OP_INT_CONST:
                std::cout << " " << std::get<int>(element.value);
                break;
            case OPSCode::OP_FLOAT_CONST:
                std::cout << " " << std::fixed << std::setprecision(2) << std::get<float>(element.value); // Adjust precision as needed
                break;
            case OPSCode::OP_IDENT:
                std::cout << " " << std::get<std::string>(element.value);
                break;
            case OPSCode::OP_JF:
            case OPSCode::OP_JMP:
                break;
            // For Operators (+, -, *, etc.), READ, PRINT, ASSIGN - operands are on the stack, print only the operator
            case OPSCode::OP_ADD:
            case OPSCode::OP_SUB:
            case OPSCode::OP_MUL:
            case OPSCode::OP_DIV:
            case OPSCode::OP_LS:
            case OPSCode::OP_LE:
            case OPSCode::OP_GS:
            case OPSCode::OP_GE:
            case OPSCode::OP_EQ:
            case OPSCode::OP_NE:
            case OPSCode::OP_ASSIGN:
            case OPSCode::OP_READ:
            case OPSCode::OP_PRINT:
                // No extra value to print here, operands/targets are handled by stack/previous elements
                break;
            case OPSCode::OP_LABEL:
                // Handled above, prints label name and newline
                break;
            default:
                std::cout << " UNKNOWN_VALUE"; // Fallback
                break;
            }
            // Add space after non-label elements for readability
            if (element.code != OPSCode::OP_LABEL)
            {
                std::cout << " ";
            }
        }
    }
    std::cout << std::endl; // Final newline
}

// --- Вспомогательная функция для чтения файла ---
std::string read_file(const std::string &filename)
{
    std::string text;
    std::ifstream input_file(filename);

    if (!input_file.is_open())
    {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return ""; // Возвращаем пустую строку в случае ошибки
    }

    // Читаем файл построчно и объединяем строки, добавляя обратно символы новой строки
    std::string line;
    while (std::getline(input_file, line))
    {
        text += line;
        text += '\n'; // Добавляем обратно символ новой строки
    }

    input_file.close();
    return text;
}

// --- Главная функция программы ---
int main()
{
    std::string filename = "C:\\C_Projects\\lexer\\test.txt"; // Укажите правильный путь к файлу

    std::string text = convert(filename);
    cout << text;
    // Создаем лексер с текстом из файла
    Lexer lexer(text);

    // Создаем парсер, передавая ему лексер
    Parser parser(lexer);

    // Запускаем процесс парсинга
    parser.parse();

    // Печатаем сгенерированную ОПС, если не было ошибок синтаксиса
    if (!parser.hasSyntaxError())
    {
        parser.printOPS();
        // Здесь должен быть вызов функции для интерпретации сгенерированного OPS (Задача 3)
        // interpret_ops(ops_code);
    }

    // Возвращаем статус в зависимости от результата парсинга
    if (parser.hasSyntaxError())
        return 1; // Возвращаем ненулевой код для ошибки синтаксиса или лексической ошибки
    return 0;
}