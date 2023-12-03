from roboflow import Roboflow
import requests
from PIL import Image
from io import BytesIO

# Replace with your ESP32-CAM IP address
ESP32_CAM_IP = '192.168.137.195'

# URL to capture image
url = f'http://{ESP32_CAM_IP}/capture'

# Send GET request to capture image
response = requests.get(url)

rf = Roboflow(api_key="MXhlkstuCIX5CAlrjKNH")
project = rf.workspace().project("plant-classification-n0wen")
model = project.version(1).model

if response.status_code == 200:
    # Load the image
    img = Image.open(BytesIO(response.content))

    # Save the image locally
    img.save("captured_image.jpg")

    # Now, use the saved image for prediction
    print(model.predict("captured_image.jpg").json())
    model.predict("captured_image.jpg").save("prediction.jpg")
else:
    print('Failed to capture image')