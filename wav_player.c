#include <stdio.h>
#include <alsa/asoundlib.h>

#define PCM_DEVICE "default"

int main(int argc, char *argv[])
{
	unsigned int result, temp;
	snd_pcm_t *pcm_handle;
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_uframes_t period;

	int rate, channels, seconds;
	int buff_size, loops;
	char *buff;

	rate = 44100;
	channels = 1;
	seconds = 3;

	//Open PCM_DEVICE for playback
	result = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
	if (result < 0) {
		printf("Cannot open PCM Device: %s\n", snd_strerror(result));
		exit(1);
	}


	//Allocate Hardware Parameters Object
	snd_pcm_hw_params_alloca(&hw_params);

	snd_pcm_hw_params_any(pcm_handle, hw_params);

	//Set Parameters
	result = snd_pcm_hw_params_set_access(pcm_handle, hw_params,
						SND_PCM_ACCESS_RW_INTERLEAVED);
	if (result < 0) {
		printf("Unable to set interleaved mode: %s\n", snd_strerror(result));
		exit(1);
	}

	result = snd_pcm_hw_params_set_format(pcm_handle, hw_params,
						SND_PCM_FORMAT_S16_LE);
	if(result < 0) {
		printf("Cannot set format: %s\n", snd_strerror(result));
		exit(0);
	}


	result = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, channels);
	if (result < 0) {
		printf("Cannot set channels: %s\n", snd_strerror(result));
		exit(1);
	}


	result = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &rate, 0);
	if (result < 0) {
		printf("Unable to set rate: %s\n", snd_strerror(result));
		exit(1);
	}

	//Write Params
	result = snd_pcm_hw_params(pcm_handle, hw_params);
	if (result < 0) {
		printf("Cannot write hardware params: %s\n", snd_strerror(result));
		exit(1);
	}

	//Get Indentifier of PCM Handle
	printf("PCM name: '%s'\n", snd_pcm_name(pcm_handle));

	printf("PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(pcm_handle)));

	snd_pcm_hw_params_get_channels(hw_params, &temp);
	printf("Channels: %i \n", temp);


	snd_pcm_hw_params_get_rate(hw_params, &temp, 0);
	printf("Rate: %d bps\n", temp);
	printf("Seconds: %d\n", seconds);

	//Extract period size from a configuration space. 
	result = snd_pcm_hw_params_get_period_size(hw_params, &period, 0);
	if(result < 0) {
		printf("Cannot extract period size: %s\n", snd_strerror(result));
		exit(1);
	}

	buff_size = period * channels * 2;
	buff = (char *) malloc(buff_size);

	snd_pcm_hw_params_get_period_time(hw_params, &temp, NULL);

	system("amixer sset PCM,0 100%");
	for (loops = (seconds * 1000000) / temp; loops > 0; loops--) {
		result = read(0, buff, buff_size);
		if (result == 0) {
			printf("Early EOF\n");
			return 0;
		}

		result = snd_pcm_writei(pcm_handle, buff, period);
		if(result == -EPIPE) {
			printf("XRUN occured\n");
			snd_pcm_prepare(pcm_handle);
		} else if (result < 0) {
			printf("Cannot write to PCM Device: %s\n", snd_strerror(result));
		}
	}


	snd_pcm_drain(pcm_handle);
	snd_pcm_close(pcm_handle);
	free(buff);
	return 0;
}
