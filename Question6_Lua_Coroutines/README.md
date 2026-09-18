# Question 6 — Adaptive Nairobi Traffic Control

## Course Task

CCS 2105 — Problem-Solving Exercises on Lua Coroutines, **Question 6**.

The question requires four fictional roads — Thika Road, Mombasa Road, Ngong Road and Waiyaki Way — to maintain separate queue information, yield traffic state from road coroutines, schedule using congestion and fairness, prevent starvation, and discuss the fairness/throughput trade-off.

## Language

- Lua 5.3+
- Standard Lua library only

## Repository Structure

```text
Question6_Lua_Coroutines/
├── src/
│   └── traffic_control.lua
├── docs/
│   ├── CONCEPT_NOTE.md
│   └── TESTING.md
├── .gitignore
└── README.md
```

## How to Run

From the repository root:

```bash
lua src/traffic_control.lua
```

Run the automated tests:

```bash
lua src/traffic_control.lua --test
```

For systems using `lua5.4` explicitly:

```bash
lua5.4 src/traffic_control.lua
lua5.4 src/traffic_control.lua --test
```

## Solution Design

### 1. Independent road state

Each road has its own coroutine created with `coroutine.create()`.

Inside each road coroutine, queue size, waiting cycles, green-period count and vehicles-served count are local variables. Therefore each road keeps its own execution state.

The scheduler supplies the latest fictional sensor queue through `coroutine.resume()` using a command such as:

```lua
{
    action = "SENSE",
    sensorQueue = sensorQueues[road][round],
    round = round
}
```

### 2. Yielding traffic state

Each road calls `coroutine.yield()` to return its current traffic state to the scheduler. A selected road then receives a `GREEN` command and yields a post-green state.

This demonstrates the required coroutine mechanism: `create()`, `resume()`, `yield()`, and `status()` are all used. The general exercise instructions also require checking resume results and avoiding resumption of dead coroutines.

### 3. Scheduling score

The scheduler uses:

```text
score = queue × CONGESTION_WEIGHT
      + waitCycles × FAIRNESS_WEIGHT
```

With the values in the program:

```text
CONGESTION_WEIGHT = 1.0
FAIRNESS_WEIGHT   = 8.0
STARVATION_LIMIT  = 3
```

A larger queue increases priority, while waiting adds a fairness bonus.

### 4. Starvation prevention

Before normal scoring, the scheduler checks whether any road has reached `STARVATION_LIMIT`. If one has, it is force-selected; among multiple forced roads, the one with the largest queue is chosen.

This means a low-volume road cannot be ignored indefinitely simply because another road continuously has a larger queue.

### 5. Green-light service

The simulation uses a fictional `GREEN_CAPACITY = 20` vehicles per green period. The selected coroutine reduces its local queue by:

```lua
local discharged = math.min(queue, GREEN_CAPACITY)
queue = queue - discharged
```

The value is fictional and only used to make the scheduling trade-off visible.

## Fairness versus Throughput

Giving busy roads more green periods can reduce the largest queues faster, which can improve throughput under heavy demand. However, always selecting the largest queue could make a smaller queue wait indefinitely.

This design adds waiting-time priority and a forced-service threshold. The trade-off is that occasionally serving a quieter road consumes a green period that could have been given to a heavily congested road. The benefit is a bound on waiting and better fairness.

This is a simulation of cooperative scheduling, not a real traffic-control recommendation.

## Coroutine Safety

The helper function `safeResume()` checks:

```lua
coroutine.status(worker)
```

before attempting a resume. It also checks the first return value from `coroutine.resume()` and reports errors rather than silently continuing after a coroutine failure.

At the end of the monitoring horizon, the scheduler sends `STOP` only to coroutines that are not already dead.

## Testing

`docs/TESTING.md` contains the test cases and the actual test transcript.

The test suite checks congestion selection, starvation prevention, discharge-capacity behaviour, coroutine state transition, and safe handling of a dead coroutine.

## Viva Questions

### Why is this cooperative rather than pre-emptive?

A coroutine runs until it executes `coroutine.yield()` or finishes. The scheduler chooses when to call `coroutine.resume()`. There is no timer or operating-system pre-emption in this program.

### Why is coroutine state preserved?

`yield()` suspends execution instead of destroying the function's execution context. Local variables such as `queue`, `waitCycles` and `greenPeriods` remain associated with that coroutine and are available when it resumes.

### How does starvation prevention work?

Every round increases `waitCycles` for roads that are not selected. When a road reaches `STARVATION_LIMIT`, the scheduler gives it forced priority.

### What is the main limitation?

The roads are interleaved cooperatively rather than executing in true parallelism. A long-running coroutine that never yields can block the scheduler and therefore delay every other road.
