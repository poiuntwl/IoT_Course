import json
import os
from datetime import datetime, timezone
from decimal import Decimal
from pathlib import Path

import boto3
from boto3.dynamodb.conditions import Attr
from fastapi import FastAPI, HTTPException, Query
from fastapi.responses import FileResponse
from botocore.exceptions import BotoCoreError, ClientError
from pydantic import BaseModel

os.environ.setdefault("AWS_PROFILE", "fastapi-backend")

AWS_REGION = os.getenv("AWS_REGION", "eu-central-1")
SENSOR_TABLE_NAME = os.getenv("SENSOR_TABLE_NAME", "iot_course_sensor_data")
LED_COMMAND_TOPIC = "iot-course/volodya/commands/led"
CONTROL_PAGE = Path(__file__).with_name("index.html")


class LedCommand(BaseModel):
    state: str


app = FastAPI()
dynamodb = boto3.resource("dynamodb", region_name=AWS_REGION)
sensor_table = dynamodb.Table(SENSOR_TABLE_NAME)
iot = boto3.client("iot-data", region_name=AWS_REGION)


def _json_value(value):
    if isinstance(value, Decimal):
        return int(value) if value % 1 == 0 else float(value)
    if isinstance(value, dict):
        return {key: _json_value(item) for key, item in value.items()}
    if isinstance(value, list):
        return [_json_value(item) for item in value]
    return value


def _sensor_response(item):
    sample_time = int(item["sample_time"])
    device_data = _json_value(item.get("device_data", {}))

    return {
        "timestamp": datetime.fromtimestamp(
            sample_time / 1000, tz=timezone.utc
        ).isoformat(),
        **device_data,
    }


def _scan_sensor_items(min_sample_time=None):
    scan_args = {}
    if min_sample_time is not None:
        scan_args["FilterExpression"] = Attr("sample_time").gte(min_sample_time)

    items = []
    while True:
        response = sensor_table.scan(**scan_args)
        items.extend(response.get("Items", []))

        last_key = response.get("LastEvaluatedKey")
        if not last_key:
            return items

        scan_args["ExclusiveStartKey"] = last_key

@app.get("/")
def get_control_page():
    return FileResponse(CONTROL_PAGE)


@app.get("/sensors/latest")
def get_sensors_latest():
    try:
        items = _scan_sensor_items()
    except ClientError as error:
        raise HTTPException(
            status_code=502,
            detail="Failed to read sensor data from DynamoDB",
        ) from error

    if not items:
        raise HTTPException(status_code=404, detail="No sensor data found")

    latest = max(items, key=lambda item: int(item["sample_time"]))
    return _sensor_response(latest)


@app.get("/sensors/history")
def get_sensors_history(minutes: int = Query(default=30, gt=0)):
    cutoff_ms = int(
        (datetime.now(timezone.utc).timestamp() - minutes * 60) * 1000
    )

    try:
        items = _scan_sensor_items(min_sample_time=cutoff_ms)
    except ClientError as error:
        raise HTTPException(
            status_code=502,
            detail="Failed to read sensor data from DynamoDB",
        ) from error

    items.sort(key=lambda item: int(item["sample_time"]))
    return [_sensor_response(item) for item in items]


@app.post("/actuators/led")
def post_actuators_led(command: LedCommand):
    state = command.state.strip().lower()
    if state not in {"on", "off"}:
        raise HTTPException(
            status_code=400,
            detail="LED state must be 'on' or 'off'",
        )

    payload = json.dumps({"action": "set", "value": state}).encode("utf-8")

    try:
        iot.publish(
            topic=LED_COMMAND_TOPIC,
            qos=1,
            payload=payload,
        )
    except (BotoCoreError, ClientError) as error:
        raise HTTPException(
            status_code=502,
            detail="Failed to publish LED command to AWS IoT",
        ) from error

    return {"status": "published", "state": state}
