// ConsoleApplication3.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <math.h>
#include <thread>
#include <mutex>
#include "WavFile.h"
#include "Vokaturi.h"
#include "SharedBuffer.h"

static std::mutex foo, bar;
static int numberOfSeconds = 1;
static int numberOfMilliseconds = 100;
static int numberOfSamples = 220555/5;
static int64_t receivedSamplesForUnity = 0;
static double samplingFrequency;
static double emotions[5];
static FILE* log;
static FILE* log2;
static FILE* logSuccess;

//static void lock_tab() {
//	tab2.lock();
//	tab.lock();
//	return;
//}
//
//static void unlock_tab() {
//	tab2.unlock();
//	tab.unlock();
//	return;
//}


//Pour permettre à Unity de savoir où on en est
static int receivedSample() {
	return(receivedSamplesForUnity);
}

//Pour utiliser readEmotions -> d'abord on vérifie que receivedSamplesForUnity est supérieur au précédent receivedSamples, comme ça cela veut dire que le tableau est mis à jour;
static double* readEmotions() {
	return(emotions);
}

//A utiliser au début, pour initialiser le buffer
static void initBuffer(double m_samplingFrequency, double m_minTime) {
	samplingFrequency = m_samplingFrequency;
	theSharedBuffer.samplingFrequency = m_samplingFrequency;
	theSharedBuffer.minTime = m_minTime;
	theSharedBuffer.samplePointer = 0;
	theSharedBuffer.numberOfSentSamples = 0;
	theSharedBuffer.end = false;
}

//A utiliser au début pour initialiser les logs
static void initFiles() {
	errno_t err, err2, err3;
	err = fopen_s(&log, "timerCallBack_log.txt", "w+");
	if (err == 0)
	{
		printf("The file 'timerCallBack_log.txt' was opened\n");
	}
	else
	{
		printf("The file 'timerCallBack_log.txt' was not opened\n");
	}

	err2 = fopen_s(&log2, "timerCallBack_successlog.txt", "w+");
	if (err2 == 0)
	{
		printf("The file 'timerCallBack_successlog.txt' was opened\n");
	}
	else
	{
		printf("The file 'timerCallBack_successlog.txt' was not opened\n");
	}
	err3 = fopen_s(&logSuccess, "timerCallBack_successlog.txt", "w+");
	if (err3 == 0)
	{
		printf("The file 'timerCallBack_successlog.txt' was opened\n");
	}
	else
	{
		printf("The file 'timerCallBack_successlog.txt' was not opened\n");
	}


}

//Pour envoyer un certain nombre de samples au buffer
static void sendDataToBuffer(double *samples, int nbOfSample) {
	//On initialise des compteurs
	double sec;
	int32_t samplePointer = theSharedBuffer.samplePointer;

	//On lock le buffer pour écrire dedans

	//fprintf(stderr, "rendercallback trying to lock foo \n");
	foo.lock();
	//fprintf(stderr, "rendercallback locked foo \n");
	sec = (theSharedBuffer.numberOfReceivedSamples / theSharedBuffer.samplingFrequency);
	theSharedBuffer.currentTime = sec;
	fprintf(log, "Sample n_ %d  sent ~ %f sec\n", theSharedBuffer.numberOfReceivedSamples,  sec);

	if ((samplePointer + nbOfSample) >= SHARED_BUFFER_SIZE)
		samplePointer = 0;
	theSharedBuffer.previousPointer = samplePointer;	
	memcpy(theSharedBuffer.samples + samplePointer, samples, nbOfSample*sizeof(double));
	
	samplePointer += nbOfSample;
	theSharedBuffer.samplePointer = samplePointer;
	theSharedBuffer.numberOfReceivedSamples += nbOfSample;
	//fprintf(stderr, "rendercallback unlock foo \n");
	foo.unlock();

	//On lock ce verrou pour ne pas retenter immediatement d'envoyer

	//fprintf(stderr, "rendercallback trying to lock bar \n");
	bar.lock();
	/*fprintf(stderr, "rendercallback locked bar \n");
	fprintf(stderr, "rendercallback unlock bar \n");*/
	bar.unlock();

	//On delock dés qu'onla reçu, car on est sûr que l'analyseur a pris la relève

}

