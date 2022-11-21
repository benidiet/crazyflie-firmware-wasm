#!/usr/bin/env python

import cffirmware

def test_controller_non_linear_attitude():

    cffirmware.controllerNonLinearAttitudeInit()

    control = cffirmware.control_t()
    setpoint = cffirmware.setpoint_t()
    setpoint.mode.z = cffirmware.modeVelocity
    setpoint.velocity.z = 0
    setpoint.acceleration.z = 0
    setpoint.mode.x = cffirmware.modeVelocity
    setpoint.velocity.x = 0
    setpoint.acceleration.x = 0
    setpoint.mode.y = cffirmware.modeVelocity
    setpoint.velocity.y = 0
    setpoint.acceleration.y = 0
    setpoint.mode.yaw = cffirmware.modeVelocity
    setpoint.attitudeRate.yaw = 0

    state = cffirmware.state_t()
    state.attitudeQuaternion.x = 0
    state.attitudeQuaternion.y = 0
    state.attitudeQuaternion.z = 0
    state.attitudeQuaternion.w = 1
    state.position.x = 0
    state.position.y = 0
    state.position.z = 0
    state.velocity.x = 0
    state.velocity.y = 0
    state.velocity.z = 0

    sensors = cffirmware.sensorData_t()
    sensors.gyro.x = 0
    sensors.gyro.y = 0
    sensors.gyro.z = 0

    tick = 100

    cffirmware.controllerNonLinearAttitude(control, setpoint,sensors,state,tick)
    assert control.controlMode == cffirmware.controlModeForceTorque
    # control.thrustSi will be at a (tuned) hover-state
    assert control.torqueX == 0
    assert control.torqueY == 0
    assert control.torqueZ == 0
