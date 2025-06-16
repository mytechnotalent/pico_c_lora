/**
 * @file stepper.c
 * @brief ULN2003 28BYJ-48 4-Phase Stepper Motor Driver Implementation
 * @ver        stepper_apply_step(motor, motor->current_step);

        // NO DELAYS - immediate interrupt response
    } @date 2025-06-15
 * @author Kevin Thomas
 *
 * This source file implements stepper motor control functions for the ULN2003
 * 28BYJ-48 4-phase stepper motors. Uses 4-phase half-stepping for smooth operation.
 *
 * @copyright Copyright (c) 2025 Kevin Thomas
 */

#include <stdio.h>
#include "stepper.h"

// Global interrupt flag to stop stepper operations immediately
static volatile bool stepper_interrupt_flag = false;

// 4-phase stepping sequence for smooth motor operation (half-stepping)
static const bool step_sequence[8][4] = {
    {1, 0, 0, 0}, // Step 0: Phase 1
    {1, 1, 0, 0}, // Step 1: Phase 1+2
    {0, 1, 0, 0}, // Step 2: Phase 2
    {0, 1, 1, 0}, // Step 3: Phase 2+3
    {0, 0, 1, 0}, // Step 4: Phase 3
    {0, 0, 1, 1}, // Step 5: Phase 3+4
    {0, 0, 0, 1}, // Step 6: Phase 4
    {1, 0, 0, 1}  // Step 7: Phase 4+1
};

static void stepper_apply_step(stepper_motor_t *motor, int step)
{
    if (!motor->enabled)
    {
        return;
    }

    gpio_put(motor->pin1, step_sequence[step][0]);
    gpio_put(motor->pin2, step_sequence[step][1]);
    gpio_put(motor->pin3, step_sequence[step][2]);
    gpio_put(motor->pin4, step_sequence[step][3]);

    motor->current_step = step;
}

bool stepper_init(stepper_motor_t *motor, uint pin1, uint pin2, uint pin3, uint pin4, uint step_delay)
{
    if (motor == NULL)
    {
        return false;
    }

    motor->pin1 = pin1;
    motor->pin2 = pin2;
    motor->pin3 = pin3;
    motor->pin4 = pin4;
    motor->step_delay = step_delay;
    motor->current_step = 0;
    motor->enabled = true;

    gpio_init(pin1);
    gpio_init(pin2);
    gpio_init(pin3);
    gpio_init(pin4);

    gpio_set_dir(pin1, GPIO_OUT);
    gpio_set_dir(pin2, GPIO_OUT);
    gpio_set_dir(pin3, GPIO_OUT);
    gpio_set_dir(pin4, GPIO_OUT);

    stepper_apply_step(motor, 0);

    return true;
}

void stepper_move_steps(stepper_motor_t *motor, uint steps, stepper_direction_t direction)
{
    if (motor == NULL || !motor->enabled)
    {
        return;
    }

    for (uint i = 0; i < steps; i++)
    {
        // Check for interrupt flag before each step
        if (stepper_interrupt_flag)
        {
            return;
        }

        if (direction == STEPPER_CW)
        {
            motor->current_step = (motor->current_step + 1) % 8;
        }
        else
        {
            motor->current_step = (motor->current_step - 1 + 8) % 8;
        }

        stepper_apply_step(motor, motor->current_step);

        // Essential delay for stepper motor timing - but make it interruptible
        uint remaining_delay = motor->step_delay;
        while (remaining_delay > 0 && !stepper_interrupt_flag)
        {
            uint chunk_delay = (remaining_delay > 1) ? 1 : remaining_delay;
            sleep_ms(chunk_delay);
            remaining_delay -= chunk_delay;
        }

        // Exit immediately if interrupted during delay
        if (stepper_interrupt_flag)
        {
            return;
        }
    }
}

void stepper_rotate_degrees(stepper_motor_t *motor, float degrees, stepper_direction_t direction)
{
    if (motor == NULL || !motor->enabled)
    {
        return;
    }

    uint steps = (uint)((degrees / 360.0f) * STEPS_PER_REVOLUTION);
    stepper_move_steps(motor, steps, direction);
}

void stepper_disable(stepper_motor_t *motor)
{
    if (motor == NULL)
    {
        return;
    }

    motor->enabled = false;

    gpio_put(motor->pin1, 0);
    gpio_put(motor->pin2, 0);
    gpio_put(motor->pin3, 0);
    gpio_put(motor->pin4, 0);
}

void stepper_enable(stepper_motor_t *motor)
{
    if (motor == NULL)
    {
        return;
    }

    motor->enabled = true;
    stepper_apply_step(motor, motor->current_step);
}

void stepper_set_speed(stepper_motor_t *motor, uint step_delay)
{
    if (motor == NULL)
    {
        return;
    }

    motor->step_delay = step_delay;
}