//Pour recevoir en permanence les données du buffer et mettre à jour le tab emotions pour unity
static void receiveDataFromBuffer() {
	double* emotionTable[5];
	static VokaturiVoice theVoice;
	if (!theVoice) {
		theVoice = VokaturiVoice_create(44100.0, numberOfSamples * 3);
		if (!theVoice)
			return;
	}
	while(true){
		//On lock le buffer pour pouvoir lire dedans

		//fprintf(stderr, "timercallback trying to lock bar \n");
		bar.lock();
		/*fprintf(stderr, "timercallback locked bar \n");
		fprintf(stderr, "timercallback trying to lock foo \n");*/
		foo.lock();
		//fprintf(stderr, "timercallback locked foo \n");
		if (theSharedBuffer.numberOfReceivedSamples > theSharedBuffer.numberOfSentSamples) {

			//On extrait les samples
			for (int64_t i = theSharedBuffer.numberOfSentSamples;
				i < theSharedBuffer.numberOfReceivedSamples; i += numberOfSamples)
			{
				//int32_t indexOfSampleToBeSent = (int32_t)(i % SHARED_BUFFER_SIZE);				
				VokaturiVoice_fill(theVoice, numberOfSamples,
					&theSharedBuffer.samples[theSharedBuffer.previousPointer]);
			}
			theSharedBuffer.numberOfSentSamples = theSharedBuffer.numberOfReceivedSamples;


			//On delock le buffer
			//fprintf(stderr, "timercallback unlock bar \n");
			bar.unlock();
			//fprintf(stderr, "timercallback unlock foo \n");
			foo.unlock();

			//analyse des samples reçus

			static VokaturiQuality quality;
			static VokaturiEmotionProbabilities emotionProbabilities;
			VokaturiVoice_extract(theVoice, &quality, &emotionProbabilities);
			fprintf(stderr, "Sample n_ %d  received \n", theSharedBuffer.numberOfReceivedSamples);
			fprintf(stderr, "time : %f ~ %d:%d \n", theSharedBuffer.currentTime, (int)floor(theSharedBuffer.currentTime / 60), (int)(theSharedBuffer.currentTime) % 60);
			fprintf(log, "Sample n_ %d  received \n", theSharedBuffer.numberOfReceivedSamples);
			fprintf(log, "time : %f ~ %d:%d \n", theSharedBuffer.currentTime, (int)floor(theSharedBuffer.currentTime / 60), (int)(theSharedBuffer.currentTime) % 60);
			if (quality.valid) {
				fprintf(logSuccess, "Sample n_ %d  received \n", theSharedBuffer.numberOfReceivedSamples);
				fprintf(logSuccess, "time : %f ~ %d:%d \n", theSharedBuffer.currentTime, (int)floor(theSharedBuffer.currentTime / 60), (int)(theSharedBuffer.currentTime) % 60);
				fprintf(logSuccess, "Neutrality %.3f\n", emotionProbabilities.neutrality);
				fprintf(logSuccess, "Happiness %.3f\n", emotionProbabilities.happiness);
				fprintf(logSuccess, "Sadness %.3f\n", emotionProbabilities.sadness);
				fprintf(logSuccess, "Anger %.3f\n", emotionProbabilities.anger);
				fprintf(logSuccess, "Fear %.3f\n", emotionProbabilities.fear);

				printf("Neutrality %.3f\n", emotionProbabilities.neutrality);
				printf("Happiness %.3f\n", emotionProbabilities.happiness);
				printf("Sadness %.3f\n", emotionProbabilities.sadness);
				printf("Anger %.3f\n", emotionProbabilities.anger);
				printf("Fear %.3f\n", emotionProbabilities.fear);

				emotions[0] = emotionProbabilities.neutrality;
				emotions[1] = emotionProbabilities.happiness;
				emotions[2] = emotionProbabilities.sadness;
				emotions[3] = emotionProbabilities.anger;
				emotions[4] = emotionProbabilities.fear;

				receivedSamplesForUnity = theSharedBuffer.numberOfSentSamples;
			}
			else {
				printf("Not enough sonorancy to determine emotions\n");
			}
			//VokaturiVoice_reset(theVoice);
		}
		else {
			//On delock le buffer
			//fprintf(stderr, "timercallback unlock bar \n");
			bar.unlock();
			//fprintf(stderr, "timercallback unlock foo \n");
			foo.unlock();
		}

		////On delock le buffer
		////fprintf(stderr, "timercallback unlock bar \n");
		//bar.unlock();
		////fprintf(stderr, "timercallback unlock foo \n");
		//foo.unlock();

		if (theSharedBuffer.end) {
			return;
		}

	}
}


