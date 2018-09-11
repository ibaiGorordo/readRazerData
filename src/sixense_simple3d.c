#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <sixense.h>
#include <sixense_math.hpp>
#ifdef WIN32
#include <sixense_utils/mouse_pointer.hpp>
#endif
#include <sixense_utils/derivatives.hpp>
#include <sixense_utils/button_states.hpp>
#include <sixense_utils/event_triggers.hpp>
#include <sixense_utils/controller_manager/controller_manager.hpp>
#include <windows.h>
#include <ctime>

#define SECOND_TO_MILLIS 1000
#define SAMPLING_PERIOD 4

// whether or not we are currently logging position data to a file, and the file pointer to which to log
static int is_logging = 0;
static FILE *log_file = 0;

// flags that the controller manager system can set to tell the graphics system to draw the instructions
// for the player
static bool controller_manager_screen_visible = true;
std::string controller_manager_text_string;

// Variables to check the elapsed time
LONG time_init;
LONG time_now;
LONG time_prev;


// Write a bunch of instruction text, as well as the current position and rotation information
void draw_controller_info() {
	sixenseAllControllerData acd;
	int base, cont;
	unsigned int ellapsedMillis = time_now - time_init;
	
	// Update the text
	sixenseSetActiveBase(0);

	for (base = 0; base < sixenseGetMaxBases(); base++) {
		sixenseSetActiveBase(base);
		sixenseGetAllNewestData(&acd);

		for (cont = 0; cont < sixenseGetMaxControllers(); cont++) {

			if (sixenseIsControllerEnabled(cont)) {
				
				// Print the position of the controllers
				printf("base: %d controller: %d   pos: %f %f %f\n", base, cont,
					acd.controllers[cont].pos[0], acd.controllers[cont].pos[1], acd.controllers[cont].pos[2]);				
				

				/*
				// Print information about the controllers:
				printf("Controller index: %d\nFirmware Version: %u\nHardware Version: %u\nHemi Tracking enabled: %u\n Magnetic frequency: %u\nPacket type: %u\nSequence number: %u\nHand: %u\n\n",
					acd.controllers[cont].controller_index, acd.controllers[cont].firmware_revision, acd.controllers[cont].hardware_revision, acd.controllers[cont].hemi_tracking_enabled, acd.controllers[cont].magnetic_frequency,
					acd.controllers[cont].packet_type, acd.controllers[cont].sequence_number, acd.controllers[cont].which_hand);
				*/

				/*
				// Print the controller index and the incremental number (To check if there are missing samples)
				printf("Cont: %d Incr: %u\n", acd.controllers[cont].controller_index, acd.controllers[cont].sequence_number);
				*/

				// If logging is enabled then save the data into the file:
				if (is_logging)
				{
					// Write the ellapsed seconds and milliseconds
					fprintf(log_file, "%d, %d, ", ellapsedMillis / SECOND_TO_MILLIS, ellapsedMillis % SECOND_TO_MILLIS);

					// Write the controller ID and the incremental value
					fprintf(log_file, "%d, %u, ", acd.controllers[cont].controller_index, acd.controllers[cont].sequence_number);

					// Write the position of the controller
					fprintf(log_file, "%f, %f, %f, ", acd.controllers[cont].pos[0], acd.controllers[cont].pos[1], acd.controllers[cont].pos[2]);

					// Write the rotation matrix
					fprintf(log_file, "%f, %f, %f, %f, %f, %f, %f, %f, %f",
						acd.controllers[cont].rot_mat[0][0], acd.controllers[cont].rot_mat[0][1], acd.controllers[cont].rot_mat[0][2],
						acd.controllers[cont].rot_mat[1][0], acd.controllers[cont].rot_mat[1][1], acd.controllers[cont].rot_mat[1][2],
						acd.controllers[cont].rot_mat[2][0], acd.controllers[cont].rot_mat[2][1], acd.controllers[cont].rot_mat[2][2]);

					fprintf(log_file, "\n");
				}
			}

		}

	}
}

