#!./venv/bin/python

import requests

# this is extremely stupid I guess, I will introduce some framework I guess


response = requests.get("http://localhost:8080")

print(f"Got Response: {response}")
