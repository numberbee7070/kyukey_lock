from pymongo import MongoClient
import json, requests
import boto3


def sendmsgiot(n, room):
    iot_client = boto3.client("iot-data")
    iot_client.publish(
        topic="iot/device/door/" + room + "/otp",
        qos=0,
        payload='{"key":"' + str(n) + '"}',
    )


def sendsms(phone):
    r = requests.post(
        url="http://kyukey-dev.ap-south-1.elasticbeanstalk.com/otp/",
        data={
            "pass": "otp",
            "no": phone,
        },
    )
    return r.content.decode("ASCII")


def lambda_handler(event, context):
    client = MongoClient(
        "mongodb+srv://subodhk:Subodhverma@kyukey-fnmki.mongodb.net/test?retryWrites=true&w=majority"
    )
    collection = client.KyuKey

    if event["status"] == "locked" and event["action"] == "unlock":
        table = collection.main_checkin
        cust_data = table.find_one({"room_id": event["room"], "checkedout": False})
        n = sendsms(cust_data["customer_contactno"])
        sendmsgiot(n, event["room"])

    elif event["status"] == "unlocked" and event["action"] == "unlock":
        table = collection.main_room
        table.update_one({"room_no": event["room"]}, {"$set": {"lock": "open"}})
    elif event["status"] == "unlocked" and event["action"] == "lock":
        table = collection.main_room
        table.update_one({"room_no": event["room"]}, {"$set": {"lock": "locked"}})
    return True