int stepper_get_position(stepper_motor_t *motor)
{
    if (motor == NULL)
    {
        return -1;
    }

    return motor->current_step;
}

void stepper_rotate_multiple_degrees(stepper_motor_t *motors[], uint num_motors,
                                     float degrees, stepper_direction_t direction)
{
    if (motors == NULL || num_motors == 0)
    {
        return;
    }

    // Calculate steps needed for all motors
    uint steps = (uint)((degrees / 360.0f) * STEPS_PER_REVOLUTION);

    // Rotate all motors simultaneously step by step
    for (uint step = 0; step < steps; step++)
    {
        // Check for interrupt flag before each step
        if (stepper_interrupt_flag)
        {
            return;
        }

        for (uint i = 0; i < num_motors; i++)
        {
            stepper_motor_t *motor = motors[i];
            if (motor != NULL && motor->enabled)
            {
                if (direction == STEPPER_CW)
                {
                    motor->current_step = (motor->current_step + 1) % 8;
                }
                else
                {
                    motor->current_step = (motor->current_step - 1 + 8) % 8;
                }
                stepper_apply_step(motor, motor->current_step);
            }
        }

        // Essential delay for stepper motor timing - but make it interruptible
        // Use the delay from the first valid motor
        for (uint i = 0; i < num_motors; i++)
        {
            if (motors[i] != NULL && motors[i]->enabled)
            {
                uint remaining_delay = motors[i]->step_delay;
                while (remaining_delay > 0 && !stepper_interrupt_flag)
                {
                    uint chunk_delay = (remaining_delay > 1) ? 1 : remaining_delay;
                    sleep_ms(chunk_delay);
                    remaining_delay -= chunk_delay;
                }
                break; // Only use delay from first motor
            }
        }

        // Exit immediately if interrupted during delay
        if (stepper_interrupt_flag)
        {
            return;
        }
    }
}

void stepper_demo_sequence(stepper_motor_t *motors[], uint num_motors,
                           float degrees, uint pause_ms)
{
    if (motors == NULL || num_motors == 0)
    {
        return;
    }

    uint remaining_ms; // For interruptible delays

    printf("Running stepper motor demonstration sequence...\n");

    // Clockwise rotation for all motors
    printf("Rotating all motors clockwise %.1f degrees\n", degrees);
    stepper_rotate_multiple_degrees(motors, num_motors, degrees, STEPPER_CW);

    // Check for interrupt after clockwise rotation
    if (stepper_interrupt_flag)
    {
        printf("Stepper: Demo sequence interrupted after clockwise rotation\n");
        return;
    }

    // Use interruptible delay - break pause into smaller chunks
    remaining_ms = pause_ms;
    while (remaining_ms > 0 && !stepper_interrupt_flag)
    {
        uint chunk_ms = (remaining_ms > 10) ? 10 : remaining_ms;
        sleep_ms(chunk_ms);
        remaining_ms -= chunk_ms;
    }

    // Check for interrupt after pause
    if (stepper_interrupt_flag)
    {
        printf("Stepper: Demo sequence interrupted during pause\n");
        return;
    }

    // Counter-clockwise rotation for all motors
    printf("Rotating all motors counter-clockwise %.1f degrees\n", degrees);
    stepper_rotate_multiple_degrees(motors, num_motors, degrees, STEPPER_CCW);

    // Check for interrupt after counter-clockwise rotation
    if (stepper_interrupt_flag)
    {
        printf("Stepper: Demo sequence interrupted after counter-clockwise rotation\n");
        return;
    }

    // Use interruptible delay - break pause into smaller chunks
    remaining_ms = pause_ms;
    while (remaining_ms > 0 && !stepper_interrupt_flag)
    {
        uint chunk_ms = (remaining_ms > 10) ? 10 : remaining_ms;
        sleep_ms(chunk_ms);
        remaining_ms -= chunk_ms;
    }

    printf("Demonstration sequence complete\n");
}

void stepper_set_interrupt(void)
{
    stepper_interrupt_flag = true;
}

void stepper_emergency_stop_all(stepper_motor_t *motors, uint num_motors)
{
    if (motors == NULL)
    {
        return;
    }

    // Immediately disable all motor outputs - NO DELAYS
    for (uint i = 0; i < num_motors; i++)
    {
        stepper_motor_t *motor = &motors[i];

        // Turn off all GPIO pins immediately
        gpio_put(motor->pin1, 0);
        gpio_put(motor->pin2, 0);
        gpio_put(motor->pin3, 0);
        gpio_put(motor->pin4, 0);

        // Mark motor as disabled
        motor->enabled = false;
    }
}

void stepper_clear_interrupt(void)
{
    stepper_interrupt_flag = false;
}

bool stepper_is_interrupted(void)
{
    return stepper_interrupt_flag;
}
