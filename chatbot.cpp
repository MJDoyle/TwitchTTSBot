#include "chatbot.hpp"

//Constructor
Chatbot::Chatbot(std::string passcode, std::string botName, std::string channelName) : _passcode(passcode), _botName(botName), _channelName(channelName)
{
	_connected = false;

	_socket = std::shared_ptr<sf::TcpSocket>(new sf::TcpSocket());

	_voice = NULL;

	//Initialize the windows COM library
	if (FAILED(::CoInitialize(NULL)))
		std::cout << "COM not initialized" << std::endl;

	//Intialize the voice
	if (FAILED(CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&_voice)))
		std::cout << "Voice not initialized" << std::endl;

	LoadSoundBuffers();

}

//Connect to twitch
int Chatbot::Connect()
{
	//Try to connect
	if (_socket->connect("irc.chat.twitch.tv", 6667) != sf::Socket::Done)
	{
		std::cout << "[Client] Failed to connect to server!\n";

		return CHATBOT_CONNECT_FAILURE;
	}

	//Send all required details
	Send("PASS " + _passcode + "\r\n");

	Send("NICK " + _botName + "\r\n");	//NOTE: The NICK IRC command is no-op, however needs to be sent

	Send("JOIN #" + _channelName + "\r\n");

	Send("CAP REQ :twitch.tv/membership\r\n");

	Send("CAP REQ :twitch.tv/tags\r\n");

	Send("CAP REQ :twitch.tv/commands\r\n");



	sf::Clock timeoutClock;

	//wait for confirmation of connection
	while (!_connected && timeoutClock.getElapsedTime().asMilliseconds() < 3000)
	{
		int botReturn = InitialReceive();

		if (botReturn == CHATBOT_AUTH_FAILURE)
			return CHATBOT_AUTH_FAILURE;
	}

	//If no connection is made, return error message
	if (!_connected)
		return CHATBOT_NO_RESPONSE;

	//Set socket as non blocking in preparation for main loop
	_socket->setBlocking(false);

	std::cout << "Starting up" << std::endl;

	//Send startup message to channel
	Send("PRIVMSG #" + _channelName + " :TTSBot starting up\r\n");

	return CHATBOT_SUCCESS;
}

//Intial receive of connection confirmation
int Chatbot::InitialReceive()
{
	char data[1024];
	std::size_t received;

	//If there is data received
	if (_socket->receive(data, sizeof(data), received) == sf::Socket::Done)
	{
		const std::string response(data, received);

		std::cout << std::endl << response << std::endl;

		//If the response is login failure, return error message
		if (response.find("Login authentication failed") != std::string::npos)
		{
			std::cout << "FAIL" << std::endl;

			return CHATBOT_AUTH_FAILURE;
		}

		//If the correct substring is found, connection is succesful
		else if (response.find("twitch.tv/commands") != std::string::npos)
		{
			_connected = true;

			return CHATBOT_SUCCESS;
		}
	}

	return -1;
}

//Main loop receive function
void Chatbot::Receive()
{
	char data[1024];
	std::size_t received;

	//If data is received
	if (_socket->receive(data, sizeof(data), received) == sf::Socket::Done)
	{
		_readBuffer.append(data, received);

		const std::string response(data, received);

		//Respond directly to ping
		if (response == "PING :tmi.twitch.tv\r\n")
		{
			std::cout << "PING" << std::endl;

			Send("PONG :tmi.twitch.tv\r\n");
		}
	}
}