//Test d'analyse du fichier sans utiliser le buffer en commun
static void analyzeWithoutBuffer(const char * fileName) {
	//On ouvre les fichiers logs
	/*errno_t err;
	FILE *f;
	err = fopen_s(&f, "renderCallBack_log.txt", "w+");
	if (err == 0)
	{
		printf("The file 'renderCallBack_log.txt' was opened\n");
	}
	else
	{
		printf("The file 'renderCallBack_log.txt' was not opened\n");
	}*/
	errno_t err, err2;
	FILE *f, *f2;
	err = fopen_s(&f, "timerCallBack_log.txt", "w+");
	if (err == 0)
	{
		printf("The file 'timerCallBack_log.txt' was opened\n");
	}
	else
	{
		printf("The file 'timerCallBack_log.txt' was not opened\n");
	}

	err2 = fopen_s(&f2, "timerCallBack_successlog.txt", "w+");
	if (err == 0)
	{
		printf("The file 'timerCallBack_successlog.txt' was opened\n");
	}
	else
	{
		printf("The file 'timerCallBack_successlog.txt' was not opened\n");
	}

	//On ouvre le fichier Wav
	printf("\nEmotion analysis of WAV file %s:\n", fileName);
	VokaturiWavFile wavFile;
	VokaturiWavFile_open(fileName, &wavFile);
	if (!VokaturiWavFile_valid(&wavFile)) {
		fprintf(stderr, "Error: WAV file not analyzed.\n");
		exit(1);
	}
	static VokaturiVoice theVoice;
	if (!theVoice) {
		theVoice = VokaturiVoice_create(44100.0, numberOfSamples * 3);
		if (!theVoice)
			return;
	}
	//On note quelques données
	theSharedBuffer.duration = wavFile.numberOfSamples / wavFile.samplingFrequency;
	theSharedBuffer.samplingFrequency = wavFile.samplingFrequency;
	theSharedBuffer.nbOfSamples = wavFile.numberOfSamples;
	theSharedBuffer.numberOfSentSamples = 0;
	fprintf(stderr, "Nb of channel : %d \n", wavFile.numberOfChannels);
	fprintf(f, "duration : %f sec \n", theSharedBuffer.duration);
	fprintf(f, "Number of samples : %d \n", theSharedBuffer.nbOfSamples);
	fprintf(f, "sampling frequency : %f \n", theSharedBuffer.samplingFrequency);
	fprintf(stderr, "Number of samples : %d \n", wavFile.numberOfSamples);

	//On initialise des compteurs
	int startSample = 0;
	double sec;
	int32_t samplePointer = startSample % SHARED_BUFFER_SIZE;

	//On simule le streaming recording
	while (startSample < wavFile.numberOfSamples) {

		//On lock le buffer pour écrire dedans

		//fprintf(stderr, "rendercallback trying to lock foo \n");
		foo.lock();
		//fprintf(stderr, "rendercallback locked foo \n");
		sec = (theSharedBuffer.numberOfReceivedSamples / wavFile.samplingFrequency);
		theSharedBuffer.currentTime = sec;
		fprintf(f, "Sample n_ %d  / %d  sent ~ %f sec\n", startSample, wavFile.numberOfSamples, sec);
		for (int i = 0; i < wavFile.numberOfSamples; i+=numberOfSamples) {
			if (samplePointer >= SHARED_BUFFER_SIZE)
				samplePointer -= SHARED_BUFFER_SIZE;
			VokaturiWavFile_fillVoice(&wavFile, theVoice,0, startSample, numberOfSamples);
			samplePointer += sizeof(double);
			startSample += numberOfSamples;
			
		}
		/*VokaturiWavFile_fillVoice(&wavFile, theVoice,0, startSample, wavFile.numberOfSamples);
		*/
		static VokaturiQuality quality;
		static VokaturiEmotionProbabilities emotionProbabilities;
		VokaturiVoice_extract(theVoice, &quality, &emotionProbabilities);
		fprintf(stderr, "Sample n_ %d  received \n", theSharedBuffer.numberOfReceivedSamples);
		fprintf(stderr, "time : %f ~ %d:%d \n", theSharedBuffer.currentTime, (int)floor(theSharedBuffer.currentTime / 60), (int)(theSharedBuffer.currentTime) % 60);
		fprintf(f, "Sample n_ %d  received \n", theSharedBuffer.numberOfReceivedSamples);
		fprintf(f, "time : %f ~ %d:%d \n", theSharedBuffer.currentTime, (int)floor(theSharedBuffer.currentTime / 60), (int)(theSharedBuffer.currentTime) % 60);
		if (quality.valid) {
			fprintf(f2, "Sample n_ %d  received \n", theSharedBuffer.numberOfReceivedSamples);
			fprintf(f2, "time : %f ~ %d:%d \n", theSharedBuffer.currentTime, (int)floor(theSharedBuffer.currentTime / 60), (int)(theSharedBuffer.currentTime) % 60);
			fprintf(f2, "Neutrality %.3f\n", emotionProbabilities.neutrality);
			fprintf(f2, "Happiness %.3f\n", emotionProbabilities.happiness);
			fprintf(f2, "Sadness %.3f\n", emotionProbabilities.sadness);
			fprintf(f2, "Anger %.3f\n", emotionProbabilities.anger);
			fprintf(f2, "Fear %.3f\n", emotionProbabilities.fear);

			printf("Neutrality %.3f\n", emotionProbabilities.neutrality);
			printf("Happiness %.3f\n", emotionProbabilities.happiness);
			printf("Sadness %.3f\n", emotionProbabilities.sadness);
			printf("Anger %.3f\n", emotionProbabilities.anger);
			printf("Fear %.3f\n", emotionProbabilities.fear);
		}
		else {
			printf("Not enough sonorancy to determine emotions\n");
		}
		//VokaturiVoice_reset(theVoice);
	

		/*VokaturiWavFile_fillSamples(&wavFile, 0, startSample, numberOfSamples, theSharedBuffer.samples+samplePointer);
		samplePointer += numberOfSamples * sizeof(double);
		startSample += numberOfSamples;*/
		theSharedBuffer.numberOfReceivedSamples += numberOfSamples;
		//fprintf(stderr, "rendercallback unlock foo \n");
		foo.unlock();

		//On lock ce verrou pour ne pas retenter immediatement d'envoyer

		//fprintf(stderr, "rendercallback trying to lock bar \n");
		bar.lock();
		/*fprintf(stderr, "rendercallback locked bar \n");
		fprintf(stderr, "rendercallback unlock bar \n");*/
		bar.unlock();

		//On delock dés qu'onla reçu, car on est sûr que l'analyseur a pris la relève
	}
	theSharedBuffer.end = true;
	return;
}


