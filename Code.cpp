#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <sqlite3.h>

/*
    Класс Employee представляет сотрудника с полями: fullName, birthDate, gender и age.
    Конструктор инициализирует эти поля.
    Методы getFullName, getBirthDate, getGender и getAge возвращают соответствующие поля.
    Статический метод calculateAge вычисляет возраст на основе даты рождения.
*/
class Employee {
public:
    Employee(const std::string& fullName, const std::string& birthDate, const std::string& gender, int age)
        : fullName(fullName), birthDate(birthDate), gender(gender), age(age) {}

    std::string getFullName() const { return fullName; }
    std::string getBirthDate() const { return birthDate; }
    std::string getGender() const { return gender; }
    int getAge() const { return age; }

    static int calculateAge(const std::string& birthDate);

private:
    std::string fullName;
    std::string birthDate;
    std::string gender;
    int age;
};

/*
    Преобразует строку с датой рождения в структуру tm.
    Преобразует tm в time_t.
    Вычисляет разницу во времени между текущим временем и временем рождения.
    Возвращает возраст в годах.
*/
int Employee::calculateAge(const std::string& birthDate) {
    std::tm tm = {};
    std::istringstream ss(birthDate);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    std::time_t birthTime = std::mktime(&tm);

    std::time_t now = std::time(nullptr);
    double ageInSeconds = std::difftime(now, birthTime);
    return static_cast<int>(ageInSeconds / (365.25 * 24 * 60 * 60));
}

/* 
    Database
    Конструктор открывает соединение с базой данных и бросает исключение в случае ошибки.
    Деструктор закрывает соединение с базой данных.
    createTable создает таблицу с полями: ID, FULLNAME, BIRTHDATE, GENDER и AGE.
    insertEmployee добавляет запись сотрудника в базу данных.
    getAllEmployees извлекает всех сотрудников из базы данных и возвращает их в виде вектора.
    insertMultipleEmployees вставляет несколько записей сотрудников.
    getEmployeesByCriteria извлекает сотрудников по заданным критериям.
    callback преобразует строки результата запроса в объекты Employee.
*/
class Database {
public:
    Database(const std::string& dbName) {
        if (sqlite3_open(dbName.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Не удается открыть базу данных: " + std::string(sqlite3_errmsg(db)));
        }
    }

    ~Database() {
        sqlite3_close(db);
    }

    void createTable() {
        const char* sql = "CREATE TABLE IF NOT EXISTS EMPLOYEE("
            "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
            "FULLNAME TEXT NOT NULL, "
            "BIRTHDATE TEXT NOT NULL, "
            "GENDER TEXT NOT NULL, "
            "AGE INTEGER NOT NULL);";

        char* errMsg = 0;
        if (sqlite3_exec(db, sql, 0, 0, &errMsg) != SQLITE_OK) {
            std::string error = "SQL error: " + std::string(errMsg);
            sqlite3_free(errMsg);
            throw std::runtime_error(error);
        }
    }

    void insertEmployee(const Employee& employee) {
        std::string sql = "INSERT INTO EMPLOYEE (FULLNAME, BIRTHDATE, GENDER, AGE) VALUES ('" +
            employee.getFullName() + "', '" + employee.getBirthDate() + "', '" + employee.getGender() + "', " +
            std::to_string(employee.getAge()) + ");";

        char* errMsg = 0;
        if (sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg) != SQLITE_OK) {
            std::string error = "SQL error: " + std::string(errMsg);
            sqlite3_free(errMsg);
            throw std::runtime_error(error);
        }
    }

    std::vector<Employee> getAllEmployees() {
        const char* sql = "SELECT * FROM EMPLOYEE ORDER BY FULLNAME;";
        std::vector<Employee> employees;
        char* errMsg = 0;

        if (sqlite3_exec(db, sql, callback, &employees, &errMsg) != SQLITE_OK) {
            std::string error = "SQL error: " + std::string(errMsg);
            sqlite3_free(errMsg);
            throw std::runtime_error(error);
        }

        return employees;
    }

