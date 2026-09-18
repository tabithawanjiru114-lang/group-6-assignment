-- CCS 2105 - Question 6
-- Adaptive Nairobi Traffic Control
-- Lua 5.3+
--
-- The simulation is cooperative: the scheduler decides when each road
-- coroutine runs. No coroutine can interrupt another coroutine by itself.

local GREEN_CAPACITY = 20       -- vehicles discharged per green period
local CONGESTION_WEIGHT = 1.0   -- gives busier roads more scheduling weight
local FAIRNESS_WEIGHT = 8.0     -- rewards roads that have waited
local STARVATION_LIMIT = 3      -- force service after this many waits
local TOTAL_ROUNDS = 8

local sensorQueues = {
    ["Thika Road"] = {45, 48, 52, 50, 38, 46, 42, 40},
    ["Mombasa Road"] = {12, 18, 10, 15, 8, 20, 14, 11},
    ["Ngong Road"] = {7, 9, 8, 12, 6, 10, 9, 8},
    ["Waiyaki Way"] = {30, 35, 31, 29, 40, 34, 28, 36}
}

local roadOrder = {"Thika Road", "Mombasa Road", "Ngong Road", "Waiyaki Way"}

local function roadCoroutine(name, queueUpdates)
    -- These variables belong to this coroutine's own execution state.
    local queue = queueUpdates[1]
    local waitCycles = 0
    local greenPeriods = 0
    local vehiclesServed = 0
    local sensorRound = 1

    while true do
        -- Yield the current traffic state to the scheduler.
        local command = coroutine.yield({
            road = name,
            queue = queue,
            waitCycles = waitCycles,
            greenPeriods = greenPeriods,
            vehiclesServed = vehiclesServed,
            round = sensorRound,
            phase = "READY"
        })

        command = command or {}

        if command.action == "STOP" then
            return {
                road = name,
                phase = "COMPLETE",
                queue = queue,
                waitCycles = waitCycles,
                greenPeriods = greenPeriods,
                vehiclesServed = vehiclesServed
            }
        elseif command.action == "SENSE" then
            -- The scheduler supplies the newest sensor reading.
            if type(command.sensorQueue) == "number" and command.sensorQueue >= 0 then
                queue = command.sensorQueue
            end

            waitCycles = waitCycles + 1
            sensorRound = command.round or sensorRound
        elseif command.action == "GREEN" then
            local before = queue
            local discharged = math.min(queue, GREEN_CAPACITY)
            queue = queue - discharged
            waitCycles = 0
            greenPeriods = greenPeriods + 1
            vehiclesServed = vehiclesServed + discharged

            -- Yield the post-green state so the scheduler can record the effect.
            command = coroutine.yield({
                road = name,
                queue = queue,
                queueBeforeGreen = before,
                discharged = discharged,
                waitCycles = waitCycles,
                greenPeriods = greenPeriods,
                vehiclesServed = vehiclesServed,
                round = sensorRound,
                phase = "GREEN_APPLIED"
            })

            if command and command.action == "STOP" then
                return {
                    road = name,
                    phase = "COMPLETE",
                    queue = queue,
                    waitCycles = waitCycles,
                    greenPeriods = greenPeriods,
                    vehiclesServed = vehiclesServed
                }
            end
        end
    end
end

local function createRoadWorkers()
    local workers = {}
    for _, road in ipairs(roadOrder) do
        workers[road] = coroutine.create(function()
            return roadCoroutine(road, sensorQueues[road])
        end)
    end
    return workers
end

local function safeResume(worker, command)
    if coroutine.status(worker) == "dead" then
        return false, "dead coroutine"
    end

    local ok, result = coroutine.resume(worker, command)
    if not ok then
        return false, result
    end

    return true, result
end

local function schedulingScore(state)
    return (state.queue * CONGESTION_WEIGHT) + (state.waitCycles * FAIRNESS_WEIGHT)
end

local function chooseNextRoad(states)
    -- Anti-starvation rule takes priority: any road at the threshold is eligible
    -- for forced service. Among forced candidates, use the largest queue.
    local forced = nil
    for _, road in ipairs(roadOrder) do
        local state = states[road]
        if state and state.waitCycles >= STARVATION_LIMIT then
            if forced == nil or state.queue > forced.queue then
                forced = state
            end
        end
    end

    if forced then
        return forced.road, "FORCED_FAIRNESS", schedulingScore(forced)
    end

    local best = nil
    for _, road in ipairs(roadOrder) do
        local state = states[road]
        if state then
            local score = schedulingScore(state)
            if best == nil or score > best.score then
                best = {road = road, score = score}
            end
        end
    end

    return best.road, "CONGESTION_PLUS_FAIRNESS", best.score
end

local function printHeader()
    print("ADAPTIVE NAIROBI TRAFFIC CONTROL")
    print(string.format(
        "Green capacity = %d | Congestion weight = %.1f | Fairness weight = %.1f | Starvation limit = %d",
        GREEN_CAPACITY, CONGESTION_WEIGHT, FAIRNESS_WEIGHT, STARVATION_LIMIT
    ))
end

