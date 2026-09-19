import os

from fastapi import FastAPI
import boto3
import json

os.environ["AWS_PROFILE"] = "fastapi-backend"

app = FastAPI()
iot = boto3.client("iot-data")

@app.get("/sensors/latest")
async def get_sensors_latest():
    return {"Hello": "World"}


@app.get("/sensors/history?minutes={mins}")
async def get_sensors_history(mins: int = 30):
    return {"mins": mins}


@app.post("/actuators/led")
async def post_actuators_led():
    iot.publish(
        topic="iot-course/volodya/actuators/led",
        qos=1,
        payload=json.dumps({
            "state": "on"
        })
    )
    return {"Hello": "World"}
