#pragma once

class Wave
{
public: 
	virtual float get_left_phase() const = 0;
	virtual float get_right_phase() const = 0;

	virtual void stream(unsigned int curr_frame) = 0;
	virtual float get_gain() const = 0;

	virtual ~Wave() = 0;
};