# simple activity monitor

This example is based on the utility client example from websocketpp. It runs a listener
service for messages generated by the Harmony Hub and logs a description of their content
to screen.

## Usage

Run the executable `harmony_monitor`

Command List:

- connect &lt;IP address&gt;
- close [&lt;close code:default=1000&gt;] [&lt;close reason&gt;]
- help: Display this help text
- quit: Exit the program

Optionally you may pass the Hub's IP address directly on the command line for instant connect.


