#include "model.h"

/*
Метод для ввода нового выражения
*/
void s21::Model::InputNewExpression(std::string expression) {
  expression_ = expression;
  stack_.clear();
  output_.clear();
}

/*
Главный метод подсчета, вызывающий методы для
приведения выражения в нужный вид,
парсинга и подсчета результата.
Возвращает лексему,
хранящую в себе пару из double и int вида:
{[ответ], NUM} в случае, если выражение было корректным, и
{[0.0], ERROR} в случае, если выражение было некорректным
*/
s21::Model::lexem s21::Model::GetCalculated() {
  if (BringToNormal_() == 0) {
    if (ParseCycle_() == 0) {
      if (GetResult_() == 1)
        ErrorHandling_();
    } else {
      ErrorHandling_();
    }
  } else {
    ErrorHandling_();
  }
  if (output_.empty() == true)
    std::swap(stack_, output_);
  return output_.front();
}

/*
Метод для подсчета точек графика
Возвращает
1 - в выражении не было переменной х
0 - точки посчитаны
*/
int s21::Model::GraphCalculation(GraphData *data) {
  int graph_result = 0;
  if (data->expression.find('x') == std::string::npos) {
    graph_result = 1;
  } else if (data->Xmax == data->Xmin) {
    graph_result = 2;
  } else {
    double step = 0.005 * (fabs(data->Xmin) + fabs(data->Xmax));
    double Y = 0;
    double X = (double)data->Xmin;
    data->Ymin = 0;
    data->Ymax = 0;
    std::string expression_tmp = data->expression;
    while (X < (double)data->Xmax) {
      data->x.push_back(X);
      ExpressionReplace_(expression_tmp, std::to_string(X), 'x');
      InputNewExpression(expression_tmp);
      auto result = GetCalculated();
      Y = result.first;
      if (X == (double)data->Xmin) {
        data->Ymin = Y;
        data->Ymax = Y;
        if (data->Ymin == 0) {
          data->Ymin = -1;
          data->Ymax = 1;
        }
      }
      if (Y < data->Ymin)
        data->Ymin = Y;
      if (Y > data->Ymax)
        data->Ymax = Y;
      data->y.push_back(Y);
      X += step;
      expression_tmp = data->expression;
    }
  }
  return graph_result;
}

/*
Метод для подсчета данных кредитного калькулятора
перегрузка для ануитетного кредита
*/
void s21::Model::CalculateMonthPay(CreditAnuitetData *data) {
  std::string formula = "S*(P/(1-(1+P)^(0-C)))";
  ExpressionReplace_(formula, std::to_string(data->credit_sum), 'S');
  ExpressionReplace_(formula, std::to_string(data->percent / (100 * 12)), 'P');
  ExpressionReplace_(formula, std::to_string(data->month_count), 'C');
  InputNewExpression(formula);
  auto result = GetCalculated();
  data->monthly_payment = result.first;
  data->summary_pay = data->monthly_payment * data->month_count;
  data->overpay = data->summary_pay - data->credit_sum;
}

/*
Метод для подсчета данных кредитного калькулятора
перегрузка для дифференцированного кредита
*/
void s21::Model::CalculateMonthPay(CreditDifferensiveData *data) {
  data->monthly_payment_max = 0;
  data->monthly_payment_min = 0;
  CalcDifferensiveCycle_(data);
  data->overpay = data->summary_pay - data->credit_sum;
}

