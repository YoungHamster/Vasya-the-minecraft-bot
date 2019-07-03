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