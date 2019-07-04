#pragma once

enum ConnectionStates
{
	Handshaking,
	Login,
	Play,
	Disconnected
};

enum Dimensions
{
	Nether = -1,
	Overworld = 0,
	End = 1
};

enum ChangeGameStateReasons
{
	InvalidBed = 0,
	EndRaining = 1,
	BeginRaining = 2,
	ChangeGamemode = 3,
	ExitEnd = 4,
	DemoMessage = 5,
	ArrowHittingPlayer = 6,
	FadeValue = 7,
	FadeTime = 8,
	PlayPufferFishStingSound = 9, // W T
	PlayElderGuardianMobAppearance = 10 // F
};

enum EntityStatuses
{

};

#define MAX_PACKET_SIZE 2097153 /* max packet size is 2097152, 
								   number is bigger to easily detect too big packets and disconnect from serverts, that send them */
#define BLOCK_OR_SKY_LIGHT_SIZE 2048