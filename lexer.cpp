#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
using namespace std;

string convert(string filename)	//преобразование файла в одну строку
{
    string text;
    fstream input;
    input.open(filename, ios::in);
    while (!input.eof()) {
        string temp;
        getline(input, temp);
        if (!input.eof())
            temp += '\n';
        text += temp;
    }
    input.close();
    return text;
}

bool isskip(char ch)
{
    if (isspace(ch)) return 1;
    if (ch == ';') return 1;
    if (ch == '(') return 1;
    if (ch == ')') return 1;
    if (ch == '{') return 1;
    if (ch == '}') return 1;
    return 0;
}

enum TokenType {
    ID,         // Идентификатор
    INT_CONST,  // Целочисленная константа
    FLOAT_CONST,// Вещественная константа
    OPERATOR,   // Оператор
    DELIMITER,  // Разделитель
    KEYWORD,    // Ключевое слово
    TOKEN_EOF         // Конец файла
};

struct Token {
    TokenType type;
    string value;

    Token(TokenType t, const string& v) : type(t), value(v) {}
};

enum State {
    START,       // Начальное состояние
    IDENT,       // Идентификатор
    INT,         // Целое число
    DOT,         // Точка в числе
    FLOAT,       // Вещественное число
    LES,         // Меньше ("<")
    GRT,         // Больше (">")
    EQU,         // Равно ("=")
    Z,           // Лексема распознана
    Z_STAR,      // Лексема распознана с откатом
    ERR          // Ошибка
};

class Lexer {
private:
    std::string input;  // Входной текст
    size_t pos;         // Текущая позиция в тексте
    char currentChar;   // Текущий символ

    // Вспомогательные функции
    void advance();     // Переход к следующему символу
    State nextState(State, char ); // Определение следующего состояния
    Token makeToken(State, const string&); // Создание токена

public:
    Lexer(const string& text); // Конструктор
    Token getNextToken();           // Получение следующего токена
};

Lexer::Lexer(const string& text) : input(text), pos(0) {
    if (!input.empty())
        currentChar = input[pos];
    else
        currentChar = '\0'; // Конец строки
}

void Lexer::advance() {
    pos++;
    if (pos < input.length())
        currentChar = input[pos];
    else
        currentChar = '\0'; // Конец строки
}

State Lexer::nextState(State currentState, char ch) {
    switch (currentState) {
    case START:
        if (isalpha(ch)) return IDENT;
        if (isdigit(ch)) return INT;
        if (isskip(ch)) return START;
        if (ch == '=') return EQU;
        if (ch == '<') return LES;
        if (ch == '>') return GRT;
        if (ch == ';') return Z;
        return ERR;

    case IDENT:
        if (isalnum(ch)) return IDENT;
        if (isskip(ch)) return Z;
        return Z_STAR;

    case INT:
        if (isdigit(ch)) return INT;
        if (ch == '.') return DOT;
        if (isalpha(ch)) return ERR;
        if (isskip(ch)) return Z;
        return Z_STAR;

    case DOT:
        if (isdigit(ch)) return FLOAT;
        return ERR;

    case FLOAT:
        if (isdigit(ch)) return FLOAT;
        if (isalpha(ch)) return ERR;
        if (ch == '.') return ERR;
        return Z_STAR;

    case LES:
        if (ch == '=') return Z;
        if (ch == '>') return Z;
        if (isskip(ch)) return Z;
        if (isalnum(ch)) return Z_STAR;
        return ERR;

    case GRT:
        if (ch == '=') return Z;
        if (isskip(ch)) return Z;
        if (isalnum(ch)) return Z_STAR;
        return ERR;

    case EQU:
        if (isalnum(ch)) return Z_STAR;
        if (isskip(ch)) return Z;
        if (ch == '=') return Z;
        return ERR;

    default:
        return ERR;
    }
}

Token Lexer::makeToken(State state, const string& lexeme) {
    unordered_map<string, TokenType>::const_iterator it; // Выносим объявление
    static unordered_map<string, TokenType> keywords = {
            {"if", KEYWORD}, {"else", KEYWORD},
            {"while", KEYWORD}, {"print", KEYWORD},
            {"read", KEYWORD}
    };
    switch (state) {
    case IDENT:
        // Проверка на ключевые слова
        
        it = keywords.find(lexeme);
        if (it != keywords.end())
            return Token(it->second, lexeme);
        return Token(ID, lexeme);

    case INT:
        return Token(INT_CONST, lexeme);

    case FLOAT:
        return Token(FLOAT_CONST, lexeme);

    case LES:
        return Token(OPERATOR, "<");

    case GRT:
        return Token(OPERATOR, ">");

    case EQU:
        return Token(OPERATOR, "=");
    case Z:
        // Обработка скобок
        if (lexeme == "(" || lexeme == ")" || lexeme == "{" || lexeme == "}" || lexeme == ";")
            return Token(DELIMITER, lexeme);
        throw std::runtime_error("Unexpected delimiter");
    default:
        throw runtime_error("Invalid token");
    }
}

Token Lexer::getNextToken() {
    State currentState = START;
    string lexeme;

    while (currentChar != EOF) {
        State nextState = this->nextState(currentState, currentChar);

        if (nextState == Z || nextState == Z_STAR) {
            // Если достигнуто состояние Z или Z*, создаем токен
            Token token = makeToken(currentState, lexeme);

             //Если состояние Z_STAR, откатываем позицию на 1 символ
            if (nextState == Z_STAR) {
                pos--; // Откат позиции
                currentChar = input[pos]; // Обновляем текущий символ
            }

            return token;
        }

        if (nextState == ERR) {
            if (currentState == START) {
                advance();
                continue;
            }
            break;
        }

        if(currentState != Z_STAR)
            lexeme += currentChar;
        currentState = nextState;
        advance();
    }

    return makeToken(currentState, lexeme);
}

int main() {
    
    string filename = "test.txt";
    string text = convert(filename);

    Lexer lexer(text);

    Token token = lexer.getNextToken();
    while (token.type != TOKEN_EOF) {
        std::cout << "Token Type: " << token.type << ", Value: " << token.value << "\n";
        token = lexer.getNextToken();
    }

    return 0;
}
