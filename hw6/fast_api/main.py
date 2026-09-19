import os

from fastapi import FastAPI
import boto3
import json
from datetime import datetime, timezone, timedelta

os.environ["AWS_PROFILE"] = "fastapi-backend"

app = FastAPI()
iot = boto3.client("iot-data")

@app.get("/sensors/latest")
async def get_sensors_latest():
    return {
        "timestamp": str(datetime.now(timezone.utc)),
        "humidity": 73
    }


@app.get("/sensors/history")
async def get_sensors_history(minutes: int = 30):
    return [
    {
        "timestamp": datetime.now(timezone.utc),
        "temperature": 22.8
    },
    {
        "timestamp": datetime.now(timezone.utc) - timedelta(minutes=5),
        "temperature": 22.6
    },
    {
        "timestamp": datetime.now(timezone.utc) - timedelta(minutes=10),
        "temperature": 22.3
    },
    {
        "timestamp": datetime.now(timezone.utc) - timedelta(minutes=15),
        "temperature": 22.1
    },
    {
        "timestamp": datetime.now(timezone.utc) - timedelta(minutes=20),
        "temperature": 21.9
    },
    {
        "timestamp": datetime.now(timezone.utc) - timedelta(minutes=25),
        "temperature": 21.7
    },
    {
        "timestamp": datetime.now(timezone.utc) - timedelta(minutes=30),
        "temperature": 21.5
    }
]


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
