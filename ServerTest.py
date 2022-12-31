import requests, dotenv, os

dotenv.load_dotenv()

url = "https://"+os.getenv('HOST_IP')+":18724/"
# url = "https://localhost:18724/"

response = requests.get(url, verify=False)

if response.status_code == 200:
  # Success - the response body will contain the data you requested
  data = response.json()
else:
  # Error - the request failed
  data = None

print(data)

headers = {
    "Content-Type": "application/json"
}

# Set up the JSON payload
payload = {
    "command": "next"
}

# Send the HTTP POST request
response = requests.post(url, headers=headers, json=payload, verify=False)

# Check the response status code
if response.status_code == 200:
    print("Successfully skipped song")
else:
    print("Failed to skip song. Error: " + str(response.status_code))
