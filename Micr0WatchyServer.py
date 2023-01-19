# APIs
import spotipy, pyowm
# Networking
import requests, json
from flask import Flask, jsonify, request, redirect
# Other
import time, threading, os, dotenv, signal, sys

# Load .env
dotenv.load_dotenv()

# <ALL USER INFO IS IN THE .env FILE>

# Your OpenWeather API Key
owm = pyowm.OWM(os.getenv('OPENWEATHER_KEY'))
mgr = owm.weather_manager()

city = os.getenv('OPENWEATHER_CITY')

# Your client ID and client secret
client_id = os.getenv('SP_CLIENT_ID')
client_secret = os.getenv('SP_CLIENT_SECRET')

# Your Spotify username
username = os.getenv('SP_USERNAME')

keyPass = os.getenv('KEY_PASSWD')

# Redirect uri for spotify authentication
redirect_uri = "http://"+os.getenv('HOST_IP')+":18723/"

# <ALL USER INFO IS IN THE .env FILE>

# Get path of project directory
path = os.path.dirname(__file__)+"/"

isShutingDown = False
runningServiceCount = 0

# --- Redirect Server for spotify authorization ---

reapp = Flask("Redirect Server")

@reapp.route("/", methods=["GET"])
def handle_redirect():
    if request.method == "GET":
        return jsonify({}), 200

rdhserver = threading.Thread(target=lambda: reapp.run(host="0.0.0.0", port=18723, debug=True, use_reloader=False))

print("Starting Redirect Server, ", end="")
runningServiceCount += 1
rdhserver.start()
print("Handling redirects on port 18723... ", end="")
print("Success!")

# Get an access token and refresh token
scope = "user-read-currently-playing user-read-playback-state user-modify-playback-state"

oauth = spotipy.oauth2.SpotifyOAuth(client_id, client_secret, redirect_uri, scope=scope, username=username)
auth_url = oauth.get_authorize_url()


# Check for cached tokens, if none then ask user to authenticate
try:
    token_info = oauth.get_cached_token()
    if (token_info["expires_at"]-int(time.time()) <= 0):
        raise TypeError
except TypeError:
    # Redirect the user to the authorization URL
    print("Please go to the following URL and grant access to your Spotify account:")
    print(auth_url)
    # Wait for the user to grant access
    try:
        input("Press Enter after granting access:")
    except EOFError:
        pass

# Get the access token and refresh token
access_token = oauth.get_access_token(as_dict=False)
sp = spotipy.Spotify(auth=access_token)

# <--- Info Collection --->

infoDict = {
    "name":"NA",
    "artists":"NA",
    "isPlaying":False,
    "temperature":0,
    "status":"NA",
    "spiresponsecode":503,
    "sptresponsecode":503,
    "wtresponsecode":503
}

# --- Spotify Token Refresh Loop ---
def tokenloop():
    global sp, access_token, runningServiceCount, infoDict
    reftime = 0
    time.sleep(1)
    runningServiceCount+=1
    loopCount = 0
    while not isShutingDown:
        if loopCount >= reftime:
            print("Refreshing Token...", end="")
            try:
                token_info = oauth.get_cached_token()
                reftime = token_info["expires_at"]-int(time.time())-200
                refresh_token = token_info["refresh_token"]
                oauth.refresh_access_token(refresh_token)
                access_token = oauth.get_cached_token()["access_token"]
                sp = spotipy.Spotify(auth=access_token)
            except requests.exceptions.ReadTimeout:
                print("Timed Out...")
                infoDict["sptresponsecode"] = 504
            except spotipy.client.SpotifyException:
                print("SpotifyException, Most likely expired token")
                infoDict["sptresponsecode"] = 502
            except requests.exceptions.ConnectionError:
                print("ConnectionError")
                infoDict["sptresponsecode"] = 503
            except Exception as e:
                infoDict["sptresponsecode"] = 500
                print(e)
            infoDict["sptresponsecode"] = 200
            print(" Success! Next refresh in "+str(reftime)+" seconds.")
            loopCount=0
        
        loopCount+=1
        time.sleep(1)
    print("Shuting Down Token Loop... Success!")
    runningServiceCount-=1

# --- Spotify Loop ---

