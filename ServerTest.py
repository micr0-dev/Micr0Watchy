import requests

url = 'https://localhost:18724/'

response = requests.get(url, verify=False)

if response.status_code == 200:
  # Success - the response body will contain the data you requested
  data = response.json()
else:
  # Error - the request failed
  data = None

print(data)