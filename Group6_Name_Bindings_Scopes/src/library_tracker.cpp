#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// Named constants required by the laboratory task.
constexpr int MAX_LOAN_DAYS = 14;
constexpr double CHARGE_PER_DAY = 20.0;

struct Book {
    string accessionNo;
    string title;
    int availableCopies;
    string status;
};

struct Borrower {
    string studentId;
    string name;
    string borrowedAccessionNo;
};

enum class TransactionType { Borrow, Return };

struct Transaction {
    int id;
    TransactionType type;
    string studentId;
    string accessionNo;
    int day;
    int overdueDays;
    double fine;
    bool success;
    string message;
};

// A normal local object used to demonstrate ordinary automatic lifetime.
struct SearchResult {
    int index;
    Book* book;
};

void refreshStatus(Book& book) {
    book.status = (book.availableCopies > 0) ? "Available" : "No copies";
}

// The local searchIndex has function scope and automatic lifetime.
SearchResult searchBook(vector<Book>& books, const string& accessionNo) {
    int searchIndex = -1;

    for (size_t i = 0; i < books.size(); ++i) {
        if (books[i].accessionNo == accessionNo) {
            searchIndex = static_cast<int>(i);
            break;
        }
    }

    if (searchIndex == -1) {
        return {-1, nullptr};
    }
    return {searchIndex, &books[searchIndex]};
}

const Book* searchBookConst(const vector<Book>& books, const string& accessionNo) {
    for (const auto& book : books) {
        if (book.accessionNo == accessionNo) {
            return &book;
        }
    }
    return nullptr;
}

bool hasActiveLoan(const vector<Borrower>& activeLoans, const string& studentId, const string& accessionNo) {
    return any_of(activeLoans.begin(), activeLoans.end(), [&](const Borrower& loan) {
        return loan.studentId == studentId && loan.borrowedAccessionNo == accessionNo;
    });
}

void removeActiveLoan(vector<Borrower>& activeLoans, const string& studentId, const string& accessionNo) {
    activeLoans.erase(
        remove_if(activeLoans.begin(), activeLoans.end(), [&](const Borrower& loan) {
            return loan.studentId == studentId && loan.borrowedAccessionNo == accessionNo;
        }),
        activeLoans.end());
}

// Scope is local to this function, but the static variable's lifetime lasts
// for the entire program. This directly demonstrates scope versus lifetime.
bool borrowBook(vector<Book>& books,
                vector<Borrower>& activeLoans,
                vector<Transaction>& log,
                const string& studentId,
                const string& studentName,
                const string& accessionNo,
                int day) {
    static int totalLoans = 0;

    SearchResult result = searchBook(books, accessionNo);
    Transaction transaction{
        static_cast<int>(log.size()) + 1,
        TransactionType::Borrow,
        studentId,
        accessionNo,
        day,
        0,
        0.0,
        false,
        ""
    };

    if (!result.book) {
        transaction.message = "Book not found.";
        log.push_back(transaction);
        return false;
    }

    if (result.book->availableCopies <= 0) {
        transaction.message = "Borrow failed: no copy is available.";
        log.push_back(transaction);
        return false;
    }

    if (hasActiveLoan(activeLoans, studentId, accessionNo)) {
        transaction.message = "Borrow failed: borrower already has this book.";
        log.push_back(transaction);
        return false;
    }

    result.book->availableCopies--;
    refreshStatus(*result.book);
    activeLoans.push_back({studentId, studentName, accessionNo});
    ++totalLoans;
    cout << "[Static counter] totalLoans = " << totalLoans << " after successful borrow." << '\n';
    transaction.success = true;
    transaction.message = "Borrow successful.";
    log.push_back(transaction);
    return true;
}