def spotifyloop():
    global runningServiceCount, infoDict
    runningServiceCount+=1
    while not isShutingDown:
        tmpname = str(infoDict["name"])
        tmpartists = str(infoDict["artists"])
        tmpisPlaying = bool(infoDict["isPlaying"])
        
        try:
            current_song = sp.current_playback()
            infoDict["isPlaying"] = current_song['is_playing']
            infoDict["name"] = str(current_song['item']['name'])
            infoDict["artists"] = str(current_song['item']['artists'][0]['name'])
            for artist in current_song['item']['artists'][1:]:
                infoDict["artists"]+=", "+str(artist['name'])
            infoDict["spiresponsecode"] = 200
        except TypeError:
            infoDict["name"] = tmpname
            infoDict["artists"] = tmpartists
            infoDict["isPlaying"] = tmpisPlaying
        except requests.exceptions.ReadTimeout:
            print("Timed Out...")
            infoDict["spiresponsecode"] = 504
        except spotipy.client.SpotifyException:
            print("SpotifyException, Most likely expired token")
            infoDict["spiresponsecode"] = 502
        except requests.exceptions.ConnectionError:
            print("ConnectionError")
            infoDict["spiresponsecode"] = 503
        except Exception as e:
            infoDict["spiresponsecode"] = 500
            print(e)


        # if (tmpname != infoDict["name"] or tmpartists != infoDict["artists"]):
        #     print("Name: " + infoDict["name"])
        #     print("Artist: " + infoDict["artists"])
        # if (tmpisPlaying != infoDict["isPlaying"]):
        #     print(infoDict["isPlaying"])
        time.sleep(0.5)
    print("Shuting Down Spotify Loop... Success!")
    runningServiceCount-=1

# --- OpenWeather Fetcher Loop ---

def weatherloop():
    global runningServiceCount, infoDict
    runningServiceCount+=1
    loopCount = 60*10
    while not isShutingDown:
        if loopCount >= 60*10:
            print("Getting Weather in "+city+"...", end="")
            try:
                observation = mgr.weather_at_place(city)
                w = observation.weather
                infoDict["temperature"] = round(w.temperature('celsius')['temp'])
                infoDict["status"] = w.detailed_status.title()
                infoDict["wtresponsecode"] = 200
                print(" Success!")
            except Exception as e:
                infoDict["wtresponsecode"] = 500
                print(e)
            loopCount=0

        # # Print the temperature and status
        # print("Temperature: "+str(infoDict["temperature"])+"°C")
        # print("Status: "+str(infoDict["status"]))
        
        loopCount+=1
        time.sleep(1)
    print("Shuting Down Weather Loop... Success!")
    
        
# --- Starting threads ---

# Create a new thread to run the loop
tokenthread = threading.Thread(target=tokenloop)
tokenthread.daemon = True
# Start the thread
tokenthread.start()

sptthread = threading.Thread(target=spotifyloop)
sptthread.daemon = True

sptthread.start()

weatherthread = threading.Thread(target=weatherloop)
weatherthread.daemon = True

weatherthread.start()

# --- RequestHandler Server ---

app = Flask(__name__)

@app.route('/', methods=['GET'])
def handle_get():
    try:
        return jsonify(infoDict), 200
    except Exception as e:
        print(e)
        return jsonify({}), 500

@app.route('/', methods=['POST'])
def handle_post():
    try:
        data = request.get_json()
        if data["key"] != os.getenv('HANDLER_KEY'):
            return jsonify({"error": "Access denied"}), 403

        # Check the value of the "command" field
        if data["command"] == "next":
            sp.next_track()
        elif data["command"] == "prev":
            sp.previous_track()
        elif data["command"] == "pause":
            if infoDict["isPlaying"]:
                sp.pause_playback()
            else:
                sp.start_playback()
        else:
            return
        return jsonify({}), 200
    except Exception as e:
        print(e)

runningServiceCount+=1

rqhserver = threading.Thread(target=lambda: app.run(host="0.0.0.0", port=18724, debug=True, use_reloader=False))
print("Starting Request Handler Server, ", end="")
rqhserver.start()
print("Handling Requests on port 18724... ", end="")
print("Success!")

def sigterm_handler(_signo, _stack_frame):
    global isShutingDown
    global runningServiceCount
    print("Running Service count: " + str(runningServiceCount))
    # Perform cleanup tasks here
    # ...
    # Exit gracefully
    isShutingDown=True
    loopCount = 0
    while runningServiceCount > 2 and loopCount < 3:
        time.sleep(1)
        loopCount += 1
    sys.exit(runningServiceCount)

signal.signal(signal.SIGTERM, sigterm_handler)

# print("Shuting Down Redirect Server... ", end="")
# rdhserver.kill()
# rdhserver.join()
# print("Success!")

while not isShutingDown:
    time.sleep(1)
    pass
# print("Shuting Down Request Handler Server... ", end="")
# rqhserver.kill()
# rqhserver.join()
# print("Success!")