//envoi les datas
static void renderCallBack(const char* fileName) {
	std::chrono::milliseconds timespan( numberOfSeconds*1000+numberOfMilliseconds);
	//On ouvre les fichiers logs
	errno_t err;
	FILE *f;
	err = fopen_s(&f, "renderCallBack_log.txt", "w+");
	if (err == 0)
	{
		printf("The file 'renderCallBack_log.txt' was opened\n");
	}
	else
	{
		printf("The file 'renderCallBack_log.txt' was not opened\n");
	}

	//On ouvre le fichier Wav
	printf("\nEmotion analysis of WAV file %s:\n", fileName);
	VokaturiWavFile wavFile;
	VokaturiWavFile_open(fileName, &wavFile);
	if (!VokaturiWavFile_valid(&wavFile)) {
		fprintf(stderr, "Error: WAV file not analyzed.\n");
		exit(1);
	}

	//On note quelques données
	theSharedBuffer.duration = wavFile.numberOfSamples / wavFile.samplingFrequency;
	theSharedBuffer.samplingFrequency = wavFile.samplingFrequency;
	theSharedBuffer.nbOfSamples = wavFile.numberOfSamples;
	theSharedBuffer.numberOfSentSamples = 0;
	fprintf(stderr, "Nb of channel : %d \n", wavFile.numberOfChannels);
	fprintf(f, "duration : %f sec \n", theSharedBuffer.duration);
	fprintf(f, "Number of samples : %d \n", theSharedBuffer.nbOfSamples);
	fprintf(f, "sampling frequency : %f \n", theSharedBuffer.samplingFrequency);
	fprintf(stderr, "Number of samples : %d \n", wavFile.numberOfSamples);

	//On initialise des compteurs
	int startSample = 0; 
	double sec;
	int32_t samplePointer = startSample % SHARED_BUFFER_SIZE;	

	//On simule le streaming recording
	while (startSample < wavFile.numberOfSamples) {

		//On lock le buffer pour écrire dedans

		//fprintf(stderr, "rendercallback trying to lock foo \n");
		foo.lock();
		std::this_thread::sleep_for(timespan);
		//fprintf(stderr, "rendercallback locked foo \n");
		sec = (theSharedBuffer.numberOfReceivedSamples / wavFile.samplingFrequency);
		theSharedBuffer.currentTime = sec;
		fprintf(f, "Sample n_ %d  / %d  sent ~ %f sec\n", startSample, wavFile.numberOfSamples, sec);
		/*for (int i = 0; i < numberOfSamples; i+=numberOfSamples) {
			if (samplePointer >= SHARED_BUFFER_SIZE)
				samplePointer -= SHARED_BUFFER_SIZE;
			VokaturiWavFile_fillSamples(&wavFile, 0, startSample, numberOfSamples, theSharedBuffer.samples + samplePointer*sizeof(double));
			samplePointer +=  numberOfSamples;
			startSample += numberOfSamples;
		}*/

		if ((samplePointer + numberOfSamples) >= SHARED_BUFFER_SIZE)
			samplePointer = 0;
		theSharedBuffer.previousPointer = samplePointer;
		VokaturiWavFile_fillSamples(&wavFile, 0, startSample, numberOfSamples, &theSharedBuffer.samples[samplePointer]);
		samplePointer += numberOfSamples;
		startSample += numberOfSamples;

			
		/*VokaturiWavFile_fillSamples(&wavFile, 0, startSample, numberOfSamples, theSharedBuffer.samples+samplePointer);
		samplePointer += numberOfSamples * sizeof(double);
		startSample += numberOfSamples;*/
		theSharedBuffer.numberOfReceivedSamples += numberOfSamples;
		//fprintf(stderr, "rendercallback unlock foo \n");
		foo.unlock();
		
		//On lock ce verrou pour ne pas retenter immediatement d'envoyer

		//fprintf(stderr, "rendercallback trying to lock bar \n");
		bar.lock();
		/*fprintf(stderr, "rendercallback locked bar \n");
		fprintf(stderr, "rendercallback unlock bar \n");*/
		bar.unlock();
		
		//On delock dés qu'onla reçu, car on est sûr que l'analyseur a pris la relève
	}
	theSharedBuffer.end = true;
	return;

}