bool returnBook(vector<Book>& books,
                vector<Borrower>& activeLoans,
                vector<Transaction>& log,
                const string& studentId,
                const string& accessionNo,
                int returnDay) {
    SearchResult result = searchBook(books, accessionNo);
    Transaction transaction{
        static_cast<int>(log.size()) + 1,
        TransactionType::Return,
        studentId,
        accessionNo,
        returnDay,
        0,
        0.0,
        false,
        ""
    };

    if (!result.book) {
        transaction.message = "Return failed: book not found.";
        log.push_back(transaction);
        return false;
    }

    auto loanIt = find_if(activeLoans.begin(), activeLoans.end(), [&](const Borrower& loan) {
        return loan.studentId == studentId && loan.borrowedAccessionNo == accessionNo;
    });

    if (loanIt == activeLoans.end()) {
        transaction.message = "Return failed: no active loan exists for this borrower/book.";
        log.push_back(transaction);
        return false;
    }

    // The training loan period is MAX_LOAN_DAYS. For this prototype, the
    // borrowing day is stored in the matching previous transaction.
    int borrowDay = -1;
    for (auto it = log.rbegin(); it != log.rend(); ++it) {
        if (it->type == TransactionType::Borrow && it->studentId == studentId &&
            it->accessionNo == accessionNo && it->success) {
            borrowDay = it->day;
            break;
        }
    }

    if (borrowDay < 0) {
        transaction.message = "Return failed: original successful borrowing record not found.";
        log.push_back(transaction);
        return false;
    }

    const int dueDay = borrowDay + MAX_LOAN_DAYS;
    transaction.overdueDays = max(0, returnDay - dueDay);
    transaction.fine = transaction.overdueDays * CHARGE_PER_DAY;

    result.book->availableCopies++;
    refreshStatus(*result.book);
    removeActiveLoan(activeLoans, studentId, accessionNo);

    transaction.success = true;
    ostringstream message;
    message << "Return successful. Fine = KSh " << fixed << setprecision(2) << transaction.fine;
    transaction.message = message.str();
    log.push_back(transaction);
    return true;
}

string typeToString(TransactionType type) {
    return type == TransactionType::Borrow ? "BORROW" : "RETURN";
}

void printBooks(const vector<Book>& books) {
    cout << "\n=== CURRENT BOOK STOCK ===\n";
    cout << left << setw(10) << "Accession"
         << setw(36) << "Title"
         << setw(10) << "Copies"
         << "Status\n";
    cout << string(72, '-') << '\n';

    for (const auto& book : books) {
        cout << left << setw(10) << book.accessionNo
             << setw(36) << book.title
             << setw(10) << book.availableCopies
             << book.status << '\n';
    }
}

void printTransactionLog(const vector<Transaction>& log) {
    cout << "\n=== TRANSACTION LOG ===\n";
    cout << left << setw(5) << "ID"
         << setw(9) << "Type"
         << setw(12) << "Student"
         << setw(10) << "Book"
         << setw(8) << "Day"
         << setw(10) << "Fine"
         << "Result\n";
    cout << string(95, '-') << '\n';

    for (const auto& t : log) {
        cout << left << setw(5) << t.id
             << setw(9) << typeToString(t.type)
             << setw(12) << t.studentId
             << setw(10) << t.accessionNo
             << setw(8) << t.day
             << "KSh " << setw(5) << fixed << setprecision(0) << t.fine
             << (t.success ? "SUCCESS" : "FAILED") << " - " << t.message << '\n';
    }
}

void printSummary(const vector<Transaction>& log) {
    int successfulBorrow = 0;
    int failedBorrow = 0;
    int successfulReturn = 0;
    int failedReturn = 0;
    double totalFines = 0.0;

    for (const auto& t : log) {
        if (t.type == TransactionType::Borrow) {
            if (t.success) ++successfulBorrow;
            else ++failedBorrow;
        } else {
            if (t.success) ++successfulReturn;
            else ++failedReturn;
            if (t.success) totalFines += t.fine;
        }
    }

    cout << "\n=== SUMMARY ===\n";
    cout << "Successful borrowing attempts: " << successfulBorrow << '\n';
    cout << "Failed borrowing attempts:     " << failedBorrow << '\n';
    cout << "Successful returns:             " << successfulReturn << '\n';
    cout << "Failed returns:                 " << failedReturn << '\n';
    cout << "Total overdue charges:          KSh " << fixed << setprecision(2) << totalFines << '\n';
}

