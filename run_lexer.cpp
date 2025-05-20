
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
using namespace std;

enum TokenType
{
    ID,          // Идентификатор
    INT_CONST,   // Целочисленная константа
    FLOAT_CONST, // Вещественная константа
    OPERATOR,    // Оператор
    DELIMITER,   // Разделитель
    KEYWORD,     // Ключевое слово
    TOKEN_EOF
};

struct Token
{
    TokenType type;
    string str_;
    int int_;
    float flo_;
    Token() : type(ID), str_(""), int_(0), flo_(0) {}
    Token(TokenType t, const string &v) : type(t), str_(v), int_(0), flo_(0) {}
    Token(TokenType t, const int &v) : type(t), str_(""), int_(v), flo_(0) {}
    Token(TokenType t, const float &v) : type(t), str_(""), int_(0), flo_(v) {}
};

enum State
{
    START,  // Начальное состояние
    IDENT,  // Идентификатор
    INT,    // Целое число
    DOT,    // Точка в числе
    FLOAT,  // Вещественное число
    LES,    // Меньше ("<")
    GRT,    // Больше (">")
    EQU,    // Равно ("=")
    Z,      // Лексема распознана
    Z_STAR, // Лексема распознана с откатом
    ERR     // Ошибка
};

class Lexer
{
private:
    // Входной текст
    string input;
    size_t pos;          // Текущая позиция в тексте
    State current_state; // Текущее состояние, выделил потому чтобы кучу раз во все функции не передавать аргументом.
    char currentChar;    // Текущий символ
    size_t row;
    size_t column;
    string name;
    int num;
    float flo;
    string op;
    float d = 1;

    // Вспомогательные функции
    void advance();        // Переход к следующему символу
    State nextState(char); // Определение следующего состояния
    Token makeToken();     // Создание токена
    void Programs(int);

public:
    Lexer(const string &text); // Конструктор
    Lexer();
    Token getNextToken(); // Получение следующего токена
    size_t get_pos();
    size_t get_row();
    size_t get_column();
    string get_input();
};
string Lexer::get_input()
{
    return input;
}
size_t Lexer::get_pos()
{
    return pos;
}

size_t Lexer::get_row()
{
    return row;
}

size_t Lexer::get_column()
{
    return column;
}

bool isdelim(char ch)
{
    return (ch == '(' || ch == ')' || ch == '{' || ch == '}' || ch == ';');
}

bool isOperator(char ch)
{
    return (ch == '+' || ch == '-' || ch == '*' || ch == '/');
}

Lexer::Lexer(const string &text) : input(text), pos(0), row(0), column(0)
{
    current_state = START;
    if (!input.empty())
        currentChar = input[pos];
    else
        currentChar = '\0'; // Конец строки
}
Lexer::Lexer() : input(""), pos(0), row(0), column(0) {};
void Lexer::Programs(int c)
{
    switch (c)
    {
    case 1:
        name = currentChar;
        break;
    case 2:
        num = currentChar - 48;
        break;
    case 3:
        name += currentChar;
        break;
    case 4:
        num = num * 10 + (currentChar - 48);
        break;
    case 5:
        flo = num;
        break;
    case 6:
        d = d * 0.1;
        flo = flo + (currentChar - 48) * d;
        break;
    case 7:
        op = currentChar;
        break;
    case 8:
        op += currentChar;
        advance();
        break;
    }
}

void Lexer::advance()
{
    pos++;
    column++;
    if (pos < input.size()) // Проверка
        currentChar = input[pos];
    else
        currentChar = '\0'; // Конец строки
    if (currentChar == '\n')
    {
        row++;
        column = 0;
    }
}

