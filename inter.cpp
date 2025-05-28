using namespace std;
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <variant>   // Для   variant
#include <stdexcept> // Для   runtime_error
#include "syntaxer.cpp"
// --- ИНТЕРПРЕТАТОР (Задача 3) ---
class Interpreter
{
private:
    // Стек для выполнения ОПС (операнды, промежуточные результаты)
    //   variant позволяет хранить int или float
    vector<variant<int, float, string>> runtime_stack;

    // Таблица символов (переменных)
    // Хранит имена переменных и их текущие значения (int или float)
    unordered_map<string, variant<int, float, string>> symbol_table;

    // Карта для разрешения меток: имя метки (L1:) -> индекс в векторе ops_code
    unordered_map<string, size_t> label_addresses;

    // Вектор с последовательностью ОПС
    const vector<OPSElement> &ops_code; // Ссылка на сгенерированный код ОПС

    // Вспомогательные функции для стека
    void push(variant<int, float, string> val)
    {
        runtime_stack.push_back(val);
    }

    variant<int, float, string> pop()
    {
        if (runtime_stack.empty())
        {
            throw runtime_error("Runtime Error: Stack underflow.");
        }
        variant<int, float, string> val = runtime_stack.back();
        runtime_stack.pop_back();
        return val;
    }

    // Вспомогательные функции для операций
    variant<int, float, string> perform_binary_op(variant<int, float, string> op1, variant<int, float, string> op2, OPSCode op_code);
    bool is_false(const variant<int, float, string> &val);

    // Подготовка меток перед выполнением
    void resolve_labels();

public:
    Interpreter(const vector<OPSElement> &code);
    void run(); // Запускает выполнение ОПС
};

// Конструктор интерпретатора
Interpreter::Interpreter(const vector<OPSElement> &code) : ops_code(code)
{
    // Разрешаем метки сразу после создания
    resolve_labels();
}

// Разрешение меток (заполнение label_addresses)
void Interpreter::resolve_labels()
{
    for (size_t i = 0; i < ops_code.size(); ++i)
    {
        if (ops_code[i].code == OPSCode::OP_LABEL)
        {
            // Метки хранятся как строки, например "L1:"
            label_addresses[get<string>(ops_code[i].value)] = i;
        }
    }
}

// Выполнение бинарных операций (арифметика и сравнения)
variant<int, float, string> Interpreter::perform_binary_op(variant<int, float, string> op1, variant<int, float, string> op2, OPSCode op_code)
{
    // Нужна обработка переменных
    if (holds_alternative<string>(op1))
    {
        string var1 = get<string>(op1);
        if (symbol_table.count(var1))
            op1 = symbol_table[var1];
        else
            throw runtime_error("Runtime Error: Undefined variable.");
    }
    if (holds_alternative<string>(op2))
    {
        string var2 = get<string>(op2);
        if (symbol_table.count(var2))
            op2 = symbol_table[var2];
        else
            throw runtime_error("Runtime Error: Undefined variable.");
    }
    // Обработка типов: если один из операндов float, результат float
    bool is_float = holds_alternative<float>(op1) || holds_alternative<float>(op2);

    float f_op1 = is_float ? (holds_alternative<float>(op1) ? get<float>(op1) : static_cast<float>(get<int>(op1))) : 0.0f;
    float f_op2 = is_float ? (holds_alternative<float>(op2) ? get<float>(op2) : static_cast<float>(get<int>(op2))) : 0.0f;

    int i_op1 = holds_alternative<int>(op1) ? get<int>(op1) : static_cast<int>(get<float>(op1));
    int i_op2 = holds_alternative<int>(op2) ? get<int>(op2) : static_cast<int>(get<float>(op2));

    // Результаты сравнений всегда int (0 или 1)
    if (op_code == OPSCode::OP_LS || op_code == OPSCode::OP_LE || op_code == OPSCode::OP_GS ||
        op_code == OPSCode::OP_GE || op_code == OPSCode::OP_EQ || op_code == OPSCode::OP_NE)
    {
        bool result = true;
        if (is_float)
        {
            switch (op_code)
            {
            case OPSCode::OP_LS:
                result = f_op1 < f_op2;
            case OPSCode::OP_LE:
                result = f_op1 <= f_op2;
            case OPSCode::OP_GS:
                result = f_op1 > f_op2;
            case OPSCode::OP_GE:
                result = f_op1 >= f_op2;
            case OPSCode::OP_EQ:
                result = f_op1 == f_op2;
            case OPSCode::OP_NE:
                result = f_op1 != f_op2;
            }
        }
        else
        {
            switch (op_code)
            {
            case OPSCode::OP_LS:
                result = i_op1 < i_op2;
            case OPSCode::OP_LE:
                result = i_op1 <= i_op2;
            case OPSCode::OP_GS:
                result = i_op1 > i_op2;
            case OPSCode::OP_GE:
                result = i_op1 >= i_op2;
            case OPSCode::OP_EQ:
                result = i_op1 == i_op2;
            case OPSCode::OP_NE:
                result = i_op1 != i_op2;
            }
        }
        return result;
    }

    // Арифметические операции
    if (is_float)
    {
        if (op_code == OPSCode::OP_ADD)
            return f_op1 + f_op2;
        if (op_code == OPSCode::OP_SUB)
            return f_op1 - f_op2;
        if (op_code == OPSCode::OP_MUL)
            return f_op1 * f_op2;
        if (op_code == OPSCode::OP_DIV)
        {
            if (f_op2 == 0.0f)
                throw runtime_error("Runtime Error: Division by zero (float).");
            return f_op1 / f_op2;
        }
    }
    else
    {
        if (op_code == OPSCode::OP_ADD)
            return i_op1 + i_op2;
        if (op_code == OPSCode::OP_SUB)
            return i_op1 - i_op2;
        if (op_code == OPSCode::OP_MUL)
            return i_op1 * i_op2;
        if (op_code == OPSCode::OP_DIV)
        {
            if (i_op2 == 0)
                throw runtime_error("Runtime Error: Division by zero (integer).");
            return i_op1 / i_op2;
        }
    }
    throw runtime_error("Internal Error: Unknown binary operation.");
}