//Parse read buffer
void Chatbot::ParseReadBuffer()
{
	///////////////////////
	//	Go through buffer, pull out one message at a time and deal with it, then delete message from buffer
	///////////////////////

	bool allMessagesHandled = false;

	while (!allMessagesHandled)
	{
		auto lastMessageChar = _readBuffer.begin();

		//Find the linefeed to signify end of message

		for (auto c = _readBuffer.begin(); c != _readBuffer.end(); c++)
		{
			if (int(*c == 10))	//Line feed
			{
				lastMessageChar = c;

				break;
			}
		}

		///////////////////////
		//	If no LINE FEED is found, or there are no chars left in the message, messsage handling is complete
		///////////////////////

		if (lastMessageChar == _readBuffer.end() || lastMessageChar == _readBuffer.begin())
			allMessagesHandled = true;


		///////////////////////
		//	Else, process the message
		///////////////////////

		else
		{

			///////////////////////
			//	Create the message string
			///////////////////////

			std::string message;

			for (auto c = _readBuffer.begin(); c != lastMessageChar + 1; c++)
			{
				message.push_back(*c);
			}

			///////////////////////
			//	Work out what kind of message it is and handle appropriately (only handle PRIVMSG for now)
			///////////////////////

			if (message.find("PRIVMSG") != std::string::npos)
			{
				///////////////////////
				//	Private message - find the username
				///////////////////////

				size_t nameChar = message.find("display-name=") + 13;

				std::string username;

				while (message[nameChar] != ';')
				{
					username.push_back(message[nameChar]);

					nameChar++;
				}

				std::cout << username << ": ";

				///////////////////////
				//	Now handle the content of the message
				///////////////////////

				//Find comment start 
				size_t commentStart = message.find("PRIVMSG");

				while (message[commentStart] != ':')
				{
					commentStart++;
				}

				commentStart++;

				std::string comment;

				for (size_t c = commentStart; c != message.size(); c++)
				{
					comment.push_back(message[c]);

					std::cout << message[c];
				}

				//Add comment to message list to handle later
				_messages.push_back(std::shared_ptr<Message>(new Message()));

				_messages.back()->message = comment;
				_messages.back()->username = username;
				_messages.back()->speak = false;

				///////////////////////
				//	Check if there is a TTS reward and speak message if so (this will speak all custom rewards)
				///////////////////////

				if (message.find("custom-reward-id") != std::string::npos)
					_messages.back()->speak = true;

			}

			//Delete the message up to the line feed from the buffer
			_readBuffer.erase(_readBuffer.begin(), lastMessageChar + 1);
		}
	}
}

//Handle parsed messages
void Chatbot::HandleMessages()
{
	while (_messages.size())
	{
		//Check message for SFX keywords

		for (auto keyword = _soundBuffers.begin(); keyword != _soundBuffers.end(); keyword++)
		{

			std::cout << "KEY: " << keyword->first << std::endl;

			std::cout << "NUM SOUNDS: " << keyword->second.size() << std::endl;


			if (_messages.back()->message.find(keyword->first) != std::string::npos)
			{
				//Create new sound, play it, and add it to list of sounds
				std::shared_ptr<sf::Sound> sound = std::shared_ptr<sf::Sound>(new sf::Sound);

				sound->setBuffer(*_soundBuffers[keyword->first][_soundBuffers[keyword->first].size() * RandFloat()]);


				sound->play();

				_sounds.push_back(sound);
			}
		}

		//Speak message if if must be spake
		if (_messages.back()->speak)
			Say(_messages.back()->message);


		//Remove message from queue
		_messages.pop_back();
	}
}

//Send function
void Chatbot::Send(std::string message)
{
	std::cout << "Sending " << message << std::endl;

	std::size_t sent;

	//Try to send the message
	if (_socket->send(message.data(), message.length(), sent) != sf::Socket::Done)
	{
		std::cout << "Send error" << std::endl;
	}
}

//Speak a string using windows TTS
void Chatbot::Say(std::string string)
{
	//Convert string to LPCWSTR
	int len;
	int slength = (int)string.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, string.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, string.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;

	//Speak the string
	_voice->Speak(r.c_str(), 0, NULL);
}

//Load the sound files from the sound.txt index into the sound buffer map
void Chatbot::LoadSoundBuffers()
{
	//Open the sound index file
	std::ifstream soundFile("sounds.txt");

	//Read in pairs of lines at a time. The first line contains the command used to invoke the sound(s), the second line contains a list of sounds that will be invoked by the command (one chosen randomly from the list)

	std::string commandLine;
	std::string soundsLine;

	//Get the first line
	while (std::getline(soundFile, commandLine))
	{
		//Try and get the second line
		if (std::getline(soundFile, soundsLine))
		{
			std::cout << "SOUND LINE: " << soundsLine << std::endl;

			std::vector<std::shared_ptr<sf::SoundBuffer>> buffers;

			//parse the second line into elements

			std::stringstream ss(soundsLine);

			while (ss.good())
			{
				std::string soundFilename;
				std::getline(ss, soundFilename, ',');

				//Try to load file with that filename

				std::shared_ptr<sf::SoundBuffer> buffer = std::shared_ptr<sf::SoundBuffer>(new sf::SoundBuffer());

				if (buffer->loadFromFile(soundFilename))
					buffers.push_back(buffer);

				else
				{
					std::cout << "Couldn't load " << soundFilename << std::endl;

					break;
				}

			}

			_soundBuffers[commandLine] = buffers;
		}

		else
		{
			std::cout << "Couldn't find files for  " << commandLine << std::endl;

			break;
		}
	}

	soundFile.close();
}

//Handle anything else that needs handling
void Chatbot::Update()
{
	//Remove sounds if they are no longer playing
	for (auto sound = _sounds.begin(); sound != _sounds.end(); )
	{
		if ((*sound)->getStatus() != sf::Sound::Playing)
			sound = _sounds.erase(sound);

		else
			sound++;
	}
}