State Lexer::nextState(char ch)
{
    switch (current_state)
    {
    case START:
        if (isalpha(ch))
        {
            Programs(1);
            return IDENT;
        }
        if (isdigit(ch))
        {
            Programs(2);
            return INT;
        }
        if (ch == '=')
        {
            Programs(7);
            return EQU;
        }
        if (ch == '<')
        {
            Programs(7);
            return LES;
        }
        if (ch == '>')
        {
            Programs(7);
            return GRT;
        }
        if (isdelim(ch) || isOperator(ch))
        {
            Programs(7);
            return Z;
        }
        if (isspace(ch))
            return START;
        return ERR;

    case IDENT:
        if (isalnum(ch))
        {
            Programs(3);
            return IDENT;
        }
        if (isdelim(ch) || isOperator(ch))
            return Z;
        return Z;

    case INT:
        if (isdigit(ch))
        {
            Programs(4);
            return INT;
        }
        if (ch == '.')
        {
            Programs(5);
            return DOT;
        }
        if (isdelim(ch) || isOperator(ch))
            return Z;
        if (isspace(ch))
            return Z;
        if (isalpha(ch))
            return ERR;
        return Z;

    case DOT:
        if (isdigit(ch))
        {
            Programs(6);
            return FLOAT;
        }
        return ERR;

    case FLOAT:
        if (isdigit(ch))
        {
            Programs(6);
            return FLOAT;
        }
        if (isdelim(ch) || isOperator(ch))
            return Z;
        if (isalpha(ch) || ch == '.')
            return ERR;
        if (isspace(ch))
            return Z;
        return Z;

    case LES:
        if (ch == '=' || ch == '>')
        {
            Programs(8);
            return Z;
        }
        if (isdelim(ch) || isOperator(ch))
            return Z;
        if (isalnum(ch))
            return Z;
        if (isspace(ch))
            return Z;
        return ERR;

    case GRT:
        if (ch == '=')
        {
            Programs(8);
            return Z;
        }
        if (isdelim(ch) || isOperator(ch))
            return Z;
        if (isalnum(ch))
            return Z;
        if (isspace(ch))
            return Z;
        return ERR;

    case EQU:
        if (ch == '=')
        {
            Programs(8);
            return Z;
        }
        if (isdelim(ch) || isOperator(ch))
            return Z;
        if (isalnum(ch))
            return Z;
        if (isspace(ch))
            return Z;
        return ERR;

    default:
        return ERR;
    }
}

Token Lexer::makeToken()
{
    unordered_map<string, TokenType>::const_iterator it; // Выносим объявление
    static unordered_map<string, TokenType> keywords = {
        {"if", KEYWORD}, {"else", KEYWORD}, {"while", KEYWORD}, {"print", KEYWORD}, {"read", KEYWORD}};
    switch (current_state)
    {
    case IDENT:
        // Проверка на ключевые слова
        it = keywords.find(name);
        if (it != keywords.end())
            return Token(KEYWORD, name);
        return Token(ID, name);

    case INT:
        return Token(INT_CONST, num);
    case FLOAT:
        return Token(FLOAT_CONST, flo);
    case LES:
        return Token(OPERATOR, op);
    case GRT:
        return Token(OPERATOR, op);
    case EQU:
        return Token(OPERATOR, op);
    case Z:
        if (op == "(" || op == ")" || op == "{" || op == "}" || op == ";")
            return Token(DELIMITER, op);
        if (op == "/" || op == "*" || op == "-" || op == "+")
            return Token(OPERATOR, op);
        return Token(OPERATOR, op);
    default:
        cout << "Error in row and column " << row + 1 << " " << column;
        exit(-1);
    }
}

Token Lexer::getNextToken()
{
    State nextState = current_state;
    while (currentChar != EOF && currentChar != '\0' && currentChar)
    {
        nextState = this->nextState(currentChar);
        if (nextState == Z || nextState == ERR)
        {
            if (current_state == START)
            {
                current_state = nextState;
                advance();
            }
            Token token = makeToken();
            d = 1;
            current_state = START;
            return token;
        }

        current_state = nextState;
        advance();
    }
    current_state = nextState;
    Token t = Token();
    t.type = TOKEN_EOF;
    return t;
}

string convert(string filename) // преобразование файла в одну строку
{
    string text;
    fstream input;
    string temp;
    input.open(filename, ios::in);
    if (!input.is_open())
        return "NULL";

    while (getline(input, temp))
    { // Если getline(input, temp) с ошибкой считался значит закончили (вернул False), input.eof не нужон
        text += temp;
        text += "\n";
    }

    text += '\0';
    input.close();
    return text;
}

int main()
{
    string filename = "C:\\C++proj\\Translator-interpreter_for_programming_language\\test.txt";
    string text = convert(filename);
    if (text == "NULL")
    {
        // Обрабатываем кейс когда файл не найден.
        cout << "The file for reading was not found in the directory.";
        return 1;
    }
    Lexer lexer(text);

    size_t sz = text.size();
    while (lexer.get_pos() != (sz - 2))
    {
        Token token = lexer.getNextToken();
        cout << "Token Type: " << token.type << ", Value: ";
        if ((token.str_).size())
            cout << token.str_;
        if (token.int_)
            cout << token.int_;
        if (token.flo_)
            cout << token.flo_;
        cout << endl;
    }

    if (lexer.get_pos() == (sz - 2))
    {
        cout << "End of lexical analysis";
        return 0;
    }
    cout << "Error during lexical analysis";
    return 1;
}