//recoit les datas
static void timerCallBack() {
	errno_t err, err2;
	FILE *f , *f2;
	err = fopen_s(&f, "timerCallBack_log.txt", "w+");
	if (err == 0)
	{
		printf("The file 'timerCallBack_log.txt' was opened\n");
	}
	else
	{
		printf("The file 'timerCallBack_log.txt' was not opened\n");
	}

	err2 = fopen_s(&f2, "timerCallBack_successlog.txt", "w+");
	if (err == 0)
	{
		printf("The file 'timerCallBack_successlog.txt' was opened\n");
	}
	else
	{
		printf("The file 'timerCallBack_successlog.txt' was not opened\n");
	}

	static VokaturiVoice theVoice;
	if (!theVoice) {
		theVoice = VokaturiVoice_create(44100.0, numberOfSamples*3);
		if (!theVoice)
			return;
	}
	
	while (true) {
		//On lock le buffer pour pouvoir lire dedans

		//fprintf(stderr, "timercallback trying to lock bar \n");
		bar.lock();
		/*fprintf(stderr, "timercallback locked bar \n");
		fprintf(stderr, "timercallback trying to lock foo \n");*/
		foo.lock();		
		//fprintf(stderr, "timercallback locked foo \n");
		if (theSharedBuffer.numberOfReceivedSamples > theSharedBuffer.numberOfSentSamples) {
			
			//On extrait les samples
			for (int64_t i = theSharedBuffer.numberOfSentSamples;
				i < theSharedBuffer.numberOfReceivedSamples; i+=numberOfSamples)
			{
				//int32_t indexOfSampleToBeSent = (int32_t)(i % SHARED_BUFFER_SIZE);				
				VokaturiVoice_fill(theVoice, numberOfSamples,
					&theSharedBuffer.samples[theSharedBuffer.previousPointer]);
			}			
			theSharedBuffer.numberOfSentSamples = theSharedBuffer.numberOfReceivedSamples;


			//On delock le buffer
			//fprintf(stderr, "timercallback unlock bar \n");
			bar.unlock();
			//fprintf(stderr, "timercallback unlock foo \n");
			foo.unlock();

			//analyse des samples reçus
			
			static VokaturiQuality quality;
			static VokaturiEmotionProbabilities emotionProbabilities;
			VokaturiVoice_extract(theVoice, &quality, &emotionProbabilities);			
			fprintf(stderr, "Sample n_ %d  received \n", theSharedBuffer.numberOfReceivedSamples);
			fprintf(stderr, "time : %f ~ %d:%d \n", theSharedBuffer.currentTime, (int)floor(theSharedBuffer.currentTime / 60), (int)(theSharedBuffer.currentTime) % 60);
			fprintf(f, "Sample n_ %d  received \n", theSharedBuffer.numberOfReceivedSamples);
			fprintf(f, "time : %f ~ %d:%d \n", theSharedBuffer.currentTime, (int)floor(theSharedBuffer.currentTime / 60), (int)(theSharedBuffer.currentTime) % 60);
			if (quality.valid) {
				fprintf(f2, "Sample n_ %d  received \n", theSharedBuffer.numberOfReceivedSamples);
				fprintf(f2, "time : %f ~ %d:%d \n", theSharedBuffer.currentTime, (int)floor(theSharedBuffer.currentTime / 60), (int)(theSharedBuffer.currentTime) % 60);
				fprintf(f2,"Neutrality %.3f\n", emotionProbabilities.neutrality);
				fprintf(f2,"Happiness %.3f\n", emotionProbabilities.happiness);
				fprintf(f2,"Sadness %.3f\n", emotionProbabilities.sadness);
				fprintf(f2,"Anger %.3f\n", emotionProbabilities.anger);
				fprintf(f2,"Fear %.3f\n", emotionProbabilities.fear);

				printf("Neutrality %.3f\n", emotionProbabilities.neutrality);
				printf("Happiness %.3f\n", emotionProbabilities.happiness);
				printf("Sadness %.3f\n", emotionProbabilities.sadness);
				printf("Anger %.3f\n", emotionProbabilities.anger);
				printf("Fear %.3f\n", emotionProbabilities.fear);

				emotions[0] = emotionProbabilities.neutrality;
				emotions[1] = emotionProbabilities.happiness;
				emotions[2] = emotionProbabilities.sadness;
				emotions[3] = emotionProbabilities.anger;
				emotions[4] = emotionProbabilities.fear;

				receivedSamplesForUnity = theSharedBuffer.numberOfSentSamples;
			}
			else {
				printf("Not enough sonorancy to determine emotions\n");
			}
			//VokaturiVoice_reset(theVoice);
		}
		else {
			//On delock le buffer
			//fprintf(stderr, "timercallback unlock bar \n");
			bar.unlock();
			//fprintf(stderr, "timercallback unlock foo \n");
			foo.unlock();
		}

		////On delock le buffer
		////fprintf(stderr, "timercallback unlock bar \n");
		//bar.unlock();
		////fprintf(stderr, "timercallback unlock foo \n");
		//foo.unlock();
		
		if (theSharedBuffer.end) {
			return;
		}
		
	}

	return;

}


