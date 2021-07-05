1. User initiates the lock

2. Locks creates an MQTT connection with IoT Core service. Publishes and subscribes to respective topic.

3. Lambda function is invoked by iot core when lock publishes to respective topic.

4. Lambda function requests Django server to generate OTP to unlock. It then publishes the OTP to lock.

5. User recieves OTP on web app.

6. Lock matches the OTP with the user entered.

7. Lock again pushlishes to lambda function the current lock status. Lambda function updates the MongoDB entries for the lock.
