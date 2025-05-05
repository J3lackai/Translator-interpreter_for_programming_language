using namespace std;
#include <string>
#include <iostream>
#include <fstream>
#include "lexer_methods.cpp"

string convert(string filename) // преобразование файла в одну строку
{
    string text;
    fstream input;
    string temp;
    input.open(filename, ios::in);
    if (!input.is_open())
        return "NULL";

    while (getline(input, temp)) // Если getline(input, temp) с ошибкой считался значит закончили (вернул False), input.eof не нужон
        text += temp + '\n';
    if (!text.empty()) // Очень важно. В исходном файле был символ \0, но getline его не записывает в переменную, значит добавляем
        text[text.size() - 1] = '\0';
    else
        text += '\0';
    input.close();
    return text;
}
int main()
{
    // файл с тестами теперь называется test_for_lexer
    string filename = "C:\\C_Projects\\lexer\\test_for_lexer.txt"; // по-другому нихера не работает
    string text = convert(filename);
    if (text == "NULL")
    {
        // Обрабатываем кейс когда файл не найден.
        cout << "The file for reading was not found in the directory.";
        return 1;
    }
    Lexer lexer(text);

    Token token = lexer.getNextToken();
    while (token.type != TOKEN_ERR && token.type != TOKEN_FIN)
    {
        std::cout << "Token Type: " << token.type << ", Value: " << token.value << "\n";
        token = lexer.getNextToken();
    }
    if (token.type == TOKEN_FIN)
    {
        cout << "All characters in the input stream have been successfully recognized";
        return 0;
    }
    cout << "Error during lexical analysis";
    return 1;
}