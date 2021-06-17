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
		const std::string response(data, received);

		std::cout << std::endl << response << std::endl;

		//twitch will occasionally PING the bot, if it does, return PONG to maintain connection
		if (response == "PING :tmi.twitch.tv\r\n")
		{
			std::cout << "PING" << std::endl;

			Send("PONG :tmi.twitch.tv\r\n");
		}

		//Else if the response contains substring indicating that custom reward has been redeemed, run the TTS
		//This does mean that other custom rewards will also trigger TTS
		else if (response.find("custom-reward-id=") != std::string::npos)
		{
			int i = response.size() - 1;

			bool colonFound = false;

			//Find the position of the last colon in the reponse, this indicates the start of the message to be spoken
			while (!colonFound)
			{
				i--;

				if (response[i] == ':')
					colonFound = true;
			}

			std::string speech;

			//Set up the string to be spoken
			for (int j = i; j < response.size(); j++)
			{
				speech.push_back(response[j]);
			}

			//Speak the string
			Say(speech);
		}
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