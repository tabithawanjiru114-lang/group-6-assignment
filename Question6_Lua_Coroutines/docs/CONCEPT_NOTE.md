# Question 6 Concept Note — Adaptive Nairobi Traffic Control

## Requirement-to-code mapping

| Question 6 requirement | Implementation |
|---|---|
| Separate queue information for every road | `sensorQueues` table plus one independent coroutine state per road |
| Road coroutine yields current traffic state | `coroutine.yield({... phase = "READY" ...})` |
| Scheduling uses congestion and fairness | `schedulingScore()` combines `queue` and `waitCycles` |
| Prevent starvation | `STARVATION_LIMIT` with forced selection |
| Discuss fairness/throughput | README explanation and final scheduler output |

The assignment specifically asks for those five elements.

## Coroutine lifecycle

1. `coroutine.create()` creates one coroutine per road.
2. The scheduler starts each coroutine with `coroutine.resume()`.
3. The road yields its current state with `coroutine.yield()`.
4. Each scheduling round sends sensor data using `coroutine.resume()`.
5. The scheduler selects one road and resumes it with `GREEN`.
6. The selected road yields a post-green state.
7. After the simulation horizon, suspended coroutines receive `STOP` and return.
8. `coroutine.status()` is checked before resuming any worker.

## State kept by each road coroutine

Inside `roadCoroutine()`:

```lua
local queue
local waitCycles
local greenPeriods
local vehiclesServed
local sensorRound
```

These are separate state variables for each coroutine invocation.

## Scheduling logic

The main score is:

```text
score = congestion contribution + fairness contribution
      = queue * 1.0 + waitCycles * 8.0
```

The fairness mechanism is deliberately stronger than score alone: once `waitCycles >= 3`, forced service occurs before normal score comparison.

## Why starvation is prevented

Suppose Thika Road continuously has a queue of 50 while Ngong Road has a queue of 8. If the scheduler selected only by queue length, Ngong Road could theoretically wait indefinitely. In this design, Ngong Road's wait count increases whenever another road is selected. Once the threshold of 3 is reached, Ngong Road becomes a forced candidate.

## Fairness / throughput trade-off

A purely congestion-driven scheduler spends more service capacity on larger queues, potentially improving short-term throughput. A fairness-aware scheduler deliberately sacrifices some of that capacity to serve roads that have waited longer. The chosen design balances both by using congestion as the normal score and a hard anti-starvation rule.

## Cooperative execution

The scheduler is the controller. Road coroutines do not execute simultaneously. A road executes until it yields, then the scheduler may resume another road. Therefore the program demonstrates quasi-concurrent/cooperative execution through interleaving.

A real multithreaded or process-based system would be appropriate when independent sensor processing or actuator control must happen truly simultaneously and reliably under real-time constraints.