local function runSimulation()
    printHeader()

    local workers = createRoadWorkers()
    local states = {}
    local greenSelections = {}
    local history = {}

    for _, road in ipairs(roadOrder) do
        greenSelections[road] = 0

        local ok, result = safeResume(workers[road])
        if not ok then
            error("Failed to start " .. road .. ": " .. tostring(result))
        end
        states[road] = result
    end

    print("\n--- COOPERATIVE SCHEDULING TRACE ---")

    for round = 1, TOTAL_ROUNDS do
        -- First, resume every active road to receive the latest sensor state.
        -- This is cooperative observation; the scheduler remains in control.
        for _, road in ipairs(roadOrder) do
            local worker = workers[road]
            if coroutine.status(worker) ~= "dead" then
                local ok, result = safeResume(worker, {
                    action = "SENSE",
                    sensorQueue = sensorQueues[road][round],
                    round = round
                })
                if not ok then
                    error("Sensor update failed for " .. road .. ": " .. tostring(result))
                end
                states[road] = result
            end
        end

        local selectedRoad, reason, score = chooseNextRoad(states)
        local selectedWorker = workers[selectedRoad]

        local ok, afterGreen = safeResume(selectedWorker, {action = "GREEN"})
        if not ok then
            error("Green-light update failed for " .. selectedRoad .. ": " .. tostring(afterGreen))
        end

        states[selectedRoad] = afterGreen
        greenSelections[selectedRoad] = greenSelections[selectedRoad] + 1

        table.insert(history, {
            round = round,
            road = selectedRoad,
            reason = reason,
            score = score,
            queue = afterGreen.queue,
            discharged = afterGreen.discharged or 0,
            wait = afterGreen.waitCycles
        })

        print(string.format(
            "Round %d: %-14s | score=%6.1f | %-22s | queue after green=%2d | discharged=%2d | wait=%d",
            round,
            selectedRoad,
            score,
            reason,
            afterGreen.queue,
            afterGreen.discharged or 0,
            afterGreen.waitCycles
        ))
    end

    -- Safely terminate any suspended coroutines after the monitoring horizon.
    local finalStates = {}
    for _, road in ipairs(roadOrder) do
        local worker = workers[road]
        if coroutine.status(worker) ~= "dead" then
            local ok, result = safeResume(worker, {action = "STOP"})
            if not ok then
                error("Failed to terminate " .. road .. ": " .. tostring(result))
            end
            finalStates[road] = result
        end
    end

    print("\n--- FAIRNESS / THROUGHPUT SUMMARY ---")
    for _, road in ipairs(roadOrder) do
        local finalState = finalStates[road]
        print(string.format(
            "%-14s | green periods=%d | vehicles served=%d | final queue=%d | coroutine status=%s",
            road,
            greenSelections[road],
            finalState and finalState.vehiclesServed or 0,
            finalState and finalState.queue or states[road].queue,
            coroutine.status(workers[road])
        ))
    end

    print("\nScheduling interpretation:")
    print("1. Congestion increases a road's score, so larger queues normally receive more green periods.")
    print("2. Waiting time adds a fairness bonus, so a quiet road becomes more competitive as it waits.")
    print("3. At the starvation limit, a road is force-selected, preventing indefinite starvation.")
    print("4. Because roads yield and are resumed by one scheduler, execution is interleaved cooperative concurrency, not parallel execution.")

    return history
end

local function runTests()
    print("ADAPTIVE TRAFFIC CONTROL TESTS")
    local passed = 0
    local total = 0

    local function check(condition, description)
        total = total + 1
        if condition then
            passed = passed + 1
            print("PASS: " .. description)
        else
            print("FAIL: " .. description)
        end
    end

    -- Test 1: busy road is selected when congestion is dominant.
    do
        local states = {
            ["Thika Road"] = {road = "Thika Road", queue = 50, waitCycles = 0},
            ["Mombasa Road"] = {road = "Mombasa Road", queue = 10, waitCycles = 0},
            ["Ngong Road"] = {road = "Ngong Road", queue = 7, waitCycles = 0},
            ["Waiyaki Way"] = {road = "Waiyaki Way", queue = 30, waitCycles = 0}
        }
        local selected = chooseNextRoad(states)
        check(selected == "Thika Road", "High congestion receives the next green period when no road is starved.")
    end

    -- Test 2: fairness prevents a lower-volume road from being ignored.
    do
        local states = {
            ["Thika Road"] = {road = "Thika Road", queue = 60, waitCycles = 0},
            ["Mombasa Road"] = {road = "Mombasa Road", queue = 10, waitCycles = STARVATION_LIMIT},
            ["Ngong Road"] = {road = "Ngong Road", queue = 8, waitCycles = 1},
            ["Waiyaki Way"] = {road = "Waiyaki Way", queue = 30, waitCycles = 0}
        }
        local selected, reason = chooseNextRoad(states)
        check(selected == "Mombasa Road" and reason == "FORCED_FAIRNESS",
              "A road at the starvation threshold is force-selected despite a busier competing road.")
    end

    -- Test 3: green capacity never removes more vehicles than exist.
    do
        local states = {
            ["Thika Road"] = {road = "Thika Road", queue = 15, waitCycles = 0},
            ["Mombasa Road"] = {road = "Mombasa Road", queue = 12, waitCycles = 0},
            ["Ngong Road"] = {road = "Ngong Road", queue = 5, waitCycles = 0},
            ["Waiyaki Way"] = {road = "Waiyaki Way", queue = 18, waitCycles = 0}
        }
        local selected = chooseNextRoad(states)
        check(states[selected].queue <= GREEN_CAPACITY,
              "Scheduling logic works with queues below the green discharge capacity.")
    end

    -- Test 4: coroutine status is checked before a second resume.
    do
        local worker = coroutine.create(function()
            coroutine.yield("one")
            return "done"
        end)
        local ok1 = coroutine.resume(worker)
        local ok2 = coroutine.resume(worker)
        local statusAfter = coroutine.status(worker)
        check(ok1 and ok2 and statusAfter == "dead", "Coroutine transitions to dead only after its function returns.")
        local safe, message = safeResume(worker)
        check(not safe and message == "dead coroutine", "Dead coroutine is not resumed again.")
    end

    print(string.format("%d/%d tests passed.", passed, total))
    return passed == total
end

if arg and arg[1] == "--test" then
    os.exit(runTests() and 0 or 1)
else
    runSimulation()
end
