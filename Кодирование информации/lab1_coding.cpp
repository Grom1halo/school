/*
 * Лабораторная работа №1
 * Тема: Построение кода с контролем ошибок (табличный метод)
 *
 * Программа:
 *   - при заданной мощности M и числе исправляемых ошибок t определяет
 *     минимальную длину кода n, формирует разрешённые и запрещённые комбинации;
 *   - реализует кодер: decimal [0..M-1] -> разрешённое кодовое слово;
 *   - реализует внесение ошибки заданной кратности;
 *   - реализует декодер: исправление/обнаружение ошибок, отказ от декодирования.
 */

#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <random>
#include <limits>
#include <string>

using namespace std;

// ---------- Вспомогательные функции ----------

// Вес Хэмминга (число единиц)
int weight(int x) {
    int cnt = 0;
    while (x) { cnt += x & 1; x >>= 1; }
    return cnt;
}

// Расстояние Хэмминга между двумя словами
int hammingDist(int a, int b) {
    return weight(a ^ b);
}

// Минимальное кодовое расстояние набора кодовых слов
int minCodeDist(const vector<int>& cw) {
    if ((int)cw.size() < 2) return INT_MAX;
    int minD = INT_MAX;
    for (int i = 0; i < (int)cw.size(); i++)
        for (int j = i + 1; j < (int)cw.size(); j++)
            minD = min(minD, hammingDist(cw[i], cw[j]));
    return minD;
}

// Жадный поиск M слов длины n бит с min расстоянием >= dMin
// Возвращает true, если удалось набрать M слов
bool findAllowed(int n, int M, int dMin, vector<int>& result) {
    result.clear();
    int total = 1 << n;
    result.push_back(0);
    for (int w = 1; w < total && (int)result.size() < M; w++) {
        bool ok = true;
        for (int c : result) {
            if (hammingDist(w, c) < dMin) { ok = false; break; }
        }
        if (ok) result.push_back(w);
    }
    return (int)result.size() >= M;
}

// Минимальная длина кода n для заданных M и dMin
int findN(int M, int dMin) {
    // Нижняя граница: нужно хотя бы ceil(log2(M)) информационных разрядов
    int n = (M > 1) ? (int)ceil(log2((double)M)) : 1;
    if (n < 1) n = 1;
    vector<int> tmp;
    while (n <= 25) {
        if (findAllowed(n, M, dMin, tmp)) return n;
        n++;
    }
    return -1; // не нашли
}

// Вывод двоичного слова длины n (старший бит слева)
void printBin(int w, int n) {
    for (int i = n - 1; i >= 0; i--)
        cout << ((w >> i) & 1);
}

