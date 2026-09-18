# Group 6 Concept Note

## 1. Variable / object concept map

| Name / object | Type | Scope | Lifetime | Role in the program |
|---|---|---|---|---|
| `MAX_LOAN_DAYS` | `constexpr int` | File scope | Entire program | Named constant for loan period |
| `CHARGE_PER_DAY` | `constexpr double` | File scope | Entire program | Named training fine |
| `searchIndex` | `int` | `searchBook()` function | One call to `searchBook()` | Tracks the matched book index |
| `totalLoans` | `static int` | `borrowBook()` function | From first initialization to program termination | Demonstrates local scope + extended lifetime |
| `selectedBook` | `Book*` | `demonstrateAliasing()` function | One call to the function | Pointer name referring to selected book |
| `secondName` | `Book&` | `demonstrateAliasing()` function | One call to the function | Second name (reference) for same book object |
| dynamically allocated `Book` | `Book` | Object is accessed through pointer/reference names | `new` until `delete` | Demonstrates explicit heap-dynamic storage |

## 2. Aliasing

The aliasing experiment creates a pointer and then a reference to the same selected `Book` object:

```cpp
Book* selectedBook = searchBook(books, "LIB005").book;
Book& secondName = *selectedBook;
```

A change made through `selectedBook` is immediately visible through `secondName`. Therefore the two names are aliases for the same object/storage.

## 3. Scope versus lifetime

`searchIndex` is declared inside `searchBook()`:

```cpp
int searchIndex = -1;
```

Its **scope** is limited to the `searchBook()` function body and its normal automatic **lifetime** ends when that invocation returns.

`totalLoans` is declared as:

```cpp
static int totalLoans = 0;
```

Its name is still only usable inside `borrowBook()`, but its lifetime extends until program termination. A later call sees the same preserved object/value.

The dynamically allocated `Book` is different: its storage remains allocated after a helper function returns, until the owning pointer is used with `delete`.

## 4. Explicit heap-dynamic storage

The code uses:

```cpp
Book* dynamicBook = new Book{...};
```

The object is then used in two helper calls. After those calls, the object is still alive. It is released using:

```cpp
delete dynamicBook;
dynamicBook = nullptr;
```

This demonstrates that object lifetime can extend beyond the lifetime/scope of individual local helper variables.

## 5. L-values and R-values

For:

```cpp
double totalMark = catMark + practicalMark + examMark;
```

`totalMark` is the destination storage location (l-value role), while `catMark + practicalMark + examMark` is an expression that produces a value (r-value role).

For:

```cpp
examMark = examMark + 5;
```

the left `examMark` identifies the destination storage location; the right `examMark` is read for its value and participates in the resulting r-value expression.

## 6. Binding observations

- The declared types of fields such as `availableCopies` and `status` are fixed by their C++ declarations.
- The names are associated with storage when the relevant objects are created.
- Values such as `availableCopies` change during execution after borrow and return operations.
- The named constants are intentionally not reassigned.

## 7. Answers to the key reflection questions

### If two variables have the same name in different scopes, must they share a memory location?

No. In general, scope determines which declaration a name refers to. Two declarations with the same spelling can denote different objects and therefore different storage locations. The program deliberately keeps local names such as `searchIndex` separate from other variables.

### If two different names share one location, what relationship exists?

They are aliases. The pointer/reference experiment demonstrates this: `selectedBook` and `secondName` both access the same `Book` object, so changing the object through one name is observed through the other.
