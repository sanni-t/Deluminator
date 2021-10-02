#ifndef roomlights_h
#define roomlights_h

#include<Arduino.h>

enum class LightStatus
{
	OFF = bool(0),
	ON = bool(1)
};


struct LightInfo
{
	String on_event;
	String off_event;
	LightStatus status;
};

class RoomLights
{
    public:
        RoomLights(LightInfo* light_info_array, uint8_t num_of_lights);
		LightInfo* all_lights();
		void update_all_lights_status(LightStatus status);
		LightInfo* get_next_light();
		bool are_all_on();
		bool are_all_off();
		uint8_t num_lights;
		~RoomLights();
	private:
		LightInfo* _lights = NULL;
		uint8_t _last_handled_light = 0;
};

#endif