void demonstrateAliasing(vector<Book>& books) {
    cout << "\n=== ALIASING EXPERIMENT ===\n";
    Book* selectedBook = searchBook(books, "LIB005").book; // pointer to a Book object
    if (!selectedBook) {
        cout << "Selected book not found.\n";
        return;
    }

    Book& secondName = *selectedBook; // second name refers to the same object
    cout << "Before modification: pointer view = " << selectedBook->availableCopies
         << ", reference view = " << secondName.availableCopies << '\n';

    selectedBook->availableCopies--;
    refreshStatus(*selectedBook);

    cout << "After modifying through pointer: pointer view = " << selectedBook->availableCopies
         << ", reference view = " << secondName.availableCopies << '\n';
    cout << "Both names observe the same storage location/object.\n";

    // Restore the stock after the experiment so the final table remains correct.
    secondName.availableCopies++;
    refreshStatus(secondName);
}

void demonstrateDynamicLifetime() {
    cout << "\n=== LIFETIME EXPERIMENT ===\n";
    Book* dynamicBook = new Book{"DYN001", "Coroutine Patterns (Training Copy)", 1, "Available"};
    cout << "Dynamically allocated object created: " << dynamicBook->accessionNo << '\n';

    auto inspectDynamicBook = [](const Book& book) {
        int searchIndex = 0; // automatic local; lifetime ends when this call returns
        cout << "Inside helper: object = " << book.accessionNo
             << ", local searchIndex = " << searchIndex << '\n';
    };

    inspectDynamicBook(*dynamicBook);
    inspectDynamicBook(*dynamicBook);
    cout << "The dynamically allocated Book is still alive after both calls.\n";
    delete dynamicBook;
    dynamicBook = nullptr;
    cout << "Dynamic Book released with delete; its lifetime has ended.\n";
}

void conceptDemonstration() {
    cout << "\n=== L-VALUE / R-VALUE EXPERIMENT ===\n";
    double catMark = 18.0;
    double practicalMark = 16.0;
    double examMark = 42.0;

    double totalMark = catMark + practicalMark + examMark;
    cout << "totalMark = catMark + practicalMark + examMark -> " << totalMark << '\n';
    cout << "L-values here: totalMark is the destination storage location; each named variable can also denote a storage location.\n";
    cout << "R-value expression: catMark + practicalMark + examMark produces the value assigned to totalMark.\n";

    examMark = examMark + 5;
    cout << "After examMark = examMark + 5, examMark = " << examMark << '\n';
    cout << "Left examMark is the l-value destination; right examMark is read as a value within the r-value expression.\n";
}

void runMainDemo() {
    vector<Book> books = {
        {"LIB001", "Introduction to C++", 2, ""},
        {"LIB002", "Programming Languages Concepts", 3, ""},
        {"LIB003", "Data Structures", 1, ""},
        {"LIB004", "Operating Systems", 2, ""},
        {"LIB005", "Database Systems", 2, ""},
        {"LIB006", "Computer Networks", 2, ""},
        {"LIB007", "Software Engineering", 1, ""},
        {"LIB008", "Discrete Mathematics", 2, ""}
    };
    for (auto& book : books) refreshStatus(book);

    vector<Borrower> activeLoans;
    vector<Transaction> log;

    cout << "UNIVERSITY LIBRARY BORROWING, FINES AND BOOK-STATE TRACKER\n";
    cout << "Training constants: MAX_LOAN_DAYS = " << MAX_LOAN_DAYS
         << ", CHARGE_PER_DAY = KSh " << CHARGE_PER_DAY << "\n";

    // More than the required five borrow/return transactions are demonstrated.
    borrowBook(books, activeLoans, log, "ST001", "Alice", "LIB001", 1);
    borrowBook(books, activeLoans, log, "ST002", "Brian", "LIB001", 1);
    borrowBook(books, activeLoans, log, "ST003", "Carol", "LIB001", 2); // no copy available
    returnBook(books, activeLoans, log, "ST001", "LIB001", 18); // 3 days overdue
    borrowBook(books, activeLoans, log, "ST003", "Carol", "LIB001", 19);
    borrowBook(books, activeLoans, log, "ST004", "Daniel", "LIB003", 3);
    returnBook(books, activeLoans, log, "ST004", "LIB003", 19); // 2 days overdue
    returnBook(books, activeLoans, log, "ST009", "LIB003", 19); // invalid return
    borrowBook(books, activeLoans, log, "ST003", "Carol", "LIB009", 20); // invalid accession

    printTransactionLog(log);
    printBooks(books);
    printSummary(log);
    demonstrateAliasing(books);
    demonstrateDynamicLifetime();
    conceptDemonstration();

    cout << "\n=== SCOPE / LIFETIME NOTES ===\n";
    cout << "searchIndex: local to searchBook(), automatic lifetime; it is not visible after searchBook() returns.\n";
    cout << "totalLoans: local name inside borrowBook(), but static lifetime lasts until program termination.\n";
    cout << "dynamicBook's Book object: explicit heap-dynamic lifetime until delete, independent of helper-call scope.\n";
    cout << "The pointer and reference in the aliasing experiment are two names referring to the same Book object.\n";
}

