# Question 6 Testing Evidence

## Test command

```text
lua src/traffic_control.lua --test
```

## Test cases

| Test | Expected result |
|---|---|
| 1 | Highest congestion receives the next green period when no road is starved |
| 2 | Road at the starvation threshold is forced ahead of a busier road |
| 3 | Low queues are handled correctly without discharging more than the queue |
| 4 | Coroutine becomes `dead` after its function returns |
| 5 | Attempting to resume a dead coroutine is rejected by `safeResume()` |

## Actual test transcript

```text
ADAPTIVE TRAFFIC CONTROL TESTS
PASS: High congestion receives the next green period when no road is starved.
PASS: A road at the starvation threshold is force-selected despite a busier competing road.
PASS: Scheduling logic works with queues below the green discharge capacity.
PASS: Coroutine transitions to dead only after its function returns.
PASS: Dead coroutine is not resumed again.
5/5 tests passed.
```

## Normal demonstration run

The normal command is:

```text
lua src/traffic_control.lua
```

It prints an 8-round cooperative scheduling trace, including:

- selected road
- scheduling score
- whether selection came from normal congestion/fairness scoring or forced fairness
- queue after green
- vehicles discharged
- waiting count
- final green-period counts
- final coroutine statuses

## What the output should demonstrate

Busy roads such as Thika Road and Waiyaki Way should receive repeated service when their queue advantage is strong, while the starvation threshold prevents lower-volume roads from being ignored indefinitely.
