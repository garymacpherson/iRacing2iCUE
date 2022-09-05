#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <Windows.h>

#include "LightingManager.h"
#include "iCUEHandler.h"

class iCUEHandler {
	private:
		LightingManager* mLightManager;
		EC ec;
	public:
		iCUEHandler() {
			mLightManager = new LightingManager();
			ec = mLightManager->init();
			if (ec == EC::NotOK) {
				printf("Unable to init LightingManager\n");
			}
		}

		void updateiCUE(char input)
		{
			if (ec == EC::NotOK) {
				printf("Unable to handle request\n");
			}
			else {
				switch (input) {
				case 'g': mLightManager->greenFlag();
					break;
				case 'b': mLightManager->blueFlag();
					break;
				case 'r': mLightManager->redFlag();
					break;
				case 'y': mLightManager->yellowFlag();
					break;
				case 'w': mLightManager->whiteFlag();
					break;
				case 'k': mLightManager->blackFlag();
				default:
					break;
				}
			}
		}
};