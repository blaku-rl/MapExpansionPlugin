#pragma once

struct InputControls {
	bool overrideThrottle;
	float throttle;
	bool overrideSteer;
	float steer;
	bool overridePitch;
	float pitch;
	bool overrideYaw;
	float yaw;
	bool overrideRoll;
	float roll;
	bool overrideDodgeForward;
	float dodgeForward;
	bool overrideDodgeStrafe;
	float dodgeStrafe;
	bool overrideHandbrake;
	unsigned long handbrake;
	bool overrideJump;
	unsigned long jump;
	bool overrideActivateBoost;
	unsigned long activateBoost;
	bool overrideHoldingBoost;
	unsigned long holdingBoost;
	bool overrideJumped;
	unsigned long jumped;
};