void s21::Model::CalcDifferensiveCycle_(CreditDifferensiveData *data) {
  std::string formula = "S/N+s*p*31/365/100";
  std::string tmp_expression = formula;
  int i = data->month_count;
  double ostatok = data->credit_sum;
  while (i > 0) {
    tmp_expression = formula;
    ExpressionReplace_(tmp_expression, std::to_string(data->credit_sum), 'S');
    ExpressionReplace_(tmp_expression, std::to_string(ostatok), 's');
    ExpressionReplace_(tmp_expression, std::to_string(data->month_count), 'N');
    ExpressionReplace_(tmp_expression, std::to_string(data->percent), 'p');
    InputNewExpression(tmp_expression);
    auto result = GetCalculated();
    data->summary_pay += result.first;
    if (i == data->month_count) {
      data->monthly_payment_max = result.first;
      data->monthly_payment_min = result.first;
    }
    if (result.first > data->monthly_payment_max)
      data->monthly_payment_max = result.first;
    if (result.first < data->monthly_payment_min)
      data->monthly_payment_min = result.first;
    i--;
    ostatok = ostatok - result.first;
  }
}

/*
Метод, описывающий действия для обработки ошибок,
вызываемый при возникновении проблем
на любой стадии работы приложения
*/
void s21::Model::ErrorHandling_() {
  stack_.clear();
  output_.clear();
  output_.push_front({0.0, kERROR});
}

/*
Метод для приведения строки в нужный вид и
обработки ошибок перед парсингом выражения
возвращает:
true = если была найдена фатальная ошибка
false = если можно продолжить работу
*/
bool s21::Model::BringToNormal_() {
  bool result = false;
  if (expression_.size() != 0) {
    size_t open = 0, close = 0;
    for (auto it = expression_.begin(); it != expression_.end(); ++it) {
      *it = tolower(*it);
      if (*it == kOPEN)
        open++;
      if (*it == kCLOSE)
        close++;
    }
    if (open != close)
      result = true;
  } else {
    result = true;
  }
  return result;
}

/*
Цикл парсинга
определяет на какую лексему мы наткнулись (число/функция/операция),
отправляет лексему в метод фильтрации лексем
и переносит из стэка все лексемы в вывод после прохода по всем лексемам.
Возвращает
0 - если выражение было без ошибок
1 - если была найдена ошибка
*/
int s21::Model::ParseCycle_() {
  int parse_result = 0;
  lexem parsed_lexem = {0.0, 0};
  for (auto it = expression_.begin();
       it != expression_.end() && parsed_lexem.second != kERROR; ++it) {
    if (IsLetter_(*it)) { // funcs
      parsed_lexem = GetFunc_(it);
    } else if (IsNum_(*it)) { // number
      parsed_lexem = GetNumber_(it);
    } else if (*it == ' ') {
      continue;
    } else { // operation
      auto prev_lexem = parsed_lexem;
      parsed_lexem = GetOperation_(it);
      // если цикл наткнулся на унарный минус
      if (parsed_lexem.second == kSUB &&
          (prev_lexem.second != kNUM || it == expression_.begin())) {
        parsed_lexem.second = kU_MIN;
        *it = '~';
      }
    }
    FilterLexem_(parsed_lexem);
  }
  if (parsed_lexem.second != kERROR) {
    MoveToOutput_();
  } else {
    ErrorHandling_();
    parse_result = 1;
  }
  return parse_result;
}

/*
Метод для определения операции (+,-, и пр.)
Принимает итератор на строку с выражением
Ищет данный символ в словаре с операциями
Возвращает лексему с кодом найденной операции,
либо кодом ошибки, в случае если найден неизвестный символ
*/
s21::Model::lexem s21::Model::GetOperation_(parse_iter &iter) {
  lexem result = {0.0, 0};
  auto operation = operations_dictionary_.find(*iter);
  // если данная операция есть в словаре
  if (operation != operations_dictionary_.end())
    result.second = (*operation).second;
  else
    result.second = kERROR;
  return result;
}

