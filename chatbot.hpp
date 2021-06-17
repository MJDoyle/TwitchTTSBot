#pragma once

#include "config.hpp"

class Chatbot
{
public:

	//Constructor
	Chatbot(std::string passcode, std::string botName, std::string channelName);

	//Connect to twitch
	int Connect();

	//Loko for initial confirmation that connection has been made
	int InitialReceive();

	//Main loop receive function
	void Receive();

	//Send a message
	void Send(std::string message);

	//Speak a string using windows text to speech
	void Say(std::string message);

private:

	//TCP socket for connection with twitch
	std::shared_ptr<sf::TcpSocket> _socket;

	//Connection state
	bool _connected;

	//Microsoft voice interface
	ISpVoice* _voice;

	//Bot OAUTH code
	std::string _passcode;

	//Bot name
	std::string _botName;

	//Name of twitch channel that the bot should connect to
	std::string _channelName;
};