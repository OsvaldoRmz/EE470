import paho.mqtt.client as mqtt
import mysql.connector

# ========= MQTT CONFIG =========
BROKER_URL = "broker.hivemq.com"
BROKER_PORT = 1883
TOPIC = "testtopic/temp/outTopic/ramirezheo"  #

# ========= DB CONFIG (Hostinger) =========
DB_HOST = "srv1279.hstgr.io"
DB_USER = "u926109375_db_OsvaldoRmz"
DB_PASS = "Osvaldo10010028"
DB_NAME = "u926109375_Osvaldoramirez"

DB_TABLE = "sensor_values"          

def push_value_to_db(sensor_value: float):
    """Insert a single sensor value into the database."""
    try:
        connection = mysql.connector.connect(
            host=DB_HOST,
            user=DB_USER,
            password=DB_PASS,
            database=DB_NAME
        )

        if connection.is_connected():
            cursor = connection.cursor()
            # assumes table has columns: id (auto), value (FLOAT/DOUBLE), created_at (TIMESTAMP default)
            insert_query = f"INSERT INTO {DB_TABLE} (value) VALUES (%s)"
            cursor.execute(insert_query, (sensor_value,))
            connection.commit()
            print(f"[DB] Inserted value {sensor_value}")
    except mysql.connector.Error as err:
        print(f"[DB] Error: {err}")
    finally:
        try:
            if connection.is_connected():
                cursor.close()
                connection.close()
        except NameError:
            pass


# ========= MQTT CALLBACKS =========
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("[MQTT] Connected to broker")
        client.subscribe(TOPIC)
        print(f"[MQTT] Subscribed to topic: {TOPIC}")
    else:
        print(f"[MQTT] Failed to connect, return code {rc}")


def on_message(client, userdata, msg):
    payload_str = msg.payload.decode().strip()
    print(f"[MQTT] Received from {msg.topic}: {payload_str}")

    # Try to convert payload to a number; ignore non-numeric messages
    try:
        value = float(payload_str)
    except ValueError:
        print("[MQTT] Non-numeric payload, ignoring for DB")
        return

    push_value_to_db(value)


def main():
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    print("[MQTT] Connecting to broker...")
    client.connect(BROKER_URL, BROKER_PORT, keepalive=60)

    # This will loop forever, receiving messages and writing to DB
    client.loop_forever()


if __name__ == "__main__":
    main()
