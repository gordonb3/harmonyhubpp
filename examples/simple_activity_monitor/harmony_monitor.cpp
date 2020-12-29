/******************************************************************************
    Copyright (C) 2019  Gordon Bos

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <harmonyhubclient.hpp>
#include <boost/bind/bind.hpp>
#include <thread>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>
#include <jsoncpp/json.h>

#ifndef _WIN32
#define sprintf_s(buffer, buffer_size, stringbuffer, ...) (sprintf(buffer, stringbuffer, __VA_ARGS__))
#endif


namespace harmonyhubpp {
namespace activity {
namespace status {
	bool isChanging(const value activitystatus)
	{
		return (static_cast<int>(activitystatus) & 1);
	}
}; // namespace status
}; // namespace activity

namespace connection {
namespace status {
	enum value {
		connecting,
		connected,
		closing,
		closed
	};
}; // namespace status
}; // namespace connection
}; // namespace harmonyhubpp


bool stoprequested;
harmonyhubpp::HarmonyClient Client;
harmonyhubpp::connection::status::value myState;
bool checktimeout;

std::string datetime()
{
	time_t now=time(0);
	struct tm ltime;
	localtime_r(&now, &ltime);

	char szTime[20];
	sprintf_s(szTime, 19, "%04d-%02d-%02d %02d:%02d:%02d", ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec);
	return std::string(szTime, 19);
}

void handle_notification(const Json::Value &j_data)
{
	std::string msgtype = j_data["type"].asString();
	size_t loc = msgtype.find_last_of('.');
	std::string shorttype = msgtype.substr(loc+1);

	if (shorttype == "stateDigest?notify")
	{
		int activityStatus = j_data["data"]["activityStatus"].asInt();

		harmonyhubpp::activity::status::value hubstate = static_cast<harmonyhubpp::activity::status::value>(activityStatus);
		if (harmonyhubpp::activity::status::isChanging(hubstate))
			std::cout << datetime() << " hub is changing activity\n";

	}
	else if ((shorttype == "engine?startActivityFinished") || (shorttype == "engine?helpdiscretes"))
	{
		std::string activityId = j_data["data"]["activityId"].asString();
		if (activityId == "-1")
			std::cout << datetime() << " hub has shut down all activities\n";
		else
			std::cout << datetime() << " hub is now running activity " << activityId << "\n";
	}
	else if (shorttype == "button?pressType")
	{
		std::cout << datetime() << " button pressed (type = " << j_data["data"]["type"].asString() << ")\n";
	}
	else
	{
		// unhandled
		std::cout << datetime() << " unhandled notification:\n";
		std::cout << j_data.toStyledString() << std::endl;
	}
}

void parse_message(const std::string szdata)
{
	Json::Value j_result;
	Json::Reader j_reader;

	bool ret = j_reader.parse(szdata.c_str(), j_result);

	if ((!ret) || (!j_result.isObject()))
	{
		std::cout << datetime() << " Invalid data received\n";
		return;
	}

	if (!j_result.isMember("code"))
	{
		// notification
		handle_notification(j_result);
		return;
	}

	checktimeout = false;

	int returncode = atoi(j_result["code"].asString().c_str());

	if (myState == harmonyhubpp::connection::status::connecting)
	{
		if (returncode == 10000)
		{
			myState = harmonyhubpp::connection::status::connected;
			std::cout << datetime() << " connected to hub\n";
			return;
		}
		else if (returncode > 10000)
		{
			myState = harmonyhubpp::connection::status::closed;
			std::cout << datetime() << " connect failed with error " << returncode - 10000 <<  ": " << j_result["msg"].asString() << "\n";
			return;
		}
	}
	if ((myState == harmonyhubpp::connection::status::closing) && (returncode == 1000))
	{
		myState = harmonyhubpp::connection::status::closed;
		std::cout << datetime() << " connection closed\n";
		return;
	}

	if (returncode >= 10000)
	{
		// HTTP error
		std::cout << datetime() << " HTTP error " << returncode - 10000 <<  ": " << j_result["msg"].asString() << "\n";
		return;
	}

	if (returncode >= 1000)
	{
		// websocket connection ended
		myState = harmonyhubpp::connection::status::closed;
		std::cout << datetime() << " connection closed unexpectedly (error = " << returncode <<  ")\n";
		return;
	}


	std::string cmd;
	if (j_result.isMember("cmd"))
		cmd = j_result["cmd"].asString();

	if (cmd == "connect.ping")
	{
		if (returncode == 200)
		{
			// don't print this
			return;
		}
		std::cout << datetime() << " unexpected websocket error (error = " << returncode <<  ")\n";
		return;
	}

	// unhandled
	std::cout << datetime() << " unhandled message:\n";
	std::cout << datetime() << " " << j_result.toStyledString() << std::endl;
}


void keep_alive()
{
	std::cout << datetime() << " starting keep alive thread\n";
	int timer = 0;
	while (!stoprequested)
	{
		if (checktimeout)
		{
			sleep(1);
			if (checktimeout)
				parse_message("{\"code\":10028,\"msg\":\"operation timeout\"}");
		}
		else
		{
			sleep(1);
		}
		if (myState == harmonyhubpp::connection::status::connected)
		{
			timer++;
			if (timer == 40)
			{
				checktimeout = true;
				Client.ping();
				timer = 0;
			}
		}
	}
	std::cout << datetime() << " keep alive thread stopped\n";
}

int main(int argc, char * argv[])
{
	bool done = false;
	std::string input;
	myState = harmonyhubpp::connection::status::closed;

	stoprequested = false;
	checktimeout = false;
	std::shared_ptr<std::thread> m_thread;
	m_thread = std::make_shared<std::thread>(&keep_alive);
	sleep(1);
	Client.set_message_handler(boost::bind(&parse_message, boost::placeholders::_1));

	if (argc > 1)
	{
		myState = harmonyhubpp::connection::status::connecting;
		Client.connect(argv[1]);
		sleep(1);
	}


	while (!done) {
		std::cout << "Enter Command: ";
		std::getline(std::cin, input);

		if (input.empty())
			continue;

		if (input == "quit") {
			done = true;
			stoprequested = true;
			sleep(2);
			if (m_thread)
				m_thread->join();

		} else if (input == "help") {
			std::cout
				<< "\nCommand List:\n"
				<< "connect <IP address>\n"
				<< "close [<close code:default=1000>] [<close reason>]\n"
				<< "help: Display this help text\n"
				<< "quit: Exit the program\n"
				<< std::endl;
		} else if (input.substr(0,7) == "connect") {
			if (myState == harmonyhubpp::connection::status::connected)
				std::cout << "> Already connected" << std::endl;
			else
			{
				myState = harmonyhubpp::connection::status::connecting;
				checktimeout = true;
				Client.connect(input.substr(8));
			}
		} else if (input.substr(0,5) == "close") {
			if (myState == harmonyhubpp::connection::status::closed)
				std::cout << "> Already closed" << std::endl;
			else
			{
				std::stringstream ss(input);

				std::string cmd;
				int id;
				std::string reason;

				ss >> cmd >> id;
				std::getline(ss,reason);

				myState = harmonyhubpp::connection::status::closing;
				checktimeout = true;
				Client.close(reason);
			}
		} else {
			std::cout << "> Unrecognized Command" << std::endl;
		}
		sleep(1);
	}

	return 0;
}

