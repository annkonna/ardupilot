/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// ArduSub scheduling, originally copied from ArduCopter

#include "Sub.h"

#define SCHED_TASK(func, rate_hz, max_time_micros) SCHED_TASK_CLASS(Sub, &sub, func, rate_hz, max_time_micros)

/*
  scheduler table for fast CPUs - all regular tasks apart from the fast_loop()
  should be listed here, along with how often they should be called (in hz)
  and the maximum time they are expected to take (in microseconds)
 */
const AP_Scheduler::Task Sub::scheduler_tasks[] = {
    SCHED_TASK(fifty_hz_loop,         50,     75),
    SCHED_TASK_CLASS(AP_GPS, &sub.gps, update, 50, 200),
#if OPTFLOW == ENABLED
    SCHED_TASK_CLASS(OpticalFlow,          &sub.optflow,             update,         200, 160),
#endif
    SCHED_TASK(update_batt_compass,   10,    120),
    SCHED_TASK(read_rangefinder,      20,    100),
    SCHED_TASK(update_altitude,       10,    100),
    SCHED_TASK(three_hz_loop,          3,     75),
    SCHED_TASK(update_turn_counter,   10,     50),
    SCHED_TASK_CLASS(AP_Baro,             &sub.barometer,    accumulate,          50,  90),
    SCHED_TASK_CLASS(AP_Notify,           &sub.notify,       update,              50,  90),
    SCHED_TASK(one_hz_loop,            1,    100),
    SCHED_TASK_CLASS(GCS,                 (GCS*)&sub._gcs,   update_receive,     400, 180),
    SCHED_TASK_CLASS(GCS,                 (GCS*)&sub._gcs,   update_send,        400, 550),
#if AC_FENCE == ENABLED
    SCHED_TASK_CLASS(AC_Fence,            &sub.fence,        update,              10, 100),
#endif
#if HAL_MOUNT_ENABLED
    SCHED_TASK_CLASS(AP_Mount,            &sub.camera_mount, update,              50,  75),
#endif
#if CAMERA == ENABLED
    SCHED_TASK_CLASS(AP_Camera,           &sub.camera,       update,              50,  75),
#endif
    SCHED_TASK(ten_hz_logging_loop,   10,    350),
    SCHED_TASK(twentyfive_hz_logging, 25,    110),
    SCHED_TASK_CLASS(AP_Logger,     &sub.logger,    periodic_tasks,     400, 300),
    SCHED_TASK_CLASS(AP_InertialSensor,   sub.ins,          periodic,           400,  50),
    SCHED_TASK_CLASS(AP_Scheduler,        &sub.scheduler,    update_logging,     0.1,  75),
#if RPM_ENABLED == ENABLED
    SCHED_TASK(rpm_update,            10,    200),
#endif
    SCHED_TASK_CLASS(Compass,          &sub.compass,              cal_update, 100, 100),
    SCHED_TASK(accel_cal_update,      10,    100),
    SCHED_TASK(terrain_update,        10,    100),
#if GRIPPER_ENABLED == ENABLED
    SCHED_TASK_CLASS(AP_Gripper,          &sub.g2.gripper,       update,              10,  75),
#endif
#ifdef USERHOOK_FASTLOOP
    SCHED_TASK(userhook_FastLoop,    100,     75),
#endif
#ifdef USERHOOK_50HZLOOP
    SCHED_TASK(userhook_50Hz,         50,     75),
#endif
#ifdef USERHOOK_MEDIUMLOOP
    SCHED_TASK(userhook_MediumLoop,   10,     75),
#endif
#ifdef USERHOOK_SLOWLOOP
    SCHED_TASK(userhook_SlowLoop,     3.3,    75),
#endif
#ifdef USERHOOK_SUPERSLOWLOOP
    SCHED_TASK(userhook_SuperSlowLoop, 1,   75),
#endif
    SCHED_TASK(read_airspeed,          10,    100),
};

void Sub::get_scheduler_tasks(const AP_Scheduler::Task *&tasks,
                                 uint8_t &task_count,
                                 uint32_t &log_bit)
{
    tasks = &scheduler_tasks[0];
    task_count = ARRAY_SIZE(scheduler_tasks);
    log_bit = MASK_LOG_PM;
}