// This is the callback that gets registered with the sixenseUtils::controller_manager. It will get called each time the user completes
// one of the setup steps so that the game can update the instructions to the user. If the engine supports texture mapping, the 
// controller_manager can prove a pathname to a image file that contains the instructions in graphic form.
// The controller_manager serves the following functions:
//  1) Makes sure the appropriate number of controllers are connected to the system. The number of required controllers is designaged by the
//     game type (ie two player two controller game requires 4 controllers, one player one controller game requires one)
//  2) Makes the player designate which controllers are held in which hand.
//  3) Enables hemisphere tracking by calling the Sixense API call sixenseAutoEnableHemisphereTracking. After this is completed full 360 degree
//     tracking is possible.
void controller_manager_setup_callback( sixenseUtils::ControllerManager::setup_step step ) {

	if( sixenseUtils::getTheControllerManager()->isMenuVisible() ) {

		// Turn on the flag that tells the graphics system to draw the instruction screen instead of the controller information. The game
		// should be paused at this time.
		controller_manager_screen_visible = true;

		// Ask the controller manager what the next instruction string should be.
		controller_manager_text_string = sixenseUtils::getTheControllerManager()->getStepString();

		// We could also load the supplied controllermanager textures using the filename: sixenseUtils::getTheControllerManager()->getTextureFileName();

	} else {

		// We're done with the setup, so hide the instruction screen.
		controller_manager_screen_visible = false;

	}

}

// Draw the grey screen with a single yellow line of text to prompt the user through the setup steps.
void draw_controller_manager_screen() {
	static std::string prevStr("");

	// Only print the String if it is different from the previous displayed String
	if (prevStr.compare(controller_manager_text_string.c_str()) != 0)
	{
		prevStr = controller_manager_text_string.c_str();
		printf(controller_manager_text_string.c_str());
		printf("\n");
		
	}
}


// The function calls this function each frame
static void display(void)
{
	// update the controller manager with the latest controller data here
	sixenseSetActiveBase(0);
	sixenseAllControllerData acd;
	sixenseGetAllNewestData( &acd );
	sixenseUtils::getTheControllerManager()->update( &acd );

	// Either draw the controller manager instruction screen, or display the controller information
	if( controller_manager_screen_visible ) {
		draw_controller_manager_screen();
	} else {
		draw_controller_info();
	}
/*
	if( is_logging ) {
		updateLog();
	}*/
}

// Toggle logging data to a file
static void 
	toggleLogging() {

	char time_char[80];
	char fileName[80];
	time_t rawtime;
	struct tm * timeinfo;

	
	if( is_logging ) {
		is_logging = 0;

		fclose( log_file );
	} else {
		is_logging = 1;

		// Get local time for the logging file name:
		time(&rawtime);
		timeinfo = localtime(&rawtime);

		// Write the fileName and open it
		sprintf(fileName, "..\\..\\Output Data\\trial_%d_%d_%d-%d_%d.txt", timeinfo->tm_year+1900, timeinfo->tm_mon+1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min);
		log_file = fopen(fileName, "w" );

		// Write the Header for the logging file
		fprintf(log_file, "Seconds, Millis, Contr, Frame, posX, posY, posZ, rot11, rot12, rot13, rot21, rot22, rot23, rot31, rot32, rot33\n");
	}
}

// Function to calculate the time in milliseconds
long long milliseconds_now() {
	static LARGE_INTEGER s_frequency;
	static BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
	if (s_use_qpc) {
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return (1000LL * now.QuadPart) / s_frequency.QuadPart;
	}
	else {
		return GetTickCount();
	}
}

int main(int argc, char *argv[])
{
	float hemi_vec[3] = { 0, 1, 0 };
	int counter = 0;
	int elapsedTotalMilliSeconds;
	
	// Init sixense
	sixenseInit();

	// Init logging:
	toggleLogging();

	// Init the controller manager. This makes sure the controllers are present, assigned to left and right hands, and that
	// the hemisphere calibration is complete.
	sixenseUtils::getTheControllerManager()->setGameType( sixenseUtils::ControllerManager::ONE_PLAYER_TWO_CONTROLLER );
	sixenseUtils::getTheControllerManager()->registerSetupCallback( controller_manager_setup_callback );

	// Init the time count
	time_init = milliseconds_now();
	time_prev = time_init;
	
	while (1)
	{
		// Get the current time
		time_now = milliseconds_now();
		
		if (time_now - time_prev >= SAMPLING_PERIOD)
		{
			display();
			time_prev = time_now;	
		}


		// If the exit key is pressed then finish the program
		if (GetKeyState('Q') & 0x8000)
		{
			// Finish logging
			toggleLogging();

			// Calculate the total ellapsed time
			elapsedTotalMilliSeconds = (time_now - time_init);
			printf("Seconds: %d, Mili: %d", elapsedTotalMilliSeconds/1000, elapsedTotalMilliSeconds % 1000);

			break;
		}
	}
	
	sixenseExit();

	return EXIT_SUCCESS;
}
