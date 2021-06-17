// TTSBot.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "chatbot.hpp"

int main()
{
    //Load the bot settings
    std::ifstream configFile("config.txt");

    std::string pass;

    std::string join;

    std::getline(configFile, pass);

    std::getline(configFile, join);

    configFile.close();

    //Set up the bot
    std::shared_ptr<Chatbot> chatbot = std::shared_ptr<Chatbot>(new Chatbot(pass, join, join));

    //Try to connect the bot
    if (chatbot->Connect() != CHATBOT_SUCCESS)
        return 0;

    //Run the main loop
    while (true)
    {
        //Sleep to reduce CPU load
        Sleep(10);

        chatbot->Receive();
    }

    return 0;
}