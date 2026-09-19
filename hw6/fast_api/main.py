from fastapi import FastAPI

app = FastAPI()


@app.get("/")
def read_root():
    return {"Hello": "World"}

@app.get("/sensors/latest")
async def get_sensors_latest():
    return {"Hello": "World"}

@app.get("/sensors/history?minutes={mins}")
async def get_sensors_history(mins: int = 30):
    return {"mins": mins}

@app.post("/actuators/led")
async def post_actuators_led():
    return {"Hello": "World"}