constexpr int8_t Sub::_failsafe_priorities[5];

// Main loop - 400hz
void Sub::fast_loop()
{
    // update INS immediately to get current gyro data populated
    ins->update();

    //don't run rate controller in manual or motordetection modes
    if (control_mode != MANUAL && control_mode != MOTOR_DETECT) {
        // run low level rate controllers that only require IMU data
        attitude_control.rate_controller_run();
    }

    // send outputs to the motors library
    motors_output();

    // run EKF state estimator (expensive)
    // --------------------
    read_AHRS();

    // Inertial Nav
    // --------------------
    read_inertia();

    // check if ekf has reset target heading
    check_ekf_yaw_reset();

    // run the attitude controllers
    update_flight_mode();

    // update home from EKF if necessary
    update_home_from_EKF();

    // check if we've reached the surface or bottom
    update_surface_and_bottom_detector();

#if HAL_MOUNT_ENABLED
    // camera mount's fast update
    camera_mount.update_fast();
#endif

    // log sensor health
    if (should_log(MASK_LOG_ANY)) {
        Log_Sensor_Health();
    }

    AP_Vehicle::fast_loop();
}

// 50 Hz tasks
void Sub::fifty_hz_loop()
{
    // check pilot input failsafe
    failsafe_pilot_input_check();

    failsafe_crash_check();

    failsafe_ekf_check();

    failsafe_sensors_check();

    // Update rc input/output
    rc().read_input();
    SRV_Channels::output_ch_all();
}

// update_batt_compass - read battery and compass
// should be called at 10hz
void Sub::update_batt_compass()
{
    // read battery before compass because it may be used for motor interference compensation
    battery.read();

    if (AP::compass().available()) {
        // update compass with throttle value - used for compassmot
        compass.set_throttle(motors.get_throttle());
        compass.read();
    }
}

// ten_hz_logging_loop
// should be run at 10hz
void Sub::ten_hz_logging_loop()
{
    // log attitude data if we're not already logging at the higher rate
    if (should_log(MASK_LOG_ATTITUDE_MED) && !should_log(MASK_LOG_ATTITUDE_FAST)) {
        Log_Write_Attitude();
        ahrs_view.Write_Rate(motors, attitude_control, pos_control);
        if (should_log(MASK_LOG_PID)) {
            logger.Write_PID(LOG_PIDR_MSG, attitude_control.get_rate_roll_pid().get_pid_info());
            logger.Write_PID(LOG_PIDP_MSG, attitude_control.get_rate_pitch_pid().get_pid_info());
            logger.Write_PID(LOG_PIDY_MSG, attitude_control.get_rate_yaw_pid().get_pid_info());
            logger.Write_PID(LOG_PIDA_MSG, pos_control.get_accel_z_pid().get_pid_info());
        }
    }
    if (should_log(MASK_LOG_MOTBATT)) {
        Log_Write_MotBatt();
    }
    if (should_log(MASK_LOG_RCIN)) {
        logger.Write_RCIN();
    }
    if (should_log(MASK_LOG_RCOUT)) {
        logger.Write_RCOUT();
    }
    if (should_log(MASK_LOG_NTUN) && (mode_requires_GPS(control_mode) || !mode_has_manual_throttle(control_mode))) {
        pos_control.write_log();
    }
    if (should_log(MASK_LOG_IMU) || should_log(MASK_LOG_IMU_FAST) || should_log(MASK_LOG_IMU_RAW)) {
        AP::ins().Write_Vibration();
    }
    if (should_log(MASK_LOG_CTUN)) {
        attitude_control.control_monitor_log();
    }
}

// twentyfive_hz_logging_loop
// should be run at 25hz
void Sub::twentyfive_hz_logging()
{
    if (should_log(MASK_LOG_ATTITUDE_FAST)) {
        Log_Write_Attitude();
        ahrs_view.Write_Rate(motors, attitude_control, pos_control);
        if (should_log(MASK_LOG_PID)) {
            logger.Write_PID(LOG_PIDR_MSG, attitude_control.get_rate_roll_pid().get_pid_info());
            logger.Write_PID(LOG_PIDP_MSG, attitude_control.get_rate_pitch_pid().get_pid_info());
            logger.Write_PID(LOG_PIDY_MSG, attitude_control.get_rate_yaw_pid().get_pid_info());
            logger.Write_PID(LOG_PIDA_MSG, pos_control.get_accel_z_pid().get_pid_info());
        }
    }

    // log IMU data if we're not already logging at the higher rate
    if (should_log(MASK_LOG_IMU) && !should_log(MASK_LOG_IMU_RAW)) {
        AP::ins().Write_IMU();
    }
}

