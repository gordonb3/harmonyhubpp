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

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>
#include <harmonyhubclient.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <boost/bind.hpp>


#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif


namespace harmonyhubcontrol {
namespace status {
	enum value {
		initializing,
		connecting,
		connected,
		waitforanswer,
		closing,
		closed
	};
}; // namespace status
}; // namespace harmonyhubcontrol


using namespace std;


harmonyhubcontrol::status::value myState = harmonyhubcontrol::status::initializing;
bool bQuietMode;
std::string szCommand;
std::vector<std::string> szCommandParameters;

void log(const std::string message, bool bQuiet)
{
	if (bQuiet)
	{
/*
		if (!hclient.GetErrorString().empty())
		{
			cout << "{\n   \"status\":\"error\",\n   \"message\":\"" << hclient.GetErrorString() << "\"\n}\n";
		}
		else if (message.find("FAILURE") != std::string::npos)
		{
			cout << "{\n   \"status\":\"error\",\n   \"message\":\"general failure\"\n}\n";
		}
*/
		return;
	}

	cout << message << endl;
}

int usage(int returnstate)
{
	const char *suffix = "";
#if WIN32
	suffix = ".exe";
#endif
	printf("Syntax:\n");
	printf("HarmonyHubControl%s [harmony_ip] [command]\n", suffix);
	printf("    where command can be any of the following:\n");
	printf("        list_activities\n");
	printf("        list_activities_raw\n");
	printf("        get_current_activity_id\n");
	printf("        get_current_activity_id_raw\n");
	printf("        start_activity <ID>\n");
	printf("        start_activity_raw <ID>\n");
	printf("        list_devices\n");
	printf("        list_devices_raw\n");
	printf("        list_device_commands <deviceId>\n");
	printf("        list_device_commands_raw <deviceId>\n");
	printf("        issue_device_command <deviceId> <command> [...]\n");
	printf("        issue_device_command_raw <deviceId> <command> [...]\n");
	printf("        get_config\n");
	printf("        get_config_raw\n");
	printf("\n");
	return returnstate;
}

