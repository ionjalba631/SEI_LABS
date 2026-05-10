#ifndef TRAFFIC_FSM_H
#define TRAFFIC_FSM_H

#include <Arduino.h>

typedef enum {
    TRAFFIC_LAMP_RED = 0,
    TRAFFIC_LAMP_YELLOW = 1,
    TRAFFIC_LAMP_GREEN = 2
} TrafficLampState;

typedef enum {
    TRAFFIC_PHASE_EAST_GREEN = 0,
    TRAFFIC_PHASE_EAST_YELLOW = 1,
    TRAFFIC_PHASE_ALL_RED_TO_NORTH = 2,
    TRAFFIC_PHASE_NORTH_GREEN = 3,
    TRAFFIC_PHASE_NORTH_YELLOW = 4,
    TRAFFIC_PHASE_ALL_RED_TO_EAST = 5
} TrafficPhase;

typedef struct {
    bool red;
    bool yellow;
    bool green;
    TrafficLampState lamp;
} DirectionLights;

typedef struct {
    TrafficPhase phase;
    uint32_t phase_elapsed_ms;
    uint32_t phase_duration_ms;
} TrafficFsmContext;

typedef struct {
    TrafficPhase phase;
    DirectionLights east;
    DirectionLights north;
    bool north_request_pending;
    uint32_t phase_elapsed_ms;
    uint32_t phase_duration_ms;
} TrafficSnapshot;

void traffic_fsm_init(TrafficFsmContext *context);
void traffic_fsm_tick(
    TrafficFsmContext *context,
    bool north_request_pending,
    uint32_t elapsed_ms
);
bool traffic_fsm_should_clear_request(const TrafficFsmContext *context);
TrafficSnapshot traffic_fsm_get_snapshot(
    const TrafficFsmContext *context,
    bool north_request_pending
);

#endif
