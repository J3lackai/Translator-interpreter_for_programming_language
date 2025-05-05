#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include "classes_for_lexer.cpp"
using namespace std;

bool isskip(char ch)
{
    return (isspace(ch) || ch == ';' || ch == '(' || ch == ')' || ch == '{' || ch == '}');
}

Lexer::Lexer(const string &text) : input(text), pos(0)
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
    if (pos < input.length()) // Проверка
        currentChar = input[pos];
    else
        currentChar = '\0'; // Конец строки
}

State Lexer::nextState(char ch)
{
    switch (current_state)
    {
    case START:
        if (isalpha(ch))
            return IDENT;
        if (isdigit(ch))
            return INT;
        if (isskip(ch))
            return START;
        if (ch == '=')
            return EQU;
        if (ch == '<')
            return LES;
        if (ch == '>')
            return GRT;
        if (ch == ';')
            return Z;
        if (ch == '\0')
            return FIN;
        return ERR;

    case IDENT:
        if (isalnum(ch))
            return IDENT;
        if (isskip(ch))
            return Z;
        if (ch == '\0')
            return FIN;
        return Z_STAR;

    case INT:
        if (isdigit(ch))
            return INT;
        if (ch == '.')
            return DOT;
        if (isalpha(ch))
            return ERR;
        if (isskip(ch))
            return Z;
        if (ch == '\0')
            return FIN;
        return Z_STAR;

    case DOT:
        if (isdigit(ch))
            return FLOAT;
        return ERR;

    case FLOAT:
        if (isdigit(ch))
            return FLOAT;
        if (isalpha(ch) || ch == '.')
            return ERR;
        if (ch == '\0')
            return FIN;
        return Z_STAR;

    case LES:
        if (ch == '=' || ch == '>' || isskip(ch))
            return Z;
        if (isalnum(ch))
            return Z_STAR;
        return ERR;

    case GRT:
        if (ch == '=' || isskip(ch))
            return Z;
        if (isalnum(ch))
            return Z_STAR;
        return ERR;

    case EQU:
        if (ch == '=' || isskip(ch))
            return Z;
        if (isalnum(ch))
            return Z_STAR;
        return ERR;

    default:
        return ERR;
    }
}

Token Lexer::makeToken(const string &lexeme)
{
    unordered_map<string, TokenType>::const_iterator it; // Выносим объявление
    static unordered_map<string, TokenType> keywords = {
        {"if", KEYWORD}, {"else", KEYWORD}, {"while", KEYWORD}, {"print", KEYWORD}, {"read", KEYWORD}};
    switch (current_state)
    {
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
    case FIN:
        return Token(TOKEN_FIN, "┴");
    case ERR:
        return Token(TOKEN_ERR, "");
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

Token Lexer::getNextToken()
{
    current_state = START;
    State nextState = current_state;
    string lexeme;
    while (currentChar != EOF && currentChar != '\0')
    {
        while (isskip(currentChar))
        {
            advance();
        }
        nextState = this->nextState(currentChar);
        if (nextState == Z || nextState == Z_STAR || nextState == ERR)
        { // Токен делаем и для ERR чтобы обработать было проще.
            // Если достигнуто состояние Z или Z*, создаем токен
            Token token = makeToken(lexeme);

            // Если состояние Z_STAR, откатываем позицию на 1 символ
            if (nextState == Z_STAR)
            {
                pos--;                    // Откат позиции
                currentChar = input[pos]; // Обновляем текущий символ
            }

            return token;
        }
        if (current_state != Z_STAR)
            lexeme += currentChar;
        current_state = nextState;
        advance();
    }
    current_state = nextState;
    return makeToken("");
}
