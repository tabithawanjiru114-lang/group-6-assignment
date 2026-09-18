# Group 6 — University Library Borrowing, Fines and Book-State Tracker

## Course Task

Programming Languages Laboratory — **Names, Bindings and Scopes**, Group 6.

The laboratory case asks for a university library prototype that tracks books, borrowers, due days and a training overdue charge while demonstrating aliases, pointers/references, explicit heap-dynamic storage, lifetime, and scope. The implementation follows those requirements directly. The task specifies named constants `MAX_LOAN_DAYS` and `CHARGE_PER_DAY`, at least 8 books and at least 5 borrowing/return transactions.

## Language

- C++17
- Standard library only; no external packages

## Repository Structure

```text
Group6_Name_Bindings_Scopes/
├── src/
│   └── library_tracker.cpp
├── docs/
│   ├── CONCEPT_NOTE.md
│   └── TESTING.md
├── .gitignore
└── README.md
```

## How to Compile and Run

### Linux / macOS / Git Bash

```bash
g++ -std=c++17 -Wall -Wextra -pedantic src/library_tracker.cpp -o library_tracker
./library_tracker
```

### Windows MinGW

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic src/library_tracker.cpp -o library_tracker.exe
./library_tracker.exe
```

Run the automated tests:

```text
./library_tracker --test
```

On Windows:

```text
./library_tracker.exe --test
```

## What the Program Demonstrates

### 1. Borrow/return workflow

The program includes:

- 8 books
- borrower records
- book searching
- successful borrowing
- failed borrowing when no copy is available
- successful returns
- failed returns for invalid loans
- overdue-day calculation
- training fines using the named constant `CHARGE_PER_DAY`

### 2. Aliasing experiment

The program obtains a `Book*` pointer and then creates a reference:

```cpp
Book* selectedBook = searchBook(books, "LIB005").book;
Book& secondName = *selectedBook;
```

Both names refer to the same `Book` object. When `availableCopies` is changed through `selectedBook`, the same changed value is observed through `secondName`.

### 3. Explicit heap-dynamic storage

The lifetime experiment creates a book with:

```cpp
Book* dynamicBook = new Book{...};
```

The object remains alive across multiple helper-function calls and is released explicitly with `delete`.

### 4. Scope versus lifetime

`searchIndex` is a normal local variable inside `searchBook()`. Its name is visible only in that function and its automatic lifetime ends when the call returns.

`totalLoans` is a `static` local inside `borrowBook()`. Its **scope is local to the function**, but its **lifetime extends until program termination**.

The dynamically allocated `Book` object can remain alive even when a particular local pointer or helper-function name is no longer in scope.

### 5. L-value / R-value demonstration

The program explicitly demonstrates:

```cpp
double totalMark = catMark + practicalMark + examMark;
examMark = examMark + 5;
```

The README/docs explain which side provides a storage location and which side provides a value.

## Named Constants

| Constant | Value | Purpose |
|---|---:|---|
| `MAX_LOAN_DAYS` | 14 | Training maximum loan period |
| `CHARGE_PER_DAY` | KSh 20 | Training overdue charge per day |

These are fictional laboratory values and are not presented as current institutional policy.

## Testing Evidence

The repository includes `docs/TESTING.md` with the automated test transcript and the normal demonstration run.

The automated suite contains **three successful functional tests and two boundary/error tests**, matching the general laboratory testing expectation.

## Viva Preparation

The main concepts to be ready to explain are:

1. Why `selectedBook` and `secondName` are aliases for one object.
2. Why scope and lifetime are not the same thing.
3. Why `totalLoans` has function-local scope but program-long lifetime.
4. Why explicit heap allocation requires correct deallocation.
5. How `examMark` acts as a storage location on the left of an assignment and contributes a value on the right.

See `docs/CONCEPT_NOTE.md` for a line-linked explanation of the concepts used in the code.