/*
Метод для определения функции (sin, cos, и пр.)
Принимает итератор на строку с выражением
Ищет данную ф-цию в словаре с ф-циями
Возвращает лексему с кодом найденной ф-ции,
либо кодом ошибки, в случае если найдена неизвестная ф-ция
*/
s21::Model::lexem s21::Model::GetFunc_(parse_iter &iter) {
  lexem result = {0.0, 0};
  size_t pos = iter - expression_.begin();
  size_t n = pos;
  while (n < expression_.size() && IsLetter_(expression_.at(n)))
    n++;
  std::string separated_func = expression_.substr(pos, n - pos);
  auto function = functions_dictionary_.find(separated_func);
  // если данная ф-я есть в словаре
  if (function != functions_dictionary_.end()) {
    result.second = (*function).second;
    iter += (separated_func.size() - 1);
  } else {
    result.second = kERROR;
  }
  return result;
}

/*
Метод для определения числа
Принимает итератор на строку с выражением
Переводит строку в double
Возвращает лексему с числом,
либо кодом ошибки, в случае если в числе было две точки
*/
s21::Model::lexem s21::Model::GetNumber_(parse_iter &iter) {
  lexem result = {0.0, 0};
  size_t pos = iter - expression_.begin();
  size_t n = pos;
  size_t dots_counter = 0;
  while (n < expression_.size() && IsNum_(expression_.at(n))) {
    if (expression_.at(n) == '.')
      dots_counter++;
    n++;
  }
  if (dots_counter > 1) {
    result.second = kERROR;
  } else {
    std::string separated_num = expression_.substr(pos, n - pos);
    if (*(iter - 1) == '~') {
      separated_num.insert(separated_num.begin(), '-');
      iter += (separated_num.size() - 2);
    } else {
      iter += (separated_num.size() - 1);
    }
    result.first = atof(separated_num.c_str());
  }
  return result;
}

/*
Метод для опеределения приоритета операции
Принимает текущую лексему
Возвращает приоритет операции 0 - 4
*/
int s21::Model::CalcPrecedence_(lexem curr_lexem) {
  int precedence = 0;
  if (curr_lexem.second >= kCOS && curr_lexem.second <= kLOG)
    precedence = 4;
  else if (curr_lexem.second == kPOW)
    precedence = 3;
  else if (curr_lexem.second >= kMULT && curr_lexem.second <= kMOD)
    precedence = 2;
  else if (curr_lexem.second >= kSUM && curr_lexem.second <= kSUB)
    precedence = 1;
  return precedence;
}

/*
Метод фильтрации лексем
Принимает текущую лексему и
направляет ее в нужный список
*/
void s21::Model::FilterLexem_(lexem curr_lexem) {
  if (curr_lexem.second != kERROR) {
    if (curr_lexem.second == kNUM) {
      output_.push_back(curr_lexem);
    } else if (curr_lexem.second == kU_MIN) {
      ;
    } else {
      if (!ProcessBrackets_(curr_lexem)) {
        ProcessPrecedence_(curr_lexem);
      }
    }
  }
}

/*
Обработка скобок в выражении,
принимает текущую лексему, возвращает:
false - если лексема была не скобкой
true - если лексема была скобкой
*/
bool s21::Model::ProcessBrackets_(lexem curr_lexem) {
  bool is_bracket = false;
  if (curr_lexem.second == kOPEN) {
    stack_.push_front(curr_lexem);
    is_bracket = true;
  } else if (curr_lexem.second == kCLOSE) {
    while (!stack_.empty() && stack_.front().second != kOPEN) {
      output_.push_back(stack_.front());
      stack_.pop_front();
    }
    stack_.pop_front();
    is_bracket = true;
  }
  return is_bracket;
}

/*
Метод опеределяющий в какой из списков добавить лексему
*/
void s21::Model::ProcessPrecedence_(lexem curr_lexem) {
  if (CalcPrecedence_(curr_lexem) > CalcPrecedence_(stack_.front())) {
    stack_.push_front(curr_lexem);
  } else if (CalcPrecedence_(curr_lexem) < CalcPrecedence_(stack_.front())) {
    while (!stack_.empty() &&
           CalcPrecedence_(curr_lexem) < CalcPrecedence_(stack_.front())) {
      output_.push_back(stack_.front());
      stack_.pop_front();
    }
    stack_.push_front(curr_lexem);
  } else {
    if (curr_lexem.second == kMOD) {
      stack_.push_front(curr_lexem);
    } else {
      output_.push_back(stack_.front());
      stack_.pop_front();
      stack_.push_front(curr_lexem);
    }
  }
}

