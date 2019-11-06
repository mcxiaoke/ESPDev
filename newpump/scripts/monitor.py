from datetime import datetime
import time
import logging
import requests
import re
import paho.mqtt.client as mqtt
from config import *

MSG_LIMIT_PER_MIN = 20
MSG_LIMIT_PER_HOUR = 40
MSG_LIMIT_PER_DAY = 400

sendCounter = {}


def get_full_class_name(obj):
    module = obj.__class__.__module__
    if module is None or module == str.__class__.__module__:
        return obj.__class__.__name__
    return module + '.' + obj.__class__.__name__


def get_log_filename():
    dt = datetime.now().strftime("%Y%m")
    return '/tmp/pump-monitor-{}.log'.format(dt)


def logging_config():
    logging.basicConfig(level=logging.INFO,
                        format='[%(asctime)s][%(name)s][%(levelname)s] %(message)s',
                        datefmt='%Y-%m-%d %H:%M:%S',
                        filename=get_log_filename(),
                        filemode='a')
    console = logging.StreamHandler()
    console.setLevel(logging.DEBUG)
    formatter = logging.Formatter(
        '[%(asctime)s][%(name)s][%(levelname)s] %(message)s')
    console.setFormatter(formatter)
    logging.getLogger('').addHandler(console)


logging_config()
logger = logging.getLogger("monitor")


def send_report(sender, msg):
    global sendCounter
    now = datetime.now()
    min_key = now.strftime("Min:%Y-%m-%d %H:%M")
    day_key = now.strftime("Day:%Y-%m-%d")
    min_value = sendCounter.get(min_key, 0)
    day_value = sendCounter.get(day_key, 0)
    if min_value > MSG_LIMIT_PER_MIN:
        logger.warning(
            'Report exceed limits: {} = {}'.format(min_key, min_value))
        return
    if day_value > MSG_LIMIT_PER_DAY:
        logger.warning(
            'Report exceed limits: {} = {}'.format(day_key, day_value))
        return
    sendCounter[min_key] = min_value + 1
    sendCounter[day_key] = day_value + 1
    suffix = now.strftime("%m%d%H%M%S")
    if len(msg) < 32:
        suffix = msg
    try:
        data = {
            "text": "Pump_Status_{}_{}".format(sender, suffix),
            "desp": msg.replace("\n", "  \n"),
        }
        r = requests.post(WX_REPORT_URL, data=data, timeout=5)
        print(r.text[0:64])
        if r.ok:
            logger.info("Send %s report successful", sender)
        else:

            logger.warning("Send %s report failed, status: %s",
                           sender, r.status_code)
    except Exception as e:
        logger.warning("Send %s report failed: %s",
                       sender,  get_full_class_name(e))


def on_message(client, userdata, msg):
    topic = msg.topic
    message = msg.payload.decode('utf8')
    logger.info(topic+" - "+message.replace("\n", " ") +
                " ("+str(msg.qos)+","+str(msg.retain)+")")
    m = re.match(r"^pump/(\S+)/status$", topic)
    if m and m.group(1):
        send_report(m.group(1), message)


def on_connect(client, userdata, flags, rc):
    logger.info("Connected with result: "+mqtt.error_string(rc))
    # client.subscribe("$SYS/#")
    client.subscribe("#")
    client.publish("monitor/pump", "Online", retain=True)


def on_disconnect(client, userdata, rc):
    logger.info("Disconnected with result: "+mqtt.error_string(rc))


def create_client():
    client = mqtt.Client(client_id=MQTT_CLIENT_ID, clean_session=True)
    # client.enable_logger()
    # client.on_log = on_log
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_message = on_message
    client.username_pw_set(MQTT_USER, MQTT_PASS)
    client.will_set("monitor/pump", payload="Offline", retain=True)
    client.connect(MQTT_SERVER, port=MQTT_PORT, keepalive=60)
    return client


if __name__ == "__main__":
    client = create_client()
    logger.info("====================")
    logger.info("Pump monitor start")
    client.loop_forever()
