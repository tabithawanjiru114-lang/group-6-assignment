# Group 6 Testing Evidence

## Test command

```text
./library_tracker --test
```

## Automated test cases

The test runner covers five cases:

| Test | Category | Expected result |
|---|---|---|
| 1 | Successful | Borrow an available book and reduce copies to 0 |
| 2 | Successful | Return on time; fine is KSh 0 and stock is restored |
| 3 | Successful | One overdue day produces `CHARGE_PER_DAY` |
| 4 | Boundary / Error | Borrowing a book with zero available copies is rejected |
| 5 | Boundary / Error | Returning a book with no active loan is rejected |

## Actual test transcript

```text
GROUP 6 AUTOMATED TESTS
[Static counter] totalLoans = 1 after successful borrow.
PASS: Successful borrowing reduces available copies to zero.
[Static counter] totalLoans = 2 after successful borrow.
PASS: On-time return produces zero fine and restores stock.
[Static counter] totalLoans = 3 after successful borrow.
PASS: One overdue day charges exactly CHARGE_PER_DAY.
PASS: Boundary/error: borrowing with zero copies is rejected.
PASS: Boundary/error: return without an active loan is rejected.
ALL AUTOMATED TESTS PASSED.
```

## Normal demonstration run

The normal run also exercises:

- 8-book dataset
- more than 5 borrowing/return transactions
- an unavailable-copy rejection
- an invalid return
- an invalid accession number
- an overdue return and fine calculation
- aliasing through pointer/reference
- explicit heap allocation and release
- scope/lifetime notes
- l-value/r-value example

## Expected important observations

- An unavailable book cannot be borrowed.
- A valid return increases available copies.
- Overdue days are `max(0, returnDay - (borrowDay + MAX_LOAN_DAYS))`.
- Fine is `overdueDays * CHARGE_PER_DAY`.
- The aliasing experiment shows the same copy count through both names.
- The dynamic object survives multiple helper calls before `delete` ends its lifetime.
