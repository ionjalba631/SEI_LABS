#include "traffic_fsm.h"

namespace {

constexpr uint32_t EAST_MIN_GREEN_MS = 5000UL;
constexpr uint32_t NORTH_GREEN_MS = 4000UL;
constexpr uint32_t YELLOW_MS = 2000UL;
constexpr uint32_t ALL_RED_MS = 1000UL;

DirectionLights build_direction(const TrafficLampState lamp) {
    DirectionLights lights = {};
    lights.lamp = lamp;
    lights.red = lamp == TRAFFIC_LAMP_RED;
    lights.yellow = lamp == TRAFFIC_LAMP_YELLOW;
    lights.green = lamp == TRAFFIC_LAMP_GREEN;
    return lights;
}

uint32_t phase_duration_for(const TrafficPhase phase) {
    switch (phase) {
        case TRAFFIC_PHASE_EAST_GREEN:
            return EAST_MIN_GREEN_MS;
        case TRAFFIC_PHASE_EAST_YELLOW:
            return YELLOW_MS;
        case TRAFFIC_PHASE_ALL_RED_TO_NORTH:
            return ALL_RED_MS;
        case TRAFFIC_PHASE_NORTH_GREEN:
            return NORTH_GREEN_MS;
        case TRAFFIC_PHASE_NORTH_YELLOW:
            return YELLOW_MS;
        case TRAFFIC_PHASE_ALL_RED_TO_EAST:
            return ALL_RED_MS;
        default:
            return 0;
    }
}

void advance_phase(TrafficFsmContext *context, const TrafficPhase next_phase) {
    context->phase = next_phase;
    context->phase_elapsed_ms = 0;
    context->phase_duration_ms = phase_duration_for(next_phase);
}

}  // namespace

void traffic_fsm_init(TrafficFsmContext *context) {
    if (context == nullptr) {
        return;
    }

    context->phase = TRAFFIC_PHASE_EAST_GREEN;
    context->phase_elapsed_ms = 0;
    context->phase_duration_ms = phase_duration_for(context->phase);
}

void traffic_fsm_tick(
    TrafficFsmContext *context,
    const bool north_request_pending,
    const uint32_t elapsed_ms
) {
    if (context == nullptr) {
        return;
    }

    context->phase_elapsed_ms += elapsed_ms;

    switch (context->phase) {
        case TRAFFIC_PHASE_EAST_GREEN:
            if (north_request_pending &&
                context->phase_elapsed_ms >= phase_duration_for(TRAFFIC_PHASE_EAST_GREEN)) {
                advance_phase(context, TRAFFIC_PHASE_EAST_YELLOW);
            }
            break;
        case TRAFFIC_PHASE_EAST_YELLOW:
            if (context->phase_elapsed_ms >= phase_duration_for(TRAFFIC_PHASE_EAST_YELLOW)) {
                advance_phase(context, TRAFFIC_PHASE_ALL_RED_TO_NORTH);
            }
            break;
        case TRAFFIC_PHASE_ALL_RED_TO_NORTH:
            if (context->phase_elapsed_ms >= phase_duration_for(TRAFFIC_PHASE_ALL_RED_TO_NORTH)) {
                advance_phase(context, TRAFFIC_PHASE_NORTH_GREEN);
            }
            break;
        case TRAFFIC_PHASE_NORTH_GREEN:
            if (context->phase_elapsed_ms >= phase_duration_for(TRAFFIC_PHASE_NORTH_GREEN)) {
                advance_phase(context, TRAFFIC_PHASE_NORTH_YELLOW);
            }
            break;
        case TRAFFIC_PHASE_NORTH_YELLOW:
            if (context->phase_elapsed_ms >= phase_duration_for(TRAFFIC_PHASE_NORTH_YELLOW)) {
                advance_phase(context, TRAFFIC_PHASE_ALL_RED_TO_EAST);
            }
            break;
        case TRAFFIC_PHASE_ALL_RED_TO_EAST:
            if (context->phase_elapsed_ms >= phase_duration_for(TRAFFIC_PHASE_ALL_RED_TO_EAST)) {
                advance_phase(context, TRAFFIC_PHASE_EAST_GREEN);
            }
            break;
        default:
            advance_phase(context, TRAFFIC_PHASE_EAST_GREEN);
            break;
    }
}

bool traffic_fsm_should_clear_request(const TrafficFsmContext *context) {
    if (context == nullptr) {
        return false;
    }

    return context->phase == TRAFFIC_PHASE_NORTH_GREEN && context->phase_elapsed_ms == 0;
}

TrafficSnapshot traffic_fsm_get_snapshot(
    const TrafficFsmContext *context,
    const bool north_request_pending
) {
    TrafficSnapshot snapshot = {};

    if (context == nullptr) {
        return snapshot;
    }

    snapshot.phase = context->phase;
    snapshot.north_request_pending = north_request_pending;
    snapshot.phase_elapsed_ms = context->phase_elapsed_ms;
    snapshot.phase_duration_ms = context->phase_duration_ms;

    switch (context->phase) {
        case TRAFFIC_PHASE_EAST_GREEN:
            snapshot.east = build_direction(TRAFFIC_LAMP_GREEN);
            snapshot.north = build_direction(TRAFFIC_LAMP_RED);
            break;
        case TRAFFIC_PHASE_EAST_YELLOW:
            snapshot.east = build_direction(TRAFFIC_LAMP_YELLOW);
            snapshot.north = build_direction(TRAFFIC_LAMP_RED);
            break;
        case TRAFFIC_PHASE_ALL_RED_TO_NORTH:
        case TRAFFIC_PHASE_ALL_RED_TO_EAST:
            snapshot.east = build_direction(TRAFFIC_LAMP_RED);
            snapshot.north = build_direction(TRAFFIC_LAMP_RED);
            break;
        case TRAFFIC_PHASE_NORTH_GREEN:
            snapshot.east = build_direction(TRAFFIC_LAMP_RED);
            snapshot.north = build_direction(TRAFFIC_LAMP_GREEN);
            break;
        case TRAFFIC_PHASE_NORTH_YELLOW:
            snapshot.east = build_direction(TRAFFIC_LAMP_RED);
            snapshot.north = build_direction(TRAFFIC_LAMP_YELLOW);
            break;
        default:
            snapshot.east = build_direction(TRAFFIC_LAMP_RED);
            snapshot.north = build_direction(TRAFFIC_LAMP_RED);
            break;
    }

    return snapshot;
}
