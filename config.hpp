#pragma once

#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <random>
#include <list>

#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <SFML/Audio.hpp>

#include <Windows.h>
#include <Dwmapi.h>

#include <stdio.h>

#include <sapi.h>

//bot error codes
const enum BOT_RETURNS { CHATBOT_SUCCESS, CHATBOT_AUTH_FAILURE, CHATBOT_CONNECT_FAILURE, CHATBOT_NO_RESPONSE, NUM_BOT_RETURNS };