// three_hz_loop - 3.3hz loop
void Sub::three_hz_loop()
{
    leak_detector.update();

    failsafe_leak_check();

    failsafe_internal_pressure_check();

    failsafe_internal_temperature_check();

    // check if we've lost contact with the ground station
    failsafe_gcs_check();

    // check if we've lost terrain data
    failsafe_terrain_check();

#if AC_FENCE == ENABLED
    // check if we have breached a fence
    fence_check();
#endif // AC_FENCE_ENABLED

    ServoRelayEvents.update_events();
}

// one_hz_loop - runs at 1Hz
void Sub::one_hz_loop()
{
    bool arm_check = arming.pre_arm_checks(false);
    ap.pre_arm_check = arm_check;
    AP_Notify::flags.pre_arm_check = arm_check;
    AP_Notify::flags.pre_arm_gps_check = position_ok();
    AP_Notify::flags.flying = motors.armed();

    if (should_log(MASK_LOG_ANY)) {
        Log_Write_Data(LogDataID::AP_STATE, ap.value);
    }

    if (!motors.armed()) {
        // make it possible to change ahrs orientation at runtime during initial config
        ahrs.update_orientation();

        // set all throttle channel settings
        motors.set_throttle_range(channel_throttle->get_radio_min(), channel_throttle->get_radio_max());
    }

    // update assigned functions and enable auxiliary servos
    SRV_Channels::enable_aux_servos();

    // update position controller alt limits
    update_poscon_alt_max();

    // log terrain data
    terrain_logging();

    // need to set "likely flying" when armed to allow for compass
    // learning to run
    set_likely_flying(hal.util->get_soft_armed());
}

void Sub::read_AHRS()
{
    // Perform IMU calculations and get attitude info
    //-----------------------------------------------
    // <true> tells AHRS to skip INS update as we have already done it in fast_loop()
    ahrs.update(true);
    ahrs_view.update();
}

// read baro and rangefinder altitude at 10hz
void Sub::update_altitude()
{
    // read in baro altitude
    read_barometer();

    if (should_log(MASK_LOG_CTUN)) {
        Log_Write_Control_Tuning();
#if HAL_GYROFFT_ENABLED
        gyro_fft.write_log_messages();
#else
        write_notch_log_messages();
#endif
    }
}

bool Sub::control_check_barometer()
{
#if CONFIG_HAL_BOARD != HAL_BOARD_SITL
    if (!ap.depth_sensor_present) { // can't hold depth without a depth sensor
        gcs().send_text(MAV_SEVERITY_WARNING, "Depth sensor is not connected.");
        return false;
    } else if (failsafe.sensor_health) {
        gcs().send_text(MAV_SEVERITY_WARNING, "Depth sensor error.");
        return false;
    }
#endif
    return true;
}

// vehicle specific waypoint info helpers
bool Sub::get_wp_distance_m(float &distance) const
{
    // see GCS_MAVLINK_Sub::send_nav_controller_output()
    distance = sub.wp_nav.get_wp_distance_to_destination() * 0.01;
    return true;
}

// vehicle specific waypoint info helpers
bool Sub::get_wp_bearing_deg(float &bearing) const
{
    // see GCS_MAVLINK_Sub::send_nav_controller_output()
    bearing = sub.wp_nav.get_wp_bearing_to_destination() * 0.01;
    return true;
}

// vehicle specific waypoint info helpers
bool Sub::get_wp_crosstrack_error_m(float &xtrack_error) const
{
    // no crosstrack error reported, see GCS_MAVLINK_Sub::send_nav_controller_output()
    xtrack_error = 0;
    return true;
}

AP_HAL_MAIN_CALLBACKS(&sub);
