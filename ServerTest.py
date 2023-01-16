import requests, dotenv, os

import urllib3
urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

dotenv.load_dotenv()

url = "http://"+os.getenv('HOST_IP')+":18724/"
# url = "https://localhost:18724/"

response = requests.get(url)

if response.status_code == 200:
  # Success - the response body will contain the data you requested
  data = response.json()
else:
  # Error - the request failed
  data = None

response.close()

print(data)

headers = {
    "Content-Type": "application/json"
}

# Set up the JSON payload
payload = {
    "key": os.getenv('HANDLER_KEY'),
    "command": "next"
}

# Send the HTTP POST request
response = requests.post(url, headers=headers, json=payload)

# Check the response status code
if response.status_code == 200:
    print("Successfully skipped song")
else:
    print("Failed to skip song. Error: " + str(response.status_code))

response.close()

# testlen = int(input("Length of GET stress-test: "))

# geterrs = 0

# print("Starting GET request stress-test")

# for i in range(testlen):
#     response = requests.get(url, verify=False)
#     if response.status_code == 200:
#         # Success - the response body will contain the data you requested
#         data = response.json()
#     else:
#         # Error - the request failed
#         data = None

#     if data['spresponsecode'] != 200 and data['wtresponsecode'] != 200:
#         print(data)
#         geterrs+=1

# # posterrs = 0

# # print("Starting POST request stress-test")

# # for i in range(testlen):
# #     # Send the HTTP POST request
# #     response = requests.post(url, headers=headers, json=payload, verify=False)

# #     # Check the response status code
# #     if response.status_code == 200:
# #         pass
# #     else:
# #         print("Failed to skip song. Error: " + str(response.status_code))
# #         posterrs+=1

# print("GET error count: " + str(geterrs) + "/" + str(testlen))#+"\n"+ "POST error count: " + str(posterrs) + "/" + str(testlen))