void handle_reply(const std::string szdata)
{
	Json::Value jResult;
	Json::Reader jReader;

	bool ret = jReader.parse(szdata.c_str(), jResult);

	if ((!ret) || (!jResult.isObject()))
	{
		if (bQuietMode)
			std::cout << "{\n   \"status\":\"error\",\n   \"message\":\"Invalid data received\"\n}\n";
		else
			std::cout << "JSON PARSER ERROR              : Invalid data received\n";
		return;
	}


	int returncode = atoi(jResult["code"].asString().c_str());

	if (myState == harmonyhubcontrol::status::connecting)
	{
		if (returncode == 10000)
		{
			myState = harmonyhubcontrol::status::connected;
			return;
		}
		log("HARMONY COMMUNICATION LOGIN    : FAILURE", bQuietMode);
	}

	else if (myState == harmonyhubcontrol::status::closing)
	{
		if (returncode == 1000)
		{
			myState = harmonyhubcontrol::status::closed;
			return;
		}
	}

	else if (myState == harmonyhubcontrol::status::waitforanswer)
	{
		if (returncode == 100)
		{
			// command is in progress
			return;
		}

		if (returncode == 200)
		{
			// command was successful
			myState = harmonyhubcontrol::status::connected;

			std::string cmd = jResult["cmd"].asString();
			if (cmd == "harmony.engine?getCurrentActivity")
			{
				std::ofstream current("current.json", ios::trunc);
				std::string szResult = jResult["data"].toStyledString();
				current << szResult;
				current.close();
				std::cout << szResult;
				return;
			}

			else if (cmd == "harmony.engine?config")
			{
				std::ofstream config("config.json", ios::trunc);
				std::string szResult = jResult["data"].toStyledString();
				config << szResult;
				config.close();

				if (szCommand == "get_config")
				{
					std::cout << szResult;
					return;
				}

				Json::Value *jConfig = &jResult["data"];

				if (szCommand == "list_activities")
				{
					Json::Value jActivities;
					int l = static_cast<int>((*jConfig)["activity"].size());
					for (int i=0; i < l; i++)
					{
						jActivities[(*jConfig)["activity"][i]["label"].asString()] = (*jConfig)["activity"][i]["id"].asString();
					}
					std::ofstream fsActivities("activities.json", ios::trunc);
					std::string szActivities = jActivities.toStyledString();
					fsActivities << szActivities;
					fsActivities.close();
					std::cout << szActivities;
					return;
				}

				if (szCommand == "list_devices")
				{
					Json::Value jDevices;
					int l = static_cast<int>((*jConfig)["device"].size());
					for (int i=0; i < l; i++)
					{
						jDevices[(*jConfig)["device"][i]["label"].asString()] = (*jConfig)["device"][i]["id"].asString();
					}
					std::ofstream fsDevices("devices.json", ios::trunc);
					std::string szDevices = jDevices.toStyledString();
					fsDevices << szDevices;
					fsDevices.close();
					std::cout << szDevices;
					return;
				}


				if (szCommand == "list_device_commands")
				{
					Json::Value jCommands;
					Json::Value *jDevice;
					int l = static_cast<int>((*jConfig)["device"].size());
					int i = 0;
					while (i < l)
					{
						if ((*jConfig)["device"][i]["id"].asString() == szCommandParameters[0])
						{
							jDevice = &(*jConfig)["device"][i];
							break;
						}
						i++;
					}

					jCommands["ID"] = (*jDevice)["id"].asString(); // "52813210",
					jCommands[(*jDevice)["manufacturer"].asString()] = (*jDevice)["model"].asString();
					jCommands[(*jDevice)["type"].asString()] = (*jDevice)["label"].asString();

					int lc = static_cast<int>((*jDevice)["controlGroup"].size());
					for (int i = 0; i < lc; i++)
					{
						std::string cgname = (*jDevice)["controlGroup"][i]["name"].asString();
						int lf = static_cast<int>((*jDevice)["controlGroup"][i]["function"].size());
						for (int j = 0; j < lf; j++)
						{
							Json::Value *jFunction = &(*jDevice)["controlGroup"][i]["function"][j];
							std::string cmd = (*jFunction)["action"].asString();
							size_t offset = cmd.find("\"command");
							if (offset == std::string::npos)
								continue;
							size_t start = cmd.find_first_not_of("\\\":", offset + 8);
							if (start == std::string::npos)
								continue;
							size_t stop = cmd.find_first_of("\\\",}", start);
							if (start == std::string::npos)
								continue;
							jCommands["Functions"][cgname][cmd.substr(start, stop - start)] = (*jFunction)["label"].asString();
						}

					}
					std::string fsName = szCommandParameters[0];
					fsName.append(".json");
					std::ofstream fsCommands(fsName, ios::trunc);
					std::string szCommands = jCommands.toStyledString();
					fsCommands << szCommands;
					fsCommands.close();
					std::cout << szCommands;
					return;
				}
			}
		}
	}

	else
	{
		// we don't want this
		return;
	}

	// std::cout << jResult.toStyledString() << std::endl;
}

