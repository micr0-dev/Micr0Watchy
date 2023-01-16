## Work in progress! The server is still very buggy!

![](/Concept/Micr0Watchy.png)
![](/Concept/Watchy.png)

# Micr0Watchy
A back-end, front-end system for Watchy with integration with Spotify and OpenWeather

## What is Micr0Watchy?
Micr0Watchy is a replacement for the ![Watchy by SQFMI](https://github.com/sqfmi/Watchy) library. It is a currently lighter version of Watchy that allows for more advanced watch faces. This is achieved by removing unnecessary functions such as the settings menu and the already non-functional BTE OTA. The default watch face comes with integration with Spotify to show the current playing song and control playback.

## Features
- Integration with Spotify to control music playback and view the currently playing song
- Integration with OpenWeather to display current weather and forecast

## Future Development
Micr0Watchy is a work in progress, and there are many more features and improvements that we plan to add in the future. Some of the planned features include:

- Easily customizable watch faces
- Install script (Auto creation of .env and settings files)
- Customizable watch faces
- WIFI OTA updates
- Additional APIs for more features
- Improved server performance and stability

We are constantly working to improve Micr0Watchy and make it the best it can be. We appreciate any feedback and suggestions from the community, so please feel free to reach out to us with your ideas.

## Installation
Currently, since Micr0Watchy is in the very early stage of development, there is no installation guide.

## Layout
This image is an original plan/sketch of the idea behind Micr0Watchy. The main idea of having a separate back-end server originated due to Spotify's Authorization Code Flow which makes it impossible for the API to be used on the watch. Meanwhile, this might seem like a limitation in a way it may be better since it allows for much more computing power to be at hand in case it is needed. All data that is sent to the watch can be filtered and prepared beforehand.

<img src='https://user-images.githubusercontent.com/26364458/212608119-7944b8f6-bbf1-4d9d-b287-bb81564ebc6e.png' width='400'>

## Technical Details
Micr0Watchy is a back-end, front-end system for watchy that uses several technologies to achieve its functionality.

On the back-end side, Micr0Watchy uses a Python-based server that utilizes the socketserver library to handle HTTP requests from the watch. The server uses the Spotify Web API to control music playback and view the currently playing song. It also uses the OpenWeather API to get the current weather and forecast. The server also uses the JSON library to convert the data received from the APIs into a JSON format that can be easily parsed by the watch.

On the front-end side, Micr0Watchy uses an ESP32 chip as the main processor for the watch. The watch communicates with the back-end server using the HTTPClient library to make GET and POST requests. The watch also uses the ArduinoJson library to parse the JSON data received from the server.

The communication between the watch and the server is done over WiFi. Currently, The watch connects to a pre-defined WiFi network defined in the settings file. The watch uses the IP address of the server to make requests. The server can be hosted on any device that has a Python environment installed, although I use an Ubuntu Server and have created a systemd service for the Micr0Watchy back-end server to make it easier to manage.

Overall, Micr0Watchy utilizes a combination of technologies to achieve its functionality. The back end uses Python and several libraries to handle API calls and data manipulation, while the front end uses an ESP32 chip and several libraries to handle the display and communication with the back-end server.

## Contribution
Micr0Watchy is an open-source project and contributions are welcome. If you find any bugs or have any suggestions for new features, please open an issue or submit a pull request.

## License
Micr0Watchy is licensed under the MIT License. See the LICENSE file for more details.