// Проверка, является ли значение "ложным" для условных переходов
bool Interpreter::is_false(const variant<int, float, string> &val)
{
    if (holds_alternative<int>(val))
    {
        return get<int>(val) == 0;
    }
    else if (holds_alternative<float>(val))
    {
        return get<float>(val) == 0.0f;
    }
    return true; // Неожиданный тип, трактуем как ложь
}

// Запуск выполнения ОПС
void Interpreter::run()
{
    size_t program_counter = 0; // Указатель на текущую инструкцию ОПС

    while (program_counter < ops_code.size())
    {
        const OPSElement &current_element = ops_code[program_counter];
        program_counter++; // Переходим к следующей инструкции по умолчанию

        try
        {
            switch (current_element.code)
            {
            // --- Операнды (помещаются на стек) ---
            case OPSCode::OP_INT_CONST:
                // cout << get<int>(current_element.value);
                push(get<int>(current_element.value));
                break;
            case OPSCode::OP_FLOAT_CONST:
                // cout << get<float>(current_element.value);
                push(get<float>(current_element.value));
                break;
            case OPSCode::OP_IDENT:
            {
                string var_name = get<string>(current_element.value);
                // cout << get<string>(current_element.value);
                if (symbol_table.count(var_name))
                {
                    push(var_name); // symbol_table[var_name]
                }
                else
                {
                    symbol_table[var_name] = 0;
                    push(var_name); // symbol_table[var_name]
                }
                break;
            }

            // --- Арифметические операции ---
            case OPSCode::OP_ADD:
            case OPSCode::OP_SUB:
            case OPSCode::OP_MUL:
            case OPSCode::OP_DIV:
            {
                variant<int, float, string> op2 = pop();
                variant<int, float, string> op1 = pop();
                push(perform_binary_op(op1, op2, current_element.code));
                break;
            }

            // --- Сравнения ---
            case OPSCode::OP_LS:
            case OPSCode::OP_LE:
            case OPSCode::OP_GS:
            case OPSCode::OP_GE:
            case OPSCode::OP_EQ:
            case OPSCode::OP_NE:
            {
                variant<int, float, string> op2 = pop();
                variant<int, float, string> op1 = pop();
                push(perform_binary_op(op1, op2, current_element.code));
                break;
            }

            // --- Управление потоком ---
            case OPSCode::OP_LABEL:
                // Метки - это просто маркеры, интерпретатор их пропускает
                break;
            case OPSCode::OP_JF:
            {
                // Ожидаем, что следующая инструкция - это OP_LABEL с именем метки
                if (program_counter >= ops_code.size() || ops_code[program_counter].code != OPSCode::OP_LABEL)
                {
                    throw runtime_error("Internal Error: JF expected a label reference.");
                }
                string target_label_name = get<string>(ops_code[program_counter].value);
                program_counter++; // Пропускаем элемент с меткой

                variant<int, float, string> condition_result = pop();
                if (is_false(condition_result))
                {
                    // Переходим к адресу метки
                    if (label_addresses.count(target_label_name))
                    {
                        program_counter = label_addresses[target_label_name];
                    }
                    else
                    {
                        throw runtime_error("Runtime Error: Undefined label '" + target_label_name + "'.");
                    }
                }
                break;
            }
            case OPSCode::OP_JMP:
            {
                // Ожидаем, что следующая инструкция - это OP_LABEL с именем метки
                if (program_counter >= ops_code.size() || ops_code[program_counter].code != OPSCode::OP_LABEL)
                {
                    throw runtime_error("Internal Error: JMP expected a label reference.");
                }
                string target_label_name = get<string>(ops_code[program_counter].value);
                program_counter++; // Пропускаем элемент с меткой

                // Безусловный переход
                if (label_addresses.count(target_label_name))
                {
                    program_counter = label_addresses[target_label_name];
                }
                else
                {
                    throw runtime_error("Runtime Error: Undefined label '" + target_label_name + "'.");
                }
                break;
            }

            // --- Память ---
            case OPSCode::OP_ASSIGN:
            {
                variant<int, float, string> value_to_assign = pop();
                // Ожидаем, что следующая инструкция - это OP_IDENT с именем переменной  || !holds_alternative<string>(runtime_stack.back())
                if (runtime_stack.empty())
                {
                    throw runtime_error("Internal Error: ASSIGN expected variable name on stack.");
                }
                //  string var_name = get<string>(pop());

                string var_name = get<string>(pop()); // Получаем имя переменной с вершины стека
                // string var_name = get<string>(current_element.value);
                // cout << var_name << "\n\n\n";
                symbol_table[var_name] = value_to_assign; // Присваиваем значение
                break;
            }
            // --- Ввод/Вывод ---
            case OPSCode::OP_READ:
            {
                // Ожидаем, что следующая инструкция - это OP_IDENT с именем переменной || !holds_alternative<string>(runtime_stack.back())
                if (runtime_stack.empty())
                {
                    throw runtime_error("Internal Error: READ expected variable name on stack.");
                }
                string var_name = get<string>(pop()); // Получаем имя переменной с вершины стека
                // string var_name = get<string>(current_element.value);
                cout << "Enter value for " << var_name << ": ";
                string input_str;
                cin >> input_str;

                // Пробуем преобразовать в int или float
                try
                {
                    size_t pos_int;
                    int int_val = stoi(input_str, &pos_int);
                    if (pos_int == input_str.length())
                    { // Вся строка - int
                        symbol_table[var_name] = int_val;
                    }
                    else
                    { // Может быть float
                        size_t pos_float;
                        float float_val = stof(input_str, &pos_float);
                        if (pos_float == input_str.length())
                        { // Вся строка - float
                            symbol_table[var_name] = float_val;
                        }
                        else
                        {
                            throw runtime_error("Runtime Error: Invalid input for variable '" + var_name + "'.");
                        }
                    }
                }
                catch (const exception &e)
                {
                    throw runtime_error("Runtime Error: Invalid input format for variable '" + var_name + "'. " + e.what());
                }
                break;
            }
            case OPSCode::OP_PRINT:
            {
                variant<int, float, string> val = pop();
                if (holds_alternative<int>(val))
                {
                    cout << get<int>(val) << endl;
                }
                else if (holds_alternative<float>(val))
                {
                    cout << get<float>(val) << endl;
                }
                else if (holds_alternative<string>(val))
                {
                    cout << get<string>(val) << endl;
                }
                break;
            }
            case OPSCode::OP_ERROR:
                throw runtime_error("Internal Error: Encountered OP_ERROR in OPS code.");
            }
        }
        catch (const runtime_error &e)
        {
            cerr << e.what() << " OPS index: " << program_counter - 1 << endl;
            break; // Останавливаем выполнение при первой же ошибке
        }
        catch (const bad_variant_access &e)
        {
            cerr << "Internal Runtime Error: Type mismatch in OPS element value at index " << program_counter - 1 << ". " << e.what() << endl;
            break;
        }
    }
}
// --- Главная функция программы ---
int main()
{
    string filename = "test.txt"; // Укажите правильный путь к файлу

    string text = convert(filename);
    cout << text;
    // Создаем лексер с текстом из файла
    Lexer lexer(text);

    vector<OPSElement> ops_code;
    // Создаем парсер, передавая ему лексер
    Parser parser(lexer, ops_code);

    // Запускаем процесс парсинга
    parser.parse();

    // Печатаем сгенерированную ОПС, если не было ошибок синтаксиса
    if (!parser.hasSyntaxError())
    {
        printOPS(ops_code);
        Interpreter inter(ops_code);
        inter.run();
        return 0;
    }
    return 1; // Возвращаем ненулевой код для ошибки синтаксиса или лексической ошибки
}