// Безопасный ввод целого числа в диапазоне [lo, hi]
int readInt(const string& prompt, int lo, int hi) {
    while (true) {
        cout << prompt;
        int v;
        if (cin >> v) {
            if (v >= lo && v <= hi) return v;
            cout << "  Ошибка: введите число в диапазоне ["
                 << lo << ", " << hi << "]\n";
        } else {
            cout << "  Ошибка: ожидается целое число.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
}

// Внесение ошибки заданной кратности в слово длины n бит
int injectErrors(int word, int n, int errCount, mt19937& rng) {
    vector<int> pos(n);
    iota(pos.begin(), pos.end(), 0);
    shuffle(pos.begin(), pos.end(), rng);
    int result = word;
    for (int i = 0; i < errCount; i++)
        result ^= (1 << pos[i]);
    return result;
}

// ---------- Декодирование ----------

// Результат декодирования
struct DecodeResult {
    bool isAllowed;       // true — принятое слово разрешённое
    bool corrected;       // true — ошибка исправлена
    bool refused;         // true — отказ от декодирования
    bool ambiguous;       // true — неоднозначность
    int  data;            // данные (если isAllowed или corrected)
    int  errorDistance;   // кратность обнаруженной ошибки
};

DecodeResult decode(int received, const vector<int>& allowed, int M, int t,
                    const vector<bool>& isAllowedTable) {
    DecodeResult res{};
    res.data = -1;

    if (isAllowedTable[received]) {
        res.isAllowed = true;
        for (int i = 0; i < M; i++)
            if (allowed[i] == received) { res.data = i; break; }
        return res;
    }

    // Запрещённое слово — ищем ближайшую разрешённую комбинацию
    int bestDist = INT_MAX, bestIdx = -1;
    bool ambig = false;
    for (int i = 0; i < M; i++) {
        int d = hammingDist(received, allowed[i]);
        if (d < bestDist) { bestDist = d; bestIdx = i; ambig = false; }
        else if (d == bestDist) ambig = true;
    }

    res.errorDistance = bestDist;

    if (ambig) {
        // Равноудалено от нескольких разрешённых — неоднозначность
        res.ambiguous = true;
        res.refused = true;
    } else if (t == 0) {
        // Только обнаружение ошибок
        res.refused = true;
    } else if (bestDist <= t) {
        // Ошибка исправима
        res.corrected = true;
        res.data = bestIdx;
    } else {
        // Ошибка обнаружена, но не исправима
        res.refused = true;
    }
    return res;
}

// ---------- Главная программа ----------

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    cout << "========================================\n";
    cout << "   Лабораторная работа 1\n";
    cout << "   Код с контролем ошибок\n";
    cout << "========================================\n\n";

    // --- Ввод параметров ---
    int M = readInt("Введите мощность кода M (>= 2): ", 2, 512);
    int t = readInt("Введите число исправляемых ошибок t\n"
                    "  (0 = только обнаружение однократной ошибки, >=1 = исправление): ",
                    0, 5);

    int dMin = (t == 0) ? 2 : 2 * t + 1;
    cout << "\nМинимальное кодовое расстояние: d = " << dMin << "\n";

    // --- Определение длины кода ---
    cout << "Поиск минимальной длины кода n ...\n";
    int n = findN(M, dMin);
    if (n < 0) {
        cerr << "Не удалось подобрать длину кода (n > 25). "
                "Уменьшите M или t.\n";
        return 1;
    }

    // --- Формирование разрешённых комбинаций ---
    vector<int> allowed;
    findAllowed(n, M, dMin, allowed);
    allowed.resize(M);

    int actualD = minCodeDist(allowed);
    int total = 1 << n;

    cout << "Длина кода: n = " << n << "\n";
    cout << "Кодовое расстояние: d = " << actualD << "\n";
    cout << "Всего комбинаций 2^n = " << total << "\n";
    cout << "  Разрешённых: " << M << "\n";
    cout << "  Запрещённых: " << (total - M) << "\n\n";

    // Таблица разрешённых для быстрого поиска
    vector<bool> isAllowedTable(total, false);
    for (int c : allowed) isAllowedTable[c] = true;

    // --- Вывод кодера ---
    cout << "Таблица кодера:\n";
    cout << "  Данные | Кодовое слово\n";
    cout << "  -------+-" << string(n, '-') << "\n";
    for (int i = 0; i < M; i++) {
        cout << "  " << i << "      | ";
        printBin(allowed[i], n);
        cout << "\n";
    }

    // --- Вывод запрещённых комбинаций ---
    cout << "\nЗапрещённые комбинации: ";
    bool first = true;
    for (int i = 0; i < total; i++) {
        if (!isAllowedTable[i]) {
            if (!first) cout << "  ";
            printBin(i, n);
            first = false;
        }
    }
    cout << "\n";

    // --- Таблица декодера ---
    cout << "\nТаблица декодера (все " << total << " комбинаций):\n";
    cout << "  Принятое слово | Результат\n";
    cout << "  " << string(n, '-') << "----+-" << string(12, '-') << "\n";
    for (int i = 0; i < total; i++) {
        cout << "  ";
        printBin(i, n);
        cout << "     | ";
        auto r = decode(i, allowed, M, t, isAllowedTable);
        if (r.isAllowed)
            cout << "разрешённое -> " << r.data;
        else if (r.corrected)
            cout << "исправлено  -> " << r.data
                 << " (ошибка кратности " << r.errorDistance << ")";
        else if (r.ambiguous)
            cout << "ОТКАЗ (неоднозначность)";
        else if (t == 0)
            cout << "ОШИБКА ОБНАРУЖЕНА";
        else
            cout << "ОТКАЗ (кратность " << r.errorDistance
                 << " > t=" << t << ")";
        cout << "\n";
    }

    // --- Скорость кода ---
    int k = (M > 1) ? (int)ceil(log2((double)M)) : 1;
    cout << "\nИнформационных разрядов k = ceil(log2(" << M << ")) = " << k << "\n";
    cout << "Скорость кода R = k/n = " << k << "/" << n
         << " = " << (double)k / n << "\n";

    // --- Интерактивная работа ---
    mt19937 rng(random_device{}());

    while (true) {
        cout << "\n-------- Меню --------\n";
        cout << "1. Кодировать сообщение\n";
        cout << "2. Декодировать слово вручную\n";
        cout << "3. Демонстрация: кодирование + ошибка + декодирование\n";
        cout << "0. Выход\n";
        int choice = readInt("Выбор: ", 0, 3);

        if (choice == 0) break;

        // --- Кодирование ---
        if (choice == 1) {
            int data = readInt("Данные [0.." + to_string(M - 1) + "]: ", 0, M - 1);
            cout << "Закодировано: ";
            printBin(allowed[data], n);
            cout << "\n";

        // --- Декодирование вручную ---
        } else if (choice == 2) {
            cout << "Введите " << n << "-битное слово: ";
            string s;
            cin >> s;

            // Проверка формата
            if ((int)s.size() != n || s.find_first_not_of("01") != string::npos) {
                cout << "Ошибка: ожидается строка из " << n << " бит (только 0 и 1).\n";
                continue;
            }
            int word = 0;
            for (char c : s) word = (word << 1) | (c - '0');

            auto r = decode(word, allowed, M, t, isAllowedTable);
            if (r.isAllowed) {
                cout << "Разрешённое слово. Данные: " << r.data << "\n";
            } else if (r.corrected) {
                cout << "Запрещённое слово. Ошибка исправлена"
                     << " (кратность " << r.errorDistance << ")."
                     << " Данные: " << r.data << "\n";
            } else if (r.ambiguous) {
                cout << "Запрещённое слово. Неоднозначное декодирование. ОТКАЗ.\n";
            } else if (t == 0) {
                cout << "Запрещённое слово. ОШИБКА ОБНАРУЖЕНА. Отказ от декодирования.\n";
            } else {
                cout << "Запрещённое слово. Ошибка не может быть исправлена"
                     << " (кратность " << r.errorDistance << " > t=" << t << "). ОТКАЗ.\n";
            }

        // --- Демонстрация ---
        } else if (choice == 3) {
            int data = readInt("Данные [0.." + to_string(M - 1) + "]: ", 0, M - 1);
            int encoded = allowed[data];
            cout << "  Закодировано:  "; printBin(encoded, n); cout << "\n";

            // Кратность ошибки (разрешаем вводить > t, чтобы показать обнаружение)
            int maxErr = min(n, t > 0 ? t + 1 : 2);
            int errCnt = readInt("Кратность ошибки [1.." + to_string(maxErr) + "]: ",
                                 1, maxErr);

            int corrupted = injectErrors(encoded, n, errCnt, rng);
            int errVec    = encoded ^ corrupted;

            cout << "  Вектор ошибки: "; printBin(errVec, n);
            cout << " (кратность " << weight(errVec) << ")\n";
            cout << "  Принято:       "; printBin(corrupted, n); cout << "\n";

            auto r = decode(corrupted, allowed, M, t, isAllowedTable);
            cout << "  Декодировано:  ";
            if (r.isAllowed) {
                if (r.data == data)
                    cout << r.data << "  [ошибок нет / ошибка не обнаружена]\n";
                else
                    cout << r.data << "  [ОШИБКА НЕ ОБНАРУЖЕНА! Должно: " << data << "]\n";
            } else if (r.corrected) {
                cout << r.data;
                cout << (r.data == data ? "  [исправлено верно]"
                                        : "  [исправлено НЕВЕРНО! Должно: "
                                          + to_string(data) + "]");
                cout << "\n";
            } else if (r.ambiguous) {
                cout << "ОТКАЗ (неоднозначность)\n";
            } else if (t == 0) {
                cout << "ОШИБКА ОБНАРУЖЕНА, декодирование отклонено\n";
            } else {
                cout << "ОТКАЗ (ошибка кратности " << r.errorDistance
                     << " > t=" << t << ")\n";
            }
        }
    }

    cout << "\nПрограмма завершена.\n";
    return 0;
}