/*
Метод перемещающий все лексемы из стека в аутпут
Вызывается после цикла парсинга и распределения всех лексем
*/
void s21::Model::MoveToOutput_() {
  if (!stack_.empty()) {
    while (!stack_.empty()) {
      output_.push_back(stack_.front());
      stack_.pop_front();
    }
  }
}

/*
Метод подсчета ответа, использует заранее
заготовленный список аутпут.
Возвращает:
0 - если подсчет прошел успешно
1 - если в выражении была ошибка
*/
int s21::Model::GetResult_() {
  int result = 0;
  int one_number = 0;
  if (output_.front().second == kNUM && output_.size() == 1)
    one_number = 1;
  while (1) {
    if (output_.front().second == kNUM) {
      stack_.push_front(output_.front());
      output_.pop_front();
      if (one_number == 1)
        break;
    } else {
      if (ApplyOperation_() == 1) {
        result = 1;
        break;
      }
      if (output_.size() == 1)
        break;
    }
  }
  return result;
}

/*
Метод для подсчета промежуточных ответов
Вызывает методы класса calculation для подсчета результатов
Возвращает:
0 - промежуточный ответ был рассчитан
1 - в выражении была ошибка
*/
int s21::Model::ApplyOperation_() {
  double num1 = 0, num2 = 0, res = 0;
  int result = 0;
  int operation = output_.front().second;
  if (operation >= kCOS && operation <= kLOG) {
    if (GetOne_(&num1) == 0)
      res = calculation_.CalcOne(operation, num1);
    else
      result = 1;
  } else {
    if (GetTwo_(&num1, &num2) == 0)
      res = calculation_.CalcTwo(operation, num1, num2);
    else
      result = 1;
  }
  lexem result_lexem = {res, kNUM};
  output_.push_front(result_lexem);
  return result;
}

/*
Метод получения двух операндов для подсчета промежуточного ответа
Принимает два указателя на числа
Берет числа из стека, удаляет операцию из аутпута
Возвращает:
0 - числа и операция были успешно взяты из списка
1 - в списках не хватает данных (к примеру: 2*3**)
*/
int s21::Model::GetTwo_(double *num1, double *num2) {
  int result = 0;
  if (stack_.size() > 1 && output_.size() > 0) {
    *num1 = stack_.front().first;
    stack_.pop_front();
    *num2 = stack_.front().first;
    stack_.pop_front();
    output_.pop_front();
  } else {
    result = 1;
  }
  return result;
}

/*
Метод получения операнда для подсчета промежуточного ответа
Принимает указатель на число
Берет число из стека, удаляет функцию из аутпута
Возвращает:
0 - число и ф-ция были успешно взяты из списка
1 - в списках не хватает данных (к примеру: sin())
*/
int s21::Model::GetOne_(double *num) {
  int result = 0;
  if (stack_.size() > 0 && output_.size() > 0) {
    *num = stack_.front().first;
    stack_.pop_front();
    output_.pop_front();
  } else {
    result = 1;
  }
  return result;
}

/*
Проверка на то, является ли символ
числом либо точкой
*/
bool s21::Model::IsNum_(char symbol) {
  return ((symbol >= '0' && symbol <= '9') || symbol == '.') ? true : false;
}

/*
Проверка на то, является ли символ
буквой
*/
bool s21::Model::IsLetter_(char symbol) {
  return (symbol >= 'a' && symbol <= 'z') ? true : false;
}

/*
Метод для замены знака в выражении на число
*/
void s21::Model::ExpressionReplace_(std::string &src, const std::string sub,
                                    char sym) {
  size_t pos = 0;
  while ((pos = src.find(sym)) != std::string::npos)
    src.replace(pos, 1, sub);
}
