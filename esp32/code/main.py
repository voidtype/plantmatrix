
import camera

camera.init(0, d0=34, d1=13, d2=26, d3=35, d4=39, d5=38, d6=37, d7=36, format=camera.JPEG, framesize=camera.FRAME_VGA, xclk_freq=camera.XCLK_20MHz, href=27, vsync=5, reset=-1, sioc=23, siod=18, xclk=4, pclk=25)

import urequests
from exceptions import UploadError
from uploader import make_request
from machine import Timer

DEVICE_UUID = "48a6118c-f22e-48b9-b4c6-3bcdd6d8641d"

def savePicture(dUUID=DEVICE_UUID):
    rq = make_request({"device":DEVICE_UUID},camera.capture())
    http_response = urequests.post("http://144.202.8.17:81/upload",headers=rq[1],data=rq[0])
    if http_response.status_code == 204 or http_response.status_code == 200:
        print('Uploaded request')
    else:
        print(http_response.text)
    http_response.close()
    return http_response

savePicture()
tim = Timer(0)
tim.init(period=10000, mode=Timer.PERIODIC, callback=savePicture)
