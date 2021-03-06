# HarmonyHubControl
C++ Application to Control a Logitech Harmony Hub 
=========

A C++ executable to control the Logitech Harmony Hub/Link without need of 
the Logitech Harmony App or Remote.

HarmonyHubControl was developed using the pyharmony library as a guide.
It was originally written in C++ using Qt to allow simpler packaging 
than pyharmony and to reduce the number of dependencies.  Subsequently,
the Qt dependency was removed so that it is now only dependent on BSD 
sockets.

The code has been compiled and tested on both Microsoft Windows (using
Visual Studio 2010) and Ubuntu 10.0.  The code has also been cross-compiled
to OpenWRT (backfire) and tested on Vera2 and VeraLite home automation 
controllers.

HarmonyHubControl was developed particularly to ease Harmony Hub/Link 
integration within home automation systems.

Special thanks to jterrace and petele for laying down the groundwork for 
this work to occur by implementing pyharmony.


Functionality
--------------

HarmonyHubControl provides the ability to perform the following functions
without requiring the Logitech Harmony App or Remote.

* Authenticate using Logitech's web service
* Authenticate to the local harmony device.
* Query for the harmony's entire configuration information.
* Request a list of activities and devices from the harmony
* Request the currently selected activity
* Start an activity by ID


Requirements
------------

In order to successfully use the executable, it is expected that the following
are in place:

A Harmony Hub/Link that is pre-configured and working properly on the local network

The IP address of the Harmony is required.



Usage
-----

The command line for HarmonyHubControl is as follows:

    HarmonyHubControl.exe [harmony_ip] [command (optional)] [primary_parameter (optional)] [secondary_parameter (optional)]\n");
    

[harmony_ip] is the IP address or DNS name of the harmony device on your network


[command] can be any of the following:

	get_current_activity_id
	list_devices
	list_activities
	start_activity [ID]
        issue_device_command [DEVICE_ID] [DEVICE_COMMAND]
	get_config


Typical example usage would be as follows:

1) Query the device for a list of activities:

	HarmonyHubControl.exe 192.168.0.XXX list_activities
	
	A list of accessories will be stored in activities.json
	{"PowerOff":"-1"}

2) Start an activity based on the activity identifiers listed in step 1:

	HarmonyHubControl.exe 192.168.0.XXX start_activity
	
	The current activity id is stored in current.json. 
	The current activity is also stored in this file when querying get_current_activity_id
	
        {"current activity":-1}

For full argument information simply run the executablewith no parameters.



Building from Source
--------------------

Building the executable from source requires either Microsoft Visual Studio 2010 or Linux and gcc 
3.4.x or above.

The application has no external dependencies besides the standard libraries provided by a default 
Visual Studio or Linux gcc development environment.

To-do
--------------------

Re-organize the code into a library
