#include <string>
#include <iostream>
#include <fstream>
#include <unordered_map>
using namespace std;

enum TokenType
{
    ID,          // Идентификатор
    INT_CONST,   // Целочисленная константа
    FLOAT_CONST, // Вещественная константа
    OPERATOR,    // Оператор
    DELIMITER,   // Разделитель
    KEYWORD     // Ключевое слово
};

struct Token
{
    TokenType type;
    string value;

    Token(TokenType t, const string& v) : type(t), value(v) {}
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
    string input;        // Входной текст
    size_t pos;          // Текущая позиция в тексте
    State current_state; // Текущее состояние, выделил потому чтобы кучу раз во все функции не передавать аргументом.
    char currentChar;    // Текущий символ

    // Вспомогательные функции
    void advance();                  // Переход к следующему символу
    State nextState(char);           // Определение следующего состояния
    Token makeToken(const string&); // Создание токена

public:
    Lexer(const string& text); // Конструктор
    Token getNextToken();      // Получение следующего токена
    int get_pos();
};

int Lexer::get_pos()
{
    return pos;
}

bool isdelim(char ch)
{
    return (ch == '(' || ch == ')' || ch == '{' || ch == '}' || ch == ';');
}

bool isOperator(char ch)
{
    return(ch == '+' || ch == '-' || ch == '*' || ch == '/');
}

Lexer::Lexer(const string& text) : input(text), pos(0)
{
    current_state = START;
    if (!input.empty())
        currentChar = input[pos];
    else
        currentChar = '\0'; // Конец строки
}

void Lexer::advance()
{
    pos++;
    if (pos < input.size()) // Проверка
        currentChar = input[pos];
    else
        currentChar = '\0'; // Конец строки
}

State Lexer::nextState(char ch)
{
    switch (current_state) {
    case START:
        if (isalpha(ch)) return IDENT;
        if (isdigit(ch)) return INT;
        //if (isspace(ch)) return START;
        if (ch == '=') return EQU;
        if (ch == '<') return LES;
        if (ch == '>') return GRT;
        if (isdelim(ch) || isOperator(ch)) return Z;
        return ERR;

    case IDENT:
        if (isalnum(ch)) return IDENT;
        //if (isspace(ch)) return Z;
        if (isdelim(ch) || isOperator(ch)) return Z;
        return Z;

    case INT:
        if (isdigit(ch)) return INT;
        if (ch == '.') return DOT;
        if (isalpha(ch)) return ERR;
        //if (isspace(ch)) return Z;
        if (isdelim(ch) || isOperator(ch)) return Z;
        return Z;

    case DOT:
        if (isdigit(ch)) return FLOAT;
        return ERR;

    case FLOAT:
        if (isdigit(ch)) return FLOAT;
        if (isalpha(ch) || ch == '.') return ERR;
        //if (isspace(ch)) return Z;
        if (isdelim(ch) || isOperator(ch)) return Z;
        return Z;

    case LES:
        //if (isspace(ch)) return Z;
        if (ch == '=' || ch == '>' || isdelim(ch) || isOperator(ch)) return Z;
        if (isalnum(ch)) return Z;
        return ERR;

    case GRT:
        //if (isspace(ch)) return Z;
        if (ch == '=' || isdelim(ch) || isOperator(ch)) return Z;
        if (isalnum(ch)) return Z;
        return ERR;

    case EQU:
        //if (isspace(ch)) return Z;
        if (ch == '=' || isdelim(ch) || isOperator(ch)) return Z;
        if (isalnum(ch)) return Z;
        return ERR;

    default:
        return ERR;
    }
}

Token Lexer::makeToken(const string& lexeme)
{
    unordered_map<string, TokenType>::const_iterator it; // Выносим объявление
    static unordered_map<string, TokenType> keywords = {
        {"if", KEYWORD}, {"else", KEYWORD}, {"while", KEYWORD}, {"print", KEYWORD}, {"read", KEYWORD} };
    switch (current_state)
    {
    case IDENT:
        // Проверка на ключевые слова
        it = keywords.find(lexeme);
        if (it != keywords.end())
            return Token(KEYWORD, lexeme);
        return Token(ID, lexeme);

    case INT:
        return Token(INT_CONST, lexeme);
    case FLOAT:
        return Token(FLOAT_CONST, lexeme);
    case LES:
        return Token(OPERATOR, lexeme);
    case GRT:
        return Token(OPERATOR, lexeme);
    case EQU:
        return Token(OPERATOR, lexeme);
    case Z:
        if (lexeme == "(" || lexeme == ")" || lexeme == "{" || lexeme == "}" || lexeme == ";")
            return Token(DELIMITER, lexeme);
        if (lexeme == "/" || lexeme == "*" || lexeme == "-" || lexeme == "+")
            return Token(OPERATOR, lexeme);
        return Token(OPERATOR, lexeme);
    default:
        cout << "Error in " << lexeme << " " << current_state;
        exit(-1);
    }
}

Token Lexer::getNextToken()
{
    State nextState = current_state;
    string lexeme;
    while (currentChar != EOF && currentChar != '\0' || !currentChar)
    {
        while (isspace(currentChar))
            advance();

        nextState = this->nextState(currentChar);
        if (nextState == ERR)
            current_state = nextState;
        if (nextState == LES || nextState == GRT || nextState == EQU) {
            lexeme += currentChar;
            advance();
        }
        
        if (nextState == Z || nextState == Z_STAR || nextState == ERR) {
            if (nextState == Z && lexeme == "") {
                current_state = nextState;
                lexeme += currentChar;
                advance();
            }
            // Токен делаем и для ERR чтобы обработать было проще.
            // Если достигнуто состояние Z или Z*, создаем токен
            Token token = makeToken(lexeme);
            // Если состояние Z_STAR, откатываем позицию на 1 символ
            if (nextState == Z_STAR) {
                pos--;                    // Откат позиции
                currentChar = input[pos]; // Обновляем текущий символ
            }
            current_state = START;
            
            return token;
        }

        if (current_state != Z_STAR && !isspace(currentChar))
            lexeme += currentChar; 

        current_state = nextState;
        advance();
       
    }
    current_state = nextState;


    return makeToken("");
}

string convert(string filename) // преобразование файла в одну строку
{
    string text;
    fstream input;
    string temp;
    input.open(filename, ios::in);
    if (!input.is_open())
        return "NULL";

    while (getline(input, temp)) // Если getline(input, temp) с ошибкой считался значит закончили (вернул False), input.eof не нужон
        text += temp;

    text += '\0';
    input.close();
    cout << text << endl;
    return text;
}

int main()
{
    string filename = "test.txt";
    string text = convert(filename);
    if (text == "NULL") {
        // Обрабатываем кейс когда файл не найден.
        cout << "The file for reading was not found in the directory.";
        return 1;
    }
    Lexer lexer(text);

    int sz = text.size();
    while (lexer.get_pos() != (sz - 1)){
        Token token = lexer.getNextToken();
        cout << "Token Type: " << token.type << ", Value: " << token.value << "\n";
    }

    if (lexer.get_pos() == (sz - 1))
        cout << "End of lexical analysis";
    else
        cout << "Error during lexical analysis";
    return 1;
}