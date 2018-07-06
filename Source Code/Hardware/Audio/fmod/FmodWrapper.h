#ifndef _WRAPPER_FMOD_H_
#define _WRAPPER_FMOD_H_

/*****************************************************************************************/
/*				ATENTION! FMOD is not free for commercial use!!!						 */
/*	Do not include it in commercial products unless you own a license for it.            */
/*	Visit: http://www.fmod.org/index.php/sales  for more details -Ionut					 */
/*****************************************************************************************/

#include "../AudioAPIWrapper.h"
#include "Utility/Headers/Singleton.h"

SINGLETON_BEGIN_EXT1(FMOD_API,AudioAPIWrapper)

public:
	void initHardware(){}
	void closeAudioApi(){}
	void initDevice(){}

	void playSound(AudioDescriptor* sound){}
	void playMusic(AudioDescriptor* music){}

	void pauseMusic(){}
	void stopMusic(){}
	void stopAllSounds(){}

	void setMusicVolume(I8 value){}
	void setSoundVolume(I8 value){}
SINGLETON_END()

#endif