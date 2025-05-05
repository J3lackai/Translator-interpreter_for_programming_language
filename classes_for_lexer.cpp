#include <string>
using namespace std;

enum TokenType
{
    ID,          // Идентификатор
    INT_CONST,   // Целочисленная константа
    FLOAT_CONST, // Вещественная константа
    OPERATOR,    // Оператор
    DELIMITER,   // Разделитель
    KEYWORD,     // Ключевое слово
    TOKEN_ERR,   // Ошибка
    TOKEN_FIN    // Успешно распознали всю входную последовательность
};
struct Token
{
    TokenType type;
    string value;

    Token(TokenType t, const string &v) : type(t), value(v) {}
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
    TERM,   // (, ), {, }, ;
    FIN,    // Успешно распознали всю входную последовательность
    ERR     // Ошибка
};
class Lexer
{
private:
    std::string input;   // Входной текст
    size_t pos;          // Текущая позиция в тексте
    State current_state; // Текущее состояние, выделил потому чтобы кучу раз во все функции не передавать аргументом.
    char currentChar;    // Текущий символ

    // Вспомогательные функции
    void advance();                  // Переход к следующему символу
    State nextState(char);           // Определение следующего состояния
    Token makeToken(const string &); // Создание токена

public:
    Lexer(const string &text); // Конструктор
    Token getNextToken();      // Получение следующего токена
};