bool assertTest(bool condition, const string& description) {
    cout << (condition ? "PASS: " : "FAIL: ") << description << '\n';
    return condition;
}

void runAutomatedTests() {
    cout << "GROUP 6 AUTOMATED TESTS\n";
    bool allPassed = true;

    // Successful test 1: borrow an available book.
    {
        vector<Book> books{{"T001", "Test Book", 1, "Available"}};
        vector<Borrower> loans;
        vector<Transaction> log;
        bool ok = borrowBook(books, loans, log, "S001", "Student One", "T001", 1);
        allPassed &= assertTest(ok && books[0].availableCopies == 0, "Successful borrowing reduces available copies to zero.");
    }

    // Successful test 2: return on time.
    {
        vector<Book> books{{"T002", "Return Book", 1, "Available"}};
        vector<Borrower> loans;
        vector<Transaction> log;
        borrowBook(books, loans, log, "S002", "Student Two", "T002", 1);
        bool ok = returnBook(books, loans, log, "S002", "T002", 15);
        allPassed &= assertTest(ok && log.back().fine == 0.0 && books[0].availableCopies == 1,
                                "On-time return produces zero fine and restores stock.");
    }

    // Successful test 3: overdue fine follows the named charge constant.
    {
        vector<Book> books{{"T003", "Fine Book", 1, "Available"}};
        vector<Borrower> loans;
        vector<Transaction> log;
        borrowBook(books, loans, log, "S003", "Student Three", "T003", 1);
        returnBook(books, loans, log, "S003", "T003", 16); // 1 day overdue
        allPassed &= assertTest(log.back().success && log.back().fine == CHARGE_PER_DAY,
                                "One overdue day charges exactly CHARGE_PER_DAY.");
    }

    // Boundary/error test 1: borrowing with no stock.
    {
        vector<Book> books{{"T004", "Empty Book", 0, "No copies"}};
        vector<Borrower> loans;
        vector<Transaction> log;
        bool ok = borrowBook(books, loans, log, "S004", "Student Four", "T004", 1);
        allPassed &= assertTest(!ok && !log.back().success, "Boundary/error: borrowing with zero copies is rejected.");
    }

    // Boundary/error test 2: invalid return without an active loan.
    {
        vector<Book> books{{"T005", "Invalid Return", 1, "Available"}};
        vector<Borrower> loans;
        vector<Transaction> log;
        bool ok = returnBook(books, loans, log, "S999", "T005", 20);
        allPassed &= assertTest(!ok && !log.back().success, "Boundary/error: return without an active loan is rejected.");
    }

    cout << (allPassed ? "ALL AUTOMATED TESTS PASSED.\n" : "ONE OR MORE TESTS FAILED.\n");
}

int main(int argc, char* argv[]) {
    if (argc > 1 && string(argv[1]) == "--test") {
        runAutomatedTests();
        return 0;
    }

    runMainDemo();
    return 0;
}
