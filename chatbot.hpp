#pragma once

#include "config.hpp"

struct Message
{
	std::string message;

	std::string username;

	bool speak;
};

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

	//Parse read buffer
	void ParseReadBuffer();

	//Handle parsed messages
	void HandleMessages();

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

	//String buffer for incoming messages from twitch server
	std::string _readBuffer;

	//Vector of messages
	std::vector<std::shared_ptr<Message>> _messages;

	//SFX

	std::map<std::string, std::vector<std::shared_ptr<sf::SoundBuffer>>> _soundBuffers;

	void LoadSoundBuffers();
};