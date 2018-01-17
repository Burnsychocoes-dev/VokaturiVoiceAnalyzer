#define SHARED_BUFFER_SIZE  2205550
#include <iostream>



struct SharedBuffer{
	bool end;
	int64_t previousPointer;
	int64_t samplePointer;
	double duration;
	double samplingFrequency;
	//minTime = temps que l'on veut analyser
	double minTime;
	double previousTime;
	double currentTime;
	int64_t nbOfSamples;
	volatile int64_t numberOfReceivedSamples;
	int64_t numberOfSentSamples;
	double samples[SHARED_BUFFER_SIZE];  // so that's approximately 1.76 megabytes
};

extern struct SharedBuffer theSharedBuffer;