int main(int argc, char * argv[])
{
	if (argc < 2)
	{
		return usage(0);
	}

	std::string strHarmonyIP = argv[1];
	Json::Value jAuth;
	Json::Value *jMyAuth;
	std::error_code ec;


	ifstream authFile("remoteID.json");
	if (authFile.is_open())
	{
		std::string line;
		std::string authContent = "";
		while (getline (authFile, line))
		{
			authContent.append(line);
		}
		authFile.close();

		Json::Reader jReader;
		jReader.parse(authContent.c_str(), jAuth);
	}


	// User requested an action to be performed
	if(argc >= 3)
	{
		szCommand = argv[2];

		bQuietMode = (szCommand.compare(szCommand.length()-4,4,"_raw") == 0);
		if (bQuietMode)
		{
			szCommand = szCommand.substr(0, szCommand.length()-4);
		}


		if (szCommand == "turn_off")
		{
			szCommand = "start_activity";
			szCommandParameters[0] = "-1";
		}
		else if (
			(szCommand == "list_activities") ||
			(szCommand == "get_current_activity_id") ||
			(szCommand == "list_devices") ||
			(szCommand == "get_config")
		    )
		{
			// all fine here
		}
		else if (
			(szCommand == "start_activity") ||
			(szCommand == "list_device_commands")
		    )
		{
			// requires a parameter
			if (argc < 4)
				return usage(1);

			szCommandParameters.push_back(argv[3]);
		}
		else if (szCommand == "issue_device_command")
		{
			// requires at least 2 parameters
			if (argc < 5)
				return usage(1);

			szCommandParameters.push_back(argv[3]);
			szCommandParameters.push_back(argv[4]);
			if(argc >= 6)
			{
				for (int i = 5; i < argc; i++)
				{
					szCommandParameters.push_back(argv[i]);
				}
			}
		}
		else
		{
			// unknown command
			return usage(1);
		}
	}



	// create harmonyhub client
	harmonyhubclient hclient;
	hclient.set_message_handler(boost::bind(&handle_reply, boost::placeholders::_1));

 	// Read the connection parameters
	if (jAuth.isMember("HarmonyHub") && jAuth["HarmonyHub"].isArray())
	{
		int i = 0;
		int l = static_cast<int>(jAuth["HarmonyHub"].size());
		while (i < l)
		{
			if (jAuth["HarmonyHub"][i]["IPaddress"].asString() == strHarmonyIP)
			{
				
				hclient.set_hubid(jAuth["HarmonyHub"][i]["remoteID"].asString(), jAuth["HarmonyHub"][i]["domain"].asString());
				break;
			}
			i++;
		}
		jMyAuth = &jAuth["HarmonyHub"][i];
		if (i == l)
			(*jMyAuth)["IPaddress"] = strHarmonyIP;
	}
	else
	{
		jAuth["HarmonyHub"] = Json::Value(Json::arrayValue);
		jMyAuth = &jAuth["HarmonyHub"][0];
		(*jMyAuth)["IPaddress"] = strHarmonyIP;
	}


	myState = harmonyhubcontrol::status::connecting;
	ec = hclient.connect(strHarmonyIP);
	if (ec)
	{
		log("HARMONY COMMUNICATION LOGIN    : FAILURE", bQuietMode);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		return 1;
	}

	int i = 0;
	while (i < 10)
	{
		if (myState == harmonyhubcontrol::status::connected)
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		i++;
	}
	if (i >= 10) // timeout
	{
		log("HARMONY COMMUNICATION LOGIN    : TIMEOUT", bQuietMode);
		return 1;
	}


	log("HARMONY COMMUNICATION LOGIN    : SUCCESS", bQuietMode);

	
	if (!jMyAuth->isMember("remoteID") || (*jMyAuth)["remoteID"].asString().empty())
	{
		(*jMyAuth)["remoteID"] = hclient.get_hubid();
		(*jMyAuth)["domain"] = hclient.get_domain();
		ofstream authFile("remoteID.json", ios::trunc);
		if (authFile.is_open())
		{
			authFile << jAuth.toStyledString();
			authFile.close();
		}
	}


	myState = harmonyhubcontrol::status::waitforanswer;

	if (szCommand.empty() || (szCommand == "get_current_activity_id"))
	{
		ec = hclient.get_current_activity();
	}

	else if (szCommand == "list_activities"      ||
		 szCommand == "list_devices"         ||
		 szCommand == "list_commands"        ||
		 szCommand == "list_device_commands" ||
		 szCommand == "get_config"
	)
	{
		ec = hclient.get_config();
	}

	else if (szCommand == "start_activity")
	{
		ec = hclient.start_activity(szCommandParameters[0]);
	}


	else if (szCommand == "issue_device_command")
	{
		for (size_t i = 1; ((i < szCommandParameters.size()) && !ec); i++)
		{
			ec = hclient.send_device_command(szCommandParameters[0], szCommandParameters[i]);
		}

		if (!ec)
		{
			// issue device command does not return any output
			myState = harmonyhubcontrol::status::connected;
			if (bQuietMode)
			{
				std::cout << "{\n   \"code\":200\n,\n   \"msg\":\"ok\"\n}\n";
			}
		}
	}

	if (ec)
	{
		log("HARMONY COMMAND SUBMISSION     : FAILURE", bQuietMode);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		return 1;
	}


	log("HARMONY COMMAND SUBMISSION     : SUCCESS", bQuietMode);


	// wait for answer
	i = 0;
	while (i < 10)
	{
		if (myState == harmonyhubcontrol::status::connected)
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		i++;
	}
	if (i >= 10) // timeout
		return 1;

	myState = harmonyhubcontrol::status::closing;
	hclient.close();

	i = 0;
	while (i < 10)
	{
		if (myState == harmonyhubcontrol::status::closed)
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		i++;
	}
	if (i >= 10) // timeout
		return 1;

	return 0;
}
