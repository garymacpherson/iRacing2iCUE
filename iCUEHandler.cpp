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
					std::cout << "Green" << std::endl;
					break;
				case 'b': mLightManager->blueFlag();
					std::cout << "Blue" << std::endl;
					break;
				case 'r': mLightManager->redFlag();
					std::cout << "red" << std::endl;
					break;
				case 'y': mLightManager->yellowFlag();
					std::cout << "yellow" << std::endl;
					break;
				case 'w': mLightManager->whiteFlag();
					std::cout << "white" << std::endl;
					break;
				case 'k': mLightManager->blackFlag();
					std::cout << "black" << std::endl;
				default:
					break;
				}
			}
		}
};