    void insertMultipleEmployees(const std::vector<Employee>& employees) {
        for (const auto& employee : employees) {
            insertEmployee(employee);
        }
    }

    std::vector<Employee> getEmployeesByCriteria() {
        const char* sql = "SELECT * FROM EMPLOYEE WHERE GENDER = 'Male' AND FULLNAME LIKE 'F%';";
        std::vector<Employee> employees;
        char* errMsg = 0;

        if (sqlite3_exec(db, sql, callback, &employees, &errMsg) != SQLITE_OK) {
            std::string error = "SQL error: " + std::string(errMsg);
            sqlite3_free(errMsg);
            throw std::runtime_error(error);
        }

        return employees;
    }

private:
    sqlite3* db;

    static int callback(void* data, int argc, char** argv, char** azColName) {
        auto* employees = static_cast<std::vector<Employee>*>(data);
        std::string fullName = argv[1];
        std::string birthDate = argv[2];
        std::string gender = argv[3];
        int age = std::stoi(argv[4]);

        employees->emplace_back(fullName, birthDate, gender, age);
        return 0;
    }
};

// Mode1: Создаёт таблицу.
void runMode1(Database& db) {
    db.createTable();
    std::cout << "Таблица была создана." << std::endl;
}

// Mode2: Вставляет запись сотрудника.
void runMode2(Database& db, const std::string& fullName, const std::string& birthDate, const std::string& gender) {
    int age = Employee::calculateAge(birthDate);
    Employee employee(fullName, birthDate, gender, age);
    db.insertEmployee(employee);
    std::cout << "Сотрудник был успешно добавлен." << std::endl;
}

// Mode3: Выводит всех сотрудников.
void runMode3(Database& db) {
    auto employees = db.getAllEmployees();
    for (const auto& employee : employees) {
        std::cout << employee.getFullName() << ", " << employee.getBirthDate() << ", " << employee.getGender()
            << ", Возраст: " << employee.getAge() << std::endl;
    }
}

// Mode4: Вставляет 1.000.100 сотрудников.
void runMode4(Database& db) {
    std::vector<Employee> employees;
    for (int i = 0; i < 1000000; ++i) {
        std::string gender = (i % 2 == 0) ? "Male" : "Female";
        std::string birthDate = "1990-01-01";
        int age = Employee::calculateAge(birthDate);
        employees.emplace_back("Name" + std::to_string(i), birthDate, gender, age);
    }
    for (int i = 0; i < 100; ++i) {
        std::string birthDate = "1990-01-01";
        int age = Employee::calculateAge(birthDate);
        employees.emplace_back("F" + std::to_string(i), birthDate, "Male", age);
    }
    db.insertMultipleEmployees(employees);
    std::cout << "1000100 сотрудников были успешно добавлены." << std::endl;
}

// Mode5: Выполняет выборку сотрудников по критерию и измеряет время выполнения
void runMode5(Database& db) {
    auto start = std::chrono::high_resolution_clock::now();
    auto employees = db.getEmployeesByCriteria();
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> diff = end - start;
    std::cout << "Запрос выполнен за " << diff.count() << " секунд." << std::endl;

    for (const auto& employee : employees) {
        std::cout << employee.getFullName() << ", " << employee.getBirthDate() << ", " << employee.getGender()
            << ", Возраст: " << employee.getAge() << std::endl;
    }
}

// main
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Invalid arguments." << std::endl;
        return 1;
    }

    try {
        Database db("employees.db");

        int mode = std::stoi(argv[1]);
        switch (mode) {
        case 1:
            runMode1(db);
            break;
        case 2:
            if (argc < 5) {
                std::cerr << "Invalid arguments for mode 2." << std::endl;
                return 1;
            }
            runMode2(db, argv[2], argv[3], argv[4]);
            break;
        case 3:
            runMode3(db);
            break;
        case 4:
            runMode4(db);
            break;
        case 5:
            runMode5(db);
            break;
        default:
            std::cerr << "Unknown mode." << std::endl;
            return 1;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