static void test(const char*fileName, int seconde, int milliseconds) {
	numberOfSeconds *= seconde;
	numberOfMilliseconds *= milliseconds;
	numberOfSamples = (int)(numberOfSamples*((float)seconde + (float)numberOfMilliseconds / 1000));
	std::thread rendercallback(renderCallBack, fileName);
	std::thread timercallback(timerCallBack);

	// synchronize threads:
	rendercallback.join();                // pauses until first finishes
	timercallback.join();               // pauses until second finishes
										//analyzeWithoutBuffer(fileName);

	return;
}

//Pour faire des test avec un .exe;
int main(int argc, const char * argv[])
{
	if (argc < 2) {
		printf("Usage: MeasureWav [soundfilename.wav ...]\n");
		exit(1);
	}
	printf("**********\nWAV files analyzed with:\n%s\n**********\n",
		Vokaturi_versionAndLicense());
	//for (int ifile = 1; ifile < argc; ifile++) {
		const char *fileName = argv[1];

		int seconde = atoi(argv[2]);
		numberOfSeconds *= seconde;
		
		if (argc == 4) {
			numberOfMilliseconds *= atoi(argv[3]);
		}
		else {
			numberOfMilliseconds = 0;
		}

		numberOfSamples = (int)(numberOfSamples*((float)seconde+(float)numberOfMilliseconds/1000));
		fprintf(stderr, "numberOfSamples : %d, argc %d \n", numberOfSamples, numberOfMilliseconds);
		std::thread rendercallback(renderCallBack, fileName);
		std::thread timercallback(timerCallBack);

		// synchronize threads:
		rendercallback.join();                // pauses until first finishes
		timercallback.join();               // pauses until second finishes
		//analyzeWithoutBuffer(fileName);

		return 0;
		
	
}

